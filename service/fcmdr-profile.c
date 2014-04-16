/*
 * Copyright (C) 2014 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Matthew Barnes <mbarnes@redhat.com>
 */

#include "config.h"

#include "fcmdr-profile.h"

#include <json-glib/json-glib.h>

#include "fcmdr-extensions.h"
#include "fcmdr-settings-backend.h"

#define FCMDR_PROFILE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_PROFILE, FCmdrProfilePrivate))

struct _FCmdrProfilePrivate {
	JsonParser *json_parser;
	JsonObject *json_object;

	GPtrArray *settings_backends;

	/* This is only set during instance initialization. */
	GInputStream *temporary_stream;
};

/* Forward Declarations */
static void	fcmdr_profile_initable_interface_init
						(GInitableIface *interface);

/* By default, the GAsyncInitable interface calls GInitable.init()
 * from a separate thread, so we only have to override GInitable. */
G_DEFINE_TYPE_WITH_CODE (
	FCmdrProfile,
	fcmdr_profile,
	G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE (
		G_TYPE_INITABLE,
		fcmdr_profile_initable_interface_init)
	G_IMPLEMENT_INTERFACE (
		G_TYPE_ASYNC_INITABLE, NULL))

/* XXX json-glib keeps this private for some reason. */
static const gchar *
fcmdr_json_node_type_get_name (JsonNodeType node_type)
{
	switch (node_type) {
		case JSON_NODE_OBJECT:
			return "JsonObject";
		case JSON_NODE_ARRAY:
			return "JsonArray";
		case JSON_NODE_NULL:
			return "NULL";
		case JSON_NODE_VALUE:
			return "Value";
		default:
			g_assert_not_reached ();
			break;
	}

	return "unknown";
}

static gboolean
fcmdr_profile_validate_member (JsonObject *json_object,
                               const gchar *member_name,
                               JsonNodeType expected_type,
                               gboolean required_member,
                               GError **error)
{
	JsonNode *json_node;

	json_node = json_object_get_member (json_object, member_name);

	if (json_node == NULL) {
		if (required_member) {
			g_set_error (
				error, G_IO_ERROR,
				G_IO_ERROR_INVALID_DATA,
				"Missing required '%s' member in JSON data",
				member_name);
			return FALSE;
		}

		return TRUE;
	}

	if (!JSON_NODE_HOLDS (json_node, expected_type)) {
		g_set_error (
			error, G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Member '%s' holds %s instead of %s",
			member_name,
			json_node_type_name (json_node),
			fcmdr_json_node_type_get_name (expected_type));
		return FALSE;
	}

	return TRUE;
}

static gboolean
fcmdr_profile_validate (FCmdrProfile *profile,
                        GError **error)
{
	JsonNode *json_node;
	JsonObject *json_object;
	gboolean members_valid;

	json_node = json_parser_get_root (profile->priv->json_parser);

	if (!JSON_NODE_HOLDS_OBJECT (json_node)) {
		g_set_error (
			error, G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Root JSON node holds %s instead of %s",
			json_node_type_name (json_node),
			fcmdr_json_node_type_get_name (JSON_NODE_OBJECT));
		return FALSE;
	}

	/* The top-level JSON object is unnamed.
	 * We want the embedded "profile" object. */
	json_object = json_node_get_object (json_node);

	members_valid =
		fcmdr_profile_validate_member (
		json_object, "profile", JSON_NODE_OBJECT, TRUE, error);

	if (!members_valid)
		return FALSE;

	json_object = json_object_get_object_member (json_object, "profile");

	/* Stash the "profile" object for easy access later. */
	profile->priv->json_object = json_object_ref (json_object);

	/* XXX The required members are a guess.
	 *     Alberto and I need to agree on this. */

	members_valid =
		fcmdr_profile_validate_member (
		json_object, "uid", JSON_NODE_VALUE, TRUE, error) &&
		fcmdr_profile_validate_member (
		json_object, "etag", JSON_NODE_VALUE, TRUE, error) &&
		fcmdr_profile_validate_member (
		json_object, "name", JSON_NODE_VALUE, FALSE, error) &&
		fcmdr_profile_validate_member (
		json_object, "description", JSON_NODE_VALUE, FALSE, error) &&
		fcmdr_profile_validate_member (
		json_object, "applies-to", JSON_NODE_OBJECT, TRUE, error) &&
		fcmdr_profile_validate_member (
		json_object, "resources", JSON_NODE_ARRAY, FALSE, error) &&
		fcmdr_profile_validate_member (
		json_object, "settings", JSON_NODE_OBJECT, TRUE, error);

	return members_valid;
}

