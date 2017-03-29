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


class BaseMerger(object):
    """
    Base merger class

    Default policy: Overwrite same key with new value, create new keys
    """

    KEY_NAME = 'key'

    def get_key(self, setting):
        """
        Return setting key
        """
        if self.KEY_NAME in setting:
            return setting[self.KEY_NAME]

    def merge(self, *args):
        """
        Merge settings in the given order
        """
        index = {}
        for settings in args:
            for setting in settings:
                key = self.get_key(setting)
                index[key] = setting
        return index.values()


class GSettingsMerger(BaseMerger):
    """
    GSettings merger class

    Policy: Overwrite same key with new value, create new keys
    """
    pass


class LibreOfficeMerger(BaseMerger):
    """
    LibreOffice setting merger class

    Policy: Overwrite same key with new value, create new keys
    """
    pass


class NetworkManagerMerger(BaseMerger):
    """
    Network manager setting merger class

    Policy: Overwrite same key with new value, create new keys
    """
    KEY_NAME = 'uuid'


class GOAMerger(BaseMerger):
    """
    Policy: Overwrite same account with new one, create new accounts
    """

    def get_key(self, setting):
        """
        Return setting key
        """
        return None

    def merge(self, *args):
        """
        Merge settings in the given order
        """
        accounts = {}
        for settings in args:
            for account_id in settings:
                accounts[account_id] = settings[account_id]
        return accounts
