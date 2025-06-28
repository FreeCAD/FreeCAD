# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Daniel Wood <s.d.wood.82@gmail.com>                *
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
import Path.Log as PathLog
import Path.Op.Gui.Base as PathOpGui

import Path.Base.Gui.Util as PathGuiUtil

from PySide import QtCore, QtGui

__title__ = "CAM Turning Base Gui"
__author__ = "dubstar-04 (Daniel Wood)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base class Gui implementation for turning operations."

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.NOTICE, PathLog.thisModule())

class TaskPanelTurnBase(PathOpGui.TaskPanelPage):
    """Page controller class for turning operations"""

    def initPage(self, obj):
        self.updating = False  # pylint: disable=attribute-defined-outside-init

    def getForm(self):
        """getForm() ... return UI"""
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpTurnBaseEdit.ui")

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's proprties"""
        PathLog.track()
        PathGuiUtil.updateInputField(obj, "StepOver", self.form.stepOver)

        obj.FinishPasses = self.form.finishPasses.value()
        obj.StockToLeave = self.form.stockToLeave.value()

        if obj.AllowGrooving != self.form.allowGrooving.isChecked():
            obj.AllowGrooving = self.form.allowGrooving.isChecked()

        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        PathLog.track()

        self.form.stepOver.setText(
            FreeCAD.Units.Quantity(obj.StepOver.Value, FreeCAD.Units.Length).UserString
        )
        self.form.finishPasses.setValue(obj.FinishPasses)
        self.form.stockToLeave.setValue(obj.StockToLeave)
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        if obj.AllowGrooving:
            self.form.allowGrooving.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.allowGrooving.setCheckState(QtCore.Qt.Unchecked)

        self.setOpFields(obj)

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []

        signals.append(self.form.stepOver.editingFinished)
        signals.append(self.form.finishPasses.valueChanged)
        signals.append(self.form.stockToLeave.valueChanged)
        signals.append(self.form.allowGrooving.stateChanged)

        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)

        return signals

    def setOpFields(self, obj):
        """setOpFields(obj) ... overwrite to set operations specific values.
        Should be overwritten by subclasses."""
        pass  # pylint: disable=unnecessary-pass
