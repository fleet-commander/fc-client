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

#include "fcmdr-extensions.h"
#include "fcmdr-settings-backend.h"

#define FCMDR_PROFILE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_PROFILE, FCmdrProfilePrivate))

struct _FCmdrProfilePrivate {
	JsonParser *json_parser;
	JsonObject *json_object;

	gchar *uid;
	gchar *etag;
	gchar *name;
	gchar *description;

	JsonObject *settings;
	GPtrArray *settings_backends;
};

enum {
	PROP_0,
	PROP_DESCRIPTION,
	PROP_ETAG,
	PROP_NAME,
	PROP_SETTINGS,
	PROP_UID
};

/* Forward Declarations */
static void	fcmdr_profile_initable_interface_init
					(GInitableIface *interface);

G_DEFINE_TYPE_WITH_CODE (
	FCmdrProfile,
	fcmdr_profile,
	G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE (
		G_TYPE_INITABLE,
		fcmdr_profile_initable_interface_init))

static JsonNode *
fcmdr_profile_serialize_json_object (gconstpointer boxed)
{
	JsonNode *json_node;

	json_node = json_node_new (JSON_NODE_OBJECT);
	json_node_set_object (json_node, (JsonObject *) boxed);

	return json_node;
}

static gpointer
fcmdr_profile_deserialize_json_object (JsonNode *json_node)
{
	return json_node_dup_object (json_node);
}

