#!/usr/bin/python
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

# Python imports
import os
import sys
import unittest

sys.path.append(os.path.join(os.environ['TOPSRCDIR'], 'src'))

# Fleet commander imports
from fleetcommanderclient import mergers


class TestBaseMerger(unittest.TestCase):

    maxDiff = None

    merger_class = mergers.BaseMerger

    TEST_SETTINGS_A = [
        {
          "signature": "s",
          "value": "'#FFFFFF'",
          "key": "/org/yorba/shotwell/preferences/ui/background-color",
          "schema": "org.yorba.shotwell.preferences.ui"
        },
        {
          "schema": "org.gnome.nautilus.preferences",
          "key": "/org/gnome/nautilus/preferences/default-folder-viewer",
          "value": "'list-view'",
          "signature": "s"
        },
    ]

    TEST_SETTINGS_B = [
        {
          "signature": "s",
          "value": "'#000000'",
          "key": "/org/yorba/shotwell/preferences/ui/background-color",
          "schema": "org.yorba.shotwell.preferences.ui"
        },
        {
          "schema": "org.gnome.nautilus.list-view",
          "key": "/org/gnome/nautilus/list-view/default-zoom-level",
          "value": "'large'",
          "signature": "s"
        },
    ]

    TEST_SETTINGS_MERGED = [
        {
          "signature": "s",
          "value": "'#000000'",
          "key": "/org/yorba/shotwell/preferences/ui/background-color",
          "schema": "org.yorba.shotwell.preferences.ui"
        },
        {
          "schema": "org.gnome.nautilus.preferences",
          "key": "/org/gnome/nautilus/preferences/default-folder-viewer",
          "value": "'list-view'",
          "signature": "s"
        },
        {
          "schema": "org.gnome.nautilus.list-view",
          "key": "/org/gnome/nautilus/list-view/default-zoom-level",
          "value": "'large'",
          "signature": "s"
        },
    ]

    def setUp(self):
        self.merger = self.merger_class()
        self.TEST_SETTINGS_MERGED.sort()

    def test_00_get_key(self):
        result = self.merger.get_key(self.TEST_SETTINGS_A[0])
        self.assertEqual(result, self.TEST_SETTINGS_A[0][self.merger.KEY_NAME])

    def test_01_merge(self):
        result = self.merger.merge(self.TEST_SETTINGS_A, self.TEST_SETTINGS_B)
        result.sort()
        self.assertEqual(result, self.TEST_SETTINGS_MERGED)


class TestNetworkManagerMerger(TestBaseMerger):

    merger_class = mergers.NetworkManagerMerger

    TEST_SETTINGS_A = [
        {
          "data": "{'connection': {'id': <'Company VPN'>, 'uuid': <'601d3b48-a44f-40f3-aa7a-35da4a10a099'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'old_vpnusername', 'IPSec gateway': 'old_vpn.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
          "type": "vpn",
          "uuid": "601d3b48-a44f-40f3-aa7a-35da4a10a099",
          "id": "Company VPN"
        },
        {
          "data": "{'connection': {'id': <'Marketing VPN'>, 'uuid': <'c2e76cf0-14a0-11e7-8f7e-68f728db19d3'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'vpnusername', 'IPSec gateway': 'marketing.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
          "type": "vpn",
          "uuid": "c2e76cf0-14a0-11e7-8f7e-68f728db19d3",
          "id": "Marketing VPN"
        },
    ]

    TEST_SETTINGS_B = [
        {
          "data": "{'connection': {'id': <'Company VPN'>, 'uuid': <'601d3b48-a44f-40f3-aa7a-35da4a10a099'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'vpnusername', 'IPSec gateway': 'vpn.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
          "type": "vpn",
          "uuid": "601d3b48-a44f-40f3-aa7a-35da4a10a099",
          "id": "Company VPN"
        },
        {
          "data": "{'connection': {'id': <'IT VPN'>, 'uuid': <'cf1bf3b0-14a0-11e7-a133-68f728db19d3'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'vpnusername', 'IPSec gateway': 'it.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
          "type": "vpn",
          "uuid": "cf1bf3b0-14a0-11e7-a133-68f728db19d3",
          "id": "IT VPN"
        },
    ]

    TEST_SETTINGS_MERGED = [
        {
          "data": "{'connection': {'id': <'Company VPN'>, 'uuid': <'601d3b48-a44f-40f3-aa7a-35da4a10a099'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'vpnusername', 'IPSec gateway': 'vpn.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
          "type": "vpn",
          "uuid": "601d3b48-a44f-40f3-aa7a-35da4a10a099",
          "id": "Company VPN"
        },
        {
          "data": "{'connection': {'id': <'Marketing VPN'>, 'uuid': <'c2e76cf0-14a0-11e7-8f7e-68f728db19d3'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'vpnusername', 'IPSec gateway': 'marketing.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
          "type": "vpn",
          "uuid": "c2e76cf0-14a0-11e7-8f7e-68f728db19d3",
          "id": "Marketing VPN"
        },
        {
          "data": "{'connection': {'id': <'IT VPN'>, 'uuid': <'cf1bf3b0-14a0-11e7-a133-68f728db19d3'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'vpnusername', 'IPSec gateway': 'it.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
          "type": "vpn",
          "uuid": "cf1bf3b0-14a0-11e7-a133-68f728db19d3",
          "id": "IT VPN"
        },
    ]


