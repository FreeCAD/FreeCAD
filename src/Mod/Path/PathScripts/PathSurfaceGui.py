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

    def getForm(self):
        '''getForm() ... returns UI'''
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpSurfaceEdit.ui")

    def getFields(self, obj):
        '''getFields(obj) ... transfers values from UI to obj's proprties'''
        PathGui.updateInputField(obj, 'DepthOffset', self.form.depthOffset)
        PathGui.updateInputField(obj, 'SampleInterval', self.form.sampleInterval)

        if obj.StepOver != self.form.stepOver.value():
            obj.StepOver = self.form.stepOver.value()

        if obj.Algorithm != str(self.form.algorithmSelect.currentText()):
            obj.Algorithm = str(self.form.algorithmSelect.currentText())

        if obj.BoundBox != str(self.form.boundBoxSelect.currentText()):
            obj.BoundBox = str(self.form.boundBoxSelect.currentText())

        if obj.DropCutterDir != str(self.form.dropCutterDirSelect.currentText()):
            obj.DropCutterDir = str(self.form.dropCutterDirSelect.currentText())

        obj.DropCutterExtraOffset.x = FreeCAD.Units.Quantity(self.form.boundBoxExtraOffsetX.text()).Value
        obj.DropCutterExtraOffset.y = FreeCAD.Units.Quantity(self.form.boundBoxExtraOffsetY.text()).Value

        if obj.Optimize != self.form.optimizeEnabled.isChecked():
            obj.Optimize = self.form.optimizeEnabled.isChecked()

        self.updateToolController(obj, self.form.toolController)

    def setFields(self, obj):
        '''setFields(obj) ... transfers obj's property values to UI'''
        self.selectInComboBox(obj.Algorithm, self.form.algorithmSelect)
        self.selectInComboBox(obj.BoundBox, self.form.boundBoxSelect)
        self.selectInComboBox(obj.DropCutterDir, self.form.dropCutterDirSelect)

        self.form.boundBoxExtraOffsetX.setText(str(obj.DropCutterExtraOffset.x))
        self.form.boundBoxExtraOffsetY.setText(str(obj.DropCutterExtraOffset.y))
        self.form.depthOffset.setText(FreeCAD.Units.Quantity(obj.DepthOffset.Value, FreeCAD.Units.Length).UserString)
        self.form.sampleInterval.setText(str(obj.SampleInterval))
        self.form.stepOver.setValue(obj.StepOver)

        if obj.Optimize:
            self.form.optimizeEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.optimizeEnabled.setCheckState(QtCore.Qt.Unchecked)

        self.setupToolController(obj, self.form.toolController)

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals for updating obj'''
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.algorithmSelect.currentIndexChanged)
        signals.append(self.form.boundBoxSelect.currentIndexChanged)
        signals.append(self.form.dropCutterDirSelect.currentIndexChanged)
        signals.append(self.form.boundBoxExtraOffsetX.editingFinished)
        signals.append(self.form.boundBoxExtraOffsetY.editingFinished)
        signals.append(self.form.sampleInterval.editingFinished)
        signals.append(self.form.stepOver.editingFinished)
        signals.append(self.form.depthOffset.editingFinished)
        signals.append(self.form.optimizeEnabled.stateChanged)

        return signals

    def updateVisibility(self):
        if self.form.algorithmSelect.currentText() == "OCL Dropcutter":
            self.form.boundBoxExtraOffsetX.setEnabled(True)
            self.form.boundBoxExtraOffsetY.setEnabled(True)
            self.form.boundBoxSelect.setEnabled(True)
            self.form.dropCutterDirSelect.setEnabled(True)
            self.form.stepOver.setEnabled(True)
        else:
            self.form.boundBoxExtraOffsetX.setEnabled(False)
            self.form.boundBoxExtraOffsetY.setEnabled(False)
            self.form.boundBoxSelect.setEnabled(False)
            self.form.dropCutterDirSelect.setEnabled(False)
            self.form.stepOver.setEnabled(False)

    def registerSignalHandlers(self, obj):
        self.form.algorithmSelect.currentIndexChanged.connect(self.updateVisibility)


Command = PathOpGui.SetupOperation('Surface',
        PathSurface.Create,
        TaskPanelOpPage,
        'Path-3DSurface',
        QtCore.QT_TRANSLATE_NOOP("Surface", "3D Surface"),
        QtCore.QT_TRANSLATE_NOOP("Surface", "Create a 3D Surface Operation from a model"),
        PathSurface.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathSurfaceGui... done\n")
