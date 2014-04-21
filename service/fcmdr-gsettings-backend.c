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

#include "fcmdr-gsettings-backend.h"

#include "fcmdr-extensions.h"
#include "fcmdr-utils.h"

#define FCMDR_GSETTINGS_BACKEND_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_GSETTINGS_BACKEND, FCmdrGSettingsBackendPrivate))

struct _FCmdrGSettingsBackendPrivate {
	GKeyFile *key_file;
	GHashTable *locks;
};

G_DEFINE_TYPE_WITH_CODE (
	FCmdrGSettingsBackend,
	fcmdr_gsettings_backend,
	FCMDR_TYPE_SETTINGS_BACKEND,
	fcmdr_ensure_extension_points_registered();
	g_io_extension_point_implement (
		FCMDR_SETTINGS_BACKEND_EXTENSION_POINT_NAME,
		g_define_type_id,
		"org.gnome.gsettings", 0))

static void
fcmdr_gsettings_backend_foreach_element_cb (JsonArray *json_array,
                                            guint index,
                                            JsonNode *json_node,
                                            gpointer user_data)
{
	GKeyFile *key_file = user_data;
	JsonObject *json_object;
	GVariant *value;
	const gchar *key;
	const gchar *schema;
	gchar *str;

	g_return_if_fail (JSON_NODE_HOLDS_OBJECT (json_node));

	json_object = json_node_get_object (json_node);
	g_return_if_fail (json_object_has_member (json_object, "key"));
	g_return_if_fail (json_object_has_member (json_object, "schema"));
	g_return_if_fail (json_object_has_member (json_object, "value"));

	key = json_object_get_string_member (json_object, "key");
	schema = json_object_get_string_member (json_object, "schema");

	json_node = json_object_get_member (json_object, "value");
	value = fcmdr_json_value_to_variant (json_node);

	/* This is how GKeyfileSettingsBackend does it. */
	if (value != NULL) {
		gchar *str = g_variant_print (value, FALSE);
		g_key_file_set_value (key_file, schema, key, str);
		g_variant_unref (g_variant_ref_sink (value));
		g_free (str);
	}
}

static void
fcmdr_gsettings_backend_parse_json_node (FCmdrGSettingsBackend *backend,
                                         JsonNode *json_node)
{
	JsonArray *json_array;
	gchar **groups;
	gsize ii, n_groups;

	g_return_if_fail (JSON_NODE_HOLDS_ARRAY (json_node));

	json_array = json_node_get_array (json_node);

	json_array_foreach_element (
		json_array,
		fcmdr_gsettings_backend_foreach_element_cb,
		backend->priv->key_file);

	groups = g_key_file_get_groups (backend->priv->key_file, &n_groups);

	for (ii = 0; ii < n_groups; ii++) {
		gchar **keys;
		gsize jj, n_keys;

		keys = g_key_file_get_keys (
			backend->priv->key_file,
			groups[ii], &n_keys, NULL);

		/* Steal the allocated key names for the hash table. */
		for (jj = 0; jj < n_keys; jj++) {
			g_hash_table_add (backend->priv->locks, keys[jj]);
			keys[jj] = NULL;
		}

		/* Free the array of NULL pointers. */
		g_free (keys);
	}

	g_strfreev (groups);
}

static void
fcmdr_gsettings_backend_finalize (GObject *object)
{
	FCmdrGSettingsBackendPrivate *priv;

	priv = FCMDR_GSETTINGS_BACKEND_GET_PRIVATE (object);

	g_key_file_free (priv->key_file);
	g_hash_table_destroy (priv->locks);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (fcmdr_gsettings_backend_parent_class)->
		finalize (object);
}

static void
fcmdr_gsettings_backend_constructed (GObject *object)
{
	FCmdrGSettingsBackend *backend;
	JsonNode *settings;

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (fcmdr_gsettings_backend_parent_class)->
		constructed (object);

	backend = FCMDR_GSETTINGS_BACKEND (object);

	settings = fcmdr_settings_backend_get_settings (
		FCMDR_SETTINGS_BACKEND (backend));

	fcmdr_gsettings_backend_parse_json_node (backend, settings);
}

static void
fcmdr_gsettings_backend_class_init (FCmdrGSettingsBackendClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (
		class, sizeof (FCmdrGSettingsBackendPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = fcmdr_gsettings_backend_finalize;
	object_class->constructed = fcmdr_gsettings_backend_constructed;
}

static void
fcmdr_gsettings_backend_init (FCmdrGSettingsBackend *backend)
{
	backend->priv = FCMDR_GSETTINGS_BACKEND_GET_PRIVATE (backend);

	backend->priv->key_file = g_key_file_new ();

	backend->priv->locks = g_hash_table_new_full (
		(GHashFunc) g_str_hash,
		(GEqualFunc) g_str_equal,
		(GDestroyNotify) g_free,
		(GDestroyNotify) NULL);
}

