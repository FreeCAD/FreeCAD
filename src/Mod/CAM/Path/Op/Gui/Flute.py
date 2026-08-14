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
# Coarser than the tol.edge used at generate time — we only need to decide
# whether to show the 2D controls, not whether to actually synthesize a profile.
_FLAT_UI_TOL = 0.01


def _classify_wire_selection(obj):
    """Classify the edge selection as "flat", "3d", "mixed", or "none".

    "flat"  — all selected edges lie at essentially constant Z
    "3d"    — all selected edges carry Z variation
    "mixed" — both types present (not allowed in one operation)
    "none"  — no edges selected
    """
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
    """Return total arc length (mm) of all selected edges.

    Used to convert between RampLength (mm) and RampPercent (%) in the UI.
    Uses shape.Length (exact) rather than discretizing — close enough for display.
    """
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

        # Two-way sync: connect before FreeCAD registers getSignalsForUpdate so
        # our slots fire first and the widget values are consistent when getFields
        # reads them.  blockSignals() in each slot prevents infinite loops.
        self.form.rampLength.editingFinished.connect(self._sync_pct_from_length)
        self.form.rampPercent.valueChanged.connect(self._sync_length_from_pct)

        # Stash obj so the sync slots can reach it.
        self._flute_obj = obj

    def _sync_pct_from_length(self):
        """User finished editing Ramp Length — update Ramp % display."""
        obj = getattr(self, "_flute_obj", None)
        if obj is None:
            return
        wire_len = _get_selection_wire_length(obj)
        if wire_len <= 0:
            return
        try:
            length_mm = self.form.rampLength.value().Value
        except Exception:
            return
        if length_mm <= 0:
            return
        pct = max(1, min(100, int(round(length_mm / wire_len * 100))))
        self.form.rampPercent.blockSignals(True)
        self.form.rampPercent.setValue(pct)
        self.form.rampPercent.blockSignals(False)

    def _sync_length_from_pct(self, pct):
        """User changed Ramp % — update Ramp Length display."""
        obj = getattr(self, "_flute_obj", None)
        if obj is None:
            return
        wire_len = _get_selection_wire_length(obj)
        if wire_len <= 0:
            return
        length_mm = wire_len * pct / 100.0
        q = FreeCAD.Units.Quantity("{} mm".format(length_mm))
        self.form.rampLength.blockSignals(True)
        self.form.rampLength.setValue(q)
        self.form.rampLength.blockSignals(False)

    def setFields(self, obj):
        self._flute_obj = obj

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
        flute_type = getattr(obj, "FlutingType", "RampFull")
        idx = self.form.flutingType.findText(flute_type)
        if idx >= 0:
            self.form.flutingType.setCurrentIndex(idx)

        ramp_type = getattr(obj, "RampType", "Linear")
        idx = self.form.rampType.findText(ramp_type)
        if idx >= 0:
            self.form.rampType.setCurrentIndex(idx)

        # Show RampLength widget first, then derive RampPercent from it.
        # If RampLength is 0, fall back to the stored RampPercent.
        self.rampLengthSpinBox.updateWidget()

        length_mm = getattr(obj, "RampLength", None)
        length_mm = length_mm.Value if length_mm is not None else 0.0
        wire_len = _get_selection_wire_length(obj)

        self.form.rampPercent.blockSignals(True)
        if length_mm > 0 and wire_len > 0:
            pct = max(1, min(100, int(round(length_mm / wire_len * 100))))
            self.form.rampPercent.setValue(pct)
        else:
            self.form.rampPercent.setValue(int(getattr(obj, "RampPercent", 100)))
        self.form.rampPercent.blockSignals(False)

        flip = getattr(obj, "FlipStart2D", False)
        self.form.flipStart2D.setCheckState(
            QtCore.Qt.Checked if flip else QtCore.Qt.Unchecked
        )

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
        # RampLength is authoritative; RampPercent kept in sync for storage.
        self.rampLengthSpinBox.updateProperty()
        if hasattr(obj, "RampPercent"):
            obj.RampPercent = self.form.rampPercent.value()
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

        # Mirror visibility into the data panel properties so they don't clutter
        # the tree when 2D mode isn't relevant.  2 = hidden, 0 = normal.
        mode = 0 if show_2d else 2
        for prop in ("FlutingType", "RampType", "RampLength", "RampPercent", "FlipStart2D"):
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
