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
import Path.Base.Gui.Util as PathGuiUtil
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
        PathGuiUtil.populateCombobox(form, enumTups, comboToPropertyMap)
        return form

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        if obj.CutMode != str(self.form.cutMode.currentData()):
            obj.CutMode = str(self.form.cutMode.currentData())

        if obj.ClearingPattern != str(self.form.clearingPattern.currentData()):
            obj.ClearingPattern = str(self.form.clearingPattern.currentData())

        # Reverse checkbox
        if hasattr(obj, 'Reverse') and obj.Reverse != self.form.reverse.isChecked():
            obj.Reverse = self.form.reverse.isChecked()

        if obj.Angle != self.form.angle.value():
            obj.Angle = self.form.angle.value()

        if obj.StepOver != self.form.stepOver.value():
            obj.StepOver = self.form.stepOver.value()

        PathGuiUtil.updateInputField(obj, "MaterialAllowance", self.form.materialAllowance)
        
        # Only update PassExtension if the property exists
        if hasattr(obj, 'PassExtension'):
            PathGuiUtil.updateInputField(obj, "PassExtension", self.form.passExtension)

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)
        
        self.selectInComboBox(obj.ClearingPattern, self.form.clearingPattern)
        
        # Handle new properties that may not exist in older operations
        if hasattr(obj, 'Reverse'):
            self.form.reverse.setChecked(bool(obj.Reverse))
        else:
            self.form.reverse.setChecked(False)
        
        self.form.angle.setValue(obj.Angle)
        self.form.stepOver.setValue(obj.StepOver)
        self.form.materialAllowance.setText(
            FreeCAD.Units.Quantity(obj.MaterialAllowance.Value, FreeCAD.Units.Length).UserString
        )
        
        if hasattr(obj, 'PassExtension'):
            self.form.passExtension.setText(
                FreeCAD.Units.Quantity(obj.PassExtension.Value, FreeCAD.Units.Length).UserString
            )
        else:
            self.form.passExtension.setText("3.0 mm")  # Default value

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)
        signals.append(self.form.cutMode.currentIndexChanged)
        signals.append(self.form.clearingPattern.currentIndexChanged)
        # Qt 6 compatibility for checkbox state change
        if hasattr(self.form.reverse, "checkStateChanged"):  # Qt >= 6.7.0
            signals.append(self.form.reverse.checkStateChanged)
        else:
            signals.append(self.form.reverse.stateChanged)
        signals.append(self.form.angle.editingFinished)
        signals.append(self.form.stepOver.editingFinished)
        signals.append(self.form.materialAllowance.editingFinished)
        signals.append(self.form.passExtension.editingFinished)
        
        return signals




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
