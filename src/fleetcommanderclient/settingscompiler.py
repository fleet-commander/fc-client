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

from fleetcommanderclient import mergers


class SettingsCompiler(object):
    """
    Profile settings compiler class

    Generates final profile settings merging data from files in a given path
    """

    def __init__(self, path):
        self.path = path

        # Initialize data mergers
        self.mergers = {
            'org.gnome.gsettings': mergers.GSettingsMerger(),
            'org.libreoffice.registry': mergers.LibreOfficeMerger(),
            'org.gnome.online-accounts': mergers.GOAMerger(),
            'org.freedesktop.NetworkManager': mergers.NetworkManagerMerger(),
        }

    def get_ordered_file_names(self):
        """
        Get file name list from path given at class initialization
        """
        filenames = os.listdir(path)
        filenames.sort()
        return filenames

    def read_profile_settings(self, filename):
        """
        Read profile settings from given file
        """
        filepath = os.path.join(self.path, filename)
        try:
            fd = open(filepath, 'r')
            contents = fd.read()
            data = json.loads(contents)
            return data
        except Exception, e:
            logging.error(
                'ProfileGenerator: Ignoring profile data from %(f)s: %(e)s' % {
                    'f': filepath,
                    'e': e,
                })
        return {}

    def merge_profile_settings(old, new):
        """
        Merge two profiles overwriting previous values with new ones
        """
        for key, value in new:
            # Check for merger
            if key in self.mergers:
                old[key] = mergers[key].merge(old[key], new[key])
            else:
                # Overwrite key with new one
                old[key] = new[key]
        return old

    def compile_settings(self, path):
        """
        Generate final settings
        """
        filenames = self.get_ordered_file_names(path)
        profile_settings = {}
        for filename in filenames:
            data = process_file_data(filename)
            profile_settings = self.merge_data(profile_settings, data)
        return profile_settings
