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

typedef struct _AsyncContext AsyncContext;

struct _FCmdrHttpProfileSourcePrivate {
	SoupSession *session;
};

struct _AsyncContext {
	SoupURI *uri;
	GHashTable *profiles;
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

static void
async_context_free (AsyncContext *async_context)
{
	soup_uri_free (async_context->uri);
	g_hash_table_destroy (async_context->profiles);

	g_slice_free (AsyncContext, async_context);
}

static gboolean
fcmdr_http_profile_source_get_uris (SoupSession *session,
                                    SoupURI *base_uri,
                                    GQueue *out_uris,
                                    GCancellable *cancellable,
                                    GError **error)
{
	SoupRequest *request;
	JsonNode *json_node;
	JsonArray *json_array;
	JsonParser *json_parser;
	GList *list, *link;
	gboolean success = FALSE;

	json_parser = json_parser_new ();

	request = soup_session_request_uri (session, base_uri, error);

	if (request != NULL) {
		GInputStream *stream;

		stream = soup_request_send (request, cancellable, error);

		if (stream != NULL) {
			success = json_parser_load_from_stream (
				json_parser, stream, cancellable, error);
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

	return success;
}

static FCmdrProfile *
fcmdr_http_profile_source_get_profile (SoupSession *session,
                                       SoupURI *uri,
                                       GCancellable *cancellable,
                                       GError **error)
{
	FCmdrProfile *profile = NULL;
	SoupRequest *request;

	request = soup_session_request_uri (session, uri, error);

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

static void
fcmdr_http_profile_source_load_remote_thread (GTask *task,
                                              gpointer source_object,
                                              gpointer task_data,
                                              GCancellable *cancellable)
{
	FCmdrHttpProfileSource *source;
	GQueue uris = G_QUEUE_INIT;
	AsyncContext *async_context;
	GError *local_error = NULL;

	source = FCMDR_HTTP_PROFILE_SOURCE (source_object);
	async_context = (AsyncContext *) task_data;

	fcmdr_http_profile_source_get_uris (
		source->priv->session,
		async_context->uri, &uris,
		cancellable, &local_error);

	if (local_error != NULL)
		goto exit;

	while (!g_queue_is_empty (&uris)) {
		FCmdrProfile *profile;
		SoupURI *uri;

		uri = g_queue_pop_head (&uris);

		profile = fcmdr_http_profile_source_get_profile (
			source->priv->session, uri, cancellable, &local_error);

		/* Sanity check */
		g_warn_if_fail (
			((profile != NULL) && (local_error == NULL)) ||
			((profile == NULL) && (local_error != NULL)));

		if (profile != NULL) {
			fcmdr_profile_set_source (
				profile, FCMDR_PROFILE_SOURCE (source));
			g_hash_table_add (
				async_context->profiles,
				g_object_ref (profile));
			g_object_unref (profile);
		}

		/* Errors fetching individual profiles are not fatal to
		 * the class method, but we do want to warn about them. */
		if (local_error != NULL) {
			gchar *uri_string;

			uri_string = soup_uri_to_string (uri, FALSE);
			g_warning ("%s: %s", uri_string, local_error->message);
			g_free (uri_string);

			g_clear_error (&local_error);
		}

		soup_uri_free (uri);
	}

exit:
	g_warn_if_fail (g_queue_is_empty (&uris));

	if (local_error != NULL)
		g_task_return_error (task, local_error);
	else
		g_task_return_boolean (task, TRUE);
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
fcmdr_http_profile_source_load_remote (FCmdrProfileSource *source,
                                       GCancellable *cancellable,
                                       GAsyncReadyCallback callback,
                                       gpointer user_data)
{
	GTask *task;
	GHashTable *profiles;
	AsyncContext *async_context;

	/* This is used as a set. */
	profiles = g_hash_table_new_full (
		(GHashFunc) fcmdr_profile_hash,
		(GEqualFunc) fcmdr_profile_equal,
		(GDestroyNotify) g_object_unref,
		(GDestroyNotify) NULL);

	async_context = g_slice_new0 (AsyncContext);
	async_context->uri = fcmdr_profile_source_dup_uri (source);
	async_context->profiles = profiles;  /* takes ownership */

	task = g_task_new (source, cancellable, callback, user_data);
	g_task_set_source_tag (task, fcmdr_http_profile_source_load_remote);

	g_task_set_task_data (
		task, async_context, (GDestroyNotify) async_context_free);

	g_task_run_in_thread (
		task, fcmdr_http_profile_source_load_remote_thread);

	g_object_unref (task);
}

static gboolean
fcmdr_http_profile_source_load_remote_finish (FCmdrProfileSource *source,
                                              GQueue *out_profiles,
                                              GAsyncResult *result,
                                              GError **error)
{
	g_return_val_if_fail (g_task_is_valid (result, source), FALSE);
	g_return_val_if_fail (
		g_async_result_is_tagged (
		result, fcmdr_http_profile_source_load_remote), FALSE);

	if (!g_task_had_error (G_TASK (result))) {
		AsyncContext *async_context;
		GHashTableIter iter;
		gpointer profile;

		async_context = g_task_get_task_data (G_TASK (result));

		g_hash_table_iter_init (&iter, async_context->profiles);

		while (g_hash_table_iter_next (&iter, &profile, NULL)) {
			g_object_ref (profile);
			g_queue_push_tail (out_profiles, profile);
		}
	}

	return g_task_propagate_boolean (G_TASK (result), error);
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
	source_class->load_remote = fcmdr_http_profile_source_load_remote;
	source_class->load_remote_finish = fcmdr_http_profile_source_load_remote_finish;
}

static void
fcmdr_http_profile_source_init (FCmdrHttpProfileSource *source)
{
	source->priv = FCMDR_HTTP_PROFILE_SOURCE_GET_PRIVATE (source);

	source->priv->session = soup_session_new ();
}

