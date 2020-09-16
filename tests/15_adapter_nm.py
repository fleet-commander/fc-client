#!/usr/bin/env python-wrapper.sh
# -*- coding: utf-8 -*-
# vi:ts=2 sw=2 sts=2

# Copyright (C) 2019 Red Hat, Inc.
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
import sys
import logging
import tempfile
import shutil
import uuid
import json
import unittest

import dbus.service
import dbus.mainloop.glib

import dbusmock
from dbusmock.templates.networkmanager import (
    MANAGER_IFACE,
    SETTINGS_OBJ,
    SETTINGS_IFACE,
)

sys.path.append(os.path.join(os.environ["TOPSRCDIR"], "src"))

import fleetcommanderclient.adapters.nm
from fleetcommanderclient.adapters.nm import NetworkManagerAdapter


# Set log level to debug
logging.basicConfig(level=logging.DEBUG)


def mock_getpwuid(uid):
    class UserObject:
        pw_name = "mockeduser{}".format(uid)

    return UserObject()


fleetcommanderclient.adapters.nm.pwd.getpwuid = mock_getpwuid


class TestNetworkManagerAdapter(dbusmock.DBusTestCase):

    TEST_UID = 55555
    TEST_USER_NAME = "mockeduser{}".format(TEST_UID)

    TEST_DATA = [
        {
            "data": "{'connection': {'id': <'Company VPN'>, 'uuid': <'601d3b48-a44f-40f3-aa7a-35da4a10a099'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'vpnusername', 'IPSec gateway': 'vpn.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
            "type": "vpn",
            "uuid": "601d3b48-a44f-40f3-aa7a-35da4a10a099",
            "id": "The Company VPN",
        },
        {
            "data": "{'connection': {'id': <'Intranet VPN'>, 'uuid': <'0be7d422-1635-11e7-a83f-68f728db19d3'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'vpnusername', 'IPSec gateway': 'vpn.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
            "type": "vpn",
            "uuid": "0be7d422-1635-11e7-a83f-68f728db19d3",
            "id": "Intranet VPN",
        },
    ]

    @classmethod
    def setUpClass(cls):
        cls.start_system_bus()
        cls.dbus_con = cls.get_dbus(True)

    def setUp(self):
        self.test_directory = tempfile.mkdtemp(prefix="fc-client-nm-test")
        self.cache_path = os.path.join(self.test_directory, "cache")
        self.ca = NetworkManagerAdapter()
        self.ca._TEST_CACHE_PATH = self.cache_path

        # DBus mocking of NM
        self.p_mock, self.obj_nm = self.spawn_server_template(
            "networkmanager", {"NetworkingEnabled": True}
        )
        self.settings = dbus.Interface(
            self.dbus_con.get_object(MANAGER_IFACE, SETTINGS_OBJ), SETTINGS_IFACE
        )

    def tearDown(self):
        # Finish dbus mocking
        self.p_mock.terminate()
        self.p_mock.wait()
        # Remove test directory
        shutil.rmtree(self.test_directory)

    def test_00_generate_config(self):
        # Generate configuration
        self.ca.generate_config(self.TEST_DATA)
        # Check config file exists
        config_file = os.path.join(
            self.cache_path, self.ca.NAMESPACE, "fleet-commander"
        )
        self.assertTrue(os.path.isfile(config_file))

        # Check data file contents
        with open(config_file, "r") as fd:
            data = json.loads(fd.read())
            fd.close()
        self.assertEqual(data, self.TEST_DATA)

    def test_01_deploy(self):
        uuid1 = "601d3b48-a44f-40f3-aa7a-35da4a10a099"
        uuid2 = "0be7d422-1635-11e7-a83f-68f728db19d3"
        hashed_uuid1 = str(uuid.uuid5(uuid.UUID(uuid1), self.TEST_USER_NAME))
        hashed_uuid2 = str(uuid.uuid5(uuid.UUID(uuid2), self.TEST_USER_NAME))

        # We add an existing connection to trigger an Update method
        self.settings.AddConnection(
            dbus.Dictionary(
                {
                    "connection": dbus.Dictionary(
                        {
                            "id": "test connection",
                            "uuid": hashed_uuid1,
                            "type": "802-11-wireless",
                        },
                        signature="sv",
                    ),
                    "802-11-wireless": dbus.Dictionary(
                        {"ssid": dbus.ByteArray("The_SSID".encode("UTF-8"))},
                        signature="sv",
                    ),
                }
            )
        )

        # Generate config files in cache
        self.ca.generate_config(self.TEST_DATA)
        # Execute deployment
        self.ca.deploy(self.TEST_UID)

        path1 = self.settings.GetConnectionByUuid(hashed_uuid1)
        path2 = self.settings.GetConnectionByUuid(hashed_uuid2)

        conns = self.settings.ListConnections()
        self.assertEqual(len(conns), 2)

        self.assertIn(path1, conns)
        self.assertIn(path2, conns)

        conn1 = dbus.Interface(
            self.dbus_con.get_object(MANAGER_IFACE, path1),
            "org.freedesktop.NetworkManager.Settings.Connection",
        )
        conn2 = dbus.Interface(
            self.dbus_con.get_object(MANAGER_IFACE, path2),
            "org.freedesktop.NetworkManager.Settings.Connection",
        )

        conn1_sett = conn1.GetSettings()
        conn2_sett = conn2.GetSettings()

        self.assertEqual(conn1_sett["connection"]["uuid"], hashed_uuid1)
        self.assertEqual(conn2_sett["connection"]["uuid"], hashed_uuid2)

        self.assertEqual(
            conn1_sett["connection"]["permissions"],
            [
                "user:{}:".format(self.TEST_USER_NAME),
            ],
        )
        self.assertEqual(
            conn2_sett["connection"]["permissions"],
            [
                "user:{}:".format(self.TEST_USER_NAME),
            ],
        )

        self.assertEqual(
            conn1_sett["user"]["data"]["org.fleet-commander.connection"], "true"
        )
        self.assertEqual(
            conn1_sett["user"]["data"]["org.fleet-commander.connection.uuid"], uuid1
        )

        self.assertEqual(
            conn2_sett["user"]["data"]["org.fleet-commander.connection"], "true"
        )
        self.assertEqual(
            conn2_sett["user"]["data"]["org.fleet-commander.connection.uuid"], uuid2
        )


if __name__ == "__main__":
    unittest.main()
