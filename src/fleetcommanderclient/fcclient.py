# -*- coding: utf-8 -*-
# vi:ts=4 sw=4 sts=4

# Copyright (C) 2014 Red Hat, Inc.
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

import time
import logging

import dbus
import dbus.service
import dbus.mainloop.glib

import gi
from gi.repository import GObject

DBUS_BUS_NAME = 'org.freedesktop.FleetCommanderClient'
DBUS_OBJECT_PATH = '/org/freedesktop/FleetCommanderClient'
DBUS_INTERFACE_NAME = 'org.freedesktop.FleetCommanderClient'


class FleetCommanderClientDbusClient(object):

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
                self.obj = self.bus.get_object(DBUS_BUS_NAME, DBUS_OBJECT_PATH)
                self.iface = dbus.Interface(
                    self.obj, dbus_interface=DBUS_INTERFACE_NAME)
                return
            except:
                pass
        raise Exception(
            'Timed out connecting to fleet commander client dbus service')

    def sssd_files_ready(self, uid, directory, policy):
        """
        Types:
            uid: Unsigned 32 bit integer (Real local user ID)
            directory: String (Path where the files has been deployed by SSSD)
            policy: Unsigned 16 bit integer (as specified in FreeIPA)
        """
        return self.iface.SSSDFilesReady(uid, directory, policy)


class FleetCommanderClientDbusService(dbus.service.Object):

    """
    Fleet commander client d-bus service class
    """

    def __init__(self, args):
        """
        Class initialization
        """
        # TODO: Get from config file or command line argument
        # Set logging level
        self.log_level = 'debug'
        loglevel = getattr(logging, self.log_level.upper())
        logging.basicConfig(level=loglevel)

        super(FleetCommanderClientDbusService, self).__init__()

    def run(self, sessionbus=False):
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        if not sessionbus:
            bus = dbus.SystemBus()
        else:
            bus = dbus.SessionBus()
        bus_name = dbus.service.BusName(DBUS_BUS_NAME, bus)
        dbus.service.Object.__init__(self, bus_name, DBUS_OBJECT_PATH)
        self._loop = GObject.MainLoop()

        # Enter main loop
        self._loop.run()

    @dbus.service.method(DBUS_INTERFACE_NAME,
                         in_signature='usq', out_signature='b')
    def SSSDFilesReady(self, uid, directory, policy):
        """
        Types:
            uid: Unsigned 32 bit integer (Real local user ID)
            directory: String (Path where the files has been deployed by SSSD)
            policy: Unsigned 16 bit integer (as specified in FreeIPA)
        """
        logging.debug(
            'Fleet Commander Client: Data received - %(u)s - %(d)s - %(p)s' % {
                'u': uid,
                'd': directory,
                'p': policy,
            })
        return True


if __name__ == '__main__':
    svc = FleetCommanderClientDbusService()
    svc.run()
