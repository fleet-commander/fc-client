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
            "org.gnome.gsettings": mergers.GSettingsMerger(),
            "org.libreoffice.registry": mergers.LibreOfficeMerger(),
            "org.gnome.online-accounts": mergers.GOAMerger(),
            "org.mozilla.firefox": mergers.FirefoxMerger(),
            "org.freedesktop.NetworkManager": mergers.NetworkManagerMerger(),
        }

    def get_ordered_file_names(self):
        """
        Get file name list from path given at class initialization
        """
        filenames = os.listdir(self.path)
        filenames.sort()
        return filenames

    def read_profile_settings(self, filename):
        """
        Read profile settings from given file
        """
        filepath = os.path.join(self.path, filename)
        try:
            with open(filepath, "r") as fd:
                contents = fd.read()
                data = json.loads(contents)
                fd.close()
                return data
        except Exception as e:
            logging.error(
                "ProfileGenerator: Ignoring profile data from %(f)s: %(e)s"
                % {
                    "f": filepath,
                    "e": e,
                }
            )
        return {}

    def merge_profile_settings(self, old, new):
        """
        Merge two profiles overwriting previous values with new ones
        """
        for namespace, settings in new.items():
            # Check for merger
            if namespace in self.mergers:
                if namespace not in old.keys():
                    old[namespace] = new[namespace]
                else:
                    old[namespace] = self.mergers[namespace].merge(
                        old[namespace], new[namespace]
                    )
            else:
                old[namespace] = new[namespace]
        return old

    def compile_settings(self):
        """
        Generate final settings
        """
        filenames = self.get_ordered_file_names()
        profile_settings = {}
        for filename in filenames:
            data = self.read_profile_settings(filename)
            profile_settings = self.merge_profile_settings(profile_settings, data)

        # FIXME: Right now merging libreoffice config data into gsettings
        #        because both use the same configuration adapter.
        #        We should change the config adapter interface to allow
        #        multiple namespaces for the same config adapter.

        if "org.libreoffice.registry" in profile_settings.keys():
            libreoffice_data = {
                "org.gnome.gsettings": profile_settings["org.libreoffice.registry"]
            }
            profile_settings = self.merge_profile_settings(
                profile_settings, libreoffice_data
            )
        return profile_settings
