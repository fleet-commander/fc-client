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

#include "fcmdr-service.h"

#include "fcmdr-extensions.h"
#include "fcmdr-generated.h"
#include "fcmdr-logind-monitor.h"

#define FCMDR_SERVICE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_SERVICE, FCmdrServicePrivate))

#define FCMDR_SERVICE_CACHE_DIR \
	LOCALSTATEDIR \
	G_DIR_SEPARATOR_S "cache" \
	G_DIR_SEPARATOR_S "fleet-commander"

#define FCMDR_PROFILE_CACHE_FILE \
	FCMDR_SERVICE_CACHE_DIR G_DIR_SEPARATOR_S "profiles.json"

typedef struct _AsyncContext AsyncContext;

struct _FCmdrServicePrivate {
	GDBusConnection *connection;
	FCmdrProfiles *profiles_interface;
	FCmdrLoginMonitor *login_monitor;

	/* Name -> FCmdrServiceBackend */
	GHashTable *backends;
	GMutex backends_lock;

	/* Profile UID -> FCmdrProfile */
	GHashTable *profiles;
	GMutex profiles_lock;

	GQueue sources;
	GMutex sources_lock;

	guint refresh_timeout_id;
};

struct _AsyncContext {
	/* for D-Bus methods */
	GDBusInterface *interface;
	GDBusMethodInvocation *invocation;
	FCmdrService *service;

	/* for load_remote_profiles() */
	guint unfinished_subtasks;
	gboolean apply_profiles;
	GArray *cancel_ids;
	GHashTable *errors;
};

enum {
	PROP_0,
	PROP_CONNECTION
};

/* Forward Declarations */
static void	fcmdr_service_initable_interface_init
						(GInitableIface *interface);
static gboolean
		fcmdr_service_refresh_profiles_start_cb
						(gpointer user_data);

G_DEFINE_TYPE_WITH_CODE (
	FCmdrService,
	fcmdr_service,
	G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE (
		G_TYPE_INITABLE,
		fcmdr_service_initable_interface_init))

static void
async_context_free (AsyncContext *async_context)
{
	g_clear_object (&async_context->interface);
	g_clear_object (&async_context->invocation);
	g_clear_object (&async_context->service);

	if (async_context->cancel_ids != NULL)
		g_array_free (async_context->cancel_ids, TRUE);

	if (async_context->errors != NULL)
		g_hash_table_destroy (async_context->errors);

	g_slice_free (AsyncContext, async_context);
}

