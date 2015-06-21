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

#ifndef __FCMDR_PROFILE_APPLIES_H__
#define __FCMDR_PROFILE_APPLIES_H__

#include <gio/gio.h>

#define FCMDR_TYPE_PROFILE_APPLIES \
	(fcmdr_profile_applies_get_type ())

G_BEGIN_DECLS

typedef struct _FCmdrProfileApplies FCmdrProfileApplies;

/**
 * FCmdrProfileApplies:
 * @users: a %NULL-terminated array of user names
 * @groups: a %NULL-terminated array of group names
 * @hosts: a %NULL-terminated array of host names
 *
 * A #FCmdrProfileApplies describes the applicability of a #FCmdrProfile
 * to a given user or host machine.  If a user's name is included in @users,
 * or the user is a member of any groups included in @groups, or if the local
 * machine's host name is included in @hosts, then the #FCmdrProfile applies.
 **/
struct _FCmdrProfileApplies {
	gchar **users;
	gchar **groups;
	gchar **hosts;
};

GType		fcmdr_profile_applies_get_type	(void) G_GNUC_CONST;
FCmdrProfileApplies *
		fcmdr_profile_applies_new	(void);
FCmdrProfileApplies *
		fcmdr_profile_applies_copy	(FCmdrProfileApplies *applies);
void		fcmdr_profile_applies_free	(FCmdrProfileApplies *applies);

G_END_DECLS

#endif /* __FCMDR_PROFILE_APPLIES_H__ */

