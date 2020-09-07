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

sys.path.append(os.path.join(os.environ["TOPSRCDIR"], "src"))

import fleetcommanderclient.configadapters.firefox
from fleetcommanderclient.configadapters.firefox import FirefoxConfigAdapter

# Set logging to debug
logging.basicConfig(level=logging.DEBUG)

PROFILE_FILE_CONTENTS = r"""{"org.mozilla.firefox": [{"value": 0, "key": "accessibility.typeaheadfind.flashBar"}, {"value": false, "key": "beacon.enabled"}, {"value": "{\"placements\":{\"widget-overflow-fixed-list\":[],\"PersonalToolbar\":[\"personal-bookmarks\"],\"nav-bar\":[\"back-button\",\"forward-button\",\"stop-reload-button\",\"home-button\",\"customizableui-special-spring1\",\"urlbar-container\",\"customizableui-special-spring2\",\"downloads-button\",\"library-button\",\"sidebar-button\"],\"TabsToolbar\":[\"tabbrowser-tabs\",\"new-tab-button\",\"alltabs-button\"],\"toolbar-menubar\":[\"menubar-items\"]},\"seen\":[\"developer-button\"],\"dirtyAreaCache\":[\"PersonalToolbar\",\"nav-bar\",\"TabsToolbar\",\"toolbar-menubar\"],\"currentVersion\":12,\"newElementCount\":2}", "key": "browser.uiCustomization.state"}], "com.google.chrome.Policies": [], "org.chromium.Policies": [], "org.gnome.gsettings": [], "org.libreoffice.registry": [], "org.freedesktop.NetworkManager": []}"""

PREFS_FILE_CONTENTS = r"""pref("accessibility.typeaheadfind.flashBar", 0);
pref("beacon.enabled", false);
pref("browser.uiCustomization.state", "{\"placements\":{\"widget-overflow-fixed-list\":[],\"PersonalToolbar\":[\"personal-bookmarks\"],\"nav-bar\":[\"back-button\",\"forward-button\",\"stop-reload-button\",\"home-button\",\"customizableui-special-spring1\",\"urlbar-container\",\"customizableui-special-spring2\",\"downloads-button\",\"library-button\",\"sidebar-button\"],\"TabsToolbar\":[\"tabbrowser-tabs\",\"new-tab-button\",\"alltabs-button\"],\"toolbar-menubar\":[\"menubar-items\"]},\"seen\":[\"developer-button\"],\"dirtyAreaCache\":[\"PersonalToolbar\",\"nav-bar\",\"TabsToolbar\",\"toolbar-menubar\"],\"currentVersion\":12,\"newElementCount\":2}");"""


def universal_function(*args, **kwargs):
    pass


# Monkey patch chown function in os module for firefox config adapter
fleetcommanderclient.configadapters.firefox.os.chown = universal_function


class TestFirefoxConfigAdapter(unittest.TestCase):

    TEST_UID = os.getuid()

    TEST_DATA = json.loads(PROFILE_FILE_CONTENTS)["org.mozilla.firefox"]

    def setUp(self):
        self.test_directory = tempfile.mkdtemp(prefix="fc-client-firefox-test")
        self.policies_path = os.path.join(self.test_directory, "managed")
        self.policies_file_path = os.path.join(
            self.policies_path, FirefoxConfigAdapter.PREFS_FILENAME % self.TEST_UID
        )
        self.ca = FirefoxConfigAdapter(self.policies_path)

    def tearDown(self):
        # Remove test directory
        shutil.rmtree(self.test_directory)

    def test_00_bootstrap(self):
        # Run bootstrap with no directory created should continue
        self.ca.bootstrap(self.TEST_UID)
        # Run bootstrap with existing directories
        os.makedirs(self.policies_path)
        with open(self.policies_file_path, "w") as fd:
            fd.write("PREFS")
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
        # Change file mod because test user haven't root privilege
        os.chmod(self.policies_file_path, stat.S_IRUSR)
        # Read file
        with open(self.policies_file_path, "r") as fd:
            data = fd.read()
            fd.close()
        # Check file contents are ok
        self.assertEqual(PREFS_FILE_CONTENTS, data)


if __name__ == "__main__":
    unittest.main()
