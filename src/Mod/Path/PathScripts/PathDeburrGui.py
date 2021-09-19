# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathDeburr as PathDeburr
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.PathOpGui as PathOpGui
from PySide import QtCore, QtGui

__title__ = "Path Deburr Operation UI"
__author__ = "sliptonic (Brad Collette), Schildkroet"
__url__ = "https://www.freecadweb.org"
__doc__ = "Deburr operation page controller and command implementation."


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class TaskPanelBaseGeometryPage(PathOpGui.TaskPanelBaseGeometryPage):
    '''Enhanced base geometry page to also allow special base objects.'''

    def super(self):
        return super(TaskPanelBaseGeometryPage, self)

    def addBaseGeometry(self, selection):
        self.super().addBaseGeometry(selection)


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    '''Page controller class for the Deburr operation.'''

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpDeburrEdit.ui")

    def initPage(self, obj):
        self.opImagePath = "{}Mod/Path/Images/Ops/{}".format(FreeCAD.getHomePath(), 'chamfer.svg')  # pylint: disable=attribute-defined-outside-init
        self.opImage = QtGui.QPixmap(self.opImagePath)  # pylint: disable=attribute-defined-outside-init
        self.form.opImage.setPixmap(self.opImage)
        iconMiter = QtGui.QIcon(':/icons/edge-join-miter-not.svg')
        iconMiter.addFile(':/icons/edge-join-miter.svg', state=QtGui.QIcon.On)
        iconRound = QtGui.QIcon(':/icons/edge-join-round-not.svg')
        iconRound.addFile(':/icons/edge-join-round.svg', state=QtGui.QIcon.On)
        self.form.joinMiter.setIcon(iconMiter)
        self.form.joinRound.setIcon(iconRound)

    def getFields(self, obj):
        PathGui.updateInputField(obj, 'Width', self.form.value_W)
        PathGui.updateInputField(obj, 'ExtraDepth', self.form.value_h)
        if self.form.joinRound.isChecked():
            obj.Join = 'Round'
        elif self.form.joinMiter.isChecked():
            obj.Join = 'Miter'

        if obj.Direction != str(self.form.direction.currentText()):
            obj.Direction = str(self.form.direction.currentText())

        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

    def setFields(self, obj):
        self.form.value_W.setText(FreeCAD.Units.Quantity(obj.Width.Value, FreeCAD.Units.Length).UserString)
        self.form.value_h.setText(FreeCAD.Units.Quantity(obj.ExtraDepth.Value, FreeCAD.Units.Length).UserString)
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)
        self.form.joinRound.setChecked('Round' == obj.Join)
        self.form.joinMiter.setChecked('Miter' == obj.Join)
        self.form.joinFrame.hide()
        self.selectInComboBox(obj.Direction, self.form.direction)

    def updateWidth(self):
        PathGui.updateInputField(self.obj, 'Width', self.form.value_W)

    def updateExtraDepth(self):
        PathGui.updateInputField(self.obj, 'ExtraDepth', self.form.value_h)

    def getSignalsForUpdate(self, obj):
        signals = []
        signals.append(self.form.joinMiter.clicked)
        signals.append(self.form.joinRound.clicked)
        signals.append(self.form.coolantController.currentIndexChanged)
        signals.append(self.form.direction.currentIndexChanged)
        signals.append(self.form.value_W.valueChanged)
        signals.append(self.form.value_h.valueChanged)
        return signals

    def registerSignalHandlers(self, obj):
        self.form.value_W.editingFinished.connect(self.updateWidth)
        self.form.value_h.editingFinished.connect(self.updateExtraDepth)

    def taskPanelBaseGeometryPage(self, obj, features):
        '''taskPanelBaseGeometryPage(obj, features) ... return page for adding base geometries.'''
        return TaskPanelBaseGeometryPage(obj, features)


Command = PathOpGui.SetupOperation('Deburr',
        PathDeburr.Create,
        TaskPanelOpPage,
        'Path_Deburr',
        QtCore.QT_TRANSLATE_NOOP("PathDeburr", "Deburr"),
        QtCore.QT_TRANSLATE_NOOP("PathDeburr", "Creates a Deburr Path along Edges or around Faces"),
        PathDeburr.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathDeburrGui... done\n")