static GDataInputStream *
fcmdr_service_open_config (FCmdrService *service)
{
	GDataInputStream *data_input_stream = NULL;
	const gchar * const *system_config_dirs;
	gboolean stop = FALSE;
	guint ii;

	system_config_dirs = g_get_system_config_dirs ();
	g_return_if_fail (system_config_dirs != NULL);

	for (ii = 0; !stop && system_config_dirs[ii] != NULL; ii++) {
		GFileInputStream *file_input_stream;
		GFile *file;
		gchar *path;
		GError *local_error = NULL;

		path = g_build_filename (
			system_config_dirs[ii],
			"fleet-commander.conf", NULL);

		file = g_file_new_for_path (path);
		file_input_stream = g_file_read (file, NULL, &local_error);
		g_object_unref (file);

		/* If no file found, try the next system config dir. */
		if (g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
			g_clear_error (&local_error);

		if (file_input_stream != NULL) {
			g_warn_if_fail (local_error == NULL);
			data_input_stream = g_data_input_stream_new (
				G_INPUT_STREAM (file_input_stream));
			g_object_unref (file_input_stream);
			stop = TRUE;

		} else if (local_error != NULL) {
			g_warning ("%s: %s", path, local_error->message);
			g_error_free (local_error);
			stop = TRUE;
		}

		g_free (path);
	}

	return data_input_stream;
}

static void
fcmdr_service_read_config_line (FCmdrService *service,
                                const gchar *line)
{
	if (g_ascii_strncasecmp (line, "source:", 7) == 0) {
		FCmdrProfileSource *source = NULL;
		SoupURI *uri;
		const gchar *cp = line + 7;

		while (g_ascii_isspace (*cp))
			cp++;

		if (*cp == '\0')
			return;

		uri = soup_uri_new (cp);

		if (uri == NULL) {
			g_warning ("Malformed source uri: %s", cp);
			return;
		}

		source = fcmdr_profile_source_try_new (service, uri);

		soup_uri_free (uri);

		if (source == NULL) {
			g_warning ("Unsupported source uri: %s", cp);
			return;
		}

		g_debug ("%s handling %s\n", G_OBJECT_TYPE_NAME (source), cp);

		g_mutex_lock (&service->priv->sources_lock);
		g_queue_push_tail (
			&service->priv->sources,
			g_object_ref (source));
		g_mutex_unlock (&service->priv->sources_lock);

		g_object_unref (source);
	}
}

static void
fcmdr_service_init_profile_sources (FCmdrService *service)
{
	GDataInputStream *input_stream;
	gchar *line;
	GError *local_error = NULL;

	input_stream = fcmdr_service_open_config (service);
	if (input_stream == NULL)
		return;

	line = g_data_input_stream_read_line_utf8 (
		input_stream, NULL, NULL, &local_error);

	while (line != NULL) {
		fcmdr_service_read_config_line (service, g_strstrip (line));
		g_free (line);

		line = g_data_input_stream_read_line_utf8 (
			input_stream, NULL, NULL, &local_error);
	}

	if (local_error != NULL) {
		g_warning ("%s: %s", G_STRFUNC, local_error->message);
		g_error_free (local_error);
	}
}

static void
fcmdr_service_refresh_profiles_done_cb (GObject *source_object,
                                        GAsyncResult *result,
                                        gpointer user_data)
{
	FCmdrService *service;
	GHashTable *errors;

	service = FCMDR_SERVICE (source_object);

	errors = fcmdr_service_load_remote_profiles_finish (service, result);

	if (g_hash_table_size (errors) > 0) {
		GHashTableIter iter;
		gpointer key, value;

		g_warning ("Failures occurred while fetching profiles:");

		g_hash_table_iter_init (&iter, errors);

		while (g_hash_table_iter_next (&iter, &key, &value)) {
			FCmdrProfileSource *source = key;
			GError *error = value;
			SoupURI *uri;
			gchar *uri_string;

			uri = fcmdr_profile_source_dup_uri (source);
			uri_string = soup_uri_to_string (uri, FALSE);

			g_warning ("  %s: %s", uri_string, error->message);

			g_free (uri_string);
			soup_uri_free (uri);
		}
	}

	g_hash_table_destroy (errors);

	service->priv->refresh_timeout_id = g_timeout_add_seconds (
		60 * 60, fcmdr_service_refresh_profiles_start_cb, service);
}

static gboolean
fcmdr_service_refresh_profiles_start_cb (gpointer user_data)
{
	FCmdrService *service;
	GList *profile_sources;

	service = FCMDR_SERVICE (user_data);

	profile_sources = fcmdr_service_list_profile_sources (service);

	fcmdr_service_load_remote_profiles (
		service, profile_sources, NULL,
		fcmdr_service_refresh_profiles_done_cb, NULL);

	g_list_free_full (profile_sources, (GDestroyNotify) g_object_unref);

	service->priv->refresh_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

static gboolean
fcmdr_service_handle_profiles_add_cb (FCmdrProfiles *interface,
                                      GDBusMethodInvocation *invocation,
                                      const gchar *json_data,
                                      FCmdrService *service)
{
	FCmdrProfile *profile;
	GError *local_error = NULL;

	profile = fcmdr_profile_new (json_data, -1, &local_error);

	/* Sanity check */
	g_warn_if_fail (
		((profile != NULL) && (local_error == NULL)) ||
		((profile == NULL) && (local_error != NULL)));

	if (profile != NULL) {
		fcmdr_service_add_profile (service, profile);
		g_object_unref (profile);
	}

	if (local_error == NULL) {
		fcmdr_profiles_complete_add (interface, invocation);
	} else {
		g_dbus_method_invocation_take_error (invocation, local_error);
	}

	return TRUE;
}

/* Helper for "AddFile" method handler. */
static void
fcmdr_service_add_file_load_cb (GObject *source_object,
                                GAsyncResult *result,
                                gpointer user_data)
{
	AsyncContext *async_context = user_data;
	FCmdrProfile *profile = NULL;
	gchar *json_data = NULL;
	GError *local_error = NULL;

	g_file_load_contents_finish (
		G_FILE (source_object), result,
		&json_data, NULL, NULL, &local_error);

	if (json_data != NULL) {
		profile = fcmdr_profile_new (json_data, -1, &local_error);
		g_free (json_data);
	}

	/* Sanity check */
	g_warn_if_fail (
		((profile != NULL) && (local_error == NULL)) ||
		((profile == NULL) && (local_error != NULL)));

	if (profile != NULL) {
		fcmdr_service_add_profile (async_context->service, profile);
		g_object_unref (profile);
	}

	if (local_error == NULL) {
		fcmdr_profiles_complete_add_file (
			FCMDR_PROFILES (async_context->interface),
			async_context->invocation);
	} else {
		g_dbus_method_invocation_take_error (
			async_context->invocation, local_error);
	}

	async_context_free (async_context);
}

static gboolean
fcmdr_service_handle_profiles_add_file_cb (FCmdrProfiles *interface,
                                           GDBusMethodInvocation *invocation,
                                           const gchar *json_file,
                                           FCmdrService *service)
{
	AsyncContext *async_context;
	GFile *file;
	GTask *task;

	file = g_file_new_for_commandline_arg (json_file);

	async_context = g_slice_new0 (AsyncContext);
	async_context->interface = g_object_ref (interface);
	async_context->invocation = g_object_ref (invocation);
	async_context->service = g_object_ref (service);

	g_file_load_contents_async (
		file, NULL,
		fcmdr_service_add_file_load_cb,
		async_context);

	g_object_unref (file);

	return TRUE;
}

static gboolean
fcmdr_service_handle_profiles_get_cb (FCmdrProfiles *interface,
                                      GDBusMethodInvocation *invocation,
                                      const gchar *uid,
                                      FCmdrService *service)
{
	FCmdrProfile *profile;

	profile = fcmdr_service_ref_profile (service, uid);

	if (profile != NULL) {
		gchar *json_data;

		json_data = json_gobject_to_data (G_OBJECT (profile), NULL);
		fcmdr_profiles_complete_get (interface, invocation, json_data);
		g_free (json_data);

		g_object_unref (profile);
	} else {
		g_dbus_method_invocation_return_error (
			invocation, G_IO_ERROR,
			G_IO_ERROR_NOT_FOUND,
			"No profile with UID '%s'", uid);
	}

	return TRUE;
}

static gboolean
fcmdr_service_handle_profiles_list_cb (FCmdrProfiles *interface,
                                       GDBusMethodInvocation *invocation,
                                       FCmdrService *service)
{
	GList *list, *link;
	guint length, ii = 0;
	gchar **strv;

	list = fcmdr_service_list_profiles (service);
	strv = g_new0 (gchar *, g_list_length (list) + 1);

	for (link = list; link != NULL; link = g_list_next (link)) {
		FCmdrProfile *profile;
		const gchar *profile_uid;

		profile = FCMDR_PROFILE (link->data);
		profile_uid = fcmdr_profile_get_uid (profile);
		strv[ii++] = g_strdup (profile_uid);
	}

	fcmdr_profiles_complete_list (
		interface, invocation, (const gchar * const *) strv);

	g_strfreev (strv);

	g_list_free_full (list, (GDestroyNotify) g_object_unref);

	return TRUE;
}

static void
fcmdr_service_init_backends (FCmdrService *service)
{
	GIOExtensionPoint *extension_point;
	GList *list, *link;

	fcmdr_ensure_extensions_registered ();

	extension_point = g_io_extension_point_lookup (
		FCMDR_SERVICE_BACKEND_EXTENSION_POINT_NAME);

	list = g_io_extension_point_get_extensions (extension_point);

	for (link = list; link != NULL; link = g_list_next (link)) {
		GIOExtension *extension = link->data;
		FCmdrServiceBackend *backend;
		const gchar *name;
		GType type;

		name = g_io_extension_get_name (extension);
		type = g_io_extension_get_type (extension);

		backend = g_object_new (type, "service", service, NULL);

		if (G_IS_INITABLE (backend)) {
			GError *local_error = NULL;

			g_initable_init (
				G_INITABLE (backend), NULL, &local_error);

			if (local_error != NULL) {
				g_critical (
					"Could not initialize %s: %s",
					G_OBJECT_TYPE_NAME (backend),
					local_error->message);
				g_error_free (local_error);
				g_object_unref (backend);
				continue;
			}
		}

		g_mutex_lock (&service->priv->backends_lock);

		g_hash_table_replace (
			service->priv->backends,
			g_strdup (name),
			g_object_ref (backend));

		g_mutex_unlock (&service->priv->backends_lock);

		g_object_unref (backend);
	}
}

static void
fcmdr_service_set_connection (FCmdrService *service,
                              GDBusConnection *connection)
{
	g_return_if_fail (G_IS_DBUS_CONNECTION (connection));
	g_return_if_fail (service->priv->connection == NULL);

	service->priv->connection = g_object_ref (connection);
}

static void
fcmdr_service_set_property (GObject *object,
                            guint property_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_CONNECTION:
			fcmdr_service_set_connection (
				FCMDR_SERVICE (object),
				g_value_get_object (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_service_get_property (GObject *object,
                            guint property_id,
                            GValue *value,
                            GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_CONNECTION:
			g_value_set_object (
				value,
				fcmdr_service_get_connection (
				FCMDR_SERVICE (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_service_dispose (GObject *object)
{
	FCmdrServicePrivate *priv;

	priv = FCMDR_SERVICE_GET_PRIVATE (object);

	g_clear_object (&priv->connection);
	g_clear_object (&priv->profiles_interface);
	g_clear_object (&priv->login_monitor);

	g_hash_table_remove_all (priv->backends);
	g_hash_table_remove_all (priv->profiles);

	while (!g_queue_is_empty (&priv->sources))
		g_object_unref (g_queue_pop_head (&priv->sources));

	if (priv->refresh_timeout_id > 0) {
		g_source_remove (priv->refresh_timeout_id);
		priv->refresh_timeout_id = 0;
	}

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (fcmdr_service_parent_class)->dispose (object);
}

static void
fcmdr_service_finalize (GObject *object)
{
	FCmdrServicePrivate *priv;

	priv = FCMDR_SERVICE_GET_PRIVATE (object);

	g_hash_table_destroy (priv->backends);
	g_hash_table_destroy (priv->profiles);

	g_mutex_clear (&priv->backends_lock);
	g_mutex_clear (&priv->profiles_lock);
	g_mutex_clear (&priv->sources_lock);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (fcmdr_service_parent_class)->finalize (object);
}

static void
fcmdr_service_constructed (GObject *object)
{
	FCmdrService *service;

	service = FCMDR_SERVICE (object);

	service->priv->login_monitor = fcmdr_logind_monitor_new (service);

	fcmdr_service_init_backends (service);
	fcmdr_service_init_profile_sources (service);

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (fcmdr_service_parent_class)->constructed (object);
}

static gboolean
fcmdr_service_initable_init (GInitable *initable,
                             GCancellable *cancellable,
                             GError **error)
{
	FCmdrServicePrivate *priv;

	priv = FCMDR_SERVICE_GET_PRIVATE (initable);

	/* XXX Likely we'll want to take network availability into
	 *     consideration before attempting to load remote profiles.
	 *     But that can wait until our requirements become clearer.
	 *     For now we just use a simple timeout -- a short delay
	 *     for the initial load, then refresh every hour. */
	priv->refresh_timeout_id = g_timeout_add_seconds (
		3, fcmdr_service_refresh_profiles_start_cb, initable);

	return g_dbus_interface_skeleton_export (
		G_DBUS_INTERFACE_SKELETON (priv->profiles_interface),
		priv->connection, FCMDR_SERVICE_DBUS_OBJECT_PATH, error);
}

static void
fcmdr_service_class_init (FCmdrServiceClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (FCmdrServicePrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = fcmdr_service_set_property;
	object_class->get_property = fcmdr_service_get_property;
	object_class->dispose = fcmdr_service_dispose;
	object_class->constructed = fcmdr_service_constructed;

	g_object_class_install_property (
		object_class,
		PROP_CONNECTION,
		g_param_spec_object (
			"connection",
			"Connection",
			"The system bus connection",
			G_TYPE_DBUS_CONNECTION,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));
}

static void
fcmdr_service_initable_interface_init (GInitableIface *interface)
{
	interface->init = fcmdr_service_initable_init;
}

static void
fcmdr_service_init (FCmdrService *service)
{
	GHashTable *backends;
	GHashTable *profiles;
	SoupURI *uri;

	backends = g_hash_table_new_full (
		(GHashFunc) g_str_hash,
		(GEqualFunc) g_str_equal,
		(GDestroyNotify) g_free,
		(GDestroyNotify) g_object_unref);

	profiles = g_hash_table_new_full (
		(GHashFunc) g_str_hash,
		(GEqualFunc) g_str_equal,
		(GDestroyNotify) g_free,
		(GDestroyNotify) g_object_unref);

	service->priv = FCMDR_SERVICE_GET_PRIVATE (service);
	service->priv->profiles_interface = fcmdr_profiles_skeleton_new ();
	service->priv->backends = backends;
	service->priv->profiles = profiles;

	g_mutex_init (&service->priv->backends_lock);
	g_mutex_init (&service->priv->profiles_lock);
	g_mutex_init (&service->priv->sources_lock);

	g_signal_connect (
		service->priv->profiles_interface,
		"handle-add",
		G_CALLBACK (fcmdr_service_handle_profiles_add_cb),
		service);

	g_signal_connect (
		service->priv->profiles_interface,
		"handle-add-file",
		G_CALLBACK (fcmdr_service_handle_profiles_add_file_cb),
		service);

	g_signal_connect (
		service->priv->profiles_interface,
		"handle-get",
		G_CALLBACK (fcmdr_service_handle_profiles_get_cb),
		service);

	g_signal_connect (
		service->priv->profiles_interface,
		"handle-list",
		G_CALLBACK (fcmdr_service_handle_profiles_list_cb),
		service);
}

FCmdrService *
fcmdr_service_new (GDBusConnection *connection,
                   GError **error)
{
	g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);

	return g_initable_new (
		FCMDR_TYPE_SERVICE, NULL, error,
		"connection", connection, NULL);
}

GDBusConnection *
fcmdr_service_get_connection (FCmdrService *service)
{
	g_return_val_if_fail (FCMDR_IS_SERVICE (service), NULL);

	return service->priv->connection;
}

void
fcmdr_service_user_session_hold (FCmdrService *service,
                                 uid_t uid)
{
	g_return_if_fail (FCMDR_IS_SERVICE (service));
	g_return_if_fail (uid > 0);

	g_print ("%s(%u)\n", G_STRFUNC, uid);
}

void
fcmdr_service_user_session_release (FCmdrService *service,
                                    uid_t uid)
{
	g_return_if_fail (FCMDR_IS_SERVICE (service));
	g_return_if_fail (uid > 0);

	g_print ("%s(%u)\n", G_STRFUNC, uid);
}

FCmdrServiceBackend *
fcmdr_service_ref_backend (FCmdrService *service,
                           const gchar *backend_name)
{
	FCmdrServiceBackend *backend;

	g_return_val_if_fail (FCMDR_IS_SERVICE (service), NULL);
	g_return_val_if_fail (backend_name != NULL, NULL);

	g_mutex_lock (&service->priv->backends_lock);

	backend = g_hash_table_lookup (service->priv->backends, backend_name);

	if (backend != NULL)
		g_object_ref (backend);

	g_mutex_unlock (&service->priv->backends_lock);

	return backend;
}

GList *
fcmdr_service_list_backends (FCmdrService *service)
{
	GList *list;

	g_return_val_if_fail (FCMDR_IS_SERVICE (service), NULL);

	g_mutex_lock (&service->priv->backends_lock);

	list = g_hash_table_get_values (service->priv->backends);

	g_list_foreach (list, (GFunc) g_object_ref, NULL);

	g_mutex_unlock (&service->priv->backends_lock);

	return list;
}

void
fcmdr_service_add_profile (FCmdrService *service,
                           FCmdrProfile *profile)
{
	const gchar *profile_uid;

	g_return_if_fail (FCMDR_IS_SERVICE (service));
	g_return_if_fail (FCMDR_IS_PROFILE (profile));

	g_mutex_lock (&service->priv->profiles_lock);

	profile_uid = fcmdr_profile_get_uid (profile);

	g_hash_table_insert (
		service->priv->profiles,
		g_strdup (profile_uid),
		g_object_ref (profile));

	g_mutex_unlock (&service->priv->profiles_lock);

	/* XXX Just here temporarily for testing. */
	fcmdr_service_apply_profiles (service);
}

FCmdrProfile *
fcmdr_service_ref_profile (FCmdrService *service,
                           const gchar *profile_uid)
{
	FCmdrProfile *profile;

	g_return_val_if_fail (FCMDR_IS_SERVICE (service), NULL);
	g_return_val_if_fail (profile_uid != NULL, NULL);

	g_mutex_lock (&service->priv->profiles_lock);

	profile = g_hash_table_lookup (service->priv->profiles, profile_uid);

	if (profile != NULL)
		g_object_ref (profile);

	g_mutex_unlock (&service->priv->profiles_lock);

	return profile;
}

GList *
fcmdr_service_list_profiles (FCmdrService *service)
{
	GList *list;

	g_return_val_if_fail (FCMDR_IS_SERVICE (service), NULL);

	g_mutex_lock (&service->priv->profiles_lock);

	list = g_hash_table_get_values (service->priv->profiles);

	g_list_foreach (list, (GFunc) g_object_ref, NULL);

	g_mutex_unlock (&service->priv->profiles_lock);

	return list;
}

GList *
fcmdr_service_list_profiles_for_user (FCmdrService *service,
                                      uid_t uid)
{
	g_return_val_if_fail (FCMDR_IS_SERVICE (service), NULL);

	/* FIXME Eventually this will return an ordered list of
	 *       profile objects applicable to 'uid', but for now
	 *       we just return all the profiles. */

	return fcmdr_service_list_profiles (service);
}

FCmdrProfileSource *
fcmdr_service_ref_profile_source (FCmdrService *service,
                                  SoupURI *source_uri)
{
	FCmdrProfileSource *match = NULL;
	GList *list, *link;

	g_return_val_if_fail (FCMDR_IS_SERVICE (service), NULL);
	g_return_val_if_fail (source_uri != NULL, NULL);

	g_mutex_lock (&service->priv->sources_lock);

	list = g_queue_peek_head_link (&service->priv->sources);

	for (link = list; link != NULL; link = g_list_next (link)) {
		FCmdrProfileSource *source;
		SoupURI *uri;
		gboolean uri_match;

		source = FCMDR_PROFILE_SOURCE (link->data);

		uri = fcmdr_profile_source_dup_uri (source);
		uri_match = soup_uri_equal (uri, source_uri);
		soup_uri_free (uri);

		if (uri_match) {
			match = g_object_ref (source);
			break;
		}
	}

	g_mutex_unlock (&service->priv->sources_lock);

	return match;
}

GList *
fcmdr_service_list_profile_sources (FCmdrService *service)
{
	GList *list;

	g_return_val_if_fail (FCMDR_IS_SERVICE (service), NULL);

	g_mutex_lock (&service->priv->sources_lock);

	list = g_queue_peek_head_link (&service->priv->sources);

	list = g_list_copy_deep (list, (GCopyFunc) g_object_ref, NULL);

	g_mutex_unlock (&service->priv->sources_lock);

	return list;
}

/* Helper for fcmdr_service_load_remote_profiles() */
static void
fcmdr_service_load_remote_profiles_cancelled (GCancellable *main_cancellable,
                                              GCancellable *cancellable)
{
	g_cancellable_cancel (cancellable);
}

/* Helper for fcmdr_service_load_remote_profiles() */
static void
fcmdr_service_load_remote_profiles_subtask_done_cb (GObject *source_object,
                                                    GAsyncResult *result,
                                                    gpointer user_data)
{
	FCmdrService *service;
	FCmdrProfileSource *source;
	GTask *main_task;
	GQueue profiles = G_QUEUE_INIT;
	AsyncContext *async_context;
	GError *local_error = NULL;

	source = FCMDR_PROFILE_SOURCE (source_object);
	main_task = G_TASK (user_data);

	service = g_task_get_source_object (main_task);
	async_context = g_task_get_task_data (main_task);

	fcmdr_profile_source_load_remote_finish (
		source, &profiles, result, &local_error);

	if (local_error != NULL) {
		/* Takes ownership of the GError. */
		g_hash_table_replace (
			async_context->errors,
			g_object_ref (source),
			local_error);
		local_error = NULL;
	} else {
		gboolean apply_profiles;

		apply_profiles = fcmdr_service_update_profiles (
			service, source, profiles.head);

		async_context->apply_profiles |= apply_profiles;
	}

	while (!g_queue_is_empty (&profiles))
		g_object_unref (g_queue_pop_head (&profiles));

	g_warn_if_fail (async_context->unfinished_subtasks > 0);

	async_context->unfinished_subtasks--;

	if (async_context->unfinished_subtasks == 0) {
		GCancellable *cancellable;
		guint ii;

		cancellable = g_task_get_cancellable (main_task);

		/* If no cancellable then the array should be empty. */
		for (ii = 0; ii < async_context->cancel_ids->len; ii++) {
			gulong cancel_id;

			cancel_id = g_array_index (
				async_context->cancel_ids, gulong, ii);
			g_cancellable_disconnect (cancellable, cancel_id);
		}

		if (async_context->apply_profiles)
			fcmdr_service_apply_profiles (service);

		/* Failure here is not fatal to the operation. */
		fcmdr_service_cache_profiles (service, &local_error);

		if (local_error != NULL) {
			g_warning (
				"Failed to cache profiles: %s",
				local_error->message);
			g_clear_error (&local_error);
		}

		g_task_return_pointer (
			main_task, async_context->errors,
			(GDestroyNotify) g_hash_table_destroy);
		async_context->errors = NULL;
	}

	g_object_unref (main_task);
}

/* Helper for fcmdr_service_load_remote_profiles() */
static void
fcmdr_service_load_remote_profiles_subtask (gpointer data,
                                            gpointer user_data)
{
	FCmdrProfileSource *source;
	GTask *main_task;
	GCancellable *main_cancellable;
	GCancellable *cancellable = NULL;
	AsyncContext *async_context;

	g_return_if_fail (FCMDR_IS_PROFILE_SOURCE (data));

	source = FCMDR_PROFILE_SOURCE (data);
	main_task = G_TASK (user_data);

	async_context = g_task_get_task_data (main_task);
	main_cancellable = g_task_get_cancellable (main_task);

	if (main_cancellable != NULL) {
		gulong cancel_id;

		cancellable = g_cancellable_new ();
		cancel_id = g_cancellable_connect (
			main_cancellable,
			G_CALLBACK (fcmdr_service_load_remote_profiles_cancelled),
			g_object_ref (cancellable),
			(GDestroyNotify) g_object_unref);
		g_array_append_val (async_context->cancel_ids, cancel_id);
	}

	async_context->unfinished_subtasks++;

	fcmdr_profile_source_load_remote (
		source, cancellable,
		fcmdr_service_load_remote_profiles_subtask_done_cb,
		g_object_ref (main_task));

	g_clear_object (&cancellable);
}

void
fcmdr_service_load_remote_profiles (FCmdrService *service,
                                    GList *profile_sources,
                                    GCancellable *cancellable,
                                    GAsyncReadyCallback callback,
                                    gpointer user_data)
{
	GTask *task;
	AsyncContext *async_context;

	g_return_if_fail (FCMDR_IS_SERVICE (service));

	async_context = g_slice_new0 (AsyncContext);
	async_context->cancel_ids =
		g_array_new (FALSE, FALSE, sizeof (gulong));
	async_context->errors = g_hash_table_new_full (
		(GHashFunc) g_direct_hash,
		(GEqualFunc) g_direct_equal,
		(GDestroyNotify) g_object_unref,
		(GDestroyNotify) g_error_free);

	task = g_task_new (service, cancellable, callback, user_data);
	g_task_set_source_tag (task, fcmdr_service_load_remote_profiles);

	g_task_set_task_data (
		task, async_context, (GDestroyNotify) async_context_free);

	/* Complete the trivial case directly. */
	if (profile_sources == NULL) {
		g_task_return_pointer (
			task, async_context->errors,
			(GDestroyNotify) g_hash_table_destroy);
		async_context->errors = NULL;
	} else {
		g_list_foreach (
			profile_sources,
			fcmdr_service_load_remote_profiles_subtask,
			task);
	}

	g_object_unref (task);
}

GHashTable *
fcmdr_service_load_remote_profiles_finish (FCmdrService *service,
                                           GAsyncResult *result)
{
	g_return_val_if_fail (g_task_is_valid (result, service), NULL);
	g_return_val_if_fail (
		g_async_result_is_tagged (
		result, fcmdr_service_load_remote_profiles), NULL);

	/* This works a little different from the usual async convention.
	 * Since this task dispatches multiple concurrent subtasks, what
	 * we're returning is a hash table of errors for failed subtasks.
	 * The main task itself never fails, which is why we don't take a
	 * GError argument. */

	g_warn_if_fail (!g_task_had_error (G_TASK (result)));

	return g_task_propagate_pointer (G_TASK (result), NULL);
}

gboolean
fcmdr_service_update_profiles (FCmdrService *service,
                               FCmdrProfileSource *source,
                               GList *new_profiles)
{
	GList *old_profiles, *link;
	GHashTable *old_profiles_ht;
	gboolean apply_profiles = FALSE;

	g_return_val_if_fail (FCMDR_IS_SERVICE (service), FALSE);
	g_return_val_if_fail (FCMDR_IS_PROFILE_SOURCE (source), FALSE);

	old_profiles_ht = g_hash_table_new_full (
		(GHashFunc) fcmdr_profile_hash,
		(GEqualFunc) fcmdr_profile_equal,
		(GDestroyNotify) g_object_unref,
		(GDestroyNotify) NULL);

	old_profiles = fcmdr_service_list_profiles (service);

	/* Build a set of profiles belonging to the FCmdrProfileSource. */
	for (link = old_profiles; link != NULL; link = g_list_next (link)) {
		FCmdrProfile *old_profile;
		FCmdrProfileSource *profile_source;

		old_profile = FCMDR_PROFILE (link->data);

		/* This might return NULL. */
		profile_source = fcmdr_profile_ref_source (old_profile);

		if (profile_source == source) {
			g_object_ref (old_profile);
			g_hash_table_add (old_profiles_ht, old_profile);
		}

		g_clear_object (&profile_source);
	}

	g_list_free_full (old_profiles, (GDestroyNotify) g_object_unref);
	old_profiles = NULL;

	g_mutex_lock (&service->priv->profiles_lock);

	/* Add or update profiles, taking etags into account. */
	for (link = new_profiles; link != NULL; link = g_list_next (link)) {
		FCmdrProfile *new_profile;
		FCmdrProfile *old_profile;
		const gchar *profile_uid;
		const gchar *new_etag;
		const gchar *old_etag;

		new_profile = FCMDR_PROFILE (link->data);

		/* Sanity check */
		if (!FCMDR_IS_PROFILE (new_profile)) {
			g_warn_if_reached ();
			continue;
		}

		profile_uid = fcmdr_profile_get_uid (new_profile);

		old_profile = g_hash_table_lookup (
			old_profiles_ht, new_profile);

		if (old_profile == NULL) {
			g_hash_table_replace (
				service->priv->profiles,
				g_strdup (profile_uid),
				g_object_ref (new_profile));
			apply_profiles = TRUE;
			continue;
		}

		new_etag = fcmdr_profile_get_etag (new_profile);
		old_etag = fcmdr_profile_get_etag (old_profile);

		if (g_strcmp0 (new_etag, old_etag) != 0) {
			g_hash_table_replace (
				service->priv->profiles,
				g_strdup (profile_uid),
				g_object_ref (new_profile));
			apply_profiles = TRUE;
			continue;
		}

		g_hash_table_remove (old_profiles_ht, old_profile);
	}

	old_profiles = g_hash_table_get_keys (old_profiles_ht);

	/* Remove any remaining old profiles. */
	for (link = old_profiles; link != NULL; link = g_list_next (link)) {
		FCmdrProfile *old_profile;
		const gchar *profile_uid;

		old_profile = FCMDR_PROFILE (link->data);
		profile_uid = fcmdr_profile_get_uid (old_profile);
		g_hash_table_remove (service->priv->profiles, profile_uid);
		apply_profiles = TRUE;
	}

	g_list_free (old_profiles);

	g_mutex_unlock (&service->priv->profiles_lock);

	g_hash_table_destroy (old_profiles_ht);

	return apply_profiles;
}

void
fcmdr_service_apply_profiles (FCmdrService *service)
{
	GList *backends;
	GList *profiles;
	GList *link;

	g_return_if_fail (FCMDR_IS_SERVICE (service));

	backends = fcmdr_service_list_backends (service);
	profiles = fcmdr_service_list_profiles (service);

	for (link = backends; link != NULL; link = g_list_next (link)) {
		FCmdrServiceBackend *backend;

		backend = FCMDR_SERVICE_BACKEND (link->data);
		fcmdr_service_backend_apply_profiles (backend, profiles);
	}

	g_list_free_full (backends, (GDestroyNotify) g_object_unref);
	g_list_free_full (profiles, (GDestroyNotify) g_object_unref);
}

gboolean
fcmdr_service_cache_profiles (FCmdrService *service,
                              GError **error)
{
	JsonNode *json_node;
	JsonArray *json_array;
	JsonGenerator *generator;
	GList *list, *link;
	gboolean success;

	g_return_if_fail (FCMDR_IS_SERVICE (service));

	json_array = json_array_new ();

	list = fcmdr_service_list_profiles (service);

	for (link = list; link != NULL; link = g_list_next (link)) {
		FCmdrProfile *profile;

		profile = FCMDR_PROFILE (link->data);
		json_node = json_gobject_serialize (G_OBJECT (profile));

		/* This takes ownership of the node. */
		json_array_add_element (json_array, json_node);
	}

	g_list_free_full (list, (GDestroyNotify) g_object_unref);

	generator = json_generator_new ();
	json_generator_set_pretty (generator, TRUE);

	json_node = json_node_new (JSON_NODE_ARRAY);
	json_node_take_array (json_node, json_array);
	json_generator_set_root (generator, json_node);
	json_node_free (json_node);

	g_mkdir_with_parents (FCMDR_SERVICE_CACHE_DIR, 0755);

	success = json_generator_to_file (
		generator, FCMDR_PROFILE_CACHE_FILE, error);

	g_object_unref (generator);

	return success;
}

