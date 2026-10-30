# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Jesse Trutna <jesse@trutna.com>                    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Spiral tool for sketch edit mode (Sketcher_CreateSpiral).

Sketches have no spiral geometry, so the spiral is a cubic B-spline interpolated
through points on the exact curve. A floating panel sets the type and size with a
live preview; clicking a sketch point moves the center there. The result is fixed
in place with a Block ("Fix") constraint, since its control points would otherwise
leave the sketch under-constrained.
"""

import math
import os

import FreeCAD
import FreeCADGui
import Part
import Sketcher
from PySide import QtCore, QtWidgets

translate = FreeCAD.Qt.translate

PARAM_PATH = "User parameter:BaseApp/Preferences/Mod/Sketcher/Spiral"
ICON = os.path.join(os.path.dirname(__file__), "Sketcher_CreateSpiral.svg")
POINTS_PER_TURN = 48

ARCHIMEDEAN, LOGARITHMIC, FERMAT, INVOLUTE = range(4)
TYPES = [
    (QtCore.QT_TRANSLATE_NOOP("Sketcher_CreateSpiral", "Archimedean"),
     QtCore.QT_TRANSLATE_NOOP("Sketcher_CreateSpiral", "Even spacing between turns")),
    (QtCore.QT_TRANSLATE_NOOP("Sketcher_CreateSpiral", "Logarithmic"),
     QtCore.QT_TRANSLATE_NOOP("Sketcher_CreateSpiral", "Spacing grows with each turn")),
    (QtCore.QT_TRANSLATE_NOOP("Sketcher_CreateSpiral", "Fermat"),
     QtCore.QT_TRANSLATE_NOOP("Sketcher_CreateSpiral", "Spacing shrinks outwards (equal area per turn)")),
    (QtCore.QT_TRANSLATE_NOOP("Sketcher_CreateSpiral", "Involute"),
     QtCore.QT_TRANSLATE_NOOP("Sketcher_CreateSpiral", "Unwound from a base circle, as on gear teeth")),
]


def spiral_points(kind, turns, inner, outer, start_angle, clockwise, center):
    """Points on the spiral, or raise ValueError with a message for the user."""
    if turns <= 0:
        raise ValueError(translate("Sketcher_CreateSpiral", "Turns must be more than 0"))
    sweep = 2 * math.pi * turns
    sign = -1 if clockwise else 1
    count = max(24, int(POINTS_PER_TURN * turns))
    cos0, sin0 = math.cos(start_angle), math.sin(start_angle)

    if kind == INVOLUTE:
        if inner <= 0:
            raise ValueError(translate("Sketcher_CreateSpiral", "Base radius must be more than 0"))
    elif outer <= 0:
        raise ValueError(translate("Sketcher_CreateSpiral", "Outer radius must be more than 0"))
    elif kind == LOGARITHMIC and inner <= 0:
        raise ValueError(
            translate("Sketcher_CreateSpiral", "A logarithmic spiral needs an inner radius above 0")
        )
    elif inner < 0:
        raise ValueError(translate("Sketcher_CreateSpiral", "Inner radius can't be negative"))

    points = []
    for i in range(count + 1):
        theta = sweep * i / count
        f = i / count
        if kind == INVOLUTE:
            # Involute of a circle of radius `inner`, then rotated to the start angle.
            x = inner * (math.cos(theta) + theta * math.sin(theta))
            y = sign * inner * (math.sin(theta) - theta * math.cos(theta))
            px, py = x * cos0 - y * sin0, x * sin0 + y * cos0
        else:
            if kind == ARCHIMEDEAN:
                r = inner + (outer - inner) * f
            elif kind == LOGARITHMIC:
                r = inner * (outer / inner) ** f
            else:  # FERMAT: r² grows linearly with the angle
                r = math.sqrt(inner**2 + (outer**2 - inner**2) * f)
            phi = start_angle + sign * theta
            px, py = r * math.cos(phi), r * math.sin(phi)
        points.append(FreeCAD.Vector(center.x + px, center.y + py, 0))
    return points


def spiral_curve(points):
    """Cubic B-spline through the points' ends, within 0.01 % of the spiral's size elsewhere.

    Approximating needs about a quarter of the control points of interpolating every point
    (~50 instead of ~200 for 4 turns), for about 1 µm deviation on a 10 mm spiral.
    """
    center = (points[0] + points[-1]) * 0.5
    size = max((p - center).Length for p in points)
    curve = Part.BSplineCurve()
    try:
        curve.approximate(
            Points=points, DegMin=3, DegMax=3, Tolerance=1e-4 * size, Continuity="C2"
        )
    except Part.OCCError:
        curve.interpolate(points)
    return curve


def involute_outer_radius(base, turns):
    sweep = 2 * math.pi * turns
    return base * math.sqrt(1 + sweep * sweep)


def _length_box(value):
    box = QtWidgets.QDoubleSpinBox()
    box.setRange(-1e6, 1e6)
    box.setDecimals(3)
    box.setSuffix(" mm")
    box.setValue(value)
    box.setKeyboardTracking(False)
    return box


class SpiralPanel(QtWidgets.QDialog):
    """Floating parameter panel with live preview, like Fusion's sketch tool dialogs."""

    def __init__(self, sketch):
        super().__init__(FreeCADGui.getMainWindow(), QtCore.Qt.Tool)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.setWindowTitle(translate("Sketcher_CreateSpiral", "Spiral"))
        self.sketch = sketch
        self.doc = sketch.Document
        self.geo = None
        self.closed = False
        params = FreeCAD.ParamGet(PARAM_PATH)

        self.kind = QtWidgets.QComboBox()
        for name, tip in TYPES:
            self.kind.addItem(translate("Sketcher_CreateSpiral", name))
            self.kind.setItemData(self.kind.count() - 1, translate("Sketcher_CreateSpiral", tip),
                                  QtCore.Qt.ToolTipRole)
        self.kind.setCurrentIndex(params.GetInt("Type", ARCHIMEDEAN))
        self.turns = QtWidgets.QDoubleSpinBox()
        self.turns.setRange(0.05, 1000)
        self.turns.setSingleStep(0.25)
        self.turns.setDecimals(2)
        self.turns.setValue(params.GetFloat("Turns", 3))
        self.turns.setKeyboardTracking(False)
        self.inner = _length_box(params.GetFloat("InnerRadius", 1))
        self.inner_label = QtWidgets.QLabel()
        self.outer = _length_box(params.GetFloat("OuterRadius", 10))
        self.start = QtWidgets.QDoubleSpinBox()
        self.start.setRange(-360, 360)
        self.start.setSuffix(" °")
        self.start.setValue(params.GetFloat("StartAngle", 0))
        self.start.setKeyboardTracking(False)
        self.direction = QtWidgets.QComboBox()
        self.direction.addItems([translate("Sketcher_CreateSpiral", "Counter-clockwise"),
                                 translate("Sketcher_CreateSpiral", "Clockwise")])
        self.direction.setCurrentIndex(1 if params.GetBool("Clockwise", False) else 0)
        self.cx = _length_box(0)
        self.cy = _length_box(0)
        center_row = QtWidgets.QHBoxLayout()
        center_row.addWidget(self.cx)
        center_row.addWidget(self.cy)
        self.message = QtWidgets.QLabel()
        self.message.setWordWrap(True)
        self.message.setStyleSheet("color: #c0392b;")
        hint = QtWidgets.QLabel(
            translate("Sketcher_CreateSpiral", "Click a sketch point to put the center there.")
        )
        hint.setWordWrap(True)
        hint.setEnabled(False)

        form = QtWidgets.QFormLayout()
        form.addRow(translate("Sketcher_CreateSpiral", "Type"), self.kind)
        form.addRow(translate("Sketcher_CreateSpiral", "Turns"), self.turns)
        form.addRow(self.inner_label, self.inner)
        form.addRow(translate("Sketcher_CreateSpiral", "Outer radius"), self.outer)
        form.addRow(translate("Sketcher_CreateSpiral", "Start angle"), self.start)
        form.addRow(translate("Sketcher_CreateSpiral", "Direction"), self.direction)
        form.addRow(translate("Sketcher_CreateSpiral", "Center X, Y"), center_row)
        buttons = QtWidgets.QDialogButtonBox(
            QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel
        )
        layout = QtWidgets.QVBoxLayout(self)
        layout.addLayout(form)
        layout.addWidget(hint)
        layout.addWidget(self.message)
        layout.addWidget(buttons)
        self.ok_button = buttons.button(QtWidgets.QDialogButtonBox.Ok)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)

        selected = self._selected_point()
        if selected is not None:
            self.cx.setValue(selected.x)
            self.cy.setValue(selected.y)

        self.doc.openTransaction("Create spiral")
        for box in (self.turns, self.inner, self.outer, self.start, self.cx, self.cy):
            box.valueChanged.connect(self.update_preview)
        for combo in (self.kind, self.direction):
            combo.currentIndexChanged.connect(self.update_preview)
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addObserver(self)
        # Leaving sketch edit (or closing the document) cancels the spiral.
        self.watch = QtCore.QTimer(self)
        self.watch.timeout.connect(self._check_still_editing)
        self.watch.start(500)
        self.update_preview()

    def _selected_point(self):
        for sel in FreeCADGui.Selection.getSelectionEx():
            if sel.Object == self.sketch:
                for sub in sel.SubElementNames:
                    point = self._point_of(sub)
                    if point is not None:
                        return point
        return None

    def _point_of(self, sub):
        if sub == "RootPoint":
            return FreeCAD.Vector(0, 0, 0)
        if sub.startswith("Vertex"):
            try:
                geo_id, pos = self.sketch.getGeoVertexIndex(int(sub[6:]) - 1)
            except (ValueError, IndexError):
                return None
            if geo_id == self.geo:
                return None  # the spiral's own end points
            return self.sketch.getPoint(geo_id, pos)
        return None

    # Selection observer: clicking a sketch point moves the center there.
    def addSelection(self, doc, obj, sub, pnt):
        if self.closed or obj != self.sketch.Name:
            return
        point = self._point_of(sub)
        if point is not None:
            self.cx.setValue(point.x)
            self.cy.setValue(point.y)
        QtCore.QTimer.singleShot(0, FreeCADGui.Selection.clearSelection)

    def _values(self):
        return (
            self.kind.currentIndex(),
            self.turns.value(),
            self.inner.value(),
            self.outer.value(),
            math.radians(self.start.value()),
            self.direction.currentIndex() == 1,
            FreeCAD.Vector(self.cx.value(), self.cy.value(), 0),
        )

    def update_preview(self, *_):
        kind = self.kind.currentIndex()
        involute = kind == INVOLUTE
        self.inner_label.setText(
            translate("Sketcher_CreateSpiral", "Base radius")
            if involute
            else translate("Sketcher_CreateSpiral", "Inner radius")
        )
        self.outer.setEnabled(not involute)
        if involute:
            self.outer.blockSignals(True)
            self.outer.setValue(involute_outer_radius(self.inner.value(), self.turns.value()))
            self.outer.blockSignals(False)

        if self.geo is not None:
            self.sketch.delGeometry(self.geo)
            self.geo = None
        try:
            curve = spiral_curve(spiral_points(*self._values()))
        except ValueError as e:
            self.message.setText(str(e))
            self.message.show()
            self.ok_button.setEnabled(False)
            return
        except Part.OCCError:
            self.message.setText(translate("Sketcher_CreateSpiral", "This spiral can't be built"))
            self.message.show()
            self.ok_button.setEnabled(False)
            return
        self.message.hide()
        self.ok_button.setEnabled(True)
        self.geo = self.sketch.addGeometry(curve, False)
        self.sketch.solve()

    def _check_still_editing(self):
        gui_doc = FreeCADGui.getDocument(self.doc.Name) if self.doc in FreeCAD.listDocuments().values() else None
        vp = gui_doc.getInEdit() if gui_doc else None
        if not vp or vp.Object != self.sketch:
            self.reject()

    def _finish(self):
        self.closed = True
        self.watch.stop()
        FreeCADGui.Selection.removeObserver(self)
        params = FreeCAD.ParamGet(PARAM_PATH)
        params.SetInt("Type", self.kind.currentIndex())
        params.SetFloat("Turns", self.turns.value())
        params.SetFloat("InnerRadius", self.inner.value())
        params.SetFloat("OuterRadius", self.outer.value())
        params.SetFloat("StartAngle", self.start.value())
        params.SetBool("Clockwise", self.direction.currentIndex() == 1)

    def accept(self):
        if self.closed or self.geo is None:
            return
        self._finish()
        # Without internal control-point geometry the spline would have free degrees of freedom.
        self.sketch.addConstraint(Sketcher.Constraint("Block", self.geo))
        self.doc.commitTransaction()
        self.sketch.solve()
        super().accept()

    def reject(self):
        if self.closed:
            return
        self._finish()
        try:
            self.doc.abortTransaction()
            self.sketch.solve()
        except Exception:
            pass  # document already closed
        super().reject()

    def place(self):
        """Top-right of the 3D view, out of the way of the sketch."""
        self.adjustSize()
        mdi = FreeCADGui.getMainWindow().centralWidget()
        corner = mdi.mapToGlobal(QtCore.QPoint(mdi.width() - self.width() - 16, 16))
        self.move(corner)


