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
import tempfile
import shutil
import unittest

import gi
from gi.repository import GLib

sys.path.append(os.path.join(os.environ['TOPSRCDIR'], 'src'))


from fleetcommanderclient.configadapters.gsettings import GSettingsConfigAdapter


class TestGSettingsConfigAdapter(unittest.TestCase):

    TEST_UID = 1002

    TEST_DATA = [
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
        }
    ]

    def setUp(self):
        self.test_directory = tempfile.mkdtemp(
            prefix='fc-client-gsettings-test')

        self.profiledir = os.path.join(self.test_directory, 'profile')
        self.dbdir = os.path.join(self.test_directory, 'db')
        self.kfdir = os.path.join(self.dbdir, '%s%s.d' % (
            GSettingsConfigAdapter.FC_DB_FILE, unicode(self.TEST_UID)))
        self.profilepath = os.path.join(
            self.profiledir, unicode(self.TEST_UID))
        self.kfpath = os.path.join(
            self.kfdir, GSettingsConfigAdapter.FC_PROFILE_FILE)
        self.dbpath = os.path.join(
            self.dbdir, '%s%s' % (
                GSettingsConfigAdapter.FC_DB_FILE, unicode(self.TEST_UID)))

        self.ca = GSettingsConfigAdapter(
            os.path.join(self.test_directory, 'profile'),
            os.path.join(self.test_directory, 'db'),
        )

    def tearDown(self):
        # Remove test directory
        shutil.rmtree(self.test_directory)

    def test_00_bootstrap(self):
        # Run bootstrap with no directory created should continue and warn
        self.ca.bootstrap(self.TEST_UID)
        # Run bootstrap with existing directories
        os.makedirs(self.profiledir)
        os.makedirs(self.dbdir)
        os.makedirs(self.kfdir)
        open(self.profilepath, 'wb').write('PROFILE_FILE')
        open(self.kfpath, 'wb').write('KEY_FILE')
        open(self.dbpath, 'wb').write('DB_FILE')
        self.assertTrue(os.path.isdir(self.profiledir))
        self.assertTrue(os.path.isdir(self.kfdir))
        self.assertTrue(os.path.isdir(self.dbdir))
        self.assertTrue(os.path.exists(self.profilepath))
        self.assertTrue(os.path.exists(self.kfpath))
        self.assertTrue(os.path.exists(self.dbpath))
        self.ca.bootstrap(self.TEST_UID)
        # Check files and directories had been removed
        self.assertFalse(os.path.exists(self.profilepath))
        self.assertFalse(os.path.exists(self.kfpath))
        self.assertFalse(os.path.exists(self.dbpath))
        self.assertTrue(os.path.isdir(self.profiledir))
        self.assertFalse(os.path.isdir(self.kfdir))
        self.assertTrue(os.path.isdir(self.dbdir))

    def test_01_update(self):
        self.ca.bootstrap(self.TEST_UID)
        self.ca.update(self.TEST_UID, self.TEST_DATA)
        # Check keyfile has been written
        self.assertTrue(os.path.exists(self.kfpath))
        # Read keyfile
        keyfile = GLib.KeyFile.new()
        keyfile.load_from_file(self.kfpath, GLib.KeyFileFlags.NONE)

        # Check all sections
        for item in self.TEST_DATA:
            # Check all keys and values
            keysplit = item['key'][1:].split('/')
            keypath = '/'.join(keysplit[:-1])
            keyname = keysplit[-1]
            value = item['value']
            value_keyfile = keyfile.get_string(keypath, keyname)
            self.assertEqual(value, value_keyfile)

        # Check db file has been compiled
        self.assertTrue(os.path.exists(self.dbpath))
        data = open(self.dbpath, 'rb').read()
        self.assertEqual(data, 'COMPILED\n')


if __name__ == '__main__':
    unittest.main()