static void
fcmdr_profile_init_settings_backends (FCmdrProfile *profile)
{
	GIOExtensionPoint *extension_point;
	JsonObject *json_object;
	GList *list = NULL, *link;

	fcmdr_ensure_extensions_registered ();

	extension_point = g_io_extension_point_lookup (
		FCMDR_SETTINGS_BACKEND_EXTENSION_POINT_NAME);

	json_object = json_object_get_object_member (
		profile->priv->json_object, "settings");

	if (json_object != NULL)
		list = json_object_get_members (json_object);

	/* The member names double as extension names. */
	for (link = list; link != NULL; link = g_list_next (link)) {
		GSettingsBackend *backend;
		GIOExtension *extension;
		JsonNode *settings;
		const gchar *name = link->data;

		extension = g_io_extension_point_get_extension_by_name (
			extension_point, name);
		if (extension == NULL) {
			g_critical ("No extension available for '%s'", name);
			continue;
		}

		settings = json_object_get_member (json_object, name);

		backend = g_object_new (
			g_io_extension_get_type (extension),
			"profile", profile,
			"settings", settings,
			NULL);

		g_ptr_array_add (profile->priv->settings_backends, backend);
	}

	g_list_free (list);
}

static void
fcmdr_profile_dispose (GObject *object)
{
	FCmdrProfilePrivate *priv;

	priv = FCMDR_PROFILE_GET_PRIVATE (object);

	g_warn_if_fail (priv->temporary_stream == NULL);

	g_clear_object (&priv->json_parser);

	if (priv->json_object != NULL) {
		json_object_unref (priv->json_object);
		priv->json_object = NULL;
	}

	g_ptr_array_set_size (priv->settings_backends, 0);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (fcmdr_profile_parent_class)->dispose (object);
}

static void
fcmdr_profile_finalize (GObject *object)
{
	FCmdrProfilePrivate *priv;

	priv = FCMDR_PROFILE_GET_PRIVATE (object);

	g_ptr_array_free (priv->settings_backends, TRUE);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (fcmdr_profile_parent_class)->finalize (object);
}

static gboolean
fcmdr_profile_initable_init (GInitable *initable,
                             GCancellable *cancellable,
                             GError **error)
{
	FCmdrProfile *profile;
	gboolean success;

	profile = FCMDR_PROFILE (initable);

	success = json_parser_load_from_stream (
		profile->priv->json_parser,
		profile->priv->temporary_stream,
		cancellable, error);

	if (success)
		success = fcmdr_profile_validate (profile, error);

	if (success)
		fcmdr_profile_init_settings_backends (profile);

	g_clear_object (&profile->priv->temporary_stream);

	return success;
}

