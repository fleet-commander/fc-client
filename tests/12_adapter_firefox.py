#!/usr/bin/env python-wrapper.sh
# -*- coding: utf-8 -*-
# vi:ts=2 sw=2 sts=2

# Copyright (C) 2019 Red Hat, Inc.
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
import logging
import tempfile
import shutil
import json
import unittest

sys.path.append(os.path.join(os.environ["TOPSRCDIR"], "src"))

import fleetcommanderclient.adapters.firefox
from fleetcommanderclient.adapters.firefox import FirefoxAdapter


def universal_function(*args, **kwargs):
    pass


# Monkey patch chown function in os module
fleetcommanderclient.adapters.firefox.os.chown = universal_function


# Set log level to debug
logging.basicConfig(level=logging.DEBUG)

PROFILE_FILE_CONTENTS = r"""{"org.mozilla.firefox": [{"value": 0, "key": "accessibility.typeaheadfind.flashBar"}, {"value": false, "key": "beacon.enabled"}, {"value": "{\"placements\":{\"widget-overflow-fixed-list\":[],\"PersonalToolbar\":[\"personal-bookmarks\"],\"nav-bar\":[\"back-button\",\"forward-button\",\"stop-reload-button\",\"home-button\",\"customizableui-special-spring1\",\"urlbar-container\",\"customizableui-special-spring2\",\"downloads-button\",\"library-button\",\"sidebar-button\"],\"TabsToolbar\":[\"tabbrowser-tabs\",\"new-tab-button\",\"alltabs-button\"],\"toolbar-menubar\":[\"menubar-items\"]},\"seen\":[\"developer-button\"],\"dirtyAreaCache\":[\"PersonalToolbar\",\"nav-bar\",\"TabsToolbar\",\"toolbar-menubar\"],\"currentVersion\":12,\"newElementCount\":2}", "key": "browser.uiCustomization.state"}], "com.google.chrome.Policies": [], "org.chromium.Policies": [], "org.gnome.gsettings": [], "org.libreoffice.registry": [], "org.freedesktop.NetworkManager": []}"""

PREFS_FILE_CONTENTS = r"""pref("accessibility.typeaheadfind.flashBar", 0);
pref("beacon.enabled", false);
pref("browser.uiCustomization.state", "{\"placements\":{\"widget-overflow-fixed-list\":[],\"PersonalToolbar\":[\"personal-bookmarks\"],\"nav-bar\":[\"back-button\",\"forward-button\",\"stop-reload-button\",\"home-button\",\"customizableui-special-spring1\",\"urlbar-container\",\"customizableui-special-spring2\",\"downloads-button\",\"library-button\",\"sidebar-button\"],\"TabsToolbar\":[\"tabbrowser-tabs\",\"new-tab-button\",\"alltabs-button\"],\"toolbar-menubar\":[\"menubar-items\"]},\"seen\":[\"developer-button\"],\"dirtyAreaCache\":[\"PersonalToolbar\",\"nav-bar\",\"TabsToolbar\",\"toolbar-menubar\"],\"currentVersion\":12,\"newElementCount\":2}");"""


class TestFirefoxAdapter(unittest.TestCase):

    TEST_UID = 55555

    TEST_DATA = json.loads(PROFILE_FILE_CONTENTS)["org.mozilla.firefox"]

    def setUp(self):
        self.test_directory = tempfile.mkdtemp(prefix="fc-client-firefox-test")
        self.prefs_path = self.test_directory
        self.cache_path = os.path.join(self.test_directory, "cache")
        self.prefs_file_path = os.path.join(
            self.prefs_path, FirefoxAdapter.PREFS_FILENAME.format(self.TEST_UID)
        )
        self.ca = FirefoxAdapter(self.prefs_path)
        self.ca._TEST_CACHE_PATH = self.cache_path

    def tearDown(self):
        # Remove test directory
        shutil.rmtree(self.test_directory)

    def test_00_generate_config(self):
        # Generate configuration
        self.ca.generate_config(self.TEST_DATA)
        # Check configuration file exists
        filepath = os.path.join(self.cache_path, self.ca.NAMESPACE, "fleet-commander")
        logging.debug("Checking %s exists", filepath)
        self.assertTrue(os.path.exists(filepath))
        # Check configuration file contents
        with open(filepath, "r") as fd:
            data = fd.read()
            fd.close()
        self.assertEqual(data, PREFS_FILE_CONTENTS)

    def test_01_deploy(self):
        # Generate config files in cache
        self.ca.generate_config(self.TEST_DATA)
        # Execute deployment
        self.ca.deploy(self.TEST_UID)
        # Check file has been copied to policies path
        deployed_file_path = os.path.join(
            self.prefs_path, FirefoxAdapter.PREFS_FILENAME.format(self.TEST_UID)
        )
        self.assertTrue(os.path.isfile(deployed_file_path))
        # Check both files content is the same
        with open(deployed_file_path, "r") as fd:
            data1 = fd.read()
            fd.close()
        cached_file_path = os.path.join(
            self.cache_path, self.ca.NAMESPACE, "fleet-commander"
        )
        with open(cached_file_path, "r") as fd:
            data2 = fd.read()
            fd.close()
        self.assertEqual(data1, data2)


if __name__ == "__main__":
    unittest.main()
