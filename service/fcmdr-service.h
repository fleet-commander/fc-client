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

#ifndef __FCMDR_SERVICE_H__
#define __FCMDR_SERVICE_H__

#include <gio/gio.h>

#include "fcmdr-profile.h"
#include "fcmdr-profile-source.h"
#include "fcmdr-service-backend.h"

/* Standard GObject macros */
#define FCMDR_TYPE_SERVICE \
	(fcmdr_service_get_type ())
#define FCMDR_SERVICE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), FCMDR_TYPE_SERVICE, FCmdrService))
#define FCMDR_IS_SERVICE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), FCMDR_TYPE_SERVICE))

/**
 * FCMDR_SERVICE_DBUS_OBJECT_PATH:
 *
 * The D-Bus object path on which the #FCmdrService is exported.
 **/
#define FCMDR_SERVICE_DBUS_OBJECT_PATH "/org/gnome/FleetCommander"

G_BEGIN_DECLS

typedef struct _FCmdrService FCmdrService;
typedef struct _FCmdrServiceClass FCmdrServiceClass;
typedef struct _FCmdrServicePrivate FCmdrServicePrivate;

/**
 * FCmdrService:
 *
 * Contains only private data that should be read and manipulated using the
 * functions below.
 **/
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
FCmdrServiceBackend *
		fcmdr_service_ref_backend	(FCmdrService *service,
						 const gchar *backend_name);
GList *		fcmdr_service_list_backends	(FCmdrService *service);
void		fcmdr_service_add_profile	(FCmdrService *service,
						 FCmdrProfile *profile);
FCmdrProfile *	fcmdr_service_ref_profile	(FCmdrService *service,
						 const gchar *profile_uid);
GList *		fcmdr_service_list_profiles	(FCmdrService *service);
GList *		fcmdr_service_list_profiles_for_user
						(FCmdrService *service,
						 uid_t uid);
FCmdrProfileSource *
		fcmdr_service_ref_profile_source
						(FCmdrService *service,
						 SoupURI *source_uri);
GList *		fcmdr_service_list_profile_sources
						(FCmdrService *service);
void		fcmdr_service_load_remote_profiles
						(FCmdrService *service,
						 GList *profile_sources,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
GHashTable *	fcmdr_service_load_remote_profiles_finish
						(FCmdrService *service,
						 GAsyncResult *result);
gboolean	fcmdr_service_update_profiles	(FCmdrService *service,
						 FCmdrProfileSource *source,
						 GList *new_profiles);
void		fcmdr_service_apply_profiles	(FCmdrService *service);
gboolean	fcmdr_service_cache_profiles	(FCmdrService *service,
						 GError **error);

G_END_DECLS

#endif /* __FCMDR_SERVICE_H__ */

