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
import logging
import tempfile
import json
import dbus
import dbusmock
import subprocess

# Samba imports
from samba.ndr import ndr_pack
from samba.dcerpc import security

# Mocking imports
import ldapmock
import smbmock

sys.path.append(os.path.join(os.environ['TOPSRCDIR'], 'src'))

# Fleet commander imports
from fleetcommanderclient import fcadretriever

# Set logging level to debug
log = logging.getLogger()
level = logging.getLevelName('DEBUG')
log.setLevel(level)

# Mocking assignments
fcadretriever.ldap = ldapmock
fcadretriever.ldap.sasl = ldapmock.sasl
fcadretriever.libsmb.Conn = smbmock.SMBMock


# DNS resolver mock
class DNSResolverMock(object):

    class DNSResolverResult(object):
        target = 'FC.AD/'

    def query(self, name, querytype):
        return (self.DNSResolverResult, )


fcadretriever.dns.resolver = DNSResolverMock()


class TestSettingsCompiler(dbusmock.DBusTestCase):

    maxDiff = None

    TEST_USERNAME = 'myuser'
    TEST_GROUPS = ['mygroup1', 'mygroup2']
    TEST_HOSTNAME = 'myhost'
    TEST_GLOBAL_POLICY = 1
    TEST_PROFILE = {
        'cn': '',
        'name': 'Test Profile',
        'description': 'My test profile',
        'priority': 100,
        'settings': {
            'org.freedesktop.NetworkManager': [],
            'org.gnome.gsettings': [
                {
                    'schema': 'org.gnome.desktop.notifications.application',
                    'key': '/org/gnome/desktop/notifications/application/abrt-applet/application-id',
                    'value': "'abrt-applet.desktop'",
                    'signature': 's',
                }
            ],
        },
        'users': sorted(['guest', 'admin', ]),
        'groups': sorted(['admins', 'editors', ]),
        'hosts': sorted(['client1', ]),
        'hostgroups': [],
    }

    @classmethod
    def setUpClass(klass):
        klass.start_system_bus()
        klass.dbus_con = klass.get_dbus(system_bus=True)

    def setUp(self):               
        self._quit = False
        self.fcad = fcadretriever.FleetCommanderADProfileRetriever()
        self.p_mock = None
        self.p_mock2 = None

    def tearDown(self):
        if self.p_mock:
            self.p_mock.terminate()
            self.p_mock.wait()
        if self.p_mock2:
            self.p_mock2.terminate()
            self.p_mock2.wait()

    def setupRealmdDbusMock(
            self, domain='fc.domain', server='active-directory'):

        self.p_mock = self.spawn_server('org.freedesktop.realmd',
                                        '/org/freedesktop/realmd/Sssd',
                                        'org.freedesktop.realmd.Provider',
                                        system_bus=True,
                                        stdout=subprocess.PIPE)

        self.dbus_realmd_provider_mock = self.dbus_con.get_object(
            'org.freedesktop.realmd',
            '/org/freedesktop/realmd/Sssd',
            dbusmock.MOCK_IFACE)

        self.dbus_realmd_provider_mock.AddProperty(
            'org.freedesktop.realmd.Provider',
            'Realms',
            ['/org/freedesktop/realmd/Sssd/fc_realm_X'])

        self.dbus_realmd_provider_mock.AddObject(
            '/org/freedesktop/realmd/Sssd/fc_realm_X',
            'org.freedesktop.realmd.Realm',
            {
                'Name': dbus.String(domain, variant_level=1),
                'Details': [
                    ('server-software', server),
                    ('client-software', 'sssd')
                ]
            },
            [
                ('EmptyMethod', 's', '', ''),
            ])

    def setupFCClientDbusMock(self):
        self.p_mock = self.spawn_server('org.freedesktop.FleetCommanderClient',
                                        '/org/freedesktop/FleetCommanderClient',
                                        'org.freedesktop.FleetCommanderClient',
                                        system_bus=True,
                                        stdout=subprocess.PIPE)
        self.dbus_fcclient_mock = dbus.Interface(
            self.dbus_con.get_object(
                'org.freedesktop.FleetCommanderClient', '/org/freedesktop/FleetCommanderClient'),
            dbusmock.MOCK_IFACE)

        self.dbus_fcclient_mock.AddMethod(
            'org.freedesktop.FleetCommanderClient', 'ProcessSSSDFiles', 'tsu', '', '')

        self.dbus_fcclient_mock.AddMethod(
            'org.freedesktop.FleetCommanderClient', 'ProcessFiles', '', '', '')


    def _save_test_cifs_data(self, userdir):
        dirpath = os.path.join(
            userdir,
            'fcrealm.ad/Policies/profile-cn')
        os.makedirs(dirpath)
        filepath = os.path.join(dirpath, 'fleet-commander.json')
        with open(filepath, 'w') as fd:
            data = json.dumps({
                'priority': self.TEST_PROFILE['priority'],
                'settings': self.TEST_PROFILE['settings']
            })
            fd.write(data)
            fd.close()

    def _quit_mock(self):
        self._quit = True

    def test_01_check_realm_not_ad(self):
        # Monkeypatch module
        self.fcad.quit = self._quit_mock
        # Spawn realmd dbusmock server template
        self.setupRealmdDbusMock(server='ipa')
        # Check realm with something not AD
        self.fcad.check_realm()
        self.assertTrue(self._quit)

    def test_02_check_realm_ad(self):
        # Monkeypatch module
        self.fcad.quit = self._quit_mock
        # Spawn realmd dbusmock server template
        self.setupRealmdDbusMock(server='active-directory')
        # Check realm with an AD
        self.fcad.check_realm()
        self.assertFalse(self._quit)

    def test_03_check_elements_in_list(self):
        # Single element present
        self.assertTrue(
            self.fcad.check_elements_in_list(
                [1], [2, 3, 1, 4, 5]
            ))
        # Multiple elements present
        self.assertTrue(
            self.fcad.check_elements_in_list(
                [5, 4], [2, 3, 1, 4, 5]
            ))
        # Single element not present
        self.assertFalse(
            self.fcad.check_elements_in_list(
                [0], [2, 3, 1, 4, 5]
            ))
        # Multiple elements not present
        self.assertFalse(
            self.fcad.check_elements_in_list(
                [6, 0], [2, 3, 1, 4, 5]
            ))

    def test_04_generate_priority_applies(self):

        priority = str(self.TEST_PROFILE['priority']).zfill(5)

        # Only from user
        profile = self.TEST_PROFILE.copy()
        profile.update({
            'users': ['guest', 'myuser', 'admin', ],
            'groups': ['admins', 'editors', ],
            'hosts': ['client1', ],
        })
        result = self.fcad.generate_priority_applies(
            self.TEST_USERNAME,
            self.TEST_GROUPS,
            self.TEST_HOSTNAME,
            priority,
            self.TEST_GLOBAL_POLICY,
            profile)
        self.assertEqual(
            result, '00100_00000_00000_00000')

        # Only from group
        profile = self.TEST_PROFILE.copy()
        profile.update({
            'users': ['guest', 'admin', ],
            'groups': ['admins', 'mygroup2', 'editors', ],
            'hosts': ['client1', ],
        })
        result = self.fcad.generate_priority_applies(
            self.TEST_USERNAME,
            self.TEST_GROUPS,
            self.TEST_HOSTNAME,
            priority,
            self.TEST_GLOBAL_POLICY,
            profile)
        self.assertEqual(
            result, '00000_00100_00000_00000')

        # Only from host
        profile = self.TEST_PROFILE.copy()
        profile.update({
            'users': ['guest', 'admin', ],
            'groups': ['admins', 'editors', ],
            'hosts': ['client1', 'myhost', ],
        })
        result = self.fcad.generate_priority_applies(
            self.TEST_USERNAME,
            self.TEST_GROUPS,
            self.TEST_HOSTNAME,
            priority,
            self.TEST_GLOBAL_POLICY,
            profile)
        self.assertEqual(
            result, '00000_00000_00100_00000')

        # From user and group
        profile = self.TEST_PROFILE.copy()
        profile.update({
            'users': ['myuser', 'guest', 'admin', ],
            'groups': ['admins', 'editors', 'mygroup1', ],
            'hosts': ['client1', ],
        })
        result = self.fcad.generate_priority_applies(
            self.TEST_USERNAME,
            self.TEST_GROUPS,
            self.TEST_HOSTNAME,
            priority,
            self.TEST_GLOBAL_POLICY,
            profile)
        self.assertEqual(
            result, '00100_00100_00000_00000')

        # From user and host
        profile = self.TEST_PROFILE.copy()
        profile.update({
            'users': ['guest', 'admin', 'myuser', ],
            'groups': ['admins', 'editors', ],
            'hosts': ['myhost', 'client1', ],
        })
        result = self.fcad.generate_priority_applies(
            self.TEST_USERNAME,
            self.TEST_GROUPS,
            self.TEST_HOSTNAME,
            priority,
            self.TEST_GLOBAL_POLICY,
            profile)
        self.assertEqual(
            result, '00100_00000_00100_00000')

        # From group and host
        profile = self.TEST_PROFILE.copy()
        profile.update({
            'users': ['guest', 'admin', ],
            'groups': ['admins', 'mygroup1', 'editors', ],
            'hosts': ['client1', 'myhost', 'client2', ],
        })
        result = self.fcad.generate_priority_applies(
            self.TEST_USERNAME,
            self.TEST_GROUPS,
            self.TEST_HOSTNAME,
            priority,
            self.TEST_GLOBAL_POLICY,
            profile)
        self.assertEqual(
            result, '00000_00100_00100_00000')

        # From all
        profile = self.TEST_PROFILE.copy()
        profile.update({
            'users': ['myuser', 'guest', 'admin', ],
            'groups': ['admins', 'mygroup2', 'editors', ],
            'hosts': ['client1', 'myhost', ],
        })
        result = self.fcad.generate_priority_applies(
            self.TEST_USERNAME,
            self.TEST_GROUPS,
            self.TEST_HOSTNAME,
            priority,
            self.TEST_GLOBAL_POLICY,
            profile)
        self.assertEqual(
            result, '00100_00100_00100_00000')

    def test_05_process_profile(self):
        # Set domain in fcad instance
        self.fcad.DOMAIN = 'fcrealm.ad'
        # Create temporary directory
        userdir = tempfile.mkdtemp()
        smbmock.TEMP_DIR = userdir
        # Save profile data
        self._save_test_cifs_data(userdir)
        # Process profile using that directory
        profile = self.TEST_PROFILE.copy()
        profile['cn'] = 'profile-cn'
        self.fcad.process_profile(
            profile,
            userdir,
            self.TEST_USERNAME,
            self.TEST_GROUPS,
            self.TEST_HOSTNAME,
            self.TEST_GLOBAL_POLICY)
        # Check file exists
        filename = os.path.join(
            userdir,
            'fcrealm.ad/Policies/profile-cn/fleet-commander.json')
        self.assertTrue(os.path.isfile(filename))
        # Check file contents
        with open(filename, 'rb') as fd:
            data = fd.read()
            fd.close()
        jsondata = json.loads(data)
        self.assertEqual(jsondata, {
            'priority': self.TEST_PROFILE['priority'],
            'settings': self.TEST_PROFILE['settings']
        })

    def test_06_call_fc_client(self):
        # Setup dbusmock
        self.setupFCClientDbusMock()
        # Call client method
        self.fcad.call_fc_client()

if __name__ == '__main__':
    unittest.main()