static gboolean
fcmdr_profile_validate (FCmdrProfile *profile,
                        GError **error)
{
	if (profile->priv->uid == NULL || *profile->priv->uid == '\0') {
		g_set_error_literal (
			error, G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Missing required 'uid' member in JSON data");
		return FALSE;
	}

	if (profile->priv->etag == NULL || *profile->priv->etag == '\0') {
		g_set_error_literal (
			error, G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Missing required 'etag' member in JSON data");
		return FALSE;
	}

	if (profile->priv->settings == NULL) {
		g_set_error_literal (
			error, G_IO_ERROR,
			G_IO_ERROR_INVALID_DATA,
			"Missing required 'settings' member in JSON data");
		return FALSE;
	}

	return TRUE;
}

static void
fcmdr_profile_init_settings_backends (FCmdrProfile *profile)
{
	GIOExtensionPoint *extension_point;
	JsonObject *settings;
	GList *list = NULL, *link;

	fcmdr_ensure_extensions_registered ();

	extension_point = g_io_extension_point_lookup (
		FCMDR_SETTINGS_BACKEND_EXTENSION_POINT_NAME);

	settings = fcmdr_profile_ref_settings (profile);

	list = json_object_get_members (settings);

	/* The member names double as extension names. */
	for (link = list; link != NULL; link = g_list_next (link)) {
		GSettingsBackend *backend;
		GIOExtension *extension;
		JsonNode *json_node;
		const gchar *name = link->data;

		extension = g_io_extension_point_get_extension_by_name (
			extension_point, name);
		if (extension == NULL) {
			g_critical ("No extension available for '%s'", name);
			continue;
		}

		json_node = json_object_get_member (settings, name);

		backend = g_object_new (
			g_io_extension_get_type (extension),
			"profile", profile,
			"settings", json_node,
			NULL);

		g_ptr_array_add (profile->priv->settings_backends, backend);
	}

	g_list_free (list);

	json_object_unref (settings);
}

static void
fcmdr_profile_set_description (FCmdrProfile *profile,
                               const gchar *description)
{
	g_return_if_fail (profile->priv->description == NULL);

	profile->priv->description = g_strdup (description);
}

static void
fcmdr_profile_set_etag (FCmdrProfile *profile,
                        const gchar *etag)
{
	g_return_if_fail (profile->priv->etag == NULL);

	profile->priv->etag = g_strdup (etag);
}

static void
fcmdr_profile_set_name (FCmdrProfile *profile,
                        const gchar *name)
{
	g_return_if_fail (profile->priv->name == NULL);

	profile->priv->name = g_strdup (name);
}

static void
fcmdr_profile_set_settings (FCmdrProfile *profile,
                            JsonObject *settings)
{
	g_return_if_fail (settings != NULL);
	g_return_if_fail (profile->priv->settings == NULL);

	profile->priv->settings = json_object_ref (settings);
}

static void
fcmdr_profile_set_uid (FCmdrProfile *profile,
                       const gchar *uid)
{
	g_return_if_fail (profile->priv->uid == NULL);

	profile->priv->uid = g_strdup (uid);
}

static void
fcmdr_profile_set_property (GObject *object,
                            guint property_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_DESCRIPTION:
			fcmdr_profile_set_description (
				FCMDR_PROFILE (object),
				g_value_get_string (value));
			return;

		case PROP_ETAG:
			fcmdr_profile_set_etag (
				FCMDR_PROFILE (object),
				g_value_get_string (value));
			return;

		case PROP_NAME:
			fcmdr_profile_set_name (
				FCMDR_PROFILE (object),
				g_value_get_string (value));
			return;

		case PROP_SETTINGS:
			fcmdr_profile_set_settings (
				FCMDR_PROFILE (object),
				g_value_get_boxed (value));
			return;

		case PROP_UID:
			fcmdr_profile_set_uid (
				FCMDR_PROFILE (object),
				g_value_get_string (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_profile_get_property (GObject *object,
                            guint property_id,
                            GValue *value,
                            GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_DESCRIPTION:
			g_value_set_string (
				value,
				fcmdr_profile_get_description (
				FCMDR_PROFILE (object)));
			return;

		case PROP_ETAG:
			g_value_set_string (
				value,
				fcmdr_profile_get_etag (
				FCMDR_PROFILE (object)));
			return;

		case PROP_NAME:
			g_value_set_string (
				value,
				fcmdr_profile_get_name (
				FCMDR_PROFILE (object)));
			return;

		case PROP_SETTINGS:
			g_value_take_boxed (
				value,
				fcmdr_profile_ref_settings (
				FCMDR_PROFILE (object)));
			return;

		case PROP_UID:
			g_value_set_string (
				value,
				fcmdr_profile_get_uid (
				FCMDR_PROFILE (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_profile_dispose (GObject *object)
{
	FCmdrProfilePrivate *priv;

	priv = FCMDR_PROFILE_GET_PRIVATE (object);

	g_clear_object (&priv->json_parser);

	if (priv->json_object != NULL) {
		json_object_unref (priv->json_object);
		priv->json_object = NULL;
	}

	if (priv->settings != NULL) {
		json_object_unref (priv->settings);
		priv->settings = NULL;
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

	g_free (priv->uid);
	g_free (priv->etag);
	g_free (priv->name);
	g_free (priv->description);

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

	profile = FCMDR_PROFILE (initable);

	if (!fcmdr_profile_validate (profile, error))
		return FALSE;

	fcmdr_profile_init_settings_backends (profile);

	return TRUE;
}

static void
fcmdr_profile_class_init (FCmdrProfileClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (FCmdrProfilePrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = fcmdr_profile_set_property;
	object_class->get_property = fcmdr_profile_get_property;
	object_class->dispose = fcmdr_profile_dispose;
	object_class->finalize = fcmdr_profile_finalize;

	g_object_class_install_property (
		object_class,
		PROP_DESCRIPTION,
		g_param_spec_string (
			"description",
			"Description",
			"A brief description of the profile",
			NULL,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (
		object_class,
		PROP_ETAG,
		g_param_spec_string (
			"etag",
			"ETag",
			"The profile's entity tag",
			NULL,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (
		object_class,
		PROP_NAME,
		g_param_spec_string (
			"name",
			"Name",
			"The profile's display name",
			NULL,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (
		object_class,
		PROP_SETTINGS,
		g_param_spec_boxed (
			"settings",
			"Settings",
			"Raw settings data in JSON format",
			JSON_TYPE_OBJECT,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (
		object_class,
		PROP_UID,
		g_param_spec_string (
			"uid",
			"UID",
			"The profile's unique ID",
			NULL,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));

	/* XXX json-glib does not know how to serialize or deserialize
	 *     its own boxed types, so we need to teach it how for the
	 *     "settings" property.
	 *
	 *     Because the "settings" property is construct-only,
	 *     json-glib cannot use the JsonSerializable interface for
	 *     technical reasons.  So instead register custom serialize
	 *     and deserialize functions. */

	json_boxed_register_serialize_func (
		JSON_TYPE_OBJECT, JSON_NODE_OBJECT,
		fcmdr_profile_serialize_json_object);

	json_boxed_register_deserialize_func (
		JSON_TYPE_OBJECT, JSON_NODE_OBJECT,
		fcmdr_profile_deserialize_json_object);
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
fcmdr_profile_new (const gchar *data,
                   gssize length,
                   GError **error)
{
	FCmdrProfile *profile;

	g_return_val_if_fail (data != NULL, NULL);

	profile = (FCmdrProfile *) json_gobject_from_data (
		FCMDR_TYPE_PROFILE, data, length, error);

	if (profile == NULL)
		return NULL;

	if (!g_initable_init (G_INITABLE (profile), NULL, error))
		g_clear_object (&profile);

	return profile;
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
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	return profile->priv->uid;
}

const gchar *
fcmdr_profile_get_etag (FCmdrProfile *profile)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	return profile->priv->etag;
}

const gchar *
fcmdr_profile_get_name (FCmdrProfile *profile)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	return profile->priv->name;
}

const gchar *
fcmdr_profile_get_description (FCmdrProfile *profile)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	return profile->priv->description;
}

JsonObject *
fcmdr_profile_ref_settings (FCmdrProfile *profile)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	return json_object_ref (profile->priv->settings);
}

