# SPDX-License-Identifier: LGPL-2.1-or-later

import math
from collections.abc import Sequence
from typing import Any, Literal, TypeAlias

import FreeCAD
import FreeCADGui
import Part
import Sketcher
import SketcherGui
from SketcherTests.Support import SketcherGuiTestCase

CoinPoint: TypeAlias = tuple[int, int]
"""A point in FreeCAD's physical viewport coordinate system."""

PreselectionInfo: TypeAlias = dict[str, Any]
"""The mapping returned by ``SketcherGui.getActiveSketchPreselection``."""

PreselectionKind: TypeAlias = Literal[
    "target_constraint",
    "other_constraint",
    "edge",
    "vertex",
    "other",
    "none",
]
"""Classification returned for a Sketcher preselection probe."""


def classify_preselection(
    info: PreselectionInfo | None,
    expected_constraint_name: str,
) -> PreselectionKind:
    """Classify one Sketcher preselection result for a probe assertion."""
    if not info or not info.get("ObjectName"):
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


def scan_preselection_at_viewport(
    center_coin: CoinPoint,
    expected_constraint_name: str,
    span: int = 16,
    step: int = 2,
) -> dict[PreselectionKind, int]:
    """Count Sketcher preselection classifications around a point."""
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
            counts[classify_preselection(info, expected_constraint_name)] += 1
    return counts


def find_constraint_probe_viewport_point(
    view: Any,
    seed_world_point: Any,
    expected_constraint_name: str,
    span: int = 64,
    step: int = 8,
) -> CoinPoint | None:
    """Find the center of a visible target-constraint preselection area."""
    center_coin = tuple(int(value) for value in view.getPointOnViewport(seed_world_point))
    sum_x = 0
    sum_y = 0
    target_count = 0

    for dy in range(-span, span + 1, step):
        for dx in range(-span, span + 1, step):
            coin_point = (center_coin[0] + dx, center_coin[1] + dy)
            info = SketcherGui.getActiveSketchPreselection(coin_point)
            if classify_preselection(info, expected_constraint_name) != "target_constraint":
                continue
            sum_x += coin_point[0]
            sum_y += coin_point[1]
            target_count += 1

    if target_count == 0:
        return None
    return (int(round(sum_x / target_count)), int(round(sum_y / target_count)))


def wait_for_constraint_probe_viewport_point(
    test_case: SketcherGuiTestCase,
    view: Any,
    seed_world_point: Any,
    expected_constraint_name: str,
) -> CoinPoint:
    """Wait until a target constraint has a usable viewport probe point."""
    probe_point: list[CoinPoint] = []

    def find_probe_point() -> bool:
        point = find_constraint_probe_viewport_point(
            view,
            seed_world_point,
            expected_constraint_name,
        )
        if point is None:
            return False
        probe_point[:] = [point]
        return True

    test_case.assertTrue(
        test_case.gui.wait_until(
            find_probe_point,
            timeout_ms=1000,
            description="constraint preselection probe point",
        ),
        "Expected the constraint preselection scene to become available",
    )
    return probe_point[0]


def preselection_offsets(
    center_coin: CoinPoint,
    expected_constraint_name: str,
    expected_kind: PreselectionKind,
    span: int,
    step: int,
) -> list[CoinPoint]:
    """Return offsets whose preselection has the expected classification."""
    offsets: list[CoinPoint] = []
    for dy in range(-span, span + 1, step):
        for dx in range(-span, span + 1, step):
            coin_point = (center_coin[0] + dx, center_coin[1] + dy)
            info = SketcherGui.getActiveSketchPreselection(coin_point)
            if classify_preselection(info, expected_constraint_name) == expected_kind:
                offsets.append((dx, dy))
    return offsets


def wait_for_preselection_offsets(
    test_case: SketcherGuiTestCase,
    center_coin: CoinPoint,
    expected_constraint_name: str,
    expected_kind: PreselectionKind,
    span: int,
    step: int,
) -> list[CoinPoint]:
    """Wait until at least one expected preselection offset is available."""
    offsets: list[CoinPoint] = []

    def find_offsets() -> bool:
        offsets[:] = preselection_offsets(
            center_coin,
            expected_constraint_name,
            expected_kind,
            span,
            step,
        )
        return bool(offsets)

    test_case.assertTrue(
        test_case.gui.wait_until(
            find_offsets,
            timeout_ms=1000,
            description=f"{expected_kind} preselection offsets",
        ),
        f"Expected at least one {expected_kind} preselection point",
    )
    return offsets


