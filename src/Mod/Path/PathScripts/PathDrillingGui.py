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
import PathScripts.PathCircularHoleBaseGui as PathCircularHoleBaseGui
import PathScripts.PathDrilling as PathDrilling
import PathScripts.PathLog as PathLog
import PathScripts.PathOpGui as PathOpGui

from PySide import QtCore, QtGui

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.NOTICE, PathLog.thisModule())


class TaskPanelOpPage(PathCircularHoleBaseGui.TaskPanelOpPage):

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpDrillingEdit.ui")

    def getFields(self, obj):
        PathLog.track()
        self.obj.PeckDepth = FreeCAD.Units.Quantity(self.form.peckDepth.text()).Value
        self.obj.RetractHeight = FreeCAD.Units.Quantity(self.form.retractHeight.text()).Value
        self.obj.DwellTime = FreeCAD.Units.Quantity(self.form.dwellTime.text()).Value

        self.obj.DwellEnabled = self.form.dwellEnabled.isChecked()
        self.obj.PeckEnabled = self.form.peckEnabled.isChecked()
        self.obj.AddTipLength = self.form.useTipLength.isChecked()

        self.updateToolController(obj, self.form.toolController)

    def setFields(self, obj):
        PathLog.track()

        self.form.peckDepth.setText(FreeCAD.Units.Quantity(self.obj.PeckDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.retractHeight.setText(FreeCAD.Units.Quantity(self.obj.RetractHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.dwellTime.setText(str(self.obj.DwellTime))

        if self.obj.DwellEnabled:
            self.form.dwellEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.dwellEnabled.setCheckState(QtCore.Qt.Unchecked)

        if self.obj.PeckEnabled:
            self.form.peckEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.peckEnabled.setCheckState(QtCore.Qt.Unchecked)

        if self.obj.AddTipLength:
            self.form.useTipLength.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.useTipLength.setCheckState(QtCore.Qt.Unchecked)

        self.setupToolController(self.obj, self.form.toolController)

    def getSignalsForUpdate(self, obj):
        signals = []

        signals.append(self.form.retractHeight.editingFinished)
        signals.append(self.form.peckDepth.editingFinished)
        signals.append(self.form.dwellTime.editingFinished)
        signals.append(self.form.dwellEnabled.stateChanged)
        signals.append(self.form.peckEnabled.stateChanged)
        signals.append(self.form.useTipLength.stateChanged)
        signals.append(self.form.toolController.currentIndexChanged)

        return signals

PathOpGui.SetupOperation('Drilling',
        PathDrilling.Create,
        TaskPanelOpPage,
        'Path-Drilling',
        QtCore.QT_TRANSLATE_NOOP("PathDrilling", "Drilling"),
        QtCore.QT_TRANSLATE_NOOP("PathDrilling", "Creates a Path Drilling object from a features of a base object"))

FreeCAD.Console.PrintLog("Loading PathDrillingGui... done\n")