static void
fcmdr_profile_class_init (FCmdrProfileClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (FCmdrProfilePrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->dispose = fcmdr_profile_dispose;
	object_class->finalize = fcmdr_profile_finalize;
}

static void
fcmdr_profile_initable_interface_init (GInitableIface *interface)
{
	interface->init = fcmdr_profile_initable_init;
}

static void
fcmdr_profile_init (FCmdrProfile *profile)
{
	profile->priv = FCMDR_PROFILE_GET_PRIVATE (profile);

	profile->priv->json_parser = json_parser_new ();

	profile->priv->settings_backends =
		g_ptr_array_new_with_free_func (g_object_unref);
}

FCmdrProfile *
fcmdr_profile_load_sync (GInputStream *stream,
                         GCancellable *cancellable,
                         GError **error)
{
	FCmdrProfile *profile;

	g_return_val_if_fail (G_IS_INPUT_STREAM (stream), NULL);

	profile = g_object_new (FCMDR_TYPE_PROFILE, NULL);

	/* Stash the stream until the profile is fully initialized.
	 * Don't want a permanent property for the stream since we
	 * just consume it and discard it. */
	profile->priv->temporary_stream = g_object_ref (stream);

	if (!g_initable_init (G_INITABLE (profile), cancellable, error))
		g_clear_object (&profile);

	return profile;
}

/* Helper for fcmdr_profile_load() */
static void
fcmdr_profile_init_cb (GObject *source_object,
                       GAsyncResult *result,
                       gpointer user_data)
{
	GTask *task = user_data;
	GError *local_error = NULL;

	g_async_initable_init_finish (
		G_ASYNC_INITABLE (source_object), result, &local_error);

	if (local_error == NULL) {
		g_task_return_pointer (
			task, g_object_ref (source_object),
			(GDestroyNotify) g_object_unref);
	} else {
		g_task_return_error (task, local_error);
	}

	g_object_unref (task);
}

void
fcmdr_profile_load (GInputStream *stream,
                    GCancellable *cancellable,
                    GAsyncReadyCallback callback,
                    gpointer user_data)
{
	FCmdrProfile *profile;
	GTask *task;

	g_return_if_fail (G_IS_INPUT_STREAM (stream));

	task = g_task_new (NULL, cancellable, callback, user_data);

	profile = g_object_new (FCMDR_TYPE_PROFILE, NULL);

	/* Stash the stream until the profile is fully initialized.
	 * Don't want a permanent property for the stream since we
	 * just consume it and discard it. */
	profile->priv->temporary_stream = g_object_ref (stream);

	g_async_initable_init_async (
		G_ASYNC_INITABLE (profile),
		G_PRIORITY_DEFAULT, cancellable,
		fcmdr_profile_init_cb,
		g_object_ref (task));

	g_object_unref (profile);
	g_object_unref (task);
}

FCmdrProfile *
fcmdr_profile_load_finish (GAsyncResult *result,
                           GError **error)
{
	g_return_val_if_fail (g_task_is_valid (result, NULL), NULL);

	return g_task_propagate_pointer (G_TASK (result), error);
}

guint
fcmdr_profile_hash (FCmdrProfile *profile)
{
	const gchar *uid;

	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), 0);

	uid = fcmdr_profile_get_uid (profile);

	return g_str_hash (uid);
}

gboolean
fcmdr_profile_equal (FCmdrProfile *profile1,
                     FCmdrProfile *profile2)
{
	const gchar *uid1, *uid2;

	g_return_val_if_fail (FCMDR_IS_PROFILE (profile1), FALSE);
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile2), FALSE);

	if (profile1 == profile2)
		return TRUE;

	uid1 = fcmdr_profile_get_uid (profile1);
	uid2 = fcmdr_profile_get_uid (profile2);

	return g_str_equal (uid1, uid2);
}

const gchar *
fcmdr_profile_get_uid (FCmdrProfile *profile)
{
	JsonObject *json_object;

	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	json_object = profile->priv->json_object;

	return json_object_get_string_member (json_object, "uid");
}

gchar *
fcmdr_profile_to_data (FCmdrProfile *profile,
                       gboolean pretty_print,
                       gsize *length)
{
	JsonGenerator *json_generator;
	JsonNode *json_node;
	gchar *data;

	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	json_node = json_parser_get_root (profile->priv->json_parser);

	json_generator = json_generator_new ();
	json_generator_set_root (json_generator, json_node);
	json_generator_set_pretty (json_generator, pretty_print);

	data = json_generator_to_data (json_generator, length);

	g_object_unref (json_generator);

	return data;
}

