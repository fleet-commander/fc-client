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

#ifndef __FCMDR_SERVICE_H__
#define __FCMDR_SERVICE_H__

#include <gio/gio.h>

/* Standard GObject macros */
#define FCMDR_TYPE_SERVICE \
	(fcmdr_service_get_type ())
#define FCMDR_SERVICE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), FCMDR_TYPE_SERVICE, FCmdrService))
#define FCMDR_IS_SERVICE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), FCMDR_TYPE_SERVICE))

G_BEGIN_DECLS

typedef struct _FCmdrService FCmdrService;
typedef struct _FCmdrServiceClass FCmdrServiceClass;
typedef struct _FCmdrServicePrivate FCmdrServicePrivate;

struct _FCmdrService {
	GObject parent;
	FCmdrServicePrivate *priv;
};

struct _FCmdrServiceClass {
	GObjectClass parent_class;
};

GType		fcmdr_service_get_type		(void) G_GNUC_CONST;
FCmdrService *	fcmdr_service_new		(GDBusConnection *connection,
						 GError **error);
GDBusConnection *
		fcmdr_service_get_connection	(FCmdrService *service);
void		fcmdr_service_user_session_hold	(FCmdrService *service,
						 uid_t uid);
void		fcmdr_service_user_session_release
						(FCmdrService *service,
						 uid_t uid);

G_END_DECLS

#endif /* __FCMDR_SERVICE_H__ */

