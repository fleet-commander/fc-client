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
import subprocess

import gi
from gi.repository import GLib

from fleetcommanderclient.configadapters.base import BaseConfigAdapter


class DconfConfigAdapter(BaseConfigAdapter):
    """
    Configuration adapter for Dconf
    """

    NAMESPACE = "org.gnome.gsettings"
    FC_PROFILE_FILE = "fleet-commander-dconf.conf"
    FC_DB_FILE = "fleet-commander-dconf-"

    def __init__(self, dconf_profile_path, dconf_db_path):
        self.dconf_profile_path = dconf_profile_path
        self.dconf_db_path = dconf_db_path

    def get_paths_for_uid(self, uid):
        profile_path = os.path.join(self.dconf_profile_path, str(uid))
        keyfile_dir = os.path.join(
            self.dconf_db_path, "%s%s.d" % (self.FC_DB_FILE, str(uid))
        )
        db_path = os.path.join(self.dconf_db_path, "%s%s" % (self.FC_DB_FILE, str(uid)))
        return (profile_path, keyfile_dir, db_path)

    def remove_path(self, path, throw=False):
        logging.debug('Removing path: "%s"' % path)
        try:
            if os.path.exists(path):
                if os.path.isdir(path):
                    shutil.rmtree(path)
                else:
                    os.remove(path)
        except Exception as e:
            if throw:
                logging.error('Error removing path "%s": %s' % (path, e))
                raise e
            else:
                logging.warning('Error removing path "%s": %s' % (path, e))

    def bootstrap(self, uid):
        # Remove old data
        profile_path, keyfile_dir, db_path = self.get_paths_for_uid(uid)
        self.remove_path(profile_path)
        self.remove_path(db_path)
        self.remove_path(keyfile_dir)

    def update(self, uid, data):
        profile_path, keyfile_dir, db_path = self.get_paths_for_uid(uid)

        # Prepare data for saving it in keyfile
        logging.debug("Preparing dconf data for saving to keyfile")
        keyfile = GLib.KeyFile.new()
        for item in data:
            if "key" in item and "value" in item:
                keysplit = item["key"][1:].split("/")
                keypath = "/".join(keysplit[:-1])
                keyname = keysplit[-1]
                keyfile.set_string(keypath, keyname, item["value"])

        # Create keyfile path
        logging.debug('Creating keyfile path for dconf: "%s"' % profile_path)
        try:
            os.makedirs(keyfile_dir)
        except Exception:
            logging.error('Error creating keyfile path "%s": %s' % (path, e))
            return

        # Save config file
        keyfile_path = os.path.join(keyfile_dir, self.FC_PROFILE_FILE)
        logging.debug('Saving dconf keyfile to "%s"' % keyfile_path)
        try:
            keyfile.save_to_file(keyfile_path)
        except Exception as e:
            logging.error('Error saving dconf keyfile at "%s": %s' % (keyfile_path, e))
            return

        # Compile dconf database
        try:
            self._compile_dconf_db(uid)
        except Exception as e:
            logging.error('Error compiling dconf data to "%s": %s' % (db_path, e))
            return

        # Create runtime path
        logging.debug('Creating profile path for dconf: "%s"' % profile_path)
        try:
            os.makedirs(self.dconf_profile_path)
        except Exception:
            pass
        try:
            profile_data = "user-db:user\n\nsystem-db:%s%s" % (self.FC_DB_FILE, uid)
            with open(profile_path, "w") as fd:
                fd.write(profile_data)
                fd.close()
        except Exception as e:
            logging.error('Error saving dconf profile at "%s": %s' % (profile_path, e))
            return

        logging.info("Processed dconf configuration for UID %s")

    def _compile_dconf_db(self, uid):
        """
        Compiles dconf database
        """
        profile_path, keyfile_dir, db_path = self.get_paths_for_uid(uid)

        # Execute dbus service
        cmd = subprocess.Popen(
            [
                "dconf",
                "compile",
                db_path,
                keyfile_dir,
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        cmd.wait()
        out = cmd.stdout.read()
        err = cmd.stderr.read()
        cmd.stdout.close()
        cmd.stderr.close()
        if cmd.returncode != 0:
            raise Exception("%s\n%s" % (out, err))
