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
import Path
import PathScripts.PathAreaOpGui as PathAreaOpGui
import PathScripts.PathLog as PathLog
import PathScripts.PathPocket as PathPocket
import PathScripts.PathSelection as PathSelection

from PathScripts import PathUtils
from PySide import QtCore, QtGui

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class TaskPanelOpPage(PathAreaOpGui.TaskPanelPage):

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpPocketEdit.ui")

    def getFields(self, obj):
        self.obj.MaterialAllowance = FreeCAD.Units.Quantity(self.form.extraOffset.text()).Value
        self.obj.CutMode = str(self.form.cutMode.currentText())
        self.obj.OffsetPattern = str(self.form.offsetPattern.currentText())
        self.obj.ZigZagAngle = FreeCAD.Units.Quantity(self.form.zigZagAngle.text()).Value
        self.obj.StepOver = self.form.stepOverPercent.value()
        self.obj.UseStartPoint = self.form.useStartPoint.isChecked()

        tc = PathUtils.findToolController(self.obj, self.form.toolController.currentText())
        self.obj.ToolController = tc

    def setFields(self, obj):
        self.form.extraOffset.setText(FreeCAD.Units.Quantity(self.obj.MaterialAllowance.Value, FreeCAD.Units.Length).UserString)
        self.form.useStartPoint.setChecked(self.obj.UseStartPoint)
        self.form.zigZagAngle.setText(FreeCAD.Units.Quantity(self.obj.ZigZagAngle, FreeCAD.Units.Angle).UserString)
        self.form.stepOverPercent.setValue(self.obj.StepOver)

        self.selectInComboBox(self.obj.OffsetPattern, self.form.offsetPattern)
        self.selectInComboBox(self.obj.CutMode, self.form.cutMode)
        self.setupToolController(self.obj, self.form.toolController)

    def getSignalsForUpdate(self, obj):
        signals = []
        # operation
        signals.append(self.form.cutMode.currentIndexChanged)
        signals.append(self.form.useStartPoint.clicked)

        # Pattern
        signals.append(self.form.offsetPattern.currentIndexChanged)
        signals.append(self.form.stepOverPercent.editingFinished)
        signals.append(self.form.zigZagAngle.editingFinished)
        signals.append(self.form.extraOffset.editingFinished)
        signals.append(self.form.toolController.currentIndexChanged)
        return signals

PathAreaOpGui.SetupOperation('Pocket',
        PathPocket.Create,
        TaskPanelOpPage,
        'Path-Pocket',
        QtCore.QT_TRANSLATE_NOOP("PathPocket", "Pocket"),
        "P, O",
        QtCore.QT_TRANSLATE_NOOP("PathPocket", "Creates a Path Pocket object from a face or faces"))

FreeCAD.Console.PrintLog("Loading PathPocketGui... done\n")
