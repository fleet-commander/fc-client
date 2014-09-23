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

/**
 * fcmdr_strv_find:
 * @haystack: a %NULL-terminated string array to search, or %NULL
 * @needle: a string to search for
 *
 * Returns the array index of @needle in @haystack, or -1 if not found.
 * As a convenience, @haystack may be %NULL, in which case the function
 * returns -1.
 *
 * Returns: an array index in @haystack, or -1
 **/
gint
fcmdr_strv_find (gchar **haystack,
                 const gchar *needle)
{
	g_return_val_if_fail (needle != NULL, -1);

	if (haystack != NULL) {
		gint ii;

		for (ii = 0; haystack[ii] != NULL; ii++) {
			if (g_str_equal (haystack[ii], needle))
				return ii;
		}
	}

	return -1;
}

/**
 * fcmdr_compare_uints:
 * @a: an unsigned integer stuffed in a pointer
 * @b: an unsigned integer stuffed in a pointer
 *
 * Compares two unsigned integers stuffed into pointers with
 * #GUINT_TO_POINTER.  For use with functions that take a #GCompareFunc.
 *
 * Returns: negative value if @a < @b; zero if @a = @b;
 *          positive value if @a > @b
 **/
gint
fcmdr_compare_uints (gconstpointer a,
                     gconstpointer b)
{
	guint int_a = GPOINTER_TO_UINT (a);
	guint int_b = GPOINTER_TO_UINT (b);

	return (int_a < int_b) ? -1 : (int_a == int_b) ? 0 : 1;
}

/**
 * fcmdr_recursive_delete_sync:
 * @file: a #GFile to delete
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Deletes @file.  If @file is a directory, its contents are deleted
 * recursively before @file itself is deleted.  The recursive delete
 * operation will stop on the first error.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled
 * by triggering the cancellable object from another thread.  If the
 * operation was cancelled, the error #G_IO_ERROR_CANCELLED will be
 * returned.
 *
 * Returns: %TRUE if the file was deleted, %FALSE otherwise
 **/
gboolean
fcmdr_recursive_delete_sync (GFile *file,
                             GCancellable *cancellable,
                             GError **error)
{
	/* XXX Copied from libedataserver.  Still waiting for
	 *     libgio to get its own recursive delete function. */

	GFileEnumerator *file_enumerator;
	GFileInfo *file_info;
	GFileType file_type;
	gboolean success = TRUE;
	GError *local_error = NULL;

	g_return_val_if_fail (G_IS_FILE (file), FALSE);

	file_type = g_file_query_file_type (
		file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, cancellable);

	/* If this is not a directory, delete like normal. */
	if (file_type != G_FILE_TYPE_DIRECTORY)
		return g_file_delete (file, cancellable, error);

	/* Note, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS is critical here
	 * so we only delete files inside the directory being deleted. */
	file_enumerator = g_file_enumerate_children (
		file, G_FILE_ATTRIBUTE_STANDARD_NAME,
		G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
		cancellable, error);

	if (file_enumerator == NULL)
		return FALSE;

	file_info = g_file_enumerator_next_file (
		file_enumerator, cancellable, &local_error);

	while (file_info != NULL) {
		GFile *child;
		const gchar *name;

		name = g_file_info_get_name (file_info);

		/* Here's the recursive part. */
		child = g_file_get_child (file, name);
		success = fcmdr_recursive_delete_sync (
			child, cancellable, error);
		g_object_unref (child);

		g_object_unref (file_info);

		if (!success)
			break;

		file_info = g_file_enumerator_next_file (
			file_enumerator, cancellable, &local_error);
	}

	if (local_error != NULL) {
		g_propagate_error (error, local_error);
		success = FALSE;
	}

	g_object_unref (file_enumerator);

	if (!success)
		return FALSE;

	/* The directory should be empty now. */
	return g_file_delete (file, cancellable, error);
}

/**
 * fcmdr_get_connection_unix_user_sync:
 * @connection: a #GDBusConnection
 * @bus_name: a name owned by @connection
 * @out_uid: return location for a user ID
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Returns the UNIX user ID of the process owning the given @bus_name.
 * If unable to determine it, the function sets @error and returns %FALSE.
 *
 * This function synchronously invokes the "GetConnectionUnixUser" method
 * of the "org.freedesktop.DBus" interface on the message bus represented
 * by @connection.
 *
 * Returns: %TRUE if the method call succeeded, %FALSE if @error is set
 **/
gboolean
fcmdr_get_connection_unix_user_sync (GDBusConnection *connection,
                                     const gchar *bus_name,
                                     uid_t *out_uid,
                                     GCancellable *cancellable,
                                     GError **error)
{
	GVariant *result;
	guint32 uid;

	g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), FALSE);
	g_return_val_if_fail (g_dbus_is_name (bus_name), FALSE);

	result = g_dbus_connection_call_sync (
		connection,
		"org.freedesktop.DBus",
		"/",
		"org.freedesktop.DBus",
		"GetConnectionUnixUser",
		g_variant_new ("(s)", bus_name),
		NULL,
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		cancellable,
		error);

	if (result == NULL)
		return FALSE;

	if (out_uid != NULL) {
		g_variant_get (result, "(u)", &uid);
		*out_uid = (uid_t) uid;
	}

	g_variant_unref (result);

	return TRUE;
}

