#!/usr/bin/python
# -*- coding: utf-8 -*-
# vi:ts=2 sw=2 sts=2

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
import sys
import ConfigParser
import tempfile
import shutil
import unittest

import gi
from gi.repository import GLib

sys.path.append(os.path.join(os.environ['TOPSRCDIR'], 'src'))

from fleetcommanderclient.configadapters import networkmanager as canm
from fleetcommanderclient.configadapters import NetworkManagerConfigAdapter


class NetworkManagerDbusHelperMock(object):
    """
    Network manager dbus helper
    """

    connections = {}

    def variant_parse(self, serialized_data):
        return GLib.Variant.parse(None, serialized_data, None, None)

    def get_connection_path_by_uuid(self, uuid):
        """
        Returns connection path as an string
        """
        if uuid == '0be7d422-1635-11e7-a83f-68f728db19d3':
            return '/org/freedesktop/NetworkManager/Settings/21'
        else:
            raise Exception('NOT FOUND')

    def add_connection(self, connection_data):
        variant = self.variant_parse(connection_data)
        pydata = variant.unpack()
        uuid = pydata['connection']['uuid']
        if uuid in self.connections:
            raise Exception('CONNECTION ALREADY EXIST')
        self.connections[uuid] = pydata

    def update_connection(self, connection_path, connection_data):
        variant = self.variant_parse(connection_data)
        pydata = variant.unpack()
        uuid = pydata['connection']['uuid']
        if uuid not in self.connections:
            raise Exception('CONNECTION DOES NOT EXIST')
        self.connections[uuid] = pydata


# Replace DBus helper with mock
canm.NetworkManagerDbusHelper = NetworkManagerDbusHelperMock

class TestNetworkManagerConfigAdapter(unittest.TestCase):

    TEST_UID = 1002

    TEST_DATA = [
        {
            "data": "{'connection': {'id': <'Company VPN'>, 'uuid': <'601d3b48-a44f-40f3-aa7a-35da4a10a099'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'vpnusername', 'IPSec gateway': 'vpn.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
            "type": "vpn",
            "uuid": "601d3b48-a44f-40f3-aa7a-35da4a10a099",
            "id": "The Company VPN"
        },
        {
            "data": "{'connection': {'id': <'Intranet VPN'>, 'uuid': <'0be7d422-1635-11e7-a83f-68f728db19d3'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'vpnusername', 'IPSec gateway': 'vpn.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
            "type": "vpn",
            "uuid": "0be7d422-1635-11e7-a83f-68f728db19d3",
            "id": "Intranet VPN"
        }
    ]

    def setUp(self):
        self.ca = NetworkManagerConfigAdapter()

    def tearDown(self):
        pass

    def test_00_bootstrap(self):
        self.ca.bootstrap(self.TEST_UID)

    def test_01_update(self):
        self.ca.bootstrap(self.TEST_UID)
        self.ca.update(self.TEST_UID, self.TEST_DATA)

if __name__ == '__main__':
    unittest.main()
