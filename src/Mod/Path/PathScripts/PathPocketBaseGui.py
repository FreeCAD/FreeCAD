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
import PathScripts.PathLog as PathLog
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathPocket as PathPocket
import PathScripts.PathSelection as PathSelection

from PySide import QtCore, QtGui

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

FeaturePocket = 0x01
FeatureFacing = 0x02

class TaskPanelOpPage(PathOpGui.TaskPanelPage):

    def getForm(self):
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpPocketFullEdit.ui")

        if not FeaturePocket & self.pocketFeatures():
            form.pocketWidget.hide()

        if not FeatureFacing & self.pocketFeatures():
            form.facingWidget.hide()

        return form

    def getFields(self, obj):
        self.obj.CutMode = str(self.form.cutMode.currentText())
        self.obj.StepOver = self.form.stepOverPercent.value()
        self.obj.OffsetPattern = str(self.form.offsetPattern.currentText())
        self.obj.ZigZagAngle = FreeCAD.Units.Quantity(self.form.zigZagAngle.text()).Value

        self.updateToolController(self.obj, self.form.toolController)

        if FeaturePocket & self.pocketFeatures():
            self.obj.MaterialAllowance = FreeCAD.Units.Quantity(self.form.extraOffset.text()).Value
            self.obj.UseStartPoint = self.form.useStartPoint.isChecked()

        if FeatureFacing & self.pocketFeatures():
            self.obj.PassExtension = FreeCAD.Units.Quantity(self.form.passExtension.text()).Value
            self.obj.BoundaryShape = str(self.form.boundaryShape.currentText())

    def setFields(self, obj):
        self.form.zigZagAngle.setText(FreeCAD.Units.Quantity(self.obj.ZigZagAngle, FreeCAD.Units.Angle).UserString)
        self.form.stepOverPercent.setValue(self.obj.StepOver)

        self.selectInComboBox(self.obj.OffsetPattern, self.form.offsetPattern)
        self.selectInComboBox(self.obj.CutMode, self.form.cutMode)
        self.setupToolController(self.obj, self.form.toolController)

        if FeaturePocket & self.pocketFeatures():
            self.form.useStartPoint.setChecked(self.obj.UseStartPoint)
            self.form.extraOffset.setText(FreeCAD.Units.Quantity(self.obj.MaterialAllowance.Value, FreeCAD.Units.Length).UserString)

        if FeatureFacing & self.pocketFeatures():
            self.form.passExtension.setText(FreeCAD.Units.Quantity(self.obj.PassExtension.Value, FreeCAD.Units.Length).UserString)
            self.selectInComboBox(self.obj.BoundaryShape, self.form.boundaryShape)

    def getSignalsForUpdate(self, obj):
        signals = []

        signals.append(self.form.cutMode.currentIndexChanged)
        signals.append(self.form.offsetPattern.currentIndexChanged)
        signals.append(self.form.stepOverPercent.editingFinished)
        signals.append(self.form.zigZagAngle.editingFinished)
        signals.append(self.form.toolController.currentIndexChanged)

        if FeaturePocket & self.pocketFeatures():
            signals.append(self.form.extraOffset.editingFinished)
            signals.append(self.form.useStartPoint.clicked)

        if FeatureFacing & self.pocketFeatures():
            signals.append(self.form.boundaryShape.currentIndexChanged)
            signals.append(self.form.passExtension.editingFinished)

        return signals
