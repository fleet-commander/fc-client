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
 * SECTION: fcmdr-service-backend
 * @short_description: Backend to parse and apply settings
 *
 * A #FCmdrServiceBackend parses and applies a section of
 * #FCmdrProfile:settings data from a #FCmdrProfile for a specific
 * desktop configuration system or application.
 **/
#include "config.h"

#include "fcmdr-service-backend.h"

#include "fcmdr-service.h"

#define FCMDR_SERVICE_BACKEND_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_SERVICE_BACKEND, FCmdrServiceBackendPrivate))

struct _FCmdrServiceBackendPrivate {
	GWeakRef service;
};

enum {
	PROP_0,
	PROP_SERVICE
};

G_DEFINE_ABSTRACT_TYPE (
	FCmdrServiceBackend,
	fcmdr_service_backend,
	G_TYPE_OBJECT)

static GIOExtension *
fcmdr_service_backend_lookup_extension (FCmdrServiceBackend *backend)
{
	GIOExtensionPoint *extension_point;
	GList *list, *link;

	extension_point = g_io_extension_point_lookup (
		FCMDR_SERVICE_BACKEND_EXTENSION_POINT_NAME);

	list = g_io_extension_point_get_extensions (extension_point);

	for (link = list; link != NULL; link = g_list_next (link)) {
		GIOExtension *extension = link->data;
		GType type;

		type = g_io_extension_get_type (extension);
		if (type == G_OBJECT_TYPE (backend))
			return extension;
	}

	return NULL;
}

static void
fcmdr_service_backend_set_service (FCmdrServiceBackend *backend,
                                   FCmdrService *service)
{
	g_return_if_fail (FCMDR_IS_SERVICE (service));

	g_weak_ref_set (&backend->priv->service, service);
}

