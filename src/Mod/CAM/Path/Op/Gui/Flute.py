# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
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

__title__ = "CAM Flute Operation UI"
__author__ = "Connor (Billy Huddleston <billy@ivdc.com>)"
__url__ = "https://www.freecad.org"
__doc__ = "Flute operation task panel controller and command implementation."

import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.Util as PathGuiUtil
import Path.Op.Flute as PathFlute
import Path.Op.Gui.Base as PathOpGui

from PySide import QtCore

translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

# Z-variation threshold for flat-wire detection in the UI layer (mm).
_FLAT_UI_TOL = 0.01


def _classify_wire_selection(obj):
    """Classify the edge selection as "flat", "3d", "mixed", or "none"."""
    if not hasattr(obj, "Base") or not obj.Base:
        return "none"
    has_flat = False
    has_3d = False
    for base, subs in obj.Base:
        for sub in subs:
            if not sub.startswith("Edge"):
                continue
            try:
                shape = base.Shape.getElement(sub)
                zs = [v.Point.z for v in shape.Vertexes]
                if not zs:
                    continue
                if (max(zs) - min(zs)) < _FLAT_UI_TOL:
                    has_flat = True
                else:
                    has_3d = True
            except Exception:
                pass
    if has_flat and has_3d:
        return "mixed"
    if has_flat:
        return "flat"
    if has_3d:
        return "3d"
    return "none"


