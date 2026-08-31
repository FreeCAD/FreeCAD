# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
from PySide import QtCore, QtGui
from Gui.Wait import FreeCADGui
from Support import temporary_preference
from SketcherTests.Support import SketcherGuiTestCase


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
            self.gui.key_click(widget, self.KEYS[ch], ch)

    def active_spinbox(self):
        return self.gui.focused_widget(QtGui.QAbstractSpinBox)

    def visible_spinboxes(self):
        return self.gui.find_widgets(QtGui.QAbstractSpinBox, visible_only=True)

    def begin_sketch_edit_with_task_dialog(self):
        self.enter_sketch_edit(self.doc, self.sketch)
        self.assertIsNotNone(
            self.gui.wait_for_task_panel(), "Expected the Sketcher task dialog to open"
        )

    def begin_rectangle_with_visible_ovp(self):
        self.begin_sketch_edit_with_task_dialog()

        view = FreeCADGui.ActiveDocument.ActiveView
        view.viewTop()
        view.fitAll()

        origin = FreeCAD.Vector(0, 0, 0)
        viewport = self.gui.active_view().viewport

        def wait_for_origin_point():
            first_point = None

            def origin_is_framed():
                nonlocal first_point

                width, height = view.getSize()
                if width <= 0 or height <= 0:
                    return False

                view.fitAll()
                point = self.gui.active_view().world_to_screen(origin)
                interior_rect = viewport.rect().adjusted(20, 20, -20, -20)
                if not interior_rect.contains(point):
                    return False

                first_point = point
                return True

            self.assertTrue(
                self.gui.wait_until(origin_is_framed, timeout_ms=5000, step_ms=100),
                "Expected the sketch origin to project to an interior viewport point",
            )
            self.assertIsNotNone(first_point)
            return first_point

        FreeCADGui.runCommand("Sketcher_CreateRectangle")

        first_point = wait_for_origin_point()

        move_target = self.gui.clamp_to_widget(
            viewport,
            QtCore.QPoint(first_point.x() + 80, first_point.y() - 60),
        )

        self.gui.move(viewport, first_point)
        self.gui.click(viewport, first_point)
        self.gui.move(viewport, move_target)

        self.assertTrue(
            self.gui.wait_until(lambda: len(self.visible_spinboxes()) == 2, timeout_ms=1000),
            "Expected the rectangle OVPs to become visible after the first click",
        )
        self.assertIsNotNone(
            self.gui.wait_for_focus(QtGui.QAbstractSpinBox, timeout_ms=1000),
            "Expected the first rectangle OVP to receive focus",
        )

        return viewport, first_point

    def test_reset_edit_closes_sketch_task_dialog(self):
        self.begin_sketch_edit_with_task_dialog()

        FreeCADGui.ActiveDocument.resetEdit()

        self.assertTrue(
            self.gui.wait_until(lambda: self.gui.active_task_panel() is None, timeout_ms=1000),
            "Expected resetEdit() to close the Sketcher task dialog",
        )
        self.assert_sketch_edit_inactive()

    def test_task_dialog_reject_exits_sketch_edit(self):
        self.begin_sketch_edit_with_task_dialog()

        self.gui.active_task_panel().reject()

        self.assertTrue(
            self.gui.wait_until(lambda: self.gui.active_task_panel() is None, timeout_ms=1000),
            "Expected reject() to close the Sketcher task dialog",
        )
        self.assert_sketch_edit_inactive()

    def test_task_dialog_accept_exits_sketch_edit(self):
        self.begin_sketch_edit_with_task_dialog()

        self.gui.active_task_panel().accept()

        self.assertTrue(
            self.gui.wait_until(lambda: self.gui.active_task_panel() is None, timeout_ms=1000),
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
        self.gui.key_click(first_spinbox, QtCore.Qt.Key_Tab, "\t")

        self.assertTrue(
            self.gui.wait_until(
                lambda: self.active_spinbox() is not None
                and self.active_spinbox() is not first_spinbox,
                timeout_ms=1000,
            ),
            "Expected Tab to move focus to the second rectangle OVP",
        )

        second_spinbox = self.active_spinbox()
        self.assertIsNotNone(second_spinbox, "Expected the second rectangle OVP to have focus")
        self.key_text(second_spinbox, "20")
        self.gui.key_click(second_spinbox, QtCore.Qt.Key_Return, "\r")

        self.assertTrue(
            self.gui.wait_until(
                lambda: self.sketch.GeometryCount >= 4,
                timeout_ms=1000,
                description="rectangle geometry",
            ),
            "Expected the rectangle to be created after accepting both OVPs",
        )
        self.assertGreaterEqual(
            self.sketch.GeometryCount,
            4,
            "Expected the rectangle to be created after accepting both OVPs",
        )

    def test_rectangle_ovp_escape_resets_tool_without_exiting_sketch(self):
        viewport, first_point = self.begin_rectangle_with_visible_ovp()

        first_spinbox = self.active_spinbox()
        self.assertIsNotNone(first_spinbox, "Expected the first rectangle OVP to have focus")
        self.gui.key_click(first_spinbox, QtCore.Qt.Key_Escape)

        self.assertTrue(
            self.gui.wait_until(lambda: len(self.visible_spinboxes()) == 0, timeout_ms=1000),
            "Expected Esc to close the rectangle OVPs",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)
        self.assert_sketch_edit_active()

        restart_point = self.gui.clamp_to_widget(
            viewport,
            QtCore.QPoint(first_point.x() + 120, first_point.y() + 50),
        )
        restart_move = self.gui.clamp_to_widget(
            viewport,
            QtCore.QPoint(restart_point.x() + 70, restart_point.y() - 40),
        )

        self.gui.move(viewport, restart_point)
        self.gui.click(viewport, restart_point)
        self.gui.move(viewport, restart_move)

        self.assertTrue(
            self.gui.wait_until(lambda: len(self.visible_spinboxes()) == 2, timeout_ms=1000),
            "Expected Esc to reset the rectangle tool back to its first stage",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)
        self.assertIsNotNone(
            self.gui.wait_for_focus(QtGui.QAbstractSpinBox, timeout_ms=1000),
            "Expected the first rectangle OVP to receive focus after reset",
        )

    def test_rectangle_ovp_escape_then_right_click_exits_tool(self):
        viewport, first_point = self.begin_rectangle_with_visible_ovp()

        first_spinbox = self.active_spinbox()
        self.assertIsNotNone(first_spinbox, "Expected the first rectangle OVP to have focus")
        self.gui.key_click(first_spinbox, QtCore.Qt.Key_Escape)

        self.assertTrue(
            self.gui.wait_until(lambda: len(self.visible_spinboxes()) == 0, timeout_ms=1000),
            "Expected Esc to close the rectangle OVPs",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)
        self.assert_sketch_edit_active()

        cancel_point = self.gui.clamp_to_widget(
            viewport,
            QtCore.QPoint(first_point.x() + 120, first_point.y() + 50),
        )
        retry_move = self.gui.clamp_to_widget(
            viewport,
            QtCore.QPoint(cancel_point.x() + 70, cancel_point.y() - 40),
        )

        self.gui.right_click(viewport, cancel_point)
        self.assertTrue(
            self.gui.wait_until(lambda: len(self.visible_spinboxes()) == 0, timeout_ms=400),
            "Expected right click to keep the rectangle OVPs closed after canceling",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)
        self.assert_sketch_edit_active()

        self.gui.move(viewport, cancel_point)
        self.gui.click(viewport, cancel_point)
        self.gui.move(viewport, retry_move)

        self.assertFalse(
            self.gui.wait_until(lambda: len(self.visible_spinboxes()) == 2, timeout_ms=500),
            "Expected right click to exit the rectangle tool after OVP Esc",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)

    def test_rectangle_ovp_escape_then_escape_then_escape_exits_sketch(self):
        viewport, first_point = self.begin_rectangle_with_visible_ovp()

        first_spinbox = self.active_spinbox()
        self.assertIsNotNone(first_spinbox, "Expected the first rectangle OVP to have focus")
        self.gui.key_click(first_spinbox, QtCore.Qt.Key_Escape)

        self.assertTrue(
            self.gui.wait_until(lambda: len(self.visible_spinboxes()) == 0, timeout_ms=1000),
            "Expected the first Esc to close the rectangle OVPs",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)
        self.assert_sketch_edit_active()

        view = FreeCADGui.ActiveDocument.ActiveView
        graphics_view = view.graphicsView()
        graphics_view.setFocus(QtCore.Qt.OtherFocusReason)
        self.assertIsNotNone(
            self.gui.wait_for_focus(parent=graphics_view, timeout_ms=500),
            "Expected the Sketcher viewport to receive focus",
        )

        self.gui.key_click(graphics_view, QtCore.Qt.Key_Escape)
        self.assert_sketch_edit_active()

        cancel_point = self.gui.clamp_to_widget(
            viewport,
            QtCore.QPoint(first_point.x() + 120, first_point.y() + 50),
        )
        retry_move = self.gui.clamp_to_widget(
            viewport,
            QtCore.QPoint(cancel_point.x() + 70, cancel_point.y() - 40),
        )

        self.gui.move(viewport, cancel_point)
        self.gui.click(viewport, cancel_point)
        self.gui.move(viewport, retry_move)

        self.assertFalse(
            self.gui.wait_until(lambda: len(self.visible_spinboxes()) == 2, timeout_ms=500),
            "Expected the second Esc to exit the rectangle tool",
        )
        self.assertEqual(self.sketch.GeometryCount, 0)

        graphics_view.setFocus(QtCore.Qt.OtherFocusReason)
        self.assertIsNotNone(
            self.gui.wait_for_focus(parent=graphics_view, timeout_ms=500),
            "Expected the Sketcher viewport to receive focus",
        )
        self.gui.key_click(graphics_view, QtCore.Qt.Key_Escape)

        self.assertTrue(
            self.gui.wait_until(lambda: self.gui.active_task_panel() is None, timeout_ms=1000),
            "Expected the third Esc to close the Sketcher task dialog",
        )
        self.assert_sketch_edit_inactive()

    def test_auto_color_restores_line_color_from_preferences(self):
        color_key = "SketchEdgeColor"

        try:
            manual_color = 0x112233FF
            preference_color = 0x44AA88FF

            view = self.sketch.ViewObject
            view.AutoColor = False
            view.LineColor = manual_color
            self.assertEqual(self.pack_color(view.LineColor), manual_color)

            with temporary_preference(
                "User parameter:BaseApp/Preferences/View",
                color_key,
                preference_color,
                "Unsigned Long",
            ):
                self.assertEqual(self.pack_color(view.LineColor), manual_color)

                view.AutoColor = True
                self.assertTrue(
                    self.gui.wait_until(
                        lambda: self.pack_color(view.LineColor) == preference_color,
                        timeout_ms=1000,
                        description="automatic sketch edge color",
                    ),
                    "Expected AutoColor to apply the preference color",
                )
                self.assertEqual(self.pack_color(view.LineColor), preference_color)
        finally:
            view.AutoColor = True
