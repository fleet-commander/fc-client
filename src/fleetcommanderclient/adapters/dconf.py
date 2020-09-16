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
import subprocess
import logging
import shutil

from gi.repository import GLib

from fleetcommanderclient.adapters.base import BaseAdapter


class DconfAdapter(BaseAdapter):
    """
    DConf configuration adapter class
    """

    # Namespace this config adapter handles
    NAMESPACE = "org.gnome.gsettings"

    PROFILE_FILE = "fleet-commander-dconf.conf"
    DB_FILE = "fleet-commander-dconf.db"

    def __init__(self, dconf_profile_path, dconf_db_path):
        self.dconf_profile_path = dconf_profile_path
        self.dconf_db_path = dconf_db_path

    def _get_paths_for_uid(self, uid):
        struid = str(uid)
        profile_path = os.path.join(self.dconf_profile_path, struid)
        keyfile_dir = os.path.join(
            self.dconf_db_path, "{}-{}.d".format(self.DB_FILE, struid)
        )
        db_path = os.path.join(self.dconf_db_path, "{}-{}".format(self.DB_FILE, struid))
        return (profile_path, keyfile_dir, db_path)

    def _compile_dconf_db(self, keyfiles_dir, db_file):
        """
        Compiles dconf database
        """
        # Execute dbus service
        cmd = subprocess.Popen(
            [
                "dconf",
                "compile",
                db_file,
                keyfiles_dir,
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
            raise Exception("{}\n{}".format(out, err))

    def _remove_path(self, path, throw=False):
        logging.debug("Removing path: %s", path)
        try:
            if os.path.exists(path):
                if os.path.isdir(path):
                    shutil.rmtree(path)
                else:
                    os.remove(path)
        except Exception as e:
            if throw:
                logging.error("Error removing path %s: %s", path, e)
                raise e
            logging.warning("Error removing path %s: %s", path, e)

    def process_config_data(self, config_data, cache_path):
        """
        Process configuration data and save cache files to be deployed.
        This method needs to be defined by each configuration adapter.
        """

        # Create keyfile path
        keyfiles_dir = os.path.join(cache_path, "keyfiles")
        logging.debug("Creating keyfiles directory %s", keyfiles_dir)
        try:
            os.makedirs(keyfiles_dir)
        except Exception as e:
            logging.error("Error creating keyfiles path %s: %s", keyfiles_dir, e)
            return

        # Prepare data for saving it in keyfile
        logging.debug("Preparing dconf data for saving to keyfile")
        keyfile = GLib.KeyFile.new()
        for item in config_data:
            if "key" in item and "value" in item:
                keysplit = item["key"][1:].split("/")
                keypath = "/".join(keysplit[:-1])
                keyname = keysplit[-1]
                keyfile.set_string(keypath, keyname, item["value"])

        # Save keyfile
        keyfile_path = os.path.join(keyfiles_dir, self.PROFILE_FILE)
        logging.debug("Saving dconf keyfile to %s", keyfile_path)
        try:
            keyfile.save_to_file(keyfile_path)
        except Exception as e:
            logging.error('Error saving dconf keyfile at "%s": %s', keyfile_path, e)
            return

        # Compile dconf database
        db_path = os.path.join(cache_path, self.DB_FILE)
        try:
            self._compile_dconf_db(keyfiles_dir, db_path)
        except Exception as e:
            logging.error("Error compiling dconf data to %s: %s", cache_path, e)
            return

    def deploy_files(self, cache_path, uid):
        """
        Copy cached policies file to policies directory
        This method will be called by privileged process
        """

        cached_db_file_path = os.path.join(cache_path, self.DB_FILE)

        if os.path.isfile(cached_db_file_path):
            logging.debug(
                "Deploying dconf settings from database file %s", cached_db_file_path
            )

            profile_path, keyfile_dir, db_path = self._get_paths_for_uid(uid)

            # Remove old paths
            for path in [profile_path, keyfile_dir, db_path]:
                self._remove_path(path)

            # Create runtime path
            logging.debug("Creating profile path for dconf %s", profile_path)
            try:
                os.makedirs(self.dconf_profile_path)
            except Exception:
                pass

            # Copy db file from cache to db path
            deploy_db_file_path = os.path.join(
                self.dconf_db_path, "{}-{}".format(self.DB_FILE, uid)
            )
            shutil.copyfile(cached_db_file_path, deploy_db_file_path)

            # Save runtime file
            try:
                profile_data = "user-db:user\n\nsystem-db:{}-{}".format(
                    self.DB_FILE, uid
                )
                with open(profile_path, "w") as fd:
                    fd.write(profile_data)
                    fd.close()
            except Exception as e:
                logging.error("Error saving dconf profile at %s: %s", profile_path, e)
                return

            logging.info("Processed dconf configuration for UID %s", uid)

            # # Change permissions and ownership for accounts file
            # os.chown(deploy_file_path, uid, -1)
            # os.chmod(deploy_file_path, stat.S_IREAD)

            # # Change permissions and ownership for GOA runtime directory
            # os.chown(runtime_path, uid, -1)
            # os.chmod(runtime_path, stat.S_IREAD | stat.S_IEXEC)

        else:
            logging.debug(
                "Dconf settings database file %s not present. Ignoring.",
                cached_db_file_path,
            )
