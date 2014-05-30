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

/**
 * SECTION: fcmdr-profile
 * @short_description: A settings profile
 *
 * A #FCmdrProfile instance holds settings to be locked for some combination
 * of users, groups, and hosts.  Profiles are typically loaded from a remote
 * source as described by a #FCmdrProfileSource.
 *
 * A #FCmdrProfile consists of a unique identifier or #FCmdrProfile:uid
 * field, an entity tag or #FCmdrProfile:etag field for fast comparisons,
 * human-readable #FCmdrProfile:name and #FCmdrProfile:description fields,
 * and an assortment of #FCmdrProfile:settings grouped by context.
 *
 * Each #FCmdrProfile:settings context in a profile is handled by a custom
 * #FCmdrServiceBackend.  For example the "org.gnome.gsettings" context is
 * handled by a custom #FCmdrServiceBackend that knows how to parse and
 * apply those settings by interacting with
 * <ulink url="https://wiki.gnome.org/Projects/dconf">dconf</ulink>.
 **/

#include "config.h"

#include "fcmdr-profile.h"

#include <grp.h>
#include <pwd.h>
#include <unistd.h>

#include "fcmdr-extensions.h"
#include "fcmdr-utils.h"

#define FCMDR_PROFILE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_PROFILE, FCmdrProfilePrivate))

struct _FCmdrProfilePrivate {
	gchar *uid;
	gchar *etag;
	gchar *name;
	gchar *description;

	FCmdrProfileApplies *applies_to;
	JsonObject *settings;

	GWeakRef source;
};

enum {
	PROP_0,
	PROP_APPLIES_TO,
	PROP_DESCRIPTION,
	PROP_ETAG,
	PROP_NAME,
	PROP_SETTINGS,
	PROP_SOURCE,
	PROP_UID
};

/* Forward Declarations */
static void	fcmdr_profile_initable_interface_init
					(GInitableIface *interface);
static void	fcmdr_profile_serializable_interface_init
					(JsonSerializableIface *interface);

G_DEFINE_TYPE_WITH_CODE (
	FCmdrProfile,
	fcmdr_profile,
	G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE (
		G_TYPE_INITABLE,
		fcmdr_profile_initable_interface_init)
	G_IMPLEMENT_INTERFACE (
		JSON_TYPE_SERIALIZABLE,
		fcmdr_profile_serializable_interface_init))

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

static JsonNode *
fcmdr_profile_serialize_profile_applies (gconstpointer boxed)
{
	const FCmdrProfileApplies *applies = boxed;
	JsonNode *json_node;
	JsonObject *json_object;

	g_return_val_if_fail (applies != NULL, NULL);

	json_object = json_object_new ();

	if (applies->users != NULL) {
		/* This takes ownership of the JsonArray. */
		json_object_set_array_member (
			json_object, "users",
			fcmdr_strv_to_json_array (applies->users));
	}

	if (applies->groups != NULL) {
		/* This takes ownership of the JsonArray. */
		json_object_set_array_member (
			json_object, "groups",
			fcmdr_strv_to_json_array (applies->groups));
	}

	if (applies->hosts != NULL) {
		/* This takes ownership of the JsonArray. */
		json_object_set_array_member (
			json_object, "hosts",
			fcmdr_strv_to_json_array (applies->hosts));
	}

	json_node = json_node_alloc ();
	json_node_init_object (json_node, json_object);

	json_object_unref (json_object);

	return json_node;
}

