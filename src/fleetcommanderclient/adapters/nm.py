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
import logging
import uuid
import pwd
import json

import gi

gi.require_version("NM", "1.0")

from gi.repository import Gio
from gi.repository import GLib
from gi.repository import NM

from fleetcommanderclient.adapters import BaseAdapter


class NetworkManagerDbusHelper(object):
    """
    Network manager dbus helper
    """

    BUS_NAME = "org.freedesktop.NetworkManager"
    DBUS_OBJECT_PATH = "/org/freedesktop/NetworkManager/Settings"
    DBUS_INTERFACE_NAME = "org.freedesktop.NetworkManager.Settings"

    def __init__(self):
        self.bus = Gio.bus_get_sync(Gio.BusType.SYSTEM, None)
        self.client = NM.Client.new(None)

    def get_user_name(self, uid):
        return pwd.getpwuid(uid).pw_name

    def get_connection_path_by_uuid(self, conn_uuid):
        """
        Returns connection path as an string
        """
        conn = self.client.get_connection_by_uuid(conn_uuid)
        if conn:
            return conn.get_path()
        return None

    def add_connection(self, connection_data):
        return self.bus.call_sync(
            self.BUS_NAME,
            self.DBUS_OBJECT_PATH,
            self.DBUS_INTERFACE_NAME,
            "AddConnection",
            GLib.Variant.new_tuple(connection_data),
            GLib.VariantType("(o)"),
            Gio.DBusCallFlags.NONE,
            -1,
            None,
        )

    def update_connection(self, connection_path, connection_data):
        return self.bus.call_sync(
            self.BUS_NAME,
            connection_path,
            self.DBUS_INTERFACE_NAME + ".Connection",
            "Update",
            GLib.Variant.new_tuple(connection_data),
            GLib.VariantType("()"),
            Gio.DBusCallFlags.NONE,
            -1,
            None,
        )


class NetworkManagerAdapter(BaseAdapter):
    """
    Configuration adapter for Network Manager
    """

    NAMESPACE = "org.freedesktop.NetworkManager"

    def _add_connection_metadata(self, serialized_data, uname, conn_uuid):
        sc = NM.SimpleConnection.new_from_dbus(
            GLib.Variant.parse(None, serialized_data, None, None)
        )
        setu = sc.get_setting(NM.SettingUser)
        if not setu:
            sc.add_setting(NM.SettingUser())
            setu = sc.get_setting(NM.SettingUser)

        setc = sc.get_setting(NM.SettingConnection)

        hashed_uuid = str(uuid.uuid5(uuid.UUID(conn_uuid), uname))

        setu.set_data("org.fleet-commander.connection", "true")
        setu.set_data("org.fleet-commander.connection.uuid", conn_uuid)
        setc.set_property("uuid", hashed_uuid)
        setc.add_permission("user", uname, None)

        return (sc.to_dbus(NM.ConnectionSerializationFlags.NO_SECRETS), hashed_uuid)

    def process_config_data(self, config_data, cache_path):
        """
        Process configuration data and save cache files to be deployed
        """
        # Write data as JSON
        path = os.path.join(cache_path, "fleet-commander")
        logging.debug("Writing NM data to {}".format(path))
        with open(path, "w") as fd:
            fd.write(json.dumps(config_data))
            fd.close()

    def deploy_files(self, cache_path, uid):
        """
        Create connections using NM dbus service
        This method will be called by privileged process
        """
        path = os.path.join(cache_path, "fleet-commander")

        if os.path.isfile(path):
            logging.debug("Deploying connections from file {}".format(path))
            nmhelper = NetworkManagerDbusHelper()
            uname = nmhelper.get_user_name(uid)
            with open(path, "r") as fd:
                data = json.loads(fd.read())
                fd.close()

            for connection in data:
                conn_uuid = connection["uuid"]
                connection_data, hashed_uuid = self._add_connection_metadata(
                    connection["data"], uname, conn_uuid
                )

                logging.debug(
                    "Checking connection %s + %s -> %s"
                    % (conn_uuid, uname, hashed_uuid)
                )

                # Check if connection already exist
                path = nmhelper.get_connection_path_by_uuid(hashed_uuid)

                if path is not None:
                    try:
                        nmhelper.update_connection(path, connection_data)
                    except Exception as e:
                        logging.error(
                            "Error updating connection %s: %s" % (conn_uuid, e)
                        )
                else:
                    # Connection does not exist. Add it
                    try:
                        nmhelper.add_connection(connection_data)
                    except Exception as e:
                        # Error adding connection
                        logging.error("Error adding connection %s: %s" % (conn_uuid, e))
        else:
            logging.debug("Connections file {} is not present. Ignoring.".format(path))
