# SPDX-License-Identifier: LGPL-2.1-or-later

"""GUI visual regression test for Coin selection/preselection ordering.

Run with:
    FreeCAD -t TestCoinSelectionVisual
"""

from contextlib import suppress
import os
import tempfile
import time
import unittest

import FreeCAD
import FreeCADGui
import Part
from pivy import coin

try:
    from PySide6 import QtWidgets
    from PySide6.QtGui import QImage
except ImportError:
    from PySide import QtGui as QtWidgets  # type: ignore
    from PySide.QtGui import QImage  # type: ignore


PART_PLANE_TYPE = f"{Part.__name__}::Plane"
Selection = getattr(FreeCADGui, "Selection", None)


class TestCoinSelectionVisual(unittest.TestCase):
    """Verify that live preselection draws above Coin selection overlays."""

    _COLOR_DELTA_MIN = 0.12
    _COLOR_DELTA_RESTORE_MAX = 0.05
    _LUMINANCE_DELTA_MAX = 0.04
    _UNCHANGED_FACE_DELTA_MAX = 0.08
    _SELECTION_ORANGE = (247.0 / 255.0, 103.0 / 255.0, 7.0 / 255.0)

    def setUp(self):
        global Selection
        if Selection is None:
            Selection = getattr(FreeCADGui, "Selection", None)
        if Selection is None:
            self.skipTest("FreeCADGui.Selection is not available")

        self._clear_selection_state()
        self.doc = FreeCAD.newDocument("TestCoinSelectionVisual")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)
        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.viewer = self.view.getViewer()
        self._path = None

        self._had_axis_cross = self.view.hasAxisCross()
        self.view.setAxisCross(False)

        self._had_navi_cube = self.viewer.isEnabledNaviCube()
        self.viewer.setEnabledNaviCube(False)

    def tearDown(self):
        if self._path is not None:
            with suppress(Exception):
                Selection.clearCoinSelection(self._path)
            with suppress(Exception):
                Selection.clearCoinHighlight(self._path)
            with suppress(Exception):
                self._path.unref()

        self._clear_selection_state()

        if getattr(self, "view", None) is not None:
            with suppress(Exception):
                self._flush_gui()

        with suppress(Exception):
            self.view.setAxisCross(self._had_axis_cross)

        self._set_navi_cube_enabled(self._had_navi_cube)

        if FreeCAD.getDocument(self.doc.Name):
            FreeCAD.closeDocument(self.doc.Name)

    def test_preselection_overrides_coin_selection_overlay(self):
        plane = self._create_test_plane()
        self._prepare_view()
        self._path = self._find_scene_path(plane.ViewObject.RootNode)

        base_color = self._center_pixel_color()

        Selection.applyCoinSelection(
            self._path,
            mode=Selection.SelectionActionMode.All,
            color=(0.0, 0.6, 0.0),
        )
        self._flush_gui()
        selection_color = self._center_pixel_color()

        Selection.setPreselection(plane, "Face1")
        self._flush_gui()
        preselection_color = self._center_pixel_color()

        self.assertGreater(
            self._color_distance(base_color, selection_color),
            self._COLOR_DELTA_MIN,
            msg=(
                "Coin selection overlay did not visibly change the rendered face. "
                f"base={base_color}, selection={selection_color}"
            ),
        )
        self.assertGreater(
            self._color_distance(selection_color, preselection_color),
            self._COLOR_DELTA_MIN,
            msg=(
                "Preselection did not visibly override the Coin selection overlay. "
                f"selection={selection_color}, preselection={preselection_color}"
            ),
        )

    def test_coin_selection_can_be_cleared(self):
        plane = self._create_test_plane()
        self._prepare_view()
        self._path = self._find_scene_path(plane.ViewObject.RootNode)

        base_color = self._center_pixel_color()

        Selection.applyCoinSelection(
            self._path,
            mode=Selection.SelectionActionMode.All,
            color=(0.0, 0.6, 0.0),
        )
        self._flush_gui()
        selection_color = self._center_pixel_color()

        Selection.clearCoinSelection(self._path)
        self._flush_gui()
        cleared_color = self._center_pixel_color()

        self._assert_color_changed(
            base_color,
            selection_color,
            "Coin selection overlay did not visibly change the rendered face.",
        )
        self._assert_color_restored(
            base_color,
            cleared_color,
            "Clearing Coin selection did not restore the original rendering.",
        )

    def test_coin_highlight_can_be_cleared(self):
        plane = self._create_test_plane()
        self._prepare_view()
        self._path = self._find_scene_path(plane.ViewObject.RootNode)

        base_color = self._center_pixel_color()

        Selection.applyCoinHighlight(self._path, color=(0.85, 0.2, 0.2))
        self._flush_gui()
        highlight_color = self._center_pixel_color()

        Selection.clearCoinHighlight(self._path)
        self._flush_gui()
        cleared_color = self._center_pixel_color()

        self._assert_color_changed(
            base_color,
            highlight_color,
            "Coin highlight did not visibly change the rendered face.",
        )
        self._assert_color_restored(
            base_color,
            cleared_color,
            "Clearing Coin highlight did not restore the original rendering.",
        )

    def test_same_color_coin_highlight_keeps_selection_shading(self):
        plane = self._create_test_plane()
        self._prepare_view()
        self._path = self._find_scene_path(plane.ViewObject.RootNode)

        Selection.applyCoinSelection(
            self._path,
            mode=Selection.SelectionActionMode.All,
            color=self._SELECTION_ORANGE,
        )
        self._flush_gui()
        selection_color = self._center_pixel_color()

        Selection.applyCoinHighlight(self._path, color=self._SELECTION_ORANGE)
        self._flush_gui()
        highlight_color = self._center_pixel_color()

        selection_luma = self._relative_luminance(selection_color)
        highlight_luma = self._relative_luminance(highlight_color)
        self.assertLess(
            abs(highlight_luma - selection_luma),
            self._LUMINANCE_DELTA_MAX,
            msg=(
                "Coin highlight should keep comparable shading when it uses the selection color. "
                f"selection={selection_color} ({selection_luma}), "
                f"highlight={highlight_color} ({highlight_luma})"
            ),
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
        left_face_point = (190, 300)
        right_face_point = (350, 300)

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

    def _clear_selection_state(self):
        with suppress(Exception):
            Selection.clearSelection()
        with suppress(Exception):
            Selection.clearPreselection()

    def _find_scene_path(self, root_node):
        search = coin.SoSearchAction()
        search.setNode(root_node)
        search.setSearchingAll(False)
        search.apply(self.view.getSceneGraph())

        path = search.getPath()
        self.assertIsNotNone(
            path, "Could not find the object root node in the active 3D scene graph"
        )
        # Retain the returned path; SoSearchAction owns it otherwise.
        path.ref()
        return path

    def _center_pixel_color(self):
        image = self._saved_image()
        return self._pixel_color(image, (image.width() // 2, image.height() // 2))

    def _saved_image(self):
        handle = tempfile.NamedTemporaryFile(
            prefix="FreeCAD-TestCoinSelectionVisual-", suffix=".png", delete=False
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

    @staticmethod
    def _relative_luminance(color):
        r, g, b = color
        return 0.2126 * r + 0.7152 * g + 0.0722 * b
