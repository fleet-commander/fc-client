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

#include "fcmdr-profile-source.h"

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
			"The URI of this profile source",
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

FCmdrService *
fcmdr_profile_source_ref_service (FCmdrProfileSource *source)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE_SOURCE (source), NULL);

	return g_weak_ref_get (&source->priv->service);
}

SoupURI *
fcmdr_profile_source_dup_uri (FCmdrProfileSource *source)
{
	g_return_val_if_fail (FCMDR_IS_PROFILE_SOURCE (source), NULL);

	return soup_uri_copy (source->priv->uri);
}

void
fcmdr_profile_source_load_cached (FCmdrProfileSource *source,
                                  const gchar *path)
{
	FCmdrProfileSourceClass *class;

	g_return_if_fail (FCMDR_IS_PROFILE_SOURCE (source));
	g_return_if_fail (path != NULL);

	class = FCMDR_PROFILE_SOURCE_GET_CLASS (source);

	if (class->load_cached != NULL)
		class->load_cached (source, path);
}

void
fcmdr_profile_source_load_remote (FCmdrProfileSource *source)
{
	FCmdrProfileSourceClass *class;

	g_return_if_fail (FCMDR_IS_PROFILE_SOURCE (source));

	class = FCMDR_PROFILE_SOURCE_GET_CLASS (source);

	if (class->load_remote != NULL)
		class->load_remote (source);
}

