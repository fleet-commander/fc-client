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

/**
 * SECTION: fcmdr-profile-source
 * @short_description: A source for settings profiles
 *
 * A #FCmdrProfileSource describes a location from which to obtain profile
 * data that can be deserialized into #FCmdrProfile instances.
 *
 * The #FCmdrProfileSource class itself is abstract.  Each subclass handles
 * profile loading for a particular URI scheme, like "http".
 **/

#include "config.h"

#include "fcmdr-profile-source.h"

#include "fcmdr-extensions.h"
#include "fcmdr-service.h"

#define FCMDR_PROFILE_SOURCE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_PROFILE_SOURCE, FCmdrProfileSourcePrivate))

struct _FCmdrProfileSourcePrivate {
	GWeakRef service;
	SoupURI *uri;
};

enum {
	PROP_0,
	PROP_SERVICE,
	PROP_URI
};

G_DEFINE_ABSTRACT_TYPE (
	FCmdrProfileSource,
	fcmdr_profile_source,
	G_TYPE_OBJECT)

static void
fcmdr_profile_source_set_service (FCmdrProfileSource *source,
                                  FCmdrService *service)
{
	g_return_if_fail (FCMDR_IS_SERVICE (service));

	g_weak_ref_set (&source->priv->service, service);
}

static void
fcmdr_profile_source_set_uri (FCmdrProfileSource *source,
                              SoupURI *uri)
{
	g_return_if_fail (uri != NULL);
	g_return_if_fail (source->priv->uri == NULL);

	source->priv->uri = soup_uri_copy (uri);
}

