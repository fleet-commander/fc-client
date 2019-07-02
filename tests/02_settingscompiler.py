#!/usr/bin/env python-wrapper.sh
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
import json

PYTHONPATH = os.path.join(os.environ['TOPSRCDIR'], 'src')
sys.path.append(PYTHONPATH)

# Fleet commander imports
from fleetcommanderclient.settingscompiler import SettingsCompiler


class TestSettingsCompiler(unittest.TestCase):

    maxDiff = None

    ordered_filenames = [
        '0050-0050-0000-0000-0000-Test1.profile',
        '0060-0060-0000-0000-0000-Test2.profile',
        '0070-0070-0000-0000-0000-Invalid.profile',
        '0090-0090-0000-0000-0000-Test3.profile',
    ]

    invalid_profile_filename = ordered_filenames[2]

    PROFILE_1_SETTINGS = {
        "org.freedesktop.NetworkManager": [
            {
                "data": "{'connection': {'id': <'Company VPN'>, 'uuid': <'601d3b48-a44f-40f3-aa7a-35da4a10a099'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'vpnusername', 'IPSec gateway': 'vpn.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
                "type": "vpn",
                "uuid": "601d3b48-a44f-40f3-aa7a-35da4a10a099",
                "id": "Company VPN"
            }
        ],
        "org.gnome.gsettings": [
            {
                "signature": "s",
                "value": "'#FFFFFF'",
                "key": "/org/yorba/shotwell/preferences/ui/background-color",
                "schema": "org.yorba.shotwell.preferences.ui"
            },
        ],
        "org.libreoffice.registry": [
            {
                "value": "'Company'",
                "key": "/org/libreoffice/registry/org.openoffice.UserProfile/Data/o",
                "signature": "s"
            }
        ]
    }

    PROFILE_2_SETTINGS = {
        "org.gnome.gsettings": [
            {
                "signature": "s",
                "value": "'#CCCCCC'",
                "key": "/org/yorba/shotwell/preferences/ui/background-color",
                "schema": "org.yorba.shotwell.preferences.ui"
            },
            {
                "key": "/org/gnome/software/popular-overrides",
                "value": "['firefox.desktop','builder.desktop']",
                "signature": "as"
            }
        ],
        "org.libreoffice.registry": [
            {
                "value": "true",
                "key": "/org/libreoffice/registry/org.openoffice.Office.Writer/Layout/Window/HorizontalRuler",
                "signature": "b"
            },
            {
                "value": "'Our Company'",
                "key": "/org/libreoffice/registry/org.openoffice.UserProfile/Data/o",
                "signature": "s"
            }
        ],
        "org.gnome.online-accounts": {
            "Template account_fc_1490729747_0": {
                "FilesEnabled": False,
                "PhotosEnabled": False,
                "ContactsEnabled": False,
                "CalendarEnabled": True,
                "Provider": "google",
                "DocumentsEnabled": False,
                "PrintersEnabled": False,
                "MailEnabled": True
            }
        }
    }

    PROFILE_SETTINGS_MERGED = {
        "org.freedesktop.NetworkManager": [
            {
                "data": "{'connection': {'id': <'Company VPN'>, 'uuid': <'601d3b48-a44f-40f3-aa7a-35da4a10a099'>, 'type': <'vpn'>, 'autoconnect': <false>, 'secondaries': <@as []>}, 'ipv6': {'method': <'auto'>, 'dns': <@aay []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'ipv4': {'method': <'auto'>, 'dns': <@au []>, 'dns-search': <@as []>, 'address-data': <@aa{sv} []>, 'route-data': <@aa{sv} []>}, 'vpn': {'service-type': <'org.freedesktop.NetworkManager.vpnc'>, 'data': <{'NAT Traversal Mode': 'natt', 'ipsec-secret-type': 'ask', 'IPSec secret-flags': '2', 'xauth-password-type': 'ask', 'Vendor': 'cisco', 'Xauth username': 'vpnusername', 'IPSec gateway': 'vpn.mycompany.com', 'Xauth password-flags': '2', 'IPSec ID': 'vpngroupname', 'Perfect Forward Secrecy': 'server', 'IKE DH Group': 'dh2', 'Local Port': '0'}>, 'secrets': <@a{ss} {}>}}",
                "type": "vpn",
                "uuid": "601d3b48-a44f-40f3-aa7a-35da4a10a099",
                "id": "Company VPN"
            }
        ],
        "org.gnome.gsettings": [
            {
                "signature": "s",
                "value": "'#CCCCCC'",
                "key": "/org/yorba/shotwell/preferences/ui/background-color",
                "schema": "org.yorba.shotwell.preferences.ui"
            },
            {
                "key": "/org/gnome/software/popular-overrides",
                "value": "['firefox.desktop','builder.desktop']",
                "signature": "as"
            }
        ],
        "org.libreoffice.registry": [
            {
                "value": "true",
                "key": "/org/libreoffice/registry/org.openoffice.Office.Writer/Layout/Window/HorizontalRuler",
                "signature": "b"
            },
            {
                "value": "'Our Company'",
                "key": "/org/libreoffice/registry/org.openoffice.UserProfile/Data/o",
                "signature": "s"
            }
        ],
        "org.gnome.online-accounts": {
            "Template account_fc_1490729747_0": {
                "FilesEnabled": False,
                "PhotosEnabled": False,
                "ContactsEnabled": False,
                "CalendarEnabled": True,
                "Provider": "google",
                "DocumentsEnabled": False,
                "PrintersEnabled": False,
                "MailEnabled": True
            }
        }
    }

    COMPILED_SETTINGS = {
        "org.freedesktop.NetworkManager": [
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
        ],
        "org.gnome.gsettings": [
            {
                "signature": "s",
                "value": "'#CCCCCC'",
                "key": "/org/yorba/shotwell/preferences/ui/background-color",
                "schema": "org.yorba.shotwell.preferences.ui"
            },
            {
                "key": "/org/gnome/software/popular-overrides",
                "value": "['riot.desktop','matrix.desktop']",
                "signature": "as"
            },
            # FIXME: This data is added to merge libreoffice settings with
            #        gsettings to deploy all together with dconf config adapter
            {
                "value": "'The Company'",
                "key": "/org/libreoffice/registry/org.openoffice.UserProfile/Data/o",
                "signature": "s"
            },
            {
                "value": "true",
                "key": "/org/libreoffice/registry/org.openoffice.Office.Writer/Layout/Window/HorizontalRuler",
                "signature": "b"
            },
        ],
        "org.libreoffice.registry": [
            {
                "value": "true",
                "key": "/org/libreoffice/registry/org.openoffice.Office.Writer/Layout/Window/HorizontalRuler",
                "signature": "b"
            },
            {
                "value": "'The Company'",
                "key": "/org/libreoffice/registry/org.openoffice.UserProfile/Data/o",
                "signature": "s"
            }
        ],
        "org.gnome.online-accounts": {
            "Template account_fc_1490729747_0": {
                "FilesEnabled": True,
                "PhotosEnabled": False,
                "ContactsEnabled": False,
                "CalendarEnabled": True,
                "Provider": "google",
                "DocumentsEnabled": False,
                "PrintersEnabled": True,
                "MailEnabled": True
            },
            "Template account_fc_1490729585_0": {
                "PhotosEnabled": False,
                "Provider": "facebook",
                "MapsEnabled": False
            }
        }
    }

    def setUp(self):
        self.sc = SettingsCompiler(
            os.path.join(
                os.environ['TOPSRCDIR'], 'tests/data/sampleprofiledata/'))

    def test_00_get_ordered_file_names(self):
        # Read from invalid filename
        result = self.sc.get_ordered_file_names()
        self.assertEqual(result, self.ordered_filenames)

    def test_01_read_profile_settings(self):
        # Read from invalid filename
        result = self.sc.read_profile_settings('nonexistent')
        self.assertEqual(result, {})
        # Read invalid data
        result = self.sc.read_profile_settings(self.invalid_profile_filename)
        self.assertEqual(result, {})
        # Read valid data
        result = self.sc.read_profile_settings(self.ordered_filenames[0])
        self.assertEqual(result, self.PROFILE_1_SETTINGS)

    def test_02_merge_profile_settings(self):
        # Read from invalid filename
        result = self.sc.merge_profile_settings(
            self.PROFILE_1_SETTINGS, self.PROFILE_2_SETTINGS)
        result = sorted(result)
        expected = sorted(
            self.PROFILE_SETTINGS_MERGED)
        self.assertEqual(result, expected)

    def test_03_compile_settings(self):
        # Read from invalid filename
        result = self.sc.compile_settings()
        self.assertEqual(
            json.dumps(sorted(result), sort_keys=True),
            json.dumps(sorted(self.COMPILED_SETTINGS), sort_keys=True))


if __name__ == '__main__':
    unittest.main()
