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

struct _FCmdrServicePrivate {
	GDBusConnection *connection;
	FCmdrMain *interface;
	FCmdrLoginMonitor *login_monitor;
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

static gboolean
fcmdr_service_handle_add_session_cb (FCmdrMain *interface,
                                     GDBusMethodInvocation *invocation,
                                     const gchar *user,
                                     FCmdrService *service)
{
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
	g_clear_object (&priv->interface);
	g_clear_object (&priv->login_monitor);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (fcmdr_service_parent_class)->dispose (object);
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
		G_DBUS_INTERFACE_SKELETON (priv->interface),
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
	service->priv = FCMDR_SERVICE_GET_PRIVATE (service);
	service->priv->interface = fcmdr_main_skeleton_new ();

	g_signal_connect (
		service->priv->interface, "handle-add-session",
		G_CALLBACK (fcmdr_service_handle_add_session_cb),
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

