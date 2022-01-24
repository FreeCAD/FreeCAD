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

import PathScripts.PathOpGui as PathOpGui
from PySide import QtCore
import PathScripts.PathAdaptive as PathAdaptive
import PathScripts.PathFeatureExtensionsGui as PathFeatureExtensionsGui
import FreeCADGui


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    def getForm(self):
        """getForm() ... return UI"""

        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpAdaptiveEdit.ui")
        comboToPropertyMap = [("Side", "Side"), ("OperationType", "OperationType")]

        enumTups = PathAdaptive.PathAdaptive.propertyEnumerations(dataType="raw")

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

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        # signals.append(self.form.button.clicked)
        signals.append(self.form.Side.currentIndexChanged)
        signals.append(self.form.OperationType.currentIndexChanged)
        signals.append(self.form.ToolController.currentIndexChanged)
        signals.append(self.form.StepOver.valueChanged)
        signals.append(self.form.Tolerance.valueChanged)
        signals.append(self.form.HelixAngle.valueChanged)
        signals.append(self.form.HelixConeAngle.valueChanged)
        signals.append(self.form.HelixDiameterLimit.valueChanged)
        signals.append(self.form.LiftDistance.valueChanged)
        signals.append(self.form.KeepToolDownRatio.valueChanged)
        signals.append(self.form.StockToLeave.valueChanged)
        signals.append(self.form.coolantController.currentIndexChanged)

        # signals.append(self.form.ProcessHoles.stateChanged)
        signals.append(self.form.ForceInsideOut.stateChanged)
        signals.append(self.form.FinishingProfile.stateChanged)
        signals.append(self.form.useOutline.stateChanged)
        signals.append(self.form.StopButton.toggled)
        return signals

    def setFields(self, obj):
        self.selectInComboBox(obj.Side, self.form.Side)
        self.selectInComboBox(obj.OperationType, self.form.OperationType)
        self.form.StepOver.setValue(obj.StepOver)
        self.form.Tolerance.setValue(int(obj.Tolerance * 100))
        self.form.HelixAngle.setValue(obj.HelixAngle)
        self.form.HelixConeAngle.setValue(obj.HelixConeAngle)
        self.form.HelixDiameterLimit.setValue(obj.HelixDiameterLimit)
        self.form.LiftDistance.setValue(obj.LiftDistance)
        if hasattr(obj, "KeepToolDownRatio"):
            self.form.KeepToolDownRatio.setValue(obj.KeepToolDownRatio)

        if hasattr(obj, "StockToLeave"):
            self.form.StockToLeave.setValue(obj.StockToLeave)

        # self.form.ProcessHoles.setChecked(obj.ProcessHoles)
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

        obj.StepOver = self.form.StepOver.value()
        obj.Tolerance = 1.0 * self.form.Tolerance.value() / 100.0
        obj.HelixAngle = self.form.HelixAngle.value()
        obj.HelixConeAngle = self.form.HelixConeAngle.value()
        obj.HelixDiameterLimit = self.form.HelixDiameterLimit.value()
        obj.LiftDistance = self.form.LiftDistance.value()

        if hasattr(obj, "KeepToolDownRatio"):
            obj.KeepToolDownRatio = self.form.KeepToolDownRatio.value()

        if hasattr(obj, "StockToLeave"):
            obj.StockToLeave = self.form.StockToLeave.value()

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
            )  # pylint: disable=attribute-defined-outside-init
        return self.extensionsPanel


Command = PathOpGui.SetupOperation(
    "Adaptive",
    PathAdaptive.Create,
    TaskPanelOpPage,
    "Path_Adaptive",
    QtCore.QT_TRANSLATE_NOOP("Path_Adaptive", "Adaptive"),
    QtCore.QT_TRANSLATE_NOOP("Path_Adaptive", "Adaptive clearing and profiling"),
    PathAdaptive.SetupProperties,
)
