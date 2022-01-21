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
import PathGui as PGui  # ensure Path/Gui/Resources are loaded
import PathScripts.PathCircularHoleBaseGui as PathCircularHoleBaseGui
import PathScripts.PathDrilling as PathDrilling
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.PathOpGui as PathOpGui

from PySide import QtCore

__title__ = "Path Drilling Operation UI."
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "UI and Command for Path Drilling Operation."
__contributors__ = "IMBack!"

LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.NOTICE, PathLog.thisModule())


class TaskPanelOpPage(PathCircularHoleBaseGui.TaskPanelOpPage):
    """Controller for the drilling operation's page"""

    def initPage(self, obj):
        # pylint: disable=attribute-defined-outside-init
        self.peckDepthSpinBox = PathGui.QuantitySpinBox(
            self.form.peckDepth, obj, "PeckDepth"
        )
        self.peckRetractSpinBox = PathGui.QuantitySpinBox(
            self.form.peckRetractHeight, obj, "RetractHeight"
        )
        self.dwellTimeSpinBox = PathGui.QuantitySpinBox(
            self.form.dwellTime, obj, "DwellTime"
        )

    def registerSignalHandlers(self, obj):
        self.form.peckEnabled.toggled.connect(self.form.peckDepth.setEnabled)
        self.form.peckEnabled.toggled.connect(self.form.dwellEnabled.setDisabled)

        self.form.dwellEnabled.toggled.connect(self.form.dwellTime.setEnabled)
        self.form.dwellEnabled.toggled.connect(self.form.dwellTimelabel.setEnabled)
        self.form.dwellEnabled.toggled.connect(self.form.peckEnabled.setDisabled)

        self.form.peckRetractHeight.setEnabled(True)
        self.form.retractLabel.setEnabled(True)

        if self.form.peckEnabled.isChecked():
            self.form.dwellEnabled.setEnabled(False)
            self.form.peckDepth.setEnabled(True)
            self.form.peckDepthLabel.setEnabled(True)
        elif self.form.dwellEnabled.isChecked():
            self.form.peckEnabled.setEnabled(False)
            self.form.dwellTime.setEnabled(True)
            self.form.dwellTimelabel.setEnabled(True)

    def getForm(self):
        """getForm() ... return UI"""
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpDrillingEdit.ui")

    def updateQuantitySpinBoxes(self, index=None):
        # pylint: disable=unused-argument
        self.peckDepthSpinBox.updateSpinBox()
        self.peckRetractSpinBox.updateSpinBox()
        self.dwellTimeSpinBox.updateSpinBox()

    def getFields(self, obj):
        """setFields(obj) ... update obj's properties with values from the UI"""
        PathLog.track()
        self.peckDepthSpinBox.updateProperty()
        self.peckRetractSpinBox.updateProperty()
        self.dwellTimeSpinBox.updateProperty()

        if obj.DwellEnabled != self.form.dwellEnabled.isChecked():
            obj.DwellEnabled = self.form.dwellEnabled.isChecked()
        if obj.PeckEnabled != self.form.peckEnabled.isChecked():
            obj.PeckEnabled = self.form.peckEnabled.isChecked()
        if obj.ExtraOffset != str(self.form.ExtraOffset.currentText()):
            obj.ExtraOffset = str(self.form.ExtraOffset.currentText())

        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

    def setFields(self, obj):
        """setFields(obj) ... update UI with obj properties' values"""
        PathLog.track()
        self.updateQuantitySpinBoxes()

        if obj.DwellEnabled:
            self.form.dwellEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.dwellEnabled.setCheckState(QtCore.Qt.Unchecked)

        if obj.PeckEnabled:
            self.form.peckEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.peckEnabled.setCheckState(QtCore.Qt.Unchecked)

        self.selectInComboBox(obj.ExtraOffset, self.form.ExtraOffset)

        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals which cause the receiver to update the model"""
        signals = []

        signals.append(self.form.peckRetractHeight.editingFinished)
        signals.append(self.form.peckDepth.editingFinished)
        signals.append(self.form.dwellTime.editingFinished)
        signals.append(self.form.dwellEnabled.stateChanged)
        signals.append(self.form.peckEnabled.stateChanged)
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)
        signals.append(self.form.ExtraOffset.currentIndexChanged)

        return signals

    def updateData(self, obj, prop):
        if prop in ["PeckDepth", "RetractHeight"] and not prop in ["Base", "Disabled"]:
            self.updateQuantitySpinBoxes()


Command = PathOpGui.SetupOperation(
    "Drilling",
    PathDrilling.Create,
    TaskPanelOpPage,
    "Path_Drilling",
    QtCore.QT_TRANSLATE_NOOP("Path_Drilling", "Drilling"),
    QtCore.QT_TRANSLATE_NOOP(
        "Path_Drilling",
        "Creates a Path Drilling object from a features of a base object",
    ),
    PathDrilling.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathDrillingGui... done\n")
