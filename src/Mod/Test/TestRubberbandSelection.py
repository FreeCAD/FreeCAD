# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors
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

"""GUI regression tests for drag rubberband selection.

To run tests:
    FreeCAD -t TestRubberbandSelection.TestRubberbandSelection
"""

import time
import unittest

import FreeCAD
import FreeCADGui
import Part
from PySide6 import QtCore, QtGui, QtWidgets

LEFT_BUTTON = QtCore.Qt.MouseButton.LeftButton
NO_BUTTON = QtCore.Qt.MouseButton.NoButton
NO_MODIFIER = QtCore.Qt.KeyboardModifier.NoModifier
CONTROL_MODIFIER = QtCore.Qt.KeyboardModifier.ControlModifier
SHIFT_MODIFIER = QtCore.Qt.KeyboardModifier.ShiftModifier
OTHER_FOCUS_REASON = QtCore.Qt.FocusReason.OtherFocusReason
MOUSE_MOVE = QtCore.QEvent.Type.MouseMove
MOUSE_PRESS = QtCore.QEvent.Type.MouseButtonPress
MOUSE_RELEASE = QtCore.QEvent.Type.MouseButtonRelease


class TestRubberbandSelection(unittest.TestCase):
    PLAIN_DRAG_STYLES = (
        ("CAD", "Gui::CADNavigationStyle"),
        ("OpenCascade", "Gui::OpenCascadeNavigationStyle"),
        ("Blender", "Gui::BlenderNavigationStyle"),
        ("Revit", "Gui::RevitNavigationStyle"),
        ("SolidWorks", "Gui::SolidWorksNavigationStyle"),
        ("TinkerCAD", "Gui::TinkerCADNavigationStyle"),
    )
    ADDITIVE_DRAG_STYLES = (
        ("CAD", "Gui::CADNavigationStyle"),
        ("Blender", "Gui::BlenderNavigationStyle"),
        ("Revit", "Gui::RevitNavigationStyle"),
        ("SolidWorks", "Gui::SolidWorksNavigationStyle"),
        ("TinkerCAD", "Gui::TinkerCADNavigationStyle"),
    )
    MODIFIED_DRAG_STYLES = (
        ("Gesture", "Gui::GestureNavigationStyle"),
        ("MayaGesture", "Gui::MayaGestureNavigationStyle"),
    )

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestRubberbandSelection")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)

        self.left_box = self.doc.addObject("Part::Feature", "LeftBox")
        self.left_box.Shape = Part.makeBox(6, 6, 6, FreeCAD.Vector(-20, -3, 0))

        self.right_box = self.doc.addObject("Part::Feature", "RightBox")
        self.right_box.Shape = Part.makeBox(6, 6, 6, FreeCAD.Vector(20, -3, 0))

        self.doc.recompute()

        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.viewer = self.view.getViewer()
        self.graphics_view = self.view.graphicsView()
        self.viewport = self.graphics_view.viewport()

        self.viewer.setEnabledNaviCube(False)
        self.view.setAxisCross(False)
        self._refresh_view()

        left_rect = self._object_rect(self.left_box)
        right_rect = self._object_rect(self.right_box)
        self.assertLess(
            left_rect.right(),
            right_rect.left(),
            "Test objects must not overlap in screen space",
        )

    def tearDown(self):
        FreeCADGui.Selection.clearSelection()
        FreeCAD.closeDocument(self.doc.Name)

    def test_plain_drag_selects_in_supported_styles(self):
        for label, style in self.PLAIN_DRAG_STYLES:
            with self.subTest(style=label):
                FreeCADGui.Selection.clearSelection()
                self.view.setNavigationType(style)
                self._refresh_view()

                self._drag_select(self._selection_rect(self.left_box))
                self.assertEqual(self._selected_names(), {self.left_box.Name})

    def test_ctrl_drag_adds_in_supported_styles(self):
        for label, style in self.ADDITIVE_DRAG_STYLES:
            with self.subTest(style=label):
                FreeCADGui.Selection.clearSelection()
                FreeCADGui.Selection.addSelection(self.doc.Name, self.right_box.Name)
                self.view.setNavigationType(style)
                self._refresh_view()

                self._drag_select(self._selection_rect(self.left_box), control=True)
                self.assertEqual(
                    self._selected_names(),
                    {self.left_box.Name, self.right_box.Name},
                )

    def test_shift_drag_selects_in_modified_styles(self):
        for label, style in self.MODIFIED_DRAG_STYLES:
            with self.subTest(style=label):
                FreeCADGui.Selection.clearSelection()
                self.view.setNavigationType(style)
                self._refresh_view()

                self._drag_select(self._selection_rect(self.left_box), shift=True)
                self.assertEqual(self._selected_names(), {self.left_box.Name})

    def test_shift_ctrl_drag_adds_in_modified_styles(self):
        for label, style in self.MODIFIED_DRAG_STYLES:
            with self.subTest(style=label):
                FreeCADGui.Selection.clearSelection()
                FreeCADGui.Selection.addSelection(self.doc.Name, self.right_box.Name)
                self.view.setNavigationType(style)
                self._refresh_view()

                self._drag_select(self._selection_rect(self.left_box), control=True, shift=True)
                self.assertEqual(
                    self._selected_names(),
                    {self.left_box.Name, self.right_box.Name},
                )

    def _refresh_view(self):
        self.view.viewIsometric()
        self.view.fitAll()
        self.viewport.setFocus(OTHER_FOCUS_REASON)
        self._process_events()
        self._process_events()

    def _process_events(self, wait_ms=50):
        FreeCADGui.updateGui()
        app = QtWidgets.QApplication.instance()
        app.processEvents()
        time.sleep(wait_ms / 1000.0)
        app.processEvents()

    def _device_pixel_ratio(self):
        if hasattr(self.viewport, "devicePixelRatioF"):
            return self.viewport.devicePixelRatioF()
        return float(self.viewport.devicePixelRatio())

    def _to_qpoint(self, point):
        _, height = self.view.getSize()
        scale = self._device_pixel_ratio()
        x = int(round(point[0] / scale))
        y = int(round((height - point[1] - 1) / scale))
        return QtCore.QPoint(x, y)

    def _object_rect(self, obj, pad=0):
        bound_box = obj.Shape.BoundBox
        corners = [
            FreeCAD.Vector(x, y, z)
            for x in (bound_box.XMin, bound_box.XMax)
            for y in (bound_box.YMin, bound_box.YMax)
            for z in (bound_box.ZMin, bound_box.ZMax)
        ]
        points = [self._to_qpoint(self.view.getPointOnViewport(corner)) for corner in corners]

        bounds = self.viewport.rect().adjusted(2, 2, -3, -3)
        left = max(bounds.left(), min(point.x() for point in points) - pad)
        top = max(bounds.top(), min(point.y() for point in points) - pad)
        right = min(bounds.right(), max(point.x() for point in points) + pad)
        bottom = min(bounds.bottom(), max(point.y() for point in points) + pad)
        return QtCore.QRect(QtCore.QPoint(left, top), QtCore.QPoint(right, bottom))

    def _selection_rect(self, obj):
        return self._object_rect(obj, pad=18)

    def _drag_select(self, rect, control=False, shift=False):
        start = rect.topLeft()
        center = rect.center()
        end = rect.bottomRight()
        modifiers = NO_MODIFIER
        if control:
            modifiers |= CONTROL_MODIFIER
        if shift:
            modifiers |= SHIFT_MODIFIER

        self._send_mouse_event(MOUSE_MOVE, start, NO_BUTTON, NO_BUTTON, modifiers)
        self._process_events(10)
        self._send_mouse_event(MOUSE_PRESS, start, LEFT_BUTTON, LEFT_BUTTON, modifiers)
        self._process_events(10)
        self._send_mouse_event(MOUSE_MOVE, center, NO_BUTTON, LEFT_BUTTON, modifiers)
        self._process_events(10)
        self._send_mouse_event(MOUSE_MOVE, end, NO_BUTTON, LEFT_BUTTON, modifiers)
        self._process_events(10)
        self._send_mouse_event(MOUSE_RELEASE, end, LEFT_BUTTON, NO_BUTTON, modifiers)
        self._process_events()

    def _selected_names(self):
        return {obj.Name for obj in FreeCADGui.Selection.getSelection()}

    def _send_mouse_event(self, event_type, pos, button, buttons, modifiers):
        app = QtWidgets.QApplication.instance()
        global_pos = self.viewport.mapToGlobal(pos)
        QtGui.QCursor.setPos(global_pos)
        event = QtGui.QMouseEvent(
            event_type,
            QtCore.QPointF(pos),
            QtCore.QPointF(global_pos),
            button,
            buttons,
            modifiers,
        )

        app.sendEvent(self.viewport, event)
