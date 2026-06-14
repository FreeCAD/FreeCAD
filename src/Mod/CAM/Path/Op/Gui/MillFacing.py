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

    def initPage(self, obj):
        """initPage(obj) ... Initialize page with QuantitySpinBox wrappers for expression support"""
        self.axialStockToLeaveSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.axialStockToLeave, obj, "AxialStockToLeave"
        )
        if hasattr(obj, "PassExtension"):
            self.passExtensionSpinBox = PathGuiUtil.QuantitySpinBox(
                self.form.passExtension, obj, "PassExtension"
            )
        if hasattr(obj, "StockExtension"):
            self.stockExtensionSpinBox = PathGuiUtil.QuantitySpinBox(
                self.form.stockExtension, obj, "StockExtension"
            )

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
        if hasattr(obj, "Reverse") and obj.Reverse != self.form.reverse.isChecked():
            obj.Reverse = self.form.reverse.isChecked()

        # Angle is a PropertyAngle (quantity). Compare/update by value.
        if getattr(obj.Angle, "Value", obj.Angle) != self.form.angle.value():
            obj.Angle = self.form.angle.value()

        # StepOver is an App::PropertyPercent; assign an int percentage value
        step_over_val = int(self.form.stepOver.value())
        if obj.StepOver != step_over_val:
            obj.StepOver = step_over_val

        # AxialStockToLeave and PassExtension are handled by QuantitySpinBox wrappers
        self.axialStockToLeaveSpinBox.updateProperty()
        if hasattr(obj, "PassExtension"):
            self.passExtensionSpinBox.updateProperty()
        if hasattr(obj, "StockExtension"):
            self.stockExtensionSpinBox.updateProperty()

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        # Reflect current CutMode and ClearingPattern in UI
        self.selectInComboBox(obj.CutMode, self.form.cutMode)
        self.selectInComboBox(obj.ClearingPattern, self.form.clearingPattern)

        # Handle new properties that may not exist in older operations
        if hasattr(obj, "Reverse"):
            self.form.reverse.setChecked(bool(obj.Reverse))
        else:
            self.form.reverse.setChecked(False)

        # Angle is a quantity; set spinbox with numeric degrees
        self.form.angle.setValue(getattr(obj.Angle, "Value", obj.Angle))
        self.form.stepOver.setValue(obj.StepOver)

        # Update QuantitySpinBox displays
        self.updateQuantitySpinBoxes()

    def updateQuantitySpinBoxes(self, index=None):
        """updateQuantitySpinBoxes() ... refresh QuantitySpinBox displays from properties"""
        self.axialStockToLeaveSpinBox.updateWidget()
        if hasattr(self, "passExtensionSpinBox"):
            self.passExtensionSpinBox.updateWidget()
        if hasattr(self, "stockExtensionSpinBox"):
            self.stockExtensionSpinBox.updateWidget()

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)
        signals.append(self.form.cutMode.currentIndexChanged)
        signals.append(self.form.clearingPattern.currentIndexChanged)
        signals.append(self.form.axialStockToLeave.editingFinished)
        if hasattr(self.form, "passExtension"):
            signals.append(self.form.passExtension.editingFinished)
        if hasattr(self.form, "stockExtension"):
            signals.append(self.form.stockExtension.editingFinished)
        # Qt 6 compatibility for checkbox state change
        if hasattr(self.form.reverse, "checkStateChanged"):  # Qt >= 6.7.0
            signals.append(self.form.reverse.checkStateChanged)
        else:
            signals.append(self.form.reverse.stateChanged)
        signals.append(self.form.angle.editingFinished)
        signals.append(self.form.stepOver.editingFinished)

        return signals


Command = PathOpGui.SetupOperation(
    "MillFacing",
    PathMillFacing.Create,
    TaskPanelOpPage,
    "CAM_Face",
    QT_TRANSLATE_NOOP("CAM_MillFacing", "Mill Facing"),
    QT_TRANSLATE_NOOP(
        "CAM_MillFacing", "Create a Mill Facing Operation to machine the top surface of stock"
    ),
    PathMillFacing.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathMillFacingGui... done\n")
