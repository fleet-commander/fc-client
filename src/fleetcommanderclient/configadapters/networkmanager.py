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
import time
import shutil
import logging
import pwd

import dbus

import gi
from gi.repository import Gio
from gi.repository import GLib


from fleetcommanderclient.configadapters.base import BaseConfigAdapter


class NetworkManagerDbusHelper(object):
    """
    Network manager dbus helper
    """

    BUS_NAME = 'org.freedesktop.NetworkManager'
    DBUS_OBJECT_PATH = '/org/freedesktop/NetworkManager/Settings'
    DBUS_INTERFACE_NAME = 'org.freedesktop.NetworkManager.Settings'

    def __init__(self):
        self.bus = Gio.bus_get_sync(Gio.BusType.SYSTEM, None)

    def variant_parse(self, serialized_data):
        return GLib.Variant.parse(None, serialized_data, None, None)

    def get_connection_path_by_uuid(self, uuid):
        """
        Returns connection path as an string
        """
        return self.bus.call_sync(
            self.BUS_NAME,
            self.DBUS_OBJECT_PATH,
            self.DBUS_INTERFACE_NAME,
            "GetConnectionByUuid",
            GLib.Variant.new_tuple(
                GLib.Variant.new_string(uuid)),
            GLib.VariantType("(o)"),
            Gio.DBusCallFlags.NONE, -1, None)[0]

    def add_connection(self, connection_data):
        conn_variant = self.variant_parse(connection_data)
        return self.bus.call_sync(
            self.BUS_NAME,
            self.DBUS_OBJECT_PATH,
            self.DBUS_INTERFACE_NAME,
            "AddConnection",
            GLib.Variant.new_tuple(conn_variant),
            GLib.VariantType("(o)"),
            Gio.DBusCallFlags.NONE, -1, None)

    def update_connection(self, connection_path, connection_data):
        conn_variant = self.variant_parse(connection_data)
        return self.bus.call_sync(
            self.BUS_NAME,
            connection_path,
            self.DBUS_INTERFACE_NAME + '.Connection',
            "Update",
            GLib.Variant.new_tuple(conn_variant),
            GLib.VariantType("()"),
            Gio.DBusCallFlags.NONE, -1, None)


class NetworkManagerConfigAdapter(BaseConfigAdapter):
    """
    Configuration adapter for Network Manager
    """

    NAMESPACE = 'org.freedesktop.NetworkManager'

    def bootstrap(self, uid):
        self.nmhelper = NetworkManagerDbusHelper()

    def update(self, uid, data):
        # Get username
        # username = pwd.getpwuid(uid)
        for connection in data:
            uuid = connection['uuid']
            connection_data = self.nmhelper.variant_parse(connection['data'])
            # Check if connection already exist
            try:
                path = self.nmhelper.get_connection_path_by_uuid(uuid)
            except Exception, e:
                path = None

            if path is not None:
                try:
                    self.nmhelper.update_connection(path, connection_data)
                except Exception, e:
                    logging.error('Error updating connection %s: %s' % (
                        uuid, e))
            else:
                # Connection does not exist. Add it
                try:
                    self.nmhelper.add_connection(connection_data)
                except Exception, e:
                    # Error adding connection
                    logging.error('Error adding connection %s: %s' % (
                        uuid, e))