static gpointer
fcmdr_profile_deserialize_profile_applies (JsonNode *json_node)
{
	FCmdrProfileApplies *applies;
	JsonObject *json_object;
	JsonArray *json_array;

	applies = fcmdr_profile_applies_new ();
	json_object = json_node_get_object (json_node);

	json_array = json_object_get_array_member (json_object, "users");
	if (json_array != NULL)
		applies->users = fcmdr_json_array_to_strv (json_array);

	json_array = json_object_get_array_member (json_object, "groups");
	if (json_array != NULL)
		applies->groups = fcmdr_json_array_to_strv (json_array);

	json_array = json_object_get_array_member (json_object, "hosts");
	if (json_array != NULL)
		applies->hosts = fcmdr_json_array_to_strv (json_array);

	return applies;
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
fcmdr_profile_set_applies_to (FCmdrProfile *profile,
                              FCmdrProfileApplies *applies_to)
{
	g_return_if_fail (profile->priv->applies_to == NULL);

	if (applies_to != NULL) {
		profile->priv->applies_to =
			fcmdr_profile_applies_copy (applies_to);
	}
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
		case PROP_APPLIES_TO:
			fcmdr_profile_set_applies_to (
				FCMDR_PROFILE (object),
				g_value_get_boxed (value));
			return;

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

		case PROP_SOURCE:
			fcmdr_profile_set_source (
				FCMDR_PROFILE (object),
				g_value_get_object (value));
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
		case PROP_APPLIES_TO:
			g_value_set_boxed (
				value,
				fcmdr_profile_get_applies_to (
				FCMDR_PROFILE (object)));
			return;

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

		case PROP_SOURCE:
			g_value_take_object (
				value,
				fcmdr_profile_ref_source (
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

	if (priv->settings != NULL) {
		json_object_unref (priv->settings);
		priv->settings = NULL;
	}

	g_weak_ref_set (&priv->source, NULL);

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

	fcmdr_profile_applies_free (priv->applies_to);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (fcmdr_profile_parent_class)->finalize (object);
}

static gboolean
fcmdr_profile_initable_init (GInitable *initable,
                             GCancellable *cancellable,
                             GError **error)
{
	return fcmdr_profile_validate (FCMDR_PROFILE (initable), error);
}

static JsonNode *
fcmdr_profile_serialize_property (JsonSerializable *serializable,
                                  const gchar *property_name,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
	JsonNode *node = NULL;

	if (g_str_equal (property_name, "source")) {
		FCmdrProfileSource *source;

		source = g_value_get_object (value);

		if (source != NULL) {
			SoupURI *uri;
			gchar *uri_string;

			uri = fcmdr_profile_source_dup_uri (source);
			uri_string = soup_uri_to_string (uri, FALSE);

			node = json_node_alloc ();
			json_node_init_string (node, uri_string);

			soup_uri_free (uri);
			g_free (uri_string);
		}

	} else {
		node = json_serializable_default_serialize_property (
			serializable, property_name, value, pspec);
	}

	return node;
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
		PROP_APPLIES_TO,
		g_param_spec_boxed (
			"applies-to",
			"Applies To",
			"Set of users, groups and hosts "
			"to which the profile applies",
			FCMDR_TYPE_PROFILE_APPLIES,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));

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
		PROP_SOURCE,
		g_param_spec_object (
			"source",
			"Source",
			"The source of this profile",
			FCMDR_TYPE_PROFILE_SOURCE,
			G_PARAM_READWRITE |
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

	/* XXX Because the "applies-to" property is construct-only,
	 *     json-glib cannot use the JsonSerializable interface for
	 *     technical reasons.  So instead register custom serialize
	 *     and deserialize functions. */

	json_boxed_register_serialize_func (
		FCMDR_TYPE_PROFILE_APPLIES, JSON_NODE_OBJECT,
		fcmdr_profile_serialize_profile_applies);

	json_boxed_register_deserialize_func (
		FCMDR_TYPE_PROFILE_APPLIES, JSON_NODE_OBJECT,
		fcmdr_profile_deserialize_profile_applies);
}

static void
fcmdr_profile_initable_interface_init (GInitableIface *interface)
{
	interface->init = fcmdr_profile_initable_init;
}

static void
fcmdr_profile_serializable_interface_init (JsonSerializableIface *interface)
{
	interface->serialize_property = fcmdr_profile_serialize_property;
}

static void
fcmdr_profile_init (FCmdrProfile *profile)
{
	profile->priv = FCMDR_PROFILE_GET_PRIVATE (profile);
}

/**
 * fcmdr_profile_new:
 * @data: JSON data describing a profile
 * @length: length of @data, or -1 if it is NUL-terminated
 * @error: return location for a #GError, or %NULL
 *
 * Deserializes the JSON @data into a new #FCmdrProfile instance and checks
 * the contents for validity.  If an error occurs during deserialization or
 * validation, the function sets @error and returns %NULL.
 *
 * A validated profile is guaranteed to have non-empty #FCmdrProfile:uid,
 * #FCmdrProfile:etag, and #FCmdrProfile:settings values.
 *
 * Returns: a new #FCmdrProfile, or %NULL
 **/
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

/**
 * fcmdr_profile_new_from_node:
 * @node: a #JsonNode of type #JSON_NODE_OBJECT describing a profile
 * @error: return location for a #GError, or %NULL
 *
 * Deserializes @node into a new #FCmdrProfile instance and checks the
 * contents for validity.  If an error occurs during deserialization or
 * validation, the function sets @error and returns %NULL.
 *
 * A validated profile is guaranteed to have non-empty #FCmdrProfile:uid,
 * #FCmdrProfile:etag, and #FCmdrProfile:settings values.
 *
 * Returns: a new #FCmdrProfile, or %NULL
 **/
FCmdrProfile *
fcmdr_profile_new_from_node (JsonNode *node,
                             GError **error)
{
	FCmdrProfile *profile;

	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (JSON_NODE_HOLDS_OBJECT (node), NULL);

	profile = (FCmdrProfile *) json_gobject_deserialize (
		FCMDR_TYPE_PROFILE, node);

	if (!g_initable_init (G_INITABLE (profile), NULL, error))
		g_clear_object (&profile);

	return profile;
}

/**
 * fcmdr_profile_new_from_stream:
 * @stream: an open #GInputStream
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Deserializes the @stream contents into a new #FCmdrProfile instance
 * and checks the contents for validity.  If an error occurs during
 * deserialization or validation, the function sets @error and returns %NULL.
 *
 * A validated profile is guaranteed to have non-empty #FCmdrProfile:uid,
 * #FCmdrProfile:etag, and #FCmdrProfile:settings values.
 *
 * Returns: a new #FCmdrProfile, or %NULL
 **/
FCmdrProfile *
fcmdr_profile_new_from_stream (GInputStream *stream,
                               GCancellable *cancellable,
                               GError **error)
{
	FCmdrProfile *profile = NULL;
	GOutputStream *buffer;
	gssize length;

	g_return_val_if_fail (G_IS_INPUT_STREAM (stream), NULL);

	buffer = g_memory_output_stream_new_resizable ();

	length = g_output_stream_splice (
		buffer, stream,
		G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
		NULL, error);

	if (length >= 0) {
		const gchar *data;

		data = g_memory_output_stream_get_data (
			G_MEMORY_OUTPUT_STREAM (buffer));

		if (data != NULL) {
			profile = fcmdr_profile_new (data, length, error);
		} else {
			g_set_error_literal (
				error, G_IO_ERROR,
				G_IO_ERROR_INVALID_DATA,
				"Empty input stream");
		}
	}

	g_object_unref (buffer);

	return profile;
}

/**
 * fcmdr_profile_hash:
 * @profile: a #FCmdrProfile
 *
 * Generates a hash value for @profile.  This function is intended for
 * easily hashing a #FCmdrProfile to add to a #GHashTable or similar data
 * structure.
 *
 * Returns: a hash value for @profile
 **/
guint
fcmdr_profile_hash (FCmdrProfile *profile)
{
	const gchar *uid;

	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), 0);

	uid = fcmdr_profile_get_uid (profile);

	return g_str_hash (uid);
}

/**
 * fcmdr_profile_equal:
 * @profile1: the first #FCmdrProfile
 * @profile2: the second #FCmdrProfile
 *
 * Checks two #FCmdrProfile instances for equality.  Two #FCmdrProfile
 * instances are equal if their #FCmdrProfile:uid values are equal.
 *
 * Returns: %TRUE if @profile1 and @profile2 are equal
 **/
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

/**
 * fcmdr_profile_get_uid:
 * @profile: a #FCmdrProfile
 *
 * Returns the @profile's unique identifier.
 *
 * Returns: the @profile's #FCmdrProfile:uid
 **/
const gchar *
fcmdr_profile_get_uid (FCmdrProfile *profile)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	return profile->priv->uid;
}

