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
import json

from fleetcommanderclient.configadapters.base import BaseConfigAdapter


class ChromiumConfigAdapter(BaseConfigAdapter):
    """
    Chromium config adapter
    """

    NAMESPACE = "org.chromium.Policies"
    POLICIES_FILENAME = "fleet-commander-%s.json"

    def __init__(self, policies_path):
        self.policies_path = policies_path

    def bootstrap(self, uid):
        filename = self.POLICIES_FILENAME % uid
        path = os.path.join(self.policies_path, filename)
        # Delete file at managed profiles
        logging.debug('Removing previous policies file: "%s"' % path)
        try:
            os.remove(path)
        except Exception:
            pass

    def update(self, uid, data):
        filename = self.POLICIES_FILENAME % uid
        path = os.path.join(self.policies_path, filename)
        # Create policies path
        try:
            os.makedirs(self.policies_path)
        except Exception:
            pass
        # Prepare data
        policies = {}
        for item in data:
            if "key" in item and "value" in item:
                policies[item["key"]] = item["value"]
        # Write policies data
        logging.debug('Writing %s data to: "%s"' % (self.NAMESPACE, path))
        with open(path, "w") as fd:
            # Change permissions and ownership permisions
            self._set_perms(fd, uid, -1, 0o640)
            fd.write(json.dumps(policies))
            fd.close()


class ChromeConfigAdapter(ChromiumConfigAdapter):
    """
    Chrome config adapter
    """

    NAMESPACE = "org.google.chrome.Policies"
