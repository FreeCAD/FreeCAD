# -*- coding: utf8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""The BIM Column command"""

import FreeCAD
import FreeCADGui
import ArchStructure

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Column(ArchStructure._CommandStructure):

    def __init__(self):
        super().__init__()
        self.beammode = False

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def GetResources(self):
        return {
            "Pixmap": "BIM_Column",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Column", "Column"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Column", "Creates a column at a specified location"
            ),
            "Accel": "C,O",
        }


FreeCADGui.addCommand("BIM_Column", BIM_Column())
