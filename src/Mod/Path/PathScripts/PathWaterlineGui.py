# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2020 russ4262 <russ4262@gmail.com>                      *
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
import PathScripts.PathWaterline as PathWaterline
import PathScripts.PathGui as PathGui
import PathScripts.PathOpGui as PathOpGui

from PySide import QtCore

__title__ = "Path Waterline Operation UI"
__author__ = "sliptonic (Brad Collette), russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Waterline operation page controller and command implementation."


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    '''Page controller class for the Waterline operation.'''

    def getForm(self):
        '''getForm() ... returns UI'''
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpWaterlineEdit.ui")

    def getFields(self, obj):
        '''getFields(obj) ... transfers values from UI to obj's proprties'''
        PathGui.updateInputField(obj, 'BoundaryAdjustment', self.form.boundaryAdjustment)
        PathGui.updateInputField(obj, 'SampleInterval', self.form.sampleInterval)

        if obj.StepOver != self.form.stepOver.value():
            obj.StepOver = self.form.stepOver.value()

        if obj.BoundBox != str(self.form.boundBoxSelect.currentText()):
            obj.BoundBox = str(self.form.boundBoxSelect.currentText())

        if obj.OptimizeLinearPaths != self.form.optimizeEnabled.isChecked():
            obj.OptimizeLinearPaths = self.form.optimizeEnabled.isChecked()

        self.updateToolController(obj, self.form.toolController)

    def setFields(self, obj):
        '''setFields(obj) ... transfers obj's property values to UI'''
        self.setupToolController(obj, self.form.toolController)
        self.selectInComboBox(obj.BoundBox, self.form.boundBoxSelect)
        self.form.boundaryAdjustment.setText(FreeCAD.Units.Quantity(obj.BoundaryAdjustment.Value, FreeCAD.Units.Length).UserString)
        self.form.stepOver.setValue(obj.StepOver)
        self.form.sampleInterval.setText(str(obj.SampleInterval))

        if obj.OptimizeLinearPaths:
            self.form.optimizeEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.optimizeEnabled.setCheckState(QtCore.Qt.Unchecked)

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals for updating obj'''
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.boundBoxSelect.currentIndexChanged)
        signals.append(self.form.boundaryAdjustment.editingFinished)
        signals.append(self.form.stepOver.editingFinished)
        signals.append(self.form.sampleInterval.editingFinished)
        signals.append(self.form.optimizeEnabled.stateChanged)

        return signals

    def updateVisibility(self):
        self.form.boundBoxSelect.setEnabled(True) 
        self.form.boundaryAdjustment.setEnabled(True) 
        self.form.stepOver.setEnabled(True)
        self.form.sampleInterval.setEnabled(True)
        self.form.optimizeEnabled.setEnabled(True)

    def registerSignalHandlers(self, obj):
        # self.form.clearLayers.currentIndexChanged.connect(self.updateVisibility)
        pass


Command = PathOpGui.SetupOperation('Waterline',
        PathWaterline.Create,
        TaskPanelOpPage,
        'Path-Waterline',
        QtCore.QT_TRANSLATE_NOOP("Waterline", "Waterline"),
        QtCore.QT_TRANSLATE_NOOP("Waterline", "Create a Waterline Operation from a model"),
        PathWaterline.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathWaterlineGui... done\n")