def _get_selection_wire_length(obj):
    """Return total arc length (mm) of all selected edges (conversion factor for % ↔ mm)."""
    if not hasattr(obj, "Base") or not obj.Base:
        return 0.0
    total = 0.0
    for base, subs in obj.Base:
        for sub in subs:
            if sub.startswith("Edge"):
                try:
                    total += base.Shape.getElement(sub).Length
                except Exception:
                    pass
    return total


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Task panel page for the Flute operation."""

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpFluteEdit.ui")

    def initPage(self, obj):
        self.setTitle("Flute - " + obj.Label)

        self.axialStockToLeaveSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.axialStockToLeave, obj, "AxialStockToLeave"
        )
        self.rampLengthSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.rampLength, obj, "RampLength"
        )

        self.form.flutingType.addItems(["Ramp Full", "Ramp Start", "Ramp Start End"])
        self.form.rampType.addItems(["Linear", "Smooth", "Arc"])

        # Two-way sync between Ramp Length (stored) and Ramp % (derived display).
        # Mirrors Adaptive's stepOver ↔ stepOverDistance pattern.
        # Connected here (before FreeCAD registers getSignalsForUpdate slots) so
        # our slots fire first — the length widget is correct before getFields reads it.
        self.form.rampLength.editingFinished.connect(lambda: self.updateRampPercent(obj))
        self.form.rampPercent.valueChanged.connect(lambda: self.updateRampLength(obj))

    def updateRampPercent(self, obj):
        """Ramp Length changed — update Ramp % display."""
        wire_len = _get_selection_wire_length(obj)
        if wire_len <= 0:
            return
        length_mm = self.form.rampLength.property("rawValue")
        if length_mm and length_mm > 0:
            pct = max(1, min(100, int(round(length_mm / wire_len * 100))))
            self.form.rampPercent.blockSignals(True)
            self.form.rampPercent.setValue(pct)
            self.form.rampPercent.blockSignals(False)

    def updateRampLength(self, obj):
        """Ramp % changed — update Ramp Length display so getFields reads the right value."""
        wire_len = _get_selection_wire_length(obj)
        if wire_len <= 0:
            return
        pct = self.form.rampPercent.value()
        length_mm = wire_len * pct / 100.0
        self.form.rampLength.blockSignals(True)
        self.form.rampLength.setProperty("rawValue", length_mm)
        self.form.rampLength.blockSignals(False)

    def setFields(self, obj):
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        self.form.reverseDirection.setCheckState(
            QtCore.Qt.Checked if obj.ReverseDirection else QtCore.Qt.Unchecked
        )
        self.form.blindEndCompensation.setCheckState(
            QtCore.Qt.Checked if obj.BlindEndCompensation else QtCore.Qt.Unchecked
        )

        self.axialStockToLeaveSpinBox.updateWidget()

        # 2D properties
        flute_type = getattr(obj, "FlutingType", "Ramp Full")
        idx = self.form.flutingType.findText(flute_type)
        if idx >= 0:
            self.form.flutingType.setCurrentIndex(idx)

        ramp_type = getattr(obj, "RampType", "Linear")
        idx = self.form.rampType.findText(ramp_type)
        if idx >= 0:
            self.form.rampType.setCurrentIndex(idx)

        # Load RampLength; then derive % from it.
        # Load length; derive % from it. If length is 0, default % display to 100.
        self.rampLengthSpinBox.updateWidget()
        length_mm = getattr(obj, "RampLength", None)
        length_mm = length_mm.Value if length_mm is not None else 0.0

        if length_mm > 0:
            self.updateRampPercent(obj)
        else:
            self.form.rampPercent.blockSignals(True)
            self.form.rampPercent.setValue(100)
            self.form.rampPercent.blockSignals(False)

        flip = getattr(obj, "FlipStart2D", False)
        self.form.flipStart2D.setCheckState(QtCore.Qt.Checked if flip else QtCore.Qt.Unchecked)

        self._update_2d_visibility(obj)

    def getFields(self, obj):
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        obj.ReverseDirection = self.form.reverseDirection.isChecked()
        obj.BlindEndCompensation = self.form.blindEndCompensation.isChecked()
        self.axialStockToLeaveSpinBox.updateProperty()

        if hasattr(obj, "FlutingType"):
            obj.FlutingType = self.form.flutingType.currentText()
        if hasattr(obj, "RampType"):
            obj.RampType = self.form.rampType.currentText()
        self.rampLengthSpinBox.updateProperty()
        if hasattr(obj, "FlipStart2D"):
            obj.FlipStart2D = self.form.flipStart2D.isChecked()

    def getSignalsForUpdate(self, obj):
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)

        for checkbox in (self.form.reverseDirection, self.form.blindEndCompensation):
            if hasattr(checkbox, "checkStateChanged"):
                signals.append(checkbox.checkStateChanged)
            else:
                signals.append(checkbox.stateChanged)

        signals.append(self.form.axialStockToLeave.editingFinished)
        # rampLength.editingFinished triggers recompute via FreeCAD machinery.
        # rampPercent.valueChanged is also registered so changing % triggers recompute
        # (our updateRampLength slot runs first to sync the length widget).
        signals.append(self.form.rampLength.editingFinished)
        signals.append(self.form.flutingType.currentIndexChanged)
        signals.append(self.form.rampType.currentIndexChanged)
        signals.append(self.form.rampPercent.valueChanged)
        if hasattr(self.form.flipStart2D, "checkStateChanged"):
            signals.append(self.form.flipStart2D.checkStateChanged)
        else:
            signals.append(self.form.flipStart2D.stateChanged)

        return signals

    def _update_2d_visibility(self, obj):
        """Show or hide 2D controls and property panel entries based on selection."""
        kind = _classify_wire_selection(obj)
        show_2d = kind == "flat"

        if kind == "mixed":
            Path.Log.warning(
                "Flute: mixed 2D (flat) and 3D wires selected. "
                "Use separate operations for each type.\n"
            )

        self.form.flute2DOptions.setVisible(show_2d)

        mode = 0 if show_2d else 2
        for prop in ("FlutingType", "RampType", "RampLength", "FlipStart2D"):
            if hasattr(obj, prop):
                obj.setEditorMode(prop, mode)


Command = PathOpGui.SetupOperation(
    "Flute",
    PathFlute.Create,
    TaskPanelOpPage,
    "CAM_Slot",  # placeholder icon until CAM_Flute.svg is created
    QtCore.QT_TRANSLATE_NOOP("CAM_Flute", "Flute"),
    QtCore.QT_TRANSLATE_NOOP(
        "CAM_Flute",
        "Create a ramping flute toolpath from a selected bottom face."
        "\n\nThe path runs along the centerline of the face, stepping down"
        "\nincrementally.  Supported tool types: flat, bull-nose, V-bit.",
    ),
    PathFlute.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathFluteGui... done\n")
