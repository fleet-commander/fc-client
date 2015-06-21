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

#include "fcmdr-profile-applies.h"

G_DEFINE_BOXED_TYPE (
	FCmdrProfileApplies,
	fcmdr_profile_applies,
	fcmdr_profile_applies_copy,
	fcmdr_profile_applies_free)

/**
 * fcmdr_profile_applies_new:
 *
 * Allocates and zero-fills a new #FCmdrProfileApplies structure.
 * Free the allocated structure with fcmdr_profile_applies_free().
 *
 * Returns: an allocated #FCmdrProfileApplies
 **/
FCmdrProfileApplies *
fcmdr_profile_applies_new (void)
{
	return g_slice_new0 (FCmdrProfileApplies);
}

/**
 * fcmdr_profile_applies_copy:
 * @applies: a #FCmdrProfileApplies
 *
 * Allocates a complete copy of @applies.  Free the allocated copy with
 * fcmdr_profile_applies_free().
 *
 * Returns: an allocated #FCmdrProfileApplies
 **/
FCmdrProfileApplies *
fcmdr_profile_applies_copy (FCmdrProfileApplies *applies)
{
	FCmdrProfileApplies *copy = NULL;

	g_return_val_if_fail (applies != NULL, NULL);

	copy = fcmdr_profile_applies_new ();
	copy->users = g_strdupv (applies->users);
	copy->groups = g_strdupv (applies->groups);
	copy->hosts = g_strdupv (applies->hosts);

	return copy;
}

/**
 * fcmdr_profile_applies_free:
 * @applies: a #FCmdrProfileApplies
 *
 * Frees @applies as well as any %NULL-terminated string arrays contained
 * within, or does nothing if @applies is %NULL.
 **/
void
fcmdr_profile_applies_free (FCmdrProfileApplies *applies)
{
	if (applies != NULL) {
		g_strfreev (applies->users);
		g_strfreev (applies->groups);
		g_strfreev (applies->hosts);
		g_slice_free (FCmdrProfileApplies, applies);
	}
}