_panel = None


class _CreateSpiral:
    def GetResources(self):
        return {
            "Pixmap": ICON,
            "MenuText": QtCore.QT_TRANSLATE_NOOP("Sketcher_CreateSpiral", "Spiral"),
            "ToolTip": QtCore.QT_TRANSLATE_NOOP(
                "Sketcher_CreateSpiral",
                "Creates a spiral: Archimedean, logarithmic, Fermat or involute",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        doc = FreeCADGui.ActiveDocument
        vp = doc.getInEdit() if doc else None
        return bool(vp) and vp.Object.isDerivedFrom("Sketcher::SketchObject") and not _panel_open()

    def Activated(self):
        global _panel
        # Look the module up at call time so a reloaded module takes effect.
        import SketcherSpiral

        sketch = FreeCADGui.ActiveDocument.getInEdit().Object
        _panel = SketcherSpiral.SpiralPanel(sketch)
        _panel.place()
        _panel.show()
        _panel.turns.setFocus()
        _panel.turns.selectAll()


def _panel_open():
    try:
        return _panel is not None and _panel.isVisible()
    except RuntimeError:  # deleted on close
        return False


class _MenuPlacement:
    """Puts Spiral in the sketch Geometries menu, after the B-splines.

    Besides being findable there, a command only gets the action that the S search
    and the toolbar need once it is placed in a menu or toolbar.
    """

    def modifyMenuBar(self):
        # "append" adds to the end of the menu holding menuItem ("insert ... after" does nothing
        # when menuItem is the menu's last entry, as this one is).
        return [
            {
                "append": "Sketcher_CreateSpiral",
                "menuItem": "Sketcher_CreatePeriodicBSplineByInterpolation",
            }
        ]


if "Sketcher_CreateSpiral" not in FreeCADGui.listCommands():
    FreeCADGui.addCommand("Sketcher_CreateSpiral", _CreateSpiral())
    FreeCADGui.addWorkbenchManipulator(_MenuPlacement())
