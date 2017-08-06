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
import PathScripts.PathContour as PathContour
import PathScripts.PathLog as PathLog
import PathScripts.PathSelection as PathSelection

from PathScripts import PathUtils
from PySide import QtCore, QtGui

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class TaskPanelOpPage(PathAreaOpGui.TaskPanelPage):

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpContourEdit.ui")

    def getFields(self, obj):
        PathLog.track()
        self.obj.OffsetExtra = FreeCAD.Units.Quantity(self.form.extraOffset.text()).Value
        self.obj.UseComp = self.form.useCompensation.isChecked()
        self.obj.Direction = str(self.form.direction.currentText())
        self.obj.UseStartPoint = self.form.useStartPoint.isChecked()

        tc = PathUtils.findToolController(self.obj, self.form.toolController.currentText())
        self.obj.ToolController = tc

    def setFields(self, obj):
        PathLog.track()
        self.form.extraOffset.setText(FreeCAD.Units.Quantity(self.obj.OffsetExtra.Value, FreeCAD.Units.Length).UserString)
        self.form.useCompensation.setChecked(self.obj.UseComp)
        self.form.useStartPoint.setChecked(self.obj.UseStartPoint)

        self.selectInComboBox(self.obj.Direction, self.form.direction)
        self.setupToolController(self.obj, self.form.toolController)

    def getSignalsForUpdate(self, obj):
        PathLog.track()
        signals = []
        signals.append(self.form.direction.currentIndexChanged)
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.useCompensation.clicked)
        signals.append(self.form.useStartPoint.clicked)
        signals.append(self.form.extraOffset.editingFinished)
        return signals

PathAreaOpGui.SetupOperation('Contour',
        PathContour.Create,
        TaskPanelOpPage,
        'Path-Contour',
        QtCore.QT_TRANSLATE_NOOP("PathContour", "Contour"),
        "P, C",
        QtCore.QT_TRANSLATE_NOOP("PathContour", "Creates a Contour Path for the Base Object "))

FreeCAD.Console.PrintLog("Loading PathContourGui... done\n")
