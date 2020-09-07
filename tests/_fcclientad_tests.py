#!/usr/bin/env python-wrapper.sh
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

import dbus

PYTHONPATH = os.path.join(os.environ["TOPSRCDIR"], "src")
sys.path.append(PYTHONPATH)

# Fleet commander imports
from fleetcommanderclient import fcclientad


class FleetCommanderClientADDbusClient(object):
    """
    Fleet commander client dbus client
    """

    DEFAULT_BUS = dbus.SessionBus
    CONNECTION_TIMEOUT = 2

    def __init__(self, bus=None):
        """
        Class initialization
        """
        if bus is None:
            bus = self.DEFAULT_BUS()
        self.bus = bus

        t = time.time()
        while time.time() - t < self.CONNECTION_TIMEOUT:
            try:
                self.obj = self.bus.get_object(
                    fcclientad.DBUS_BUS_NAME, fcclientad.DBUS_OBJECT_PATH
                )
                self.iface = dbus.Interface(
                    self.obj, dbus_interface=fcclientad.DBUS_INTERFACE_NAME
                )
                return
            except Exception:
                pass
        raise Exception("Timed out connecting to fleet commander client dbus service")

    def process_files(self):
        return self.iface.ProcessFiles()


class TestDbusClient(FleetCommanderClientADDbusClient):
    DEFAULT_BUS = dbus.SessionBus

    def test_service_alive(self):
        return self.iface.TestServiceAlive()


# Mock dbus client
fcclientad.FleetCommanderClientADDbusClient = TestDbusClient


class TestDbusService(unittest.TestCase):

    maxDiff = None
    MAX_DBUS_CHECKS = 1

    CACHE_FILEPATHS = [
        "org.gnome.online-accounts/fleet-commander-accounts.conf",
    ]

    def setUp(self):
        self.test_directory = tempfile.mkdtemp()

        # Execute dbus service
        self.service = subprocess.Popen(
            [
                os.path.join(
                    os.environ["TOPSRCDIR"], "tests/test_fcclientad_service.py"
                ),
                self.test_directory,
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        checks = 0
        while True:
            try:
                c = self.get_client()
                c.test_service_alive()
                break
            except Exception as e:
                checks += 1
                if checks < self.MAX_DBUS_CHECKS:
                    time.sleep(0.1)
                else:
                    self.service.kill()
                    self.print_dbus_service_output()
                    raise Exception(
                        "DBUS service taking too much time to start: %s" % e
                    )

    def tearDown(self):
        # Kill service
        self.service.kill()
        self.print_dbus_service_output()
        # shutil.rmtree(self.test_directory)

    def print_dbus_service_output(self):
        print("------- BEGIN DBUS SERVICE STDOUT -------")
        print(self.service.stdout.read())
        print("-------- END DBUS SERVICE STDOUT --------")
        print("------- BEGIN DBUS SERVICE STDERR -------")
        print(self.service.stderr.read())
        print("-------- END DBUS SERVICE STDERR --------")

    def get_client(self):
        return TestDbusClient()

    def test_00_process_files(self):
        c = self.get_client()

        # Create fake compiled files where dbus service expect them
        for fpath in self.CACHE_FILEPATHS:
            fname = os.path.join(self.test_directory, "cache", fpath)
            fdir = os.path.dirname(fname)
            if not os.path.isdir(fdir):
                os.makedirs(fdir)
            with open(fname, "w") as fd:
                fd.write("{}")
                fd.close()

        c.process_files()

        # Check GOA accounts file has been deployed
        self.assertTrue(
            os.path.isfile(
                os.path.join(
                    self.test_directory,
                    "run/goa-1.0/55555/fleet-commander-accounts.conf",
                )
            )
        )


if __name__ == "__main__":
    unittest.main()
