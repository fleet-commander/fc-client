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


from fleetcommanderclient.configadapters.dconf import DconfConfigAdapter
from fleetcommanderclient.configadapters.goa import GOAConfigAdapter
from fleetcommanderclient.configadapters.networkmanager import (
    NetworkManagerConfigAdapter,
)
from fleetcommanderclient.configadapters.chromium import (
    ChromiumConfigAdapter,
    ChromeConfigAdapter,
)
from fleetcommanderclient.configadapters.firefox import FirefoxConfigAdapter
from fleetcommanderclient.configadapters.firefoxbookmarks import (
    FirefoxBookmarksConfigAdapter,
)