/**
 * fcmdr_profile_get_etag:
 * @profile: a #FCmdrProfile
 *
 * Returns the @profile's entity tag.
 *
 * Returns: the @profile's #FCmdrProfile:etag
 **/
const gchar *
fcmdr_profile_get_etag (FCmdrProfile *profile)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	return profile->priv->etag;
}

/**
 * fcmdr_profile_get_name:
 * @profile: a #FCmdrProfile
 *
 * Returns the @profile's display name.
 *
 * Returns: the @profile's #FCmdrProfile:name
 **/
const gchar *
fcmdr_profile_get_name (FCmdrProfile *profile)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	return profile->priv->name;
}

/**
 * fcmdr_profile_get_description:
 * @profile: a #FCmdrProfile
 *
 * Returns a brief description of the @profile.
 *
 * Returns: the profile's #FCmdrProfile:description
 **/
const gchar *
fcmdr_profile_get_description (FCmdrProfile *profile)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	return profile->priv->description;
}

/**
 * fcmdr_profile_get_applies_to:
 * @profile: a #FCmdrProfile
 *
 * Returns the #FCmdrProfileApplies describing which users, groups and/or
 * hosts @profile applies to.  A %NULL return value indicates the @profile
 * applies unconditionally.
 *
 * The returned #FCmdrProfileApplies is owned by @profile and should not be
 * modified or freed.
 *
 * Returns: a #FCmdrProfileApplies, or %NULL
 **/
