# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import PathScripts.PathLog as PathLog
import PathScripts.PathMillFace as PathMillFace
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathPocketBaseGui as PathPocketBaseGui
import PathScripts.PathPocketShape as PathPocketShape
import FreeCADGui

__title__ = "Path Face Mill Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Face Mill operation page controller and command implementation."

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


class TaskPanelOpPage(PathPocketBaseGui.TaskPanelOpPage):
    """Page controller class for the face milling operation."""

    def getForm(self):
        PathLog.track()
        """getForm() ... return UI"""

        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpPocketFullEdit.ui")
        comboToPropertyMap = [
            ("cutMode", "CutMode"),
            ("offsetPattern", "OffsetPattern"),
            ("boundaryShape", "BoundaryShape"),
        ]

        enumTups = PathMillFace.ObjectFace.propertyEnumerations(dataType="raw")
        enumTups.update(
            PathPocketShape.ObjectPocket.pocketPropertyEnumerations(dataType="raw")
        )

        self.populateCombobox(form, enumTups, comboToPropertyMap)
        return form

    def populateCombobox(self, form, enumTups, comboBoxesPropertyMap):
        """fillComboboxes(form, comboBoxesPropertyMap) ... populate comboboxes with translated enumerations
        ** comboBoxesPropertyMap will be unnecessary if UI files use strict combobox naming protocol.
        Args:
            form = UI form
            enumTups = list of (translated_text, data_string) tuples
            comboBoxesPropertyMap = list of (translated_text, data_string) tuples
        """
        # Load appropriate enumerations in each combobox
        for cb, prop in comboBoxesPropertyMap:
            box = getattr(form, cb)  # Get the combobox
            box.clear()  # clear the combobox
            for text, data in enumTups[prop]:  #  load enumerations
                box.addItem(text, data)

    def pocketFeatures(self):
        """pocketFeatures() ... return FeatureFacing (see PathPocketBaseGui)"""
        return PathPocketBaseGui.FeatureFacing


Command = PathOpGui.SetupOperation(
    "MillFace",
    PathMillFace.Create,
    TaskPanelOpPage,
    "Path_Face",
    QT_TRANSLATE_NOOP("Path_MillFace", "Face"),
    QT_TRANSLATE_NOOP(
        "Path_MillFace", "Create a Facing Operation from a model or face"
    ),
    PathMillFace.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathMillFaceGui... done\n")
