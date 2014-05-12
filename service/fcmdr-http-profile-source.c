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

#include <libsoup/soup.h>

#include "fcmdr-http-profile-source.h"

#include "fcmdr-extensions.h"
#include "fcmdr-service.h"

#define FCMDR_HTTP_PROFILE_SOURCE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_HTTP_PROFILE_SOURCE, FCmdrHttpProfileSourcePrivate))

struct _FCmdrHttpProfileSourcePrivate {
	SoupSession *session;
};

G_DEFINE_TYPE_WITH_CODE (
	FCmdrHttpProfileSource,
	fcmdr_http_profile_source,
	FCMDR_TYPE_PROFILE_SOURCE,
	fcmdr_ensure_extension_points_registered ();
	g_io_extension_point_implement (
		FCMDR_PROFILE_SOURCE_EXTENSION_POINT_NAME,
		g_define_type_id,
		"http", 0))

static gboolean
fcmdr_http_profile_source_get_uris (FCmdrProfileSource *source,
                                    GQueue *out_uris,
                                    GError **error)
{
	FCmdrHttpProfileSourcePrivate *priv;
	SoupURI *base_uri;
	SoupRequest *request;
	JsonNode *json_node;
	JsonArray *json_array;
	JsonParser *json_parser;
	GList *list, *link;
	gboolean success = FALSE;

	priv = FCMDR_HTTP_PROFILE_SOURCE_GET_PRIVATE (source);

	json_parser = json_parser_new ();

	base_uri = fcmdr_profile_source_dup_uri (source);

	request = soup_session_request_uri (priv->session, base_uri, error);

	if (request != NULL) {
		GInputStream *stream;

		stream = soup_request_send (request, NULL, error);

		if (stream != NULL) {
			success = json_parser_load_from_stream (
				json_parser, stream, NULL, error);
			g_object_unref (stream);
		}

		g_object_unref (request);
	}

	if (!success)
		goto exit;

	json_node = json_parser_get_root (json_parser);

	if (!JSON_NODE_HOLDS_ARRAY (json_node)) {
		g_set_error (
			error, JSON_PARSER_ERROR,
			JSON_PARSER_ERROR_PARSE,
			"Expecting a JSON array, "
			"but the root node is of type '%s'",
			json_node_type_name (json_node));
		success = FALSE;
		goto exit;
	}

	json_array = json_node_get_array (json_node);
	list = json_array_get_elements (json_array);

	for (link = list; link != NULL; link = g_list_next (link)) {
		JsonObject *json_object;
		const gchar *filename;
		SoupURI *profile_uri;

		json_node = link->data;

		if (!JSON_NODE_HOLDS_OBJECT (json_node)) {
			g_warning (
				"Expecting a JSON array of objects, "
				"but encountered element of type '%s'",
				json_node_type_name (json_node));
			continue;
		}

		json_object = json_node_get_object (json_node);
		filename = json_object_get_string_member (json_object, "url");

		if (filename == NULL) {
			g_warning (
				"Skipping object with no 'url' member "
				"in JSON array");
			continue;
		}

		profile_uri = soup_uri_new_with_base (base_uri, filename);
		g_queue_push_tail (out_uris, profile_uri);
	}

	g_list_free (list);

exit:
	g_object_unref (json_parser);

	soup_uri_free (base_uri);

	return success;
}

static FCmdrProfile *
fcmdr_http_profile_source_get_profile (FCmdrProfileSource *source,
                                       SoupURI *uri,
                                       GError **error)
{
	FCmdrHttpProfileSourcePrivate *priv;
	FCmdrProfile *profile = NULL;
	SoupRequest *request;

	priv = FCMDR_HTTP_PROFILE_SOURCE_GET_PRIVATE (source);

	request = soup_session_request_uri (priv->session, uri, error);

	if (request != NULL) {
		GInputStream *stream;

		stream = soup_request_send (request, NULL, error);

		if (stream != NULL) {
			profile = fcmdr_profile_new_from_stream (
				stream, NULL, error);
			g_object_unref (stream);
		}

		g_object_unref (request);
	}

	return profile;
}

