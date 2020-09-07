# -*- coding: utf-8 -*-
# vi:ts=4 sw=4 sts=4

# Copyright (C) 2018 Red Hat, Inc.
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


class FirefoxConfigAdapter(BaseConfigAdapter):
    """
    Firefox config adapter
    """

    NAMESPACE = "org.mozilla.firefox"
    PREFS_FILENAME = "fleet-commander-%s.json"
    PREF_TEMPLATE = 'pref("%s", %s);'

    def __init__(self, preferences_path):
        self.preferences_path = preferences_path

    def bootstrap(self, uid):
        filename = self.PREFS_FILENAME % uid
        path = os.path.join(self.preferences_path, filename)
        # Delete file at preferences path
        logging.debug('Removing previous preferences file: "%s"' % path)
        try:
            os.remove(path)
        except Exception:
            pass

    def update(self, uid, data):
        logging.debug("Updating %s. Data received: %s" % (self.NAMESPACE, data))
        filename = self.PREFS_FILENAME % uid
        path = os.path.join(self.preferences_path, filename)
        # Create preferences path
        try:
            os.makedirs(self.preferences_path)
        except Exception:
            pass
        # Prepare data
        preferences = []
        for item in data:
            if "key" in item and "value" in item:
                # TODO: Check for locked settings and use lockPref inst
                preferences.append(
                    self.PREF_TEMPLATE % (item["key"], json.dumps(item["value"]))
                )
        # Write preferences data
        logging.debug('Writing %s data to: "%s"' % (self.NAMESPACE, path))
        with open(path, "w") as fd:
            # Change permissions and ownership permisions
            self._set_perms(fd, uid, -1, 0o640)
            fd.write("\n".join(preferences))
            fd.close()
