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

"""The BIM DimensionAligned command"""


import FreeCAD
import FreeCADGui
from draftguitools import gui_dimensions

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_DimensionAligned(gui_dimensions.Dimension):

    def __init__(self):
        super().__init__()

    def GetResources(self):
        return {
            "Pixmap": "BIM_DimensionAligned",
            "MenuText": QT_TRANSLATE_NOOP("BIM_DimensionAligned", "Aligned dimension"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_DimensionAligned", "Create an aligned dimension"
            ),
            "Accel": "D, I",
        }


class BIM_DimensionHorizontal(gui_dimensions.Dimension):

    def __init__(self):
        super().__init__()

    def GetResources(self):
        return {
            "Pixmap": "BIM_DimensionHorizontal.svg",
            "MenuText": QT_TRANSLATE_NOOP(
                "BIM_DimensionHorizontal", "Horizontal dimension"
            ),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_DimensionHorizontal", "Create an horizontal dimension"
            ),
            "Accel": "D, H",
        }

    def Activated(self):
        self.dir = FreeCAD.DraftWorkingPlane.u
        super().Activated()


class BIM_DimensionVertical(gui_dimensions.Dimension):

    def __init__(self):
        super().__init__()

    def GetResources(self):
        return {
            "Pixmap": "BIM_DimensionVertical",
            "MenuText": QT_TRANSLATE_NOOP(
                "BIM_DimensionVertical", "Vertical dimension"
            ),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_DimensionVertical", "Create a vertical dimension"
            ),
            "Accel": "D, V",
        }

    def Activated(self):
        self.dir = FreeCAD.DraftWorkingPlane.v
        super().Activated()


FreeCADGui.addCommand("BIM_DimensionVertical", BIM_DimensionVertical())
FreeCADGui.addCommand("BIM_DimensionHorizontal", BIM_DimensionHorizontal())
FreeCADGui.addCommand("BIM_DimensionAligned", BIM_DimensionAligned())
