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
from pivy import coin

try:
    import Part
except ImportError:
    Part = None
from PySide import QtCore, QtGui

LEFT_BUTTON = QtCore.Qt.LeftButton
NO_BUTTON = QtCore.Qt.NoButton
NO_MODIFIER = QtCore.Qt.NoModifier
CONTROL_MODIFIER = QtCore.Qt.ControlModifier
SHIFT_MODIFIER = QtCore.Qt.ShiftModifier
OTHER_FOCUS_REASON = QtCore.Qt.OtherFocusReason
MOUSE_MOVE = QtCore.QEvent.MouseMove
MOUSE_PRESS = QtCore.QEvent.MouseButtonPress
MOUSE_RELEASE = QtCore.QEvent.MouseButtonRelease


class TestRubberbandSelection(unittest.TestCase):
    PLAIN_DRAG_STYLES = (
        ("CAD", "Gui::CADNavigationStyle"),
        ("OpenCascade", "Gui::OpenCascadeNavigationStyle"),
        ("Blender", "Gui::BlenderNavigationStyle"),
        ("Revit", "Gui::RevitNavigationStyle"),
        ("SolidWorks", "Gui::SolidWorksNavigationStyle"),
        ("TinkerCAD", "Gui::TinkerCADNavigationStyle"),
        ("Touchpad", "Gui::TouchpadNavigationStyle"),
    )
    ADDITIVE_DRAG_STYLES = (
        ("CAD", "Gui::CADNavigationStyle"),
        ("Blender", "Gui::BlenderNavigationStyle"),
        ("Revit", "Gui::RevitNavigationStyle"),
        ("SolidWorks", "Gui::SolidWorksNavigationStyle"),
        ("TinkerCAD", "Gui::TinkerCADNavigationStyle"),
        ("Touchpad", "Gui::TouchpadNavigationStyle"),
    )
    MODIFIED_DRAG_STYLES = (
        ("Gesture", "Gui::GestureNavigationStyle"),
        ("MayaGesture", "Gui::MayaGestureNavigationStyle"),
    )
    VIEWER_HANDOFF_STYLES = (
        ("CAD", "Gui::CADNavigationStyle"),
        ("Blender", "Gui::BlenderNavigationStyle"),
        ("OpenCascade", "Gui::OpenCascadeNavigationStyle"),
        ("Revit", "Gui::RevitNavigationStyle"),
        ("SolidWorks", "Gui::SolidWorksNavigationStyle"),
        ("TinkerCAD", "Gui::TinkerCADNavigationStyle"),
        ("Touchpad", "Gui::TouchpadNavigationStyle"),
    )

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestRubberbandSelection")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)
        self.left_box = None
        self.right_box = None

        self._process_events(50)
        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.viewer = self.view.getViewer()
        try:
            self._refresh_view_widgets()
        except RuntimeError:
            FreeCAD.closeDocument(self.doc.Name)
            self.doc = None
            raise unittest.SkipTest(
                "3D view widget wrapping is unavailable in this test environment"
            )

        self.view_preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
        self.old_enable_selection = self.view_preferences.GetBool("EnableSelection", True)
        self.view_preferences.SetBool("EnableSelection", True)

        self.viewer.setEnabledNaviCube(False)
        self.view.setAxisCross(False)
        self._refresh_view()

    def tearDown(self):
        FreeCADGui.Selection.clearSelection()
        if hasattr(self, "view_preferences"):
            self.view_preferences.SetBool("EnableSelection", self.old_enable_selection)
        if self.doc is not None:
            FreeCAD.closeDocument(self.doc.Name)

    def test_plain_drag_selects_in_supported_styles(self):
        self._ensure_selection_objects()
        for label, style in self.PLAIN_DRAG_STYLES:
            with self.subTest(style=label):
                FreeCADGui.Selection.clearSelection()
                self.view.setNavigationType(style)
                self._refresh_view()

                self._drag_select(self._selection_rect(self.left_box))
                self.assertEqual(self._selected_names(), {self.left_box.Name})

    def test_ctrl_drag_adds_in_supported_styles(self):
        self._ensure_selection_objects()
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
        self._ensure_selection_objects()
        for label, style in self.MODIFIED_DRAG_STYLES:
            with self.subTest(style=label):
                FreeCADGui.Selection.clearSelection()
                self.view.setNavigationType(style)
                self._refresh_view()

                self._drag_select(self._selection_rect(self.left_box), shift=True)
                self.assertEqual(self._selected_names(), {self.left_box.Name})

    def test_shift_ctrl_drag_adds_in_modified_styles(self):
        self._ensure_selection_objects()
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

    def test_drag_motion_reaches_viewer_callbacks_before_box_selection(self):
        for label, style in self.VIEWER_HANDOFF_STYLES:
            with self.subTest(style=label):
                FreeCADGui.Selection.clearSelection()
                self.view.setNavigationType(style)
                self._refresh_view()

                state = {"armed": False, "moves": 0}

                def on_button(event_callback):
                    event = event_callback.getEvent()
                    if event.getButton() != coin.SoMouseButtonEvent.BUTTON1:
                        return
                    state["armed"] = event.getState() == coin.SoButtonEvent.DOWN

                def on_move(event_callback):
                    if not state["armed"]:
                        return
                    state["moves"] += 1
                    event_callback.setHandled()

                button_callback = self.view.addEventCallbackPivy(
                    coin.SoMouseButtonEvent.getClassTypeId(),
                    on_button,
                )
                move_callback = self.view.addEventCallbackPivy(
                    coin.SoLocation2Event.getClassTypeId(),
                    on_move,
                )

                try:
                    start = self.viewport.rect().center() + QtCore.QPoint(-60, 0)
                    end = start + QtCore.QPoint(90, 0)
                    self._drag_path(start, end)
                finally:
                    self.view.removeEventCallbackPivy(
                        coin.SoMouseButtonEvent.getClassTypeId(),
                        button_callback,
                    )
                    self.view.removeEventCallbackPivy(
                        coin.SoLocation2Event.getClassTypeId(),
                        move_callback,
                    )

                self.assertGreater(
                    state["moves"],
                    0,
                    "Viewer callbacks should receive drag motion before box selection starts",
                )

    def test_cubic_bezier_drag_release_is_not_stolen_when_selection_is_disabled(self):
        try:
            from draftguitools import gui_beziers
            from draftutils import params as draft_params
        except ImportError as exc:
            raise unittest.SkipTest("Draft GUI tools are unavailable in this build") from exc

        FreeCADGui.activateWorkbench("DraftWorkbench")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)
        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.viewer = self.view.getViewer()
        self.view.setNavigationType("CAD")
        self._refresh_view()

        tool = gui_beziers.CubicBezCurve()
        start = self.viewport.rect().center() + QtCore.QPoint(-50, 0)
        end = start + QtCore.QPoint(90, 0)

        try:
            tool.Activated()
            self._process_events(50)
            self.assertFalse(draft_params.get_param_view("EnableSelection"))

            self._drag_path(start, end)
            self.assertEqual(
                len(tool.node),
                2,
                "Cubic Bézier drag should reach button release when selection is disabled",
            )
        finally:
            tool.finish(cont=False)
            self._process_events(20)

    def _refresh_view(self):
        self.view.viewIsometric()
        self.view.fitAll()
        self._refresh_view_widgets()
        self.viewport.setFocus(OTHER_FOCUS_REASON)
        self._process_events()
        self._process_events()

    def _process_events(self, wait_ms=50):
        FreeCADGui.updateGui()
        app = QtGui.QApplication.instance()
        app.processEvents()
        time.sleep(wait_ms / 1000.0)
        app.processEvents()

    def _device_pixel_ratio(self):
        if hasattr(self.viewport, "devicePixelRatioF"):
            return self.viewport.devicePixelRatioF()
        return float(self.viewport.devicePixelRatio())

    def _refresh_view_widgets(self, timeout_ms=1000):
        deadline = time.monotonic() + (timeout_ms / 1000.0)
        while True:
            try:
                graphics_view = self.view.graphicsView()
                viewport = graphics_view.viewport()
                viewport.rect()
                self.graphics_view = graphics_view
                self.viewport = viewport
                return
            except RuntimeError:
                if time.monotonic() >= deadline:
                    raise
                self._process_events(20)

    def _to_qpoint(self, point):
        self._refresh_view_widgets()
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

    def _ensure_selection_objects(self):
        if self.left_box and self.right_box:
            return

        if Part is None:
            self.skipTest("Part.makeBox is unavailable in this build")

        left_shape = Part.makeBox(6, 6, 6, FreeCAD.Vector(-20, -3, 0))
        right_shape = Part.makeBox(6, 6, 6, FreeCAD.Vector(20, -3, 0))
        self.left_box = self._add_part_feature("LeftBox", left_shape)
        self.right_box = self._add_part_feature("RightBox", right_shape)
        self.doc.recompute()
        self._refresh_view()

        left_rect = self._object_rect(self.left_box)
        right_rect = self._object_rect(self.right_box)
        self.assertLess(
            left_rect.right(),
            right_rect.left(),
            "Test objects must not overlap in screen space",
        )

    def _add_part_feature(self, name, shape):
        if "Part::Feature" in self.doc.supportedTypes():
            obj = self.doc.addObject("Part::Feature", name)
            obj.Shape = shape
            return obj

        return Part.show(shape, name)

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

    def _drag_path(self, start, end, modifiers=NO_MODIFIER):
        center = QtCore.QPoint((start.x() + end.x()) // 2, (start.y() + end.y()) // 2)
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
        self._refresh_view_widgets()
        app = QtGui.QApplication.instance()
        global_pos = self.viewport.mapToGlobal(pos)
        QtGui.QCursor.setPos(global_pos)
        event = QtGui.QMouseEvent(
            event_type,
            pos,
            global_pos,
            button,
            buttons,
            modifiers,
        )

        app.sendEvent(self.viewport, event)
