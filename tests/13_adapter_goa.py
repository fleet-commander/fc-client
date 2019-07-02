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
import stat
import unittest

from gi.repository import GLib

sys.path.append(os.path.join(os.environ['TOPSRCDIR'], 'src'))

import fleetcommanderclient.adapters.goa
from fleetcommanderclient.adapters.goa import GOAAdapter


def universal_function(*args, **kwargs):
    pass

# Monkey patch chown function in os module for chromium config adapter
fleetcommanderclient.adapters.goa.os.chown = universal_function


# Set log level to debug
logging.basicConfig(level=logging.DEBUG)


class TestGOAAdapter(unittest.TestCase):

    TEST_UID = 55555

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
        self.test_directory = tempfile.mkdtemp(
            prefix='fc-client-goa-test')
        self.cache_path = os.path.join(self.test_directory, 'cache')
        self.ca = GOAAdapter(self.test_directory)
        self.ca._TEST_CACHE_PATH = self.cache_path

    def tearDown(self):
        # Change permissions of directories to allow removal
        runtime_dir = os.path.join(
            self.test_directory,
            str(self.TEST_UID))
        if os.path.exists(runtime_dir):
            os.chmod(
                runtime_dir,
                stat.S_IREAD | stat.S_IWRITE | stat.S_IEXEC)
        # Remove test directory
        shutil.rmtree(self.test_directory)

    def test_00_generate_config(self):
        # Generate configuration
        self.ca.generate_config(self.TEST_DATA)
        # Check configuration file exists
        filepath = os.path.join(
            self.cache_path,
            self.ca.NAMESPACE,
            self.ca.ACCOUNTS_FILE)
        logging.debug('Checking {} exists'.format(filepath))
        self.assertTrue(os.path.exists(filepath))
        # Check configuration file contents

        # Read keyfile
        keyfile = GLib.KeyFile.new()
        keyfile.load_from_file(filepath, GLib.KeyFileFlags.NONE)

        # Check section list
        accounts = self.TEST_DATA.keys()
        accounts_keyfile = keyfile.get_groups()[0]
        accounts_keyfile.sort()
        self.assertEqual(sorted(accounts), accounts_keyfile)

    def test_01_deploy(self):
        # Generate config files in cache
        self.ca.generate_config(self.TEST_DATA)
        # Execute deployment
        self.ca.deploy(self.TEST_UID)
        # Check file has been copied to policies path
        deployed_file_path = os.path.join(
            self.test_directory,
            str(self.TEST_UID),
            self.ca.ACCOUNTS_FILE)
        self.assertTrue(os.path.isfile(deployed_file_path))
        # Check both files content is the same
        with open(deployed_file_path, 'r') as fd:
            data1 = fd.read()
            fd.close()
        cached_file_path = os.path.join(
            self.cache_path,
            self.ca.NAMESPACE,
            self.ca.ACCOUNTS_FILE)
        with open(cached_file_path, 'r') as fd:
            data2 = fd.read()
            fd.close()
        self.assertEqual(data1, data2)


if __name__ == '__main__':
    unittest.main()
