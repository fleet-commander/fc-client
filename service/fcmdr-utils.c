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
 * SECTION: fcmdr-utils
 * @short_description: Miscellaneous utilities
 *
 * An assortment of simple utility functions.
 **/

#include "config.h"

#include "fcmdr-utils.h"

/**
 * fcmdr_json_value_to_variant:
 * @json_node: a #JsonNode
 *
 * Converts a #JsonNode containing a fundamental type to a #GVariant.
 * The resulting #GVariantType will be one of #G_VARIANT_TYPE_INT64,
 * #G_VARIANT_TYPE_DOUBLE, #G_VARIANT_TYPE_BOOLEAN or #G_VARIANT_TYPE_STRING.
 *
 * Returns: a floating reference to a new #GVariant instance
 **/
GVariant *
fcmdr_json_value_to_variant (JsonNode *json_node)
{
	GVariant *variant = NULL;

	g_return_val_if_fail (json_node != NULL, NULL);
	g_return_val_if_fail (JSON_NODE_HOLDS_VALUE (json_node), NULL);

	/* Cases are based on json_value_type() in json-value.c */
	switch (json_node_get_value_type (json_node)) {
		case G_TYPE_INT64:
			variant = g_variant_new_int64 (
				json_node_get_int (json_node));
			break;

		case G_TYPE_DOUBLE:
			variant = g_variant_new_double (
				json_node_get_double (json_node));
			break;

		case G_TYPE_BOOLEAN:
			variant = g_variant_new_boolean (
				json_node_get_boolean (json_node));
			break;

		case G_TYPE_STRING:
			variant = g_variant_new_string (
				json_node_get_string (json_node));
			break;

		default:
			g_warn_if_reached ();
	}

	return variant;
}

/**
 * fcmdr_json_array_to_strv:
 * @json_array: a #JsonArray
 *
 * Builds a %NULL-terminated string array from the string elements in
 * @json_array.  Free the returned string array with g_strfreev().
 *
 * Returns: a new %NULL-terminated string array
 **/
gchar **
fcmdr_json_array_to_strv (JsonArray *json_array)
{
	gchar **strv;
	guint ii, length;
	guint strv_index = 0;

	g_return_val_if_fail (json_array != NULL, NULL);

	length = json_array_get_length (json_array);
	strv = g_new0 (gchar *, length + 1);

	for (ii = 0; ii < length; ii++) {
		const gchar *element;

		element = json_array_get_string_element (json_array, ii);

		if (element != NULL)
			strv[strv_index++] = g_strdup (element);
	}

	return strv;
}

/**
 * fcmdr_strv_to_json_array:
 * @strv: a %NULL-terminated string array, or %NULL
 *
 * Converts a %NULL-terminated string array to a #JsonArray with
 * equivalent string elements.  If @strv is %NULL, the function
 * returns an empty #JsonArray.
 *
 * Returns: a new #JsonArray
 **/
JsonArray *
fcmdr_strv_to_json_array (gchar **strv)
{
	JsonArray *json_array;
	guint ii, length = 0;

	json_array = json_array_new ();

	if (strv != NULL)
		length = g_strv_length (strv);

	for (ii = 0; ii < length; ii++)
		json_array_add_string_element (json_array, strv[ii]);

	return json_array;
}

