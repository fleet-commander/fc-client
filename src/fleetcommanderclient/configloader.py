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

import logging

import gi
from gi.repository import GLib


class ConfigLoader(object):

    DEFAULTS = {
        'dconf_db_path': '/etc/dconf/db',
        'dconf_profile_path': '/run/dconf/user',
        'goa_run_path': '/run/goa-1.0',
        'log_level': 'info',
    }

    def __init__(self, configfile):
        try:
            self.configfile = configfile
            self.keyfile = GLib.KeyFile.new()
            self.keyfile.load_from_file(configfile, GLib.KeyFileFlags.NONE)
        except Exception, e:
            logging.warning(
                'Can not load config file %s. Using defaults. %s' % (
                    configfile, e))

    def get_value(self, key):
        try:
            return self.keyfile.get_string('fleet-commander', key)
        except Exception, e:
            if key in self.DEFAULTS.keys():
                return self.DEFAULTS[key]
            else:
                logging.warning('Can not read key %s from config: %s' % (
                    key, e))
        return None
