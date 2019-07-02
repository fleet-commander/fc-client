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

# Python imports
import logging


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
        return list(index.values())


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


class ChromiumMerger(BaseMerger):
    """
    Chromium setting merger class

    Policy: Overwrite same key with new value, create new keys
    Except: ManagedBookmarks key: Merge contents
    """
    def merge(self, *args):
        """
        Merge settings in the given order
        """
        index = {}
        bookmarks = []
        for settings in args:
            for setting in settings:
                key = self.get_key(setting)
                if key == 'ManagedBookmarks':
                    bookmarks = self.merge_bookmarks(bookmarks, setting['value'])
                    setting = {self.KEY_NAME: key, 'value': bookmarks}
                index[key] = setting
        return list(index.values())

    def merge_bookmarks(self, a, b):
        for elem_b in b:
            logging.debug('Processing %s' % elem_b)
            if 'children' in elem_b:
                merged = False
                for elem_a in a:
                    if elem_a['name'] == elem_b['name'] and 'children' in elem_a:
                        logging.debug(
                            'Processing children of %s' % elem_b['name'])
                        elem_a['children'] = self.merge_bookmarks(
                            elem_a['children'], elem_b['children'])
                        merged = True
                        break
                if not merged:
                    a.append(elem_b)
            else:
                if elem_b not in a:
                    a.append(elem_b)
        logging.debug('Returning %s' % a)
        return a


class FirefoxMerger(BaseMerger):
    """
    Firefox setting merger class

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
