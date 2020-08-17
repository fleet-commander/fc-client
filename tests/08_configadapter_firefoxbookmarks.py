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
import logging
import stat

import gi
from gi.repository import GLib

sys.path.append(os.path.join(os.environ['TOPSRCDIR'], 'src'))

import fleetcommanderclient.configadapters.firefoxbookmarks
from fleetcommanderclient.configadapters.firefoxbookmarks import FirefoxBookmarksConfigAdapter

# Set logging to debug
logging.basicConfig(level=logging.DEBUG)

PROFILE_FILE_CONTENTS = r"""{
    "org.mozilla.firefox.Bookmarks": [
        {
            "key": "blah",
            "value": {
                "Title": "Test bookmark",
                "URL": "https://example.com",
                "Favicon": "https://example.com/favicon.ico",
                "Placement": "toolbar",
                "Folder": "FolderName"
            }
        }
    ]
}"""

POLICIES_FILE_CONTENTS = {
    "policies": {
        "Bookmarks": [
            {
                "Title": "Test bookmark",
                "URL": "https://example.com",
                "Favicon": "https://example.com/favicon.ico",
                "Placement": "toolbar",
                "Folder": "FolderName"
            }
        ]
    }
}


def universal_function(*args, **kwargs):
    pass


# Monkey patch chown function in os module for firefox config adapter
fleetcommanderclient.configadapters.firefoxbookmarks.os.chown = universal_function


class TestFirefoxBookmarksConfigAdapter(unittest.TestCase):
    TEST_UID = os.getuid()

    TEST_DATA = json.loads(PROFILE_FILE_CONTENTS)['org.mozilla.firefox.Bookmarks']

    def setUp(self):
        self.test_directory = tempfile.mkdtemp(
            prefix='fc-client-firefoxbookmarks-test')
        policies_path_template = os.path.join(self.test_directory, '{}/firefox')
        self.policies_path = policies_path_template.format(self.TEST_UID)
        self.policies_file_path = os.path.join(
            self.policies_path,
            FirefoxBookmarksConfigAdapter.POLICIES_FILENAME)
        self.ca = FirefoxBookmarksConfigAdapter(policies_path_template)

    def tearDown(self):
        # Remove test directory
        shutil.rmtree(self.test_directory)

    def test_00_bootstrap(self):
        logging.debug('Paths do not exist yet')
        self.assertFalse(os.path.exists(self.policies_file_path))
        self.assertFalse(os.path.isdir(self.policies_path))

        logging.debug('Run bootstrap with no directory created should continue')
        self.ca.bootstrap(self.TEST_UID)

        logging.debug('Run bootstrap with existing directories')
        os.makedirs(self.policies_path)
        with open(self.policies_file_path, 'w') as fd:
            fd.write('POLICIES')
            fd.close()
        self.assertTrue(os.path.isdir(self.policies_path))
        self.assertTrue(os.path.exists(self.policies_file_path))
        self.ca.bootstrap(self.TEST_UID)
        
        logging.debug('Check file has been removed')
        self.assertFalse(os.path.exists(self.policies_file_path))
        self.assertTrue(os.path.isdir(self.policies_path))

    def test_01_update(self):
        logging.debug('Run bootstrap')
        self.ca.bootstrap(self.TEST_UID)

        logging.debug('Run update')
        self.ca.update(self.TEST_UID, self.TEST_DATA)
        
        logging.debug('Check file has been written')
        self.assertTrue(os.path.exists(self.policies_file_path))
        
        logging.debug('Check file contents')
        # Change file mod because test user haven't root privilege
        os.chmod(self.policies_file_path, stat.S_IRUSR)
        with open(self.policies_file_path, 'r') as fd:
            data = json.loads(fd.read())
            fd.close()
        self.assertEqual(
            json.dumps(POLICIES_FILE_CONTENTS, sort_keys=True),
            json.dumps(data, sort_keys=True))


if __name__ == '__main__':
    unittest.main()
