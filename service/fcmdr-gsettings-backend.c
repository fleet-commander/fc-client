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

#include "config.h"

#include "fcmdr-gsettings-backend.h"

#include "fcmdr-extensions.h"

#define FCMDR_GSETTINGS_BACKEND_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_GSETTINGS_BACKEND, FCmdrGSettingsBackendPrivate))

struct _FCmdrGSettingsBackendPrivate {
	gint placeholder;
};

G_DEFINE_TYPE_WITH_CODE (
	FCmdrGSettingsBackend,
	fcmdr_gsettings_backend,
	FCMDR_TYPE_SETTINGS_BACKEND,
	fcmdr_ensure_extension_points_registered();
	g_io_extension_point_implement (
		FCMDR_SETTINGS_BACKEND_EXTENSION_POINT_NAME,
		g_define_type_id,
		"org.gnome.gsettings", 0))

static void
fcmdr_gsettings_backend_class_init (FCmdrGSettingsBackendClass *class)
{
	g_type_class_add_private (
		class, sizeof (FCmdrGSettingsBackendPrivate));
}

static void
fcmdr_gsettings_backend_init (FCmdrGSettingsBackend *backend)
{
	backend->priv = FCMDR_GSETTINGS_BACKEND_GET_PRIVATE (backend);
}

