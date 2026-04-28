# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.Util as PathGuiUtil
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Gui.CircularHoleBase as PathCircularHoleBaseGui
import Path.Op.Helix as PathHelix
import PathGui
from PySide.QtCore import QT_TRANSLATE_NOOP

translate = FreeCAD.Qt.translate


__doc__ = "Helix operation page controller and command implementation."

LOGLEVEL = False

if LOGLEVEL:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.NOTICE, Path.Log.thisModule())


class TaskPanelOpPage(PathCircularHoleBaseGui.TaskPanelOpPage):
    """Page controller class for Helix operations."""

    def initPage(self, obj):
        self.helixMaxPitchSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.helixMaxPitch, obj, "HelixMaxPitch"
        )
        self.helixMaxRampAngleSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.helixMaxRampAngle, obj, "HelixMaxRampAngle"
        )
        self.radialStockToLeaveOuterSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.radialStockToLeaveOuter, obj, "RadialStockToLeaveOuter"
        )

    def getForm(self):
        """getForm() ... return UI"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpHelixEdit.ui")

        comboToPropertyMap = [("startAt", "StartAt"), ("cutMode", "CutMode")]
        enumTups = PathHelix.ObjectHelix.helixOpPropertyEnumerations(dataType="raw")
        self.populateCombobox(form, enumTups, comboToPropertyMap)

        return form

    def updateQuantitySpinBoxes(self, index=None):
        self.helixMaxPitchSpinBox.updateWidget()
        self.helixMaxRampAngleSpinBox.updateWidget()
        self.radialStockToLeaveOuterSpinBox.updateWidget()

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        Path.Log.track()
        self.helixMaxPitchSpinBox.updateProperty()
        self.helixMaxRampAngleSpinBox.updateProperty()
        self.radialStockToLeaveOuterSpinBox.updateProperty()

        if obj.CutMode != str(self.form.cutMode.currentData()):
            obj.CutMode = str(self.form.cutMode.currentData())
        if obj.StartAt != str(self.form.startAt.currentData()):
            obj.StartAt = str(self.form.startAt.currentData())

        if obj.StepOver != self.form.stepOverPercent.value():
            obj.StepOver = self.form.stepOverPercent.value()

        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        Path.Log.track()
        self.updateQuantitySpinBoxes()

        self.form.stepOverPercent.setValue(obj.StepOver)

        self.selectInComboBox(obj.CutMode, self.form.cutMode)
        self.selectInComboBox(obj.StartAt, self.form.startAt)

        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []

        signals.append(self.form.helixMaxPitch.editingFinished)
        signals.append(self.form.helixMaxRampAngle.editingFinished)
        signals.append(self.form.radialStockToLeaveOuter.editingFinished)
        signals.append(self.form.stepOverPercent.editingFinished)

        signals.append(self.form.cutMode.currentIndexChanged)
        signals.append(self.form.startAt.currentIndexChanged)
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)

        return signals


Command = PathOpGui.SetupOperation(
    "Helix",
    PathHelix.Create,
    TaskPanelOpPage,
    "CAM_Helix",
    QT_TRANSLATE_NOOP("CAM_Helix", "Helix"),
    QT_TRANSLATE_NOOP("CAM_Helix", "Creates a Helical toolpath from the features of a base object"),
    PathHelix.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathHelixGui... done\n")
