# -*- coding: utf-8 -*-
# vi:ts=4 sw=4 sts=4

# Copyright (C) 2017 Red Hat, Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the licence, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, see <http://www.gnu.org/licenses/>.
#
# Authors: Alberto Ruiz <aruiz@redhat.com>
#          Oliver Gutiérrez <ogutierrez@redhat.com>

import os


class BaseConfigAdapter(object):
    """
    Base configuration adapter class
    """

    # Namespace this config adapter handles
    NAMESPACE = None

    def bootstrap(self, uid):
        """
        Prepare environment for a clean configuration deploy
        """
        raise NotImplementedError("You must implement bootstrap method")

    def update(self, uid, data):
        """
        Update configuration for given user
        """
        raise NotImplementedError("You must implement update method")

    @staticmethod
    def _set_perms(fd, uid, gid, perms):
        """
        Set owner and file mode for given file descriptor
        """
        os.fchown(fd.fileno(), gid, uid)
        os.fchmod(fd.fileno(), perms)
