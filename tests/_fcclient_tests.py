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
import shutil
import tempfile
import subprocess
import time
import unittest
import json
import urllib2

import dbus

PYTHONPATH = os.path.join(os.environ['TOPSRCDIR'], 'src')
sys.path.append(PYTHONPATH)

# Fleet commander imports
from fleetcommanderclient import fcclient


class TestDbusClient(fcclient.FleetCommanderClientDbusClient):
    DEFAULT_BUS = dbus.SessionBus

    def test_service_alive(self):
        return self.iface.TestServiceAlive()

# Mock dbus client
fcclient.FleetCommanderClientDbusClient = TestDbusClient


class TestDbusService(unittest.TestCase):

    maxDiff = None
    MAX_DBUS_CHECKS = 1
    TEST_UID = 1002
    TEST_POLICY = 23

    def setUp(self):
        self.test_directory = tempfile.mkdtemp()

        # Execute dbus service
        self.service = subprocess.Popen([
            os.path.join(
                os.environ['TOPSRCDIR'],
                'tests/test_fcclient_service.py'),
            self.test_directory,
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        checks = 0
        while True:
            try:
                c = self.get_client()
                c.test_service_alive()
                break
            except Exception, e:
                checks += 1
                if checks < self.MAX_DBUS_CHECKS:
                    time.sleep(0.1)
                else:
                    self.service.kill()
                    self.print_dbus_service_output()
                    raise Exception(
                        'DBUS service taking too much time to start: %s' % e)

    def tearDown(self):
        # Kill service
        self.service.kill()
        self.print_dbus_service_output()
        # shutil.rmtree(self.test_directory)

    def print_dbus_service_output(self):
        print('------- BEGIN DBUS SERVICE STDOUT -------')
        print(self.service.stdout.read())
        print('-------- END DBUS SERVICE STDOUT --------')
        print('------- BEGIN DBUS SERVICE STDERR -------')
        print(self.service.stderr.read())
        print('-------- END DBUS SERVICE STDERR --------')

    def get_client(self):
        return fcclient.FleetCommanderClientDbusClient()

    def test_00_process_sssd_files(self):
        c = self.get_client()
        directory = os.path.join(
            os.environ['TOPSRCDIR'],
            'tests/data/sampleprofiledata/')
        c.process_sssd_files(self.TEST_UID, directory, self.TEST_POLICY)
        # TODO: Check files has been created and placed in temporary directory
        self.assertEqual(True, True)


if __name__ == '__main__':
    unittest.main()
