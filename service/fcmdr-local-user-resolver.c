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

#include "fcmdr-local-user-resolver.h"

#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>

#include "fcmdr-extensions.h"

#define FCMDR_LOCAL_USER_RESOLVER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_LOCAL_USER_RESOLVER, FCmdrLocalUserResolverPrivate))

typedef struct _UserDatabaseEntry UserDatabaseEntry;

struct _FCmdrLocalUserResolverPrivate {
	/* uid_t* -> UserDatabaseEntry */
	GHashTable *user_database;
	GMutex user_database_lock;
};

struct _UserDatabaseEntry {
	uid_t uid;
	gchar *user_name;
	gchar *real_name;
};

G_DEFINE_TYPE_WITH_CODE (
	FCmdrLocalUserResolver,
	fcmdr_local_user_resolver,
	FCMDR_TYPE_USER_RESOLVER,
	fcmdr_ensure_extension_points_registered ();
	g_io_extension_point_implement (
		FCMDR_USER_RESOLVER_EXTENSION_POINT_NAME,
		g_define_type_id, "local", 0))

static UserDatabaseEntry *
user_database_entry_new (uid_t uid)
{
	UserDatabaseEntry *entry;

	entry = g_slice_new0 (UserDatabaseEntry);
	entry->uid = uid;

	return entry;
}

static UserDatabaseEntry *
user_database_entry_dup (UserDatabaseEntry *entry)
{
	UserDatabaseEntry *duped;

	duped = user_database_entry_new (entry->uid);
	duped->user_name = g_strdup (entry->user_name);
	duped->real_name = g_strdup (entry->real_name);

	return duped;
}

static void
user_database_entry_free (UserDatabaseEntry *entry)
{
	g_free (entry->user_name);
	g_free (entry->real_name);

	g_slice_free (UserDatabaseEntry, entry);
}

static void
fcmdr_local_user_resolver_finalize (GObject *object)
{
	FCmdrLocalUserResolverPrivate *priv;

	priv = FCMDR_LOCAL_USER_RESOLVER_GET_PRIVATE (object);

	g_hash_table_destroy (priv->user_database);
	g_mutex_clear (&priv->user_database_lock);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (fcmdr_local_user_resolver_parent_class)->
		finalize (object);
}

