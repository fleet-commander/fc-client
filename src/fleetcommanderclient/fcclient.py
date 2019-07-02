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

import os
import logging
import json

import dbus
import dbus.service
import dbus.mainloop.glib

import gi
from gi.repository import GObject

from fleetcommanderclient.configloader import ConfigLoader
from fleetcommanderclient import configadapters
from fleetcommanderclient import adapters
from fleetcommanderclient.settingscompiler import SettingsCompiler

DBUS_BUS_NAME = 'org.freedesktop.FleetCommanderClient'
DBUS_OBJECT_PATH = '/org/freedesktop/FleetCommanderClient'
DBUS_INTERFACE_NAME = 'org.freedesktop.FleetCommanderClient'


class FleetCommanderClientDbusService(dbus.service.Object):

    """
    Fleet commander client d-bus service class
    """

    def __init__(self, configfile='/etc/xdg/fleet-commander-client.conf'):
        """
        Class initialization
        """
        # Load configuration options
        self.config = ConfigLoader(configfile)

        # Set logging level
        self.log_level = self.config.get_value('log_level')
        loglevel = getattr(logging, self.log_level.upper())
        logging.basicConfig(level=loglevel)

        # Configuration adapters (old)
        self.config_adapters = {}

        self.register_config_adapter(
            configadapters.DconfConfigAdapter,
            self.config.get_value('dconf_profile_path'),
            self.config.get_value('dconf_db_path'))

        self.register_config_adapter(
            configadapters.GOAConfigAdapter,
            self.config.get_value('goa_run_path'))

        self.register_config_adapter(
            configadapters.NetworkManagerConfigAdapter)

        self.register_config_adapter(
            configadapters.ChromiumConfigAdapter,
            self.config.get_value('chromium_policies_path'))

        self.register_config_adapter(
            configadapters.ChromeConfigAdapter,
            self.config.get_value('chrome_policies_path'))

        self.register_config_adapter(
            configadapters.FirefoxConfigAdapter,
            self.config.get_value('firefox_prefs_path'))

        # Configuration adapters (new)
        self.adapters = {}

        self.register_adapter(
            adapters.DconfAdapter,
            self.config.get_value('dconf_profile_path'),
            self.config.get_value('dconf_db_path'))

        self.register_adapter(
            adapters.GOAAdapter,
            self.config.get_value('goa_run_path'))

        self.register_adapter(
            adapters.NetworkManagerAdapter)

        self.register_adapter(
            adapters.ChromiumAdapter,
            self.config.get_value('chromium_policies_path'))

        self.register_adapter(
            adapters.ChromeAdapter,
            self.config.get_value('chrome_policies_path'))

        self.register_adapter(
            adapters.FirefoxAdapter,
            self.config.get_value('firefox_prefs_path'))


        # Parent initialization
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

    def quit(self):
        self._loop.quit()

    def register_adapter(self, adapterclass, *args, **kwargs):
        self.adapters[adapterclass.NAMESPACE] = adapterclass(*args, **kwargs)

    def register_config_adapter(self, adapterclass, *args, **kwargs):
        self.config_adapters[adapterclass.NAMESPACE] = adapterclass(*args, **kwargs)

    def get_peer_uid(self, sender):
        proxy = dbus.SystemBus().get_object('org.freedesktop.DBus', '/')
        interface = dbus.Interface(
            proxy, dbus_interface='org.freedesktop.DBus')
        return interface.GetConnectionUnixUser(sender)


    @dbus.service.method(DBUS_INTERFACE_NAME,
                         in_signature='usq', out_signature='')
    def ProcessSSSDFiles(self, uid, directory, policy):
        """
        Types:
            uid: Unsigned 32 bit integer (Real local user ID)
            directory: String (Path where the files has been deployed by SSSD)
            policy: Unsigned 16 bit integer (as specified in FreeIPA)
        """

        logging.debug(
            'FC Client: SSSD Data received - %(u)s - %(d)s - %(p)s' % {
                'u': uid,
                'd': directory,
                'p': policy,
            })

        # Compile settings
        sc = SettingsCompiler(directory)
        logging.debug('FC Client: Compiling settings')
        compiled_settings = sc.compile_settings()
        # Send data to configuration adapters
        logging.debug('FC Client: Applying settings')
        for namespace in compiled_settings:
            logging.debug('FC Client: Checking adapters for namespace %s' % namespace)
            if namespace in self.config_adapters:
                logging.debug('FC Client: Applying settings for namespace %s' % namespace)
                self.config_adapters[namespace].bootstrap(uid)
                data = compiled_settings[namespace]
                self.config_adapters[namespace].update(uid, data)
        self.quit()


    @dbus.service.method(DBUS_INTERFACE_NAME,
                         in_signature='', out_signature='',
                         message_keyword='dbusmessage')
    def ProcessFiles(self, dbusmessage):

        logging.debug(
            'FC Client: Applying user configuration')

        # Get peer UID for security
        uid = self.get_peer_uid(dbusmessage.get_sender())

        logging.debug(
            'FC Client: Got peer UID: {}'.format(uid))

        # Cycle through configuration adapters and deploy existing data
        for namespace, adapter in self.adapters.items():
            logging.debug(
                'FC Client: Deploying configuration for namespace {}'.format(
                    namespace))

            adapter.deploy(uid)
        self.quit()

    @dbus.service.method(DBUS_INTERFACE_NAME,
                         in_signature='', out_signature='')
    def Quit(self):
        self.quit()


if __name__ == '__main__':
    svc = FleetCommanderClientDbusService()
    svc.run()
