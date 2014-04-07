/*
 * Copyright (C) 2014 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
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

#include "fcmdr-logind-monitor.h"

#include <sys/types.h>
#include <pwd.h>
#include <string.h>

#define FCMDR_LOGIND_MONITOR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_LOGIND_MONITOR, FCmdrLogindMonitorPrivate))

#define FCMDR_LOGIND_BUS_NAME        "org.freedesktop.login1"
#define FCMDR_LOGIND_OBJECT_PATH     "/org/freedesktop/login1"
#define FCMDR_LOGIND_INTERFACE_NAME  "org.freedesktop.login1.Manager"

struct _FCmdrLogindMonitorPrivate {
	guint name_watch_id;

	GDBusConnection *connection;
	guint user_new_subscription_id;
	guint user_removed_subscription_id;

	/* set of uids */
	GHashTable *active_users;
};

G_DEFINE_TYPE (
	FCmdrLogindMonitor,
	fcmdr_logind_monitor,
	FCMDR_TYPE_LOGIN_MONITOR)

static void
fcmdr_logind_monitor_user_new_cb (GDBusConnection *connection,
                                  const gchar *sender_name,
                                  const gchar *object_path,
                                  const gchar *interface_name,
                                  const gchar *signal_name,
                                  GVariant *parameters,
                                  gpointer user_data)
{
	FCmdrLogindMonitor *monitor;
	guint32 uid = 0;
	struct passwd *pwd;

	monitor = FCMDR_LOGIND_MONITOR (user_data);

	/* The parameters are (uint32: uid, objectpath: path) */
	g_variant_get_child (parameters, 0, "u", &uid);
	g_return_if_fail (uid > 0);

	pwd = getpwuid ((uid_t) uid);

	/* FIXME This is a hack to try to recognize "human" users.
	 *       It checks that the user's home directory contains
	 *       the substring "home".  Eventually we should check
	 *       the UID with the org.freedesktop.Accounts service,
	 *       but this is good enough for now. */
	if (pwd != NULL && strstr (pwd->pw_dir, "home") != NULL) {
		FCmdrService *service;
		gpointer key = GUINT_TO_POINTER (uid);

		service = fcmdr_login_monitor_ref_service (
			FCMDR_LOGIN_MONITOR (monitor));

		g_hash_table_add (monitor->priv->active_users, key);

		fcmdr_service_user_session_hold (service, (uid_t) uid);

		g_clear_object (&service);
	}
}

static void
fcmdr_logind_monitor_user_removed_cb (GDBusConnection *connection,
                                      const gchar *sender_name,
                                      const gchar *object_path,
                                      const gchar *interface_name,
                                      const gchar *signal_name,
                                      GVariant *parameters,
                                      gpointer user_data)
{
	FCmdrLogindMonitor *monitor;
	guint32 uid = 0;
	gpointer key;

	monitor = FCMDR_LOGIND_MONITOR (user_data);

	/* The parameters are (uint32: uid, objectpath: path) */
	g_variant_get_child (parameters, 0, "u", &uid);
	g_return_if_fail (uid > 0);

	key = GUINT_TO_POINTER (uid);

	if (g_hash_table_contains (monitor->priv->active_users, key)) {
		FCmdrService *service;
		gchar *param_info;

		service = fcmdr_login_monitor_ref_service (
			FCMDR_LOGIN_MONITOR (monitor));

		g_hash_table_remove (monitor->priv->active_users, key);

		fcmdr_service_user_session_release (service, (uid_t) uid);

		g_clear_object (&service);
	}
}

static void
fcmdr_logind_monitor_name_appeared_cb (GDBusConnection *connection,
                                       const gchar *bus_name,
                                       const gchar *bus_name_owner,
                                       gpointer user_data)
{
	FCmdrLogindMonitor *monitor;

	monitor = FCMDR_LOGIND_MONITOR (user_data);

	/* Stash the connection so we can unsubscribe in dispose(). */
	g_clear_object (&monitor->priv->connection);
	monitor->priv->connection = g_object_ref (connection);

	monitor->priv->user_new_subscription_id =
		g_dbus_connection_signal_subscribe (
			connection,
			bus_name_owner,
			FCMDR_LOGIND_INTERFACE_NAME,
			"UserNew",
			FCMDR_LOGIND_OBJECT_PATH,
			NULL,
			G_DBUS_SIGNAL_FLAGS_NONE,
			fcmdr_logind_monitor_user_new_cb,
			monitor, (GDestroyNotify) NULL);

	monitor->priv->user_removed_subscription_id =
		g_dbus_connection_signal_subscribe (
			connection,
			bus_name_owner,
			FCMDR_LOGIND_INTERFACE_NAME,
			"UserRemoved",
			FCMDR_LOGIND_OBJECT_PATH,
			NULL,
			G_DBUS_SIGNAL_FLAGS_NONE,
			fcmdr_logind_monitor_user_removed_cb,
			monitor, (GDestroyNotify) NULL);
}

