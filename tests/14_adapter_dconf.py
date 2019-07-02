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

import fleetcommanderclient.adapters.dconf
from fleetcommanderclient.adapters.dconf import DconfAdapter


def universal_function(*args, **kwargs):
    pass

# Monkey patch chown function in os module for chromium config adapter
fleetcommanderclient.adapters.dconf.os.chown = universal_function


# Set log level to debug
logging.basicConfig(level=logging.DEBUG)


class TestDconfAdapter(unittest.TestCase):

    TEST_UID = 55555

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

    DCONF_USER_FILE_CONTENTS = 'user-db:user\n\nsystem-db:{}'

    def setUp(self):
        self.test_directory = tempfile.mkdtemp(
            prefix='fc-client-dconf-test')
        self.cache_path = os.path.join(self.test_directory, 'cache')
        self.ca = DconfAdapter(self.test_directory, self.test_directory)
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
        # Check keyfiles dir exist
        keyfiles_dir = os.path.join(
            self.cache_path, self.ca.NAMESPACE, 'keyfiles')
        self.assertTrue(os.path.isdir(keyfiles_dir))

        # Check keyfile exists
        keyfile_path = os.path.join(keyfiles_dir, self.ca.PROFILE_FILE)
        self.assertTrue(os.path.isfile(keyfile_path))

        # Read keyfile
        keyfile = GLib.KeyFile.new()
        keyfile.load_from_file(keyfile_path, GLib.KeyFileFlags.NONE)

        # Check all sections
        for item in self.TEST_DATA:
            # Check all keys and values
            keysplit = item['key'][1:].split('/')
            keypath = '/'.join(keysplit[:-1])
            keyname = keysplit[-1]
            value = item['value']
            value_keyfile = keyfile.get_string(keypath, keyname)
            self.assertEqual(value, value_keyfile)

        # Check db file exists
        dbfile_path = os.path.join(
            self.cache_path, self.ca.NAMESPACE, self.ca.DB_FILE)
        self.assertTrue(os.path.isfile(dbfile_path))

        # Check db file contents
        with open(dbfile_path, 'r') as fd:
            data = fd.read()
            fd.close()
        self.assertEqual(data, 'COMPILED\n')

    def test_01_deploy(self):
        # Generate config files in cache
        self.ca.generate_config(self.TEST_DATA)
        # Execute deployment
        self.ca.deploy(self.TEST_UID)

        # Check db file has been copied to db path
        deployed_file_name = '{}-{}'.format(self.ca.DB_FILE, self.TEST_UID)
        deployed_file_path = os.path.join(
            self.test_directory,
            deployed_file_name)
        self.assertTrue(os.path.isfile(deployed_file_path))

        # Check both files content is the same
        with open(deployed_file_path, 'r') as fd:
            data1 = fd.read()
            fd.close()
        cached_file_path = os.path.join(
            self.cache_path,
            self.ca.NAMESPACE,
            self.ca.DB_FILE)
        with open(cached_file_path, 'r') as fd:
            data2 = fd.read()
            fd.close()
        self.assertEqual(data1, data2)

        # Check user dconf file has been created
        dconf_user_file_path = os.path.join(
            self.test_directory,
            '{}'.format(self.TEST_UID))
        self.assertTrue(os.path.isfile(deployed_file_path))

        # Check both files content is the same
        with open(dconf_user_file_path, 'r') as fd:
            data = fd.read()
            fd.close()
        self.assertEqual(
            data,
            self.DCONF_USER_FILE_CONTENTS.format(
                deployed_file_name))


if __name__ == '__main__':
    unittest.main()
