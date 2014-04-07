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

#include "fcmdr-login-monitor.h"

#define FCMDR_LOGIN_MONITOR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_LOGIN_MONITOR, FCmdrLoginMonitorPrivate))

struct _FCmdrLoginMonitorPrivate {
	GWeakRef service;
};

enum {
	PROP_0,
	PROP_SERVICE
};

G_DEFINE_ABSTRACT_TYPE (
	FCmdrLoginMonitor,
	fcmdr_login_monitor,
	G_TYPE_OBJECT)

static void
fcmdr_login_monitor_set_service (FCmdrLoginMonitor *monitor,
                                 FCmdrService *service)
{
	g_return_if_fail (FCMDR_IS_SERVICE (service));

	g_weak_ref_set (&monitor->priv->service, service);
}

static void
fcmdr_login_monitor_set_property (GObject *object,
                                  guint property_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_SERVICE:
			fcmdr_login_monitor_set_service (
				FCMDR_LOGIN_MONITOR (object),
				g_value_get_object (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_login_monitor_get_property (GObject *object,
                                  guint property_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_SERVICE:
			g_value_take_object (
				value,
				fcmdr_login_monitor_ref_service (
				FCMDR_LOGIN_MONITOR (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_login_monitor_dispose (GObject *object)
{
	FCmdrLoginMonitorPrivate *priv;

	priv = FCMDR_LOGIN_MONITOR_GET_PRIVATE (object);

	g_weak_ref_set (&priv->service, NULL);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (fcmdr_login_monitor_parent_class)->dispose (object);
}

static void
fcmdr_login_monitor_class_init (FCmdrLoginMonitorClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (FCmdrLoginMonitorPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = fcmdr_login_monitor_set_property;
	object_class->get_property = fcmdr_login_monitor_get_property;
	object_class->dispose = fcmdr_login_monitor_dispose;

	g_object_class_install_property (
		object_class,
		PROP_SERVICE,
		g_param_spec_object (
			"service",
			"Service",
			"The central service object",
			FCMDR_TYPE_SERVICE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));
}

static void
fcmdr_login_monitor_init (FCmdrLoginMonitor *monitor)
{
	monitor->priv = FCMDR_LOGIN_MONITOR_GET_PRIVATE (monitor);
}

FCmdrService *
fcmdr_login_monitor_ref_service (FCmdrLoginMonitor *monitor)
{
	g_return_val_if_fail (FCMDR_IS_LOGIN_MONITOR (monitor), NULL);

	return g_weak_ref_get (&monitor->priv->service);
}

