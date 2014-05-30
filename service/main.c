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

#include <stdlib.h>

#include "fcmdr-service.h"

static GMainLoop *main_loop = NULL;
static FCmdrService *service = NULL;
static gboolean restart_service;

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar *name,
                 gpointer user_data)
{
	GError *local_error = NULL;

	g_debug ("Connected to the system bus");

	service = fcmdr_service_new (connection, &local_error);

	if (local_error != NULL) {
		g_critical (
			"Failed to export interface on system bus: %s",
			local_error->message);
		g_main_loop_quit (main_loop);
	}
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar *name,
                  gpointer user_data)
{
	g_debug ("Acquired the name %s on the system bus", name);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar *name,
              gpointer user_data)
{
	if (service == NULL)
		g_critical ("Failed to connect to the system bus");
	else
		g_critical (
			"Lost (or failed to acquire) the "
			"name %s on the system bus", name);

	g_main_loop_quit (main_loop);
}

static gboolean
on_sighup (gpointer user_data)
{
	g_debug ("Caught SIGHUP - restarting");

	restart_service = TRUE;

	g_main_loop_quit (main_loop);

	return G_SOURCE_CONTINUE;
}

static gboolean
on_sigint (gpointer user_data)
{
	g_debug ("Caught SIGINT - shutting down");

	g_main_loop_quit (main_loop);

	return G_SOURCE_CONTINUE;
}

gint
main (gint argc,
      gchar **argv)
{
	guint sighup_id;
	guint sigint_id;
	guint name_owner_id;

restart:
	restart_service = FALSE;

	main_loop = g_main_loop_new (NULL, FALSE);

	sighup_id = g_unix_signal_add (SIGHUP, on_sighup, NULL);
	sigint_id = g_unix_signal_add (SIGINT, on_sigint, NULL);

	name_owner_id = g_bus_own_name (
		G_BUS_TYPE_SYSTEM,
		"org.gnome.FleetCommander",
		G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
		on_bus_acquired,
		on_name_acquired,
		on_name_lost,
		NULL, (GDestroyNotify) NULL);

	g_main_loop_run (main_loop);

	g_source_remove (sighup_id);
	g_source_remove (sigint_id);
	g_bus_unown_name (name_owner_id);

	g_main_loop_unref (main_loop);
	g_clear_object (&service);

	if (restart_service)
		goto restart;

	return EXIT_SUCCESS;
}
