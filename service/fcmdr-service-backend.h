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

#ifndef __FCMDR_SERVICE_BACKEND_H__
#define __FCMDR_SERVICE_BACKEND_H__

#include <json-glib/json-glib.h>

#include "fcmdr-profile.h"

/* Standard GObject macros */
#define FCMDR_TYPE_SERVICE_BACKEND \
	(fcmdr_service_backend_get_type ())
#define FCMDR_SERVICE_BACKEND(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), FCMDR_TYPE_SERVICE_BACKEND, FCmdrServiceBackend))
#define FCMDR_SERVICE_BACKEND_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), FCMDR_TYPE_SERVICE_BACKEND, FCmdrServiceBackendClass))
#define FCMDR_IS_SERVICE_BACKEND(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), FCMDR_TYPE_SERVICE_BACKEND))
#define FCMDR_IS_SERVICE_BACKEND_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), FCMDR_TYPE_SERVICE_BACKEND))
#define FCMDR_SERVICE_BACKEND_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), FCMDR_TYPE_SERVICE_BACKEND, FCmdrServiceBackendClass))

/**
 * FCMDR_SERVICE_BACKEND_EXTENSION_POINT_NAME:
 *
 * Extension point for handling different types of settings.
 *
 * When registering extensions for this extension point, the extension
 * name should match a member name that occurs in the #FCmdrProfile:settings
 * data of a #FCmdrProfile, like "org.gnome.gsettings".
 **/
#define FCMDR_SERVICE_BACKEND_EXTENSION_POINT_NAME "fcmdr-service-backend"

G_BEGIN_DECLS

/* Avoids a circular dependency. */
struct _FCmdrService;

typedef struct _FCmdrServiceBackend FCmdrServiceBackend;
typedef struct _FCmdrServiceBackendClass FCmdrServiceBackendClass;
typedef struct _FCmdrServiceBackendPrivate FCmdrServiceBackendPrivate;

/**
 * FCmdrServiceBackend:
 *
 * Contains only private data that should be read and manipulated using the
 * functions below.
 **/
struct _FCmdrServiceBackend {
	GObject parent;
	FCmdrServiceBackendPrivate *priv;
};

struct _FCmdrServiceBackendClass {
	GObjectClass parent_class;

	/* Methods */
	void		(*apply_profiles)	(FCmdrServiceBackend *backend,
						 GList *profiles);
};

GType		fcmdr_service_backend_get_type	(void) G_GNUC_CONST;
struct _FCmdrService *
		fcmdr_service_backend_ref_service
						(FCmdrServiceBackend *backend);
JsonNode *	fcmdr_service_backend_dup_settings
						(FCmdrServiceBackend *backend,
						 FCmdrProfile *profile);
gboolean	fcmdr_service_backend_has_settings
						(FCmdrServiceBackend *backend,
						 FCmdrProfile *profile);
void		fcmdr_service_backend_apply_profiles
						(FCmdrServiceBackend *backend,
						 GList *profiles);
GList *		fcmdr_service_backend_filter_profiles
						(FCmdrServiceBackend *backend,
						 GList *profiles);

G_END_DECLS

#endif /* __FCMDR_SERVICE_BACKEND_H__ */

