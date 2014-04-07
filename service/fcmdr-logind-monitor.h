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

#ifndef __FCMDR_LOGIND_MONITOR_H__
#define __FCMDR_LOGIND_MONITOR_H__

#include "fcmdr-login-monitor.h"

/* Standard GObject macros */
#define FCMDR_TYPE_LOGIND_MONITOR \
	(fcmdr_logind_monitor_get_type ())
#define FCMDR_LOGIND_MONITOR(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), FCMDR_TYPE_LOGIND_MONITOR, FCmdrLogindMonitor))
#define FCMDR_LOGIND_MONITOR_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), FCMDR_TYPE_LOGIND_MONITOR, FCmdrLogindMonitorClass))
#define FCMDR_IS_LOGIND_MONITOR(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), FCMDR_TYPE_LOGIND_MONITOR))
#define FCMDR_IS_LOGIND_MONITOR_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), FCMDR_TYPE_LOGIND_MONITOR))
#define FCMDR_LOGIND_MONITOR_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), FCMDR_TYPE_LOGIND_MONITOR, FCmdrLoginMonitorClass))

G_BEGIN_DECLS

typedef struct _FCmdrLogindMonitor FCmdrLogindMonitor;
typedef struct _FCmdrLogindMonitorClass FCmdrLogindMonitorClass;
typedef struct _FCmdrLogindMonitorPrivate FCmdrLogindMonitorPrivate;

struct _FCmdrLogindMonitor {
	FCmdrLoginMonitor parent;
	FCmdrLogindMonitorPrivate *priv;
};

struct _FCmdrLogindMonitorClass {
	FCmdrLoginMonitorClass parent_class;
};

GType		fcmdr_logind_monitor_get_type	(void) G_GNUC_CONST;
FCmdrLoginMonitor *
		fcmdr_logind_monitor_new	(FCmdrService *service);

G_END_DECLS

#endif /* __FCMDR_LOGIND_MONITOR_H__ */

