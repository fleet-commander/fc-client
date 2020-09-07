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
import json

from fleetcommanderclient.adapters import BaseAdapter


class ChromiumAdapter(BaseAdapter):
    """
    Chromium configuration adapter class
    """

    # Namespace this config adapter handles
    NAMESPACE = "org.chromium.Policies"

    POLICIES_FILENAME = "fleet-commander-{}.json"

    def __init__(self, policies_path):
        self.policies_path = policies_path

    def _get_policies_file_path(self, uid):
        filename = self.POLICIES_FILENAME.format(uid)
        return os.path.join(self.policies_path, filename)

    def process_config_data(self, config_data, cache_path):
        """
        Process configuration data and save cache files to be deployed.
        This method needs to be defined by each configuration adapter.
        """
        # Prepare data
        policies = {}
        for item in config_data:
            if "key" in item and "value" in item:
                policies[item["key"]] = item["value"]
        # Write policies data
        path = os.path.join(cache_path, "fleet-commander.json")
        logging.debug("Writing policies data to {}".format(path))
        with open(path, "w") as fd:
            fd.write(json.dumps(policies))
            fd.close()

    def deploy_files(self, cache_path, uid):
        """
        Copy cached policies file to policies directory
        This method will be called by privileged process
        """

        cached_file_path = os.path.join(cache_path, "fleet-commander.json")

        if os.path.isfile(cached_file_path):
            logging.debug("Deploying policies at {}.".format(cached_file_path))
            # Create policies path if does not exist
            if not os.path.exists(self.policies_path):
                logging.debug(
                    "Creating policies directory {}".format(self.policies_path)
                )
                try:
                    os.makedirs(self.policies_path)
                except Exception as e:
                    logging.debug(
                        "Failed to create policies directory {}: {}".format(
                            self.policies_path, e
                        )
                    )

            # Delete any previous file at managed profiles
            path = os.path.join(self.policies_path, self.POLICIES_FILENAME.format(uid))
            if os.path.isfile(path):
                logging.debug("Removing previous policies file {}".format(path))
                try:
                    os.remove(path)
                except Exception as e:
                    logging.debug(
                        "Failed to remove old policies file {}: {}".format(path, e)
                    )

            # Deploy new policies file
            logging.debug(
                "Copying policies file at {} to {}".format(cached_file_path, path)
            )
            shutil.copyfile(cached_file_path, path)

            # Change permissions and ownership
            os.chown(path, uid, -1)
            os.chmod(path, stat.S_IREAD)
        else:
            logging.debug("No policies file at {}. Ignoring.".format(cached_file_path))


class ChromeAdapter(ChromiumAdapter):
    """
    Chrome config adapter
    """

    NAMESPACE = "org.google.chrome.Policies"