static void
fcmdr_logind_monitor_name_vanished_cb (GDBusConnection *connection,
                                       const gchar *bus_name,
                                       gpointer user_data)
{
	FCmdrLogindMonitor *monitor;
	guint subscription_id;

	monitor = FCMDR_LOGIND_MONITOR (user_data);

	subscription_id = monitor->priv->user_new_subscription_id;
	monitor->priv->user_new_subscription_id = 0;

	if (subscription_id > 0)
		g_dbus_connection_signal_unsubscribe (
			connection, subscription_id);

	subscription_id = monitor->priv->user_removed_subscription_id;
	monitor->priv->user_removed_subscription_id = 0;

	if (subscription_id > 0)
		g_dbus_connection_signal_unsubscribe (
			connection, subscription_id);
}

static void
fcmdr_logind_monitor_dispose (GObject *object)
{
	FCmdrLogindMonitorPrivate *priv;

	priv = FCMDR_LOGIND_MONITOR_GET_PRIVATE (object);

	if (priv->user_new_subscription_id > 0) {
		g_dbus_connection_signal_unsubscribe (
			priv->connection,
			priv->user_new_subscription_id);
		priv->user_new_subscription_id = 0;
	}

	if (priv->user_removed_subscription_id > 0) {
		g_dbus_connection_signal_unsubscribe (
			priv->connection,
			priv->user_removed_subscription_id);
		priv->user_removed_subscription_id = 0;
	}

	g_clear_object (&priv->connection);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (fcmdr_logind_monitor_parent_class)->dispose (object);
}

static void
fcmdr_logind_monitor_finalize (GObject *object)
{
	FCmdrLogindMonitorPrivate *priv;

	priv = FCMDR_LOGIND_MONITOR_GET_PRIVATE (object);

	g_bus_unwatch_name (priv->name_watch_id);

	g_hash_table_destroy (priv->active_users);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (fcmdr_logind_monitor_parent_class)->finalize (object);
}

static void
fcmdr_logind_monitor_constructed (GObject *object)
{
	FCmdrLogindMonitor *monitor;
	guint subscription_id;

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (fcmdr_logind_monitor_parent_class)->
		constructed (object);

	monitor = FCMDR_LOGIND_MONITOR (object);

	monitor->priv->name_watch_id = g_bus_watch_name (
		G_BUS_TYPE_SYSTEM,
		FCMDR_LOGIND_BUS_NAME,
		G_BUS_NAME_WATCHER_FLAGS_NONE,
		fcmdr_logind_monitor_name_appeared_cb,
		fcmdr_logind_monitor_name_vanished_cb,
		monitor, (GDestroyNotify) NULL);
}

static void
fcmdr_logind_monitor_class_init (FCmdrLogindMonitorClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (FCmdrLogindMonitorPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->dispose = fcmdr_logind_monitor_dispose;
	object_class->finalize = fcmdr_logind_monitor_finalize;
	object_class->constructed = fcmdr_logind_monitor_constructed;
}

static void
fcmdr_logind_monitor_init (FCmdrLogindMonitor *monitor)
{
	monitor->priv = FCMDR_LOGIND_MONITOR_GET_PRIVATE (monitor);
	monitor->priv->active_users = g_hash_table_new (NULL, NULL);
}

FCmdrLoginMonitor *
fcmdr_logind_monitor_new (FCmdrService *service)
{
	g_return_val_if_fail (FCMDR_IS_SERVICE (service), NULL);

	return g_object_new (
		FCMDR_TYPE_LOGIND_MONITOR,
		"service", service, NULL);
}

