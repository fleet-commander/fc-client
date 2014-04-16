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

#include "fcmdr-extensions.h"

#include "fcmdr-settings-backend.h"

/* Helper for fcmdr_ensure_extension_points_registered() */
static gpointer
register_extension_points (gpointer unused)
{
	GIOExtensionPoint *extension_point;

	extension_point = g_io_extension_point_register (
		FCMDR_SETTINGS_BACKEND_EXTENSION_POINT_NAME);
	g_io_extension_point_set_required_type (
		extension_point, FCMDR_TYPE_SETTINGS_BACKEND);

	return NULL;
}

void
fcmdr_ensure_extension_points_registered (void)
{
	static GOnce extension_points_registered = G_ONCE_INIT;

	g_once (&extension_points_registered, register_extension_points, NULL);
}

void
fcmdr_ensure_extensions_registered (void)
{
	/* Register GTypes for built-in extensions. */
}
