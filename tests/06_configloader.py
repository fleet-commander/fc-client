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
from fleetcommanderclient.configloader import ConfigLoader


class TestConfigLoader(unittest.TestCase):

    maxDiff = None

    def test_00_load_inexistent_config_file(self):
        # Load inexistent configuration file
        configfile = os.path.join(
            os.environ['TOPSRCDIR'], 'tests/data/inexistent_config_file.conf')
        config = ConfigLoader(configloader)
        # Read a non existent key without default specified
        result = config.get_value('inexistent_key')
        self.assertEqual(result, None)
        # Read non existent key but with default value
        for key, value in config.DEFAULTS.keys():
            result = config.get_value(key)
            self.assertEqual(result, value)

    def test_01_load_config_file(self):
        # Load inexistent configuration file
        configfile = os.path.join(
            os.environ['TOPSRCDIR'], 'tests/data/test_config_file.conf')
        config = ConfigLoader(configloader)
        # Read a non existent key without default specified
        result = config.get_value('inexistent_key')
        self.assertEqual(result, None)
        # Read non existent key but with default value
        result = config.get_value('debug_level')
        self.assertEqual(result, config.DEFAULTS['debug_level'])
        # Read existent key
        result = config.get_value('goa_run_path')
        self.assertEqual(result, '/run/goa-custom-path-1.0')
