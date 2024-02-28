# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2018 Kresimir Tusek <kresimir.tusek@gmail.com>          *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************

import FreeCADGui
import FreeCAD
import Path.Op.Adaptive as PathAdaptive
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Gui.FeatureExtension as PathFeatureExtensionsGui
from PySide import QtCore

import Path.Base.Gui.Util as PathGuiUtil


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    def getForm(self):
        """getForm() ... return UI"""

        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpAdaptiveEdit.ui")
        comboToPropertyMap = [("Side", "Side"), ("OperationType", "OperationType")]

        enumTups = PathAdaptive.PathAdaptive.propertyEnumerations(dataType="raw")

        self.populateCombobox(form, enumTups, comboToPropertyMap)
        return form

    def initPage(self, obj):
        self.form.LiftDistance.setProperty(
            "unit", obj.LiftDistance.getUserPreferred()[2]
        )
        self.form.HelixDiameterLimit.setProperty(
            "unit", obj.HelixDiameterLimit.getUserPreferred()[2]
        )
        self.form.KeepToolDownRatio.setProperty(
            "unit", obj.KeepToolDownRatio.getUserPreferred()[2]
        )
        self.form.StockToLeave.setProperty(
            "unit", obj.StockToLeave.getUserPreferred()[2]
        )

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        signals.append(self.form.Side.currentIndexChanged)
        signals.append(self.form.OperationType.currentIndexChanged)
        signals.append(self.form.ToolController.currentIndexChanged)
        signals.append(self.form.stepOverPercent.valueChanged)
        signals.append(self.form.Tolerance.valueChanged)
        signals.append(self.form.HelixAngle.valueChanged)
        signals.append(self.form.HelixConeAngle.valueChanged)
        signals.append(self.form.HelixDiameterLimit.valueChanged)
        signals.append(self.form.LiftDistance.valueChanged)
        signals.append(self.form.KeepToolDownRatio.valueChanged)
        signals.append(self.form.StockToLeave.valueChanged)
        signals.append(self.form.coolantController.currentIndexChanged)
        signals.append(self.form.ForceInsideOut.stateChanged)
        signals.append(self.form.FinishingProfile.stateChanged)
        signals.append(self.form.useOutline.stateChanged)
        signals.append(self.form.StopButton.toggled)
        return signals

    def setFields(self, obj):
        self.selectInComboBox(obj.Side, self.form.Side)
        self.selectInComboBox(obj.OperationType, self.form.OperationType)
        self.form.stepOverPercent.setValue(obj.StepOver)
        self.form.Tolerance.setValue(int(obj.Tolerance * 100))

        self.form.HelixAngle.setText(
            FreeCAD.Units.Quantity(obj.HelixAngle, FreeCAD.Units.Angle).UserString
        )

        self.form.HelixConeAngle.setText(
            FreeCAD.Units.Quantity(obj.HelixConeAngle, FreeCAD.Units.Angle).UserString
        )

        self.form.HelixDiameterLimit.setProperty(
            "rawValue", obj.HelixDiameterLimit.Value
        )

        self.form.LiftDistance.setProperty("rawValue", obj.LiftDistance.Value)

        if hasattr(obj, "KeepToolDownRatio"):
            self.form.KeepToolDownRatio.setProperty(
                "rawValue", obj.KeepToolDownRatio.Value
            )
            # self.form.KeepToolDownRatio.setValue(obj.KeepToolDownRatio)

        if hasattr(obj, "StockToLeave"):
            self.form.StockToLeave.setProperty("rawValue", obj.StockToLeave.Value)

        self.form.ForceInsideOut.setChecked(obj.ForceInsideOut)
        self.form.FinishingProfile.setChecked(obj.FinishingProfile)
        self.form.useOutline.setChecked(obj.UseOutline)
        self.setupToolController(obj, self.form.ToolController)
        self.setupCoolant(obj, self.form.coolantController)
        self.form.StopButton.setChecked(obj.Stopped)
        obj.setEditorMode("AdaptiveInputState", 2)  # hide this property
        obj.setEditorMode("AdaptiveOutputState", 2)  # hide this property
        obj.setEditorMode("StopProcessing", 2)  # hide this property
        obj.setEditorMode("Stopped", 2)  # hide this property

    def getFields(self, obj):
        if obj.Side != str(self.form.Side.currentData()):
            obj.Side = str(self.form.Side.currentData())

        if obj.OperationType != str(self.form.OperationType.currentData()):
            obj.OperationType = str(self.form.OperationType.currentData())

        if obj.StepOver != self.form.stepOverPercent.value():
            obj.StepOver = self.form.stepOverPercent.value()

        obj.Tolerance = 1.0 * self.form.Tolerance.value() / 100.0
        PathGuiUtil.updateInputField(obj, "HelixAngle", self.form.HelixAngle)
        PathGuiUtil.updateInputField(obj, "HelixConeAngle", self.form.HelixConeAngle)
        PathGuiUtil.updateInputField(
            obj, "HelixDiameterLimit", self.form.HelixDiameterLimit
        )
        PathGuiUtil.updateInputField(obj, "LiftDistance", self.form.LiftDistance)

        if hasattr(obj, "KeepToolDownRatio"):
            PathGuiUtil.updateInputField(
                obj, "KeepToolDownRatio", self.form.KeepToolDownRatio
            )

        if hasattr(obj, "StockToLeave"):
            PathGuiUtil.updateInputField(obj, "StockToLeave", self.form.StockToLeave)

        obj.ForceInsideOut = self.form.ForceInsideOut.isChecked()
        obj.FinishingProfile = self.form.FinishingProfile.isChecked()
        obj.UseOutline = self.form.useOutline.isChecked()
        obj.Stopped = self.form.StopButton.isChecked()
        if obj.Stopped:
            self.form.StopButton.setChecked(False)  # reset the button
            obj.StopProcessing = True

        self.updateToolController(obj, self.form.ToolController)
        self.updateCoolant(obj, self.form.coolantController)
        obj.setEditorMode("AdaptiveInputState", 2)  # hide this property
        obj.setEditorMode("AdaptiveOutputState", 2)  # hide this property
        obj.setEditorMode("StopProcessing", 2)  # hide this property
        obj.setEditorMode("Stopped", 2)  # hide this property

    def taskPanelBaseLocationPage(self, obj, features):
        if not hasattr(self, "extensionsPanel"):
            self.extensionsPanel = PathFeatureExtensionsGui.TaskPanelExtensionPage(
                obj, features
            )
        return self.extensionsPanel


Command = PathOpGui.SetupOperation(
    "Adaptive",
    PathAdaptive.Create,
    TaskPanelOpPage,
    "CAM_Adaptive",
    QtCore.QT_TRANSLATE_NOOP("CAM_Adaptive", "Adaptive"),
    QtCore.QT_TRANSLATE_NOOP("CAM_Adaptive", "Adaptive clearing and profiling"),
    PathAdaptive.SetupProperties,
)
