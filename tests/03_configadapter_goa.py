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


from fleetcommanderclient.configadapters.goa import GOAConfigAdapter


class TestGOAConfigAdapter(unittest.TestCase):

    TEST_UID = '1002'

    TEST_DATA = {
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

    def setUp(self):
        self.test_directory = tempfile.mkdtemp(prefix='fc-client-goa-test')
        print self.test_directory
        self.ca = GOAConfigAdapter(self.test_directory)

    def tearDown(self):
        # Remove test directory
        #shutil.rmtree(self.test_directory)
        pass

    def test_00_bootstrap(self):
        dirpath = os.path.join(self.test_directory, self.TEST_UID)
        # Run bootstrap with no directory created should continue and warn
        self.ca.bootstrap(self.TEST_UID)
        # Run bootstrap with a existing directory
        os.makedirs(dirpath)
        self.assertTrue(os.path.exists(dirpath))
        self.ca.bootstrap(self.TEST_UID)
        # Check directory has been removed
        self.assertFalse(os.path.exists(dirpath))

    def test_01_update(self):
        self.ca.bootstrap(self.TEST_UID)
        self.ca.update(self.TEST_UID, self.TEST_DATA)
        keyfile_path = os.path.join(
            self.test_directory, self.TEST_UID, self.ca.FC_ACCOUNTS_FILE)
        # Check keyfile has been written
        self.assertTrue(os.path.exists(keyfile_path))
        # Read keyfile
        keyfile = GLib.KeyFile.new()
        keyfile.load_from_file(keyfile_path, GLib.KeyFileFlags.NONE)

        # Check section list
        accounts = self.TEST_DATA.keys()
        accounts.sort()
        accounts_keyfile = keyfile.get_groups()[0]
        print accounts_keyfile
        accounts_keyfile.sort()
        self.assertEqual(accounts, accounts_keyfile)

        # Check all sections
        for account, accountdata in self.TEST_DATA.items():
            # Check all keys and values
            for key, value in accountdata.items():
                if type(value) == bool:
                    value_keyfile = keyfile.get_boolean(account, key)
                else:
                    value_keyfile = keyfile.get_string(account, key)
                self.assertEqual(value, value_keyfile)

if __name__ == '__main__':
    unittest.main()
