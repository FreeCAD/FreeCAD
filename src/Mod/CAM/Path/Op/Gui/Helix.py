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

import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.Util as PathGuiUtil
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Gui.CircularHoleBase as PathCircularHoleBaseGui
import Path.Op.Helix as PathHelix
import PathGui
from PySide.QtCore import QT_TRANSLATE_NOOP

translate = FreeCAD.Qt.translate

from PySide import QtCore

__doc__ = "Helix operation page controller and command implementation."

LOGLEVEL = False

if LOGLEVEL:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.NOTICE, Path.Log.thisModule())


class TaskPanelOpPage(PathCircularHoleBaseGui.TaskPanelOpPage):
    """Page controller class for Helix operations."""

    def getForm(self):
        """getForm() ... return UI"""

        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpHelixEdit.ui")
        comboToPropertyMap = [("startSide", "StartSide"), ("direction", "Direction")]

        enumTups = PathHelix.ObjectHelix.helixOpPropertyEnumerations(dataType="raw")

        self.populateCombobox(form, enumTups, comboToPropertyMap)
        return form

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        Path.Log.track()
        if obj.Direction != str(self.form.direction.currentData()):
            obj.Direction = str(self.form.direction.currentData())
        if obj.StartSide != str(self.form.startSide.currentData()):
            obj.StartSide = str(self.form.startSide.currentData())
        if obj.StepOver != self.form.stepOverPercent.value():
            obj.StepOver = self.form.stepOverPercent.value()
        PathGuiUtil.updateInputField(obj, "OffsetExtra", self.form.extraOffset)

        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        Path.Log.track()

        self.form.stepOverPercent.setValue(obj.StepOver)
        self.selectInComboBox(obj.Direction, self.form.direction)
        self.selectInComboBox(obj.StartSide, self.form.startSide)

        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        self.form.extraOffset.setText(
            FreeCAD.Units.Quantity(
                obj.OffsetExtra.Value, FreeCAD.Units.Length
            ).UserString
        )

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []

        signals.append(self.form.stepOverPercent.editingFinished)
        signals.append(self.form.extraOffset.editingFinished)
        signals.append(self.form.direction.currentIndexChanged)
        signals.append(self.form.startSide.currentIndexChanged)
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)

        return signals


Command = PathOpGui.SetupOperation(
    "Helix",
    PathHelix.Create,
    TaskPanelOpPage,
    "CAM_Helix",
    QT_TRANSLATE_NOOP("CAM_Helix", "Helix"),
    QT_TRANSLATE_NOOP(
        "CAM_Helix", "Creates a Helical toolpath from the features of a base object"
    ),
    PathHelix.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathHelixGui... done\n")
