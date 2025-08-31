# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018-25 Paul Lee <paullee0@gmail.com>                   *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import ArchWindow
from PySide.QtCore import QT_TRANSLATE_NOOP

class ArchSketchObject:
    def __init__(self, obj):
        pass

class ArchSketch(ArchSketchObject):
    def __init__(self, obj):
        pass

    def setPropertiesLinkCommon(self, orgFp, linkFp=None, mode=None):
        if linkFp:
            fp = linkFp
        else:
            fp = orgFp
        prop = fp.PropertiesList
        if not isinstance(fp.getLinkedObject().Proxy, ArchWindow._Window):
            pass
        else:
            if "Hosts" not in prop:
                fp.addProperty("App::PropertyLinkList","Hosts","Window",
                               QT_TRANSLATE_NOOP("App::Property",
                               "The objects that host this window"), locked=True)
                               # Arch Window's code

#from ArchSketchObjectExt import ArchSketch  # Doesn't work
