# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import PathScripts.PathSurface as PathSurface
import PathScripts.PathGui as PathGui
import PathScripts.PathOpGui as PathOpGui

from PySide import QtCore

__title__ = "Path Surface Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Surface operation page controller and command implementation."


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    '''Page controller class for the Surface operation.'''

    def initPage(self, obj):
        self.setTitle("3D Surface")
        self.updateVisibility()

    def getForm(self):
        '''getForm() ... returns UI'''
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpSurfaceEdit.ui")

    def getFields(self, obj):
        '''getFields(obj) ... transfers values from UI to obj's proprties'''
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        PathGui.updateInputField(obj, 'DepthOffset', self.form.depthOffset)
        PathGui.updateInputField(obj, 'SampleInterval', self.form.sampleInterval)

        if obj.BoundBox != str(self.form.boundBoxSelect.currentText()):
            obj.BoundBox = str(self.form.boundBoxSelect.currentText())

        if obj.ScanType != str(self.form.scanType.currentText()):
            obj.ScanType = str(self.form.scanType.currentText())

        if obj.StepOver != self.form.stepOver.value():
            obj.StepOver = self.form.stepOver.value()

        if obj.LayerMode != str(self.form.layerMode.currentText()):
            obj.LayerMode = str(self.form.layerMode.currentText())

        obj.DropCutterExtraOffset.x = FreeCAD.Units.Quantity(self.form.boundBoxExtraOffsetX.text()).Value
        obj.DropCutterExtraOffset.y = FreeCAD.Units.Quantity(self.form.boundBoxExtraOffsetY.text()).Value

        if obj.DropCutterDir != str(self.form.dropCutterDirSelect.currentText()):
            obj.DropCutterDir = str(self.form.dropCutterDirSelect.currentText())

        PathGui.updateInputField(obj, 'DepthOffset', self.form.depthOffset)
        PathGui.updateInputField(obj, 'SampleInterval', self.form.sampleInterval)

        if obj.UseStartPoint != self.form.useStartPoint.isChecked():
            obj.UseStartPoint = self.form.useStartPoint.isChecked()

        if obj.OptimizeLinearPaths != self.form.optimizeEnabled.isChecked():
            obj.OptimizeLinearPaths = self.form.optimizeEnabled.isChecked()

        if obj.OptimizeStepOverTransitions != self.form.optimizeStepOverTransitions.isChecked():
            obj.OptimizeStepOverTransitions = self.form.optimizeStepOverTransitions.isChecked()

    def setFields(self, obj):
        '''setFields(obj) ... transfers obj's property values to UI'''
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)
        self.selectInComboBox(obj.BoundBox, self.form.boundBoxSelect)
        self.selectInComboBox(obj.ScanType, self.form.scanType)
        self.selectInComboBox(obj.LayerMode, self.form.layerMode)
        self.form.boundBoxExtraOffsetX.setText(str(obj.DropCutterExtraOffset.x))
        self.form.boundBoxExtraOffsetY.setText(str(obj.DropCutterExtraOffset.y))
        self.selectInComboBox(obj.DropCutterDir, self.form.dropCutterDirSelect)
        self.form.depthOffset.setText(FreeCAD.Units.Quantity(obj.DepthOffset.Value, FreeCAD.Units.Length).UserString)
        self.form.stepOver.setValue(obj.StepOver)
        self.form.sampleInterval.setText(str(obj.SampleInterval))

        if obj.UseStartPoint:
            self.form.useStartPoint.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.useStartPoint.setCheckState(QtCore.Qt.Unchecked)

        if obj.OptimizeLinearPaths:
            self.form.optimizeEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.optimizeEnabled.setCheckState(QtCore.Qt.Unchecked)

        if obj.OptimizeStepOverTransitions:
            self.form.optimizeStepOverTransitions.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.optimizeStepOverTransitions.setCheckState(QtCore.Qt.Unchecked)

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals for updating obj'''
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)
        signals.append(self.form.boundBoxSelect.currentIndexChanged)
        signals.append(self.form.scanType.currentIndexChanged)
        signals.append(self.form.layerMode.currentIndexChanged)
        signals.append(self.form.boundBoxExtraOffsetX.editingFinished)
        signals.append(self.form.boundBoxExtraOffsetY.editingFinished)
        signals.append(self.form.dropCutterDirSelect.currentIndexChanged)
        signals.append(self.form.depthOffset.editingFinished)
        signals.append(self.form.stepOver.editingFinished)
        signals.append(self.form.sampleInterval.editingFinished)
        signals.append(self.form.useStartPoint.stateChanged)
        signals.append(self.form.optimizeEnabled.stateChanged)
        signals.append(self.form.optimizeStepOverTransitions.stateChanged)

        return signals

    def updateVisibility(self):
        if self.form.scanType.currentText() == "Planar":
            self.form.boundBoxExtraOffsetX.setEnabled(False)
            self.form.boundBoxExtraOffsetY.setEnabled(False)
            self.form.dropCutterDirSelect.setEnabled(False)
        else:
            self.form.boundBoxExtraOffsetX.setEnabled(True)
            self.form.boundBoxExtraOffsetY.setEnabled(True)
            self.form.dropCutterDirSelect.setEnabled(True)

    def registerSignalHandlers(self, obj):
        self.form.scanType.currentIndexChanged.connect(self.updateVisibility)


Command = PathOpGui.SetupOperation('Surface',
        PathSurface.Create,
        TaskPanelOpPage,
        'Path-3DSurface',
        QtCore.QT_TRANSLATE_NOOP("Surface", "3D Surface"),
        QtCore.QT_TRANSLATE_NOOP("Surface", "Create a 3D Surface Operation from a model"),
        PathSurface.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathSurfaceGui... done\n")
