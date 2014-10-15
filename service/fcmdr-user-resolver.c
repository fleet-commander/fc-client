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
 * SECTION: fcmdr-user-resolver
 * @short_description: Resolve account user names
 *
 * An #FCmdrUserResolver maps a UNIX user ID to the user's login name and,
 * if possible, the user's real name.  This helps resolve "${username}" and
 * "${realname}" variables in settings profiles.
 *
 * Unless overridden in the fleet-commander.conf file, the default user
 * resolver simply consults the local UNIX password database.  But profile
 * user names may not always match 1:1 with local UNIX user names, and so
 * the lookup API is asynchronous in anticipation of custom user resolvers
 * needing to consult some remote service like an LDAP directory.
 **/

#include "config.h"

#include "fcmdr-user-resolver.h"

#include <pwd.h>
#include <unistd.h>

#include "fcmdr-extensions.h"
#include "fcmdr-service.h"

#define FCMDR_USER_RESOLVER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_USER_RESOLVER, FCmdrUserResolverPrivate))

struct _FCmdrUserResolverPrivate {
	GWeakRef service;
};

enum {
	PROP_0,
	PROP_SERVICE
};

G_DEFINE_ABSTRACT_TYPE (
	FCmdrUserResolver,
	fcmdr_user_resolver,
	G_TYPE_OBJECT)

static void
fcmdr_user_resolver_set_service (FCmdrUserResolver *resolver,
                                 FCmdrService *service)
{
	g_return_if_fail (FCMDR_IS_SERVICE (service));

	g_weak_ref_set (&resolver->priv->service, service);
}

static void
fcmdr_user_resolver_set_property (GObject *object,
                                  guint property_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_SERVICE:
			fcmdr_user_resolver_set_service (
				FCMDR_USER_RESOLVER (object),
				g_value_get_object (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_user_resolver_get_property (GObject *object,
                                  guint property_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_SERVICE:
			g_value_take_object (
				value,
				fcmdr_user_resolver_ref_service (
				FCMDR_USER_RESOLVER (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_user_resolver_dispose (GObject *object)
{
	FCmdrUserResolverPrivate *priv;

	priv = FCMDR_USER_RESOLVER_GET_PRIVATE (object);

	g_weak_ref_set (&priv->service, NULL);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (fcmdr_user_resolver_parent_class)->dispose (object);
}

static void
fcmdr_user_resolver_class_init (FCmdrUserResolverClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (FCmdrUserResolverPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = fcmdr_user_resolver_set_property;
	object_class->get_property = fcmdr_user_resolver_get_property;
	object_class->dispose = fcmdr_user_resolver_dispose;

	g_object_class_install_property (
		object_class,
		PROP_SERVICE,
		g_param_spec_object (
			"service",
			"Service",
			"The owner of this resolver instance",
			FCMDR_TYPE_SERVICE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));
}

static void
fcmdr_user_resolver_init (FCmdrUserResolver *resolver)
{
	resolver->priv = FCMDR_USER_RESOLVER_GET_PRIVATE (resolver);
}

/**
 * fcmdr_user_resolver_try_new:
 * @service: a #FCmdrService
 * @extension_name: a user resolver extension name
 *
 * Instantiates a subclass of #FCmdrUserResolver registered under
 * @extension_name.  If no subclass is registered under @extension_name,
 * the function returns %NULL.
 *
 * Returns: a new #FCmdrUserResolver, or %NULL
 **/
FCmdrUserResolver *
fcmdr_user_resolver_try_new (FCmdrService *service,
                             const gchar *extension_name)
{
	GIOExtensionPoint *extension_point;
	GIOExtension *extension;
	FCmdrUserResolver *resolver = NULL;

	g_return_val_if_fail (FCMDR_IS_SERVICE (service), NULL);
	g_return_val_if_fail (extension_name != NULL, NULL);

	fcmdr_ensure_extensions_registered ();

	extension_point = g_io_extension_point_lookup (
		FCMDR_USER_RESOLVER_EXTENSION_POINT_NAME);

	extension = g_io_extension_point_get_extension_by_name (
		extension_point, extension_name);

	if (extension != NULL) {
		resolver = g_object_new (
			g_io_extension_get_type (extension),
			"service", service, NULL);
	}

	return resolver;
}

/**
 * fcmdr_user_resolver_ref_service:
 * @resolver: a #FCmdrUserResolver
 *
 * Returns the #FCmdrService that instantiated the @resolver.
 *
 * The returned #FCmdrService is referenced for thread-safety and must be
 * unreferenced with g_object_unref() when finished with it.
 *
 * Returns: a referenced #FCmdrService
 **/
FCmdrService *
fcmdr_user_resolver_ref_service (FCmdrUserResolver *resolver)
{
	g_return_val_if_fail (FCMDR_IS_USER_RESOLVER (resolver), NULL);

	return g_weak_ref_get (&resolver->priv->service);
}

/**
 * fcmdr_user_resolver_lookup:
 * @resolver: a #FCmdrUserResolver
 * @uid: a UNIX user ID
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: data to pass to the callback function
 *
 * Asychronously tries to obtain a user's login name and, if possible, real
 * name from the given numeric @uid.  This may be as simple as consulting
 * the local UNIX password database or it may involve a network request.
 *
 * When the operation is finished, @callback will be called.  You can then
 * call fcmdr_user_resolver_lookup_finish() to get the result of the operation.
 **/
void
fcmdr_user_resolver_lookup (FCmdrUserResolver *resolver,
                            uid_t uid,
                            GCancellable *cancellable,
                            GAsyncReadyCallback callback,
                            gpointer user_data)
{
	FCmdrUserResolverClass *class;

	g_return_if_fail (FCMDR_IS_USER_RESOLVER (resolver));

	class = FCMDR_USER_RESOLVER_GET_CLASS (resolver);
	g_return_if_fail (class->lookup != NULL);

	class->lookup (resolver, uid, cancellable, callback, user_data);
}

/**
 * fcmdr_user_resolver_lookup_finish:
 * @resolver: a #FCmdrUserResolver
 * @result: a #GAsyncResult
 * @out_user_name: return location for the user's login name, or %NULL
 * @out_real_name: return location for the user's real name, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Finishes the operation started with fcmdr_user_resolver_lookup().
 *
 * On a successful lookup, the function sets @out_user_name to the user's
 * login name, @out_real_name to either the user's real name or else %NULL
 * if that information was not available, and returns %TRUE.  If the lookup
 * operation failed, the function sets @error and returns %FALSE.
 *
 * Returns: %TRUE on success, %FALSE on failure
 **/
gboolean
fcmdr_user_resolver_lookup_finish (FCmdrUserResolver *resolver,
                                   GAsyncResult *result,
                                   gchar **out_user_name,
                                   gchar **out_real_name,
                                   GError **error)
{
	FCmdrUserResolverClass *class;

	g_return_val_if_fail (FCMDR_IS_USER_RESOLVER (resolver), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

	class = FCMDR_USER_RESOLVER_GET_CLASS (resolver);
	g_return_val_if_fail (class->lookup_finish != NULL, FALSE);

	return class->lookup_finish (
		resolver, result, out_user_name, out_real_name, error);
}

