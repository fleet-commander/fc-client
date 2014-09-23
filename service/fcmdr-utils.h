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

#ifndef __FCMDR_UTILS_H__
#define __FCMDR_UTILS_H__

#include <gio/gio.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

GVariant *	fcmdr_json_value_to_variant	(JsonNode *json_node);
gchar **	fcmdr_json_array_to_strv	(JsonArray *json_array);
JsonArray *	fcmdr_strv_to_json_array	(gchar **strv);
gint		fcmdr_strv_find			(gchar **haystack,
						 const gchar *needle);
gint		fcmdr_compare_uints		(gconstpointer a,
						 gconstpointer b);
gboolean	fcmdr_recursive_delete_sync	(GFile *file,
						 GCancellable *cancellable,
						 GError **error);
gboolean	fcmdr_get_connection_unix_user_sync
						(GDBusConnection *connection,
						 const gchar *bus_name,
						 uid_t *out_uid,
						 GCancellable *cancellable,
						 GError **error);

G_END_DECLS

#endif /* __FCMDR_UTILS_H__ */

