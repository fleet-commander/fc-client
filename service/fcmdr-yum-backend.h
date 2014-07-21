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

#ifndef __FCMDR_YUM_BACKEND_H__
#define __FCMDR_YUM_BACKEND_H__

#include "fcmdr-service-backend.h"

/* Standard GObject macros */
#define FCMDR_TYPE_YUM_BACKEND \
	(fcmdr_yum_backend_get_type ())
#define FCMDR_YUM_BACKEND(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), FCMDR_TYPE_YUM_BACKEND, FCmdrYumBackend))
#define FCMDR_YUM_BACKEND_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), FCMDR_TYPE_YUM_BACKEND, FCmdrYumBackendClass))
#define FCMDR_IS_YUM_BACKEND(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), FCMDR_TYPE_YUM_BACKEND))
#define FCMDR_IS_YUM_BACKEND_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), FCMDR_TYPE_YUM_BACKEND))
#define FCMDR_YUM_BACKEND_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), FCMDR_TYPE_YUM_BACKEND, FCmdrYumBackendClass))

G_BEGIN_DECLS

typedef struct _FCmdrYumBackend FCmdrYumBackend;
typedef struct _FCmdrYumBackendClass FCmdrYumBackendClass;
typedef struct _FCmdrYumBackendPrivate FCmdrYumBackendPrivate;

struct _FCmdrYumBackend {
	FCmdrServiceBackend parent;
	FCmdrYumBackendPrivate *priv;
};

struct _FCmdrYumBackendClass {
	FCmdrServiceBackendClass parent_class;
};

GType		fcmdr_yum_backend_get_type	(void) G_GNUC_CONST;

G_END_DECLS

#endif /* __FCMDR_YUM_BACKEND_H__ */