static void
fcmdr_local_user_resolver_lookup (FCmdrUserResolver *resolver,
                                  uid_t uid,
                                  GCancellable *cancellable,
                                  GAsyncReadyCallback callback,
                                  gpointer user_data)
{
	FCmdrLocalUserResolverPrivate *priv;
	GTask *task;
	UserDatabaseEntry *entry = NULL;
	struct passwd pw_static;
	struct passwd *pw = NULL;
	gpointer pw_buffer = NULL;
	glong pw_buffer_size;
	GError *local_error = NULL;

	priv = FCMDR_LOCAL_USER_RESOLVER_GET_PRIVATE (resolver);

	task = g_task_new (resolver, cancellable, callback, user_data);
	g_task_set_source_tag (task, fcmdr_local_user_resolver_lookup);

	g_mutex_lock (&priv->user_database_lock);

	entry = g_hash_table_lookup (priv->user_database, &uid);

	if (entry != NULL) {
		g_task_set_task_data (
			task, user_database_entry_dup (entry),
			(GDestroyNotify) user_database_entry_free);
		goto exit;
	}

	/* XXX This is based on g_get_user_database_entry()
	 *     in GLib, minus a lot of the conditional goo. */

	pw_buffer_size = sysconf (_SC_GETPW_R_SIZE_MAX);

	if (pw_buffer_size < 0)
		pw_buffer_size = 64;

	do {
		gint error;

		g_free (pw_buffer);
		pw_buffer = g_malloc (pw_buffer_size);

		error = getpwuid_r (
			uid, &pw_static, pw_buffer, pw_buffer_size, &pw);

		if (pw == NULL) {
			if (error == 0) {
				local_error = g_error_new (
					G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
					"getpwuid_r(): failed due to unknown "
					"user id (%lu)", (gulong) uid);
				break;
			}

			/* Buffer size is large enough, give up. */
			if (pw_buffer_size > 32 * 1024) {
				local_error = g_error_new (
					G_IO_ERROR,
					g_io_error_from_errno (error),
					"getpwuid_r(): failed due to: %s",
					g_strerror (error));
				break;
			}

			/* Try again with a larger buffer. */
			pw_buffer_size *= 2;
		}
	} while (pw == NULL);

	/* Sanity check */
	g_warn_if_fail (
		((pw != NULL) && (local_error == NULL)) ||
		((pw == NULL) && (local_error != NULL)));

	if (pw != NULL) {
		entry = user_database_entry_new (uid);
		entry->user_name = g_strdup (pw->pw_name);

		if (pw->pw_gecos != NULL && *pw->pw_gecos != '\0') {
			gchar **gecos_fields;
			gchar **name_parts;

			/* Split the GECOS field and substitute '&' */
			gecos_fields = g_strsplit (pw->pw_gecos, ",", 0);
			name_parts = g_strsplit (gecos_fields[0], "&", 0);
			pw->pw_name[0] = g_ascii_toupper (pw->pw_name[0]);
			entry->real_name = g_strjoinv (pw->pw_name, name_parts);
			g_strfreev (gecos_fields);
			g_strfreev (name_parts);
		}

		g_hash_table_replace (
			priv->user_database,
			&entry->uid, entry);

		g_task_set_task_data (
			task, user_database_entry_dup (entry),
			(GDestroyNotify) user_database_entry_free);
	}

	g_free (pw_buffer);

exit:
	g_mutex_unlock (&priv->user_database_lock);

	if (local_error == NULL)
		g_task_return_boolean (task, TRUE);
	else
		g_task_return_error (task, local_error);

	g_object_unref (task);
}

static gboolean
fcmdr_local_user_resolver_lookup_finish (FCmdrUserResolver *resolver,
                                         GAsyncResult *result,
                                         gchar **out_user_name,
                                         gchar **out_real_name,
                                         GError **error)
{
	g_return_val_if_fail (
		g_task_is_valid (result, resolver), FALSE);
	g_return_val_if_fail (
		g_async_result_is_tagged (
		result, fcmdr_local_user_resolver_lookup), FALSE);

	if (!g_task_had_error (G_TASK (result))) {
		UserDatabaseEntry *entry;

		entry = g_task_get_task_data (G_TASK (result));
		g_return_val_if_fail (entry != NULL, FALSE);

		if (out_user_name != NULL) {
			*out_user_name = entry->user_name;
			entry->user_name = NULL;
		}

		if (out_real_name != NULL) {
			*out_real_name = entry->real_name;
			entry->real_name = NULL;
		}
	}

	return g_task_propagate_boolean (G_TASK (result), error);
}

static void
fcmdr_local_user_resolver_class_init (FCmdrLocalUserResolverClass *class)
{
	GObjectClass *object_class;
	FCmdrUserResolverClass *resolver_class;

	g_type_class_add_private (
		class, sizeof (FCmdrLocalUserResolverPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = fcmdr_local_user_resolver_finalize;

	resolver_class = FCMDR_USER_RESOLVER_CLASS (class);
	resolver_class->lookup = fcmdr_local_user_resolver_lookup;
	resolver_class->lookup_finish = fcmdr_local_user_resolver_lookup_finish;
}

static void
fcmdr_local_user_resolver_init (FCmdrLocalUserResolver *resolver)
{
	GHashTable *user_database;

	user_database = g_hash_table_new_full (
		(GHashFunc) g_int_hash,
		(GEqualFunc) g_int_equal,
		(GDestroyNotify) NULL,
		(GDestroyNotify) user_database_entry_free);

	resolver->priv = FCMDR_LOCAL_USER_RESOLVER_GET_PRIVATE (resolver);
	resolver->priv->user_database = user_database;

	g_mutex_init (&resolver->priv->user_database_lock);
}

