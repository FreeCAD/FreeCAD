# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 sliptonic sliptonic@freecad.org                    *
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

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import Path.Op.Gui.Base as PathOpGui
import Path.Op.MillFacing as PathMillFacing
import FreeCADGui

__title__ = "CAM Mill Facing Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Mill Facing operation page controller and command implementation."

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for the mill facing operation."""

    def getForm(self):
        Path.Log.track()
        """getForm() ... return UI"""

        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpMillFacingEdit.ui")
        comboToPropertyMap = [
            ("cutMode", "CutMode"),
            ("clearingPattern", "ClearingPattern"),
        ]

        enumTups = PathMillFacing.ObjectMillFacing.propertyEnumerations(dataType="raw")

        self.populateCombobox(form, enumTups, comboToPropertyMap)
        return form




Command = PathOpGui.SetupOperation(
    "MillFacing",
    PathMillFacing.Create,
    TaskPanelOpPage,
    "CAM_Face",
    QT_TRANSLATE_NOOP("CAM_MillFacing", "Mill Facing"),
    QT_TRANSLATE_NOOP("CAM_MillFacing", "Create a Mill Facing Operation to machine the top surface of stock"),
    PathMillFacing.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathMillFacingGui... done\n")