FCmdrProfileApplies *
fcmdr_profile_get_applies_to (FCmdrProfile *profile)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	return profile->priv->applies_to;
}

/**
 * fcmdr_profile_ref_settings:
 * @profile: a #FCmdrProfile
 *
 * Returns the raw settings data as a JSON object, grouped by context with
 * member names denoting the context.
 *
 * The returned #JsonObject is referenced for thread-safety and must be
 * unreferenced with json_object_unref() when finished with it.
 *
 * Returns: a referenced #JsonObject
 **/
JsonObject *
fcmdr_profile_ref_settings (FCmdrProfile *profile)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	return json_object_ref (profile->priv->settings);
}

/**
 * fcmdr_profile_ref_source:
 * @profile: a #FCmdrProfile
 *
 * Returns the #FCmdrProfileSource from which the @profile originated, or
 * %NULL if the @profile was added interactively or by some other means.
 *
 * The returned #FCmdrProfileSource is referenced for thread-safety and must
 * be unreferenced with g_object_unref() when finished with it.
 *
 * Returns: a referenced #FCmdrProfileSource, or %NULL
 **/
FCmdrProfileSource *
fcmdr_profile_ref_source (FCmdrProfile *profile)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	return g_weak_ref_get (&profile->priv->source);
}

/**
 * fcmdr_profile_set_source:
 * @profile: a #FCmdrProfile
 * @source: a #FCmdrProfileSource, or %NULL
 *
 * Sets the #FCmdrProfileSource from which the @profile originated.
 * Typically this is called once, immediately after a #FCmdrProfile
 * is instantiated.
 **/
void
fcmdr_profile_set_source (FCmdrProfile *profile,
                          FCmdrProfileSource *source)
{
	g_return_if_fail (FCMDR_IS_PROFILE (profile));
	g_return_if_fail (source == NULL || FCMDR_IS_PROFILE_SOURCE (source));

	g_weak_ref_set (&profile->priv->source, source);
}

/**
 * fcmdr_profile_applies_to_user:
 * @profile: a #FCmdrProfile
 * @uid: a user ID
 * @out_score: return location for a match score, or %NULL
 *
 * Returns whether @profile applies to the user identified by @uid by
 * examining the #FCmdrProfile:applies-to data.
 *
 * If @profile applies, the function returns %TRUE and assigns a positive
 * score to @out_score.  The score is meant to help rank a set of applicable
 * profiles.  Profiles with higher scores should be given higher priority.
 *
 * If @profile does NOT apply, the function returns %FALSE and assigns zero
 * to @out_score.
 *
 * Note the score value is based on heuristics which are subject to change.
 *
 * Returns: whether @profile applies to @uid
 **/
gboolean
fcmdr_profile_applies_to_user (FCmdrProfile *profile,
                               uid_t uid,
                               guint *out_score)
{
	FCmdrProfileApplies *applies;
	gchar hostname[256];
	struct passwd *pw;
	guint score = 0;
	gboolean match = FALSE;

	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), FALSE);

	applies = fcmdr_profile_get_applies_to (profile);

	/* No "applies-to" means the profile applies to everyone.
	 * XXX The appropriate score here is subject to debate. */
	if (applies == NULL) {
		score = 10;
		match = TRUE;
		goto exit;
	}

	if (gethostname (hostname, sizeof (hostname)) == 0) {
		if (fcmdr_strv_find (applies->hosts, hostname) >= 0) {
			score += 100;
			match = TRUE;
		}
	}

	if ((pw = getpwuid (uid)) == NULL || pw->pw_name == NULL)
		goto exit;

	if (fcmdr_strv_find (applies->users, pw->pw_name) >= 0) {
		score += 50;
		match = TRUE;
	}

	if (applies->groups != NULL) {
		guint ii;

		for (ii = 0; applies->groups[ii] != NULL; ii++) {
			struct group *gr;

			if ((gr = getgrnam (applies->groups[ii])) == NULL)
				continue;

			if (fcmdr_strv_find (gr->gr_mem, pw->pw_name) >= 0) {
				score += 20;
				match = TRUE;
				break;
			}
		}
	}

exit:
	if (out_score != NULL)
		*out_score = score;

	return match;
}

