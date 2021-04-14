#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vi:ts=4 sw=4 sts=4

# Copyright (C) 2015 Red Hat, Inc.
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

import dbus

PYTHONPATH = os.path.join(os.environ["TOPSRCDIR"], "src")
sys.path.append(PYTHONPATH)

# Fleet commander imports
from fleetcommanderclient import fcclientad
from fleetcommanderclient.configloader import ConfigLoader
from fleetcommanderclient.adapters import goa

USER_NAME = "myuser"
USER_UID = 55555


def mocked_uname(uid):
    """
    This is a mock for os.pwd.getpwuid
    """

    class MockPwd:
        pw_name = USER_NAME
        pw_dir = sys.argv[1]

    if uid == USER_UID:
        return MockPwd()
    raise Exception("Unknown UID: %d" % uid)


def universal_function(*args, **kwargs):
    pass


# Monkey patch chown function in os module for chromium config adapter
goa.os.chown = universal_function


class TestConfigLoader(ConfigLoader):
    pass


class TestFleetCommanderClientADDbusService(
    fcclientad.FleetCommanderClientADDbusService
):

    TEST_UUID = 55555

    def __init__(self):

        # Create a config loader that loads modified defaults
        self.tmpdir = sys.argv[1]

        TestConfigLoader.DEFAULTS = {
            "dconf_db_path": os.path.join(self.tmpdir, "etc/dconf/db"),
            "dconf_profile_path": os.path.join(self.tmpdir, "run/dconf/user"),
            "goa_run_path": os.path.join(self.tmpdir, "run/goa-1.0"),
            "log_level": "info",
        }

        fcclientad.ConfigLoader = TestConfigLoader

        super().__init__(configfile="NON_EXISTENT")

        # Put all adapters in test mode
        for adapter in self.adapters.values():
            adapter._TEST_CACHE_PATH = os.path.join(self.tmpdir, "cache")

    def get_peer_uid(self, sender):
        return self.TEST_UUID

    @dbus.service.method(
        fcclientad.DBUS_INTERFACE_NAME, in_signature="", out_signature="b"
    )
    def TestServiceAlive(self):
        return True


if __name__ == "__main__":
    TestFleetCommanderClientADDbusService().run(sessionbus=True)
