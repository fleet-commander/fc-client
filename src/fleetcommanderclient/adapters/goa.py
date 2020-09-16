# -*- coding: utf-8 -*-
# vi:ts=4 sw=4 sts=4

# Copyright (C) 2019 Red Hat, Inc.
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
import stat
import logging
import shutil

from gi.repository import GLib

from fleetcommanderclient.adapters.base import BaseAdapter


class GOAAdapter(BaseAdapter):
    """
    GOA configuration adapter class
    """

    # Namespace this config adapter handles
    NAMESPACE = "org.gnome.online-accounts"

    ACCOUNTS_FILE = "fleet-commander-accounts.conf"

    def __init__(self, goa_runtime_path):
        self.goa_runtime_path = goa_runtime_path

    def process_config_data(self, config_data, cache_path):
        """
        Process configuration data and save cache files to be deployed.
        This method needs to be defined by each configuration adapter.
        """
        # Prepare data for saving it in keyfile
        logging.debug("Preparing GOA data for saving to keyfile")
        keyfile = GLib.KeyFile.new()
        for account, accountdata in config_data.items():
            for key, value in accountdata.items():
                if isinstance(value, bool):
                    keyfile.set_boolean(account, key, value)
                else:
                    keyfile.set_string(account, key, value)

        # Save config file
        keyfile_path = os.path.join(cache_path, self.ACCOUNTS_FILE)
        logging.debug('Saving GOA keyfile to "%s"', keyfile_path)
        try:
            keyfile.save_to_file(keyfile_path)
        except Exception as e:
            logging.error("Error saving GOA keyfile at %s: %s", keyfile_path, e)
            return

    def deploy_files(self, cache_path, uid):
        """
        Copy cached policies file to policies directory
        This method will be called by privileged process
        """
        cached_file_path = os.path.join(cache_path, self.ACCOUNTS_FILE)

        if os.path.isfile(cached_file_path):
            logging.debug("Deploying GOA accounts from %s", cached_file_path)

            # Remove previous GOA files
            runtime_path = os.path.join(self.goa_runtime_path, str(uid))
            logging.debug("Removing GOA runtime path %s", runtime_path)
            try:
                shutil.rmtree(runtime_path)
            except Exception as e:
                logging.warning(
                    "Error removing GOA runtime path %s: %s", runtime_path, e
                )

            # Create runtime path
            logging.debug("Creating GOA runtime path %s", runtime_path)
            try:
                os.makedirs(runtime_path)
            except Exception as e:
                logging.error("Error creating GOA runtime path %s: %s", runtime_path, e)
                return

            # Copy file from cache to runtime path
            deploy_file_path = os.path.join(runtime_path, self.ACCOUNTS_FILE)
            shutil.copyfile(cached_file_path, deploy_file_path)

            # Change permissions and ownership for accounts file
            os.chown(deploy_file_path, uid, -1)
            os.chmod(deploy_file_path, stat.S_IREAD)

            # Change permissions and ownership for GOA runtime directory
            os.chown(runtime_path, uid, -1)
            os.chmod(runtime_path, stat.S_IREAD | stat.S_IEXEC)
        else:
            logging.debug("GOA accounts file %s is not present", cached_file_path)
