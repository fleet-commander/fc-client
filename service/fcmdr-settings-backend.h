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

#ifndef __FCMDR_SETTINGS_BACKEND_H__
#define __FCMDR_SETTINGS_BACKEND_H__

#include <json-glib/json-glib.h>

#include "fcmdr-profile.h"

/* Standard GObject macros */
#define FCMDR_TYPE_SETTINGS_BACKEND \
	(fcmdr_settings_backend_get_type ())
#define FCMDR_SETTINGS_BACKEND(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), FCMDR_TYPE_SETTINGS_BACKEND, FCmdrSettingsBackend))
#define FCMDR_SETTINGS_BACKEND_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), FCMDR_TYPE_SETTINGS_BACKEND, FCmdrSettingsBackendClass))
#define FCMDR_IS_SETTINGS_BACKEND(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), FCMDR_TYPE_SETTINGS_BACKEND))
#define FCMDR_IS_SETTINGS_BACKEND_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), FCMDR_TYPE_SETTINGS_BACKEND))
#define FCMDR_SETTINGS_BACKEND_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), FCMDR_TYPE_SETTINGS_BACKEND, FCmdrSettingsBackendClass))

#define FCMDR_SETTINGS_BACKEND_EXTENSION_POINT_NAME "fcmdr-settings-backend"

G_BEGIN_DECLS

typedef struct _FCmdrSettingsBackend FCmdrSettingsBackend;
typedef struct _FCmdrSettingsBackendClass FCmdrSettingsBackendClass;
typedef struct _FCmdrSettingsBackendPrivate FCmdrSettingsBackendPrivate;

struct _FCmdrSettingsBackend {
	GObject parent;
	FCmdrSettingsBackendPrivate *priv;
};

struct _FCmdrSettingsBackendClass {
	GObjectClass parent_class;

	/* Methods */
	void	(*apply_settings)	(FCmdrSettingsBackend *backend);
};

GType		fcmdr_settings_backend_get_type
					(void) G_GNUC_CONST;
FCmdrProfile *	fcmdr_settings_backend_ref_profile
					(FCmdrSettingsBackend *backend);
JsonNode *	fcmdr_settings_backend_get_settings
					(FCmdrSettingsBackend *backend);
void		fcmdr_settings_backend_apply_settings
					(FCmdrSettingsBackend *backend);

G_END_DECLS

#endif /* __FCMDR_SETTINGS_BACKEND_H__ */

