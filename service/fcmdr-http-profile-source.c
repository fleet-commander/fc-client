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

