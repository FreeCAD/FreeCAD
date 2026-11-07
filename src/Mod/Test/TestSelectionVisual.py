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
    _CENTER_SAMPLE_RADIUS = 2
    _SELECTION_COLOR = 0x00FF00FF
    _HIGHLIGHT_COLOR = 0xFF00FFFF

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestSelectionVisual")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)
        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.viewer = self.view.getViewer()
        self._had_axis_cross = self.view.hasAxisCross()
        self.view.setAxisCross(False)

        self._had_navi_cube = self.viewer.isEnabledNaviCube()
        self.viewer.setEnabledNaviCube(False)

        self._view_preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
        self._saved_colors = {}
        for key, value in (
            ("SelectionColor", self._SELECTION_COLOR),
            ("HighlightColor", self._HIGHLIGHT_COLOR),
        ):
            self._saved_colors[key] = (
                key in self._view_preferences.GetUnsigneds(),
                self._view_preferences.GetUnsigned(key, 0),
            )
            self._view_preferences.SetUnsigned(key, value)
        self._flush_gui()

    def tearDown(self):
        with suppress(Exception):
            Selection.clearPreselection()
        with suppress(Exception):
            Selection.clearSelection()

        if hasattr(self, "_saved_colors"):
            for key, (was_set, value) in self._saved_colors.items():
                if was_set:
                    self._view_preferences.SetUnsigned(key, value)
                else:
                    self._view_preferences.RemUnsigned(key)

        with suppress(Exception):
            self.view.setAxisCross(self._had_axis_cross)

        self._set_navi_cube_enabled(self._had_navi_cube)

        if FreeCAD.getDocument(self.doc.Name):
            FreeCAD.closeDocument(self.doc.Name)

    def test_preselection_overrides_selection_overlay(self):
        plane = self._create_test_plane()
        self._prepare_view()

        base_color = self._center_region_color()

        Selection.addSelection(plane)
        self._flush_gui()
        selection_color = self._center_region_color()

        Selection.setPreselection(plane, "Face1")
        self._flush_gui()
        preselection_color = self._center_region_color()

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

    def test_whole_preselection_does_not_override_whole_selection(self):
        plane = self._create_test_plane()
        self._prepare_view()

        base_color = self._center_region_color()

        Selection.addSelection(plane)
        self._flush_gui()
        selection_color = self._center_region_color()

        Selection.setPreselection(plane, tp=2)  # SelectionChanges::MsgSource::TreeView
        self._flush_gui()
        overlapping_color = self._center_region_color()

        Selection.clearPreselection()
        Selection.clearSelection()
        Selection.setPreselection(plane, "Face1")
        self._flush_gui()
        highlight_color = self._center_region_color()

        self._assert_color_changed(
            base_color,
            selection_color,
            "Whole-object selection did not visibly change the rendered face.",
        )
        self._assert_color_changed(
            selection_color,
            highlight_color,
            "Whole-object selection and preselection colors were not distinguishable.",
        )
        selection_distance = self._color_distance(overlapping_color, selection_color)
        highlight_distance = self._color_distance(overlapping_color, highlight_color)
        self.assertGreater(
            highlight_distance - selection_distance,
            self._COLOR_DELTA_MIN,
            msg=(
                "Whole-object preselection color won over committed selection. "
                f"selection={selection_color}, overlap={overlapping_color}, "
                f"highlight={highlight_color}"
            ),
        )

    def test_whole_preselection_is_not_suppressed_by_partial_selection(self):
        shape = self.doc.addObject("Part::Feature", "TwoFaces")
        shape.Shape = Part.makeCompound(
            [
                Part.makePlane(40, 40, FreeCAD.Vector(-45, -20, 0)),
                Part.makePlane(40, 40, FreeCAD.Vector(5, -20, 0)),
            ]
        )
        shape.ViewObject.ShapeColor = (0.66, 0.66, 0.74)
        self.doc.recompute()
        self._prepare_view()

        unselected_face_color = self._region_color(0.72, 0.5)

        Selection.addSelection(shape, "Face1")
        self._flush_gui()
        partially_selected_color = self._region_color(0.72, 0.5)

        Selection.setPreselection(shape, tp=2)  # SelectionChanges::MsgSource::TreeView
        self._flush_gui()
        tree_preselection_color = self._region_color(0.72, 0.5)

        self._assert_color_restored(
            unselected_face_color,
            partially_selected_color,
            "Selecting Face1 unexpectedly changed the unselected face.",
        )
        self._assert_color_changed(
            partially_selected_color,
            tree_preselection_color,
            "Partial selection incorrectly suppressed whole-object tree preselection.",
        )

    def test_preselection_preserves_object_transparency(self):
        plane = self._create_test_plane()
        plane.ViewObject.Transparency = 70
        self._prepare_view()

        transparent_base_color = self._center_region_color()

        Selection.setPreselection(plane, "Face1")
        self._flush_gui()
        transparent_highlight_color = self._center_region_color()

        Selection.clearPreselection()
        plane.ViewObject.Transparency = 0
        self._flush_gui()
        Selection.setPreselection(plane, "Face1")
        self._flush_gui()
        opaque_highlight_color = self._center_region_color()

        self._assert_color_changed(
            transparent_base_color,
            transparent_highlight_color,
            "Preselection did not visibly highlight the transparent face.",
        )
        self.assertGreater(
            self._color_distance(transparent_highlight_color, opaque_highlight_color),
            self._COLOR_DELTA_MIN,
            msg=(
                "Preselection made the transparent face opaque. "
                f"transparent={transparent_highlight_color}, opaque={opaque_highlight_color}"
            ),
        )

    def test_link_preselection_preserves_source_transparency(self):
        source = self._create_test_plane()
        source.ViewObject.Transparency = 70

        link = self.doc.addObject("App::Link", "PlaneLink")
        link.LinkedObject = source
        source.ViewObject.Visibility = False
        self.doc.recompute()
        self._prepare_view()

        transparent_base_color = self._center_region_color()

        Selection.setPreselection(link, "Face1")
        self._flush_gui()
        transparent_highlight_color = self._center_region_color()

        Selection.clearPreselection()
        source.ViewObject.Transparency = 0
        self._flush_gui()
        Selection.setPreselection(link, "Face1")
        self._flush_gui()
        opaque_highlight_color = self._center_region_color()

        self._assert_color_changed(
            transparent_base_color,
            transparent_highlight_color,
            "Preselection did not visibly highlight the transparent Link.",
        )
        self.assertGreater(
            self._color_distance(transparent_highlight_color, opaque_highlight_color),
            self._COLOR_DELTA_MIN,
            msg=(
                "Link preselection ignored source transparency. "
                f"transparent={transparent_highlight_color}, opaque={opaque_highlight_color}"
            ),
        )

    def test_selection_can_be_cleared(self):
        plane = self._create_test_plane()
        self._prepare_view()

        base_color = self._center_region_color()

        Selection.addSelection(plane)
        self._flush_gui()
        selection_color = self._center_region_color()

        Selection.clearSelection()
        self._flush_gui()
        cleared_color = self._center_region_color()

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

        base_color = self._center_region_color()

        Selection.setPreselection(plane, "Face1")
        self._flush_gui()
        preselection_color = self._center_region_color()

        Selection.clearPreselection()
        self._flush_gui()
        cleared_color = self._center_region_color()

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

    def test_tree_preselection_is_on_top_but_committed_selection_is_depth_tested(self):
        front = self.doc.addObject("Part::Box", "Front")
        front.Length = 40
        front.Width = 40
        front.Height = 10
        front.Placement.Base = FreeCAD.Vector(0, 0, 10)
        front.ViewObject.ShapeColor = (0.2, 0.2, 0.8)

        back = self.doc.addObject("Part::Box", "Back")
        back.Length = 40
        back.Width = 40
        back.Height = 10
        back.ViewObject.ShapeColor = (0.8, 0.8, 0.2)
        self.doc.recompute()
        self._prepare_view()

        base_color = self._center_region_color()

        Selection.addSelection(back)
        self._flush_gui()
        committed_selection_color = self._center_region_color()
        self.assertLess(
            self._color_distance(base_color, committed_selection_color),
            self._COLOR_DELTA_RESTORE_MAX,
            msg=(
                "Committed selection exposed an occluded face. "
                f"base={base_color}, selection={committed_selection_color}"
            ),
        )

        Selection.clearSelection()
        Selection.setPreselection(back, tp=2)  # SelectionChanges::MsgSource::TreeView
        self._flush_gui()
        tree_preselection_color = self._center_region_color()
        self.assertGreater(
            self._color_distance(base_color, tree_preselection_color),
            self._COLOR_DELTA_MIN,
            msg=(
                "Tree preselection remained depth-tested. "
                f"base={base_color}, preselection={tree_preselection_color}"
            ),
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

    def _center_region_color(self):
        return self._region_color(0.5, 0.5)

    def _region_color(self, x_fraction, y_fraction):
        image = self.viewer.grabFramebuffer()
        radius = self._CENTER_SAMPLE_RADIUS
        self.assertGreater(image.width(), 2 * radius)
        self.assertGreater(image.height(), 2 * radius)

        center_x = round((image.width() - 1) * x_fraction)
        center_y = round((image.height() - 1) * y_fraction)
        totals = [0.0, 0.0, 0.0]
        count = 0
        for y in range(center_y - radius, center_y + radius + 1):
            for x in range(center_x - radius, center_x + radius + 1):
                color = image.pixelColor(x, y)
                totals[0] += color.redF()
                totals[1] += color.greenF()
                totals[2] += color.blueF()
                count += 1
        return tuple(channel / count for channel in totals)

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
