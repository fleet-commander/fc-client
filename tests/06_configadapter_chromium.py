#!/usr/bin/env python-wrapper.sh
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

import fleetcommanderclient.configadapters.chromium
from fleetcommanderclient.configadapters.chromium import ChromiumConfigAdapter


def universal_function(*args, **kwargs):
    pass


# Monkey patch chown function in os module for chromium config adapter
fleetcommanderclient.configadapters.chromium.os.chown = universal_function


class TestChromiumConfigAdapter(unittest.TestCase):

    TEST_UID = 1002

    TEST_DATA = [
        {"value": True, "key": "ShowHomeButton"},
        {"value": True, "key": "BookmarkBarEnabled"}
    ]

    def setUp(self):
        self.test_directory = tempfile.mkdtemp(
            prefix='fc-client-chromium-test')
        self.policies_path = os.path.join(self.test_directory, 'managed')
        self.policies_file_path = os.path.join(
            self.policies_path,
            ChromiumConfigAdapter.POLICIES_FILENAME % self.TEST_UID)
        self.ca = ChromiumConfigAdapter(self.policies_path)

    def tearDown(self):
        # Remove test directory
        shutil.rmtree(self.test_directory)

    def test_00_bootstrap(self):
        # Run bootstrap with no directory created should continue
        self.ca.bootstrap(self.TEST_UID)
        # Run bootstrap with existing directories
        os.makedirs(self.policies_path)
        with open(self.policies_file_path, 'w') as fd:
            fd.write('POLICIES')
            fd.close()
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
        with open(self.policies_file_path, 'r') as fd:
            data = json.loads(fd.read())
            fd.close()
        # Check file contents are ok
        for item in self.TEST_DATA:
            self.assertTrue(item['key'] in data)
            self.assertEqual(item['value'], data[item['key']])


if __name__ == '__main__':
    unittest.main()
