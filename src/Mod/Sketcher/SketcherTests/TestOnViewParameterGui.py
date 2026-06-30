# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
from PySide import QtCore, QtGui
from SketcherTests.GuiTestCase import FreeCADGui, SketcherGuiTestCase


class TestOnViewParameterGui(SketcherGuiTestCase):
    KEYS = {
        "0": QtCore.Qt.Key_0,
        "1": QtCore.Qt.Key_1,
        "2": QtCore.Qt.Key_2,
        "3": QtCore.Qt.Key_3,
        "4": QtCore.Qt.Key_4,
        "5": QtCore.Qt.Key_5,
        "6": QtCore.Qt.Key_6,
        "7": QtCore.Qt.Key_7,
        "8": QtCore.Qt.Key_8,
        "9": QtCore.Qt.Key_9,
        ".": QtCore.Qt.Key_Period,
        "-": QtCore.Qt.Key_Minus,
        " ": QtCore.Qt.Key_Space,
    }

    def setUp(self):
        super().setUp()

        FreeCADGui.activateWorkbench("SketcherWorkbench")
        self.doc = FreeCAD.newDocument("TestOnViewParameterGui")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.doc.recompute()

    def pack_color(self, color):
        r, g, b, a = color
        return (
            int(r * 255.0 + 0.5) << 24
            | int(g * 255.0 + 0.5) << 16
            | int(b * 255.0 + 0.5) << 8
            | int(a * 255.0 + 0.5)
        )

    def key_text(self, widget, text):
        for ch in text:
            self.key_click(widget, self.KEYS[ch], ch)

    def active_spinbox(self):
        widget = QtGui.QApplication.focusWidget()
        if isinstance(widget, QtGui.QAbstractSpinBox):
            return widget
        if isinstance(widget, QtGui.QLineEdit):
            parent = widget.parent()
            if isinstance(parent, QtGui.QAbstractSpinBox):
                return parent
        return None

    def visible_spinboxes(self):
        main_window = FreeCADGui.getMainWindow()
        return [
            spinbox
            for spinbox in main_window.findChildren(QtGui.QAbstractSpinBox)
            if spinbox.isVisible()
        ]

    def active_task_dialog(self):
        return FreeCADGui.Control.activeTaskDialog()

    def assert_sketch_edit_active(self):
        edit = FreeCADGui.ActiveDocument.getInEdit()
        self.assertIsNotNone(edit, "Expected sketch edit mode to remain active")
        self.assertTrue(
            edit.isDerivedFrom("SketcherGui::ViewProviderSketch"),
            "Expected a Sketcher view provider to remain in edit mode",
        )

    def assert_sketch_edit_inactive(self):
        self.assertIsNone(
            FreeCADGui.ActiveDocument.getInEdit(),
            "Expected sketch edit mode to be inactive",
        )

    def begin_sketch_edit_with_task_dialog(self):
        FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)
        self.pump(200)
        self.assert_sketch_edit_active()
        self.assertIsNotNone(self.active_task_dialog(), "Expected the Sketcher task dialog to open")

    def begin_rectangle_with_visible_ovp(self):
        self.begin_sketch_edit_with_task_dialog()

        view = FreeCADGui.ActiveDocument.ActiveView
        view.viewTop()
        view.fitAll()
        self.pump(100)

        viewport = view.graphicsView().viewport()
        origin = FreeCAD.Vector(0, 0, 0)

        def wait_for_origin_point():
            first_point = None

            def origin_is_framed():
                nonlocal first_point

                width, height = view.getSize()
                if width <= 0 or height <= 0:
                    return False

                view.fitAll()
                point = self.viewport_to_qpoint(view, viewport, view.getPointOnScreen(origin))
                interior_rect = viewport.rect().adjusted(20, 20, -20, -20)
                if not interior_rect.contains(point):
                    return False

                first_point = point
                return True

            self.assertTrue(
                self.wait_until(origin_is_framed, timeout_ms=5000, step_ms=100),
                "Expected the sketch origin to project to an interior viewport point",
            )
            self.assertIsNotNone(first_point)
            return first_point

        FreeCADGui.runCommand("Sketcher_CreateRectangle")
        self.pump(250)

        first_point = wait_for_origin_point()

        move_target = self.clamp_to_widget(
            viewport,
            QtCore.QPoint(first_point.x() + 80, first_point.y() - 60),
        )

        self.move(viewport, first_point)
        self.click(viewport, first_point)
        self.move(viewport, move_target)

        self.assertTrue(
            self.wait_until(lambda: len(self.visible_spinboxes()) == 2, timeout_ms=1000),
            "Expected the rectangle OVPs to become visible after the first click",
        )

        return viewport, first_point

    def test_reset_edit_closes_sketch_task_dialog(self):
        self.begin_sketch_edit_with_task_dialog()

        FreeCADGui.ActiveDocument.resetEdit()

        self.assertTrue(
            self.wait_until(lambda: self.active_task_dialog() is None, timeout_ms=1000),
            "Expected resetEdit() to close the Sketcher task dialog",
        )
        self.assert_sketch_edit_inactive()

    def test_task_dialog_reject_exits_sketch_edit(self):
        self.begin_sketch_edit_with_task_dialog()

        self.active_task_dialog().reject()

        self.assertTrue(
            self.wait_until(lambda: self.active_task_dialog() is None, timeout_ms=1000),
            "Expected reject() to close the Sketcher task dialog",
        )
        self.assert_sketch_edit_inactive()

    def test_task_dialog_accept_exits_sketch_edit(self):
        self.begin_sketch_edit_with_task_dialog()

        self.active_task_dialog().accept()

        self.assertTrue(
            self.wait_until(lambda: self.active_task_dialog() is None, timeout_ms=1000),
            "Expected accept() to close the Sketcher task dialog",
        )
        self.assert_sketch_edit_inactive()

    def test_rectangle_ovp_enter_finishes_without_crash(self):
        """
        Reproduce the rectangle OVP acceptance flow from PR #29201 review:
        click first point, type width, Tab, type height, Enter.

        If the process survives, the rectangle should be created in the sketch.
        """

        self.begin_rectangle_with_visible_ovp()

        first_spinbox = self.active_spinbox()
        self.assertIsNotNone(first_spinbox, "Expected the first rectangle OVP to have focus")
        self.key_text(first_spinbox, "10")
        self.key_click(first_spinbox, QtCore.Qt.Key_Tab, "\t")

        self.assertTrue(
            self.wait_until(
                lambda: self.active_spinbox() is not None
                and self.active_spinbox() is not first_spinbox,
                timeout_ms=1000,
            ),
            "Expected Tab to move focus to the second rectangle OVP",
        )

        second_spinbox = self.active_spinbox()
        self.assertIsNotNone(second_spinbox, "Expected the second rectangle OVP to have focus")
        self.key_text(second_spinbox, "20")
        self.key_click(second_spinbox, QtCore.Qt.Key_Return, "\r")

        self.pump(500)

        self.assertGreaterEqual(
            self.sketch.GeometryCount,
            4,
            "Expected the rectangle to be created after accepting both OVPs",
        )

    def test_rectangle_ovp_escape_resets_tool_without_exiting_sketch(self):
        viewport, first_point = self.begin_rectangle_with_visible_ovp()

        first_spinbox = self.active_spinbox()
        self.assertIsNotNone(first_spinbox, "Expected the first rectangle OVP to have focus")
        self.key_click(first_spinbox, QtCore.Qt.Key_Escape)

        self.assertTrue(
            self.wait_until(lambda: len(self.visible_spinboxes()) == 0, timeout_ms=1000),
            "Expected Esc to close the rectangle OVPs",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)
        self.assert_sketch_edit_active()

        restart_point = self.clamp_to_widget(
            viewport,
            QtCore.QPoint(first_point.x() + 120, first_point.y() + 50),
        )
        restart_move = self.clamp_to_widget(
            viewport,
            QtCore.QPoint(restart_point.x() + 70, restart_point.y() - 40),
        )

        self.move(viewport, restart_point)
        self.click(viewport, restart_point)
        self.move(viewport, restart_move)

        self.assertTrue(
            self.wait_until(lambda: len(self.visible_spinboxes()) == 2, timeout_ms=1000),
            "Expected Esc to reset the rectangle tool back to its first stage",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)
        self.assertIsNotNone(self.active_spinbox())

    def test_rectangle_ovp_escape_then_right_click_exits_tool(self):
        viewport, first_point = self.begin_rectangle_with_visible_ovp()

        first_spinbox = self.active_spinbox()
        self.assertIsNotNone(first_spinbox, "Expected the first rectangle OVP to have focus")
        self.key_click(first_spinbox, QtCore.Qt.Key_Escape)

        self.assertTrue(
            self.wait_until(lambda: len(self.visible_spinboxes()) == 0, timeout_ms=1000),
            "Expected Esc to close the rectangle OVPs",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)
        self.assert_sketch_edit_active()

        cancel_point = self.clamp_to_widget(
            viewport,
            QtCore.QPoint(first_point.x() + 120, first_point.y() + 50),
        )
        retry_move = self.clamp_to_widget(
            viewport,
            QtCore.QPoint(cancel_point.x() + 70, cancel_point.y() - 40),
        )

        self.right_click(viewport, cancel_point)
        self.assertTrue(
            self.wait_until(lambda: len(self.visible_spinboxes()) == 0, timeout_ms=400),
            "Expected right click to keep the rectangle OVPs closed after canceling",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)
        self.assert_sketch_edit_active()

        self.move(viewport, cancel_point)
        self.click(viewport, cancel_point)
        self.move(viewport, retry_move)

        self.assertFalse(
            self.wait_until(lambda: len(self.visible_spinboxes()) == 2, timeout_ms=500),
            "Expected right click to exit the rectangle tool after OVP Esc",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)

    def test_rectangle_ovp_escape_then_escape_then_escape_exits_sketch(self):
        viewport, first_point = self.begin_rectangle_with_visible_ovp()

        first_spinbox = self.active_spinbox()
        self.assertIsNotNone(first_spinbox, "Expected the first rectangle OVP to have focus")
        self.key_click(first_spinbox, QtCore.Qt.Key_Escape)

        self.assertTrue(
            self.wait_until(lambda: len(self.visible_spinboxes()) == 0, timeout_ms=1000),
            "Expected the first Esc to close the rectangle OVPs",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)
        self.assert_sketch_edit_active()

        view = FreeCADGui.ActiveDocument.ActiveView
        graphics_view = view.graphicsView()
        graphics_view.setFocus(QtCore.Qt.OtherFocusReason)
        self.pump(100)

        self.key_click(graphics_view, QtCore.Qt.Key_Escape)
        self.assert_sketch_edit_active()

        cancel_point = self.clamp_to_widget(
            viewport,
            QtCore.QPoint(first_point.x() + 120, first_point.y() + 50),
        )
        retry_move = self.clamp_to_widget(
            viewport,
            QtCore.QPoint(cancel_point.x() + 70, cancel_point.y() - 40),
        )

        self.move(viewport, cancel_point)
        self.click(viewport, cancel_point)
        self.move(viewport, retry_move)

        self.assertFalse(
            self.wait_until(lambda: len(self.visible_spinboxes()) == 2, timeout_ms=500),
            "Expected the second Esc to exit the rectangle tool",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)

        graphics_view.setFocus(QtCore.Qt.OtherFocusReason)
        self.pump(100)
        self.key_click(graphics_view, QtCore.Qt.Key_Escape)

        self.assertTrue(
            self.wait_until(lambda: self.active_task_dialog() is None, timeout_ms=1000),
            "Expected the third Esc to close the Sketcher task dialog",
        )
        self.assert_sketch_edit_inactive()

    def test_auto_color_restores_line_color_from_preferences(self):
        view_params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
        color_key = "SketchEdgeColor"
        had_color = color_key in view_params.GetUnsigneds()
        old_color = view_params.GetUnsigned(color_key, 0)

        try:
            manual_color = 0x112233FF
            preference_color = 0x44AA88FF

            view = self.sketch.ViewObject
            view.AutoColor = False
            view.LineColor = manual_color
            self.assertEqual(self.pack_color(view.LineColor), manual_color)

            view_params.SetUnsigned(color_key, preference_color)
            self.pump(100)
            self.assertEqual(self.pack_color(view.LineColor), manual_color)

            view.AutoColor = True
            self.pump(100)

            self.assertEqual(self.pack_color(view.LineColor), preference_color)
        finally:
            if had_color:
                view_params.SetUnsigned(color_key, old_color)
            else:
                view_params.RemUnsigned(color_key)
