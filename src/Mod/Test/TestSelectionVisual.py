# SPDX-License-Identifier: LGPL-2.1-or-later

"""GUI visual regression test for selection/preselection ordering.

Run with:
    FreeCAD -t TestSelectionVisual
"""

from contextlib import suppress
import time
import unittest

import FreeCAD
import FreeCADGui
from FreeCADGui import Selection
import Part

try:
    from PySide6 import QtWidgets
except ImportError:
    from PySide import QtGui as QtWidgets  # type: ignore


PART_PLANE_TYPE = f"{Part.__name__}::Plane"


class TestSelectionVisual(unittest.TestCase):
    """Verify that live preselection draws above selection overlays."""

    _COLOR_DELTA_MIN = 0.15
    _COLOR_DELTA_RESTORE_MAX = 0.05

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestSelectionVisual")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)
        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.viewer = self.view.getViewer()
        self._had_axis_cross = self.view.hasAxisCross()
        self.view.setAxisCross(False)

        self._had_navi_cube = self.viewer.isEnabledNaviCube()
        self.viewer.setEnabledNaviCube(False)

    def tearDown(self):
        with suppress(Exception):
            Selection.clearPreselection()
        with suppress(Exception):
            Selection.clearSelection()

        with suppress(Exception):
            self.view.setAxisCross(self._had_axis_cross)

        self._set_navi_cube_enabled(self._had_navi_cube)

        if FreeCAD.getDocument(self.doc.Name):
            FreeCAD.closeDocument(self.doc.Name)

    def test_preselection_overrides_selection_overlay(self):
        plane = self._create_test_plane()
        self._prepare_view()

        base_color = self._center_pixel_color()

        Selection.addSelection(plane)
        self._flush_gui()
        selection_color = self._center_pixel_color()

        Selection.setPreselection(plane, "Face1")
        self._flush_gui()
        preselection_color = self._center_pixel_color()

        self.assertGreater(
            self._color_distance(base_color, selection_color),
            self._COLOR_DELTA_MIN,
            msg=(
                "Selection overlay did not visibly change the rendered face. "
                f"base={base_color}, selection={selection_color}"
            ),
        )
        self.assertGreater(
            self._color_distance(selection_color, preselection_color),
            self._COLOR_DELTA_MIN,
            msg=(
                "Preselection did not visibly override the selection overlay. "
                f"selection={selection_color}, preselection={preselection_color}"
            ),
        )

    def test_selection_can_be_cleared(self):
        plane = self._create_test_plane()
        self._prepare_view()

        base_color = self._center_pixel_color()

        Selection.addSelection(plane)
        self._flush_gui()
        selection_color = self._center_pixel_color()

        Selection.clearSelection()
        self._flush_gui()
        cleared_color = self._center_pixel_color()

        self._assert_color_changed(
            base_color,
            selection_color,
            "Selection overlay did not visibly change the rendered face.",
        )
        self._assert_color_restored(
            base_color,
            cleared_color,
            "Clearing selection did not restore the original rendering.",
        )

    def test_preselection_can_be_cleared(self):
        plane = self._create_test_plane()
        self._prepare_view()

        base_color = self._center_pixel_color()

        Selection.setPreselection(plane, "Face1")
        self._flush_gui()
        preselection_color = self._center_pixel_color()

        Selection.clearPreselection()
        self._flush_gui()
        cleared_color = self._center_pixel_color()

        self._assert_color_changed(
            base_color,
            preselection_color,
            "Preselection overlay did not visibly change the rendered face.",
        )
        self._assert_color_restored(
            base_color,
            cleared_color,
            "Clearing preselection did not restore the original rendering.",
        )

    def _create_test_plane(self):
        plane = self.doc.addObject(PART_PLANE_TYPE, "Plane")
        plane.Length = 40
        plane.Width = 40
        plane.ViewObject.ShapeColor = (0.66, 0.66, 0.74)
        self.doc.recompute()
        return plane

    def _prepare_view(self):
        self.view.viewTop()
        self._set_orthographic_if_supported()
        self.view.fitAll()
        self._flush_gui()

    def _set_orthographic_if_supported(self):
        with suppress(Exception):
            self.view.setCameraType("Orthographic")

    def _set_navi_cube_enabled(self, enabled):
        with suppress(Exception):
            self.viewer.setEnabledNaviCube(enabled)

    def _flush_gui(self):
        for _ in range(4):
            FreeCADGui.updateGui()
            QtWidgets.QApplication.processEvents()
            self.view.redraw()
            time.sleep(0.05)

    def _center_pixel_color(self):
        image = self.viewer.grabFramebuffer()
        color = image.pixelColor(image.width() // 2, image.height() // 2)
        return (color.redF(), color.greenF(), color.blueF())

    def _assert_color_changed(self, before, after, message):
        self.assertGreater(
            self._color_distance(before, after),
            self._COLOR_DELTA_MIN,
            msg=f"{message} before={before}, after={after}",
        )

    def _assert_color_restored(self, expected, actual, message):
        self.assertLess(
            self._color_distance(expected, actual),
            self._COLOR_DELTA_RESTORE_MAX,
            msg=f"{message} expected={expected}, actual={actual}",
        )

    @staticmethod
    def _color_distance(lhs, rhs):
        return sum((a - b) ** 2 for a, b in zip(lhs, rhs)) ** 0.5
