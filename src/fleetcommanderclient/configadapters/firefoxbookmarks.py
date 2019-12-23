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
# Author: Oliver Gutiérrez <ogutierrez@redhat.com>

import os
import stat
import logging
import json

from fleetcommanderclient.configadapters.base import BaseConfigAdapter


class FirefoxBookmarksConfigAdapter(BaseConfigAdapter):
    """
    Firefox Bookmarks config adapter
    """

    NAMESPACE = 'org.mozilla.firefox.Bookmarks'
    POLICIES_FILENAME = 'policies.json'

    def __init__(self, policies_path):
        logging.debug(
            'Initialized firefox bookmarks config adapter with policies path {}'.format(
                policies_path))
        self.policies_path = policies_path

    def bootstrap(self, uid):
        path = os.path.join(self.policies_path.format(uid), self.POLICIES_FILENAME)
        # Delete existing files
        logging.debug('Removing previous policies file: "{}"'.format(path))
        try:
            os.remove(path)
        except Exception as e:
            logging.debug('Error removing previous policies file "{}": {}'.format(path, e))
            pass

    def update(self, uid, data):
        logging.debug('Updating {}. Data received: {}'.format(self.NAMESPACE, data))
        directory = self.policies_path.format(uid)
        path = os.path.join(directory, self.POLICIES_FILENAME)
        # Create directory
        try:
            os.makedirs(directory)
        except Exception:
            pass
        # Prepare data
        bookmarks = []
        for item in data:
            if 'key' in item and 'value' in item:
                bookmarks.append(item['value'])
        policies_data = {"policies": {'Bookmarks': bookmarks}}
        # Write preferences data
        logging.debug('Writing {} data to: "{}"'.format(self.NAMESPACE, path))
        with open(path, 'w') as fd:
            fd.write(json.dumps(policies_data))
            fd.close()
        # Change permissions and ownership permisions
        os.chown(path, uid, -1)
        os.chmod(path, stat.S_IREAD)
