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

from PySide import QtCore, QtGui

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


class TaskPanelBaseGeometryPage(PathOpGui.TaskPanelBaseGeometryPage):
    """Base Geometry page with an added per-edge "flip direction" checkbox.

    Flipping is independent of FlipStart2D (which reverses the whole result
    at the end) -- this overrides how one individual edge was drawn, before
    the tangent-continuity chaining logic decides overall path direction.
    """

    def setFields(self, obj):
        super(TaskPanelBaseGeometryPage, self).setFields(obj)
        flipped_keys = set(getattr(obj, "FlippedSegments", None) or [])
        show_flip = _classify_wire_selection(obj) == "flat"
        self.form.baseList.blockSignals(True)
        for i in range(self.form.baseList.count()):
            item = self.form.baseList.item(i)
            base = item.data(self.DataObject)
            sub = item.data(self.DataObjectSub)
            if show_flip and sub and str(sub).startswith("Edge"):
                item.setFlags(item.flags() | QtCore.Qt.ItemIsUserCheckable)
                item.setToolTip(
                    translate(
                        "CAM_Flute",
                        "Force-reverse this segment's direction (2D wires only).",
                    )
                )
                key = "{}.{}".format(base.Name, sub)
                item.setCheckState(
                    QtCore.Qt.Checked if key in flipped_keys else QtCore.Qt.Unchecked
                )
            else:
                item.setFlags(item.flags() & ~QtCore.Qt.ItemIsUserCheckable)
        self.form.baseList.blockSignals(False)

    def registerSignalHandlers(self, obj):
        super(TaskPanelBaseGeometryPage, self).registerSignalHandlers(obj)
        self.form.baseList.itemChanged.connect(self.updateFlippedSegments)

    def updateFlippedSegments(self, item=None):
        flipped = []
        for i in range(self.form.baseList.count()):
            it = self.form.baseList.item(i)
            if it.checkState() == QtCore.Qt.Checked:
                base = it.data(self.DataObject)
                sub = it.data(self.DataObjectSub)
                if base and sub:
                    flipped.append("{}.{}".format(base.Name, sub))
        self.obj.FlippedSegments = flipped
        self.setDirty()

    def deleteBase(self):
        super(TaskPanelBaseGeometryPage, self).deleteBase()
        self.updateFlippedSegments()

    def clearBase(self):
        super(TaskPanelBaseGeometryPage, self).clearBase()
        self.obj.FlippedSegments = []


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Task panel page for the Flute operation."""

    def taskPanelBaseGeometryPage(self, obj, features):
        return TaskPanelBaseGeometryPage(obj, features)

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

        # Icon shows WHERE along the path the ramp happens (flat vs ramping
        # sections), distinct from RampType's icons which show the Z-profile
        # curve SHAPE.
        for text, icon_res in (
            ("Ramp Full", ":/icons/flute-fluting-ramp-full.svg"),
            ("Ramp Start", ":/icons/flute-fluting-ramp-start.svg"),
            ("Ramp Start End", ":/icons/flute-fluting-ramp-start-end.svg"),
        ):
            self.form.flutingType.addItem(QtGui.QIcon(icon_res), text)
        # QComboBox scales each item icon to fit within a single iconSize box
        # shared by every item, preserving aspect ratio -- without an explicit
        # wide iconSize here, these triple-width icons get squeezed down to
        # whatever narrow default box the style uses, same as rampType's.
        self.form.flutingType.setIconSize(QtCore.QSize(54, 16))

        # Each dropdown item gets a small curve-shape icon (no text baked in --
        # the item's own label already names it, and a bare colored line reads
        # fine in both light and dark themes, unlike small embedded text).
        for text, icon_res in (
            ("Linear", ":/icons/flute-ramp-linear.svg"),
            ("S-Curve", ":/icons/flute-ramp-s-curve.svg"),
            ("Smooth", ":/icons/flute-ramp-smooth.svg"),
            ("Fillet", ":/icons/flute-ramp-fillet.svg"),
        ):
            self.form.rampType.addItem(QtGui.QIcon(icon_res), text)
        self.form.rampType.setIconSize(QtCore.QSize(24, 16))
        self.form.rampLengthType.addItems(["Length", "Percent"])

        # Ramp Length and Ramp % are independent stored values (RampLength mm /
        # RampLengthPercent), not converted into each other.  RampLengthType selects
        # which one the backend actually uses; the other stays visible/hidden
        # but its last-entered value is preserved for convenience if the user
        # switches back.  The Properties-panel editor mode for RampLength /
        # RampLengthPercent is owned by the model's onChanged (Path.Op.Flute), so it
        # stays correct even when this task panel isn't open; here we only
        # need to update the task-panel row visibility.
        self.form.rampLengthType.currentIndexChanged.connect(
            lambda: self._update_ramp_length_type(obj)
        )

        # FlutingType drives three task-panel-only side effects: the Ramp %
        # spinbox's own max (mirrors the model's Properties-panel cap), and
        # graying out controls that have no effect for the current profile
        # (Ramp Length Type/Length/% under Ramp Full; Blind End Compensation
        # under Ramp Start End).  The model applies the equivalent
        # Properties-panel editor modes independently via onChanged.
        self.form.flutingType.currentIndexChanged.connect(
            lambda: self._update_fluting_type_dependent_controls()
        )

    def setFields(self, obj):
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        self.form.reverseDirection.setCheckState(
            QtCore.Qt.Checked if obj.ReverseDirection else QtCore.Qt.Unchecked
        )
        self.form.blindEndCompensation.setCheckState(
            QtCore.Qt.Checked if obj.BlindEndCompensation else QtCore.Qt.Unchecked
        )
        combine_tangent = getattr(obj, "CombineTangentSegments", True)
        self.form.combineTangentSegments.setCheckState(
            QtCore.Qt.Checked if combine_tangent else QtCore.Qt.Unchecked
        )

        self.axialStockToLeaveSpinBox.updateWidget()

        # 2D properties
        flute_type = getattr(obj, "FlutingType", "Ramp Full")
        idx = self.form.flutingType.findText(flute_type)
        if idx >= 0:
            self.form.flutingType.setCurrentIndex(idx)
        self._update_fluting_type_dependent_controls()

        ramp_type = getattr(obj, "RampType", "Linear")
        idx = self.form.rampType.findText(ramp_type)
        if idx >= 0:
            self.form.rampType.setCurrentIndex(idx)

        ramp_length_type = getattr(obj, "RampLengthType", "Length")
        idx = self.form.rampLengthType.findText(ramp_length_type)
        if idx >= 0:
            self.form.rampLengthType.setCurrentIndex(idx)

        self.rampLengthSpinBox.updateWidget()
        self.form.rampLengthPercent.setValue(int(getattr(obj, "RampLengthPercent", 100)))

        flip = getattr(obj, "FlipStart2D", False)
        self.form.flipStart2D.setCheckState(QtCore.Qt.Checked if flip else QtCore.Qt.Unchecked)

        self._update_2d_visibility(obj)

    def getFields(self, obj):
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        obj.ReverseDirection = self.form.reverseDirection.isChecked()
        obj.BlindEndCompensation = self.form.blindEndCompensation.isChecked()
        if hasattr(obj, "CombineTangentSegments"):
            obj.CombineTangentSegments = self.form.combineTangentSegments.isChecked()
        self.axialStockToLeaveSpinBox.updateProperty()

        if hasattr(obj, "FlutingType"):
            obj.FlutingType = self.form.flutingType.currentText()
        if hasattr(obj, "RampType"):
            obj.RampType = self.form.rampType.currentText()
        if hasattr(obj, "RampLengthType"):
            obj.RampLengthType = self.form.rampLengthType.currentText()
        self.rampLengthSpinBox.updateProperty()
        if hasattr(obj, "RampLengthPercent"):
            obj.RampLengthPercent = self.form.rampLengthPercent.value()
        if hasattr(obj, "FlipStart2D"):
            obj.FlipStart2D = self.form.flipStart2D.isChecked()

    def getSignalsForUpdate(self, obj):
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)

        for checkbox in (
            self.form.reverseDirection,
            self.form.blindEndCompensation,
            self.form.combineTangentSegments,
        ):
            if hasattr(checkbox, "checkStateChanged"):
                signals.append(checkbox.checkStateChanged)
            else:
                signals.append(checkbox.stateChanged)

        signals.append(self.form.axialStockToLeave.editingFinished)
        signals.append(self.form.rampLength.editingFinished)
        signals.append(self.form.flutingType.currentIndexChanged)
        signals.append(self.form.rampType.currentIndexChanged)
        signals.append(self.form.rampLengthType.currentIndexChanged)
        signals.append(self.form.rampLengthPercent.valueChanged)
        if hasattr(self.form.flipStart2D, "checkStateChanged"):
            signals.append(self.form.flipStart2D.checkStateChanged)
        else:
            signals.append(self.form.flipStart2D.stateChanged)

        return signals

    def _update_ramp_length_type(self, obj):
        """RampLengthType changed — update which task-panel row is shown, and
        let the model apply the Properties-panel editor mode (it reacts to
        this property change directly via onChanged, from any source)."""
        self._apply_ramp_length_type_row_visibility(_classify_wire_selection(obj) == "flat")

    def _update_fluting_type_dependent_controls(self):
        """Grey out task-panel controls that have no effect for the current
        FlutingType, and keep the Ramp % spinbox's own range in sync with the
        model's Properties-panel cap.  Mirrors the model's editor-mode logic
        (Path.Op.Flute.applyRampLengthTypeEditorMode /
        applyBlindEndCompensationEditorMode) so the task panel and Properties
        panel never disagree about what's currently usable.
        """
        flute_type = self.form.flutingType.currentText()

        # Ramp Start End ramps EACH side independently, so anything above 50%
        # would just overlap and clamp anyway -- capping the spinbox directly
        # avoids the false impression of a bug.
        max_pct = 50 if flute_type == "Ramp Start End" else 100
        self.form.rampLengthPercent.setMaximum(max_pct)
        if self.form.rampLengthPercent.value() > max_pct:
            self.form.rampLengthPercent.setValue(max_pct)

        # Ramp Length Type / Length / % have no effect at all under Ramp Full
        # (ramp_frac is never read in that profile).
        ramp_full = flute_type == "Ramp Full"
        for widget in (
            self.form.rampLengthType_label,
            self.form.rampLengthType,
            self.form.rampLength_label,
            self.form.rampLength,
            self.form.rampLengthPercent_label,
            self.form.rampLengthPercent,
        ):
            widget.setEnabled(not ramp_full)

        # Blind End Compensation has no effect under Ramp Start End, which
        # always ramps back to the surface (no blind end to compensate).
        self.form.blindEndCompensation.setEnabled(flute_type != "Ramp Start End")

    def _apply_ramp_length_type_row_visibility(self, show_2d):
        """Show only the task-panel row matching the current RampLengthType.
        Properties-panel visibility for RampLength/RampLengthPercent is owned by the
        model's onChanged, not duplicated here.
        """
        is_percent = self.form.rampLengthType.currentText() == "Percent"
        self.form.rampLength_label.setVisible(show_2d and not is_percent)
        self.form.rampLength.setVisible(show_2d and not is_percent)
        self.form.rampLengthPercent_label.setVisible(show_2d and is_percent)
        self.form.rampLengthPercent.setVisible(show_2d and is_percent)

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
        for prop in ("FlutingType", "RampType", "RampLengthType", "FlipStart2D"):
            if hasattr(obj, prop):
                obj.setEditorMode(prop, mode)

        # RampLength/RampLengthPercent: hide both outright when not a 2D (flat) wire
        # selection, regardless of RampLengthType.  When 2D, restore the
        # correct split by re-applying the model's RampLengthType-driven mode
        # (it may have been left at "both hidden" from a previous 3D state).
        if not show_2d:
            for prop in ("RampLength", "RampLengthPercent"):
                if hasattr(obj, prop):
                    obj.setEditorMode(prop, 2)
        elif hasattr(obj, "Proxy") and hasattr(obj.Proxy, "applyRampLengthTypeEditorMode"):
            obj.Proxy.applyRampLengthTypeEditorMode(obj)

        self._apply_ramp_length_type_row_visibility(show_2d)


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
