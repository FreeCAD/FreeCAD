# -*- coding: utf-8 -*-
# ***************************************************************************
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
import Path.Base.Gui.Util as PathGuiUtil
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Probe as PathProbe
import PathGui

from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtCore, QtGui

__title__ = "CAM Probing Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Probing operation page controller and command implementation."


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for the Probing operation."""

    def getForm(self):
        """getForm() ... returns UI"""
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpProbeEdit.ui")

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        self.updateToolController(obj, self.form.toolController)
        PathGuiUtil.updateInputField(obj, "Xoffset", self.form.Xoffset)
        PathGuiUtil.updateInputField(obj, "Yoffset", self.form.Yoffset)
        obj.PointCountX = self.form.PointCountX.value()
        obj.PointCountY = self.form.PointCountY.value()
        obj.OutputFileName = str(self.form.OutputFileName.text())

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.setupToolController(obj, self.form.toolController)
        self.form.Xoffset.setText(
            FreeCAD.Units.Quantity(obj.Xoffset.Value, FreeCAD.Units.Length).UserString
        )
        self.form.Yoffset.setText(
            FreeCAD.Units.Quantity(obj.Yoffset.Value, FreeCAD.Units.Length).UserString
        )
        self.form.OutputFileName.setText(obj.OutputFileName)
        self.form.PointCountX.setValue(obj.PointCountX)
        self.form.PointCountY.setValue(obj.PointCountY)

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.PointCountX.valueChanged)
        signals.append(self.form.PointCountY.valueChanged)
        signals.append(self.form.OutputFileName.editingFinished)
        signals.append(self.form.Xoffset.valueChanged)
        signals.append(self.form.Yoffset.valueChanged)
        self.form.SetOutputFileName.clicked.connect(self.SetOutputFileName)
        return signals

    def SetOutputFileName(self):
        filename = QtGui.QFileDialog.getSaveFileName(
            self.form,
            translate("CAM_Probe", "Select Output File"),
            None,
            translate("CAM_Probe", "All Files (*.*)"),
        )
        if filename and filename[0]:
            self.obj.OutputFileName = str(filename[0])
            self.setFields(self.obj)


Command = PathOpGui.SetupOperation(
    "Probe",
    PathProbe.Create,
    TaskPanelOpPage,
    "CAM_Probe",
    QtCore.QT_TRANSLATE_NOOP("CAM_Probe", "Probe"),
    QtCore.QT_TRANSLATE_NOOP("CAM_Probe", "Create a Probing Grid from a job stock"),
    PathProbe.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathProbeGui... done\n")
