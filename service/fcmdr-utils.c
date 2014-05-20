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

