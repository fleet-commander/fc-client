#!/usr/bin/python
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
import logging

import dbus

PYTHONPATH = os.path.join(os.environ['TOPSRCDIR'], 'src')
sys.path.append(PYTHONPATH)

# Fleet commander imports
from fleetcommanderclient import fcclient
from fleetcommanderclient.configloader import ConfigLoader


class TestConfigLoader(ConfigLoader):
    pass


class TestFleetCommanderClientDbusService(
        fcclient.FleetCommanderClientDbusService):

    def __init__(self):

        # Create a config loader that loads modified defaults
        self.tmpdir = sys.argv[1]

        TestConfigLoader.DEFAULTS = {
            'dconf_db_path': os.path.join(self.tmpdir, 'etc/dconf/db'),
            'dconf_profile_path': os.path.join(self.tmpdir, 'run/dconf/user'),
            'goa_run_path': os.path.join(self.tmpdir, 'run/goa-1.0'),
            'log_level': 'info',
        }

        fcclient.ConfigLoader = TestConfigLoader

        super(TestFleetCommanderClientDbusService, self).__init__(configfile='NON_EXISTENT')


    @dbus.service.method(fcclient.DBUS_INTERFACE_NAME,
                         in_signature='', out_signature='b')
    def TestServiceAlive(self):
        return True


if __name__ == '__main__':
    TestFleetCommanderClientDbusService().run(sessionbus=True)
