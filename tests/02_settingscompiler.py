#!/usr/bin/python
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

PYTHONPATH = os.path.join(os.environ['TOPSRCDIR'], 'src')
sys.path.append(PYTHONPATH)

# Fleet commander imports
from fleetcommanderclient.settingscompiler import SettingsCompiler


class TestSettingsCompiler(unittest.TestCase):

    maxDiff = None

    ordered_filenames = [
        '0050-0050-0000-0000-0000-Test_Profile_1.profile',
        '0060-0060-0000-0000-0000-Test_Profile_2.profile',
        '0070-0070-0000-0000-0000-Invalid.profile',
        '0090-0090-0000-0000-0000-Test_Profile_3.profile',
    ]

    invalid_profile_filename = ordered_filenames[2]

    PROFILE_1_SETTINGS = {}

    PROFILE_2_SETTINGS = {}

    PROFILE_SETTINGS_MERGED = {}

    COMPILED_SETTINGS = {}

    def setUp(self):
        self.sc = SettingsCompiler('./data/sampleprofiledata/')

    def test_00_get_ordered_file_names(self):
        # Read from invalid filename
        result = self.sc.get_ordered_file_names()
        self.assertEqual(result, self.ordered_filenames)

    def test_01_read_profile_settings(self):
        # Read from invalid filename
        result = self.sc.read_profile_settings('nonexistent')
        self.assertEqual(result, {})
        # Read invalid data
        result = self.sc.read_profile_settings(self.invalid_profile_filename)
        self.assertEqual(result, {})
        # Read valid data
        result = self.sc.read_profile_settings(self.ordered_filenames[0])
        self.assertEqual(result, self.PROFILE_1_SETTINGS)

    def test_02_merge_profile_settings(self):
        # Read from invalid filename
        result = self.sc.merge_profile_settings(
            self.PROFILE_1_SETTINGS, self.PROFILE_2_SETTINGS)
        self.assertEqual(result, self.PROFILE_SETTINGS_MERGED)

    def test_03_compile_settings(self):
        # Read from invalid filename
        result = self.sc.compile_settings()
        self.assertEqual(result, self.COMPILED_SETTINGS)

if __name__ == '__main__':
    unittest.main()
