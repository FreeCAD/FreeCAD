# SPDX-License-Identifier: LGPL-2.1-or-later

"""GUI visual regression test for selection/preselection ordering.

Run with:
    FreeCAD -t TestSelectionVisual
"""

from contextlib import suppress
import os
import tempfile
import time
import unittest

import FreeCAD
import FreeCADGui
from FreeCADGui import Selection
import Part

try:
    from PySide6 import QtWidgets
    from PySide6.QtGui import QImage
except ImportError:
    from PySide import QtGui as QtWidgets  # type: ignore
    from PySide.QtGui import QImage  # type: ignore


PART_PLANE_TYPE = f"{Part.__name__}::Plane"


class TestSelectionVisual(unittest.TestCase):
    """Verify that live preselection draws above selection overlays."""

    _COLOR_DELTA_MIN = 0.12
    _COLOR_DELTA_RESTORE_MAX = 0.05
    _PRESELECTION_OVERRIDE_DELTA_MIN = 0.08
    _UNCHANGED_FACE_DELTA_MAX = 0.08

    def setUp(self):
        self._clear_selection_state()
        self.doc = FreeCAD.newDocument("TestSelectionVisual")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)
        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.viewer = self.view.getViewer()

        self._had_axis_cross = self.view.hasAxisCross()
        self.view.setAxisCross(False)

        self._had_navi_cube = self.viewer.isEnabledNaviCube()
        self.viewer.setEnabledNaviCube(False)

    def tearDown(self):
        self._clear_selection_state()

        if getattr(self, "view", None) is not None:
            with suppress(Exception):
                self._flush_gui()

        with suppress(Exception):
            if getattr(self, "view", None) is not None:
                self.view.setAxisCross(self._had_axis_cross)

        self._set_navi_cube_enabled(self._had_navi_cube)

        if FreeCAD.getDocument(self.doc.Name):
            FreeCAD.closeDocument(self.doc.Name)
            self._process_gui_events()

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
            self._PRESELECTION_OVERRIDE_DELTA_MIN,
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

    def test_real_face_selection_only_colors_selected_face(self):
        box = self.doc.addObject("Part::Box", "Box")
        box.Length = 40
        box.Width = 40
        box.Height = 20
        box.ViewObject.ShapeColor = (0.66, 0.66, 0.74)
        self.doc.recompute()

        self.view.viewAxonometric()
        self._set_orthographic_if_supported()
        self.view.fitAll()
        self._flush_gui()

        base_image = self._saved_image()

        Selection.addSelection(self.doc.Name, box.Name, "Face6")
        self._flush_gui()
        selected_image = self._saved_image()

        selected_face_point = (256, 180)
        left_face_point = (190, 360)
        right_face_point = (380, 360)

        self.assertGreater(
            self._color_distance(
                self._pixel_color(base_image, selected_face_point),
                self._pixel_color(selected_image, selected_face_point),
            ),
            self._COLOR_DELTA_MIN,
            msg="Selecting Face6 did not visibly color the selected top face.",
        )
        for point in (left_face_point, right_face_point):
            self.assertLess(
                self._color_distance(
                    self._pixel_color(base_image, point),
                    self._pixel_color(selected_image, point),
                ),
                self._UNCHANGED_FACE_DELTA_MAX,
                msg=f"Selecting Face6 unexpectedly changed an adjacent face at {point}.",
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

    def _process_gui_events(self):
        for _ in range(4):
            FreeCADGui.updateGui()
            QtWidgets.QApplication.processEvents()
            time.sleep(0.05)

    def _clear_selection_state(self):
        with suppress(Exception):
            Selection.clearSelection()
        with suppress(Exception):
            Selection.clearPreselection()

    def _center_pixel_color(self):
        image = self._saved_image()
        return self._pixel_color(image, (image.width() // 2, image.height() // 2))

    def _saved_image(self):
        handle = tempfile.NamedTemporaryFile(
            prefix="FreeCAD-TestSelectionVisual-", suffix=".png", delete=False
        )
        path = handle.name
        handle.close()
        try:
            self.view.saveImage(path, 512, 512, "White")
            image = QImage(path)
        finally:
            with suppress(OSError):
                os.unlink(path)

        if image.isNull():
            self.skipTest("ActiveView.saveImage did not produce a readable image")

        return image

    @staticmethod
    def _pixel_color(image, point):
        color = image.pixelColor(point[0], point[1])
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