static void
fcmdr_profile_source_set_property (GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_SERVICE:
			fcmdr_profile_source_set_service (
				FCMDR_PROFILE_SOURCE (object),
				g_value_get_object (value));
			return;

		case PROP_URI:
			fcmdr_profile_source_set_uri (
				FCMDR_PROFILE_SOURCE (object),
				g_value_get_boxed (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_profile_source_get_property (GObject *object,
                                   guint property_id,
                                   GValue *value,
                                   GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_SERVICE:
			g_value_take_object (
				value,
				fcmdr_profile_source_ref_service (
				FCMDR_PROFILE_SOURCE (object)));
			return;

		case PROP_URI:
			g_value_take_boxed (
				value,
				fcmdr_profile_source_dup_uri (
				FCMDR_PROFILE_SOURCE (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_profile_source_dispose (GObject *object)
{
	FCmdrProfileSourcePrivate *priv;

	priv = FCMDR_PROFILE_SOURCE_GET_PRIVATE (object);

	g_weak_ref_set (&priv->service, NULL);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (fcmdr_profile_source_parent_class)->dispose (object);
}

static void
fcmdr_profile_source_finalize (GObject *object)
{
	FCmdrProfileSourcePrivate *priv;

	priv = FCMDR_PROFILE_SOURCE_GET_PRIVATE (object);

	soup_uri_free (priv->uri);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (fcmdr_profile_source_parent_class)->finalize (object);
}

static void
fcmdr_profile_source_class_init (FCmdrProfileSourceClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (FCmdrProfileSourcePrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = fcmdr_profile_source_set_property;
	object_class->get_property = fcmdr_profile_source_get_property;
	object_class->dispose = fcmdr_profile_source_dispose;
	object_class->finalize = fcmdr_profile_source_finalize;

	g_object_class_install_property (
		object_class,
		PROP_SERVICE,
		g_param_spec_object (
			"service",
			"Service",
			"The owner of this profile source",
			FCMDR_TYPE_SERVICE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (
		object_class,
		PROP_URI,
		g_param_spec_boxed (
			"uri",
			"URI",
			"The URI for this profile source",
			SOUP_TYPE_URI,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));
}

static void
fcmdr_profile_source_init (FCmdrProfileSource *source)
{
	source->priv = FCMDR_PROFILE_SOURCE_GET_PRIVATE (source);
}

/**
 * fcmdr_profile_source_try_new:
 * @service: a #FCmdrService
 * @uri: a #SoupURI
 *
 * Instantiates a #FCmdrProfileSource subclass to handle the given @uri.
 * If no suitable subclass is available, the function returns %NULL.
 *
 * Returns: a new #FCmdrProfileSource, or %NULL
 **/
FCmdrProfileSource *
fcmdr_profile_source_try_new (FCmdrService *service,
                              SoupURI *uri)
{
	GIOExtensionPoint *extension_point;
	GIOExtension *extension;
	FCmdrProfileSource *source = NULL;

	g_return_val_if_fail (FCMDR_IS_SERVICE (service), NULL);
	g_return_val_if_fail (uri != NULL, NULL);

	fcmdr_ensure_extensions_registered ();

	extension_point = g_io_extension_point_lookup (
		FCMDR_PROFILE_SOURCE_EXTENSION_POINT_NAME);

	extension = g_io_extension_point_get_extension_by_name (
		extension_point, uri->scheme);

	if (extension != NULL) {
		source = g_object_new (
			g_io_extension_get_type (extension),
			"service", service, "uri", uri, NULL);
	}

	return source;
}

/**
 * fcmdr_profile_source_ref_service:
 * @source: a #FCmdrProfileSource
 *
 * Returns the #FCmdrService passed to fcmdr_profile_source_try_new().
 *
 * The returned #FCmdrService is referenced for thread-safety and must be
 * unreferenced with g_object_unref() when finished with it.
 *
 * Returns: a referenced #FCmdrService
 **/
FCmdrService *
fcmdr_profile_source_ref_service (FCmdrProfileSource *source)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE_SOURCE (source), NULL);

	return g_weak_ref_get (&source->priv->service);
}

/**
 * fcmdr_profile_source_dup_uri:
 * @source: a #FCmdrProfileSource
 *
 * Returns a copy of the #SoupURI passed to fcmdr_profile_source_try_new().
 * Free the returned #SoupURI with soup_uri_free().
 *
 * Returns: a duplicated #SoupURI
 **/
SoupURI *
fcmdr_profile_source_dup_uri (FCmdrProfileSource *source)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE_SOURCE (source), NULL);

	return soup_uri_copy (source->priv->uri);
}

/**
 * fcmdr_profile_source_load_remote:
 * @source: a #FCmdrProfileSource
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: data to pass to the callback function
 *
 * Asynchronously loads remote profile data from the @source's
 * #FCmdrProfileSource:uri.
 *
 * When the operation is finished, @callback will be called.  You can then
 * call fcmdr_profile_source_load_remote_finish() to get the result of the
 * operation.
 **/
void
fcmdr_profile_source_load_remote (FCmdrProfileSource *source,
                                  GCancellable *cancellable,
                                  GAsyncReadyCallback callback,
                                  gpointer user_data)
{
	FCmdrProfileSourceClass *class;

	g_return_if_fail (FCMDR_IS_PROFILE_SOURCE (source));

	class = FCMDR_PROFILE_SOURCE_GET_CLASS (source);
	g_return_if_fail (class->load_remote != NULL);

	class->load_remote (source, cancellable, callback, user_data);
}

/**
 * fcmdr_profile_source_load_remote_finish:
 * @source: a #FCmdrProfileSource
 * @out_profiles: a #GQueue in which to deposit profiles
 * @result: a #GAsyncResult
 * @error: return location for a #GError, or %NULL
 *
 * Finishes the operation started with fcmdr_profile_source_load_remote().
 *
 * The @out_profiles queue will be populated with freshly loaded #FCmdrProfile
 * instances.  If an error occurred, the @out_profiles queue will remain empty
 * and the function will set @error and return %FALSE.
 *
 * Returns: %TRUE on success, %FALSE on failure
 **/
gboolean
fcmdr_profile_source_load_remote_finish (FCmdrProfileSource *source,
                                         GQueue *out_profiles,
                                         GAsyncResult *result,
                                         GError **error)
{
	FCmdrProfileSourceClass *class;

	g_return_val_if_fail (FCMDR_IS_PROFILE_SOURCE (source), FALSE);
	g_return_val_if_fail (out_profiles != NULL, FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

	class = FCMDR_PROFILE_SOURCE_GET_CLASS (source);
	g_return_val_if_fail (class->load_remote_finish != NULL, FALSE);

	return class->load_remote_finish (source, out_profiles, result, error);
}

