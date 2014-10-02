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

#ifndef __FCMDR_LOCAL_USER_RESOLVER_H__
#define __FCMDR_LOCAL_USER_RESOLVER_H__

#include "fcmdr-user-resolver.h"

/* Standard GObject macros */
#define FCMDR_TYPE_LOCAL_USER_RESOLVER \
	(fcmdr_local_user_resolver_get_type ())
#define FCMDR_LOCAL_USER_RESOLVER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), FCMDR_TYPE_LOCAL_USER_RESOLVER, FCmdrLocalUserResolver))
#define FCMDR_LOCAL_USER_RESOLVER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), FCMDR_TYPE_LOCAL_USER_RESOLVER, FCmdrLocalUserResolverClass))
#define FCMDR_IS_LOCAL_USER_RESOLVER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), FCMDR_TYPE_LOCAL_USER_RESOLVER))
#define FCMDR_IS_LOCAL_USER_RESOLVER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), FCMDR_TYPE_LOCAL_USER_RESOLVER))
#define FCMDR_LOCAL_USER_RESOLVER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), FCMDR_TYPE_LOCAL_USER_RESOLVER, FCmdrLocalUserResolverClass))

G_BEGIN_DECLS

typedef struct _FCmdrLocalUserResolver FCmdrLocalUserResolver;
typedef struct _FCmdrLocalUserResolverClass FCmdrLocalUserResolverClass;
typedef struct _FCmdrLocalUserResolverPrivate FCmdrLocalUserResolverPrivate;

struct _FCmdrLocalUserResolver {
	FCmdrUserResolver parent;
	FCmdrLocalUserResolverPrivate *priv;
};

struct _FCmdrLocalUserResolverClass {
	FCmdrUserResolverClass parent_class;
};

GType		fcmdr_local_user_resolver_get_type
						(void) G_GNUC_CONST;

G_END_DECLS

#endif /* __FCMDR_LOCAL_USER_RESOLVER_H__ */

