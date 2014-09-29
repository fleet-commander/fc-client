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

#include "fcmdr-goa-backend.h"

#include <errno.h>
#include <stdlib.h>

#include "fcmdr-extensions.h"
#include "fcmdr-service.h"
#include "fcmdr-utils.h"

typedef struct _Closure Closure;

struct _Closure {
	GWeakRef backend;
	uid_t uid;
	gchar *bus_address;

	/* For profile processing. */
	GHashTable *symbol_table;
	JsonBuilder *json_builder;
};

G_DEFINE_TYPE_WITH_CODE (
	FCmdrGoaBackend,
	fcmdr_goa_backend,
	FCMDR_TYPE_SERVICE_BACKEND,
	fcmdr_ensure_extension_points_registered ();
	g_io_extension_point_implement (
		FCMDR_SERVICE_BACKEND_EXTENSION_POINT_NAME,
		g_define_type_id,
		"org.gnome.online-accounts", 0))

static void
closure_free (Closure *closure)
{
	g_weak_ref_set (&closure->backend, NULL);
	g_free (closure->bus_address);

	if (closure->symbol_table != NULL)
		g_hash_table_unref (closure->symbol_table);

	g_clear_object (&closure->json_builder);

	g_slice_free (Closure, closure);
}

static gboolean
fcmdr_goa_backend_session_added_idle_cb (gpointer user_data)
{
	FCmdrServiceBackend *backend;
	Closure *closure = user_data;

	backend = g_weak_ref_get (&closure->backend);

	if (backend != NULL) {
		fcmdr_goa_backend_configure_session (
			FCMDR_GOA_BACKEND (backend),
			closure->uid,
			closure->bus_address);
		g_object_unref (backend);
	}

	return G_SOURCE_REMOVE;
}

static void
fcmdr_goa_backend_session_added_cb (FCmdrService *service,
                                    guint uid,
                                    const gchar *bus_address,
                                    FCmdrServiceBackend *backend)
{
	Closure *closure;

	/* Schedule an idle callback so we don't block the signal
	 * emission while talking to the OnlineAccounts service. */

	closure = g_slice_new0 (Closure);
	g_weak_ref_set (&closure->backend, backend);
	closure->uid = (uid_t) uid;
	closure->bus_address = g_strdup (bus_address);

	g_idle_add_full (
		G_PRIORITY_DEFAULT_IDLE,
		fcmdr_goa_backend_session_added_idle_cb,
		closure, (GDestroyNotify) closure_free);
}

static void
fcmdr_goa_backend_constructed (GObject *object)
{
	FCmdrServiceBackend *backend;
	FCmdrService *service;

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (fcmdr_goa_backend_parent_class)->constructed (object);

	backend = FCMDR_SERVICE_BACKEND (object);
	service = fcmdr_service_backend_ref_service (backend);

	/* This backend is owned by the service, so we
	 * shouldn't need to worry about disconnecting. */
	g_signal_connect (
		service, "session-added",
		G_CALLBACK (fcmdr_goa_backend_session_added_cb), backend);
}

static void
fcmdr_goa_backend_class_init (FCmdrGoaBackendClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);
	object_class->constructed = fcmdr_goa_backend_constructed;
}

static void
fcmdr_goa_backend_init (FCmdrGoaBackend *backend)
{
}

/* Helper for fcmdr_goa_backend_configure_session() */
static void
fcmdr_goa_backend_foreach_account_cb (JsonObject *object,
                                      const gchar *member_name,
                                      JsonNode *member_node,
                                      gpointer user_data)
{
	Closure *closure = user_data;
	JsonObject *json_object;
	GList *list, *link;

	json_builder_set_member_name (closure->json_builder, member_name);
	json_builder_begin_object (closure->json_builder);

	json_object = json_node_get_object (member_node);

	list = json_object_get_members (json_object);

	for (link = list; link != NULL; link = g_list_next (link)) {
		const gchar *name = link->data;
		gchar *value;

		value = fcmdr_replace_variables (
			json_object_get_string_member (json_object, name),
			closure->symbol_table);

		json_builder_set_member_name (closure->json_builder, name);
		json_builder_add_string_value (closure->json_builder, value);

		g_free (value);
	}

	g_list_free (list);

	json_builder_end_object (closure->json_builder);
}

/* Helper for fcmdr_goa_backend_configure_session() */
static void
fcmdr_goa_backend_foreach_profile_cb (gpointer data,
                                      gpointer user_data)
{
	FCmdrProfile *profile;
	FCmdrServiceBackend *backend;
	JsonNode *json_node;
	Closure *closure = user_data;

	profile = FCMDR_PROFILE (data);

	backend = g_weak_ref_get (&closure->backend);
	g_return_if_fail (backend != NULL);

	json_node = fcmdr_service_backend_dup_settings (backend, profile);
	g_return_if_fail (json_node != NULL);

	json_object_foreach_member (
		json_node_get_object (json_node),
		fcmdr_goa_backend_foreach_account_cb, closure);

	json_node_free (json_node);
	g_object_unref (backend);
}

