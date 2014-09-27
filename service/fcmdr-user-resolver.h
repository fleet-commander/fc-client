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

#ifndef __FCMDR_USER_RESOLVER_H__
#define __FCMDR_USER_RESOLVER_H__

#include <gio/gio.h>

/* Standard GObject macros */
#define FCMDR_TYPE_USER_RESOLVER \
	(fcmdr_user_resolver_get_type ())
#define FCMDR_USER_RESOLVER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), FCMDR_TYPE_USER_RESOLVER, FCmdrUserResolver))
#define FCMDR_USER_RESOLVER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), FCMDR_TYPE_USER_RESOLVER, FCmdrUserResolverClass))
#define FCMDR_IS_USER_RESOLVER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), FCMDR_TYPE_USER_RESOLVER))
#define FCMDR_IS_USER_RESOLVER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), FCMDR_TYPE_USER_RESOLVER))
#define FCMDR_USER_RESOLVER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), FCMDR_TYPE_USER_RESOLVER, FCmdrUserResolverClass))

/**
 * FCMDR_USER_RESOLVER_EXTENSION_POINT_NAME:
 *
 * Extension point for methods of resolving account user names.
 **/
#define FCMDR_USER_RESOLVER_EXTENSION_POINT_NAME "fcmdr-user-resolver"

G_BEGIN_DECLS

/* Avoids a circular dependency. */
struct _FCmdrService;

typedef struct _FCmdrUserResolver FCmdrUserResolver;
typedef struct _FCmdrUserResolverClass FCmdrUserResolverClass;
typedef struct _FCmdrUserResolverPrivate FCmdrUserResolverPrivate;

/**
 * FCmdrUserResolver:
 *
 * Contains only private data that should be read and manipulated using the
 * function below.
 **/
struct _FCmdrUserResolver {
	GObject parent;
	FCmdrUserResolverPrivate *priv;
};

struct _FCmdrUserResolverClass {
	GObjectClass parent_class;

	/* Methods */
	void		(*lookup)		(FCmdrUserResolver *resolver,
						 uid_t uid,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
	gboolean	(*lookup_finish)	(FCmdrUserResolver *resolver,
						 GAsyncResult *result,
						 gchar **out_user_name,
						 gchar **out_real_name,
						 GError **error);
};

GType		fcmdr_user_resolver_get_type	(void) G_GNUC_CONST;
FCmdrUserResolver *
		fcmdr_user_resolver_try_new	(struct _FCmdrService *service,
						 const gchar *extension_name);
struct _FCmdrService *
		fcmdr_user_resolver_ref_service	(FCmdrUserResolver *resolver);
void		fcmdr_user_resolver_lookup	(FCmdrUserResolver *resolver,
						 uid_t uid,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gboolean	fcmdr_user_resolver_lookup_finish
						(FCmdrUserResolver *resolver,
						 GAsyncResult *result,
						 gchar **out_user_name,
						 gchar **out_real_name,
						 GError **error);

G_END_DECLS

#endif /* __FCMDR_USER_RESOLVER_H__ */

