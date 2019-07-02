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
import shutil
import logging

import gi
from gi.repository import GLib

from fleetcommanderclient.configadapters.base import BaseConfigAdapter


class GOAConfigAdapter(BaseConfigAdapter):
    """
    Configuration adapter for GNOME Online Accounts
    """

    NAMESPACE = 'org.gnome.online-accounts'
    FC_ACCOUNTS_FILE = 'fleet-commander-accounts.conf'

    def __init__(self, goa_runtime_path):
        self.goa_runtime_path = goa_runtime_path

    def bootstrap(self, uid):
        runtime_path = os.path.join(self.goa_runtime_path, str(uid))
        logging.debug('Removing runtime path for GOA: "%s"' % runtime_path)
        try:
            shutil.rmtree(runtime_path)
        except Exception as e:
            logging.warning('Error removing GOA runtime path "%s": %s' % (
                runtime_path, e))

    def update(self, uid, data):
        # Create runtime path
        runtime_path = os.path.join(self.goa_runtime_path, str(uid))
        logging.debug('Creating runtime path for GOA: "%s"' % runtime_path)
        try:
            os.makedirs(runtime_path)
        except Exception as e:
            logging.error('Error creating GOA runtime path "%s": %s' % (
                runtime_path, e))
            return

        # Prepare data for saving it in keyfile
        logging.debug('Preparing GOA data for saving to keyfile')
        keyfile = GLib.KeyFile.new()
        for account, accountdata in data.items():
            for key, value in accountdata.items():
                if type(value) == bool:
                    keyfile.set_boolean(account, key, value)
                else:
                    keyfile.set_string(account, key, value)

        # Save config file
        keyfile_path = os.path.join(runtime_path, self.FC_ACCOUNTS_FILE)
        logging.debug('Saving GOA keyfile to "%s"' % keyfile_path)
        try:
            keyfile.save_to_file(keyfile_path)
        except Exception as e:
            logging.error('Error saving GOA keyfile at "%s": %s' % (
                keyfile_path, e))
            return

        logging.info('Processed GOA configuration for UID %s')
