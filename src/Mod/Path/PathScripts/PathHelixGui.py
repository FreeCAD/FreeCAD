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
import PathScripts.PathHelix as PathHelix
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
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpHelixEdit.ui")

    def getFields(self, obj):
        PathLog.track()
        self.obj.Direction = str(self.form.direction.currentText())
        self.obj.StartSide = str(self.form.startSide.currentText())
        self.obj.StepOver = self.form.stepOverPercent.value()

        self.updateToolController(obj, self.form.toolController)

    def setFields(self, obj):
        PathLog.track()

        self.form.stepOverPercent.setValue(self.obj.StepOver)
        self.selectInComboBox(self.obj.Direction, self.form.direction)
        self.selectInComboBox(self.obj.StartSide, self.form.startSide)

        self.setupToolController(self.obj, self.form.toolController)

    def getSignalsForUpdate(self, obj):
        signals = []

        signals.append(self.form.stepOverPercent.editingFinished)
        signals.append(self.form.direction.currentIndexChanged)
        signals.append(self.form.startSide.currentIndexChanged)
        signals.append(self.form.toolController.currentIndexChanged)

        return signals

PathOpGui.SetupOperation('Helix',
        PathHelix.Create,
        TaskPanelOpPage,
        'Path-Helix',
        QtCore.QT_TRANSLATE_NOOP("PathHelix", "Helix"),
        "P, O",
        QtCore.QT_TRANSLATE_NOOP("PathHelix", "Creates a Path Helix object from a features of a base object"))

FreeCAD.Console.PrintLog("Loading PathHelixGui... done\n")
