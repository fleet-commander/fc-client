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

#ifndef __FCMDR_LOGIN_MONITOR_H__
#define __FCMDR_LOGIN_MONITOR_H__

#include "fcmdr-service.h"

/* Standard GObject macros */
#define FCMDR_TYPE_LOGIN_MONITOR \
	(fcmdr_login_monitor_get_type ())
#define FCMDR_LOGIN_MONITOR(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), FCMDR_TYPE_LOGIN_MONITOR, FCmdrLoginMonitor))
#define FCMDR_LOGIN_MONITOR_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), FCMDR_TYPE_LOGIN_MONITOR, FCmdrLoginMonitorClass))
#define FCMDR_IS_LOGIN_MONITOR(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), FCMDR_TYPE_LOGIN_MONITOR))
#define FCMDIR_IS_LOGIN_MONITOR_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), FCMDR_TYPE_LOGIN_MONITOR))
#define FCMDIR_LOGIN_MONITOR_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), FCMDR_TYPE_LOGIN_MONITOR, FCmdrLoginMonitorClass))

G_BEGIN_DECLS

typedef struct _FCmdrLoginMonitor FCmdrLoginMonitor;
typedef struct _FCmdrLoginMonitorClass FCmdrLoginMonitorClass;
typedef struct _FCmdrLoginMonitorPrivate FCmdrLoginMonitorPrivate;

struct _FCmdrLoginMonitor {
	GObject parent;
	FCmdrLoginMonitorPrivate *priv;
};

struct _FCmdrLoginMonitorClass {
	GObjectClass parent_class;
};

GType		fcmdr_login_monitor_get_type	(void) G_GNUC_CONST;
FCmdrService *	fcmdr_login_monitor_ref_service	(FCmdrLoginMonitor *monitor);

G_END_DECLS

#endif /* __FCMDR_LOGIN_MONITOR_H__ */

