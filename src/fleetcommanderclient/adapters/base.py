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
import pwd
import shutil
import logging


class BaseAdapter:
    """
    Base configuration adapter class
    """

    # Namespace this config adapter handles
    NAMESPACE = None

    # Variable for setting cache path for testing
    _TEST_CACHE_PATH = None

    def _get_cache_path(self, uid=None):
        # Use test cache path while testing
        if self._TEST_CACHE_PATH is not None:
            return os.path.join(self._TEST_CACHE_PATH, self.NAMESPACE)

        if uid is None:
            # Use current user home cache directory
            return os.path.join(
                os.path.expanduser("~"), ".cache/fleet-commander", self.NAMESPACE
            )
        # Get user directory from password database
        homedir = pwd.getpwuid(uid).pw_dir
        return os.path.join(homedir, ".cache/fleet-commander/", self.NAMESPACE)

    def cleanup_cache(self, namespace_cache_path=None):
        """
        Removes all files under cache for this adapter namespace
        """
        if namespace_cache_path is None:
            namespace_cache_path = self._get_cache_path()
        logging.debug("Cleaning up cache path %s", namespace_cache_path)
        if os.path.exists(namespace_cache_path):
            shutil.rmtree(namespace_cache_path)

    def generate_config(self, config_data):
        """
        Prepare files to be deployed
        """
        namespace_cache_path = self._get_cache_path()
        # Cleaning up cache path
        self.cleanup_cache(namespace_cache_path)
        # Create namespace cache path
        logging.debug("Creating cache path %s", namespace_cache_path)
        os.makedirs(namespace_cache_path)
        logging.debug("Processing data configuration for namespace %s", self.NAMESPACE)
        self.process_config_data(config_data, namespace_cache_path)

    def deploy(self, uid):
        """
        Deploy configuration method
        """
        namespace_cache_path = self._get_cache_path(uid)
        self.deploy_files(namespace_cache_path, uid)

    def process_config_data(self, config_data, cache_path):
        """
        Process configuration data and save cache files to be deployed.
        This method needs to be defined by each configuration adapter.
        """
        raise NotImplementedError("You must implement generate_config_data method")

    def deploy_files(self, cache_path, uid):
        """
        File deployment method to be defined by each configuration adapter.
        This method will be called by privileged process
        """
        raise NotImplementedError("You must implement deploy_files method")