static gpointer
fcmdr_http_profile_source_load_remote_thread (gpointer user_data)
{
	FCmdrProfileSource *source;
	FCmdrHttpProfileSourcePrivate *priv;
	FCmdrService *service;
	GQueue uris = G_QUEUE_INIT;
	GHashTable *profiles;
	GError *local_error = NULL;

	source = FCMDR_PROFILE_SOURCE (user_data);
	priv = FCMDR_HTTP_PROFILE_SOURCE_GET_PRIVATE (source);

	service = fcmdr_profile_source_ref_service (source);
	g_return_val_if_fail (service != NULL, NULL);

	if (!fcmdr_http_profile_source_get_uris (source, &uris, &local_error))
		goto exit;

	profiles = g_hash_table_new_full (
		(GHashFunc) fcmdr_profile_hash,
		(GEqualFunc) fcmdr_profile_equal,
		(GDestroyNotify) g_object_unref,
		(GDestroyNotify) NULL);

	while (!g_queue_is_empty (&uris)) {
		FCmdrProfile *profile;
		SoupURI *uri;

		uri = g_queue_pop_head (&uris);

		profile = fcmdr_http_profile_source_get_profile (
			source, uri, &local_error);

		/* Sanity check */
		g_warn_if_fail (
			((profile != NULL) && (local_error == NULL)) ||
			((profile == NULL) && (local_error != NULL)));

		if (profile != NULL)
			g_hash_table_add (profiles, profile);

		if (local_error != NULL) {
			gchar *uri_string;

			uri_string = soup_uri_to_string (uri, FALSE);
			g_warning ("%s: %s", uri_string, local_error->message);
			g_free (uri_string);

			g_clear_error (&local_error);
		}
	}

exit:
	if (local_error != NULL) {
		g_warning ("%s: %s", G_STRFUNC, local_error->message);
		g_error_free (local_error);
	}

	g_object_unref (source);

	return NULL;
}

static void
fcmdr_http_profile_source_dispose (GObject *object)
{
	FCmdrHttpProfileSourcePrivate *priv;

	priv = FCMDR_HTTP_PROFILE_SOURCE_GET_PRIVATE (object);

	g_clear_object (&priv->session);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (fcmdr_http_profile_source_parent_class)->
		dispose (object);
}

static void
fcmdr_http_profile_source_load_cached (FCmdrProfileSource *source,
                                       const gchar *path)
{
}

static void
fcmdr_http_profile_source_load_remote (FCmdrProfileSource *source)
{
	GThread *thread;
	GError *local_error = NULL;

	thread = g_thread_try_new (
		"http-load-remote-thread",
		fcmdr_http_profile_source_load_remote_thread,
		g_object_ref (source),
		&local_error);

	/* Sanity check */
	g_warn_if_fail (
		((thread != NULL) && (local_error == NULL)) ||
		((thread == NULL) && (local_error != NULL)));

	if (thread != NULL)
		g_thread_unref (thread);

	if (local_error != NULL) {
		g_critical ("%s: %s", G_STRFUNC, local_error->message);
		g_error_free (local_error);
	}
}

static void
fcmdr_http_profile_source_class_init (FCmdrHttpProfileSourceClass *class)
{
	GObjectClass *object_class;
	FCmdrProfileSourceClass *source_class;

	g_type_class_add_private (
		class, sizeof (FCmdrHttpProfileSourcePrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->dispose = fcmdr_http_profile_source_dispose;

	source_class = FCMDR_PROFILE_SOURCE_CLASS (class);
	source_class->load_cached = fcmdr_http_profile_source_load_cached;
	source_class->load_remote = fcmdr_http_profile_source_load_remote;
}

static void
fcmdr_http_profile_source_init (FCmdrHttpProfileSource *source)
{
	source->priv = FCMDR_HTTP_PROFILE_SOURCE_GET_PRIVATE (source);

	source->priv->session = soup_session_new ();
}

