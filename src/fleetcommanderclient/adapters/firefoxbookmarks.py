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
import stat
import logging
import shutil
import json

from fleetcommanderclient.adapters.base import BaseAdapter


class FirefoxBookmarksAdapter(BaseAdapter):
    """
    Firefox bookmarks configuration adapter class
    """

    # Namespace this config adapter handles
    NAMESPACE = "org.mozilla.firefox.Bookmarks"
    POLICIES_FILENAME = "policies.json"

    def __init__(self, policies_path):
        self.policies_path = policies_path

    def process_config_data(self, config_data, cache_path):
        """
        Process configuration data and save cache files to be deployed.
        This method needs to be defined by each configuration adapter.
        """
        # Prepare data
        bookmarks = []
        for item in config_data:
            if "key" in item and "value" in item:
                bookmarks.append(item["value"])
        policies_data = {"policies": {"Bookmarks": bookmarks}}
        # Write preferences data
        path = os.path.join(cache_path, "fleet-commander")
        logging.debug("Writing preferences data to %s", path)
        with open(path, "w") as fd:
            fd.write(json.dumps(policies_data))
            fd.close()

    def deploy_files(self, cache_path, uid):
        """
        Copy cached policies file to policies directory
        This method will be called by privileged process
        """
        cached_file_path = os.path.join(cache_path, "fleet-commander")
        if os.path.isfile(cached_file_path):
            logging.debug("Deploying preferences at %s.", cached_file_path)
            directory = self.policies_path.format(uid)
            path = os.path.join(directory, self.POLICIES_FILENAME)
            # Remove previous preferences file
            logging.debug("Removing previous policies file %s", path)
            try:
                os.remove(path)
            except Exception as e:
                logging.debug("Failed to remove previous policies file %s: %s", path, e)
            # Create directory
            try:
                os.makedirs(directory)
            except Exception:
                pass
            # Deploy new policies file
            logging.debug("Copying policies file at %s to %s", cached_file_path, path)
            shutil.copyfile(cached_file_path, path)
            # Change permissions and ownership
            os.chown(path, uid, -1)
            os.chmod(path, stat.S_IREAD)
        else:
            logging.debug("No policies file at %s. Ignoring.", cached_file_path)