class TestGOAMerger(unittest.TestCase):

    merger_class = mergers.GOAMerger

    TEST_SETTINGS_A = {
        "Template account_fc_1490729747_0": {
          "FilesEnabled": False,
          "PhotosEnabled": False,
          "ContactsEnabled": False,
          "CalendarEnabled": True,
          "Provider": "google",
          "DocumentsEnabled": False,
          "PrintersEnabled": False,
          "MailEnabled": True
        },
        "Template account_fc_1490729845_0": {
          "PhotosEnabled": False,
          "Provider": "facebook",
          "MapsEnabled": True
        },
    }

    TEST_SETTINGS_B = {
        "Template account_fc_1490729747_0": {
          "FilesEnabled": False,
          "PhotosEnabled": False,
          "ContactsEnabled": True,
          "CalendarEnabled": False,
          "Provider": "google",
          "DocumentsEnabled": False,
          "PrintersEnabled": True,
          "MailEnabled": True
        },
        "Template account_fc_1490729989_0": {
          "FilesEnabled": True,
          "ContactsEnabled": False,
          "CalendarEnabled": True,
          "Provider": "owncloud",
          "DocumentsEnabled": False,
        },
    }

    TEST_SETTINGS_MERGED = {
        "Template account_fc_1490729747_0": {
          "FilesEnabled": False,
          "PhotosEnabled": False,
          "ContactsEnabled": True,
          "CalendarEnabled": False,
          "Provider": "google",
          "DocumentsEnabled": False,
          "PrintersEnabled": True,
          "MailEnabled": True
        },
        "Template account_fc_1490729989_0": {
          "FilesEnabled": True,
          "ContactsEnabled": False,
          "CalendarEnabled": True,
          "Provider": "owncloud",
          "DocumentsEnabled": False,
        },
        "Template account_fc_1490729845_0": {
          "PhotosEnabled": False,
          "Provider": "facebook",
          "MapsEnabled": True
        },
    }

    def setUp(self):
        self.merger = mergers.GOAMerger()

    def test_00_get_key(self):
        result = self.merger.get_key(self.TEST_SETTINGS_A)
        self.assertEqual(result, None)

    def test_01_merge(self):
        result = self.merger.merge(self.TEST_SETTINGS_A, self.TEST_SETTINGS_B)
        self.assertEqual(result, self.TEST_SETTINGS_MERGED)


class TestChromiumMerger(unittest.TestCase):

    maxDiff = None

    merger_class = mergers.ChromiumMerger

    TEST_SETTINGS_A = [
        {
            'key': 'ShutUpAndTakeMyMoney',
            'value': 'FullMoney'
        },
        {
            'key': 'FooBarBaz',
            'value': 'BooFarFaz'
        },
        {
            'key': 'ManagedBookmarks',
            'value': [
                {'name': 'Fedora', 'children': [
                    {'name': 'Get Fedora', 'url': 'https://getfedora.org/'},
                    {'name': 'Fedora Project', 'url': 'https://start.fedoraproject.org/'}
                    ]
                },
                {'name':'FreeIPA','url':'http://freeipa.org'},
                {'name':'Fleet Commander Github','url':'https://github.com/fleet-commander/'}
            ]
        }
    ]

    TEST_SETTINGS_B = [
        {
            'key': 'ShutUpAndTakeMyMoney',
            'value': 'NoMoney'
        },
        {
            'key': 'AllWorkAndNoPlay',
            'value': 'MakesJackADullBoy'
        },
        {
            'key': 'ManagedBookmarks',
            'value': [
                {'name':'Fedora','children': [
                    {'name':'Get Fedora NOW!!!','url':'https://getfedora.org/'},
                    {'name':'Fedora Project','url':'https://start.fedoraproject.org/'},
                    {'name':'The Chromium Projects','url':'https://www.chromium.org/'},
                    {'name':'SSSD','url':'pagure.org/SSSD'}
                    ]
                },
                {'name':'FreeIPA','url':'http://freeipa.org'},
                {'name':'Fleet Commander Docs','url':'http://fleet-commander.org/documentation.html'}
            ]
        }
    ]

    TEST_SETTINGS_MERGED = [
        {
            'key': 'ShutUpAndTakeMyMoney',
            'value': 'NoMoney'
        },
        {
            'key': 'FooBarBaz',
            'value': 'BooFarFaz'
        },
        {
            'key': 'AllWorkAndNoPlay',
            'value': 'MakesJackADullBoy'
        },
        {
            'key': 'ManagedBookmarks',
            'value': [
                {'name':'Fedora','children': [
                    {'name': 'Get Fedora', 'url': 'https://getfedora.org/'},
                    {'name':'Fedora Project','url':'https://start.fedoraproject.org/'},
                    {'name':'Get Fedora NOW!!!','url':'https://getfedora.org/'},
                    {'name':'The Chromium Projects','url':'https://www.chromium.org/'},
                    {'name':'SSSD','url':'pagure.org/SSSD'}
                    ]
                },
                {'name':'FreeIPA','url':'http://freeipa.org'},
                {'name':'Fleet Commander Github','url':'https://github.com/fleet-commander/'},
                {'name':'Fleet Commander Docs','url':'http://fleet-commander.org/documentation.html'}
            ]
        }
    ]

    def setUp(self):
        self.merger = self.merger_class()
        self.TEST_SETTINGS_MERGED.sort()

    def test_00_get_key(self):
        result = self.merger.get_key(self.TEST_SETTINGS_A[0])
        self.assertEqual(result, self.TEST_SETTINGS_A[0][self.merger.KEY_NAME])

    def test_01_merge(self):
        result = self.merger.merge(self.TEST_SETTINGS_A, self.TEST_SETTINGS_B)
        result.sort()
        self.assertEqual(result, self.TEST_SETTINGS_MERGED)

if __name__ == '__main__':
    unittest.main()
