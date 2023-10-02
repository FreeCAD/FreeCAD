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
import Path.Op.Drilling as PathDrilling
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Gui.CircularHoleBase as PathCircularHoleBaseGui
import PathGui

from PySide import QtCore

__title__ = "Path Drilling Operation UI."
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "UI and Command for Path Drilling Operation."
__contributors__ = "IMBack!"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TaskPanelOpPage(PathCircularHoleBaseGui.TaskPanelOpPage):
    """Controller for the drilling operation's page"""

    def initPage(self, obj):
        self.peckDepthSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.peckDepth, obj, "PeckDepth"
        )
        self.peckRetractSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.peckRetractHeight, obj, "RetractHeight"
        )
        self.dwellTimeSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.dwellTime, obj, "DwellTime"
        )
        self.form.chipBreakEnabled.setEnabled(False)

    def registerSignalHandlers(self, obj):
        self.form.peckEnabled.toggled.connect(self.form.peckDepth.setEnabled)
        self.form.peckEnabled.toggled.connect(self.form.dwellEnabled.setDisabled)
        self.form.peckEnabled.toggled.connect(self.setChipBreakControl)

        self.form.dwellEnabled.toggled.connect(self.form.dwellTime.setEnabled)
        self.form.dwellEnabled.toggled.connect(self.form.dwellTimelabel.setEnabled)
        self.form.dwellEnabled.toggled.connect(self.form.peckEnabled.setDisabled)
        self.form.dwellEnabled.toggled.connect(self.setChipBreakControl)

        self.form.peckRetractHeight.setEnabled(True)
        self.form.retractLabel.setEnabled(True)

        if self.form.peckEnabled.isChecked():
            self.form.dwellEnabled.setEnabled(False)
            self.form.peckDepth.setEnabled(True)
            self.form.peckDepthLabel.setEnabled(True)
            self.form.chipBreakEnabled.setEnabled(True)
        elif self.form.dwellEnabled.isChecked():
            self.form.peckEnabled.setEnabled(False)
            self.form.dwellTime.setEnabled(True)
            self.form.dwellTimelabel.setEnabled(True)
            self.form.chipBreakEnabled.setEnabled(False)
        else:
            self.form.chipBreakEnabled.setEnabled(False)

    def setChipBreakControl(self):
        self.form.chipBreakEnabled.setEnabled(self.form.peckEnabled.isChecked())

    def getForm(self):
        """getForm() ... return UI"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpDrillingEdit.ui")

        comboToPropertyMap = [("ExtraOffset", "ExtraOffset")]
        enumTups = PathDrilling.ObjectDrilling.propertyEnumerations(dataType="raw")
        self.populateCombobox(form, enumTups, comboToPropertyMap)

        return form

    def updateQuantitySpinBoxes(self, index=None):
        self.peckDepthSpinBox.updateSpinBox()
        self.peckRetractSpinBox.updateSpinBox()
        self.dwellTimeSpinBox.updateSpinBox()

    def getFields(self, obj):
        """setFields(obj) ... update obj's properties with values from the UI"""
        Path.Log.track()
        self.peckDepthSpinBox.updateProperty()
        self.peckRetractSpinBox.updateProperty()
        self.dwellTimeSpinBox.updateProperty()

        if obj.KeepToolDown != self.form.KeepToolDownEnabled.isChecked():
            obj.KeepToolDown = self.form.KeepToolDownEnabled.isChecked()
        if obj.DwellEnabled != self.form.dwellEnabled.isChecked():
            obj.DwellEnabled = self.form.dwellEnabled.isChecked()
        if obj.PeckEnabled != self.form.peckEnabled.isChecked():
            obj.PeckEnabled = self.form.peckEnabled.isChecked()
        if obj.chipBreakEnabled != self.form.chipBreakEnabled.isChecked():
            obj.chipBreakEnabled = self.form.chipBreakEnabled.isChecked()
        if obj.ExtraOffset != str(self.form.ExtraOffset.currentData()):
            obj.ExtraOffset = str(self.form.ExtraOffset.currentData())

        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

    def setFields(self, obj):
        """setFields(obj) ... update UI with obj properties' values"""
        Path.Log.track()
        self.updateQuantitySpinBoxes()

        if not hasattr(obj,"KeepToolDown"):
            obj.addProperty("App::PropertyBool", "KeepToolDown", "Drill",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Apply G99 retraction: only retract to RetractHeight between holes in this operation"))

        if obj.KeepToolDown:
            self.form.KeepToolDownEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.KeepToolDownEnabled.setCheckState(QtCore.Qt.Unchecked)

        if obj.DwellEnabled:
            self.form.dwellEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.dwellEnabled.setCheckState(QtCore.Qt.Unchecked)

        if obj.PeckEnabled:
            self.form.peckEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.peckEnabled.setCheckState(QtCore.Qt.Unchecked)
            self.form.chipBreakEnabled.setEnabled(False)

        if obj.chipBreakEnabled:
            self.form.chipBreakEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.chipBreakEnabled.setCheckState(QtCore.Qt.Unchecked)

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
        signals.append(self.form.chipBreakEnabled.stateChanged)
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)
        signals.append(self.form.ExtraOffset.currentIndexChanged)
        signals.append(self.form.KeepToolDownEnabled.stateChanged)

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
        "Creates a Path Drilling object from the features of a base object",
    ),
    PathDrilling.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathDrillingGui... done\n")