static void
fcmdr_service_backend_set_property (GObject *object,
                                    guint property_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_SERVICE:
			fcmdr_service_backend_set_service (
				FCMDR_SERVICE_BACKEND (object),
				g_value_get_object (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}


static void
fcmdr_service_backend_get_property (GObject *object,
                                    guint property_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_SERVICE:
			g_value_take_object (
				value,
				fcmdr_service_backend_ref_service (
				FCMDR_SERVICE_BACKEND (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_service_backend_dispose (GObject *object)
{
	FCmdrServiceBackendPrivate *priv;

	priv = FCMDR_SERVICE_BACKEND_GET_PRIVATE (object);

	g_weak_ref_set (&priv->service, NULL);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (fcmdr_service_backend_parent_class)->dispose (object);
}

static void
fcmdr_service_backend_class_init (FCmdrServiceBackendClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (FCmdrServiceBackendPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = fcmdr_service_backend_set_property;
	object_class->get_property = fcmdr_service_backend_get_property;
	object_class->dispose = fcmdr_service_backend_dispose;

	g_object_class_install_property (
		object_class,
		PROP_SERVICE,
		g_param_spec_object (
			"service",
			"Service",
			"The owner of this backend instance",
			FCMDR_TYPE_SERVICE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));
}

static void
fcmdr_service_backend_init (FCmdrServiceBackend *backend)
{
	backend->priv = FCMDR_SERVICE_BACKEND_GET_PRIVATE (backend);
}

/**
 * fcmdr_service_backend_ref_service:
 * @backend: a #FCmdrServiceBackend
 *
 * Returns the #FCmdrService that instantiated the @backend.
 *
 * The returned #FCmdrService is referenced for thread-safety and must be
 * unreferenced with g_object_unref() when finished with it.
 *
 * Returns: a referenced #FCmdrService
 **/
FCmdrService *
fcmdr_service_backend_ref_service (FCmdrServiceBackend *backend)
{
	g_return_val_if_fail (FCMDR_IS_SERVICE_BACKEND (backend), NULL);

	return g_weak_ref_get (&backend->priv->service);
}

/**
 * fcmdr_service_backend_dup_settings:
 * @backend: a #FCmdrServiceBackend
 * @profile: a #FCmdrProfile
 *
 * Convenience function extracts raw JSON data for @backend from @profile.
 * It does this by matching the @backend's extension name to a member name
 * in the @profile's #FCmdrProfile:settings data.  If no match is found, the
 * function returns %NULL.  Free the returned #JsonNode with json_node_free().
 *
 * Returns: a #JsonNode containing settings data, or %NULL
 **/
JsonNode *
fcmdr_service_backend_dup_settings (FCmdrServiceBackend *backend,
                                    FCmdrProfile *profile)
{
	GIOExtension *extension;
	JsonObject *all_settings;
	JsonNode *our_settings;
	const gchar *name;

	g_return_val_if_fail (FCMDR_IS_SERVICE_BACKEND (backend), NULL);
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), NULL);

	extension = fcmdr_service_backend_lookup_extension (backend);
	g_return_val_if_fail (extension != NULL, NULL);

	all_settings = fcmdr_profile_ref_settings (profile);

	name = g_io_extension_get_name (extension);
	our_settings = json_object_dup_member (all_settings, name);

	json_object_unref (all_settings);

	return our_settings;
}

/**
 * fcmdr_service_backend_has_settings:
 * @backend: a #FCmdrServiceBackend
 * @profile: a #FCmdrProfile
 *
 * Convenience function checks whether @profile has settings for @backend.
 * It does this by matching the @backend's extension name to a member name
 * in the @profile's #FCmdrProfile:settings data.  If a match is found the
 * function returns %TRUE.
 *
 * Returns: whether @profile has settings for @backend
 **/
gboolean
fcmdr_service_backend_has_settings (FCmdrServiceBackend *backend,
                                    FCmdrProfile *profile)
{
	GIOExtension *extension;
	JsonObject *all_settings;
	gboolean has_settings;
	const gchar *name;

	g_return_val_if_fail (FCMDR_IS_SERVICE_BACKEND (backend), FALSE);
	g_return_val_if_fail (FCMDR_IS_PROFILE (profile), FALSE);

	all_settings = fcmdr_profile_ref_settings (profile);

	name = g_io_extension_get_name (extension);
	has_settings = json_object_has_member (all_settings, name);

	json_object_unref (all_settings);

	return has_settings;
}

/**
 * fcmdr_service_backend_apply_profiles:
 * @backend: a #FCmdrServiceBackend
 * @profiles: a #GList of #FCmdrProfile instances
 *
 * Applies settings specific to @backend from the list of profiles so
 * they take effect.  This operation can be expensive, so it's typically
 * only called once after all profiles from all profile sources are loaded.
 **/
void
fcmdr_service_backend_apply_profiles (FCmdrServiceBackend *backend,
                                      GList *profiles)
{
	FCmdrServiceBackendClass *class;

	g_return_if_fail (FCMDR_IS_SERVICE_BACKEND (backend));

	class = FCMDR_SERVICE_BACKEND_GET_CLASS (backend);

	if (class->apply_profiles != NULL) {
		GList *filtered;

		/* Copy and filter the list so the backend
		 * only has to deal with relevant profiles. */

		filtered = g_list_copy_deep (
			profiles, (GCopyFunc) g_object_ref, NULL);
		filtered = fcmdr_service_backend_filter_profiles (
			backend, filtered);

		class->apply_profiles (backend, filtered);

		g_list_free_full (filtered, (GDestroyNotify) g_object_unref);
	}
}

/**
 * fcmdr_service_backend_filter_profiles:
 * @backend: a #FCmdrServiceBackend
 * @profiles: a #GList of #FCmdrProfile instances
 *
 * Filters the @profiles list, keeping only #FCmdrProfile instances that
 * have settings for @backend.  Discarded #FCmdrProfile instances in the
 * list are unreferenced with g_object_unref().
 *
 * Returns: the (possibly changed) start of the @profiles list
 **/
GList *
fcmdr_service_backend_filter_profiles (FCmdrServiceBackend *backend,
                                       GList *profiles)
{
	GQueue trash = G_QUEUE_INIT;
	GList *link;

	g_return_val_if_fail (FCMDR_IS_SERVICE_BACKEND (backend), NULL);

	for (link = profiles; link != NULL; link = g_list_next (link)) {
		FCmdrProfile *profile = FCMDR_PROFILE (link->data);

		if (!fcmdr_service_backend_has_settings (backend, profile)) {
			g_clear_object (&link->data);
			g_queue_push_tail (&trash, link);
		}
	}

	while (!g_queue_is_empty (&trash)) {
		link = g_queue_pop_head (&trash);
		profiles = g_list_delete_link (profiles, link);
	}

	return profiles;
}

