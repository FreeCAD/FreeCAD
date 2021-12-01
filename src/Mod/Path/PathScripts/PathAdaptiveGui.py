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
from PySide import QtCore, QtGui
import PathScripts.PathAdaptive as PathAdaptive
import PathScripts.PathFeatureExtensionsGui as PathFeatureExtensionsGui


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    def initPage(self, obj):
        self.setTitle("Adaptive path operation")

    def getForm(self):
        form = QtGui.QWidget()
        layout = QtGui.QVBoxLayout()
        formLayout = QtGui.QFormLayout()

        # tool controller
        form.ToolController = QtGui.QComboBox()
        formLayout.addRow(QtGui.QLabel("Tool Controller"), form.ToolController)

        # Coolant controller
        form.coolantController = QtGui.QComboBox()
        formLayout.addRow(QtGui.QLabel("Coolant Mode"), form.coolantController)

        # cut region
        form.Side = QtGui.QComboBox()
        form.Side.addItem("Inside")
        form.Side.addItem("Outside")
        form.Side.setToolTip("Cut inside or outside of the selected shapes")
        formLayout.addRow(QtGui.QLabel("Cut Region"), form.Side)

        # operation type
        form.OperationType = QtGui.QComboBox()
        form.OperationType.addItem("Clearing")
        form.OperationType.addItem("Profiling")
        form.OperationType.setToolTip("Type of adaptive operation")
        formLayout.addRow(QtGui.QLabel("Operation Type"), form.OperationType)

        # step over
        form.StepOver = QtGui.QSpinBox()
        form.StepOver.setMinimum(15)
        form.StepOver.setMaximum(75)
        form.StepOver.setSingleStep(1)
        form.StepOver.setValue(25)
        form.StepOver.setToolTip("Optimal value for tool stepover")
        formLayout.addRow(QtGui.QLabel("Step Over Percent"), form.StepOver)

        # tolerance
        form.Tolerance = QtGui.QSlider(QtCore.Qt.Horizontal)
        form.Tolerance.setMinimum(5)
        form.Tolerance.setMaximum(15)
        form.Tolerance.setTickInterval(1)
        form.Tolerance.setValue(10)
        form.Tolerance.setTickPosition(QtGui.QSlider.TicksBelow)
        form.Tolerance.setToolTip("Influences calculation performance vs stability and accuracy.")
        formLayout.addRow(QtGui.QLabel("Accuracy vs Performance"), form.Tolerance)

        # helix angle
        form.HelixAngle = QtGui.QDoubleSpinBox()
        form.HelixAngle.setMinimum(1)
        form.HelixAngle.setMaximum(89)
        form.HelixAngle.setSingleStep(1)
        form.HelixAngle.setValue(5)
        form.HelixAngle.setToolTip("Angle of the helix ramp entry")
        formLayout.addRow(QtGui.QLabel("Helix Ramp Angle"), form.HelixAngle)

        # helix cone angle
        form.HelixConeAngle = QtGui.QDoubleSpinBox()
        form.HelixConeAngle.setMinimum(0)
        form.HelixConeAngle.setMaximum(6)
        form.HelixConeAngle.setSingleStep(1)
        form.HelixConeAngle.setValue(0)
        form.HelixConeAngle.setToolTip("Angle of the helix entry cone")
        formLayout.addRow(QtGui.QLabel("Helix Cone Angle"), form.HelixConeAngle)

        # helix diam. limit
        form.HelixDiameterLimit = QtGui.QDoubleSpinBox()
        form.HelixDiameterLimit.setMinimum(0.0)
        form.HelixDiameterLimit.setMaximum(90)
        form.HelixDiameterLimit.setSingleStep(0.1)
        form.HelixDiameterLimit.setValue(0)
        form.HelixDiameterLimit.setToolTip("If >0 it limits the helix ramp diameter\notherwise the 75 percent of tool diameter is used as helix diameter")
        formLayout.addRow(QtGui.QLabel("Helix Max Diameter"), form.HelixDiameterLimit)

        # lift distance
        form.LiftDistance = QtGui.QDoubleSpinBox()
        form.LiftDistance.setMinimum(0.0)
        form.LiftDistance.setMaximum(1000)
        form.LiftDistance.setSingleStep(0.1)
        form.LiftDistance.setValue(1.0)
        form.LiftDistance.setToolTip("How much to lift the tool up during the rapid linking moves over cleared regions.\nIf linking path is not clear tool is raised to clearence height.")
        formLayout.addRow(QtGui.QLabel("Lift Distance"), form.LiftDistance)

        # KeepToolDownRatio
        form.KeepToolDownRatio = QtGui.QDoubleSpinBox()
        form.KeepToolDownRatio.setMinimum(1.0)
        form.KeepToolDownRatio.setMaximum(10)
        form.KeepToolDownRatio.setSingleStep(1)
        form.KeepToolDownRatio.setValue(3.0)
        form.KeepToolDownRatio.setToolTip("Max length of keep-tool-down linking path compared to direct distance between points.\nIf exceeded link will be done by raising the tool to clearence height.")
        formLayout.addRow(QtGui.QLabel("Keep Tool Down Ratio"), form.KeepToolDownRatio)

        # stock to leave
        form.StockToLeave = QtGui.QDoubleSpinBox()
        form.StockToLeave.setMinimum(0.0)
        form.StockToLeave.setMaximum(1000)
        form.StockToLeave.setSingleStep(0.1)
        form.StockToLeave.setValue(0)
        form.StockToLeave.setToolTip("How much material to leave (i.e. for finishing operation)")
        formLayout.addRow(QtGui.QLabel("Stock to Leave"), form.StockToLeave)

        # Force inside out
        form.ForceInsideOut = QtGui.QCheckBox()
        form.ForceInsideOut.setChecked(True)
        formLayout.addRow(QtGui.QLabel("Force Clearing Inside-Out"), form.ForceInsideOut)

        # Finishing profile
        form.FinishingProfile = QtGui.QCheckBox()
        form.FinishingProfile.setChecked(True)
        formLayout.addRow(QtGui.QLabel("Finishing Profile"), form.FinishingProfile)

        # Use outline checkbox
        form.useOutline = QtGui.QCheckBox()
        form.useOutline.setChecked(False)
        formLayout.addRow(QtGui.QLabel("Use outline"), form.useOutline)

        layout.addLayout(formLayout)

        # stop button
        form.StopButton = QtGui.QPushButton("Stop")
        form.StopButton.setCheckable(True)
        layout.addWidget(form.StopButton)

        form.setLayout(layout)
        return form

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals for updating obj'''
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
        if hasattr(obj, 'KeepToolDownRatio'):
            self.form.KeepToolDownRatio.setValue(obj.KeepToolDownRatio)

        if hasattr(obj, 'StockToLeave'):
            self.form.StockToLeave.setValue(obj.StockToLeave)

        # self.form.ProcessHoles.setChecked(obj.ProcessHoles)
        self.form.ForceInsideOut.setChecked(obj.ForceInsideOut)
        self.form.FinishingProfile.setChecked(obj.FinishingProfile)
        self.form.useOutline.setChecked(obj.UseOutline)
        self.setupToolController(obj, self.form.ToolController)
        self.setupCoolant(obj, self.form.coolantController)
        self.form.StopButton.setChecked(obj.Stopped)
        obj.setEditorMode('AdaptiveInputState', 2)  # hide this property
        obj.setEditorMode('AdaptiveOutputState', 2)  # hide this property
        obj.setEditorMode('StopProcessing', 2)  # hide this property
        obj.setEditorMode('Stopped', 2)  # hide this property

    def getFields(self, obj):
        if obj.Side != str(self.form.Side.currentText()):
            obj.Side = str(self.form.Side.currentText())

        if obj.OperationType != str(self.form.OperationType.currentText()):
            obj.OperationType = str(self.form.OperationType.currentText())

        obj.StepOver = self.form.StepOver.value()
        obj.Tolerance = 1.0 * self.form.Tolerance.value() / 100.0
        obj.HelixAngle = self.form.HelixAngle.value()
        obj.HelixConeAngle = self.form.HelixConeAngle.value()
        obj.HelixDiameterLimit = self.form.HelixDiameterLimit.value()
        obj.LiftDistance = self.form.LiftDistance.value()

        if hasattr(obj, 'KeepToolDownRatio'):
            obj.KeepToolDownRatio = self.form.KeepToolDownRatio.value()

        if hasattr(obj, 'StockToLeave'):
            obj.StockToLeave = self.form.StockToLeave.value()

        obj.ForceInsideOut = self.form.ForceInsideOut.isChecked()
        obj.FinishingProfile = self.form.FinishingProfile.isChecked()
        obj.UseOutline = self.form.useOutline.isChecked()
        obj.Stopped = self.form.StopButton.isChecked()
        if(obj.Stopped):
            self.form.StopButton.setChecked(False)  # reset the button
            obj.StopProcessing = True

        self.updateToolController(obj, self.form.ToolController)
        self.updateCoolant(obj, self.form.coolantController)
        obj.setEditorMode('AdaptiveInputState', 2)  # hide this property
        obj.setEditorMode('AdaptiveOutputState', 2)  # hide this property
        obj.setEditorMode('StopProcessing', 2)  # hide this property
        obj.setEditorMode('Stopped', 2)  # hide this property

    def taskPanelBaseLocationPage(self, obj, features):
        if not hasattr(self, 'extensionsPanel'):
            self.extensionsPanel = PathFeatureExtensionsGui.TaskPanelExtensionPage(obj, features) # pylint: disable=attribute-defined-outside-init
        return self.extensionsPanel


Command = PathOpGui.SetupOperation('Adaptive',
        PathAdaptive.Create,
        TaskPanelOpPage,
        'Path_Adaptive',
        QtCore.QT_TRANSLATE_NOOP("Path_Adaptive", "Adaptive"),
        QtCore.QT_TRANSLATE_NOOP("Path_Adaptive", "Adaptive clearing and profiling"),
        PathAdaptive.SetupProperties)
