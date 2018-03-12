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
import json
import unittest

import gi
from gi.repository import GLib

sys.path.append(os.path.join(os.environ['TOPSRCDIR'], 'src'))

import fleetcommanderclient.configadapters.firefox
from fleetcommanderclient.configadapters.firefox import FirefoxConfigAdapter

PREFS_FILE_CONTENTS = """pref("accessibility.typeaheadfind.flashBar", 0);
pref("beacon.enabled", false);
pref("browser.bookmarks.restore_default_bookmarks", false);
pref("browser.newtabpage.enhanced", false);
pref("browser.newtabpage.introShown", true);
pref("browser.newtabpage.storageVersion", 1);"""

def universal_function(*args, **kwargs):
    pass

# Monkey patch chown function in os module for firefox config adapter
fleetcommanderclient.configadapters.firefox.os.chown = universal_function

class TestFirefoxConfigAdapter(unittest.TestCase):

    TEST_UID = 1002

    TEST_DATA = [
        {'key': 'accessibility.typeaheadfind.flashBar', 'value': '0'},
        {'key': 'beacon.enabled', 'value': 'false'},
        {'key': 'browser.bookmarks.restore_default_bookmarks', 'value': 'false'},
        {'key': 'browser.newtabpage.enhanced', 'value': 'false'},
        {'key': 'browser.newtabpage.introShown', 'value': 'true'},
        {'key': 'browser.newtabpage.storageVersion', 'value': '1'},
    ]

    def setUp(self):
        self.test_directory = tempfile.mkdtemp(
            prefix='fc-client-firefox-test')
        self.policies_path = os.path.join(self.test_directory, 'managed')
        self.policies_file_path = os.path.join(
            self.policies_path,
            FirefoxConfigAdapter.PREFS_FILENAME % self.TEST_UID)
        self.ca = FirefoxConfigAdapter(self.policies_path)

    def tearDown(self):
        # Remove test directory
        shutil.rmtree(self.test_directory)

    def test_00_bootstrap(self):
        # Run bootstrap with no directory created should continue
        self.ca.bootstrap(self.TEST_UID)
        # Run bootstrap with existing directories
        os.makedirs(self.policies_path)
        open(self.policies_file_path, 'wb').write('PREFS')
        self.assertTrue(os.path.isdir(self.policies_path))
        self.assertTrue(os.path.exists(self.policies_file_path))
        self.ca.bootstrap(self.TEST_UID)
        # Check file has been removed
        self.assertFalse(os.path.exists(self.policies_file_path))
        self.assertTrue(os.path.isdir(self.policies_path))

    def test_01_update(self):
        self.ca.bootstrap(self.TEST_UID)
        self.ca.update(self.TEST_UID, self.TEST_DATA)
        # Check file has been written
        self.assertTrue(os.path.exists(self.policies_file_path))
        # Read file
        with open(self.policies_file_path, 'rb') as fd:
            data = fd.read()
            fd.close()
        print data
        # Check file contents are ok
        self.assertEqual(PREFS_FILE_CONTENTS, data)

if __name__ == '__main__':
    unittest.main()