/* Helper for fcmdr_goa_backend_configure_session() */
static void
fcmdr_goa_backend_user_lookup_cb (GObject *source_object,
                                  GAsyncResult *result,
                                  gpointer user_data)
{
	FCmdrServiceBackend *backend;
	FCmdrService *service;
	Closure *closure = user_data;
	GSubprocessLauncher *launcher;
	GSubprocess *subprocess;
	gboolean no_relevant_profiles;
	gchar *user_name = NULL;
	gchar *real_name = NULL;
	gchar *uid_str;
	GList *list;
	GError *local_error = NULL;

	fcmdr_user_resolver_lookup_finish (
		FCMDR_USER_RESOLVER (source_object),
		result, &user_name, &real_name, &local_error);

	/* Sanity check. */
	g_warn_if_fail (
		((user_name != NULL) && (local_error == NULL)) ||
		((user_name == NULL) && (local_error != NULL)));

	if (local_error != NULL) {
		g_warning ("%s", local_error->message);
		g_clear_error (&local_error);
		goto exit;
	}

	if (real_name == NULL)
		real_name = g_strdup (user_name);

	closure->symbol_table = g_hash_table_new_full (
		(GHashFunc) g_str_hash,
		(GEqualFunc) g_str_equal,
		(GDestroyNotify) NULL,
		(GDestroyNotify) g_free);

	/* Symbol table takes ownership of the values. */
	g_hash_table_replace (
		closure->symbol_table,
		(gpointer) "username", user_name);
	g_hash_table_replace (
		closure->symbol_table,
		(gpointer) "realname", real_name);

	user_name = NULL;
	real_name = NULL;

	backend = g_weak_ref_get (&closure->backend);

	if (backend == NULL)
		goto exit;

	service = fcmdr_service_backend_ref_service (backend);

	closure->json_builder = json_builder_new ();
	json_builder_begin_object (closure->json_builder);

	list = fcmdr_service_list_profiles_for_user (service, closure->uid);
	list = fcmdr_service_backend_filter_profiles (backend, list);
	no_relevant_profiles = (list == NULL);
	g_list_foreach (list, fcmdr_goa_backend_foreach_profile_cb, closure);
	g_list_free_full (list, (GDestroyNotify) g_object_unref);

	json_builder_end_object (closure->json_builder);

	g_object_unref (service);
	g_object_unref (backend);

	if (no_relevant_profiles)
		goto exit;

	/* Launch the fcmdr-update-goa utility with its user ID set to the
	 * owner of the session bus address so it can connect to the session
	 * bus and call methods on the org.gnome.OnlineAccounts service. */

	g_debug ("Launching %s", FCMDR_UPDATE_GOA);

	launcher = g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDIN_PIPE);

	g_subprocess_launcher_setenv (
		launcher,
		"DBUS_SESSION_BUS_ADDRESS",
		closure->bus_address, TRUE);

	uid_str = g_strdup_printf ("%lu", (gulong) closure->uid);

	subprocess = g_subprocess_launcher_spawn (
		launcher, &local_error, FCMDR_UPDATE_GOA, uid_str, NULL);

	g_free (uid_str);

	g_object_unref (launcher);

	/* Sanity check. */
	g_warn_if_fail (
		((subprocess != NULL) && (local_error == NULL)) ||
		((subprocess == NULL) && (local_error != NULL)));

	if (subprocess != NULL) {
		GOutputStream *stdin_pipe;
		JsonGenerator *json_generator;
		JsonNode *root;

		json_generator = json_generator_new ();

		root = json_builder_get_root (closure->json_builder);
		json_generator_set_root (json_generator, root);
		json_node_free (root);

		stdin_pipe = g_subprocess_get_stdin_pipe (subprocess);

		json_generator_to_stream (
			json_generator, stdin_pipe, NULL, &local_error);

		if (local_error == NULL)
			g_output_stream_close (stdin_pipe, NULL, &local_error);

		g_object_unref (json_generator);
		g_object_unref (subprocess);
	}

	if (local_error != NULL) {
		g_warning ("%s", local_error->message);
		g_clear_error (&local_error);
		goto exit;
	}

exit:
	closure_free (closure);
}

void
fcmdr_goa_backend_configure_session (FCmdrGoaBackend *backend,
                                     uid_t uid,
                                     const gchar *bus_address)
{
	FCmdrService *service;
	FCmdrServiceBackend *service_backend;
	FCmdrUserResolver *resolver;
	Closure *closure;

	g_return_if_fail (FCMDR_IS_GOA_BACKEND (backend));
	g_return_if_fail (bus_address != NULL);

	service_backend = FCMDR_SERVICE_BACKEND (backend);
	service = fcmdr_service_backend_ref_service (service_backend);
	resolver = fcmdr_service_get_user_resolver (service);

	closure = g_slice_new0 (Closure);
	g_weak_ref_set (&closure->backend, backend);
	closure->uid = uid;
	closure->bus_address = g_strdup (bus_address);

	fcmdr_user_resolver_lookup (
		resolver, uid, NULL,
		fcmdr_goa_backend_user_lookup_cb, closure);

	g_object_unref (service);
}

