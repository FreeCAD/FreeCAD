# SPDX-License-Identifier: LGPL-2.1-or-later

import math
import time
import unittest

import FreeCAD
import FreeCADGui
import Part
import Sketcher
import SketcherGui
from PySide6 import QtCore, QtWidgets


class SketcherGuiTestCases(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not FreeCAD.GuiUp:
            raise unittest.SkipTest("Cannot run GUI tests in a CLI environment.")

        FreeCADGui.getMainWindow().show()
        cls.pump_gui_events()

    @staticmethod
    def pump_gui_events(iterations=6, delay=0.01):
        app = QtWidgets.QApplication.instance()
        for _ in range(iterations):
            app.processEvents(QtCore.QEventLoop.AllEvents, int(delay * 1000))
            if delay > 0.0:
                time.sleep(delay)

    @staticmethod
    def build_issue_25840_sketch(sketch):
        # Mirrors the uploaded repro geometry from issue #25840.
        first_line = sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(-17.60407066, 31.05172348, 0.0),
                FreeCAD.Vector(44.00962448, -33.86270142, 0.0),
            ),
            False,
        )
        second_line = sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(50.4888, 6.2351, 0.0),
                FreeCAD.Vector(30.973626440922192, -20.12834717, 0.0),
            ),
            False,
        )
        constraint_id = sketch.addConstraint(
            Sketcher.Constraint("PointOnObject", second_line, 2, first_line)
        )
        return constraint_id, FreeCAD.Vector(30.973626440922192, -20.12834717, 0.0)

    @staticmethod
    def classify_preselection(info, expected_constraint_name):
        if not info or not info["ObjectName"]:
            return "none"

        names = info.get("SubElementNames") or []
        if expected_constraint_name in names:
            return "target_constraint"
        if any(name.startswith("Constraint") for name in names):
            return "other_constraint"
        if any(name.startswith("Vertex") for name in names):
            return "vertex"
        if any(name.startswith("Edge") for name in names):
            return "edge"
        return "other"

    @classmethod
    def scan_preselection_at_viewport(cls, center_coin, expected_constraint_name, span=16, step=2):
        counts = {
            "target_constraint": 0,
            "other_constraint": 0,
            "edge": 0,
            "vertex": 0,
            "other": 0,
            "none": 0,
        }

        for dy in range(-span, span + 1, step):
            for dx in range(-span, span + 1, step):
                coin_point = (center_coin[0] + dx, center_coin[1] + dy)
                info = SketcherGui.getActiveSketchPreselection(coin_point)
                kind = cls.classify_preselection(info, expected_constraint_name)
                counts[kind] += 1

        return counts

    @classmethod
    def find_constraint_probe_viewport_point(
        cls,
        view,
        seed_world_point,
        expected_constraint_name,
        span=64,
        step=8,
    ):
        center_coin = tuple(int(value) for value in view.getPointOnViewport(seed_world_point))
        sum_x = 0
        sum_y = 0
        target_count = 0

        for dy in range(-span, span + 1, step):
            for dx in range(-span, span + 1, step):
                coin_point = (center_coin[0] + dx, center_coin[1] + dy)
                info = SketcherGui.getActiveSketchPreselection(coin_point)
                if cls.classify_preselection(info, expected_constraint_name) != "target_constraint":
                    continue

                sum_x += coin_point[0]
                sum_y += coin_point[1]
                target_count += 1

        if target_count == 0:
            return None

        return (
            int(round(sum_x / target_count)),
            int(round(sum_y / target_count)),
        )

    @classmethod
    def configure_view_state(cls, view, tilt=None):
        view.viewTop()
        cls.pump_gui_events(2)
        view.fitAll()
        cls.pump_gui_events(4)

        if tilt is not None:
            base_rotation = view.getCameraOrientation()
            view.setCameraOrientation(tilt.multiply(base_rotation))
            cls.pump_gui_events(3)
            view.fitAll()
            cls.pump_gui_events(4)

    @staticmethod
    def constraint_share(counts):
        total_hits = (
            counts["target_constraint"]
            + counts["other_constraint"]
            + counts["edge"]
            + counts["vertex"]
            + counts["other"]
        )
        if total_hits == 0:
            return 0.0
        return counts["target_constraint"] / total_hits

    def setUp(self):
        self.doc = FreeCAD.newDocument("SketchGuiTest")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.doc.recompute()

        FreeCADGui.getMainWindow().show()
        self.pump_gui_events()
        FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)
        self.pump_gui_events(6)

        self.view = FreeCADGui.ActiveDocument.ActiveView

    def tearDown(self):
        FreeCADGui.Selection.clearPreselection()
        FreeCADGui.Selection.clearSelection()
        if FreeCADGui.ActiveDocument:
            FreeCADGui.ActiveDocument.resetEdit()
        self.pump_gui_events(4, 0.01)

        if self.doc is not None:
            document_name = self.doc.Name
            self.doc = None
            FreeCAD.closeDocument(document_name)
            self.pump_gui_events(4, 0.01)

    def testPointOnObjectPreselectionMatchesTiltedHitArea(self):
        constraint_id, self.probe_point = self.build_issue_25840_sketch(self.sketch)
        self.expected_constraint_name = f"Constraint{constraint_id + 1}"
        self.doc.recompute()
        self.pump_gui_events(6)

        tilt_y = FreeCAD.Rotation(FreeCAD.Vector(0, 1, 0), 2.0)

        counts_by_state = {}
        for name, tilt in {
            "exact_top": None,
            "tilt_y_2deg": tilt_y,
        }.items():
            self.configure_view_state(self.view, tilt)
            probe_point = self.find_constraint_probe_viewport_point(
                self.view,
                self.probe_point,
                self.expected_constraint_name,
            )
            self.assertIsNotNone(probe_point)
            counts_by_state[name] = self.scan_preselection_at_viewport(
                probe_point,
                self.expected_constraint_name,
                span=12,
            )

        exact_top = counts_by_state["exact_top"]
        tilt_y = counts_by_state["tilt_y_2deg"]
        max_tilt_hits = tilt_y["target_constraint"]
        exact_top_share = self.constraint_share(exact_top)

        detail = (
            f"exact_top={exact_top}, tilt_y_2deg={tilt_y}, "
            f"constraint_share={exact_top_share:.3f}"
        )

        self.assertGreater(max_tilt_hits, 0, detail)
        self.assertGreater(exact_top["target_constraint"], 0, detail)
        self.assertGreaterEqual(
            exact_top["target_constraint"],
            int(max_tilt_hits * 0.8),
            detail,
        )
        self.assertGreaterEqual(
            exact_top_share,
            0.20,
            detail,
        )

    def testPointMarkerWinsOverOverlappingConstraintLabel(self):
        start_point = FreeCAD.Vector(80.0, 100.0, 0.0)
        end_point = FreeCAD.Vector(120.0, 140.0, 0.0)
        marker_point = FreeCAD.Vector(92.0, 88.0, 0.0)

        line_id = self.sketch.addGeometry(
            Part.LineSegment(start_point, end_point),
            False,
        )
        self.sketch.addGeometry(Part.Point(marker_point), False)
        self.doc.recompute()
        self.pump_gui_events(6)

        self.configure_view_state(self.view)
        self.pump_gui_events(8)

        marker_coin = tuple(int(value) for value in self.view.getPointOnViewport(marker_point))

        vertex_offsets = []
        for dy in range(-12, 13, 2):
            for dx in range(-12, 13, 2):
                probe_coin = (marker_coin[0] + dx, marker_coin[1] + dy)
                probe_info = SketcherGui.getActiveSketchPreselection(probe_coin)
                probe_kind = self.classify_preselection(probe_info, "Constraint0")
                if probe_kind == "vertex":
                    vertex_offsets.append((dx, dy))

        constraint_id = self.sketch.addConstraint(
            Sketcher.Constraint("Distance", line_id, 1, line_id, 2, 40.0)
        )
        self.sketch.setLabelDistance(constraint_id, -12.0 * math.sqrt(2.0))
        self.sketch.setLabelPosition(constraint_id, 0.0)
        self.expected_constraint_name = f"Constraint{constraint_id + 1}"
        self.doc.recompute()
        self.pump_gui_events(12)

        marker_info = SketcherGui.getActiveSketchPreselection(marker_coin)
        marker_kind = self.classify_preselection(marker_info, self.expected_constraint_name)

        probe_results = []
        for dx, dy in vertex_offsets:
            probe_coin = (marker_coin[0] + dx, marker_coin[1] + dy)
            probe_info = SketcherGui.getActiveSketchPreselection(probe_coin)
            probe_kind = self.classify_preselection(probe_info, self.expected_constraint_name)
            probe_results.append((dx, dy, probe_kind, probe_info))

        unexpected_probe_results = [result for result in probe_results if result[2] != "vertex"]

        detail = (
            f"marker_info={marker_info}, vertex_offsets={vertex_offsets}, "
            f"probe_results={probe_results}, marker_coin={marker_coin}"
        )

        self.assertGreater(len(vertex_offsets), 0, detail)
        self.assertEqual(marker_kind, "vertex", detail)
        self.assertEqual(unexpected_probe_results, [], detail)

    def testCurveWinsOverOverlappingDistanceDimensionLine(self):
        start_point = FreeCAD.Vector(80.0, 100.0, 0.0)
        end_point = FreeCAD.Vector(130.0, 100.0, 0.0)
        midpoint = FreeCAD.Vector(105.0, 100.0, 0.0)

        line_id = self.sketch.addGeometry(
            Part.LineSegment(start_point, end_point),
            False,
        )
        self.doc.recompute()
        self.pump_gui_events(6)

        self.configure_view_state(self.view)
        self.pump_gui_events(8)

        midpoint_coin = tuple(int(value) for value in self.view.getPointOnViewport(midpoint))

        edge_offsets = []
        for dy in range(-10, 11, 2):
            for dx in range(-16, 17, 2):
                probe_coin = (midpoint_coin[0] + dx, midpoint_coin[1] + dy)
                probe_info = SketcherGui.getActiveSketchPreselection(probe_coin)
                probe_kind = self.classify_preselection(probe_info, "Constraint0")
                if probe_kind == "edge":
                    edge_offsets.append((dx, dy))

        constraint_id = self.sketch.addConstraint(
            Sketcher.Constraint(
                "Distance",
                line_id,
                1,
                line_id,
                2,
                start_point.distanceToPoint(end_point),
            )
        )
        self.sketch.setLabelDistance(constraint_id, 0.0)
        self.sketch.setLabelPosition(constraint_id, 0.0)
        self.expected_constraint_name = f"Constraint{constraint_id + 1}"
        self.doc.recompute()
        self.pump_gui_events(12)

        probe_results = []
        for dx, dy in edge_offsets:
            probe_coin = (midpoint_coin[0] + dx, midpoint_coin[1] + dy)
            probe_info = SketcherGui.getActiveSketchPreselection(probe_coin)
            probe_kind = self.classify_preselection(probe_info, self.expected_constraint_name)
            probe_results.append((dx, dy, probe_kind, probe_info))

        unexpected_probe_results = [result for result in probe_results if result[2] != "edge"]

        detail = (
            f"edge_offsets={edge_offsets}, probe_results={probe_results}, "
            f"midpoint_coin={midpoint_coin}"
        )

        self.assertGreater(len(edge_offsets), 0, detail)
        self.assertEqual(unexpected_probe_results, [], detail)
