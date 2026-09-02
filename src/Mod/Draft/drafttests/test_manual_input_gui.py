# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Max Wilfinger
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


"""Unit tests for the Draft Workbench, manual point-input field locking."""

import math
from unittest import mock

import DraftGui
import DraftVecUtils
import FreeCAD as App
import FreeCADGui as Gui
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui
import PySide.QtWidgets as QtWidgets

from draftguitools import gui_tool_utils
from draftguitools.gui_snapper import Snapper
from drafttaskpanels.task_circulararray import TaskPanelCircularArray
from drafttaskpanels.task_polararray import TaskPanelPolarArray
from drafttests import test_base
from draftutils.todo import ToDo


class DraftGuiManualInput(test_base.DraftTestCaseDoc):
    """GUI regressions for manual point-input field locking in DraftGui."""

    def setUp(self):
        super().setUp()
        self.tb = Gui.draftToolBar

    def tearDown(self):
        self._close_point_ui()
        super().tearDown()

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _open_point_ui(self):
        """Open the Draft point-entry task panel in line mode and flush events."""
        self.tb.lineUi(title="Test Manual Input", rel=False)
        ToDo.doTasks()
        Gui.updateGui()
        QtWidgets.QApplication.processEvents()

    def _close_point_ui(self):
        """Close the point-entry task panel and flush events."""
        self.tb.offUi()
        ToDo.doTasks()
        Gui.updateGui()
        QtWidgets.QApplication.processEvents()

    def _try_focus(self, widget):
        """Attempt to focus widget. Returns True if successful (not flaky-CI proof)."""
        window = widget.window()
        if window is not None:
            window.activateWindow()
            window.raise_()
        widget.setFocus(QtCore.Qt.OtherFocusReason)
        QtWidgets.QApplication.processEvents()
        return widget.hasFocus()

    def _assert_vector_almost_equal(self, actual, expected, tol=1e-6):
        self.assertAlmostEqual(actual.x, expected.x, delta=tol)
        self.assertAlmostEqual(actual.y, expected.y, delta=tol)
        self.assertAlmostEqual(actual.z, expected.z, delta=tol)

    def _edit(self, key, text):
        field = {
            "x": self.tb.xValue,
            "y": self.tb.yValue,
            "z": self.tb.zValue,
            "length": self.tb.lengthValue,
            "angle": self.tb.angleValue,
        }[key]
        field.textEdited.emit(text)
        field.setText(text)
        QtWidgets.QApplication.processEvents()

    def _type(self, field, text):
        field.setText("")
        for character in text:
            event = QtGui.QKeyEvent(
                QtCore.QEvent.KeyPress,
                0,
                QtCore.Qt.NoModifier,
                character,
            )
            QtWidgets.QApplication.sendEvent(field, event)
        QtWidgets.QApplication.processEvents()

    def _press_enter(self, field):
        self._send_key(field, QtCore.Qt.Key_Return)

    def _send_key(self, field, key, text=""):
        event = QtGui.QKeyEvent(QtCore.QEvent.KeyPress, key, QtCore.Qt.NoModifier, text)
        QtWidgets.QApplication.sendEvent(field, event)
        QtWidgets.QApplication.processEvents()

    # ------------------------------------------------------------------
    # Lock-invalidation rules: helper-level (stable, no focus required)
    #
    # The change handlers call these helpers before recomputing the other
    # representation; they are the actual regression surface.
    # ------------------------------------------------------------------

    def test_unlock_polar_fields_clears_length_and_angle(self):
        """_unlock_polar_fields must drop length and angle locks."""
        self._open_point_ui()
        self.tb._set_field_locked("length", True)
        self.tb._set_field_locked("angle", True)

        self.tb._unlock_polar_fields()

        self.assertFalse(self.tb._is_field_locked("length"))
        self.assertFalse(self.tb._is_field_locked("angle"))

    def test_unlock_cartesian_fields_clears_x_y_z(self):
        """_unlock_cartesian_fields must drop X, Y, and Z locks."""
        self._open_point_ui()
        for key in ("x", "y", "z"):
            self.tb._set_field_locked(key, True)

        self.tb._unlock_cartesian_fields()

        for key in ("x", "y", "z"):
            self.assertFalse(self.tb._is_field_locked(key))

    # ------------------------------------------------------------------
    # Lock-invalidation rules: handler-level (skipped if focus is unstable)
    # ------------------------------------------------------------------

    def test_editing_x_unlocks_length_and_angle(self):
        """Editing X must drop length and angle locks (polar group)."""
        self._open_point_ui()
        if not self._try_focus(self.tb.xValue):
            self.skipTest("focus not available in this run")
        self.tb._set_field_locked("length", True)
        self.tb._set_field_locked("angle", True)

        self.tb.changeXValue(App.Units.Quantity(10, App.Units.Length))

        self.assertFalse(self.tb._is_field_locked("length"))
        self.assertFalse(self.tb._is_field_locked("angle"))

    def test_editing_length_unlocks_cartesian_fields(self):
        """Editing length must drop X, Y, and Z locks (cartesian group)."""
        self._open_point_ui()
        if not self._try_focus(self.tb.lengthValue):
            self.skipTest("focus not available in this run")
        for key in ("x", "y", "z"):
            self.tb._set_field_locked(key, True)

        self.tb.changeLengthValue(App.Units.Quantity(25, App.Units.Length))

        for key in ("x", "y", "z"):
            self.assertFalse(self.tb._is_field_locked(key))

    # ------------------------------------------------------------------
    # Point constraint resolution
    # ------------------------------------------------------------------

    def test_constrain_point_overrides_locked_cartesian_component(self):
        """A locked X component must replace the candidate point's X."""
        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = False

        self.tb.x = 10
        self.tb._set_field_locked("x", True)

        resolved = self.tb.constrain_point(App.Vector(1, 2, 3), App.Vector())

        self._assert_vector_almost_equal(resolved, App.Vector(10, 2, 3))

    def test_constrain_point_preserves_locked_length(self):
        """A locked length must rescale the candidate vector to that length."""
        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = False

        self.tb.lvalue = 100
        self.tb._set_field_locked("length", True)

        resolved = self.tb.constrain_point(App.Vector(3, 4, 0), App.Vector())

        self.assertAlmostEqual(resolved.Length, 100.0, delta=1e-6)

    def test_angle_lock_controls_direction_without_locking_length(self):
        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = False
        self.tb.pvalue = 90
        self._edit("angle", "30 deg")

        first = self.tb.constrain_point(App.Vector(10, 20, 0), App.Vector())
        second = self.tb.constrain_point(App.Vector(100, 20, 0), App.Vector())
        axis = self.tb._angle_lock_axis

        self.assertTrue(self.tb._is_field_locked("angle"))
        self.assertFalse(self.tb._is_field_locked("length"))
        self.assertAlmostEqual(first.cross(axis).Length, 0, delta=1e-6)
        self.assertAlmostEqual(second.cross(axis).Length, 0, delta=1e-6)
        self.assertNotAlmostEqual(first.Length, second.Length, delta=1e-6)

    def test_angle_lock_preserves_established_off_plane_direction(self):
        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = False
        self.tb.pvalue = 45
        self._edit("angle", "30 deg")

        resolved = self.tb.constrain_point(App.Vector(10, 20, 30), App.Vector())

        self.assertAlmostEqual(resolved.cross(self.tb._angle_lock_axis).Length, 0, delta=1e-6)
        self.assertNotAlmostEqual(resolved.z, 0, delta=1e-6)

    def test_length_lock_controls_magnitude_without_locking_direction(self):
        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = False
        self._edit("length", "50 mm")

        along_x = self.tb.constrain_point(App.Vector(10, 0, 0), App.Vector())
        along_y = self.tb.constrain_point(App.Vector(0, 10, 0), App.Vector())

        self.assertTrue(self.tb._is_field_locked("length"))
        self.assertFalse(self.tb._is_field_locked("angle"))
        self._assert_vector_almost_equal(along_x, App.Vector(50, 0, 0))
        self._assert_vector_almost_equal(along_y, App.Vector(0, 50, 0))

    def test_angle_input_replaces_coordinate_locks_in_preview_and_result(self):
        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = True
        last = App.Vector(10, 20, 30)
        self._edit("x", "10 mm")
        self._edit("y", "10 mm")
        self._edit("z", "0 mm")
        self.tb.pvalue = 90

        self._edit("angle", "45 deg")
        resolved = self.tb.constrain_point(last + App.Vector(20, 0, 8), last)

        self.assertFalse(any(self.tb._is_field_locked(key) for key in ("x", "y", "z")))
        self.assertTrue(self.tb._is_field_locked("angle"))
        self.assertFalse(self.tb._is_field_locked("length"))
        self.assertAlmostEqual(resolved.x - last.x, resolved.y - last.y, delta=1e-6)
        self.assertAlmostEqual(resolved.z, last.z, delta=1e-6)
        self.tb.displayPoint(resolved, last)
        result = self.tb.get_new_point(App.Vector(self.tb.x, self.tb.y, self.tb.z))
        self._assert_vector_almost_equal(result, resolved)

    def test_length_input_replaces_coordinate_locks_without_locking_angle(self):
        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = False
        for key, text in (("x", "10 mm"), ("y", "10 mm"), ("z", "0 mm")):
            self._edit(key, text)

        self._edit("length", "25 mm")
        candidate = App.Vector(0, 8, 6)
        resolved = self.tb.constrain_point(candidate, App.Vector())

        self.assertFalse(any(self.tb._is_field_locked(key) for key in ("x", "y", "z")))
        self.assertTrue(self.tb._is_field_locked("length"))
        self.assertFalse(self.tb._is_field_locked("angle"))
        self.assertAlmostEqual(resolved.Length, 25, delta=1e-6)
        self.assertAlmostEqual(resolved.cross(candidate).Length, 0, delta=1e-6)

    def test_length_lock_clears_existing_axis_constraint(self):
        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = False
        self.tb.mask = "x"
        Gui.Snapper.mask = "x"

        self._edit("length", "25 mm")
        candidate = App.Vector(0, 8, 6)
        resolved = self.tb.constrain_point(candidate, App.Vector())

        self.assertIsNone(self.tb.mask)
        self.assertIsNone(Gui.Snapper.mask)
        self.assertAlmostEqual(resolved.cross(candidate).Length, 0, delta=1e-6)

    def test_length_lock_preserves_existing_angle_lock(self):
        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = False
        self.tb.pvalue = 90
        self._edit("angle", "30 deg")

        self._edit("length", "25 mm")
        resolved = self.tb.constrain_point(App.Vector(20, 0, 0), App.Vector())

        self.assertTrue(self.tb._is_field_locked("angle"))
        self.assertTrue(self.tb._is_field_locked("length"))
        self.assertIsInstance(Gui.Snapper.mask, App.Vector)
        self.assertAlmostEqual(resolved.cross(self.tb._angle_lock_axis).Length, 0, delta=1e-6)
        self.assertAlmostEqual(resolved.Length, 25, delta=1e-6)

    def test_angle_and_length_locks_allow_opposite_direction(self):
        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = False
        self.tb.pvalue = 90
        self._edit("angle", "30 deg")
        self._edit("length", "40 mm")
        opposite = App.Vector(
            DraftVecUtils.get_cartesian_coords(10, math.pi / 2, math.radians(-150))
        )

        resolved = self.tb.constrain_point(opposite, App.Vector())
        self.tb.displayPoint(resolved, App.Vector())

        self.assertAlmostEqual(resolved.Length, 40, delta=1e-6)
        self.assertLess(resolved.dot(self.tb._angle_lock_axis), 0)
        self.assertAlmostEqual(self.tb.avalue, -150, delta=1e-6)

    def test_snap_marker_uses_resolved_angle_point(self):
        class Tracker:
            Visible = False

            def setCoords(self, point):
                self.point = App.Vector(point)

            def on(self):
                self.Visible = True

        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = False
        self.tb.pvalue = 90
        self._edit("angle", "45 deg")
        snapper = Snapper()
        snapper.tracker = Tracker()

        snapper.setPointConstraintProvider(self.tb)
        resolved = snapper._apply_point_constraint(App.Vector(10, 0, 0), App.Vector(), False)

        self._assert_vector_almost_equal(snapper.tracker.point, resolved)
        self.assertAlmostEqual(resolved.x, resolved.y, delta=1e-6)

    def test_enter_locks_valid_input_and_unit_only_input_stays_unlocked(self):
        self._open_point_ui()
        self.tb.xValue.setText("25 mm")
        self._press_enter(self.tb.xValue)
        self.assertTrue(self.tb._is_field_locked("x"))

        self.tb.yValue.setText("mm")
        self._press_enter(self.tb.yValue)
        self.assertFalse(self.tb._is_field_locked("y"))

    def test_typing_locks_the_current_value_after_inputfield_parsing(self):
        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = False

        self._type(self.tb.xValue, "25 mm")
        resolved = self.tb.constrain_point(App.Vector(1, 2, 3), App.Vector())

        self.assertTrue(self.tb._is_field_locked("x"))
        self.assertAlmostEqual(self.tb.x, 25)
        self.assertAlmostEqual(resolved.x, 25)

    def test_point_constraint_provider_follows_task_panel_lifetime(self):
        self._open_point_ui()
        self.assertIs(Gui.Snapper.pointConstraintProvider, self.tb)

        self._close_point_ui()

        self.assertIsNone(Gui.Snapper.pointConstraintProvider)

    def test_array_panels_constrain_locked_center_coordinates(self):
        for panel_class in (TaskPanelCircularArray, TaskPanelPolarArray):
            with self.subTest(panel=panel_class.__name__):
                panel = panel_class()
                panel.form.input_c_x.setProperty("rawValue", 25)
                panel.locks.set_locked("x", True)
                snapper = Snapper()
                snapper.setPointConstraintProvider(panel)

                resolved = snapper._apply_point_constraint(App.Vector(1, 2, 3), App.Vector(), True)

                self._assert_vector_almost_equal(resolved, App.Vector(25, 2, 3))

    def test_deleting_numeric_part_unlocks_field(self):
        self._open_point_ui()
        self._type(self.tb.xValue, "25 mm")
        numeric_length = self.tb.number_length(self.tb.xValue.text())
        self.tb.xValue.setSelection(0, numeric_length)

        self._send_key(self.tb.xValue, QtCore.Qt.Key_Delete)

        self.assertFalse(self.tb._is_field_locked("x"))

    def test_lock_icon_and_double_click_unlock_field(self):
        self._open_point_ui()
        self._edit("x", "25 mm")

        self.tb._locks._actions["x"].trigger()
        self.assertFalse(self.tb._is_field_locked("x"))

        self._edit("x", "25 mm")
        event = QtGui.QMouseEvent(
            QtCore.QEvent.MouseButtonDblClick,
            QtCore.QPointF(1, 1),
            QtCore.Qt.LeftButton,
            QtCore.Qt.LeftButton,
            QtCore.Qt.NoModifier,
        )
        QtWidgets.QApplication.sendEvent(self.tb.xValue, event)

        self.assertFalse(self.tb._is_field_locked("x"))

    def test_arrow_nudge_updates_locked_value(self):
        self._open_point_ui()
        self.tb.globalMode = True
        self.tb.relativeMode = False
        self._type(self.tb.xValue, "25 mm")
        previous = self.tb.x

        self._send_key(self.tb.xValue, QtCore.Qt.Key_Up)
        resolved = self.tb.constrain_point(App.Vector(1, 2, 3), App.Vector())

        self.assertTrue(self.tb._is_field_locked("x"))
        self.assertNotEqual(self.tb.x, previous)
        self.assertAlmostEqual(resolved.x, self.tb.x)

    def test_zero_snap_result_is_not_replaced_by_cursor_point(self):
        class Target:
            node = [App.Vector(1, 2, 3)]
            featureName = "Line"

        args = {
            "Position": (0, 0),
            "ShiftDown": False,
            "CtrlDown": False,
            "AltDown": False,
        }
        self.tb.mouse = True
        with (
            mock.patch.object(Gui.Snapper, "snap", return_value=App.Vector()),
            mock.patch.object(self.tb, "displayPoint"),
        ):
            point, ctrl_point, _ = gui_tool_utils.get_point(Target(), args)

        self._assert_vector_almost_equal(point, App.Vector())
        self._assert_vector_almost_equal(ctrl_point, App.Vector())
