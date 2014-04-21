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

#include "fcmdr-settings-backend.h"

#define FCMDR_SETTINGS_BACKEND_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), FCMDR_TYPE_SETTINGS_BACKEND, FCmdrSettingsBackendPrivate))

struct _FCmdrSettingsBackendPrivate {
	GWeakRef profile;
	JsonNode *settings;
};

enum {
	PROP_0,
	PROP_PROFILE,
	PROP_SETTINGS
};

G_DEFINE_ABSTRACT_TYPE (
	FCmdrSettingsBackend,
	fcmdr_settings_backend,
	G_TYPE_OBJECT)

static void
fcmdr_settings_backend_set_profile (FCmdrSettingsBackend *backend,
                                    FCmdrProfile *profile)
{
	g_return_if_fail (FCMDR_IS_PROFILE (profile));

	g_weak_ref_set (&backend->priv->profile, profile);
}

static void
fcmdr_settings_backend_set_settings (FCmdrSettingsBackend *backend,
                                     JsonNode *settings)
{
	g_return_if_fail (settings != NULL);
	g_return_if_fail (backend->priv->settings == NULL);

	backend->priv->settings = json_node_copy (settings);
}

static void
fcmdr_settings_backend_set_property (GObject *object,
                                     guint property_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_PROFILE:
			fcmdr_settings_backend_set_profile (
				FCMDR_SETTINGS_BACKEND (object),
				g_value_get_object (value));
			return;

		case PROP_SETTINGS:
			fcmdr_settings_backend_set_settings (
				FCMDR_SETTINGS_BACKEND (object),
				g_value_get_boxed (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_settings_backend_get_property (GObject *object,
                                     guint property_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_PROFILE:
			g_value_take_object (
				value,
				fcmdr_settings_backend_ref_profile (
				FCMDR_SETTINGS_BACKEND (object)));
			return;

		case PROP_SETTINGS:
			g_value_set_boxed (
				value,
				fcmdr_settings_backend_get_settings (
				FCMDR_SETTINGS_BACKEND (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
fcmdr_settings_backend_dispose (GObject *object)
{
	FCmdrSettingsBackendPrivate *priv;

	priv = FCMDR_SETTINGS_BACKEND_GET_PRIVATE (object);

	g_weak_ref_set (&priv->profile, NULL);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (fcmdr_settings_backend_parent_class)->dispose (object);
}

static void
fcmdr_settings_backend_class_init (FCmdrSettingsBackendClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (FCmdrSettingsBackendPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = fcmdr_settings_backend_set_property;
	object_class->get_property = fcmdr_settings_backend_get_property;
	object_class->dispose = fcmdr_settings_backend_dispose;

	class->apply_settings = NULL;

	g_object_class_install_property (
		object_class,
		PROP_PROFILE,
		g_param_spec_object (
			"profile",
			"Profile",
			"The owner of this backend instance",
			FCMDR_TYPE_PROFILE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (
		object_class,
		PROP_SETTINGS,
		g_param_spec_boxed (
			"settings",
			"Settings",
			"Raw settings data in JSON format",
			JSON_TYPE_NODE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_STATIC_STRINGS));
}

static void
fcmdr_settings_backend_init (FCmdrSettingsBackend *backend)
{
	backend->priv = FCMDR_SETTINGS_BACKEND_GET_PRIVATE (backend);
}

FCmdrProfile *
fcmdr_settings_backend_ref_profile (FCmdrSettingsBackend *backend)
{
	g_return_val_if_fail (FCMDR_IS_SETTINGS_BACKEND (backend), NULL);

	return g_weak_ref_get (&backend->priv->profile);
}

JsonNode *
fcmdr_settings_backend_get_settings (FCmdrSettingsBackend *backend)
{
	g_return_val_if_fail (FCMDR_IS_SETTINGS_BACKEND (backend), NULL);

	return backend->priv->settings;
}

void
fcmdr_settings_backend_apply_settings (FCmdrSettingsBackend *backend)
{
	FCmdrSettingsBackendClass *class;

	g_return_if_fail (FCMDR_IS_SETTINGS_BACKEND (backend));

	class = FCMDR_SETTINGS_BACKEND_GET_CLASS (backend);

	if (class->apply_settings != NULL)
		class->apply_settings (backend);
}

