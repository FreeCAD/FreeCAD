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
import Path
import Path.Base.Gui.Util as PathGuiUtil
import Path.Op.Deburr as PathDeburr
import Path.Op.Gui.Base as PathOpGui
from PySide import QtCore, QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "Path Deburr Operation UI"
__author__ = "sliptonic (Brad Collette), Schildkroet"
__url__ = "https://www.freecad.org"
__doc__ = "Deburr operation page controller and command implementation."


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


class TaskPanelBaseGeometryPage(PathOpGui.TaskPanelBaseGeometryPage):
    """Enhanced base geometry page to also allow special base objects."""

    def super(self):
        return super(TaskPanelBaseGeometryPage, self)

    def addBaseGeometry(self, selection):
        self.super().addBaseGeometry(selection)


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for the Deburr operation."""

    _ui_form = ":/panels/PageOpDeburrEdit.ui"

    def getForm(self):
        form = FreeCADGui.PySideUic.loadUi(self._ui_form)
        comboToPropertyMap = [("direction", "Direction")]
        enumTups = PathDeburr.ObjectDeburr.propertyEnumerations(dataType="raw")

        self.populateCombobox(form, enumTups, comboToPropertyMap)

        return form

    def initPage(self, obj):
        self.opImagePath = "{}Mod/Path/Images/Ops/{}".format(
            FreeCAD.getHomePath(), "chamfer.svg"
        )
        self.opImage = QtGui.QPixmap(self.opImagePath)
        self.form.opImage.setPixmap(self.opImage)
        iconMiter = QtGui.QIcon(":/icons/edge-join-miter-not.svg")
        iconMiter.addFile(":/icons/edge-join-miter.svg", state=QtGui.QIcon.On)
        iconRound = QtGui.QIcon(":/icons/edge-join-round-not.svg")
        iconRound.addFile(":/icons/edge-join-round.svg", state=QtGui.QIcon.On)
        self.form.joinMiter.setIcon(iconMiter)
        self.form.joinRound.setIcon(iconRound)

    def getFields(self, obj):
        PathGuiUtil.updateInputField(obj, "Width", self.form.value_W)
        PathGuiUtil.updateInputField(obj, "ExtraDepth", self.form.value_h)
        if self.form.joinRound.isChecked():
            obj.Join = "Round"
        elif self.form.joinMiter.isChecked():
            obj.Join = "Miter"

        if obj.Direction != str(self.form.direction.currentData()):
            obj.Direction = str(self.form.direction.currentData())

        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

    def setFields(self, obj):
        self.form.value_W.setText(
            FreeCAD.Units.Quantity(obj.Width.Value, FreeCAD.Units.Length).UserString
        )
        self.form.value_h.setText(
            FreeCAD.Units.Quantity(
                obj.ExtraDepth.Value, FreeCAD.Units.Length
            ).UserString
        )
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)
        self.form.joinRound.setChecked("Round" == obj.Join)
        self.form.joinMiter.setChecked("Miter" == obj.Join)
        self.form.joinFrame.hide()
        self.selectInComboBox(obj.Direction, self.form.direction)

    def updateWidth(self):
        PathGuiUtil.updateInputField(self.obj, "Width", self.form.value_W)

    def updateExtraDepth(self):
        PathGuiUtil.updateInputField(self.obj, "ExtraDepth", self.form.value_h)

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
        """taskPanelBaseGeometryPage(obj, features) ... return page for adding base geometries."""
        return TaskPanelBaseGeometryPage(obj, features)


Command = PathOpGui.SetupOperation(
    "Deburr",
    PathDeburr.Create,
    TaskPanelOpPage,
    "Path_Deburr",
    QT_TRANSLATE_NOOP("Path_Deburr", "Deburr"),
    QT_TRANSLATE_NOOP(
        "Path_Deburr", "Creates a Deburr Path along Edges or around Faces"
    ),
    PathDeburr.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathDeburrGui... done\n")
