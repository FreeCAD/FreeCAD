# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

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

    def getForm(self):
        """getForm() ... return UI"""

        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpHelixEdit.ui")
        comboToPropertyMap = [("startAt", "StartAt"), ("cutMode", "CutMode")]

        enumTups = PathHelix.ObjectHelix.helixOpPropertyEnumerations(dataType="raw")

        self.populateCombobox(form, enumTups, comboToPropertyMap)
        return form

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        Path.Log.track()
        if obj.CutMode != str(self.form.cutMode.currentData()):
            obj.CutMode = str(self.form.cutMode.currentData())
        if obj.StartAt != str(self.form.startAt.currentData()):
            obj.StartAt = str(self.form.startAt.currentData())
        if obj.StepOver != self.form.stepOverPercent.value():
            obj.StepOver = self.form.stepOverPercent.value()
        PathGuiUtil.updateInputField(
            obj, "RadialStockToLeaveOuter", self.form.RadialStockToLeaveOuter
        )

        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        Path.Log.track()

        self.form.stepOverPercent.setValue(obj.StepOver)
        self.selectInComboBox(obj.CutMode, self.form.cutMode)
        self.selectInComboBox(obj.StartAt, self.form.startAt)

        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        self.form.RadialStockToLeaveOuter.setText(
            FreeCAD.Units.Quantity(
                obj.RadialStockToLeaveOuter.Value, FreeCAD.Units.Length
            ).UserString
        )

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []

        signals.append(self.form.stepOverPercent.editingFinished)
        signals.append(self.form.RadialStockToLeaveOuter.editingFinished)
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
