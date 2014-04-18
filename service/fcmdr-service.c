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

#include "fcmdr-generated.h"
#include "fcmdr-logind-monitor.h"

#define FCMDR_SERVICE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_SERVICE, FCmdrServicePrivate))

#define DBUS_OBJECT_PATH "/org/gnome/FleetCommander"

typedef struct _AsyncContext AsyncContext;

struct _FCmdrServicePrivate {
	GDBusConnection *connection;
	FCmdrProfiles *profiles_interface;
	FCmdrLoginMonitor *login_monitor;

	/* Profile UID -> FCmdrProfile */
	GHashTable *profiles;
	GMutex profiles_lock;
};

struct _AsyncContext {
	GDBusInterface *interface;
	GDBusMethodInvocation *invocation;
	FCmdrService *service;
};

enum {
	PROP_0,
	PROP_CONNECTION
};

/* Forward Declarations */
static void	fcmdr_service_initable_interface_init
						(GInitableIface *interface);

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

	g_slice_free (AsyncContext, async_context);
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

	g_hash_table_remove_all (priv->profiles);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (fcmdr_service_parent_class)->dispose (object);
}

static void
fcmdr_service_finalize (GObject *object)
{
	FCmdrServicePrivate *priv;

	priv = FCMDR_SERVICE_GET_PRIVATE (object);

	g_hash_table_destroy (priv->profiles);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (fcmdr_service_parent_class)->finalize (object);
}

static void
fcmdr_service_constructed (GObject *object)
{
	FCmdrService *service;

	service = FCMDR_SERVICE (object);

	service->priv->login_monitor = fcmdr_logind_monitor_new (service);

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (fcmdr_service_parent_class)->constructed (object);
}

static gboolean
fcmdr_service_initable_init (GInitable *initable,
                             GCancellable *cancellable,
                             GError **error)
{
	FCmdrServicePrivate *priv;
	gboolean success;

	priv = FCMDR_SERVICE_GET_PRIVATE (initable);

	return g_dbus_interface_skeleton_export (
		G_DBUS_INTERFACE_SKELETON (priv->profiles_interface),
		priv->connection, DBUS_OBJECT_PATH, error);
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
	GHashTable *profiles;

	profiles = g_hash_table_new_full (
		(GHashFunc) g_str_hash,
		(GEqualFunc) g_str_equal,
		(GDestroyNotify) g_free,
		(GDestroyNotify) g_object_unref);

	service->priv = FCMDR_SERVICE_GET_PRIVATE (service);
	service->priv->profiles_interface = fcmdr_profiles_skeleton_new ();
	service->priv->profiles = profiles;

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

