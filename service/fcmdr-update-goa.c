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

/* This is a short-lived helper program for fcmdr-service that adds
 * GNOME Online Accounts for a user.  The program runs as that user
 * so it can connect to the user's session bus and call methods on
 * the org.gnome.OnlineAccounts service.
 *
 * Profile data is received from fcmdr-service in JSON format through
 * the standard input pipe. */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <gio/gio.h>
#include <gio/gunixinputstream.h>

#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <goa/goa.h>

#include <json-glib/json-glib.h>

static void die (const GError *error) G_GNUC_NORETURN;

static void
die (const GError *error)
{
	g_assert (error != NULL);
	g_error ("%s", error->message);
}

static void
foreach_member_cb (JsonObject *json_object,
                   const gchar *member_name,
                   JsonNode *member_node,
                   gpointer user_data)
{
	GoaClient *goa_client;
	GoaObject *goa_object;
	GoaManager *goa_manager;
	JsonObject *account_data;
	GList *list, *link;
	const gchar *arg_provider = NULL;
	const gchar *arg_identity = NULL;
	const gchar *arg_presentation_identity = NULL;
	GVariantBuilder arg_credentials;
	GVariantBuilder arg_details;
	GError *local_error = NULL;

	goa_client = GOA_CLIENT (user_data);

	/* Skip account IDs already known to the service. */
	goa_object = goa_client_lookup_by_id (goa_client, member_name);
	if (goa_object != NULL) {
		g_debug ("Account '%s' already known", member_name);
		g_object_unref (goa_object);
		return;
	}

	g_return_if_fail (JSON_NODE_HOLDS_OBJECT (member_node));
	account_data = json_node_get_object (member_node);

	/* Credentials dictionary remains empty. */
	g_variant_builder_init (&arg_credentials, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_init (&arg_details, G_VARIANT_TYPE ("a{ss}"));

	list = json_object_get_members (account_data);

	for (link = list; link != NULL; link = g_list_next (link)) {
		const gchar *name, *value;

		name = link->data;
		value = json_object_get_string_member (account_data, name);

		/* Pick out members which are passed separately. */

		if (g_str_equal (name, "Provider")) {
			arg_provider = value;
			continue;
		}

		if (g_str_equal (name, "Identity")) {
			arg_identity = value;
			continue;
		}

		if (g_str_equal (name, "PresentationIdentity")) {
			arg_presentation_identity = value;
			continue;
		}

		g_variant_builder_add (&arg_details, "{ss}", name, value);
	}

	g_list_free (list);

	g_return_if_fail (arg_provider != NULL);
	g_return_if_fail (arg_identity != NULL);
	g_return_if_fail (arg_presentation_identity != NULL);

	/* Lock the account so it can't be removed by the user. */
	g_variant_builder_add (&arg_details, "{ss}", "IsLocked", "true");

	/* Specify our own account ID so it can be recognized later. */
	g_variant_builder_add (&arg_details, "{ss}", "Id", member_name);

	g_debug ("Adding account '%s'", member_name);

	goa_manager = goa_client_get_manager (goa_client);

	goa_manager_call_add_account_sync (
		goa_manager,
		arg_provider,
		arg_identity,
		arg_presentation_identity,
		g_variant_builder_end (&arg_credentials),
		g_variant_builder_end (&arg_details),
		NULL, NULL, &local_error);

	if (local_error != NULL) {
		g_critical (
			"Failed to add account '%s': %s",
			member_name, local_error->message);
		g_error_free (local_error);
	}
}

static void
debug_pretty_print_json (JsonNode *root)
{
	JsonGenerator *generator;
	gchar *json_data;

	generator = json_generator_new ();
	json_generator_set_root (generator, root);
	json_generator_set_pretty (generator, TRUE);
	json_data = json_generator_to_data (generator, NULL);
	g_object_unref (generator);

	g_debug ("%s", json_data);

	g_free (json_data);
}

gint
main (gint argc,
      gchar **argv)
{
	JsonNode *root;
	JsonParser *parser;
	GInputStream *stream;
	GoaClient *goa_client;
	gchar *prgname;
	gulong uid;
	GError *local_error = NULL;

	/* g_log_default_handler() uses this. */
	prgname = g_path_get_basename (argv[0]);
	g_set_prgname (prgname);
	g_free (prgname);

	if (argc != 2) {
		g_printerr ("Usage: %s UID\n", g_get_prgname ());
		return EXIT_FAILURE;
	}

	errno = 0;
	uid = strtoul (argv[1], NULL, 10);
	if (errno != 0) {
		g_printerr ("Invalid UID: %s\n", argv[1]);
		return EXIT_FAILURE;
	}

	g_debug ("Parsing JSON data from standard input");

	parser = json_parser_new ();

	stream = g_unix_input_stream_new (STDIN_FILENO, FALSE);
	json_parser_load_from_stream (parser, stream, NULL, &local_error);
	g_object_unref (stream);

	if (local_error != NULL)
		die (local_error);

	root = json_parser_get_root (parser);
	g_assert (JSON_NODE_HOLDS_OBJECT (root));

	/* XXX Would be nice to know if debugging output is
	 *     suppressed so we could just skip this call.
	 *     See: https://bugzilla.gnome.org/733366 */
	debug_pretty_print_json (root);

	g_debug ("Setting process to user %lu", uid);

	errno = 0;
	if (setuid ((uid_t) uid) != 0) {
		local_error = g_error_new (
			G_IO_ERROR,
			g_io_error_from_errno (errno),
			"setuid(%lu) failed: %s",
			uid, g_strerror (errno));
		die (local_error);
	}

	g_debug ("Connecting to org.gnome.OnlineAccounts");

	goa_client = goa_client_new_sync (NULL, &local_error);

	g_assert (
		((goa_client != NULL) && (local_error == NULL)) ||
		((goa_client == NULL) && (local_error != NULL)));

	if (local_error != NULL)
		die (local_error);

	json_object_foreach_member (
		json_node_get_object (root),
		foreach_member_cb, goa_client);

	g_object_unref (parser);

	g_clear_object (&goa_client);

	g_debug ("All done, exiting");

	return EXIT_SUCCESS;
}

