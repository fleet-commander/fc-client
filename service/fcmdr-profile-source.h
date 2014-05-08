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

#ifndef __FCMDR_PROFILE_SOURCE_H__
#define __FCMDR_PROFILE_SOURCE_H__

#include <gio/gio.h>
#include <libsoup/soup.h>

/* Standard GObject macros */
#define FCMDR_TYPE_PROFILE_SOURCE \
	(fcmdr_profile_source_get_type ())
#define FCMDR_PROFILE_SOURCE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), FCMDR_TYPE_PROFILE_SOURCE, FCmdrProfileSource))
#define FCMDR_PROFILE_SOURCE_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), FCMDR_TYPE_PROFILE_SOURCE, FCmdrProfileSourceClass))
#define FCMDR_IS_PROFILE_SOURCE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), FCMDR_TYPE_PROFILE_SOURCE))
#define FCMDR_IS_PROFILE_SOURCE_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), FCMDR_TYPE_PROFILE_SOURCE))
#define FCMDR_PROFILE_SOURCE_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), FCMDR_TYPE_PROFILE_SOURCE, FCmdrProfileSourceClass))

#define FCMDR_PROFILE_SOURCE_EXTENSION_POINT_NAME "fcmdr-profile-source"

G_BEGIN_DECLS

/* Avoids a circular dependency. */
struct _FCmdrService;

typedef struct _FCmdrProfileSource FCmdrProfileSource;
typedef struct _FCmdrProfileSourceClass FCmdrProfileSourceClass;
typedef struct _FCmdrProfileSourcePrivate FCmdrProfileSourcePrivate;

struct _FCmdrProfileSource {
	GObject parent;
	FCmdrProfileSourcePrivate *priv;
};

struct _FCmdrProfileSourceClass {
	GObjectClass parent_class;

	/* Methods */
	void		(*load_cached)		(FCmdrProfileSource *source,
						 const gchar *path);
	void		(*load_remote)		(FCmdrProfileSource *source);
};

GType		fcmdr_profile_source_get_type	(void) G_GNUC_CONST;
struct _FCmdrService *
		fcmdr_profile_source_ref_service
						(FCmdrProfileSource *source);
SoupURI *	fcmdr_profile_source_dup_uri	(FCmdrProfileSource *source);
void		fcmdr_profile_source_load_cached
						(FCmdrProfileSource *source,
						 const gchar *path);
void		fcmdr_profile_source_load_remote
						(FCmdrProfileSource *source);

G_END_DECLS

#endif /* __FCMDR_PROFILE_SOURCE_H__ */
