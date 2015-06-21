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

#ifndef FCMDR_EXTENSIONS_H
#define FCMDR_EXTENSIONS_H

#include <gio/gio.h>

G_BEGIN_DECLS

void	fcmdr_ensure_extension_points_registered	(void);
void	fcmdr_ensure_extensions_registered		(void);

G_END_DECLS

#endif /* FCMDR_EXTENSIONS_H */