def wait_for_preselection_results(
    test_case: SketcherGuiTestCase,
    center_coin: CoinPoint,
    offsets: Sequence[CoinPoint],
    expected_constraint_name: str,
    expected_kind: PreselectionKind,
) -> list[tuple[int, int, PreselectionKind, PreselectionInfo | None]]:
    """Wait until all probe points have the expected classification."""
    results: list[tuple[int, int, PreselectionKind, PreselectionInfo | None]] = []

    def find_results() -> bool:
        results[:] = []
        for dx, dy in offsets:
            coin_point = (center_coin[0] + dx, center_coin[1] + dy)
            info = SketcherGui.getActiveSketchPreselection(coin_point)
            kind = classify_preselection(info, expected_constraint_name)
            results.append((dx, dy, kind, info))
        return bool(results) and all(result[2] == expected_kind for result in results)

    test_case.assertTrue(
        test_case.gui.wait_until(
            find_results,
            timeout_ms=1000,
            description=f"{expected_kind} preselection results",
        ),
        f"Expected all preselection points to classify as {expected_kind}",
    )
    return results


def configure_view_state(view: Any, tilt: Any | None = None) -> None:
    """Set a standard Sketcher view and optionally apply a camera tilt."""
    view.viewTop()
    view.fitAll()
    if tilt is not None:
        base_rotation = view.getCameraOrientation()
        view.setCameraOrientation(tilt.multiply(base_rotation))
        view.fitAll()


class SketcherGuiTestCases(SketcherGuiTestCase):

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
        super().setUp()
        self.doc = FreeCAD.newDocument("SketchGuiTest")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.doc.recompute()

        self.enter_sketch_edit(self.doc, self.sketch)

        self.view = FreeCADGui.ActiveDocument.ActiveView

    def testPointOnObjectPreselectionMatchesTiltedHitArea(self):
        constraint_id, self.probe_point = self.build_issue_25840_sketch(self.sketch)
        self.expected_constraint_name = f"Constraint{constraint_id + 1}"
        self.doc.recompute()

        tilt_y = FreeCAD.Rotation(FreeCAD.Vector(0, 1, 0), 2.0)

        counts_by_state = {}
        for name, tilt in {
            "exact_top": None,
            "tilt_y_2deg": tilt_y,
        }.items():
            configure_view_state(self.view, tilt)
            probe_point = wait_for_constraint_probe_viewport_point(
                self,
                self.view,
                self.probe_point,
                self.expected_constraint_name,
            )
            counts_by_state[name] = scan_preselection_at_viewport(
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

        configure_view_state(self.view)

        marker_coin = tuple(int(value) for value in self.view.getPointOnViewport(marker_point))

        vertex_offsets = wait_for_preselection_offsets(
            self,
            marker_coin,
            "Constraint0",
            "vertex",
            span=12,
            step=2,
        )

        constraint_id = self.sketch.addConstraint(
            Sketcher.Constraint("Distance", line_id, 1, line_id, 2, 40.0)
        )
        self.sketch.setLabelDistance(constraint_id, -12.0 * math.sqrt(2.0))
        self.sketch.setLabelPosition(constraint_id, 0.0)
        self.expected_constraint_name = f"Constraint{constraint_id + 1}"
        self.doc.recompute()

        constraint_probe = wait_for_constraint_probe_viewport_point(
            self,
            self.view,
            marker_point,
            self.expected_constraint_name,
        )

        self.assertTrue(
            self.gui.wait_until(
                lambda: classify_preselection(
                    SketcherGui.getActiveSketchPreselection(marker_coin),
                    self.expected_constraint_name,
                )
                == "vertex",
                timeout_ms=1000,
                description="marker vertex preselection",
            ),
            "Expected the marker to remain preselectable as a vertex",
        )
        marker_info = SketcherGui.getActiveSketchPreselection(marker_coin)
        marker_kind = classify_preselection(marker_info, self.expected_constraint_name)

        probe_results = wait_for_preselection_results(
            self,
            marker_coin,
            vertex_offsets,
            self.expected_constraint_name,
            "vertex",
        )

        unexpected_probe_results = [result for result in probe_results if result[2] != "vertex"]

        detail = (
            f"marker_info={marker_info}, vertex_offsets={vertex_offsets}, "
            f"probe_results={probe_results}, marker_coin={marker_coin}, "
            f"constraint_probe={constraint_probe}"
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

        configure_view_state(self.view)

        midpoint_coin = tuple(int(value) for value in self.view.getPointOnViewport(midpoint))

        edge_offsets = wait_for_preselection_offsets(
            self,
            midpoint_coin,
            "Constraint0",
            "edge",
            span=16,
            step=2,
        )

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

        constraint_probe = wait_for_constraint_probe_viewport_point(
            self,
            self.view,
            midpoint,
            self.expected_constraint_name,
        )

        probe_results = wait_for_preselection_results(
            self,
            midpoint_coin,
            edge_offsets,
            self.expected_constraint_name,
            "edge",
        )

        unexpected_probe_results = [result for result in probe_results if result[2] != "edge"]

        detail = (
            f"edge_offsets={edge_offsets}, probe_results={probe_results}, "
            f"midpoint_coin={midpoint_coin}, constraint_probe={constraint_probe}"
        )

        self.assertGreater(len(edge_offsets), 0, detail)
        self.assertEqual(unexpected_probe_results, [], detail)
