# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tests for the BIM Stair Designer object model."""

from importlib import import_module
import math
import os
import tempfile
import unittest

import FreeCAD
import Part

from bimtests import TestArchBase
from stairdesigner.geometry_core import (
    BalancedSection,
    distribute_treads,
    straight_stair_metrics,
)
from stairdesigner.geometry_plan import (
    balanced_partition_is_valid,
    balanced_tread_faces,
    corner_post_sections,
    even_walking_line_goings,
    fit_balanced_sections_to_footprint,
    fit_tangent_sections_to_footprint,
    inner_corner_newel_sections,
    make_stair_footprint,
    make_tangent_stair_footprint,
    newel_fan_plan,
    newel_post_plan,
    newel_post_square,
    shift_section_ends,
    tangent_tread_faces,
    walking_line_points,
    walking_line_treads,
)
from stairdesigner.geometry_steps import (
    _section_band_faces,
    make_balanced_concrete_shape,
    make_balanced_riser_shape,
    make_balanced_tread_shape,
)
from stairdesigner.geometry_stringer_path import (
    _circular_stringer_data,
    _stringer_elevations,
    stringer_flight_runs,
)
from stairdesigner.geometry_stringer_shapes import make_housed_stringer_shape
from stairdesigner.geometry_tangent import tangent_flight_sections
from stairdesigner.geometry_winders import balanced_winder_sections
from stairdesigner.object_factory import make_stair, resize_flights
from stairdesigner.object_proxies import ViewProviderComponentGroup
from stairdesigner.object_stair import StairProxy
from stairdesigner.object_utils import get_flights, sync_all_flight_side_lengths
from stairdesigner.taskpanel_position import (
    _selected_vertices,
    _translated_stair_placement,
)


class TestArchStairDesigner(TestArchBase.TestArchBase):

    def test_inner_winder_stringer_and_newel_joints(self):
        stair = make_stair()
        flight = get_flights(stair)[0]
        stair.Proxy._updating = True
        # Check the plain boards against the steps they would be housing.
        stair.StringerJoinery = False
        stair.FloorHeight = 2800
        stair.NumberOfSteps = 15
        stair.Nosing = 30
        stair.StepRiserOverlap = 10
        stair.RiserThickness = 18
        stair.Proxy._updating = False
        for side in ("Left", "Right"):
            for end_with_riser in (False, True):
                with self.subTest(side=side, end_with_riser=end_with_riser):
                    flight.Proxy._updating = True
                    flight.LeftLength = 3500
                    flight.Width = 1000
                    flight.EntryDirection = "From " + side.lower()
                    flight.ExitDirection = "To " + side.lower()
                    flight.LeftHandrailEnabled = True
                    flight.RightHandrailEnabled = True
                    flight.LeftStringerType = "Housed stringer"
                    flight.RightStringerType = "Housed stringer"
                    flight.Proxy._updating = False
                    stair.Proxy._updating = True
                    stair.EndWithRiser = end_with_riser
                    stair.Proxy._updating = False
                    stair.Proxy.rebuild(stair)
                    self.assertEqual(stair.GeometryStatus, "Balanced entry/exit stair")
                    board = next(p for p in stair.StringersGroup.Group if p.Side == side)
                    board.Shape.check(True)
                    self.assertEqual(len(board.Shape.Solids), 1)
                    # The steps' ends and the board's centre line, within the
                    # stair's width by half a post.
                    y = 965.0 if side == "Left" else 35.0
                    spans = []
                    for x in (1050.0, 1400.0, 1750.0, 2100.0, 2450.0):
                        probe = Part.makeLine(FreeCAD.Vector(x, y, 0), FreeCAD.Vector(x, y, 4000))
                        section = board.Shape.common(probe)
                        self.assertTrue(section.Edges)
                        spans.append((section.BoundBox.ZMin, section.BoundBox.ZMax))
                    # The risers between the newels are balanced too, so the
                    # board follows their varying pitch: it climbs steadily,
                    # never back down.
                    for edge in (0, 1):
                        increments = [b[edge] - a[edge] for a, b in zip(spans, spans[1:])]
                        for increment in increments:
                            self.assertGreater(increment, 0.0)

                    posts = sorted(
                        (
                            p
                            for p in stair.HandrailsGroup.Group
                            if p.Side == side and p.StairDesignerRole == "HandrailPost"
                        ),
                        key=lambda p: p.Shape.BoundBox.XMin,
                    )
                    self.assertEqual(len(posts), 2)
                    self.assertGreaterEqual(
                        posts[-1].Shape.BoundBox.ZMax,
                        stair.FloorHeight.Value + stair.HandrailHeightAboveNosing.Value,
                    )
                    rail = next(
                        p
                        for p in stair.HandrailsGroup.Group
                        if p.Side == side and p.StairDesignerRole == "HandrailTopRail"
                    )
                    rail_tops = []
                    for x in (1250.0, 1750.0, 2250.0):
                        probe = Part.makeLine(FreeCAD.Vector(x, y, 0), FreeCAD.Vector(x, y, 5000))
                        rail_tops.append(rail.Shape.common(probe).BoundBox.ZMax)
                    board_rise = (spans[-1][1] - spans[0][1]) / 1400.0
                    # The straight rail between the newels follows the board's
                    # mean pitch; the board's own pitch varies with the balance.
                    self.assertAlmostEqual(
                        (rail_tops[-1] - rail_tops[0]) / 1000.0, board_rise, delta=0.05
                    )
                    picket_lengths = [
                        p.Shape.BoundBox.ZLength
                        for p in stair.HandrailsGroup.Group
                        if p.Side == side and p.StairDesignerRole == "HandrailPicket"
                    ]
                    self.assertGreater(len(picket_lengths), 2)
                    # So the pickets standing on the board vary a little.
                    self.assertLess(max(picket_lengths) - min(picket_lengths), 75.0)
                    self.assertGreater(
                        posts[-1].Shape.BoundBox.ZMax - rail.Shape.BoundBox.ZMax, 200.0
                    )
                    strip = Part.makeBox(3500, 0.002, 4000, FreeCAD.Vector(0, y - 0.001, 0))
                    treads = sorted(
                        (p for p in stair.StepsGroup.Group if p.StairDesignerRole == "Tread"),
                        key=lambda p: p.Index,
                    )
                    clearances = []
                    for tread in treads[5:10]:
                        # Where the tread meets the board: balanced treads
                        # start earlier on their outer side.
                        x = tread.Shape.common(strip).BoundBox.XMin
                        top = self._top_at(board.Shape, x, y)
                        # Square to the board's edge, at its local pitch.
                        pitch = (
                            self._top_at(board.Shape, x + 10.0, y)
                            - self._top_at(board.Shape, x - 10.0, y)
                        ) / 20.0
                        clearance = (top - tread.Shape.BoundBox.ZMax) / math.sqrt(1 + pitch**2)
                        clearances.append(clearance)
                        self.assertGreaterEqual(clearance, board.NosingOffset.Value - 1e-5)
                        self.assertLessEqual(clearance, board.NosingOffset.Value + 20.0)
                    # The board's pitches are fitted over the nosings: the
                    # closest one is within a couple of millimetres.
                    self.assertAlmostEqual(min(clearances), board.NosingOffset.Value, delta=2.0)
                    housing = Part.makeBox(
                        1500, 20, 4000, FreeCAD.Vector(1000, 980 if side == "Left" else 0, 0)
                    )
                    support = board.Shape.multiFuse([p.Shape for p in posts])
                    for tread in treads:
                        exposed = tread.Shape.common(housing).cut(support)
                        self.assertLess(exposed.Volume, 1e-4, f"Exposed tread {tread.Index}")

                    # The first/last winders meet the actual posts. The fans,
                    # wholly at a post on this side, must not emerge beyond
                    # its flight face; a winder leaving the post is housed in
                    # the board, as checked above.
                    def is_fan(tread, post):
                        inner = tread.Shape.common(strip).BoundBox
                        return inner.XLength <= post.Shape.BoundBox.XLength + 0.01

                    for tread in treads[:4]:
                        self.assertGreater(tread.Shape.common(posts[0].Shape).Volume, 1e-5)
                        if is_fan(tread, posts[0]):
                            self.assertLessEqual(
                                tread.Shape.common(strip).BoundBox.XMax,
                                posts[0].Shape.BoundBox.XMax + 0.01,
                            )
                    for tread in treads[-4:]:
                        self.assertGreater(tread.Shape.common(posts[-1].Shape).Volume, 1e-5)
                        if is_fan(tread, posts[-1]):
                            self.assertGreaterEqual(
                                tread.Shape.common(strip).BoundBox.XMin,
                                posts[-1].Shape.BoundBox.XMin - 0.01,
                            )

    def test_newel_stringer_custom_width_is_preserved(self):
        stair = make_stair()
        flight = get_flights(stair)[0]
        flight.Proxy._updating = True
        flight.EntryDirection = "From left"
        flight.ExitDirection = "To left"
        flight.LeftStringerType = "Housed stringer"
        flight.Proxy._updating = False
        stair.Proxy.rebuild(stair)
        board = next(p for p in stair.StringersGroup.Group if p.Side == "Left")
        stair.StringerCustomWidth = True
        stair.StringerWidth = 270
        stair.Proxy.rebuild(stair)
        self.assertAlmostEqual(board.Width.Value, 270)
        board.OverrideWidth = True
        board.Width = 310
        stair.StringerCustomWidth = False
        stair.Proxy.rebuild(stair)
        self.assertAlmostEqual(board.Width.Value, 310)

    def test_fair_curve_bridges_short_hollows_and_clears_every_pin(self):
        from stairdesigner.geometry_fairing import FairCurve, Pin, fair_curve

        # Pins on one pitch: the batten lies straight on them.
        straight = fair_curve([Pin(u, 0.5 * u) for u in (0.0, 250.0, 500.0, 750.0)], 400.0)
        self.assertTrue(straight.is_straight())
        self.assertAlmostEqual(straight.value(600.0), 300.0, places=6)
        self.assertAlmostEqual(straight.value(-100.0), -50.0, places=6)

        # One pin 30 mm low between its neighbours: the batten bridges the
        # hollow instead of dipping into it, and never passes under a pin.
        pins = [Pin(u, 0.5 * u - (30.0 if u == 500.0 else 0.0)) for u in range(0, 1001, 250)]
        bridged = fair_curve(pins, 400.0)
        for pin in pins:
            self.assertGreaterEqual(bridged.value(pin.u), pin.value - 1e-6)
        self.assertGreater(bridged.value(500.0), 0.5 * 500.0 - 10.0)
        # A hollow several stiffness lengths long is followed.
        long_pins = [
            Pin(u, 0.5 * u - 30.0 * max(0.0, 1.0 - abs(u - 2500.0) / 1500.0))
            for u in range(0, 5001, 250)
        ]
        followed = fair_curve(long_pins, 400.0)
        self.assertLess(followed.value(2500.0), 0.5 * 2500.0 - 25.0)

        # A clamped end arrives at its level with no pitch, and runs on level.
        clamped = fair_curve(pins, 400.0, end_slope=0.0, end_value=520.0)
        self.assertAlmostEqual(clamped.value(1000.0), 520.0, places=6)
        self.assertAlmostEqual(clamped.slope(1000.0), 0.0, places=9)
        self.assertAlmostEqual(clamped.value(1040.0), 520.0, places=6)
        for pin in pins[:-1]:
            self.assertGreaterEqual(clamped.value(pin.u), pin.value - 1e-6)

        # The curve has no kink: its pitch is continuous across every knot.
        for knot in bridged.knots[1:-1]:
            self.assertAlmostEqual(bridged.slope(knot - 1e-6), bridged.slope(knot + 1e-6), places=5)
        spline = FairCurve.through([(0.0, 0.0), (100.0, 10.0), (200.0, 40.0)], 0.0, 0.5)
        self.assertAlmostEqual(spline.slope(0.0), 0.0, places=9)
        self.assertAlmostEqual(spline.slope(200.0), 0.5, places=9)
        self.assertAlmostEqual(spline.value(100.0), 10.0, places=9)
        # One pin alone does not place a batten.
        with self.assertRaises(ValueError):
            fair_curve([Pin(0.0, 10.0), Pin(500.0, 0.0, False, 0.0)], 400.0)

    def test_winder_board_profile_is_one_fair_curve_per_edge(self):
        from stairdesigner.geometry_stringer_path import EdgeBlock, make_stringer_profile_face

        # Nosings as winders space them along a wall board: flat around the
        # corners, steep between them.
        parameters = [0.0, 420.0, 740.0, 1000.0, 1225.0, 1455.0, 1885.0, 2180.0]
        tops = [697.0, 867.0, 1082.0, 1295.0, 1505.0, 1701.0, 1900.0, 2001.0]
        supports = [
            (453.0, 582.0),
            (762.0, 789.0),
            (1018.0, 996.0),
            (1244.0, 1204.0),
            (1474.0, 1411.0),
            (1909.0, 1618.0),
        ]

        def edges_at(face, u):
            probe = Part.makeLine(FreeCAD.Vector(u, 0.0, -1000.0), FreeCAD.Vector(u, 0.0, 5000.0))
            crossing = face.common(probe).BoundBox
            return crossing.ZMin, crossing.ZMax

        def check(face, levelled):
            self.assertTrue(face.isValid())
            kinds = [edge.Curve.TypeId for edge in face.OuterWire.Edges]
            # One spline for the upper edge and one for the lower edge; the
            # rest are the plumb ends and the level runs into the blocks.
            self.assertEqual(kinds.count("Part::GeomBSplineCurve"), 2, kinds)
            self.assertNotIn("Part::GeomCircle", kinds)
            for u, top in zip(parameters, tops):
                self.assertGreaterEqual(edges_at(face, min(max(u, 0.5), 2179.5))[1], top - 0.5, u)
            for u, z in supports:
                # 50 mm square to the board is more than that plumb.
                self.assertLessEqual(edges_at(face, u)[0], z - 50.0 + 1e-3, u)
            stations = [5.0 + 10.0 * index for index in range(218)]
            for edge in (0, 1):
                heights = [edges_at(face, u)[edge] for u in stations]
                bends = [abs(a - 2.0 * b + c) for a, b, c in zip(heights, heights[1:], heights[2:])]
                # No kink anywhere: over 10 mm the edge never turns as
                # sharply as a 100 mm radius would.
                self.assertLess(max(bends), 1.0, edge)
                # The stair only climbs, and so do both edges.
                self.assertTrue(all(b >= a - 1e-6 for a, b in zip(heights, heights[1:])), edge)
                if levelled:
                    self.assertAlmostEqual(heights[0], heights[1], places=3)

        check(
            make_stringer_profile_face(
                (0.0, 0.0), (1.0, 0.0), parameters, tops, 235.0, supports=supports
            ),
            False,
        )
        blocked = make_stringer_profile_face(
            (0.0, 0.0),
            (1.0, 0.0),
            parameters,
            tops,
            235.0,
            supports=supports,
            start_block=EdgeBlock(27.0, 705.0, 360.0),
            end_block=EdgeBlock(2140.0, 2010.0, 1660.0),
        )
        check(blocked, True)
        # The edges reach the blocks' levels and run level through them.
        for u, (bottom, top) in ((10.0, (360.0, 705.0)), (2160.0, (1660.0, 2010.0))):
            found = edges_at(blocked, u)
            self.assertAlmostEqual(found[0], bottom, places=3)
            self.assertAlmostEqual(found[1], top, places=3)

        # Along a straight flight the edges stay straight lines.
        regular = make_stringer_profile_face(
            (0.0, 0.0),
            (1.0, 0.0),
            [250.0 * index for index in range(8)],
            [300.0 + 180.0 * index for index in range(8)],
            280.0,
        )
        self.assertTrue(
            all(edge.Curve.TypeId == "Part::GeomLine" for edge in regular.OuterWire.Edges)
        )

    def test_corner_blocks_keep_the_housings_inside_the_wall_boards(self):
        # Winders spanning a corner house a tread low beside the block. The
        # block's underside may not stand above that housing, nor a ramp up
        # to it cut the tread's end out of the board.
        stair, flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        # A first flight wider than the second, with little timber asked
        # under the steps: the third tread then reaches far along the first
        # wall board, 100 mm below where a block halfway between the two
        # boards' undersides would put its lower edge.
        stair.Proxy._updating = True
        stair.StringerJoinery = False
        stair.WalkingLineOffset = 500.0
        stair.StringerUndersideOffset = 15.0
        stair.StringerNosingOffset = 20.0
        stair.Proxy._updating = False
        for flight, width, (left, right) in zip(
            flights, (850.0, 700.0), ((855.0, 155.0), (2240.0, 1390.0))
        ):
            flight.Proxy._updating = True
            flight.Width = width
            flight.LeftLength = left
            flight.RightLength = right
            flight.Proxy._updating = False
        stair.Proxy.rebuild(stair)
        self.assertEqual(stair.GeometryStatus, "Balanced multi-flight stair")
        timber = stair.StringerUndersideOffset.Value
        treads = [
            part.Shape for part in stair.StepsGroup.Group if part.StairDesignerRole == "Tread"
        ]
        checked = 0
        for board in stair.StringersGroup.Group:
            if board.Side != "Left":
                continue
            bounds = board.Shape.BoundBox
            slab = Part.makeBox(
                bounds.XLength,
                bounds.YLength,
                stair.FloorHeight.Value + 1000.0,
                FreeCAD.Vector(bounds.XMin, bounds.YMin, 0.0),
            )
            for tread in treads:
                housed = tread.common(slab)
                if housed.Volume < 1.0:
                    continue
                lowest = min(vertex.Point.z for vertex in housed.Vertexes)
                if lowest < 1.0:
                    continue
                centre = housed.BoundBox.Center
                for vertex in housed.Vertexes:
                    point = vertex.Point
                    if point.z > lowest + 1e-6:
                        continue
                    # Just inside the housing, off its faces.
                    inside = (
                        point + (FreeCAD.Vector(centre.x, centre.y, point.z) - point).normalize()
                    )
                    bottom = self._bottom_at(board.Shape, inside.x, inside.y)
                    if bottom is None:
                        continue
                    self.assertLessEqual(bottom, point.z - timber + 0.5, (board.Label, point))
                    checked += 1
        self.assertGreater(checked, 8)

    def test_deferred_rebuilds_wait_for_regenerate(self):
        # The task panel's manual mode: changes only mark the stair until it
        # is regenerated.
        stair, _flights = self._short_inner_side_stair("Housed stringer", handrails=False)

        def top():
            return max(part.Shape.BoundBox.ZMax for part in stair.StepsGroup.Group)

        before = top()
        stair.Proxy.defer_rebuilds(stair, True)
        stair.FloorHeight = stair.FloorHeight.Value + 220.0
        self.document.recompute()
        self.assertAlmostEqual(top(), before, places=6)
        self.assertTrue(stair.Proxy.rebuild_pending())
        stair.Proxy.regenerate(stair)
        self.assertFalse(stair.Proxy.rebuild_pending())
        raised = top()
        self.assertGreater(raised, before + 100.0)
        # Still deferred after a regeneration; ending it rebuilds what waits.
        stair.FloorHeight = stair.FloorHeight.Value - 220.0
        self.assertAlmostEqual(top(), raised, places=6)
        stair.Proxy.defer_rebuilds(stair, False)
        self.assertFalse(stair.Proxy.rebuild_pending())
        self.assertAlmostEqual(top(), before, places=6)

    def test_exit_width_sets_where_a_turned_exit_opens(self):
        # The second flight exits to the right, onto its inner side, x = 110,
        # which ends at y = -1540. The last riser runs along that side from
        # the exit newel, the flight width before its end by default. The
        # last tread reaches the floor there, beyond the board.
        stair, flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        last = flights[-1]
        self.assertIn("ExitWidth", last.PropertiesList)
        self.assertEqual(last.ExitWidth.Value, 0.0)

        def newel_y():
            tread = max(
                (part for part in stair.StepsGroup.Group if part.StairDesignerRole == "Tread"),
                key=lambda part: part.Shape.BoundBox.ZMax,
            )
            return max(
                vertex.Point.y for vertex in tread.Shape.Vertexes if vertex.Point.x < 110.0 + 1e-6
            )

        self.assertAlmostEqual(newel_y(), -1540.0 + last.Width.Value, places=3)
        last.ExitWidth = 500.0
        self.document.recompute()
        self.assertEqual(stair.GeometryStatus, "Balanced multi-flight stair")
        self.assertAlmostEqual(newel_y(), -1540.0 + 500.0, places=3)
        for part in stair.StepsGroup.Group + stair.StringersGroup.Group:
            self.assertTrue(part.Shape.isValid(), part.Label)

    def test_steps_are_housed_into_the_newel_posts(self):
        # The treads around a newel end in it at the housing depth, as in a
        # board: none passes through the post, and its fan treads meet it.
        stair, _flights = self._short_inner_side_stair("Housed stringer")
        depth = stair.StringerStepOverlap.Value
        posts = [
            part
            for part in stair.HandrailsGroup.Group
            # The newels on the inner side of the turns: at the corner between
            # the flights and at the exit turning to the right.
            if part.StairDesignerRole == "HandrailPost"
            and part.Side == "Right"
            and part.Shape.BoundBox.ZMin < stair.FloorHeight.Value - 1.0
        ]
        treads = [part for part in stair.StepsGroup.Group if part.StairDesignerRole == "Tread"]
        self.assertTrue(posts)
        newels = 0
        for post in posts:
            box = post.Shape.BoundBox
            core = Part.makeBox(
                box.XLength - 2.0 * depth,
                box.YLength - 2.0 * depth,
                box.ZLength,
                FreeCAD.Vector(box.XMin + depth, box.YMin + depth, box.ZMin),
            )
            meeting = 0
            for tread in treads:
                # The joinery has housed the tread into the post: no part of
                # the tread is left in its core.
                self.assertLess(
                    tread.Shape.common(core).Volume, 1.0, f"{tread.Label} in {post.Label}"
                )
                outline = Part.makeBox(
                    box.XLength + 0.2,
                    box.YLength + 0.2,
                    box.ZLength,
                    FreeCAD.Vector(box.XMin - 0.1, box.YMin - 0.1, box.ZMin),
                )
                meeting += tread.Shape.common(outline).Volume > 1e-3
            newels += meeting >= 2
        self.assertEqual(newels, len(posts))

    def test_winders_end_at_the_newel_or_clear_of_it(self):
        # A wide short first flight turning into a narrower one that exits to
        # the side. Around the inner corner's newel and the exit's, centred
        # on their corners, a riser ends at the corner (inside the post),
        # against the post's face just past it, or a post's size away.
        from stairdesigner.geometry_plan import _boundary_data

        specs = [(505.0, 800.0, 0.0), (1800.0, 650.0, -90.0)]
        sections, _going = balanced_winder_sections(
            specs,
            11,
            1.0,
            ["Herse balancing"],
            exit_direction="To right",
            winding_parameters=[(50.0, 50.0)],
            nosing=30.0,
            walking_line_offset=300.0,
            exit_width=800.0,
        )
        footprint = make_stair_footprint(specs)
        fitted = fit_balanced_sections_to_footprint(sections, footprint)
        fans = corner_post_sections(fitted, "Straight", "To right", sections, 70.0)
        fans = inner_corner_newel_sections(fans, footprint, 70.0)
        self.assertTrue(
            balanced_partition_is_valid(balanced_tread_faces(fans, footprint), footprint, 11)
        )
        boundary = _boundary_data(footprint)
        vertices = boundary["vertices"]
        orientation = 1.0 if boundary["signed_area"] > 0.0 else -1.0
        newels = [fans[-1].right]
        for index, corner in enumerate(vertices):
            previous, following = vertices[index - 1], vertices[(index + 1) % len(vertices)]
            turn = (corner[0] - previous[0]) * (following[1] - corner[1]) - (
                corner[1] - previous[1]
            ) * (following[0] - corner[0])
            if turn * orientation < -1e-7:
                newels.append(corner)
        self.assertGreaterEqual(len(newels), 2)
        at_faces = 0
        for newel in newels:
            for section in fans[1:-1]:
                for end in (section.left, section.right):
                    distance = math.dist(end, newel)
                    if distance < 70.0 - 1e-6:
                        self.assertTrue(
                            distance < 1e-6 or abs(distance - 35.0) < 1e-6,
                            f"riser ends {distance:.1f} mm from the newel at {newel}",
                        )
                        at_faces += abs(distance - 35.0) < 1e-6
        # Here a riser balanced to meet the second flight's side 52 mm past
        # the inner corner, clear of the post, ends against its face rather
        # than being pulled through the post to its centre.
        self.assertGreater(at_faces, 0)

    def test_treads_turning_about_a_newel_keep_their_walking_line_going(self):
        # The same stair: an 800 mm exit opening beside a 650 mm flight.
        # Pulled onto the newel, the risers no longer cross the walking line
        # at their stations; about the newel, they keep their end on the post
        # and cross the walking line, round the newel, at equal goings.
        specs = [(505.0, 800.0, 0.0), (1800.0, 650.0, -90.0)]
        sections, _going = balanced_winder_sections(
            specs,
            11,
            1.0,
            ["Herse balancing"],
            exit_direction="To right",
            winding_parameters=[(50.0, 50.0)],
            nosing=30.0,
            walking_line_offset=300.0,
            exit_width=800.0,
        )
        footprint = make_stair_footprint(specs)
        fitted = fit_balanced_sections_to_footprint(sections, footprint)
        pinned = corner_post_sections(fitted, "Straight", "To right", sections, 70.0)
        pinned = inner_corner_newel_sections(pinned, footprint, 70.0)
        fans, steps, _notched = newel_fan_plan(
            pinned,
            footprint,
            "Straight",
            "To right",
            70.0,
            walking_line=([800.0, 650.0], 300.0),
        )
        self.assertIsNone(steps)
        faces = balanced_tread_faces(fans, footprint)
        self.assertTrue(balanced_partition_is_valid(faces, footprint, 11))
        newel = pinned[-1].right
        moved = [
            index for index, (before, after) in enumerate(zip(pinned, fans)) if before is not after
        ]
        self.assertTrue(moved)
        for index in moved:
            # The newel end stays; only the outer end moves.
            self.assertEqual(fans[index].right, pinned[index].right)
        around_exit = [
            index
            for index in range(1, len(fans) - 1)
            if math.dist(fans[index].right, newel) < 35.0 + 1e-6
        ]
        self.assertGreaterEqual(len(around_exit), 2)
        # The treads wholly about the newel, to the exit; the one from the
        # last riser housed in the board keeps the walking line's place.
        self.assertEqual(fans[around_exit[0]].left, pinned[around_exit[0]].left)
        # The walking line runs round the newel 300 mm from the outer side.
        radius = 650.0 - 300.0

        def crossing(section):
            fixed, outer = section.right, section.left
            dx, dy = outer[0] - fixed[0], outer[1] - fixed[1]
            length = math.hypot(dx, dy)
            dx, dy = dx / length, dy / length
            px, py = fixed[0] - newel[0], fixed[1] - newel[1]
            b = px * dx + py * dy
            t = -b + math.sqrt(b * b - (px * px + py * py - radius * radius))
            return (fixed[0] + dx * t, fixed[1] + dy * t)

        def goings(sections):
            points = [crossing(sections[index]) for index in range(around_exit[0], len(sections))]
            return [math.dist(a, b) for a, b in zip(points, points[1:])]

        before = goings(pinned)
        after = goings(fans)
        self.assertGreater(max(before) / min(before), 1.2)
        self.assertLess(max(after) / min(after), 1.001)

    def test_risers_spread_along_the_newel_post(self):
        # About the exit newel, the risers end apart along the post's faces
        # in the stair, in order; the last tread opens beside the post onto
        # the floor and takes no share of it, so the riser before it ends
        # where the post meets the exit, with the exit line.
        specs = [(505.0, 800.0, 0.0), (1800.0, 650.0, -90.0)]
        sections, _going = balanced_winder_sections(
            specs,
            11,
            1.0,
            ["Herse balancing"],
            exit_direction="To right",
            winding_parameters=[(50.0, 50.0)],
            nosing=30.0,
            walking_line_offset=300.0,
            exit_width=800.0,
        )
        footprint = make_stair_footprint(specs)
        fitted = fit_balanced_sections_to_footprint(sections, footprint)
        pinned = corner_post_sections(fitted, "Straight", "To right", sections, 70.0)
        pinned = inner_corner_newel_sections(pinned, footprint, 70.0)
        corner = pinned[-1].right
        square = newel_post_square(corner, (1.0, 0.0), 30.0)
        post = newel_post_square(corner, (1.0, 0.0), 70.0)
        steps, notched = newel_post_plan(pinned, footprint, [square], [post])
        self.assertIsNot(steps, pinned)
        on_post = [index for index, section in enumerate(steps) if "right" in section.post_ends]
        inner = [index for index in on_post if 0 < index < len(steps) - 1]
        self.assertGreaterEqual(len(inner), 2)
        ends = [steps[index].right for index in inner]
        self.assertEqual(len({(round(x, 6), round(y, 6)) for x, y in ends}), len(ends))
        self.assertLess(math.dist(steps[inner[-1]].right, steps[-1].right), 1e-6)
        self.assertTrue(
            balanced_partition_is_valid(
                balanced_tread_faces(steps, notched), notched, len(steps) - 1
            )
        )

    def test_side_offset_moves_one_step_front_along_its_side(self):
        specs = [(505.0, 800.0, 0.0), (1800.0, 650.0, -90.0)]
        sections, _going = balanced_winder_sections(
            specs, 11, 1.0, ["Herse balancing"], walking_line_offset=300.0
        )
        footprint = make_stair_footprint(specs)
        fitted = fit_balanced_sections_to_footprint(sections, footprint)
        offsets = [(0.0, 0.0)] * len(fitted)
        offsets[7] = (-10.0, 0.0)
        shifted = shift_section_ends(fitted, footprint, offsets)
        self.assertIsNotNone(shifted)
        for index, (before, after) in enumerate(zip(fitted, shifted)):
            if index == 7:
                self.assertAlmostEqual(math.dist(before.left, after.left), 10.0, places=6)
                self.assertEqual(after.right, before.right)
                # Down the stair: toward the previous front on that side.
                self.assertLess(
                    math.dist(after.left, fitted[6].left), math.dist(before.left, fitted[6].left)
                )
            else:
                self.assertIs(after, before)
        # A shift past the next front would cross it.
        offsets[7] = (1000.0, 0.0)
        self.assertIsNone(shift_section_ends(fitted, footprint, offsets))

        # Through the stair: a step's own offset moves only the steps beside
        # that front.
        stair, _flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        treads = {
            part.Index: part for part in stair.StepsGroup.Group if part.StairDesignerRole == "Tread"
        }
        self.assertIn("LeftOffset", treads[6].PropertiesList)
        before = {index: part.Shape.copy() for index, part in treads.items()}
        treads[6].LeftOffset = -10.0
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        changed = {
            index
            for index, part in treads.items()
            if abs(part.Shape.Volume - before[index].Volume) > 1e-3
            or part.Shape.BoundBox.Center.distanceToPoint(before[index].BoundBox.Center) > 1e-6
        }
        self.assertTrue(changed)
        self.assertTrue(changed <= {5, 6}, changed)

        # At a turned exit the last step runs on beyond the board to the
        # floor: the riser before it, ending on the newel, can move along
        # the newel out past the board.
        stair, _flights = self._short_inner_side_stair("Housed stringer")
        treads = {
            part.Index: part for part in stair.StepsGroup.Group if part.StairDesignerRole == "Tread"
        }
        last = max(treads)
        before = {index: part.Shape.copy() for index, part in treads.items()}
        treads[last].RightOffset = 15.0
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        changed = {
            index
            for index, part in treads.items()
            if abs(part.Shape.Volume - before[index].Volume) > 1e-3
        }
        self.assertTrue(changed)
        self.assertTrue(changed <= {last - 1, last}, changed)
        self.assertLess(treads[last].Shape.Volume, before[last].Volume)
        self.assertTrue(treads[last].Shape.isValid())

    def test_walking_line_gives_each_step_its_going(self):
        # The stair keeps, step by step, its stretch of the walking line at
        # the step's top and the step's going along it, for the view.
        stair, _flights = self._short_inner_side_stair("Housed stringer")
        treads = sorted(
            (part for part in stair.StepsGroup.Group if part.StairDesignerRole == "Tread"),
            key=lambda part: part.Index,
        )
        counts = list(stair.WalkingLineCounts)
        goings = list(stair.WalkingLineGoings)
        points = list(stair.WalkingLinePoints)
        self.assertEqual(len(counts), len(treads))
        self.assertEqual(len(goings), len(treads))
        self.assertEqual(sum(counts), len(points))
        inverse = stair.getGlobalPlacement().inverse().toMatrix()
        start = 0
        for tread, count, going in zip(treads, counts, goings):
            stretch = points[start : start + count]
            start += count
            self.assertGreaterEqual(count, 2, tread.Label)
            length = sum((b - a).Length for a, b in zip(stretch, stretch[1:]))
            self.assertAlmostEqual(length, going, delta=1e-6)
            self.assertGreater(going, 100.0)
            top = tread.Shape.copy()
            top.transformShape(inverse)
            for point in stretch:
                self.assertAlmostEqual(point.z, top.BoundBox.ZMax, places=3)
        # The treads sharing the turn about the inner corner's newel share
        # its walking line evenly.
        self.assertAlmostEqual(goings[0], goings[1], delta=0.1)

    def test_even_walking_line_centres_a_wide_exit_and_opens_treads_outward(self):
        # A turn about two newels into a second flight whose last steps turn
        # on to an exit wider than the flight: the plan the balancing gives
        # the even pass on a 795 then 645 wide stair.
        newels = [
            [(175.0, -525.0), (175.0, -555.0), (205.0, -555.0), (205.0, -525.0)],
            [(205.0, 50.0), (175.0, 50.0), (175.0, 20.0), (205.0, 20.0)],
        ]
        plan = [
            ((0.0, 830.0), (0.0, 35.0), 0, 0.0),
            ((285.6, 830.0), (96.3, 35.0), 0, 184.4),
            ((746.8, 830.0), (118.5, 35.0), 0, 368.9),
            ((835.0, 389.4), (205.0, 50.0), 1, 553.3),
            ((835.0, 70.6), (190.0, -66.5), 1, 737.7),
            ((835.0, -179.7), (190.0, -196.3), 1, 922.1),
            ((835.0, -395.2), (190.0, -357.3), 1, 1106.6),
            ((835.0, -627.2), (205.0, -530.0), 1, 1291.0),
            ((835.0, -1036.8), (205.0, -550.0), 1, 1475.4),
            ((594.5, -1370.0), (190.0, -555.0), 1, 1659.8),
            ((155.0, -1370.0), (155.0, -555.0), 1, 1844.3),
        ]
        sections = []
        for left, right, flight, station in plan:
            width = math.dist(left, right)
            sections.append(
                BalancedSection(
                    center=((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0),
                    tangent=((left[1] - right[1]) / width, (right[0] - left[0]) / width),
                    left=left,
                    right=right,
                    station=station,
                    width=width,
                    flight_index=flight,
                )
            )

        def face(points):
            return Part.Face(
                Part.makePolygon([FreeCAD.Vector(x, y, 0.0) for x, y in points + points[:1]])
            )

        footprint = face(
            [
                (175.0, 35.0),
                (0.0, 35.0),
                (0.0, 830.0),
                (835.0, 830.0),
                (835.0, -1370.0),
                (155.0, -1370.0),
                (155.0, -555.0),
                (205.0, -555.0),
                (205.0, -525.0),
                (190.0, -525.0),
                (190.0, 20.0),
                (205.0, 20.0),
                (205.0, 50.0),
                (175.0, 50.0),
            ]
        )
        outline = face(
            [
                (0.0, 830.0),
                (835.0, 830.0),
                (835.0, -1370.0),
                (190.0, -1370.0),
                (190.0, 35.0),
                (0.0, 35.0),
            ]
        )
        widths = [795.0, 645.0]
        line = walking_line_points(sections, newels, widths, 500.0)
        # In the middle of the steps: of the entry's 795, and of the exit's
        # 815 from the newel's face to the wall, not of the flight's 645.
        self.assertAlmostEqual(line[0][1], (35.0 + 830.0) / 2.0, delta=0.5)
        self.assertAlmostEqual(line[-1][1], (-555.0 - 1370.0) / 2.0, delta=0.5)

        for offset in (300.0, 500.0):
            even = even_walking_line_goings(
                sections, footprint, newels, widths, offset, 70.0, outline
            )
            self.assertIsNotNone(even, offset)
            goings = [going for _points, going in walking_line_treads(even, newels, widths, offset)]
            for going in goings:
                self.assertAlmostEqual(going, goings[0], delta=0.5)
            # No tread is wider on the inner side, the right, than on the
            # outer: the risers next to one turned about a newel turn with it.
            for index, (a, b) in enumerate(zip(even, even[1:])):
                self.assertGreaterEqual(
                    math.dist(a.left, b.left), math.dist(a.right, b.right) - 1e-6, (offset, index)
                )

    def _plan_points(self, shape):
        return [
            vertex.Point
            for face in shape.Faces
            if abs(face.normalAt(0, 0).z) > 1e-3
            for vertex in face.Vertexes
        ]

    def test_stair_width_holds_its_stringers_and_posts(self):
        # A flight's width is the opening the stair stands in: its housed
        # boards and handrail posts stand within it, and its steps are
        # narrower by the room they take on each side.
        stair = make_stair(
            floor_height=2800.0,
            flight_length=3000.0,
            width=900.0,
            steps=15,
            stair_type="Wood",
        )
        flight = get_flights(stair)[0]
        flight.Proxy._updating = True
        for side in ("Left", "Right"):
            setattr(flight, f"{side}StringerType", "Housed stringer")
        flight.LeftHandrailEnabled = False
        flight.RightHandrailEnabled = True
        flight.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        self.assertEqual(stair.GeometryStatus, "Straight flight")
        inverse = stair.getGlobalPlacement().inverse()

        def local(shape):
            shape = shape.copy()
            shape.transformShape(inverse.toMatrix())
            return shape.BoundBox

        # A housed board stands out its thickness less its housing (20), a
        # post half its size from the board's centre line (35).
        for part in stair.StepsGroup.Group:
            if part.StairDesignerRole == "Tread":
                box = local(part.Shape)
                self.assertAlmostEqual(box.YMin, 35.0, places=3)
                self.assertAlmostEqual(box.YMax, 880.0, places=3)
        parts = list(stair.StringersGroup.Group) + [
            part for part in stair.HandrailsGroup.Group if part.StairDesignerRole == "HandrailPost"
        ]
        self.assertTrue(any(part.StairDesignerRole == "HandrailPost" for part in parts))
        for part in parts:
            box = local(part.Shape)
            self.assertGreaterEqual(box.YMin, -1e-6, part.Label)
            self.assertLessEqual(box.YMax, 900.0 + 1e-6, part.Label)

        # Turning flights, with a newel at the inner corner and at the turned
        # exit: the boards and posts stay within the stair's outline.
        stair, flights = self._short_inner_side_stair("Housed stringer")
        # The boards run on below the first step, before the entry, and the
        # one across the top under the floor, past the exit.
        outline = make_stair_footprint([(460.0, 700.0, 0.0), (1890.0, 700.0, -90.0)]).fuse(
            [
                Part.makePlane(500.0, 700.0, FreeCAD.Vector(-500.0, 0.0, 0.0)),
                Part.makePlane(310.0, 700.0, FreeCAD.Vector(-200.0, -1540.0, 0.0)),
            ]
        )
        outline.Placement = stair.getGlobalPlacement()
        parts = list(stair.StringersGroup.Group) + [
            part for part in stair.HandrailsGroup.Group if part.StairDesignerRole == "HandrailPost"
        ]
        self.assertTrue(any(part.StairDesignerRole == "HandrailPost" for part in parts))
        for part in parts:
            for point in self._plan_points(part.Shape):
                point = FreeCAD.Vector(point.x, point.y, outline.BoundBox.ZMin)
                self.assertTrue(outline.isInside(point, 1e-3, True), (part.Label, point))
        # The last step still reaches the floor along the turned exit.
        exit_line = max(
            (part for part in stair.StepsGroup.Group if part.StairDesignerRole == "Tread"),
            key=lambda part: part.Index,
        )
        local_box = exit_line.Shape.copy()
        local_box.transformShape(stair.getGlobalPlacement().inverse().toMatrix())
        self.assertLessEqual(local_box.BoundBox.XMin, 110.0 + 1e-3)

    def test_turned_exit_balances_like_a_turn_between_flights(self):
        # A short first flight turning into a second one that exits to the
        # side, as in a quarter-turn stair arriving beside a landing newel.
        specs = [(460.0, 700.0, 0.0), (1850.0, 700.0, -90.0)]
        sections, _going = balanced_winder_sections(
            specs,
            10,
            1.0,
            ["Herse balancing"],
            exit_direction="To right",
            winding_parameters=[(50.0, 50.0)],
            nosing=30.0,
            walking_line_offset=300.0,
        )
        footprint = make_stair_footprint(specs)
        fitted = fit_balanced_sections_to_footprint(sections, footprint)
        fans = corner_post_sections(fitted, "Straight", "To right", sections, 70.0)
        fans = inner_corner_newel_sections(fans, footprint, 70.0)
        faces = balanced_tread_faces(fans, footprint)
        self.assertTrue(balanced_partition_is_valid(faces, footprint, 10))
        # The exit's turn spreads over the second flight as the corner's does:
        # at most one of its risers stays square to it.
        second = [section for section in fans[1:-1] if section.flight_index == 1]
        square = [
            section for section in second if abs(section.tangent[0]) < math.sin(math.radians(1.0))
        ]
        self.assertLessEqual(len(square), 1)
        # Its inner side, x = 110 here, narrows towards the exit newel.
        newel = fans[-1].right
        landings = sorted(
            {
                round(section.right[1], 6)
                for section in second
                if abs(section.right[0] - newel[0]) < 1e-6
            }
            - {round(newel[1], 6)},
            reverse=True,
        )
        goings = [first - second for first, second in zip(landings, landings[1:] + [newel[1]])]
        self.assertGreaterEqual(len(goings), 3)
        for nearer, farther in zip(goings[-3:][1:], goings[-3:]):
            self.assertLess(nearer, farther)
        # Riser ends closer to the newel than its size die into it.
        self.assertGreaterEqual(goings[-1], 70.0)

    def test_corner_post_winders_preserve_partition_and_outer_nosings(self):
        for length in (1800.0, 3500.0):
            for entry in ("Straight", "From left", "From right"):
                for exit_direction in ("Straight", "To left", "To right"):
                    with self.subTest(length=length, entry=entry, exit=exit_direction):
                        specs = [(length, 1000.0, 0.0)]
                        sections, _ = balanced_winder_sections(
                            specs,
                            15,
                            entry_direction=entry,
                            exit_direction=exit_direction,
                            nosing=30.0,
                            walking_line_offset=300.0,
                        )
                        footprint = make_stair_footprint(specs)
                        sections = fit_balanced_sections_to_footprint(sections, footprint)
                        fans = corner_post_sections(sections, entry, exit_direction)
                        faces = balanced_tread_faces(fans, footprint)
                        self.assertTrue(balanced_partition_is_valid(faces, footprint, 15))
                        for before, after in zip(sections, fans):
                            self.assertEqual(before.station, after.station)
                            if before.locked_to_flight:
                                self.assertEqual(before, after)
                            self.assertTrue(
                                before.left == after.left or before.right == after.right
                            )
                        for side in ("left", "right"):
                            if length != 3500.0 or side not in entry or side not in exit_direction:
                                continue
                            outer_side = "right" if side == "left" else "left"
                            first_x = getattr(fans[0], side)[0]
                            last_x = getattr(fans[-1], side)[0]
                            for section in fans:
                                outer = getattr(section, outer_side)
                                if first_x < outer[0] < last_x:
                                    # Past the newel, the riser must be square
                                    # to the flight, never a reverse fan.
                                    self.assertAlmostEqual(
                                        section.left[0], section.right[0], places=6
                                    )

    @unittest.skipUnless(FreeCAD.GuiUp, "Requires the GUI")
    def test_closed_panel_does_not_install_deferred_dragger(self):
        from PySide import QtCore, QtGui
        from stairdesigner.taskpanel import StairDesignerTaskPanel

        stair = make_stair()
        panel = StairDesignerTaskPanel(stair)
        panel.accept()
        QtGui.QApplication.processEvents()
        self.assertIsNone(panel._position_dragger)
        panel.form.deleteLater()
        QtCore.QCoreApplication.sendPostedEvents(None, QtCore.QEvent.DeferredDelete)

        panel = StairDesignerTaskPanel(stair)
        panel.form.deleteLater()
        QtCore.QCoreApplication.sendPostedEvents(None, QtCore.QEvent.DeferredDelete)
        QtGui.QApplication.processEvents()
        self.assertTrue(panel._closed)
        self.assertIsNone(panel._position_dragger)
        self.assertFalse(panel._selection_observer_registered)

    def test_side_entry_stringers_follow_turns_and_pass_bop_check(self):
        stair = make_stair()
        flight = get_flights(stair)[0]
        for entry, exit_direction in (
            ("From left", "To left"),
            ("From left", "To right"),
            ("From right", "To left"),
            ("From right", "To right"),
            ("Straight", "To left"),
            ("From left", "Straight"),
        ):
            for kind in ("Housed stringer", "Notched stringer"):
                with self.subTest(entry=entry, exit=exit_direction, kind=kind):
                    flight.Proxy._updating = True
                    flight.EntryDirection = entry
                    flight.ExitDirection = exit_direction
                    flight.LeftStringerType = kind
                    flight.RightStringerType = kind
                    flight.Proxy._updating = False
                    stair.Proxy.rebuild(stair)
                    self.assertEqual(stair.GeometryStatus, "Balanced entry/exit stair")
                    roles = [part.StairDesignerRole for part in stair.StringersGroup.Group]
                    self.assertEqual(roles.count("LeftStringer"), 1)
                    self.assertEqual(roles.count("RightStringer"), 1)
                    # Each board turning the corner is a part of its own, and
                    # boards meeting there do not share timber.
                    self.assertGreater(len(roles), 2)
                    for side in ("Left", "Right"):
                        boards = [p for p in stair.StringersGroup.Group if p.Side == side]
                        for index, first in enumerate(boards):
                            for second in boards[index + 1 :]:
                                self.assertLess(first.Shape.common(second.Shape).Volume, 1.0)
                    for part in stair.StringersGroup.Group:
                        self.assertTrue(part.Shape.isValid())
                        self.assertTrue(part.Shape.Solids)
                        # isValid() alone misses C0 edges rejected by Check Geometry.
                        part.Shape.check(True)
                    if entry == "From left" and exit_direction == "To left":
                        outer = Part.makeCompound(
                            [p.Shape for p in stair.StringersGroup.Group if p.Side == "Right"]
                        )
                        for x in (0.0, flight.LeftLength.Value):
                            if kind == "Notched stringer":
                                # Notched boards sit inside the tread edge.
                                x += 40.0 if x == 0.0 else -40.0
                            probe = Part.makeLine(
                                FreeCAD.Vector(x, 500.0, -100.0), FreeCAD.Vector(x, 500.0, 4000.0)
                            )
                            self.assertTrue(outer.common(probe).Edges)

    def test_winder_rail_clears_intermediate_nosings(self):
        from stairdesigner.geometry_handrails import (
            fit_handrail_to_nosings,
            make_handrail_path,
            sample_handrail_path,
        )
        from stairdesigner.geometry_stringer_path import boundary_stringer_runs
        from stairdesigner.geometry_core import assign_section_elevations

        specs = [(3500.0, 1000.0, 0.0)]
        sections, _ = balanced_winder_sections(
            specs,
            15,
            entry_direction="From left",
            exit_direction="To left",
            walking_line_offset=300.0,
        )
        footprint = make_stair_footprint(specs)
        sections = fit_balanced_sections_to_footprint(sections, footprint)
        sections = assign_section_elevations(sections, [i * 2800.0 / 15 for i in range(16)])
        for side in ("Left", "Right"):
            for run in boundary_stringer_runs(sections, side, footprint):
                path = make_handrail_path(run, 2800.0 / 15, side, 0.0, 900.0)
                fit_handrail_to_nosings(path, run, side)
                for section in run:
                    point = section.left if side == "Left" else section.right
                    distance = sum(
                        (point[i] - path["start"][i]) * path["tangent"][i] for i in (0, 1)
                    )
                    sample = sample_handrail_path(path, distance / path["length"])
                    self.assertGreaterEqual(sample["top"] + 1e-7, section.top_elevation + 900.0)

    def test_side_entry_rails_meet_at_corner_posts(self):
        stair = make_stair()
        flight = get_flights(stair)[0]
        flight.Proxy._updating = True
        flight.EntryDirection = "From left"
        flight.ExitDirection = "To right"
        flight.LeftStringerType = "Housed stringer"
        flight.RightStringerType = "Housed stringer"
        flight.LeftHandrailEnabled = True
        flight.RightHandrailEnabled = True
        flight.Proxy._updating = False
        stair.Proxy.rebuild(stair)
        for side in ("Left", "Right"):
            rails = sorted(
                (
                    part
                    for part in stair.HandrailsGroup.Group
                    if part.Side == side and part.StairDesignerRole == "HandrailTopRail"
                ),
                key=lambda part: part.ElementIndex,
            )
            self.assertEqual(len(rails), 2)
            post = next(
                part
                for part in stair.HandrailsGroup.Group
                if part.Side == side
                and part.StairDesignerRole == "HandrailPost"
                and part.ElementIndex == 1
            )
            for rail in rails:
                rail.Shape.check(True)
                self.assertLess(rail.Shape.distToShape(post.Shape)[0], 1e-7)

    def _short_inner_side_stair(self, stringer_type, handrails=True):
        # The first flight's inner side (110) is shorter than the first going.
        stair = make_stair(
            floor_height=2280.0,
            flight_length=460.0,
            width=700.0,
            steps=11,
            stair_type="Wood",
        )
        stair.Proxy._updating = True
        try:
            stair.Nosing = 30.0
            stair.WalkingLineOffset = 300.0
            flights = resize_flights(stair, 2, width=700.0, rotations=["Right"])
            for flight, (left, right) in zip(flights, ((810.0, 110.0), (2240.0, 1540.0))):
                flight.Proxy._updating = True
                flight.LeftLength = left
                flight.RightLength = right
                for side in ("Left", "Right"):
                    setattr(flight, f"{side}StringerType", stringer_type)
                    setattr(flight, f"{side}HandrailEnabled", handrails)
                flight.Proxy._updating = False
            flights[1].Proxy._updating = True
            flights[1].ExitDirection = "To right"
            flights[1].Proxy._updating = False
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair)
        self.assertEqual(stair.GeometryStatus, "Balanced multi-flight stair")
        return stair, flights

    def test_short_inner_side_winders_and_exit_turn_rails(self):
        # The first winder nosing crosses the whole second flight. It must
        # not drag the following winders onto one point of the inner side,
        # and the second flight, which turns at its exit, must butt the first
        # one's rails and stringers at the corners.
        stair, flights = self._short_inner_side_stair("Housed stringer")

        treads = [
            part.Shape for part in stair.StepsGroup.Group if part.StairDesignerRole == "Tread"
        ]
        # The first tread winds around the inner corner of the first flight;
        # it must not run down the second flight's inner side. Its riser may
        # end at the corner newel, the tread running on under it into the
        # post, not past it.
        self.assertGreater(
            treads[0].BoundBox.YMin,
            -max(stair.HandrailPostWidth.Value, stair.HandrailPostThickness.Value) / 2.0,
        )
        for first in range(len(treads)):
            for second in range(first + 4, len(treads)):
                with self.subTest(first=first, second=second):
                    level = treads[second].copy()
                    level.translate(
                        FreeCAD.Vector(
                            0.0,
                            0.0,
                            treads[first].BoundBox.ZMin - level.BoundBox.ZMin,
                        )
                    )
                    self.assertLess(treads[first].common(level).Volume, 1.0)

        def parts(role, side, flight):
            return [
                part
                for part in stair.HandrailsGroup.Group
                if part.StairDesignerRole == role
                and part.Side == side
                and part.SourceFlight == flight
            ]

        for side, corner in (("Left", (810.0, 700.0)), ("Right", (110.0, 0.0))):
            with self.subTest(side=side):
                posts = [
                    part
                    for part in stair.HandrailsGroup.Group
                    if part.StairDesignerRole == "HandrailPost"
                    and math.dist(
                        (
                            part.Shape.BoundBox.Center.x,
                            part.Shape.BoundBox.Center.y,
                        ),
                        corner,
                    )
                    < 100.0
                ]
                self.assertEqual(len(posts), 1)
                incoming = parts("HandrailTopRail", side, flights[0])
                outgoing = min(
                    parts("HandrailTopRail", side, flights[1]),
                    key=lambda part: part.Shape.distToShape(posts[0].Shape)[0],
                )
                for rail in incoming + [outgoing]:
                    self.assertLess(rail.Shape.distToShape(posts[0].Shape)[0], 1e-6)
                stringer = next(
                    part
                    for part in stair.StringersGroup.Group
                    if part.SourceFlight == flights[1] and part.Side == side
                )
                self.assertLess(stringer.Shape.distToShape(posts[0].Shape)[0], 1e-6)

    def test_notched_stringer_newels_stay_whole_through_the_steps(self):
        # Notched boards run under the tread ends, so the newels stand in the
        # steps. The steps are notched around them; housing them would cut
        # the posts in two and drop their lower part.
        stair, flights = self._short_inner_side_stair("Notched stringer")
        self.assertTrue(stair.StringerJoinery)
        posts = [
            part for part in stair.HandrailsGroup.Group if part.StairDesignerRole == "HandrailPost"
        ]
        steps = [
            part for part in stair.StepsGroup.Group if part.StairDesignerRole in ("Tread", "Riser")
        ]
        for post in posts:
            with self.subTest(post=post.Label):
                self.assertEqual(len(post.Shape.Solids), 1)
                # The post stops at the floor, not below it.
                self.assertGreater(post.Shape.BoundBox.ZMin, -1e-6)
                for step in steps:
                    self.assertLess(post.Shape.common(step.Shape).Volume, 1e-3, step.Label)
        corner = next(
            post
            for post in posts
            if math.dist((post.Shape.BoundBox.Center.x, post.Shape.BoundBox.Center.y), (110.0, 0.0))
            < 100.0
        )
        board = next(
            part
            for part in stair.StringersGroup.Group
            if part.SourceFlight == flights[0] and part.Side == "Right"
        )
        # The first flight's inner side is too short for a rail: the corner
        # newel is the first post of that side and stands on the floor.
        self.assertFalse(
            [
                post
                for post in posts
                if math.dist(
                    (post.Shape.BoundBox.Center.x, post.Shape.BoundBox.Center.y), (0.0, 0.0)
                )
                < 100.0
            ]
        )
        self.assertAlmostEqual(corner.Shape.BoundBox.ZMin, 0.0, places=6)
        # The inner newel carries the first flight's board down to the floor.
        self.assertLess(corner.Shape.BoundBox.ZMin, board.Shape.BoundBox.ZMin + 1e-6)
        self.assertLess(corner.Shape.distToShape(board.Shape)[0], 1e-6)

        def step_volume(x_min, x_max, y_min, y_max):
            box = Part.makeBox(
                x_max - x_min,
                y_max - y_min,
                stair.FloorHeight.Value + 1000.0,
                FreeCAD.Vector(x_min, y_min, -500.0),
            )
            return sum(step.Shape.common(box).Volume for step in steps)

        # The step overhang dies into the newels instead of passing their
        # outer faces: beside the outer floor post, and at the inner corner.
        first = next(post for post in posts if post.Shape.BoundBox.XMin < 0.0)
        bounds = first.Shape.BoundBox
        self.assertLess(bounds.YMax, 699.0)
        self.assertLess(step_volume(bounds.XMin, bounds.XMax, bounds.YMax, 800.0), 1e-3)
        bounds = corner.Shape.BoundBox
        self.assertGreater(bounds.XMin, 111.0)
        self.assertLess(step_volume(110.0, bounds.XMin, 0.0, bounds.YMin), 1e-3)
        # The first flight's treads beside the corner newel are not overhang.
        self.assertGreater(step_volume(110.0, bounds.XMin, bounds.YMin, bounds.YMax), 1000.0)

    def test_housed_newel_board_follows_winder_nosings(self):
        # The second flight's inner board ends at the exit newel and carries
        # the steep winders of the corner. A straight board would have to be
        # far wider than the stair's board to house them all.
        stair, flights = self._short_inner_side_stair("Housed stringer")
        board = next(
            part
            for part in stair.StringersGroup.Group
            if part.SourceFlight == flights[1] and part.Side == "Right"
        )
        self.assertAlmostEqual(board.Width.Value, stair.StringerWidth.Value, places=6)
        self.assertEqual(len(board.Shape.Solids), 1)
        self.assertTrue(board.Shape.isValid())

    @staticmethod
    def _top_at(shape, x, y):
        probe = Part.makeLine(FreeCAD.Vector(x, y, -1000.0), FreeCAD.Vector(x, y, 10000.0))
        crossing = shape.common(probe)
        return crossing.BoundBox.ZMax if crossing.Edges else None

    @staticmethod
    def _bottom_at(shape, x, y):
        probe = Part.makeLine(FreeCAD.Vector(x, y, -1000.0), FreeCAD.Vector(x, y, 10000.0))
        crossing = shape.common(probe)
        return crossing.BoundBox.ZMin if crossing.Edges else None

    def test_housed_boards_keep_timber_under_every_housing(self):
        # Winders turning the corners and converging on the inner newel sink
        # some step corners far below a regular board's lower edge.
        stair, flights = self._short_inner_side_stair("Housed stringer")
        self.assertAlmostEqual(stair.StringerUndersideOffset.Value, 50.0)
        closest = self._closest_housing_margins(stair, flights, 50.0)
        # The lower edge follows the treads rather than one deep pitch: half
        # of them keep the 50 mm square to the board under their closest
        # corner, some 80 mm measured plumb at this pitch. It is one fair
        # curve, which stands a few centimetres further off the others
        # rather than bend at each of them.
        self.assertLess(closest[(len(closest) - 1) // 2], 90.0)
        self.assertLess(closest[len(closest) // 2], 120.0)

    def test_housed_board_underside_position_is_adjustable(self):
        stair, flights = self._short_inner_side_stair("Housed stringer")
        regular = self._closest_housing_margins(stair, flights, 50.0)
        width = stair.StringerWidth.Value
        stair.StringerUndersideOffset = 80.0
        stair.Proxy.rebuild(stair)
        self.assertAlmostEqual(stair.StringerWidth.Value, width + 30.0, places=6)
        deeper = self._closest_housing_margins(stair, flights, 80.0)
        self.assertGreater(deeper[len(deeper) // 2], regular[len(regular) // 2] + 25.0)
        # Measured plumb, the same distance leaves less timber square to the
        # sloping board, and a narrower automatic board.
        self.assertEqual(stair.StringerUndersideOffsetDirection, "Perpendicular")
        stair.StringerUndersideOffsetDirection = "Vertical"
        stair.Proxy.rebuild(stair)
        self.assertLess(stair.StringerWidth.Value, width + 30.0 - 5.0)
        plumb = self._closest_housing_margins(stair, flights, 80.0)
        self.assertLess(plumb[len(plumb) // 2], deeper[len(deeper) // 2] - 10.0)

    def test_open_wedged_housings_let_the_steps_slide_in(self):
        stair, flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        self.assertFalse(stair.StringerOpenHousings)
        self.assertFalse(stair.StringerWedgedHousings)

        def board():
            return next(
                part
                for part in stair.StringersGroup.Group
                if part.SourceFlight == flights[1] and part.Side == "Left"
            ).Shape

        # A straight tread of the second flight, which climbs towards -y, its
        # end housed in the left board at x = 790: the board, its outer face
        # on the stair's side at x = 810, stands 20 beyond the steps.
        treads = [
            part.Shape
            for part in stair.StepsGroup.Group
            if part.StairDesignerRole == "Tread"
            and part.FlightIndex == 2
            and abs(part.Shape.BoundBox.XMax - 790.0) < 1.0
            and abs(part.Shape.BoundBox.XLength - 660.0) < 1.0
        ]
        self.assertGreater(len(treads), 2)
        bounds = treads[len(treads) // 2].BoundBox
        x = bounds.XMax - 3.0
        # Behind the tread at its top, and under its back end.
        behind = FreeCAD.Vector(x, bounds.YMin - 5.0, bounds.ZMax - 3.0)
        under = FreeCAD.Vector(x, bounds.YMin + 30.0, bounds.ZMin - 4.0)

        def solid_at(point):
            return board().isInside(point, 1e-6, True)

        self.assertTrue(solid_at(behind))
        self.assertTrue(solid_at(under))
        volume = board().Volume
        stair.StringerOpenHousings = True
        stair.Proxy.rebuild(stair)
        self.assertTrue(board().isValid())
        self.assertEqual(len(board().Solids), 1)
        self.assertFalse(solid_at(behind))
        self.assertTrue(solid_at(under))
        # The housings stay within the board: its outer face is whole.
        self.assertTrue(board().isInside(FreeCAD.Vector(805.0, behind.y, behind.z), 1e-6, True))
        opened = board().Volume
        self.assertLess(opened, volume)
        stair.StringerWedgedHousings = True
        stair.Proxy.rebuild(stair)
        self.assertTrue(board().isValid())
        self.assertEqual(len(board().Solids), 1)
        self.assertFalse(solid_at(behind))
        self.assertFalse(solid_at(under))
        self.assertLess(board().Volume, opened)

    def test_open_housings_let_the_risers_slide_up(self):
        from stairdesigner.geometry_joinery import WEDGE_TAPER, _plan_section

        stair, flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        # The short board standing on the floor keeps its housings closed:
        # nothing slides into it from below.
        short = next(
            part.Name
            for part in stair.StringersGroup.Group
            if part.SourceFlight == flights[0] and part.Side == "Right"
        )
        closed = stair.Document.getObject(short).Shape.Volume
        # Where each riser, trimmed by the joinery, is housed in the boards
        # as they are before it.
        stair.StringerJoinery = False
        stair.Proxy.rebuild(stair)
        uncut = {board.Name: board.Shape.copy() for board in stair.StringersGroup.Group}
        stair.StringerJoinery = True
        stair.Proxy.rebuild(stair)
        risers = [part for part in stair.StepsGroup.Group if part.StairDesignerRole == "Riser"]
        housed = []
        for riser in risers:
            for board in stair.StringersGroup.Group:
                if board.Name == short:
                    continue
                housing = riser.Shape.common(uncut[board.Name])
                if housing.Volume > 1.0:
                    bounds = housing.BoundBox
                    footprint = _plan_section(housing, bounds.Center.z)
                    housed.append((board.Name, footprint.CenterOfMass, bounds))
        self.assertGreater(len(housed), 4)

        def wood(name, x, y, z0, z1):
            line = Part.makeLine(FreeCAD.Vector(x, y, z0), FreeCAD.Vector(x, y, z1))
            return sum(
                edge.Length for edge in stair.Document.getObject(name).Shape.common(line).Edges
            )

        stair.StringerJoinery = True
        stair.StringerOpenHousings = True
        for wedged in (False, True):
            stair.StringerWedgedHousings = wedged
            stair.Proxy.rebuild(stair)
            self.assertAlmostEqual(stair.Document.getObject(short).Shape.Volume, closed, delta=1.0)
            for name, centre, bounds in housed:
                board = stair.Document.getObject(name)
                self.assertTrue(board.Shape.isValid())
                self.assertEqual(len(board.Shape.Solids), 1)
                # Nothing under the housing down to the lower edge.
                self.assertLess(
                    wood(
                        name, centre.x, centre.y, board.Shape.BoundBox.ZMin - 5.0, bounds.ZMax - 0.5
                    ),
                    0.5,
                    name,
                )
        # Wedged, the gap behind each riser follows it, angled ones too:
        # open at its foot, closing towards its top.
        stair.Proxy.rebuild(stair)
        angled = 0
        for riser in risers:
            bounds = riser.Shape.BoundBox
            section = _plan_section(riser.Shape, bounds.Center.z)
            edge = max(section.OuterWire.Edges, key=lambda edge: edge.Length)
            direction = edge.Vertexes[-1].Point - edge.Vertexes[0].Point
            direction.z = 0.0
            direction.normalize()
            normal = FreeCAD.Vector(-direction.y, direction.x, 0.0)
            for name, centre, housing_bounds in housed:
                if abs(housing_bounds.ZMin - bounds.ZMin) > 1.0 or not section.isInside(
                    FreeCAD.Vector(centre.x, centre.y, section.CenterOfMass.z), 1e-3, True
                ):
                    continue
                height = housing_bounds.ZMax - housing_bounds.ZMin
                gaps = []
                checked = False
                for side in (normal, -normal):
                    back = max(vertex.Point.dot(side) for vertex in section.Vertexes)
                    point = centre + side * (back - centre.dot(side) + 0.5 * WEDGE_TAPER * height)
                    line = Part.makeLine(
                        FreeCAD.Vector(point.x, point.y, housing_bounds.ZMin + 1.0),
                        FreeCAD.Vector(point.x, point.y, housing_bounds.ZMax - 1.0),
                    )
                    # Only where the board stands behind the riser, not past
                    # its end.
                    if uncut[name].common(line).Length < height - 3.0:
                        gaps.append(False)
                        continue
                    checked = True
                    gaps.append(
                        wood(
                            name,
                            point.x,
                            point.y,
                            housing_bounds.ZMin + 1.0,
                            housing_bounds.ZMin + 0.2 * height,
                        )
                        < 0.5
                        and wood(
                            name,
                            point.x,
                            point.y,
                            housing_bounds.ZMax - 0.2 * height,
                            housing_bounds.ZMax - 1.0,
                        )
                        > 0.5
                    )
                if checked:
                    self.assertEqual(sum(gaps), 1, name)
                    if abs(direction.x) > 1e-3 and abs(direction.y) > 1e-3:
                        angled += 1
        self.assertGreater(angled, 0)

    def test_wedges_fill_the_room_left_on_the_stock(self):
        from stairdesigner.geometry_joinery import WEDGE_TAPER
        from stairdesigner.stair_bom import default_materials, flat_pieces, stair_layout

        stair, _flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        stair.StringerOpenHousings = True
        stair.StringerWedgedHousings = True
        stair.Proxy.rebuild(stair)
        pieces = flat_pieces(stair)
        wedges = [piece for piece in pieces if piece.kind == "Wedge"]
        recorded = sum(
            len(getattr(part, "TreadWedges", [])) + len(getattr(part, "RiserWedges", []))
            for part in stair.StringersGroup.Group
        )
        self.assertGreater(recorded, 4)
        self.assertEqual(len(wedges), recorded)
        for wedge in wedges:
            # Tapering 1 in 10 from its tip.
            xs = [x for x, _y in wedge.outline]
            ys = [y for _x, y in wedge.outline]
            self.assertLessEqual(max(ys), WEDGE_TAPER * max(xs) + 1e-6)

        materials, assignment = default_materials(stair, pieces)
        self.assertEqual(assignment["Wedge"], assignment["Stringer"])
        others = [piece for piece in pieces if piece.kind != "Wedge"]
        without, _unassigned = stair_layout(materials, assignment, others)
        layouts, unassigned = stair_layout(materials, assignment, pieces)
        self.assertEqual(unassigned, [])
        # The wedges take no stock of their own.
        self.assertEqual(
            [len(found.stocks) for found in layouts], [len(found.stocks) for found in without]
        )
        placed = [
            item
            for found in layouts
            for stock in found.stocks
            for item in stock.placed
            if item.part.kind == "Wedge"
        ]
        offcuts = [piece for found in layouts for piece in found.unplaced if piece.kind == "Wedge"]
        self.assertEqual(len(placed) + len(offcuts), len(wedges))
        self.assertTrue(placed)
        for found in layouts:
            for stock in found.stocks:
                faces = [
                    Part.Face(
                        Part.makePolygon(
                            [FreeCAD.Vector(x, y, 0) for x, y in item.outline + item.outline[:1]]
                        )
                    )
                    for item in stock.placed
                ]
                for index, face in enumerate(faces):
                    for other in faces[index + 1 :]:
                        self.assertLess(face.common(other).Area, 1.0)

    def test_stringers_end_level_on_a_last_tread_with_no_riser(self):
        # Without a final riser the last tread is the floor: the board over it
        # runs on level instead of climbing a riser that is not there.
        stair, _flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        floor = stair.FloorHeight.Value
        for end_with_riser in (True, False):
            stair.EndWithRiser = end_with_riser
            stair.Proxy.rebuild(stair, allow_structure_changes=True)
            self.document.recompute()
            riser = stair.RiserHeight.Value
            parts = [part for part in stair.StringersGroup.Group if not part.Shape.isNull()]
            self.assertTrue(parts)
            top = max(part.Shape.BoundBox.ZMax for part in parts)
            # Still above the floor-level tread it houses, nowhere near a
            # riser above it.
            self.assertGreater(top, floor, end_with_riser)
            self.assertLess(top, floor + 0.5 * riser, end_with_riser)
            for part in parts:
                self.assertTrue(part.Shape.isValid(), part.Label)

    def test_bill_of_materials_lists_the_stock_of_every_member(self):
        import csv
        import io

        from stairdesigner.stair_bom import stair_stock, stock_csv, stock_totals

        stair, flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        self.assertNotIn("BomStockLayout", stair.PropertiesList)
        treads = {
            part.Label: part.Shape
            for part in stair.StepsGroup.Group
            if part.StairDesignerRole == "Tread"
        }
        rows = stair_stock(stair)
        stringers = [row for row in rows if row["kind"] == "Stringer"]
        self.assertTrue(stringers)
        for row in stringers:
            self.assertAlmostEqual(row["thickness"], stair.StringerThickness.Value, delta=1.0)
            self.assertGreater(row["length"], row["width"])
        # Every tread once, its stock as thick as the step.
        tread_rows = [row for row in rows if row["kind"] == "Tread"]
        self.assertEqual(
            sorted(label for row in tread_rows for label in row["parts"]), sorted(treads)
        )
        for row in tread_rows:
            self.assertAlmostEqual(row["thickness"], stair.StepThickness.Value, delta=1.0)
        widest = max(row["width"] for row in tread_rows)

        # Glued up from narrower planks: every tread is covered by its strips.
        plank = 200.0
        self.assertLess(plank, widest)
        rows = stair_stock(stair, plank)
        covered = {}
        for row in rows:
            if row["kind"] not in ("Tread", "Tread strip"):
                continue
            self.assertLessEqual(row["width"], plank + 1.0)
            self.assertAlmostEqual(row["stock_width"], plank)
            for label in row["parts"]:
                if row["kind"] == "Tread strip":
                    # Strips are numbered after their tread.
                    label = label.rsplit(" (", 1)[0]
                covered[label] = covered.get(label, 0.0) + row["length"] * row["width"]
        self.assertEqual(sorted(covered), sorted(treads))
        for label, shape in treads.items():
            top = max(shape.Faces, key=lambda face: face.Area).Area
            self.assertGreaterEqual(covered[label], top - 1.0, label)
        self.assertIn(plank, [width for _thickness, width, _length in stock_totals(rows)])
        table = list(csv.reader(io.StringIO(stock_csv(rows))))
        self.assertEqual(table[0][0], "Part")
        self.assertEqual(len(table), len(rows) + 3 + len(stock_totals(rows)))

    def test_tapered_parts_pair_head_to_tail_on_the_stock(self):
        from stairdesigner.stair_nesting import lay_out, polygon_area

        # Two winder-like treads, nosing along X, tapering across it: turned
        # half a turn, one fits along the other's back on a narrow plank.
        tread = [(0.0, 0.0), (900.0, 0.0), (900.0, 400.0), (0.0, 120.0)]
        other = [(0.0, 0.0), (900.0, 0.0), (900.0, 400.0), (0.0, 120.0)]
        stocks, unplaced = lay_out([("a", tread), ("b", other)], 3000.0, 530.0, 5.0)
        self.assertEqual(unplaced, [])
        self.assertEqual(len(stocks), 1)
        placed = stocks[0].placed
        self.assertEqual(sorted(item.turned for item in placed), [False, True])
        for item in placed:
            self.assertAlmostEqual(polygon_area(item.outline), polygon_area(tread))
            for x, y in item.outline:
                self.assertTrue(-1e-6 <= x <= 3000.0 + 1e-6 and -1e-6 <= y <= 530.0 + 1e-6)
            # Half a turn keeps the grain along the plank.
            xs = [x for x, _y in item.outline]
            self.assertAlmostEqual(max(xs) - min(xs), 900.0)
        faces = [
            Part.Face(
                Part.makePolygon(
                    [FreeCAD.Vector(x, y, 0) for x, y in item.outline + item.outline[:1]]
                )
            )
            for item in placed
        ]
        self.assertLess(faces[0].common(faces[1]).Area, 1e-6)
        self.assertGreaterEqual(faces[0].distToShape(faces[1])[0], 5.0 - 1e-6)
        # A part larger than the stock is reported, not lost.
        stocks, unplaced = lay_out(
            [("big", [(0, 0), (4000, 0), (4000, 100), (0, 100)])], 3000.0, 300.0
        )
        self.assertEqual((stocks, unplaced), ([], ["big"]))

    def test_stock_layout_counts_planks_and_exports_dxf(self):
        from stairdesigner.stair_bom import (
            Material,
            default_materials,
            flat_pieces,
            set_stair_materials,
            stair_layout,
            stair_materials,
        )
        from stairdesigner.stair_dxf import stock_dxf

        stair, _flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        pieces = flat_pieces(stair)
        boards = [piece for piece in pieces if piece.kind == "Stringer"]
        self.assertEqual(
            len(boards),
            len([part for part in stair.StringersGroup.Group if not part.Shape.isNull()]),
        )
        # Housed boards lie with their housings up, to the overlap's depth;
        # mortises for the next board's tenon go deeper, never through.
        overlap = stair.StringerStepOverlap.Value
        for board in boards:
            depths = [depth for depth, _outline in board.pockets + board.back_pockets]
            self.assertIn(
                round(overlap, 2), [depth for depth, _outline in board.pockets], board.label
            )
            self.assertLess(max(depths), board.thickness - 1.0, board.label)

        materials, assignment = default_materials(stair, pieces)
        # Each material keeps its own cut spacing.
        materials[1].spacing = 3.2
        set_stair_materials(stair, materials, assignment)
        self.assertEqual(
            [material.spacing for material in stair_materials(stair)[0]], [5.0, 3.2, 5.0]
        )
        longest = max(board.length for board in boards)
        self.assertGreater(materials[0].length, longest)
        # A board longer than the planks is reported, not dropped.
        length = materials[0].length
        materials[0].length = longest - 10.0
        layouts, _unassigned = stair_layout(materials, assignment, pieces)
        self.assertIn(max(boards, key=lambda board: board.length), layouts[0].unplaced)
        materials[0].length = length
        layouts, unassigned = stair_layout(materials, assignment, pieces)
        self.assertEqual(unassigned, [])
        self.assertEqual(len(layouts), 3)
        for found in layouts:
            self.assertEqual(found.unplaced, [])
            material = found.material
            for stock in found.stocks:
                faces = []
                for item in stock.placed:
                    for x, y in item.outline:
                        self.assertGreaterEqual(x, -1e-6)
                        self.assertGreaterEqual(y, -1e-6)
                        self.assertLessEqual(x, material.length + 1e-6)
                        self.assertLessEqual(y, material.width + 1e-6)
                    faces.append(
                        Part.Face(
                            Part.makePolygon(
                                [
                                    FreeCAD.Vector(x, y, 0)
                                    for x, y in item.outline + item.outline[:1]
                                ]
                            )
                        )
                    )
                for index, face in enumerate(faces):
                    for other in faces[index + 1 :]:
                        self.assertLess(face.common(other).Area, 1.0)
        # Treads wider than their planks come as strips, all laid out.
        treads = [found for found in layouts if found.material.name == assignment["Tread"]][0]
        kinds = {item.part.kind for stock in treads.stocks for item in stock.placed}
        self.assertIn("Tread strip", kinds)

        # One panel for everything: the thinner risers get panels of their own.
        panel = Material("Panel", 2500.0, 1250.0)
        layouts, unassigned = stair_layout(
            [panel], {"Stringer": "Panel", "Tread": "Panel", "Riser": "Panel"}, pieces
        )
        self.assertEqual(unassigned, [])
        self.assertEqual(
            [found.thickness for found in layouts],
            [stair.StepThickness.Value, stair.RiserThickness.Value],
        )
        kinds = [
            {item.part.kind for stock in found.stocks for item in stock.placed} for found in layouts
        ]
        self.assertNotIn("Tread strip", kinds[0])
        self.assertEqual(kinds[1], {"Riser"})

        text = stock_dxf(treads.stocks)
        self.assertTrue(text.endswith("0\nEOF\n"))
        self.assertEqual(
            text.count("\nPOLYLINE\n"),
            len(treads.stocks) + sum(len(stock.placed) for stock in treads.stocks),
        )
        stringers = [found for found in layouts if found.material.name == "Panel"][0]
        self.assertIn("\nPOCKET_", stock_dxf(stringers.stocks))

    def test_flat_pieces_know_how_they_lie_in_their_part(self):
        from stairdesigner.stair_bom import flat_pieces

        stair, _flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        pieces = [piece for piece in flat_pieces(stair) if piece.kind != "Wedge"]
        self.assertGreater(len(pieces), 10)
        sources = [piece.source.Name for piece in pieces]
        checked = 0
        for piece in pieces:
            if sources.count(piece.source.Name) > 1:
                # One of the boards of a part: the part reaches further.
                continue
            checked += 1
            with self.subTest(piece=piece.label):
                shape = piece.source.Shape
                origin, along, across = piece.frame
                self.assertAlmostEqual(along.Length, 1.0, places=9)
                self.assertAlmostEqual(across.Length, 1.0, places=9)
                self.assertAlmostEqual(along.dot(across), 0.0, places=9)
                # The outline is measured from the frame's origin, along
                # its axes: the part reaches as far as the outline does.
                points = [
                    point for edge in shape.Edges for point in edge.discretize(Deflection=0.05)
                ]
                # A winder's riser has its ends cut on the bevel: the board
                # is longer than its face by up to its thickness.
                bevel = piece.thickness if piece.kind == "Riser" else 0.0
                for axis, size in ((along, piece.length), (across, piece.width)):
                    reach = [point.dot(axis) for point in points]
                    self.assertAlmostEqual(min(reach), origin.dot(axis), delta=0.3 + bevel)
                    self.assertAlmostEqual(max(reach) - min(reach), size, delta=0.3 + bevel)
                    self.assertGreaterEqual(max(reach) - min(reach), size - 0.3)
        self.assertGreater(checked, 10)

    def _closest_housing_margins(self, stair, flights, timber):
        """Return each housed tread's smallest plumb margin to the second
        flight's inner board underside, sorted, checking ``timber`` square
        to the board under every corner."""

        steps = [
            (part.StairDesignerRole, part.Shape)
            for part in stair.StepsGroup.Group
            if part.StairDesignerRole in ("Tread", "Riser")
        ]
        board = next(
            part
            for part in stair.StringersGroup.Group
            if part.SourceFlight == flights[1] and part.Side == "Right"
        )
        bounds = board.Shape.BoundBox
        # The board stands across x; its outer face, free of housings, is
        # the low-x one.
        slab = Part.makeBox(
            bounds.XLength,
            bounds.YLength,
            stair.FloorHeight.Value,
            FreeCAD.Vector(bounds.XMin, bounds.YMin, 0.0),
        )
        closest = []
        for role, step in steps:
            housed = step.common(slab)
            if housed.Volume < 1.0:
                continue
            lowest = min(vertex.Point.z for vertex in housed.Vertexes)
            if lowest < 1.0:
                continue
            margins = []
            for vertex in housed.Vertexes:
                point = vertex.Point
                if point.z > lowest + 1e-6 or not bounds.YMin + 1.0 < point.y < bounds.YMax - 1.0:
                    continue
                bottom = self._bottom_at(board.Shape, bounds.XMin + 1.0, point.y)
                if bottom is None:
                    continue
                margins.append(point.z - bottom)
                self.assertLessEqual(bottom, point.z - timber + 1e-3, (point.y, point.z))
            if margins and role == "Tread":
                closest.append(min(margins))
        self.assertGreater(len(closest), 3)
        return sorted(closest)

    def test_housed_board_starts_past_the_first_nosing(self):
        # Without posts, nothing trims the board's front end.
        stair, flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        tread = min(
            (part for part in stair.StepsGroup.Group if part.StairDesignerRole == "Tread"),
            key=lambda part: part.Shape.BoundBox.ZMin,
        )
        board = next(
            part
            for part in stair.StringersGroup.Group
            if part.SourceFlight == flights[0] and part.Side == "Left"
        )
        self.assertAlmostEqual(stair.StringerStartExtension.Value, 30.0)
        # 30 mm beyond the nosing tip, which the board houses.
        self.assertAlmostEqual(
            board.Shape.BoundBox.XMin,
            tread.Shape.BoundBox.XMin - 30.0,
            places=3,
        )

    def test_short_inner_side_gets_a_short_board_on_the_floor(self):
        # The first flight's 110 mm inner side is shorter than one going: its
        # winders climb far too steeply for a board following their nosings.
        # A short board stands on the floor there instead, from the first
        # nosing to the corner newel, with no post in front and no rail.
        stair, flights = self._short_inner_side_stair("Housed stringer")
        boards = [
            part
            for part in stair.StringersGroup.Group
            if part.SourceFlight == flights[0] and part.Side == "Right"
        ]
        self.assertEqual(len(boards), 1)
        board = boards[0].Shape
        self.assertTrue(board.isValid())
        self.assertEqual(len(board.Solids), 1)
        bounds = board.BoundBox
        self.assertAlmostEqual(bounds.ZMin, 0.0, places=6)

        posts = [
            part for part in stair.HandrailsGroup.Group if part.StairDesignerRole == "HandrailPost"
        ]

        def near(post, x, y):
            centre = post.Shape.BoundBox.Center
            return math.dist((centre.x, centre.y), (x, y)) < 100.0

        self.assertFalse([post for post in posts if near(post, 0.0, 0.0)])
        self.assertFalse(
            [
                part
                for part in stair.HandrailsGroup.Group
                if part.StairDesignerRole == "HandrailTopRail"
                and part.SourceFlight == flights[0]
                and part.Side == "Right"
            ]
        )
        # The corner newel stands on the floor and takes the board's end.
        corner = next(post for post in posts if near(post, 110.0, 0.0))
        self.assertAlmostEqual(corner.Shape.BoundBox.ZMin, 0.0, places=6)
        self.assertLess(board.distToShape(corner.Shape)[0], 1e-6)

        # Its winders are clipped where they converge on the newel: only the
        # first tread ends in the board, which stands just above it, level,
        # from as far in front of the nosing as any board starts.
        treads = [
            part.Shape for part in stair.StepsGroup.Group if part.StairDesignerRole == "Tread"
        ]
        first = min(treads, key=lambda tread: tread.BoundBox.ZMin)
        self.assertLess(first.distToShape(board)[0], 1e-6)
        self.assertAlmostEqual(
            bounds.ZMax, first.BoundBox.ZMax + stair.StringerNosingOffset.Value, places=3
        )
        self.assertAlmostEqual(
            bounds.XMin, first.BoundBox.XMin - stair.StringerStartExtension.Value, places=3
        )

        # The first step keeps its square end, housed in the board.
        nose = first.BoundBox.XMin
        self.assertTrue(
            first.isInside(FreeCAD.Vector(nose + 5.0, 37.0, first.BoundBox.ZMax - 1.0), 1e-6, True)
        )

    def test_short_board_shoulders_up_to_the_treads_it_houses(self):
        from stairdesigner.geometry_stringer_shapes import make_short_housed_stringer_shape

        # A side 190 long, along y = 0, the steps on its +y side.
        def section(u, top):
            return BalancedSection(
                center=(u, 350.0),
                tangent=(1.0, 0.0),
                left=(u, 700.0),
                right=(u, 0.0),
                station=u,
                width=700.0,
                flight_index=0,
                top_elevation=top,
            )

        sections = [section(0.0, 200.0), section(100.0, 400.0), section(190.0, 600.0)]

        def tread(start, end, top):
            return Part.makeBox(end - start, 300.0, 40.0, FreeCAD.Vector(start, 0.0, top - 40.0))

        def board(steps):
            # 40 thick, housing the steps 20 deep; 60 before the first riser,
            # 20 of timber over the nosings of 30; the newel takes the treads
            # starting within 90 of the corner.
            shape = make_short_housed_stringer_shape(
                sections, 200.0, "Right", 40.0, 20.0, 60.0, 20.0, 30.0, steps, 90.0
            )
            self.assertTrue(shape.isValid())
            self.assertEqual(len(shape.Solids), 1)
            return shape

        def top(shape, x):
            line = Part.makeLine(FreeCAD.Vector(x, -10.0, -10.0), FreeCAD.Vector(x, -10.0, 2000.0))
            return max(vertex.Point.z for vertex in shape.common(line).Vertexes)

        def front(shape):
            return max(
                vertex.Point.z
                for vertex in shape.Vertexes
                if abs(vertex.Point.x - shape.BoundBox.XMin) < 1e-6
            )

        # Three treads end in the board; the fourth starts at the newel.
        treads = [
            tread(-30.0, 100.0, 200.0),
            tread(70.0, 120.0, 400.0),
            tread(90.0, 190.0, 600.0),
            tread(160.0, 400.0, 800.0),
        ]
        shape = board(treads)
        bounds = shape.BoundBox
        for value, expected in (
            (bounds.XMin, -60.0),
            (bounds.XMax, 190.0),
            (bounds.YMin, -20.0),
            (bounds.YMax, 20.0),
            (bounds.ZMin, 0.0),
            (bounds.ZMax, 620.0),
        ):
            self.assertAlmostEqual(value, expected, places=6)
        # A plumb front as high as the first tread asks, then a quarter of
        # an ellipse up to the level top, reached above the last nosing.
        self.assertAlmostEqual(front(shape), 220.0, places=6)
        self.assertAlmostEqual(top(shape, 0.0), 220.0 + 400.0 * 0.8, places=3)
        self.assertAlmostEqual(top(shape, 90.0), 620.0, places=6)
        self.assertAlmostEqual(top(shape, 150.0), 620.0, places=6)
        # The timber kept in front of each nosing tip, and above it.
        for start, level in ((-30.0, 200.0), (70.0, 400.0), (90.0, 600.0)):
            for x, z in ((start - 19.0, level), (start, level + 19.0)):
                self.assertTrue(shape.isInside(FreeCAD.Vector(x, 0.0, z), 1e-6, True), (x, z))

        # A high tread far in front lifts the front for its nosing.
        high = board([treads[0], tread(-30.0, 100.0, 560.0), treads[2]])
        self.assertGreater(front(high), 500.0)
        self.assertAlmostEqual(high.BoundBox.ZMax, 620.0, places=6)
        for x, z in ((-49.0, 560.0), (-30.0, 579.0)):
            self.assertTrue(high.isInside(FreeCAD.Vector(x, 0.0, z), 1e-6, True), (x, z))

        # With only its first tread, or none given, the board is a level
        # block above the first step.
        for steps in (treads[:1], [treads[0], treads[3]], []):
            block = board(steps)
            self.assertAlmostEqual(block.BoundBox.ZMax, 220.0, places=6)
            self.assertAlmostEqual(block.Volume, 250.0 * 40.0 * 220.0, places=3)

    def test_rounded_first_step_frees_its_end(self):
        # On a side too short for a regular board the rounded end stands in
        # front of the corner newel, which carries the winders alone.
        stair, flights = self._short_inner_side_stair("Housed stringer")
        self.assertEqual(stair.RoundedFirstStep, "None")
        stair.RoundedFirstStep = "Right"
        self.assertFalse(
            [
                part
                for part in stair.StringersGroup.Group
                if part.SourceFlight == flights[0] and part.Side == "Right"
            ]
        )
        tread = min(
            (part for part in stair.StepsGroup.Group if part.StairDesignerRole == "Tread"),
            key=lambda part: part.Shape.BoundBox.ZMin,
        ).Shape
        self.assertEqual(len(tread.Solids), 1)
        self.assertTrue(tread.isValid())
        bounds = tread.BoundBox
        nose = bounds.XMin
        # Through the whole thickness: the square corner at the nosing is
        # gone, the arc keeps the middle of the step's end, and the wall side
        # stays square. The steps end half a post (35) within the stair's
        # sides, y = 0 and 700.
        for z in (bounds.ZMin + 1.0, bounds.ZMax - 1.0):
            with self.subTest(z=z):

                def inside(x, y):
                    return tread.isInside(FreeCAD.Vector(x, y, z), 1e-6, True)

                self.assertFalse(inside(nose + 50.0, 37.0))
                self.assertTrue(inside(nose + 50.0, 95.0))
                self.assertTrue(inside(nose + 5.0, 660.0))

    def test_rounded_first_step_starts_the_boards_at_the_second_riser(self):
        for stringer_type in ("Housed stringer", "Notched stringer", "None"):
            with self.subTest(stringer_type=stringer_type):
                stair = make_stair(flight_length=3600.0, width=900.0, steps=16)
                flight = get_flights(stair)[0]
                for side in ("Left", "Right"):
                    setattr(flight, f"{side}StringerType", stringer_type)
                    setattr(flight, f"{side}HandrailEnabled", stringer_type != "None")
                stair.Proxy.rebuild(stair)

                def members():
                    steps = sorted(
                        (
                            part
                            for part in stair.StepsGroup.Group
                            if part.StairDesignerRole == "Tread"
                        ),
                        key=lambda part: part.Shape.BoundBox.ZMin,
                    )
                    boards = list(stair.StringersGroup.Group) if stair.StringersGroup else []
                    posts = (
                        [
                            part
                            for part in stair.HandrailsGroup.Group
                            if part.StairDesignerRole == "HandrailPost"
                        ]
                        if stair.HandrailsGroup
                        else []
                    )
                    return steps, boards, posts

                steps, boards, posts = members()
                whole = steps[0].Shape.copy()
                second = steps[1].Shape.Volume
                riser = steps[1].Shape.BoundBox.XMin + stair.Nosing.Value
                starts = {part.Side: part.Shape.BoundBox.XMin for part in boards}

                stair.RoundedFirstStep = "Both sides"
                steps, boards, posts = members()
                tread = steps[0].Shape
                self.assertTrue(tread.isValid())
                self.assertEqual(len(tread.Solids), 1)
                # The same bite out of both front corners, where it was.
                removed = whole.cut(tread)
                self.assertEqual(len(removed.Solids), 2)
                first, last = sorted(removed.Solids, key=lambda solid: solid.BoundBox.YMin)
                self.assertAlmostEqual(first.Volume, last.Volume, delta=1.0)
                self.assertGreater(first.Volume, 1e5)
                middle = whole.BoundBox.Center.y
                self.assertAlmostEqual(
                    middle - first.BoundBox.YMin, last.BoundBox.YMax - middle, places=3
                )
                self.assertAlmostEqual(tread.BoundBox.XMin, whole.BoundBox.XMin, places=6)
                # The second step is whole, but for the posts now beside it.
                self.assertGreater(steps[1].Shape.Volume, 0.95 * second)
                self.assertAlmostEqual(
                    steps[1].Shape.BoundBox.XMin, riser - stair.Nosing.Value, places=6
                )
                # The boards and the first posts stand behind the first step.
                for part in boards:
                    self.assertGreater(part.Shape.BoundBox.XMin, starts[part.Side] + 100.0)
                    self.assertLess(part.Shape.BoundBox.XMin, riser + 1.0)
                    self.assertTrue(part.Shape.isValid())
                for side in ("Left", "Right"):
                    if posts:
                        post = min(
                            (part for part in posts if part.Side == side),
                            key=lambda part: part.Shape.BoundBox.XMin,
                        )
                        self.assertAlmostEqual(post.Shape.BoundBox.Center.x, riser, delta=100.0)
                        self.assertGreater(post.Shape.BoundBox.XMin, starts.get(side, 0.0) + 100.0)

                stair.RoundedFirstStep = "Left"
                steps, boards, _posts = members()
                removed = whole.cut(steps[0].Shape)
                self.assertEqual(len(removed.Solids), 1)
                self.assertGreater(removed.BoundBox.YMin, middle)
                self.assertAlmostEqual(removed.Volume, first.Volume, delta=1.0)
                for part in boards:
                    if part.Side == "Right":
                        self.assertAlmostEqual(part.Shape.BoundBox.XMin, starts["Right"], places=3)

                stair.RoundedFirstStep = "None"
                steps, _boards, _posts = members()
                self.assertAlmostEqual(steps[0].Shape.Volume, whole.Volume, delta=1.0)

    def test_shapes_cut_in_place_keep_their_part_placement(self):
        from stairdesigner.object_utils import _set_global_shape

        part = self.document.addObject("Part::Feature", "Placed")
        part.Shape = Part.makeBox(100.0, 50.0, 20.0)
        placement = FreeCAD.Placement(
            FreeCAD.Vector(10.0, 35.0, 5.0), FreeCAD.Rotation(FreeCAD.Vector(0.0, 0.0, 1.0), 30.0)
        )
        part.Placement = placement
        whole = part.Shape.copy()
        corner = whole.Vertexes[0].Point
        cut = whole.cut(Part.makeSphere(15.0, corner))
        self.assertLess(cut.Volume, whole.Volume - 100.0)
        _set_global_shape(part, cut)
        self.assertTrue(part.Placement.isSame(placement, 1e-9))
        self.assertAlmostEqual(part.Shape.Volume, cut.Volume, places=3)
        self.assertLess(part.Shape.cut(cut).Volume, 1e-3)
        self.assertLess(cut.cut(part.Shape).Volume, 1e-3)

    def test_wall_boards_meet_level_at_corners(self):
        # The stair rises through a corner, so two sloping boards cannot be
        # flush across their thickness there. They meet on a level block,
        # the lower one ramping up to it, at the flight seam and at the turn
        # of the exit. Without posts, which would stand over both corners.
        stair, flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        boards = {
            part.SourceFlight: part.Shape
            for part in stair.StringersGroup.Group
            if part.Side == "Left"
        }
        first, second = boards[flights[0]], boards[flights[1]]
        both = first.fuse(second)
        thickness = stair.StringerThickness.Value
        side = second.BoundBox
        corners = (
            # Flight seam: the second flight's side board crosses the first's.
            (side.XMax - thickness, side.XMax, first.BoundBox.YMin, first.BoundBox.YMax),
            # Exit turn: the end board crosses the side board.
            (side.XMax - thickness, side.XMax, side.YMin, side.YMin + thickness),
        )
        for x_min, x_max, y_min, y_max in corners:
            for probe in (self._top_at, self._bottom_at):
                with self.subTest(corner=(x_min, y_min), edge=probe.__name__):
                    heights = [
                        probe(both, x, y)
                        for x in (x_min + 2.0, (x_min + x_max) / 2.0, x_max - 2.0)
                        for y in (y_min + 2.0, (y_min + y_max) / 2.0, y_max - 2.0)
                    ]
                    self.assertNotIn(None, heights)
                    self.assertLess(max(heights) - min(heights), 1e-3)
                    level = heights[0]
                    # Both boards leave the block from its level, on either
                    # face, above and below.
                    for x, y in (
                        (x_min - 1.0, y_min + 2.0),
                        (x_min - 1.0, y_max - 2.0),
                        (x_min + 2.0, y_min - 1.0),
                        (x_max - 2.0, y_min - 1.0),
                        (x_min + 2.0, y_max + 1.0),
                        (x_max - 2.0, y_max + 1.0),
                    ):
                        height = probe(both, x, y)
                        if height is not None:
                            self.assertLess(abs(height - level), 1.0, (x, y))

    def test_exit_turn_end_board_runs_past_the_last_nosing(self):
        # Turning at its exit, the outer side's last board crosses the
        # flight under the last tread, whose nosing overhangs its end.
        stair, flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        last = max(
            (part for part in stair.StepsGroup.Group if part.StairDesignerRole == "Tread"),
            key=lambda part: part.Shape.BoundBox.ZMax,
        )
        # The end board is a part of its own, after the flight's main board.
        board = next(
            part
            for part in stair.StringersGroup.Group
            if part.SourceFlight == flights[1]
            and part.Side == "Left"
            and part.StairDesignerRole == "LeftStringerBoard"
        )
        self.assertIn(", end board", board.Label)
        self.assertAlmostEqual(stair.StringerEndExtension.Value, 0.0)
        self.assertLessEqual(board.Shape.BoundBox.XMin, last.Shape.BoundBox.XMin + 1e-3)

    def test_short_flight_straight_part_stops_at_inner_corner(self):
        from stairdesigner.geometry_winders import (
            _herse_corner_trims,
            _inner_corner_setbacks,
        )

        self.assertEqual(
            tuple(round(value, 9) for value in _inner_corner_setbacks((0, 700, 0), (0, 900, -90))),
            (450.0, 350.0),
        )
        self.assertEqual(_inner_corner_setbacks((0, 700, 0), (0, 700, 0)), (0.0, 0.0))
        # Inner side 110: perpendicular nosings must stop 350 before the
        # axis vertex, although 48% of the 460 axis would stop at 221.
        specs = [(460.0, 700.0, 0.0), (1890.0, 700.0, -90.0)]
        incoming, outgoing = _herse_corner_trims(specs, [(50.0, 1.0)])[0]
        self.assertGreaterEqual(incoming, 350.0)
        self.assertLessEqual(incoming, 460.0 * 0.98)
        self.assertAlmostEqual(outgoing, 700.0)
        # Unchanged where the requested zone already fits in the flight.
        long_specs = [(3500.0, 1000.0, 0.0), (3500.0, 1000.0, 90.0)]
        self.assertEqual(_herse_corner_trims(long_specs, [(50.0, 1.0)]), [(1000.0, 1000.0)])

    def test_off_rail_nosing_does_not_drag_its_neighbours(self):
        from stairdesigner.geometry_plan import (
            _boundary_data,
            _boundary_point,
            _monotone_boundary_parameters,
        )

        footprint = make_stair_footprint([(1000.0, 500.0, 0.0)])
        boundary = _boundary_data(footprint)
        # The middle nosing crossed to the opposite side; the others are
        # evenly spaced along the right side.
        points = [(0.0, 0.0), (250.0, 0.0), (500.0, 500.0), (750.0, 0.0), (1000.0, 0.0)]
        sections = [
            BalancedSection(
                center=(x, 250.0),
                tangent=(1.0, 0.0),
                left=(x, 500.0),
                right=point,
                station=x,
                width=500.0,
                flight_index=0,
            )
            for x, point in zip((0.0, 250.0, 500.0, 750.0, 1000.0), points)
        ]
        orientation = 1.0 if boundary["signed_area"] > 0.0 else -1.0
        parameters = _monotone_boundary_parameters(points, sections, boundary, orientation)
        for parameter, expected in zip(parameters, (0.0, 250.0, 500.0, 750.0, 1000.0)):
            point = _boundary_point(parameter, boundary)
            self.assertAlmostEqual(point[0], expected, places=6)
            self.assertAlmostEqual(point[1], 0.0, places=6)

    def test_walking_line_offset_changes_winder_distribution(self):
        from stairdesigner.geometry_winders import _walking_path_stations

        # The walking line 300 from the inner side of a 1000 wide quarter
        # turn runs 200 inside its middle: shorter by 200*pi/2. Past the
        # middle, it runs in the middle.
        points = [
            (1000 * math.cos(i * math.pi / 2000), 1000 * math.sin(i * math.pi / 2000), 1000.0, 0)
            for i in range(1001)
        ]
        center = _walking_path_stations(points)[-1]
        inside = _walking_path_stations(points, 300.0)[-1]
        self.assertAlmostEqual(center - inside, 200 * math.pi / 2, delta=0.4)
        self.assertEqual(_walking_path_stations(points, 1000.0)[-1], center)
        stair = make_stair()
        flight = get_flights(stair)[0]
        flight.EntryDirection = "From left"
        flight.ExitDirection = "To left"
        going = stair.TreadWidth.Value
        first_shape = stair.StepsGroup.Group[0].Shape.copy()
        # Nearer the inner side, the walking line is shorter.
        stair.WalkingLineOffset = 300.0
        self.assertLess(stair.TreadWidth.Value, going)
        self.assertFalse(first_shape.isEqual(stair.StepsGroup.Group[0].Shape))
        self.assertEqual(stair.GeometryStatus, "Balanced entry/exit stair")

    def test_standalone_landing_has_no_spurious_risers(self):
        stair = make_stair()
        flight = get_flights(stair)[0]
        flight.EntryDirection = "From left"
        flight.ExitDirection = "To right"
        flight.FlightType = "Straight landing"
        self.assertEqual(str(flight.EntryDirection), "Straight")
        self.assertEqual(str(flight.ExitDirection), "Straight")
        for entry, exit_direction in (("From left", "To right"), ("Straight", "Straight")):
            flight.EntryDirection = entry
            flight.ExitDirection = exit_direction
            self.assertEqual(stair.NumberOfTreads, 1)
            self.assertEqual(stair.RiserHeight.Value, 0.0)
            self.assertEqual(len(stair.StepsGroup.Group), 1)
            tread = stair.StepsGroup.Group[0]
            self.assertEqual(tread.StairDesignerRole, "Tread")
            self.assertAlmostEqual(tread.Shape.BoundBox.ZMax, 0.0)
            tread.Shape.check(True)
        flight.FlightType = "Straight"
        self.assertEqual(stair.NumberOfTreads, 14)
        self.assertAlmostEqual(stair.RiserHeight.Value, 2800.0 / 15)

    def test_side_entry_save_restore_preserves_stringer_overrides(self):
        stair = make_stair()
        flight = get_flights(stair)[0]
        flight.EntryDirection = "From left"
        flight.ExitDirection = "To right"
        flight.LeftStringerType = "Housed stringer"
        part = stair.StringersGroup.Group[0]
        part.OverrideThickness = True
        part.Thickness = 65.0
        stair.Proxy.rebuild(stair)
        expected_volume = part.Shape.Volume
        with tempfile.TemporaryDirectory() as directory:
            filename = os.path.join(directory, "stair.FCStd")
            self.document.recompute()
            self.document.saveAs(filename)
            FreeCAD.closeDocument(self.document.Name)
            self.document = FreeCAD.openDocument(filename)
            stair = self.document.getObject("Stair")
            stair.Proxy.rebuild(stair)
            part = stair.StringersGroup.Group[0]
            self.assertTrue(part.OverrideThickness)
            self.assertAlmostEqual(part.Thickness.Value, 65.0)
            self.assertAlmostEqual(part.Shape.Volume, expected_volume, delta=1e-3)
            part.Shape.check(True)
            FreeCAD.closeDocument(self.document.Name)
            self.document = FreeCAD.newDocument(self.doc_name)

    def test_ifc_export_includes_stair_and_separate_railing(self):
        try:
            import ifcopenshell
            import ifcopenshell.geom
            import ifcopenshell.validate
        except ImportError:
            self.skipTest("IfcOpenShell is not installed")
        from importers import exportIFC

        stair = make_stair(floor_height=600.0, flight_length=1000.0, steps=4)
        flight = get_flights(stair)[0]
        flight.LeftStringerType = "Housed stringer"
        flight.LeftHandrailEnabled = True
        container = self.document.addObject("App::Part", "Container")
        container.addObject(stair)
        container.Placement.Base = FreeCAD.Vector(10000, 20000, 3000)
        stair.Placement.Base = FreeCAD.Vector(1000, 2000, 300)
        self.document.recompute()
        preferences = exportIFC.getPreferences()
        preferences.update(STORE_UID=False, SERIALIZE=True, IFC_UNIT="metre", SCALE_FACTOR=0.001)
        with tempfile.TemporaryDirectory() as directory:
            for schema in ("IFC2X3", "IFC4"):
                with self.subTest(schema=schema):
                    preferences["SCHEMA"] = schema
                    filename = os.path.join(directory, schema + ".ifc")
                    exportIFC.export([stair], filename, preferences=preferences)
                    model = ifcopenshell.open(filename)
                    self.assertEqual(len(model.by_type("IfcStair")), 1)
                    self.assertEqual(len(model.by_type("IfcRailing")), 1)
                    logger = ifcopenshell.validate.json_logger()
                    ifcopenshell.validate.validate(model, logger)
                    self.assertEqual(logger.statements, [])
                    settings = ifcopenshell.geom.settings()
                    settings.set(settings.USE_WORLD_COORDS, True)
                    for product in model.by_type("IfcElement"):
                        shape = ifcopenshell.geom.create_shape(settings, product)
                        self.assertTrue(shape.geometry.faces)
                        vertices = shape.geometry.verts
                        self.assertGreater(min(vertices[0::3]), 10.0)
                        self.assertGreater(min(vertices[1::3]), 21.0)
                        self.assertGreater(min(vertices[2::3]), 3.0)

    def test_component_group_icons_are_available(self):
        import_module("Arch_rc")
        from PySide import QtCore

        paths = {
            "handrails": ":/icons/Arch_Handrail_Tree.svg",
            "stringers": ":/icons/Arch_Stringer_Tree.svg",
        }
        for path in (
            ":/icons/Arch_Handrail.svg",
            ":/icons/Arch_Stringer.svg",
            *paths.values(),
            ":/ui/TaskStairDesigner.ui",
            ":/ui/TaskStairDesignerHandrails.ui",
            ":/ui/TaskStairDesignerStair.ui",
            ":/ui/TaskStairDesignerSteps.ui",
            ":/ui/TaskStairDesignerStringers.ui",
        ):
            self.assertTrue(QtCore.QResource(path).isValid(), path)

        provider = ViewProviderComponentGroup.__new__(ViewProviderComponentGroup)
        provider.Object = type("ComponentGroup", (), {})()
        for section, path in paths.items():
            provider.Object.PanelSection = section
            self.assertEqual(provider.getIcon(), path)

    def test_straight_metrics(self):
        metrics = straight_stair_metrics(2800.0, 3500.0, 15)
        self.assertEqual(metrics.tread_count, 14)
        self.assertAlmostEqual(metrics.riser_height, 2800.0 / 15.0)
        self.assertAlmostEqual(metrics.tread_width, 250.0)
        self.assertAlmostEqual(metrics.blondel_value, 623.3333333333334)
        self.assertTrue(metrics.blondel_compliant)

    def test_deprecated_model_property_is_removed(self):
        stair = self.document.addObject("App::FeaturePython", "Stair")
        StairProxy(stair)
        stair.addProperty("App::PropertyString", "Model", "Stair")
        stair.Model = "Quarter turn"

        stair.Proxy.set_properties(stair)

        self.assertNotIn("Model", stair.PropertiesList)

    def test_generated_object_links_use_child_scope(self):
        stair = make_stair()

        for name in (
            "PlanSketch",
            "FlightsGroup",
            "StepsGroup",
            "StringersGroup",
            "HandrailsGroup",
            "ConcreteGeometry",
        ):
            self.assertEqual(
                stair.getTypeIdOfProperty(name),
                "App::PropertyLinkChild",
            )

    def test_stair_position_can_snap_origin_or_vertex(self):
        container = self.document.addObject("App::Part", "Container")
        container.Placement = FreeCAD.Placement(
            FreeCAD.Vector(500.0, -200.0, 100.0),
            FreeCAD.Rotation(FreeCAD.Vector(0.0, 0.0, 1.0), 30.0),
        )
        stair = make_stair()
        container.addObject(stair)
        stair.Placement = FreeCAD.Placement(
            FreeCAD.Vector(100.0, 200.0, 300.0),
            FreeCAD.Rotation(FreeCAD.Vector(0.0, 0.0, 1.0), 15.0),
        )

        target = FreeCAD.Vector(1200.0, 800.0, 450.0)
        stair.Placement = _translated_stair_placement(stair, target)
        self.assertLess(
            stair.getGlobalPlacement().Base.sub(target).Length,
            1e-7,
        )

        local_vertex = FreeCAD.Vector(250.0, 100.0, 180.0)
        source = stair.getGlobalPlacement().multVec(local_vertex)
        target = FreeCAD.Vector(-400.0, 600.0, 950.0)
        stair.Placement = _translated_stair_placement(
            stair,
            target,
            source,
        )
        moved_vertex = stair.getGlobalPlacement().multVec(local_vertex)
        self.assertLess(moved_vertex.sub(target).Length, 1e-7)

        external = self.document.addObject("Part::Feature", "External")
        external.Shape = Part.makeBox(100.0, 100.0, 100.0)
        external.Placement.Base = FreeCAD.Vector(1000.0, 0.0, 0.0)
        tread = next(
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        )

        def selection(obj):
            parents = obj.Parents
            if len(parents) == 1:
                root = parents[0][0]
                subname = parents[0][1] + "Vertex1"
            else:
                root = obj
                subname = "Vertex1"
            return type(
                "Selection",
                (),
                {
                    "Object": root,
                    "SubElementNames": (subname,),
                    # SelectionEx returns subshapes transformed into their
                    # selection context, including the object's placement.
                    "SubObjects": (root.getSubObject(subname),),
                },
            )()

        vertices = _selected_vertices(
            (selection(external), selection(tread)),
            stair,
        )
        self.assertEqual([owned for owned, _point in vertices], [False, True])
        external_vertex = external.Shape.Vertexes[0].Point
        self.assertAlmostEqual(external_vertex.x, 1000.0)
        self.assertLess(
            vertices[0][1].sub(external_vertex).Length,
            1e-7,
        )
        self.assertLess(
            vertices[1][1].sub(selection(tread).SubObjects[0].Point).Length,
            1e-7,
        )
        stair.Placement = _translated_stair_placement(
            stair,
            vertices[0][1],
        )
        self.assertLess(
            stair.getGlobalPlacement().Base.sub(external_vertex).Length,
            1e-7,
        )

    def test_treads_are_distributed_over_flights(self):
        self.assertEqual(distribute_treads([2000.0, 1000.0], 9), [6, 3])
        self.assertEqual(distribute_treads([1000.0, 1000.0, 1000.0], 8), [3, 3, 2])

    def test_flight_side_lengths_are_linked(self):
        stair = make_stair(flight_length=1600.0, width=800.0)
        flights = resize_flights(stair, 2, length=1600.0, width=800.0, rotations=["Right"])

        self.assertAlmostEqual(flights[0].LeftLength.Value, 2000.0)
        self.assertAlmostEqual(flights[0].RightLength.Value, 1200.0)
        self.assertAlmostEqual(flights[1].LeftLength.Value, 2000.0)
        self.assertAlmostEqual(flights[1].RightLength.Value, 1200.0)
        flights[0].RightLength = 1300.0
        self.assertAlmostEqual(flights[0].LeftLength.Value, 2100.0)
        flights[0].LeftLength = 2300.0
        self.assertAlmostEqual(flights[0].RightLength.Value, 1500.0)

        flights[1].Width = 1000.0
        self.assertAlmostEqual(flights[0].LeftLength.Value, 2400.0)
        self.assertAlmostEqual(flights[0].RightLength.Value, 1400.0)
        self.assertAlmostEqual(flights[1].LeftLength.Value, 2000.0)
        self.assertAlmostEqual(flights[1].RightLength.Value, 1200.0)
        flights[1].Rotation = "Left"
        self.assertAlmostEqual(flights[0].LeftLength.Value, 1400.0)
        self.assertAlmostEqual(flights[0].RightLength.Value, 2400.0)
        self.assertAlmostEqual(flights[1].LeftLength.Value, 1200.0)
        self.assertAlmostEqual(flights[1].RightLength.Value, 2000.0)

        flights[1].Angle = 45.0
        expected_difference = (1000.0 - 800.0 * math.cos(math.radians(45.0))) / math.sin(
            math.radians(45.0)
        )
        self.assertAlmostEqual(
            flights[0].RightLength.Value - flights[0].LeftLength.Value,
            expected_difference,
        )
        expected_incoming_difference = (800.0 - 1000.0 * math.cos(math.radians(45.0))) / math.sin(
            math.radians(45.0)
        )
        self.assertAlmostEqual(
            flights[1].RightLength.Value - flights[1].LeftLength.Value,
            expected_incoming_difference,
        )
        flights[1].Angle = 90.0

        flights[1].LeftLength = 1400.0
        self.assertAlmostEqual(flights[1].RightLength.Value, 2200.0)

        flights[1].FlightType = "Circular"
        self.assertAlmostEqual(flights[1].Angle.Value, 90.0)
        self.assertIn("Hidden", flights[1].getEditorMode("LeftLength"))
        self.assertIn("Hidden", flights[1].getEditorMode("RightLength"))
        self.assertNotIn("Hidden", flights[1].getEditorMode("InnerRadius"))
        self.assertNotIn("Hidden", flights[1].getEditorMode("OuterRadius"))
        self.assertAlmostEqual(flights[0].LeftLength.Value, 1900.0)
        self.assertAlmostEqual(flights[0].RightLength.Value, 1900.0)
        self.assertAlmostEqual(flights[1].InnerRadius.Value, 500.0)
        self.assertAlmostEqual(flights[1].OuterRadius.Value, 1500.0)
        flights[1].InnerRadius = 650.0
        self.assertAlmostEqual(flights[1].OuterRadius.Value, 1650.0)
        flights[1].OuterRadius = 1800.0
        self.assertAlmostEqual(flights[1].InnerRadius.Value, 800.0)
        flights[1].Width = 1100.0
        self.assertAlmostEqual(flights[1].OuterRadius.Value, 1900.0)
        flights[1].Angle = 60.0
        self.assertAlmostEqual(flights[1].Angle.Value, 60.0)
        inner_radius = flights[1].InnerRadius.Value
        outer_radius = flights[1].OuterRadius.Value
        flights[1].Rotation = "Right"
        self.assertAlmostEqual(
            flights[1].InnerRadius.Value,
            inner_radius,
        )
        self.assertAlmostEqual(
            flights[1].OuterRadius.Value,
            outer_radius,
        )
        self.assertEqual(stair.GeometryStatus, "Tangential circular stair")
        circular_treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        ]
        self.assertTrue(all(tread.Shape.isValid() for tread in circular_treads))
        self.assertTrue(all(len(tread.Shape.Solids) == 1 for tread in circular_treads))
        self.assertTrue(
            any(isinstance(item, Part.ArcOfCircle) for item in stair.PlanSketch.Geometry)
        )

    def test_winding_parameters_belong_to_junction_flights(self):
        stair = make_stair(flight_length=1800.0, width=800.0)
        flights = resize_flights(stair, 3, length=1800.0, width=800.0)

        self.assertAlmostEqual(stair.WalkingLineOffset.Value, 500.0)
        self.assertNotIn("WindingCoefficient", stair.PropertiesList)
        self.assertEqual(stair.StepsGroup.PanelSection, "steps")
        for flight in flights:
            self.assertIn("WindingLocal", flight.PropertiesList)
            self.assertIn("WindingDistant", flight.PropertiesList)
            self.assertEqual(flight.WindingLocal, 50)
            self.assertEqual(flight.WindingDistant, 50)
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.assertIn("Hidden", flights[0].getEditorMode("WindingLocal"))
        self.assertNotIn("Hidden", flights[1].getEditorMode("WindingLocal"))

        flights[1].WindingLocal = 80
        flights[2].WindingDistant = 20
        self.document.recompute()
        self.assertTrue(
            all(
                not child.Shape.isNull()
                for child in stair.StepsGroup.Group
                if hasattr(child, "Shape")
            )
        )

    def test_endpoint_angles_and_side_directions(self):
        width = 1000.0
        left_length = 2500.0
        difference = width * math.tan(math.radians(30.0))
        center_length = left_length + difference / 2.0
        specs = [(center_length, width, 0.0)]

        angled_footprint = make_stair_footprint(specs, 30.0, 0.0)
        angled_sections, _going = balanced_winder_sections(
            specs,
            12,
            1.0,
            None,
            30.0,
            0.0,
        )
        angled_sections = fit_balanced_sections_to_footprint(angled_sections, angled_footprint)
        angled_faces = balanced_tread_faces(angled_sections, angled_footprint)
        self.assertTrue(balanced_partition_is_valid(angled_faces, angled_footprint, 12))
        self.assertAlmostEqual(
            angled_sections[0].left[0] - angled_sections[0].right[0],
            difference,
        )
        self.assertTrue(all(abs(section.tangent[1]) < 1e-7 for section in angled_sections[6:]))

        rectangular_footprint = make_stair_footprint([(3500.0, width, 0.0)])
        side_sections, _going = balanced_winder_sections(
            [(3500.0, width, 0.0)],
            14,
            1.0,
            None,
            0.0,
            0.0,
            "From right",
            "To left",
        )
        side_sections = fit_balanced_sections_to_footprint(side_sections, rectangular_footprint)
        side_faces = balanced_tread_faces(side_sections, rectangular_footprint)
        self.assertTrue(balanced_partition_is_valid(side_faces, rectangular_footprint, 14))
        self.assertAlmostEqual(side_sections[0].tangent[0], 0.0)
        self.assertAlmostEqual(side_sections[0].tangent[1], 1.0)
        self.assertAlmostEqual(side_sections[-1].tangent[0], 0.0)
        self.assertAlmostEqual(side_sections[-1].tangent[1], 1.0)
        self.assertTrue(
            any(
                section.locked_to_flight and abs(section.tangent[0] - 1.0) < 1e-7
                for section in side_sections
            )
        )
        first_tread = make_balanced_tread_shape(
            side_sections[0],
            side_sections[1],
            rectangular_footprint,
            40.0,
            40.0,
            30.0,
            28.0,
            side_faces[0],
        )
        first_riser = make_balanced_riser_shape(
            side_sections[0],
            0.0,
            100.0,
            18.0,
            rectangular_footprint,
        )
        self.assertTrue(first_tread.isValid())
        self.assertEqual(len(first_tread.Solids), 1)
        self.assertAlmostEqual(first_tread.BoundBox.YMin, -30.0)
        self.assertTrue(first_riser.isValid())
        self.assertEqual(len(first_riser.Solids), 1)
        self.assertAlmostEqual(first_riser.BoundBox.XLength, width)
        self.assertAlmostEqual(first_riser.BoundBox.YLength, 18.0)

        stair = make_stair(
            flight_length=left_length,
            width=width,
            steps=13,
        )
        flight = get_flights(stair)[0]
        flight.StartAngle = 30.0
        self.assertAlmostEqual(flight.LeftLength.Value, left_length)
        self.assertAlmostEqual(flight.RightLength.Value, left_length + difference)
        flight.EndAngle = 10.0
        self.assertAlmostEqual(flight.LeftLength.Value, left_length)
        self.assertAlmostEqual(
            flight.RightLength.Value,
            left_length + difference - width * math.tan(math.radians(10.0)),
        )
        flight.EndAngle = 0.0
        flight.StartAngle = 0.0
        self.assertAlmostEqual(flight.LeftLength.Value, left_length)
        self.assertAlmostEqual(flight.RightLength.Value, left_length)
        flight.StartAngle = 30.0
        flight.EndAngle = 10.0
        flight.EntryDirection = "From right"
        flight.ExitDirection = "To left"
        self.document.recompute()
        self.assertEqual(stair.GeometryStatus, "Balanced entry/exit stair")
        generated_parts = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") in ("Tread", "Riser")
        ]
        self.assertTrue(generated_parts)
        self.assertTrue(
            all(part.Shape.isValid() and len(part.Shape.Solids) == 1 for part in generated_parts)
        )

    def test_tangential_circular_flight_geometry(self):
        radius = 1000.0
        inner_radius = 600.0
        arc_length = math.pi * radius / 2.0
        specs = [
            ("Straight", 1200.0, 800.0, 0.0, "Left"),
            ("Circular", inner_radius, 800.0, 90.0, "Left"),
            ("Straight", 1000.0, 800.0, 90.0, "Left"),
        ]
        sections, going = tangent_flight_sections(specs, 12)
        footprint = make_tangent_stair_footprint(specs)
        sections = fit_tangent_sections_to_footprint(sections, footprint)

        self.assertEqual(len(sections), 13)
        self.assertAlmostEqual(sections[0].tangent[0], 1.0, places=3)
        self.assertAlmostEqual(sections[0].tangent[1], 0.0, places=3)
        self.assertAlmostEqual(sections[-1].tangent[0], 0.0, places=3)
        self.assertAlmostEqual(sections[-1].tangent[1], 1.0, places=3)
        self.assertTrue(
            any(
                abs(section.tangent[0]) > 0.1 and abs(section.tangent[1]) > 0.1
                for section in sections
            )
        )
        self.assertAlmostEqual(going, (2200.0 + arc_length) / 12.0, delta=0.1)
        self.assertAlmostEqual(footprint.BoundBox.XMin, 0.0, delta=0.1)
        self.assertAlmostEqual(footprint.BoundBox.XMax, 2600.0, delta=0.1)
        self.assertAlmostEqual(footprint.BoundBox.YMin, 0.0, delta=0.1)
        self.assertAlmostEqual(footprint.BoundBox.YMax, 2400.0, delta=0.1)
        self.assertTrue(
            any(
                abs(edge.curvatureAt((edge.FirstParameter + edge.LastParameter) / 2.0)) > 1e-9
                for edge in footprint.Edges
            )
        )
        faces = balanced_tread_faces(sections, footprint)
        self.assertTrue(balanced_partition_is_valid(faces, footprint, 12))
        self.assertTrue(all(face.isValid() for face in faces))

        concrete = make_balanced_concrete_shape(sections, footprint, 180.0, 150.0)
        self.assertTrue(concrete.isValid())
        self.assertEqual(len(concrete.Solids), 1)
        cylindrical_sides = [
            face for face in concrete.Faces if isinstance(face.Surface, Part.Cylinder)
        ]
        helical_bottoms = [
            face for face in concrete.Faces if isinstance(face.Surface, Part.BSplineSurface)
        ]
        self.assertEqual(len(cylindrical_sides), 2)
        self.assertEqual(len(helical_bottoms), 1)
        self.assertEqual(
            sorted(round(face.Surface.Radius, 3) for face in cylindrical_sides),
            [inner_radius, inner_radius + 800.0],
        )
        inset_concrete = make_balanced_concrete_shape(
            sections,
            footprint,
            180.0,
            150.0,
            structure_width_offset=30.0,
        )
        inset_cylindrical_sides = [
            face for face in inset_concrete.Faces if isinstance(face.Surface, Part.Cylinder)
        ]
        self.assertEqual(
            sorted(round(face.Surface.Radius, 3) for face in inset_cylindrical_sides),
            [inner_radius + 30.0, inner_radius + 800.0 - 30.0],
        )

        unequal_specs = [
            ("Straight", 1900.0, 800.0, 0.0, "Left"),
            ("Circular", 400.0, 1000.0, 90.0, "Right"),
        ]
        unequal_sections, _going = tangent_flight_sections(unequal_specs, 14)
        unequal_footprint = make_tangent_stair_footprint(unequal_specs)
        unequal_sections = fit_tangent_sections_to_footprint(unequal_sections, unequal_footprint)
        unequal_faces = balanced_tread_faces(unequal_sections, unequal_footprint)
        self.assertAlmostEqual(
            sum(face.Area for face in unequal_faces),
            unequal_footprint.Area,
            delta=1.0,
        )
        self.assertTrue(
            balanced_partition_is_valid(unequal_faces, unequal_footprint, 14),
            (
                unequal_footprint.Area,
                sum(face.Area for face in unequal_faces),
                [face.isValid() for face in unequal_faces],
            ),
        )

        circular_pair = [
            ("Circular", inner_radius, 800.0, 90.0, "Left"),
            ("Circular", inner_radius, 800.0, 90.0, "Right"),
        ]
        pair_sections, _going = tangent_flight_sections(circular_pair, 10, turn_types=["Landing"])
        pair_footprint = make_tangent_stair_footprint(circular_pair, ["Landing"])
        pair_sections = fit_tangent_sections_to_footprint(pair_sections, pair_footprint)
        pair_faces = balanced_tread_faces(pair_sections, pair_footprint)
        self.assertFalse(any(section.landing_to_next for section in pair_sections))
        self.assertTrue(balanced_partition_is_valid(pair_faces, pair_footprint, 10))

        angled_specs = [
            ("Straight", 1000.0, 800.0, 0.0, "Left"),
            ("Circular", inner_radius, 800.0, 60.0, "Left"),
            ("Straight", 500.0, 800.0, 90.0, "Left"),
        ]
        angled_sections, angled_going = tangent_flight_sections(angled_specs, 10)
        angled_footprint = make_tangent_stair_footprint(angled_specs)
        angled_sections = fit_tangent_sections_to_footprint(angled_sections, angled_footprint)
        angled_faces = balanced_tread_faces(angled_sections, angled_footprint)
        self.assertAlmostEqual(
            angled_sections[-1].tangent[0], math.cos(math.radians(60.0)), places=3
        )
        self.assertAlmostEqual(
            angled_sections[-1].tangent[1], math.sin(math.radians(60.0)), places=3
        )
        self.assertAlmostEqual(
            angled_going,
            (1500.0 + radius * math.radians(60.0)) / 10.0,
            delta=0.1,
        )
        self.assertTrue(balanced_partition_is_valid(angled_faces, angled_footprint, 10))

    def test_explicit_straight_and_circular_landings_are_level(self):
        def sloped_faces(shape):
            result = []
            for face in shape.Faces:
                parameters = face.ParameterRange
                normal = face.normalAt(
                    (parameters[0] + parameters[1]) / 2.0,
                    (parameters[2] + parameters[3]) / 2.0,
                )
                if 0.01 < abs(normal.z) < 0.999999:
                    result.append(face)
            return result

        straight_landing_specs = [
            (
                "Straight",
                1600.0,
                800.0,
                0.0,
                "Left",
                "Straight",
                "Straight",
            ),
            (
                "Straight landing",
                2400.0,
                800.0,
                90.0,
                "Left",
                "From left",
                "To left",
            ),
            (
                "Straight",
                1600.0,
                800.0,
                0.0,
                "Left",
                "Straight",
                "Straight",
            ),
        ]

        def build(specs, landing_replaces_tread=True):
            sections, _going = tangent_flight_sections(
                specs,
                8,
                1.0,
                ["Landing", "Landing"],
                landing_replaces_tread=landing_replaces_tread,
            )
            footprint = make_tangent_stair_footprint(specs, ["Landing", "Landing"])
            sections = fit_tangent_sections_to_footprint(sections, footprint)
            faces = tangent_tread_faces(sections, specs)
            concrete = make_balanced_concrete_shape(sections, footprint, 180.0, 150.0, faces)
            return sections, faces, concrete

        sections, faces, concrete = build(straight_landing_specs)
        level_indices = [
            index for index, section in enumerate(sections[:-1]) if section.level_to_next
        ]
        self.assertEqual(level_indices, [4])
        self.assertEqual(len(sections), 9)
        self.assertEqual(sections[4].riser_index, 5)
        self.assertEqual(sections[5].riser_index, 6)
        self.assertAlmostEqual(faces[4].Area, 2400.0 * 800.0, delta=1.0)
        self.assertLess(faces[3].common(faces[4]).Area, 0.01)
        self.assertLess(faces[4].common(faces[5]).Area, 0.01)
        self.assertTrue(all(face.isValid() for face in faces))
        concrete_sections, concrete_faces, concrete = build(straight_landing_specs, False)
        concrete_index = next(
            index for index, section in enumerate(concrete_sections[:-1]) if section.level_to_next
        )
        self.assertEqual(len(concrete_sections), 10)
        self.assertEqual(
            concrete_sections[concrete_index - 1].riser_index,
            concrete_sections[concrete_index].riser_index,
        )
        self.assertEqual(
            concrete_sections[concrete_index + 1].riser_index,
            concrete_sections[concrete_index].riser_index + 1,
        )
        self.assertTrue(concrete.isValid())
        self.assertEqual(len(concrete.Solids), 1)
        self.assertEqual(len(sloped_faces(concrete)), 2)
        landing_bottom = concrete_sections[concrete_index].riser_index * 180.0 - 150.0
        landing_bottoms = [
            face
            for face in concrete.Faces
            if face.BoundBox.ZLength < 1e-7 and abs(face.BoundBox.ZMin - landing_bottom) < 0.01
        ]
        self.assertTrue(landing_bottoms)
        self.assertGreater(
            max(face.Area for face in landing_bottoms),
            concrete_faces[concrete_index].Area * 0.99,
        )

        circular_landing_specs = list(straight_landing_specs)
        circular_landing_specs[1] = (
            "Circular landing",
            500.0,
            800.0,
            90.0,
            "Left",
            "Straight",
            "Straight",
        )
        circular_sections, circular_faces, _circular_wood_concrete = build(circular_landing_specs)
        circular_index = next(
            index for index, section in enumerate(circular_sections[:-1]) if section.level_to_next
        )
        self.assertEqual(circular_sections[circular_index].flight_index, 1)
        self.assertTrue(
            any(
                abs(edge.curvatureAt((edge.FirstParameter + edge.LastParameter) / 2.0)) > 1e-9
                for edge in circular_faces[circular_index].Edges
            )
        )
        (
            circular_concrete_sections,
            circular_concrete_faces,
            circular_concrete,
        ) = build(circular_landing_specs, False)
        circular_concrete_index = next(
            index
            for index, section in enumerate(circular_concrete_sections[:-1])
            if section.level_to_next
        )
        self.assertEqual(
            circular_concrete_sections[circular_concrete_index - 1].riser_index,
            circular_concrete_sections[circular_concrete_index].riser_index,
        )
        self.assertEqual(
            circular_concrete_sections[circular_concrete_index + 1].riser_index,
            circular_concrete_sections[circular_concrete_index].riser_index + 1,
        )
        self.assertTrue(circular_concrete.isValid())
        self.assertEqual(len(circular_concrete.Solids), 1)
        self.assertEqual(len(sloped_faces(circular_concrete)), 2)
        circular_bottom = (
            circular_concrete_sections[circular_concrete_index].riser_index * 180.0 - 150.0
        )
        circular_bottoms = [
            face
            for face in circular_concrete.Faces
            if face.BoundBox.ZLength < 1e-7 and abs(face.BoundBox.ZMin - circular_bottom) < 0.01
        ]
        self.assertTrue(circular_bottoms)
        self.assertGreater(
            max(face.Area for face in circular_bottoms),
            circular_concrete_faces[circular_concrete_index].Area * 0.99,
        )

        outgoing = circular_concrete_sections[circular_concrete_index + 1]
        outgoing_probe = (
            outgoing.center[0] + outgoing.tangent[0] * 0.01,
            outgoing.center[1] + outgoing.tangent[1] * 0.01,
        )
        outgoing_cut = circular_concrete.common(
            Part.makeLine(
                FreeCAD.Vector(*outgoing_probe, -1.0),
                FreeCAD.Vector(*outgoing_probe, 4000.0),
            )
        )
        self.assertGreaterEqual(
            min(vertex.Point.z for vertex in outgoing_cut.Vertexes),
            circular_bottom - 0.1,
        )

        end = circular_concrete_sections[-1]
        end_probe = (
            end.center[0] - end.tangent[0] * 0.01,
            end.center[1] - end.tangent[1] * 0.01,
        )
        end_cut = circular_concrete.common(
            Part.makeLine(
                FreeCAD.Vector(*end_probe, -1.0),
                FreeCAD.Vector(*end_probe, 4000.0),
            )
        )
        end_elevations = [vertex.Point.z for vertex in end_cut.Vertexes]
        self.assertAlmostEqual(max(end_elevations) - min(end_elevations), 150.0, delta=0.1)

    def test_balanced_winder_sections_have_equal_goings(self):
        sections, going = balanced_winder_sections(
            [(1600.0, 800.0, 0.0), (1600.0, 800.0, 90.0)],
            9,
            1.0,
        )
        self.assertEqual(len(sections), 10)
        for first, second in zip(sections, sections[1:]):
            self.assertAlmostEqual(second.station - first.station, going)
        winding_sections = [
            section
            for section in sections
            if abs(section.tangent[0]) > 0.1 and abs(section.tangent[1]) > 0.1
        ]
        self.assertGreaterEqual(len(winding_sections), 2)
        self.assertTrue(
            any(
                abs(
                    (rear.left[0] - front.left[0]) ** 2
                    + (rear.left[1] - front.left[1]) ** 2
                    - going**2
                )
                > 1.0
                for front, rear in zip(sections, sections[1:])
            )
        )

        long_sections, _long_going = balanced_winder_sections(
            [(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)],
            20,
            1.0,
        )
        long_footprint = make_stair_footprint([(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)])
        long_sections = fit_balanced_sections_to_footprint(long_sections, long_footprint)
        nosed_sections, _nosed_going = balanced_winder_sections(
            [(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)],
            20,
            1.0,
            nosing=30.0,
        )
        nosed_sections = fit_balanced_sections_to_footprint(nosed_sections, long_footprint)
        self.assertTrue(any(not section.locked_to_flight for section in long_sections))
        self.assertLess(
            sum(not section.locked_to_flight for section in long_sections),
            len(long_sections) / 2,
        )
        for section in long_sections:
            far_from_turn = (section.flight_index == 0 and section.center[0] < 2200.0) or (
                section.flight_index == 1 and section.center[1] > 1200.0
            )
            if not far_from_turn:
                continue
            self.assertTrue(section.locked_to_flight, section)
            direction = ((1.0, 0.0), (0.0, 1.0))[section.flight_index]
            self.assertAlmostEqual(
                section.tangent[0] * direction[1] - section.tangent[1] * direction[0],
                0.0,
            )
        for original, corrected in zip(long_sections, nosed_sections):
            if original.locked_to_flight and corrected.locked_to_flight:
                self.assertAlmostEqual(corrected.center[0], original.center[0])
                self.assertAlmostEqual(corrected.center[1], original.center[1])
        self.assertTrue(
            any(
                math.hypot(
                    corrected.center[0] - original.center[0],
                    corrected.center[1] - original.center[1],
                )
                > 0.01
                for original, corrected in zip(long_sections, nosed_sections)
                if not original.locked_to_flight
            )
        )

        footprint = make_stair_footprint([(1600.0, 800.0, 0.0), (1600.0, 800.0, 90.0)])
        sections = fit_balanced_sections_to_footprint(sections, footprint)
        for section in sections:
            self.assertAlmostEqual(
                (section.left[0] - section.center[0]) * section.tangent[0]
                + (section.left[1] - section.center[1]) * section.tangent[1],
                0.0,
            )
            self.assertAlmostEqual(
                (section.right[0] - section.center[0]) * section.tangent[0]
                + (section.right[1] - section.center[1]) * section.tangent[1],
                0.0,
            )
        tread_faces = balanced_tread_faces(sections, footprint)
        self.assertAlmostEqual(footprint.Area, 2560000.0)
        tread_volume = sum(
            make_balanced_tread_shape(
                front,
                rear,
                footprint,
                1.0,
                1.0,
                0.0,
                base_face=base_face,
            ).Volume
            for front, rear, base_face in zip(sections, sections[1:], tread_faces)
        )
        self.assertAlmostEqual(tread_volume, footprint.Area, delta=1.0)
        self.assertAlmostEqual(sum(face.Area for face in tread_faces), footprint.Area, delta=1.0)
        for front, rear, base_face in zip(sections[:-2], sections[1:-1], tread_faces[:-1]):
            tread = make_balanced_tread_shape(
                front,
                rear,
                footprint,
                40.0,
                40.0,
                30.0,
                28.0,
                base_face=base_face,
            )
            required_rear_support = make_balanced_riser_shape(
                rear,
                0.0,
                1.0,
                28.0,
                footprint,
            )
            actual_riser = make_balanced_riser_shape(
                rear,
                0.0,
                100.0,
                18.0,
                footprint,
            )
            self.assertTrue(tread.isValid())
            self.assertEqual(len(tread.Solids), 1)
            self.assertTrue(actual_riser.isValid())
            self.assertAlmostEqual(
                required_rear_support.cut(tread).Volume,
                0.0,
                delta=0.1,
            )

        half_turn_specs = [
            (1200.0, 800.0, 0.0),
            (1200.0, 800.0, 90.0),
            (1200.0, 800.0, 180.0),
        ]
        half_turn_sections, _going = balanced_winder_sections(
            half_turn_specs,
            9,
            1.0,
        )
        half_turn_footprint = make_stair_footprint(half_turn_specs)
        half_turn_sections = fit_balanced_sections_to_footprint(
            half_turn_sections, half_turn_footprint
        )
        half_turn_faces = balanced_tread_faces(half_turn_sections, half_turn_footprint)
        half_turn_treads = [
            make_balanced_tread_shape(
                front,
                rear,
                half_turn_footprint,
                1.0,
                1.0,
                0.0,
                base_face=base_face,
            )
            for front, rear, base_face in zip(
                half_turn_sections, half_turn_sections[1:], half_turn_faces
            )
        ]
        self.assertAlmostEqual(half_turn_footprint.Area, 2880000.0)
        self.assertAlmostEqual(
            sum(tread.Volume for tread in half_turn_treads),
            half_turn_footprint.Area,
            delta=1.0,
        )
        self.assertTrue(all(tread.isValid() for tread in half_turn_treads))
        self.assertTrue(all(len(tread.Solids) == 1 for tread in half_turn_treads))

        short_turn_specs = [(700.0, 800.0, 0.0), (700.0, 800.0, 90.0)]
        short_turn_sections, _going = balanced_winder_sections(short_turn_specs, 7, 1.0)
        short_turn_footprint = make_stair_footprint(short_turn_specs)
        short_turn_sections = fit_balanced_sections_to_footprint(
            short_turn_sections, short_turn_footprint
        )
        short_turn_faces = balanced_tread_faces(short_turn_sections, short_turn_footprint)
        short_turn_union = short_turn_faces[0]
        for face in short_turn_faces[1:]:
            short_turn_union = short_turn_union.fuse(face)
        self.assertAlmostEqual(
            sum(face.Area for face in short_turn_faces),
            short_turn_footprint.Area,
            delta=1.0,
        )
        self.assertAlmostEqual(short_turn_union.Area, short_turn_footprint.Area, delta=1.0)
        self.assertTrue(all(face.isValid() for face in short_turn_faces))

        angled_specs = [(1600.0, 800.0, 0.0), (1600.0, 800.0, 45.0)]
        angled_sections, _going = balanced_winder_sections(angled_specs, 9, 1.0)
        angled_footprint = make_stair_footprint(angled_specs)
        angled_sections = fit_balanced_sections_to_footprint(angled_sections, angled_footprint)
        angled_faces = balanced_tread_faces(angled_sections, angled_footprint)
        angled_vertices = [
            (vertex.Point.x, vertex.Point.y)
            for vertex in angled_footprint.OuterWire.OrderedVertexes
        ]
        miter_offset = 400.0 * math.tan(math.radians(22.5))
        self.assertEqual(len(angled_vertices), 6)
        self.assertTrue(
            any(
                abs(x - (1600.0 - miter_offset)) < 0.01 and abs(y - 800.0) < 0.01
                for x, y in angled_vertices
            )
        )
        self.assertTrue(
            any(
                abs(x - (1600.0 + miter_offset)) < 0.01 and abs(y) < 0.01
                for x, y in angled_vertices
            )
        )
        self.assertAlmostEqual(angled_footprint.Area, 2560000.0, delta=1.0)
        self.assertTrue(balanced_partition_is_valid(angled_faces, angled_footprint, 9))

        angled_landing_sections, _going = balanced_winder_sections(
            angled_specs, 9, 1.0, ["Landing"]
        )
        angled_landing_sections = fit_balanced_sections_to_footprint(
            angled_landing_sections, angled_footprint
        )
        angled_landing_faces = balanced_tread_faces(angled_landing_sections, angled_footprint)
        self.assertEqual(
            sum(section.landing_to_next for section in angled_landing_sections),
            1,
        )
        self.assertTrue(balanced_partition_is_valid(angled_landing_faces, angled_footprint, 9))

        variable_turn_specs = [
            (2200.0, 600.0, 0.0),
            (700.0, 1100.0, 90.0),
            (1500.0, 500.0, 180.0),
        ]
        variable_sections, _going = balanced_winder_sections(variable_turn_specs, 7, 1.0)
        variable_footprint = make_stair_footprint(variable_turn_specs)
        variable_sections = fit_balanced_sections_to_footprint(
            variable_sections, variable_footprint
        )
        variable_faces = balanced_tread_faces(variable_sections, variable_footprint)
        self.assertAlmostEqual(
            sum(face.Area for face in variable_faces),
            variable_footprint.Area,
            delta=1.0,
        )
        self.assertTrue(all(face.isValid() for face in variable_faces))

        overlapping_specs = [
            (700.0, 800.0, 0.0),
            (700.0, 800.0, 90.0),
            (700.0, 800.0, 180.0),
        ]
        overlapping_sections, _going = balanced_winder_sections(overlapping_specs, 12, 1.0)
        overlapping_footprint = make_stair_footprint(overlapping_specs)
        overlapping_sections = fit_balanced_sections_to_footprint(
            overlapping_sections, overlapping_footprint
        )
        overlapping_faces = balanced_tread_faces(overlapping_sections, overlapping_footprint)
        self.assertFalse(balanced_partition_is_valid(overlapping_faces, overlapping_footprint, 12))

        wider_balance, _going = balanced_winder_sections(
            [(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)],
            9,
            1.5,
        )
        reference_balance, _going = balanced_winder_sections(
            [(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)],
            9,
            1.0,
        )
        self.assertTrue(
            any(
                abs(first.center[0] - second.center[0]) > 1.0
                or abs(first.center[1] - second.center[1]) > 1.0
                for first, second in zip(reference_balance, wider_balance)
            )
        )
        local_balance, _going = balanced_winder_sections(
            [(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)],
            9,
            winding_parameters=[(100, 50)],
        )
        distant_balance, _going = balanced_winder_sections(
            [(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)],
            9,
            winding_parameters=[(50, 100)],
        )
        for adjusted in (local_balance, distant_balance):
            self.assertTrue(
                any(
                    math.hypot(
                        first.center[0] - second.center[0],
                        first.center[1] - second.center[1],
                    )
                    > 1.0
                    for first, second in zip(reference_balance, adjusted)
                )
            )

    def test_balanced_nosings_do_not_cross_the_stair_sides(self):
        specs = [(1600.0, 800.0, 0.0), (1600.0, 800.0, 90.0)]
        footprint = make_stair_footprint(specs)
        sections, _going = balanced_winder_sections(specs, 9, 1.0)
        sections = fit_balanced_sections_to_footprint(sections, footprint)
        faces = balanced_tread_faces(sections, footprint)

        envelope_face = footprint.copy()
        envelope_face.translate(FreeCAD.Vector(0.0, 0.0, -1.0))
        envelope = envelope_face.extrude(FreeCAD.Vector(0.0, 0.0, 42.0))
        checked = 0
        for index, (front, rear, face) in enumerate(zip(sections, sections[1:], faces)):
            if index == 0 or front.locked_to_flight:
                continue
            tread = make_balanced_tread_shape(
                front,
                rear,
                footprint,
                40.0,
                40.0,
                30.0,
                base_face=face,
            )
            self.assertLess(tread.cut(envelope).Volume, 1e-3)
            for band_face in _section_band_faces(front, footprint, -30.0):
                expected_band = band_face.extrude(FreeCAD.Vector(0.0, 0.0, 40.0))
                self.assertLess(expected_band.cut(tread).Volume, 1e-3)
            checked += 1
        self.assertGreater(checked, 0)

        corner_index = 5
        corner = (
            footprint.BoundBox.XMax,
            footprint.BoundBox.YMin,
        )
        corner_front = sections[corner_index]
        corner_nosing = -(
            (corner[0] - corner_front.center[0]) * corner_front.tangent[0]
            + (corner[1] - corner_front.center[1]) * corner_front.tangent[1]
        )
        self.assertGreater(corner_nosing, 0.0)
        corner_tread = make_balanced_tread_shape(
            corner_front,
            sections[corner_index + 1],
            footprint,
            40.0,
            40.0,
            corner_nosing,
            base_face=faces[corner_index],
        )
        for band_face in _section_band_faces(corner_front, footprint, -corner_nosing):
            expected_band = band_face.extrude(FreeCAD.Vector(0.0, 0.0, 40.0))
            self.assertLess(expected_band.cut(corner_tread).Volume, 1e-3)

    def test_landing_is_one_rectangular_corner_tread(self):
        specs = [(1600.0, 800.0, 0.0), (1600.0, 800.0, 90.0)]
        sections, _going = balanced_winder_sections(
            specs,
            9,
            1.0,
            ["Landing"],
        )
        self.assertEqual(len(sections), 10)
        landing_indices = [
            index for index, section in enumerate(sections[:-1]) if section.landing_to_next
        ]
        self.assertEqual(len(landing_indices), 1)
        landing_index = landing_indices[0]

        footprint = make_stair_footprint(specs)
        sections = fit_balanced_sections_to_footprint(sections, footprint)
        directions = ((1.0, 0.0), (0.0, 1.0))
        for section in sections:
            direction = directions[section.flight_index]
            self.assertTrue(section.locked_to_flight)
            self.assertAlmostEqual(
                section.tangent[0] * direction[1] - section.tangent[1] * direction[0],
                0.0,
            )
        faces = balanced_tread_faces(sections, footprint)
        landing = faces[landing_indices[0]]
        self.assertAlmostEqual(landing.Area, 800.0 * 800.0, delta=1.0)
        self.assertAlmostEqual(landing.BoundBox.XLength, 800.0, delta=0.1)
        self.assertAlmostEqual(landing.BoundBox.YLength, 800.0, delta=0.1)
        self.assertTrue(balanced_partition_is_valid(faces, footprint, 9))

        concrete = make_balanced_concrete_shape(sections, footprint, 180.0, 150.0)
        self.assertTrue(concrete.isValid())
        self.assertEqual(len(concrete.Solids), 1)
        landing_top = (landing_index + 1) * 180.0
        expected_landing_bottom = landing_top - 150.0
        landing_bottoms = [
            face
            for face in concrete.Faces
            if face.BoundBox.ZLength < 1e-7
            and abs(face.BoundBox.ZMin - expected_landing_bottom) < 0.01
            and face.BoundBox.XMax > landing.BoundBox.XMin + 0.01
            and face.BoundBox.XMin < landing.BoundBox.XMax - 0.01
            and face.BoundBox.YMax > landing.BoundBox.YMin + 0.01
            and face.BoundBox.YMin < landing.BoundBox.YMax - 0.01
        ]
        self.assertEqual(len(landing_bottoms), 1)
        landing_bottom = landing_bottoms[0]
        self.assertGreater(landing_bottom.Area, landing.Area * 0.5)
        self.assertAlmostEqual(
            landing_top - landing_bottom.BoundBox.ZMin,
            150.0,
        )

        concrete_sections, _going = balanced_winder_sections(
            specs,
            9,
            1.0,
            ["Landing"],
            landing_replaces_tread=False,
        )
        concrete_sections = fit_balanced_sections_to_footprint(concrete_sections, footprint)
        concrete_faces = balanced_tread_faces(concrete_sections, footprint)
        concrete_landing_index = next(
            index for index, section in enumerate(concrete_sections[:-1]) if section.landing_to_next
        )
        self.assertEqual(len(concrete_sections), 11)
        self.assertEqual(
            concrete_sections[concrete_landing_index - 1].riser_index,
            concrete_sections[concrete_landing_index].riser_index,
        )
        self.assertEqual(
            concrete_sections[concrete_landing_index + 1].riser_index,
            concrete_sections[concrete_landing_index].riser_index + 1,
        )
        self.assertTrue(
            balanced_partition_is_valid(concrete_faces, footprint, len(concrete_sections) - 1)
        )
        concrete_landing = make_balanced_concrete_shape(
            concrete_sections,
            footprint,
            180.0,
            150.0,
            concrete_faces,
        )
        self.assertTrue(concrete_landing.isValid())
        self.assertEqual(len(concrete_landing.Solids), 1)
        sloped_faces = []
        for face in concrete_landing.Faces:
            parameters = face.ParameterRange
            normal = face.normalAt(
                (parameters[0] + parameters[1]) / 2.0,
                (parameters[2] + parameters[3]) / 2.0,
            )
            if 0.01 < abs(normal.z) < 0.999999:
                sloped_faces.append(face)
        self.assertEqual(len(sloped_faces), 2)

        expected_end_bottom = (len(sections) - 1) * 180.0 - 150.0
        for point in (sections[-1].left, sections[-1].right):
            self.assertTrue(
                any(
                    math.hypot(
                        vertex.Point.x - point[0],
                        vertex.Point.y - point[1],
                    )
                    < 0.01
                    and abs(vertex.Point.z - expected_end_bottom) < 0.01
                    for vertex in concrete.Vertexes
                )
            )

        expanded_treads = []
        for index, (front, rear, face) in enumerate(zip(sections, sections[1:], faces)):
            expanded_treads.append(
                make_balanced_tread_shape(
                    front,
                    rear,
                    footprint,
                    40.0,
                    40.0,
                    30.0,
                    28.0,
                    base_face=face,
                    local_expansion=(
                        front.landing_to_next
                        or rear.landing_to_next
                        or (index > 0 and sections[index - 1].landing_to_next)
                    ),
                )
            )
        self.assertLessEqual(expanded_treads[landing_index - 1].BoundBox.YMax, 800.1)
        self.assertGreaterEqual(expanded_treads[landing_index + 1].BoundBox.XMin, 1199.9)
        for index in (landing_index - 1, landing_index + 1):
            front = sections[index]
            rear = sections[index + 1]
            direction = front.tangent
            self.assertAlmostEqual(
                direction[0] * rear.tangent[1] - direction[1] * rear.tangent[0],
                0.0,
            )

            def projection_bounds(shape):
                projections = [
                    vertex.Point.x * direction[0] + vertex.Point.y * direction[1]
                    for vertex in shape.Vertexes
                ]
                return min(projections), max(projections)

            tread_min, tread_max = projection_bounds(expanded_treads[index])
            front_riser_min, _front_riser_max = projection_bounds(
                make_balanced_riser_shape(front, 0.0, 100.0, 18.0)
            )
            _rear_riser_min, rear_riser_max = projection_bounds(
                make_balanced_riser_shape(rear, 0.0, 100.0, 18.0)
            )
            self.assertAlmostEqual(front_riser_min - tread_min, 30.0)
            self.assertAlmostEqual(tread_max - rear_riser_max, 10.0)

        mixed_specs = [
            (1600.0, 800.0, 0.0),
            (1600.0, 800.0, 90.0),
            (1600.0, 800.0, 180.0),
        ]
        mixed_sections, _going = balanced_winder_sections(
            mixed_specs,
            12,
            1.0,
            ["Landing", "Herse balancing"],
        )
        self.assertEqual(sum(section.landing_to_next for section in mixed_sections), 1)
        mixed_footprint = make_stair_footprint(mixed_specs)
        mixed_sections = fit_balanced_sections_to_footprint(mixed_sections, mixed_footprint)
        mixed_directions = ((1.0, 0.0), (0.0, 1.0), (-1.0, 0.0))
        self.assertTrue(any(not section.locked_to_flight for section in mixed_sections))
        for section in mixed_sections:
            if not section.locked_to_flight:
                continue
            direction = mixed_directions[section.flight_index]
            self.assertAlmostEqual(
                section.tangent[0] * direction[1] - section.tangent[1] * direction[0],
                0.0,
            )
        mixed_faces = balanced_tread_faces(mixed_sections, mixed_footprint)
        self.assertTrue(balanced_partition_is_valid(mixed_faces, mixed_footprint, 12))

    def test_turn_and_half_turn_flights(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=1600.0,
            width=800.0,
            steps=10,
        )
        stair.Proxy._updating = True
        try:
            flights = resize_flights(stair, 2, length=1600.0, width=800.0, rotations=["Left"])
            flights[0].Angle = 0.0
            flights[1].Angle = 90.0
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        flights = get_flights(stair)
        self.assertEqual(len(flights), 2)
        self.assertEqual(str(flights[1].Rotation), "Left")
        self.assertAlmostEqual(flights[1].Angle.Value, 90.0)
        self.assertEqual(sum(flight.NumberOfTreads for flight in flights), 9)
        second_flight_treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread" and child.FlightIndex == 2
        ]
        self.assertTrue(second_flight_treads)
        self.assertTrue(all(tread.Shape.isValid() for tread in second_flight_treads))
        self.assertTrue(all(len(tread.Shape.Solids) == 1 for tread in second_flight_treads))
        herse_treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        ]
        self.assertTrue(all(len(tread.Shape.Solids) == 1 for tread in herse_treads))
        self.assertGreater(
            max(tread.Shape.BoundBox.YMax for tread in second_flight_treads),
            800.0,
        )

        flights[1].TurnType = "Landing"
        self.document.recompute()
        self.assertEqual(stair.GeometryStatus, "Multi-flight stair with landing")
        self.assertTrue(
            all(
                child.Shape.isValid()
                for child in stair.StepsGroup.Group
                if getattr(child, "StairDesignerRole", "") in ("Tread", "Riser")
            )
        )
        landing_treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
            and 800.0 < child.Shape.BoundBox.XLength <= 830.1
            and 800.0 < child.Shape.BoundBox.YLength <= 828.1
        ]
        self.assertEqual(len(landing_treads), 1)
        generated_treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        ]
        self.assertTrue(
            all(
                tread.Shape.BoundBox.XLength <= 830.1 and tread.Shape.BoundBox.YLength <= 828.1
                for tread in generated_treads
            )
        )

        flights[1].TurnType = "Herse balancing"
        flights[1].Rotation = "Right"
        self.document.recompute()
        self.assertLess(
            min(tread.Shape.BoundBox.YMin for tread in second_flight_treads),
            0.0,
        )

        stair.Proxy._updating = True
        try:
            flights = resize_flights(
                stair,
                3,
                length=1200.0,
                width=800.0,
                rotations=["Left", "Left"],
            )
            for index, flight in enumerate(flights):
                flight.Proxy._updating = True
                try:
                    flight.LeftLength = 1200.0
                    flight.RightLength = 1200.0
                    flight.Width = 800.0
                    flight.Angle = 0.0 if index == 0 else 90.0
                    if index:
                        flight.Rotation = "Left"
                finally:
                    flight.Proxy._updating = False
            sync_all_flight_side_lengths(stair)
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        flights = get_flights(stair)
        self.assertEqual(len(flights), 3)
        self.assertEqual([str(flight.Rotation) for flight in flights[1:]], ["Left", "Left"])
        self.assertEqual(sum(flight.NumberOfTreads for flight in flights), 9)
        self.assertGreater(stair.PlanSketch.GeometryCount, stair.NumberOfTreads)

    def test_balanced_wood_risers_can_be_toggled(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=1600.0,
            width=800.0,
            steps=10,
            stair_type="Wood",
        )
        stair.Proxy._updating = True
        try:
            flights = resize_flights(stair, 2, length=1600.0, width=800.0, rotations=["Left"])
            flights[0].Angle = 0.0
            flights[1].Angle = 90.0
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)

        stair.RisersEnabled = False
        self.document.recompute()
        self.assertFalse(
            any(
                getattr(child, "StairDesignerRole", "") == "Riser"
                for child in stair.StepsGroup.Group
            )
        )

        stair.RisersEnabled = True
        self.document.recompute()
        risers = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Riser"
        ]
        self.assertEqual(len(risers), stair.NumberOfTreads + 1)
        self.assertTrue(all(riser.Shape.isValid() for riser in risers))

    def test_individual_tread_extra_width_redistributes_the_going(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=2400.0,
            width=800.0,
            steps=10,
            stair_type="Wood",
        )
        self.document.recompute()
        treads = sorted(
            (
                child
                for child in stair.StepsGroup.Group
                if getattr(child, "StairDesignerRole", "") == "Tread"
            ),
            key=lambda child: child.Index,
        )
        target = treads[6]
        neighbour = treads[5]
        following = treads[7]
        terminal = treads[-1]
        self.assertIn("ExtraWidth", target.PropertiesList)
        terminal_end = terminal.Shape.BoundBox.XMax

        target.ExtraWidth = 50.0
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        general_going = (2400.0 - 50.0) / 9.0
        self.assertAlmostEqual(target.ExtraWidth.Value, 50.0)
        self.assertAlmostEqual(
            stair.TreadWidth.Value,
            general_going,
        )
        self.assertAlmostEqual(
            target.Shape.BoundBox.XMin - neighbour.Shape.BoundBox.XMin,
            general_going,
        )
        self.assertAlmostEqual(
            following.Shape.BoundBox.XMin - target.Shape.BoundBox.XMin,
            general_going + 50.0,
        )
        self.assertAlmostEqual(
            terminal.Shape.BoundBox.XMax,
            terminal_end,
        )

        following.ExtraWidth = 50.0
        terminal.ExtraWidth = 50.0
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        self.assertAlmostEqual(stair.TreadWidth.Value, 250.0)
        self.assertAlmostEqual(
            following.Shape.BoundBox.XMin - target.Shape.BoundBox.XMin,
            300.0,
        )
        self.assertAlmostEqual(
            terminal.Shape.BoundBox.XMin - following.Shape.BoundBox.XMin,
            300.0,
        )
        self.assertAlmostEqual(
            terminal.Shape.BoundBox.XMax,
            terminal_end,
        )

        following.ExtraWidth = 0.0
        terminal.ExtraWidth = 0.0
        target.ExtraWidth = -50.0
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        general_going = (2400.0 + 50.0) / 9.0
        self.assertEqual(
            target.getTypeIdOfProperty("ExtraWidth"),
            "App::PropertyDistance",
        )
        self.assertAlmostEqual(target.ExtraWidth.Value, -50.0)
        self.assertAlmostEqual(stair.TreadWidth.Value, general_going)
        self.assertAlmostEqual(
            target.Shape.BoundBox.XMin - neighbour.Shape.BoundBox.XMin,
            general_going,
        )
        self.assertAlmostEqual(
            following.Shape.BoundBox.XMin - target.Shape.BoundBox.XMin,
            general_going - 50.0,
        )
        self.assertAlmostEqual(
            terminal.Shape.BoundBox.XMax,
            terminal_end,
        )

        stair.Proxy._updating = True
        try:
            resize_flights(
                stair,
                2,
                length=1600.0,
                width=800.0,
                rotations=["Left"],
            )
        finally:
            stair.Proxy._updating = False
        target.ExtraWidth = 0.0
        following.ExtraWidth = 0.0
        terminal.ExtraWidth = 0.0
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        balanced_volume = target.Shape.Volume
        target.ExtraWidth = 50.0
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        self.assertTrue(target.Shape.isValid())
        self.assertEqual(len(target.Shape.Solids), 1)
        self.assertGreater(target.Shape.Volume, balanced_volume)

    def test_balanced_extra_width_changes_nosing_stations(self):
        specs = [(1600.0, 800.0, 0.0), (1600.0, 800.0, 90.0)]
        reference, _reference_going = balanced_winder_sections(specs, 9)
        extras = [0.0] * 9
        extras[6] = 50.0
        sections, going = balanced_winder_sections(
            specs,
            9,
            extra_widths=extras,
        )

        total_run = reference[-1].station
        self.assertAlmostEqual(going, (total_run - 50.0) / 9.0)
        self.assertAlmostEqual(sections[-1].station, total_run)
        for index, (front, rear) in enumerate(zip(sections, sections[1:])):
            expected = going + (50.0 if index == 6 else 0.0)
            self.assertAlmostEqual(rear.station - front.station, expected)

        extras[6] = -50.0
        sections, going = balanced_winder_sections(
            specs,
            9,
            extra_widths=extras,
        )
        self.assertAlmostEqual(going, (total_run + 50.0) / 9.0)
        self.assertAlmostEqual(sections[-1].station, total_run)
        for index, (front, rear) in enumerate(zip(sections, sections[1:])):
            expected = going + (-50.0 if index == 6 else 0.0)
            self.assertAlmostEqual(rear.station - front.station, expected)

    def test_individual_riser_extra_height_redistributes_the_rises(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=2400.0,
            width=800.0,
            steps=10,
            stair_type="Wood",
        )
        stair.RisersEnabled = True
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        treads = sorted(
            (
                child
                for child in stair.StepsGroup.Group
                if getattr(child, "StairDesignerRole", "") == "Tread"
            ),
            key=lambda child: child.Index,
        )
        risers = sorted(
            (
                child
                for child in stair.StepsGroup.Group
                if getattr(child, "StairDesignerRole", "") == "Riser"
            ),
            key=lambda child: child.Index,
        )
        target = treads[6]
        neighbour = treads[5]
        following = treads[7]

        target.ExtraHeight = 50.0
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        general_height = (1800.0 - 50.0) / 10.0
        self.assertEqual(
            target.getTypeIdOfProperty("ExtraHeight"),
            "App::PropertyDistance",
        )
        self.assertAlmostEqual(target.ExtraHeight.Value, 50.0)
        self.assertAlmostEqual(stair.RiserHeight.Value, general_height)
        self.assertAlmostEqual(
            target.Shape.BoundBox.ZMax - neighbour.Shape.BoundBox.ZMax,
            general_height + 50.0,
        )
        self.assertAlmostEqual(
            following.Shape.BoundBox.ZMax - target.Shape.BoundBox.ZMax,
            general_height,
        )
        self.assertAlmostEqual(
            treads[-1].Shape.BoundBox.ZMax + general_height,
            stair.FloorHeight.Value,
        )
        self.assertAlmostEqual(
            risers[6].Shape.BoundBox.ZMax,
            target.Shape.BoundBox.ZMin,
        )

        target.ExtraHeight = -50.0
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        general_height = (1800.0 + 50.0) / 10.0
        self.assertAlmostEqual(target.ExtraHeight.Value, -50.0)
        self.assertAlmostEqual(stair.RiserHeight.Value, general_height)
        self.assertAlmostEqual(
            target.Shape.BoundBox.ZMax - neighbour.Shape.BoundBox.ZMax,
            general_height - 50.0,
        )
        self.assertAlmostEqual(
            following.Shape.BoundBox.ZMax - target.Shape.BoundBox.ZMax,
            general_height,
        )
        self.assertAlmostEqual(
            treads[-1].Shape.BoundBox.ZMax + general_height,
            stair.FloorHeight.Value,
        )

        stair.Proxy._updating = True
        try:
            resize_flights(
                stair,
                2,
                length=1600.0,
                width=800.0,
                rotations=["Left"],
            )
        finally:
            stair.Proxy._updating = False
        target.ExtraHeight = 50.0
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        self.assertTrue(target.Shape.isValid())
        self.assertTrue(risers[6].Shape.isValid())
        self.assertAlmostEqual(
            target.Shape.BoundBox.ZMax - neighbour.Shape.BoundBox.ZMax,
            (1800.0 - 50.0) / 10.0 + 50.0,
        )

    def test_wood_stair_supports_per_side_stringer_types(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=1600.0,
            width=800.0,
            steps=10,
            stair_type="Wood",
        )
        stair.Proxy._updating = True
        # The broad-face checks below count the plain board faces.
        stair.StringerJoinery = False
        stair.Proxy._updating = False
        self.document.recompute()

        self.assertIsNone(stair.StringersGroup)
        self.assertNotIn("StepMaterial", stair.PropertiesList)
        self.assertNotIn("RiserMaterial", stair.PropertiesList)
        self.assertTrue(
            all("Material" not in child.PropertiesList for child in stair.StepsGroup.Group)
        )

        first_flight = get_flights(stair)[0]
        first_flight.Proxy._updating = True
        try:
            first_flight.RightStringerType = [
                "None",
                "Housed stringer",
                "Cut stringer",
            ]
            first_flight.RightStringerType = "Cut stringer"
        finally:
            first_flight.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.assertEqual(str(first_flight.RightStringerType), "Notched stringer")

        stair.Proxy._updating = True
        first_flight.Proxy._updating = True
        try:
            first_flight.LeftStringerType = "Housed stringer"
            first_flight.RightStringerType = "Notched stringer"
        finally:
            first_flight.Proxy._updating = False
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        self.assertIsNotNone(stair.StringersGroup)
        self.assertNotIn("LeftStringerType", stair.PropertiesList)
        self.assertNotIn("RightStringerType", stair.PropertiesList)
        stringers = stair.StringersGroup.Group
        self.assertEqual(
            {part.StairDesignerRole for part in stringers},
            {"LeftStringer", "RightStringer"},
        )
        self.assertTrue(
            all(
                part.Shape.isValid()
                and len(part.Shape.Solids) == 1
                and part.Shape.Volume > 0.0
                and "Material" not in part.PropertiesList
                for part in stringers
            )
        )
        self.assertFalse(stair.StringerCustomWidth)
        self.assertGreaterEqual(stair.StringerWidth.Value, 235.0)

        left_stringer = next(part for part in stringers if part.StairDesignerRole == "LeftStringer")
        right_stringer = next(
            part for part in stringers if part.StairDesignerRole == "RightStringer"
        )
        broad_faces = [
            face
            for face in right_stringer.Shape.Faces
            if face.Vertexes
            and max(vertex.Point.y for vertex in face.Vertexes)
            - min(vertex.Point.y for vertex in face.Vertexes)
            < 1e-7
        ]
        self.assertEqual(len(broad_faces), 2)

        stair.RisersEnabled = True
        self.document.recompute()
        risers = [child for child in stair.StepsGroup.Group if child.StairDesignerRole == "Riser"]
        self.assertTrue(risers)
        self.assertTrue(
            all(right_stringer.Shape.common(riser.Shape).Volume < 1e-7 for riser in risers)
        )
        stair.RisersEnabled = False
        self.document.recompute()
        stair.Proxy._updating = True
        try:
            stair.StringerNosingOffset = 0.0
            stair.StringerStartExtension = 100.0
        finally:
            stair.Proxy._updating = False
        for direction in ("Perpendicular", "Vertical"):
            stair.Proxy._updating = True
            try:
                stair.StringerNosingOffsetDirection = direction
            finally:
                stair.Proxy._updating = False
            stair.Proxy.rebuild(stair, allow_structure_changes=True)
            section_plane = Part.makePlane(
                20000.0,
                20000.0,
                FreeCAD.Vector(-stair.Nosing.Value, 10000.0, -10000.0),
                FreeCAD.Vector(1.0, 0.0, 0.0),
            )
            section = left_stringer.Shape.section(section_plane)
            self.assertFalse(section.isNull())
            self.assertAlmostEqual(
                section.BoundBox.ZMax,
                stair.RiserHeight.Value,
                places=5,
            )

        stair.StringerCustomWidth = True
        stair.StringerWidth = 260.0
        stair.NumberOfSteps = 11
        self.document.recompute()
        self.assertAlmostEqual(stair.StringerWidth.Value, 260.0)
        self.assertTrue(
            all(abs(part.Width.Value - 260.0) < 1e-7 for part in stair.StringersGroup.Group)
        )

        left_stringer.OverrideThickness = True
        left_stringer.Thickness = 55.0
        stair.StringerThickness = 45.0
        self.document.recompute()
        self.assertAlmostEqual(left_stringer.Thickness.Value, 55.0)
        self.assertTrue(
            all(
                abs(part.Thickness.Value - 45.0) < 1e-7
                for part in stair.StringersGroup.Group
                if part != left_stringer
            )
        )

        stair.Proxy._updating = True
        try:
            flights = resize_flights(stair, 2, length=1600.0, width=800.0, rotations=["Left"])
            flights[0].Angle = 0.0
            flights[1].Angle = 90.0
            flights[1].Proxy._updating = True
            flights[1].LeftStringerType = "Housed stringer"
            flights[1].RightStringerType = "None"
            flights[1].Proxy._updating = False
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        self.assertEqual(len(stair.StringersGroup.Group), 3)
        self.assertEqual(
            sum(part.StairDesignerRole == "LeftStringer" for part in stair.StringersGroup.Group),
            2,
        )
        flights[1].RightStringerType = "Notched stringer"
        self.document.recompute()
        self.assertEqual(len(stair.StringersGroup.Group), 4)
        self.assertEqual(left_stringer.SourceFlight.Name, flights[0].Name)
        self.assertAlmostEqual(left_stringer.Thickness.Value, 55.0)
        for role in ("LeftStringer", "RightStringer"):
            parts = sorted(
                (part for part in stair.StringersGroup.Group if part.StairDesignerRole == role),
                key=lambda part: part.FlightIndex,
            )
            self.assertEqual(len(parts), 2)
            self.assertLess(
                parts[0].Shape.distToShape(parts[1].Shape)[0],
                1e-7,
            )
            self.assertLess(
                parts[0].Shape.common(parts[1].Shape).Volume,
                1e-7,
            )
            for part in parts:
                broad_face_offsets = {
                    round(
                        vertex.Point.y if part.FlightIndex == 1 else vertex.Point.x,
                        6,
                    )
                    for vertex in part.Shape.Vertexes
                }
                self.assertEqual(len(broad_face_offsets), 2)
        self.assertTrue(
            all(
                part.Shape.isValid() and len(part.Shape.Solids) == 1 and part.Shape.Volume > 0.0
                for part in stair.StringersGroup.Group
            )
        )

        stair.Proxy._updating = True
        try:
            flights[1].Proxy._updating = True
            flights[1].FlightType = "Circular"
            flights[1].InnerRadius = 450.0
            flights[1].OuterRadius = 1250.0
            flights[1].Angle = 90.0
            flights[1].Proxy._updating = False
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        self.assertEqual(len(stair.StringersGroup.Group), 4)
        tangent_x = (flights[0].LeftLength.Value + flights[0].RightLength.Value) / 2.0
        for role in ("LeftStringer", "RightStringer"):
            parts = sorted(
                (part for part in stair.StringersGroup.Group if part.StairDesignerRole == role),
                key=lambda part: part.FlightIndex,
            )
            self.assertEqual(len(parts), 2)
            self.assertLessEqual(parts[0].Shape.BoundBox.XMax, tangent_x + 1e-5)
            self.assertLess(parts[0].Shape.distToShape(parts[1].Shape)[0], 1e-5)
            cylindrical_faces = [
                face for face in parts[1].Shape.Faces if isinstance(face.Surface, Part.Cylinder)
            ]
            cylindrical_radii = {round(face.Surface.Radius, 5) for face in cylindrical_faces}
            self.assertEqual(len(cylindrical_radii), 2, role)
            self.assertAlmostEqual(
                max(cylindrical_radii) - min(cylindrical_radii),
                parts[1].Thickness.Value,
                places=4,
            )
        self.assertTrue(
            all(
                part.Shape.isValid() and len(part.Shape.Solids) == 1 and part.Shape.Volume > 0.0
                for part in stair.StringersGroup.Group
            )
        )

        stair.Proxy._updating = True
        try:
            flights = resize_flights(
                stair, 3, length=1600.0, width=800.0, rotations=["Left", "Left"]
            )
            flight_types = ("Straight", "Straight landing", "Straight")
            lengths = (1600.0, 2400.0, 1600.0)
            angles = (0.0, 90.0, 0.0)
            for flight, flight_type, length, angle in zip(flights, flight_types, lengths, angles):
                flight.Proxy._updating = True
                try:
                    flight.FlightType = flight_type
                    flight.LeftLength = length
                    flight.RightLength = length
                    flight.Width = 800.0
                    flight.Angle = angle
                    if flight_type == "Straight landing":
                        flight.EntryDirection = "From left"
                        flight.ExitDirection = "To left"
                        flight.LeftStringerType = "Housed stringer"
                        flight.RightStringerType = "Notched stringer"
                    else:
                        flight.LeftStringerType = "Housed stringer"
                        flight.RightStringerType = "Notched stringer"
                finally:
                    flight.Proxy._updating = False
            sync_all_flight_side_lengths(stair)
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        self.assertEqual(len(stair.StringersGroup.Group), 6)
        self.assertTrue(
            all(
                part.Shape.isValid() and len(part.Shape.Solids) == 1 and part.Shape.Volume > 0.0
                for part in stair.StringersGroup.Group
            )
        )

        stair.StairType = "Concrete"
        self.document.recompute()
        self.assertIsNone(stair.StringersGroup)

    def test_circular_housed_stringer_keeps_nosing_clearance(self):
        specs = [
            ("Straight", 1600.0, 800.0, 0.0, "Left", "Straight", "Straight"),
            ("Circular", 500.0, 800.0, 90.0, "Left", "Straight", "Straight"),
        ]
        sections, _going = tangent_flight_sections(specs, 9)
        footprint = make_tangent_stair_footprint(specs)
        sections = fit_tangent_sections_to_footprint(sections, footprint)
        circular_run = stringer_flight_runs(sections, [spec[0] for spec in specs])[1][1]
        elevations = _stringer_elevations(circular_run, 180.0)

        for side in ("Left", "Right"):
            shape = make_housed_stringer_shape(
                circular_run,
                180.0,
                side,
                40.0,
                330.0,
                20.0,
                0.0,
                0.0,
                50.0,
                "Perpendicular",
                20.0,
            )
            circular = _circular_stringer_data(circular_run, side, 40.0, 20.0, True)
            profile = circular["profile"]
            radius = (profile.inner_radius + profile.outer_radius) / 2.0
            clearances = []
            top_elevations = []
            for angle in circular["angles"]:
                x = profile.center[0] + radius * math.cos(angle)
                y = profile.center[1] + radius * math.sin(angle)
                probe = Part.makeLine(
                    FreeCAD.Vector(x, y, -1000.0),
                    FreeCAD.Vector(x, y, 3000.0),
                )
                intersection = shape.common(probe)
                self.assertTrue(intersection.Vertexes, side)
                top_elevations.append(max(vertex.Point.z for vertex in intersection.Vertexes))

            clearances = [
                top - elevation for top, elevation in zip(top_elevations[:-1], elevations[:-1])
            ]

            self.assertTrue(shape.isValid(), side)
            self.assertEqual(len(shape.Solids), 1, side)
            for clearance in clearances[1:]:
                self.assertAlmostEqual(clearance, clearances[0], places=5)
            self.assertAlmostEqual(top_elevations[-1] - top_elevations[-2], 180.0, places=5)

    def test_linear_stringer_after_circular_flight_has_straight_pitch(self):
        specs = [
            ("Straight", 1600.0, 800.0, 0.0, "Left", "Straight", "Straight"),
            ("Circular", 500.0, 800.0, 90.0, "Left", "Straight", "Straight"),
            ("Straight", 1600.0, 800.0, 0.0, "Left", "Straight", "Straight"),
        ]
        sections, _going = tangent_flight_sections(specs, 10)
        footprint = make_tangent_stair_footprint(specs)
        sections = fit_tangent_sections_to_footprint(sections, footprint)
        final_run = stringer_flight_runs(sections, [spec[0] for spec in specs])[2][1]
        shape = make_housed_stringer_shape(
            final_run,
            180.0,
            "Left",
            40.0,
            330.0,
            20.0,
            0.0,
            100.0,
            50.0,
            "Perpendicular",
            20.0,
        )

        def top_at(point):
            probe = Part.makeLine(
                FreeCAD.Vector(point[0], point[1], -1000.0),
                FreeCAD.Vector(point[0], point[1], 3000.0),
            )
            intersection = shape.common(probe)
            self.assertTrue(intersection.Vertexes)
            return max(vertex.Point.z for vertex in intersection.Vertexes)

        rail_points = [section.left for section in final_run]
        tops = [top_at(point) for point in rail_points]
        for first, second in zip(tops, tops[1:]):
            self.assertAlmostEqual(second - first, 180.0, places=5)
        for first_point, second_point, first_top, second_top in zip(
            rail_points, rail_points[1:], tops, tops[1:]
        ):
            midpoint = (
                (first_point[0] + second_point[0]) / 2.0,
                (first_point[1] + second_point[1]) / 2.0,
            )
            self.assertAlmostEqual(top_at(midpoint), (first_top + second_top) / 2.0, places=5)

    def test_final_flight_end_angle_keeps_stringers_on_flight_axis(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=1600.0,
            width=800.0,
            steps=10,
            stair_type="Wood",
        )
        stair.Proxy._updating = True
        try:
            # The coverage check below needs boards without housings.
            stair.StringerJoinery = False
            flights = resize_flights(
                stair,
                2,
                length=1600.0,
                width=800.0,
                rotations=["Left"],
            )
            for flight in flights:
                flight.Proxy._updating = True
                try:
                    flight.LeftStringerType = "Housed stringer"
                    flight.RightStringerType = "Housed stringer"
                finally:
                    flight.Proxy._updating = False
        finally:
            stair.Proxy._updating = False
        heading = math.radians(90.0)
        normal = (-math.sin(heading), math.cos(heading))
        for end_angle in (0.0, 30.0):
            flights[-1].EndAngle = end_angle
            stair.Proxy.rebuild(stair, allow_structure_changes=True)
            self.document.recompute()

            final_stringers = [
                part for part in stair.StringersGroup.Group if part.SourceFlight == flights[-1]
            ]
            final_treads = [
                part
                for part in stair.StepsGroup.Group
                if part.StairDesignerRole == "Tread" and part.FlightIndex == 2
            ]
            self.assertEqual(len(final_stringers), 2)
            for stringer in final_stringers:
                projections = [
                    vertex.Point.x * normal[0] + vertex.Point.y * normal[1]
                    for vertex in stringer.Shape.Vertexes
                ]
                self.assertTrue(stringer.Shape.isValid())
                self.assertEqual(len(stringer.Shape.Solids), 1)
                self.assertLessEqual(
                    max(projections) - min(projections),
                    stringer.Thickness.Value + 1e-5,
                )
                bounds = stringer.Shape.BoundBox
                plan_prism = Part.makeBox(
                    max(bounds.XLength, 0.01),
                    max(bounds.YLength, 0.01),
                    4000.0,
                    FreeCAD.Vector(
                        bounds.XMin,
                        bounds.YMin,
                        -1000.0,
                    ),
                )
                for tread in final_treads:
                    expected_overlap = tread.Shape.common(plan_prism)
                    if expected_overlap.Volume < 1e-6:
                        continue
                    uncovered = expected_overlap.cut(stringer.Shape)
                    self.assertAlmostEqual(
                        uncovered.Volume,
                        0.0,
                        delta=1e-3,
                        msg=(
                            f"end={end_angle}, "
                            f"side={stringer.Side}, "
                            f"step={tread.Index}, "
                            f"bounds={uncovered.BoundBox}, "
                            f"stringer={stringer.Shape.BoundBox}, "
                            f"tread={tread.Shape.BoundBox}"
                        ),
                    )

    def test_landing_after_circular_flight_rebuilds_and_has_stringers(self):
        for landing_type in ("Straight landing", "Circular landing"):
            stair = make_stair(
                floor_height=1800.0,
                flight_length=1600.0,
                width=800.0,
                steps=10,
                stair_type="Wood",
            )
            stair.Proxy._updating = True
            try:
                # Count only the board radii, not the housing faces.
                stair.StringerJoinery = False
                flights = resize_flights(
                    stair,
                    2,
                    length=1600.0,
                    width=800.0,
                    rotations=["Left"],
                )
                flights[1].Proxy._updating = True
                flights[1].FlightType = "Circular"
                flights[1].Proxy._updating = False
            finally:
                stair.Proxy._updating = False
            stair.Proxy.rebuild(stair, allow_structure_changes=True)

            stair.Proxy._updating = True
            try:
                flights = resize_flights(
                    stair,
                    3,
                    length=1600.0,
                    width=800.0,
                    rotations=["Left", "Left"],
                )
            finally:
                stair.Proxy._updating = False
            stair.Proxy.rebuild(stair, allow_structure_changes=True)
            self.document.recompute()
            self.assertEqual(str(flights[2].FlightType), "Straight")

            flights[2].FlightType = landing_type
            self.document.recompute()
            self.assertEqual(stair.GeometryStatus, "Multi-flight stair with landing")
            self.assertTrue(
                all(
                    part.Shape.isValid() and len(part.Shape.Solids) == 1 and part.Shape.Volume > 0.0
                    for part in stair.StepsGroup.Group
                )
            )

            stair.Proxy._updating = True
            try:
                for flight in flights:
                    flight.Proxy._updating = True
                    try:
                        flight.LeftStringerType = "Housed stringer"
                        flight.RightStringerType = "Notched stringer"
                    finally:
                        flight.Proxy._updating = False
            finally:
                stair.Proxy._updating = False
            stair.Proxy.rebuild(stair, allow_structure_changes=True)
            self.document.recompute()

            stringers = stair.StringersGroup.Group
            self.assertEqual(len(stringers), 6)
            self.assertTrue(
                all(
                    part.Shape.isValid() and len(part.Shape.Solids) == 1 and part.Shape.Volume > 0.0
                    for part in stringers
                )
            )
            self.assertEqual(
                sum(part.SourceFlight == flights[2] for part in stringers),
                2,
            )
            for role in ("LeftStringer", "RightStringer"):
                parts = sorted(
                    (part for part in stringers if part.StairDesignerRole == role),
                    key=lambda part: part.FlightIndex,
                )
                self.assertLess(
                    parts[1].Shape.distToShape(parts[2].Shape)[0],
                    1e-5,
                    f"{landing_type}: {role}",
                )
            if landing_type == "Circular landing":
                for part in (part for part in stringers if part.FlightIndex == 3):
                    radii = {
                        round(face.Surface.Radius, 5)
                        for face in part.Shape.Faces
                        if isinstance(face.Surface, Part.Cylinder)
                    }
                    self.assertEqual(len(radii), 2)

            stair.StairType = "Concrete"
            self.document.recompute()
            shape = stair.ConcreteGeometry.Shape
            self.assertFalse(shape.isNull())
            self.assertTrue(shape.isValid())
            self.assertEqual(len(shape.Solids), 1)
            self.assertIsNone(stair.StringersGroup)

    def test_handrails_support_linear_circular_wood_and_concrete_stairs(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=2400.0,
            width=900.0,
            steps=10,
            stair_type="Wood",
        )
        first_flight = get_flights(stair)[0]
        first_flight.Proxy._updating = True
        try:
            first_flight.LeftStringerType = "Housed stringer"
            first_flight.LeftHandrailEnabled = True
        finally:
            first_flight.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        self.assertIsNotNone(stair.HandrailsGroup)
        parts = stair.HandrailsGroup.Group
        top_rails = [part for part in parts if part.StairDesignerRole == "HandrailTopRail"]
        posts = [part for part in parts if part.StairDesignerRole == "HandrailPost"]
        pickets = [part for part in parts if part.StairDesignerRole == "HandrailPicket"]
        self.assertEqual(len(top_rails), 1)
        self.assertEqual(len(posts), 2)
        self.assertGreater(len(pickets), 1)
        self.assertTrue(
            all(
                part.Shape.isValid() and len(part.Shape.Solids) == 1 and part.Shape.Volume > 0.0
                for part in parts
            )
        )
        self.assertTrue(
            all(isinstance(face.Surface, Part.Plane) for face in top_rails[0].Shape.Faces)
        )
        first_post = next(post for post in posts if post.ElementIndex == 0)
        self.assertAlmostEqual(first_post.Shape.BoundBox.ZMin, 0.0)
        post_centers = sorted(
            (post.Shape.BoundBox.XMin + post.Shape.BoundBox.XMax) / 2.0 for post in posts
        )
        picket_centers = sorted(
            (picket.Shape.BoundBox.XMin + picket.Shape.BoundBox.XMax) / 2.0 for picket in pickets
        )
        clear_gaps = [
            picket_centers[0]
            - post_centers[0]
            - (stair.HandrailPostThickness.Value + stair.HandrailPicketThickness.Value) / 2.0,
            *[
                second - first - stair.HandrailPicketThickness.Value
                for first, second in zip(picket_centers, picket_centers[1:])
            ],
            post_centers[-1]
            - picket_centers[-1]
            - (stair.HandrailPostThickness.Value + stair.HandrailPicketThickness.Value) / 2.0,
        ]
        self.assertLessEqual(
            max(clear_gaps),
            stair.HandrailPicketMaximumSpacing.Value + 1e-7,
        )

        initial_picket_count = len(pickets)
        stair.HandrailPicketMaximumSpacing = 50.0
        self.document.recompute()
        tighter_pickets = [
            part
            for part in stair.HandrailsGroup.Group
            if part.StairDesignerRole == "HandrailPicket"
        ]
        self.assertGreater(len(tighter_pickets), initial_picket_count)

        stair.Proxy._updating = True
        try:
            flights = resize_flights(
                stair,
                2,
                length=1800.0,
                width=900.0,
                rotations=["Left"],
            )
            flights[1].Proxy._updating = True
            flights[1].FlightType = "Circular"
            flights[1].InnerRadius = 500.0
            flights[1].OuterRadius = 1400.0
            flights[1].Angle = 90.0
            flights[1].LeftHandrailEnabled = True
            flights[1].RightHandrailEnabled = True
            flights[1].Proxy._updating = False
        finally:
            stair.Proxy._updating = False
        stair.HandrailPicketMaximumSpacing = 100.0
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        circular_rails = [
            part
            for part in stair.HandrailsGroup.Group
            if part.StairDesignerRole == "HandrailTopRail" and part.FlightIndex == 2
        ]
        self.assertEqual(len(circular_rails), 2)
        for rail in circular_rails:
            cylindrical_radii = {
                round(face.Surface.Radius, 5)
                for face in rail.Shape.Faces
                if isinstance(face.Surface, Part.Cylinder)
            }
            self.assertEqual(len(cylindrical_radii), 2)
        corner_posts = [
            part for part in stair.HandrailsGroup.Group if part.StairDesignerRole == "HandrailPost"
        ]
        post_positions = {
            (
                round(
                    (post.Shape.BoundBox.XMin + post.Shape.BoundBox.XMax) / 2.0,
                    5,
                ),
                round(
                    (post.Shape.BoundBox.YMin + post.Shape.BoundBox.YMax) / 2.0,
                    5,
                ),
            )
            for post in corner_posts
        }
        self.assertEqual(len(post_positions), len(corner_posts))

        stair.Proxy._updating = True
        try:
            stair.HandrailTopRailShape = "Circular"
            stair.HandrailPicketShape = "Circular"
            stair.HandrailPostShape = "Circular"
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        self.assertTrue(
            all(
                part.Shape.isValid() and len(part.Shape.Solids) == 1
                for part in stair.HandrailsGroup.Group
            )
        )
        for role in ("HandrailPost", "HandrailPicket"):
            self.assertTrue(
                all(
                    any(isinstance(face.Surface, Part.Cylinder) for face in part.Shape.Faces)
                    for part in stair.HandrailsGroup.Group
                    if part.StairDesignerRole == role
                )
            )

        stair.StairType = "Concrete"
        self.document.recompute()
        self.assertIsNone(stair.StringersGroup)
        self.assertIsNotNone(stair.HandrailsGroup)
        self.assertTrue(
            all(
                part.Shape.isValid()
                and len(part.Shape.Solids) == 1
                and part.Shape.BoundBox.ZMin >= -1e-7
                for part in stair.HandrailsGroup.Group
            )
        )

        for flight in flights:
            flight.Proxy._updating = True
            try:
                flight.LeftHandrailEnabled = False
                flight.RightHandrailEnabled = False
            finally:
                flight.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        self.assertIsNone(stair.HandrailsGroup)

    def test_concrete_balanced_pickets_meet_their_local_treads(self):
        stair = make_stair(
            floor_height=2800.0,
            flight_length=2600.0,
            width=900.0,
            steps=16,
            stair_type="Concrete",
        )
        stair.Proxy._updating = True
        try:
            flights = resize_flights(
                stair,
                2,
                length=2600.0,
                width=900.0,
                rotations=["Left"],
            )
            for flight in flights:
                flight.Proxy._updating = True
                try:
                    flight.LeftHandrailEnabled = True
                    flight.RightHandrailEnabled = True
                finally:
                    flight.Proxy._updating = False
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        concrete = stair.ConcreteGeometry.Shape
        pickets = [
            part
            for part in stair.HandrailsGroup.Group
            if part.StairDesignerRole == "HandrailPicket"
        ]
        self.assertGreater(len(pickets), 4)
        for picket in pickets:
            self.assertLess(
                picket.Shape.distToShape(concrete)[0],
                1e-5,
                picket.Label,
            )
            self.assertLess(
                picket.Shape.common(concrete).Volume,
                1e-5,
                picket.Label,
            )

    def test_single_flight_handrail_and_stringer_end_conditions(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=2400.0,
            width=900.0,
            steps=10,
            stair_type="Wood",
        )
        self.assertAlmostEqual(stair.StringerStartExtension.Value, 30.0)
        self.assertAlmostEqual(stair.StringerEndExtension.Value, 0.0)
        self.assertAlmostEqual(stair.HandrailPostAboveTopRail.Value, 70.0)

        flight = get_flights(stair)[0]
        flight.Proxy._updating = True
        try:
            flight.LeftStringerType = "Housed stringer"
            flight.LeftHandrailEnabled = True
        finally:
            flight.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        stringer = stair.StringersGroup.Group[0]
        self.assertGreaterEqual(stringer.Shape.BoundBox.ZMin, -1e-7)
        posts = sorted(
            (
                part
                for part in stair.HandrailsGroup.Group
                if part.StairDesignerRole == "HandrailPost"
            ),
            key=lambda part: part.ElementIndex,
        )
        top_rail = next(
            part
            for part in stair.HandrailsGroup.Group
            if part.StairDesignerRole == "HandrailTopRail"
        )
        post_centers = [
            (post.Shape.BoundBox.XMin + post.Shape.BoundBox.XMax) / 2.0 for post in posts
        ]
        # The housed board runs 30 mm past the first nosing by default, and
        # the first post stands on its front end.
        front = -(stair.Nosing.Value + 30.0)
        self.assertAlmostEqual(post_centers[0], front)
        self.assertAlmostEqual(post_centers[1], 2400.0)
        # A 35 mm penetration into a 70 mm post ends at its center.
        self.assertAlmostEqual(top_rail.Shape.BoundBox.XMin, front)
        self.assertAlmostEqual(top_rail.Shape.BoundBox.XMax, 2400.0)

        stair.Proxy._updating = True
        try:
            stair.StringerStartExtension = 100.0
            stair.StringerEndExtension = 100.0
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        self.assertGreaterEqual(stringer.Shape.BoundBox.ZMin, -1e-7)
        posts = sorted(
            (
                part
                for part in stair.HandrailsGroup.Group
                if part.StairDesignerRole == "HandrailPost"
            ),
            key=lambda part: part.ElementIndex,
        )
        post_centers = [
            (post.Shape.BoundBox.XMin + post.Shape.BoundBox.XMax) / 2.0 for post in posts
        ]
        # The extended board is trimmed at the end posts and tenoned into
        # them: its ends stop inside the posts rather than at their centers.
        first_post = posts[0].Shape.BoundBox
        last_post = posts[1].Shape.BoundBox
        self.assertGreater(stringer.Shape.BoundBox.XMin, first_post.XMin)
        self.assertLess(stringer.Shape.BoundBox.XMin, -100.0)
        self.assertLess(stringer.Shape.BoundBox.XMax, last_post.XMax)
        self.assertGreater(stringer.Shape.BoundBox.XMax, 2500.0)
        self.assertLess(stringer.Shape.common(posts[0].Shape).Volume, 1e-3)
        self.assertLess(stringer.Shape.common(posts[1].Shape).Volume, 1e-3)
        # The start extension counts from the first nosing.
        front = -(stair.Nosing.Value + 100.0)
        self.assertAlmostEqual(post_centers[0], front)
        self.assertAlmostEqual(post_centers[1], 2500.0)
        self.assertAlmostEqual(top_rail.Shape.BoundBox.XMin, front)
        self.assertAlmostEqual(top_rail.Shape.BoundBox.XMax, 2500.0)

        stair.StairType = "Concrete"
        self.document.recompute()
        posts = sorted(
            (
                part
                for part in stair.HandrailsGroup.Group
                if part.StairDesignerRole == "HandrailPost"
            ),
            key=lambda part: part.ElementIndex,
        )
        top_post = posts[-1]
        self.assertAlmostEqual(top_post.Shape.BoundBox.ZMin, 1620.0)
        self.assertLess(
            top_post.Shape.distToShape(stair.ConcreteGeometry.Shape)[0],
            1e-7,
        )

    def test_housed_stringer_profile_does_not_loop_on_backtracking_sections(
        self,
    ):
        stations = (0.0, 850.0, 250.0, 1200.0, 1600.0)
        sections = [
            BalancedSection(
                center=(station, 450.0),
                tangent=(1.0, 0.0),
                left=(station, 900.0),
                right=(station, 0.0),
                station=float(index) * 400.0,
                width=900.0,
                flight_index=0,
                riser_index=index + 1,
            )
            for index, station in enumerate(stations)
        ]
        shape = make_housed_stringer_shape(
            sections,
            180.0,
            "Left",
            40.0,
            300.0,
            20.0,
            0.0,
            0.0,
            50.0,
            "Perpendicular",
            30.0,
        )

        self.assertFalse(shape.isNull())
        self.assertTrue(shape.isValid())
        self.assertEqual(len(shape.Solids), 1)
        # The edges are straight pitches or fair curves running evenly
        # along the board, never looping back over it.
        for edge in shape.Edges:
            self.assertIn(edge.Curve.TypeId, ("Part::GeomLine", "Part::GeomBSplineCurve"))
            along = [point.x for point in edge.discretize(Number=50)]
            steps = [second - first for first, second in zip(along, along[1:])]
            self.assertTrue(
                all(step >= -1e-9 for step in steps) or all(step <= 1e-9 for step in steps)
            )
        self.assertGreaterEqual(shape.BoundBox.XMin, -1e-7)
        self.assertLessEqual(shape.BoundBox.XMax, 1600.0 + 1e-7)
        self.assertGreaterEqual(shape.BoundBox.ZMin, -1e-7)

    def test_balanced_corner_posts_align_with_both_stringers(
        self,
    ):
        stair = make_stair(
            floor_height=2800.0,
            flight_length=3500.0,
            width=1000.0,
            steps=15,
            stair_type="Wood",
        )
        stair.Proxy._updating = True
        try:
            flights = resize_flights(
                stair,
                2,
                length=3500.0,
                width=1000.0,
                rotations=["Left"],
            )
            for flight in flights:
                flight.Proxy._updating = True
                for side in ("Left", "Right"):
                    setattr(
                        flight,
                        f"{side}StringerType",
                        "Housed stringer",
                    )
                    setattr(
                        flight,
                        f"{side}HandrailEnabled",
                        True,
                    )
                flight.Proxy._updating = False
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        for side in ("Left", "Right"):
            incoming_stringer = next(
                part
                for part in stair.StringersGroup.Group
                if part.SourceFlight == flights[0] and part.Side == side
            )
            outgoing_stringer = next(
                part
                for part in stair.StringersGroup.Group
                if part.SourceFlight == flights[1] and part.Side == side
            )
            corner_post = next(
                part
                for part in stair.HandrailsGroup.Group
                if part.StairDesignerRole == "HandrailPost"
                and part.SourceFlight == flights[0]
                and part.Side == side
                and part.ElementIndex == 1
            )
            bounds = corner_post.Shape.BoundBox
            distances = [
                corner_post.Shape.distToShape(stringer.Shape)[0]
                for stringer in (
                    incoming_stringer,
                    outgoing_stringer,
                )
            ]
            self.assertTrue(
                all(distance < 1e-7 for distance in distances),
                side,
            )
            self.assertLessEqual(
                bounds.ZMin,
                min(
                    incoming_stringer.Shape.BoundBox.ZMax,
                    outgoing_stringer.Shape.BoundBox.ZMax,
                )
                - stair.HandrailPostBelowStringer.Value,
            )

    def test_concrete_quarter_turn_is_monolithic(self):
        stair = make_stair(
            floor_height=1200.0,
            flight_length=1200.0,
            width=700.0,
            steps=7,
            stair_type="Concrete",
        )
        stair.Proxy._updating = True
        try:
            flights = resize_flights(stair, 2, length=1200.0, width=700.0, rotations=["Left"])
            flights[0].Angle = 0.0
            flights[1].Angle = 90.0
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        shape = stair.ConcreteGeometry.Shape
        self.assertFalse(shape.isNull())
        self.assertTrue(shape.isValid())
        self.assertEqual(len(shape.Solids), 1)
        self.assertEqual(sum(flight.NumberOfTreads for flight in get_flights(stair)), 6)

        flights[1].FlightType = "Circular"
        self.document.recompute()
        shape = stair.ConcreteGeometry.Shape
        self.assertEqual(stair.GeometryStatus, "Tangential circular stair")
        self.assertFalse(shape.isNull())
        self.assertTrue(shape.isValid())
        self.assertEqual(len(shape.Solids), 1)
        self.assertLess(len(shape.Faces), 80)

        flights[1].Angle = 60.0
        flights[1].Rotation = "Right"
        self.document.recompute()
        shape = stair.ConcreteGeometry.Shape
        self.assertTrue(shape.isValid())
        self.assertEqual(len(shape.Solids), 1)
        self.assertLess(len(shape.Faces), 80)

    def test_make_wood_stair(self):
        stair = make_stair(
            floor_height=2800.0,
            flight_length=3500.0,
            width=1000.0,
            steps=15,
            stair_type="Wood",
            name="Test Stair Designer",
        )
        self.document.recompute()

        self.assertIsNotNone(stair)
        self.assertEqual(stair.Label, "Test Stair Designer")
        self.assertTrue(stair.Name.startswith("Stair"))
        self.assertEqual(stair.Proxy.Type, "StairDesigner")
        self.assertTrue(stair.EndWithRiser)
        self.assertNotIn("Model", stair.PropertiesList)
        self.assertIsNotNone(stair.PlanSketch)
        self.assertEqual(stair.PlanSketch.GeometryCount, 18)
        self.assertIsNotNone(stair.FlightsGroup)
        self.assertIsNotNone(stair.StepsGroup)
        self.assertIsNone(stair.StringersGroup)
        self.assertIsNone(stair.HandrailsGroup)
        self.assertNotIn("LinksGroup", stair.PropertiesList)
        self.assertEqual(str(get_flights(stair)[0].TurnType), "Herse balancing")

        treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        ]
        risers = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Riser"
        ]
        self.assertEqual(len(treads), 14)
        self.assertEqual(len(risers), 15)
        self.assertTrue(all(not child.Shape.isNull() for child in treads + risers))
        self.assertTrue(stair.BlondelCompliant)

        first_riser = sorted(risers, key=lambda child: child.Index)[0]
        riser_box = first_riser.Shape.BoundBox
        self.assertAlmostEqual(riser_box.XMin, 0.0)
        self.assertAlmostEqual(riser_box.XMax, stair.RiserThickness.Value)
        self.assertAlmostEqual(
            riser_box.ZMax,
            stair.RiserHeight.Value - stair.StepThickness.Value,
        )

        ordered_treads = sorted(treads, key=lambda child: child.Index)
        ordered_risers = sorted(risers, key=lambda child: child.Index)
        terminal_riser = ordered_risers[-1]
        self.assertAlmostEqual(terminal_riser.Shape.BoundBox.XMin, 3500.0)
        self.assertAlmostEqual(
            terminal_riser.Shape.BoundBox.ZMax,
            stair.FloorHeight.Value,
        )
        self.assertAlmostEqual(
            ordered_treads[0].Shape.BoundBox.XMax,
            ordered_risers[1].Shape.BoundBox.XMax + stair.StepRiserOverlap.Value,
        )
        self.assertAlmostEqual(
            ordered_treads[-1].Shape.BoundBox.XMax,
            3500.0 + stair.RiserThickness.Value + stair.StepRiserOverlap.Value,
        )
        self.assertTrue(
            all(
                abs(tread.Shape.BoundBox.XLength - ordered_treads[0].Shape.BoundBox.XLength) < 1e-7
                for tread in ordered_treads
            )
        )

        stair.PriorityToRiser = True
        self.document.recompute()
        lower_tread_box = ordered_treads[0].Shape.BoundBox
        next_riser_box = ordered_risers[1].Shape.BoundBox
        self.assertAlmostEqual(
            lower_tread_box.XMax,
            next_riser_box.XMin + stair.StepRiserOverlap.Value,
        )
        self.assertAlmostEqual(lower_tread_box.ZMin, next_riser_box.ZMin)

        stair.Proxy._updating = True
        try:
            stair.Nosing = 0.0
            stair.RiserUpperOffset = -10.0
            stair.RiserLowerOffset = -5.0
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        riser_box = first_riser.Shape.BoundBox
        self.assertAlmostEqual(riser_box.XMin, 0.0)
        self.assertAlmostEqual(riser_box.ZMin, -5.0)
        self.assertAlmostEqual(
            riser_box.ZMax,
            stair.RiserHeight.Value - stair.StepThickness.Value + 10.0,
        )

        stair.NumberOfSteps = 16
        self.document.recompute()
        treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        ]
        self.assertEqual(len(treads), 15)

    def test_stair_placement_moves_generated_hierarchy(self):
        stair = make_stair(stair_type="Wood")
        self.document.recompute()

        self.assertEqual(stair.TypeId, "Part::FeaturePython")
        self.assertTrue(stair.hasExtension("App::GeoFeatureGroupExtensionPython"))
        self.assertIn("Placement", stair.PropertiesList)

        tread = next(
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        )
        local_placement = tread.Placement
        stair_placement = FreeCAD.Placement(
            FreeCAD.Vector(120.0, -80.0, 30.0),
            FreeCAD.Rotation(FreeCAD.Vector(0.0, 0.0, 1.0), 35.0),
        )

        stair.Placement = stair_placement
        self.document.recompute()

        self.assertTrue(tread.Placement.isSame(local_placement))
        self.assertTrue(
            tread.getGlobalPlacement().isSame(stair_placement.multiply(local_placement))
        )

    def test_end_with_riser_controls_terminal_riser(self):
        wood = make_stair(
            floor_height=2800.0,
            flight_length=3500.0,
            width=1000.0,
            steps=15,
            stair_type="Wood",
        )
        self.document.recompute()

        wood.EndWithRiser = False
        self.document.recompute()
        treads = sorted(
            (
                child
                for child in wood.StepsGroup.Group
                if getattr(child, "StairDesignerRole", "") == "Tread"
            ),
            key=lambda child: child.Index,
        )
        risers = sorted(
            (
                child
                for child in wood.StepsGroup.Group
                if getattr(child, "StairDesignerRole", "") == "Riser"
            ),
            key=lambda child: child.Index,
        )
        self.assertEqual(len(treads), 15)
        self.assertEqual(len(risers), 15)
        self.assertTrue(
            all(
                abs(tread.Shape.BoundBox.XLength - treads[0].Shape.BoundBox.XLength) < 1e-7
                for tread in treads
            )
        )
        self.assertAlmostEqual(
            treads[-1].Shape.BoundBox.ZMax,
            wood.FloorHeight.Value,
        )
        self.assertLess(risers[-1].Shape.BoundBox.XMin, 3500.0)

        concrete = make_stair(
            floor_height=2800.0,
            flight_length=3500.0,
            width=1000.0,
            steps=15,
            stair_type="Concrete",
        )
        self.document.recompute()
        self.assertAlmostEqual(
            concrete.ConcreteGeometry.Shape.BoundBox.ZMax,
            concrete.FloorHeight.Value - concrete.RiserHeight.Value,
        )

        concrete.EndWithRiser = False
        self.document.recompute()
        self.assertEqual(concrete.NumberOfTreads, 15)
        self.assertAlmostEqual(
            concrete.ConcreteGeometry.Shape.BoundBox.ZMax,
            concrete.FloorHeight.Value,
        )

    def test_concrete_end_cut_distances_extend_only_the_structure(self):
        stair = make_stair(
            floor_height=2800.0,
            flight_length=3500.0,
            width=1000.0,
            steps=15,
            stair_type="Concrete",
        )
        self.document.recompute()
        dimensions = (
            stair.FloorHeight.Value,
            stair.RiserHeight.Value,
            stair.TreadWidth.Value,
            stair.NumberOfTreads,
        )
        baseline = stair.ConcreteGeometry.Shape
        self.assertAlmostEqual(stair.BottomCutDistance.Value, 0.0)
        self.assertAlmostEqual(stair.TopCutDistance.Value, 0.0)
        self.assertAlmostEqual(baseline.BoundBox.ZMin, 0.0)
        self.assertAlmostEqual(baseline.BoundBox.XMax, 3500.0)

        stair.BottomCutDistance = 100.0
        self.document.recompute()
        bottom_cut = stair.ConcreteGeometry.Shape
        self.assertTrue(bottom_cut.isValid())
        self.assertEqual(len(bottom_cut.Solids), 1)
        self.assertAlmostEqual(bottom_cut.BoundBox.ZMin, -100.0)
        self.assertGreater(bottom_cut.Volume, baseline.Volume)

        stair.BottomCutDistance = 1000000.0
        self.document.recompute()
        full_bottom = stair.ConcreteGeometry.Shape
        self.assertTrue(full_bottom.isValid())
        self.assertGreater(full_bottom.BoundBox.ZMin, -1000000.0)
        self.assertLess(full_bottom.BoundBox.ZMin, -100.0)

        stair.BottomCutDistance = 0.0
        stair.TopCutDistance = 100.0
        self.document.recompute()
        top_cut = stair.ConcreteGeometry.Shape
        self.assertTrue(top_cut.isValid())
        self.assertEqual(len(top_cut.Solids), 1)
        self.assertAlmostEqual(top_cut.BoundBox.XMax, 3600.0)
        self.assertAlmostEqual(
            top_cut.BoundBox.ZMax,
            baseline.BoundBox.ZMax,
        )
        vertical_cut_edges = [
            edge
            for edge in top_cut.Edges
            if len(edge.Vertexes) == 2
            and all(abs(vertex.Point.x - 3600.0) < 1e-6 for vertex in edge.Vertexes)
            and abs(edge.Vertexes[0].Point.z - edge.Vertexes[1].Point.z) > 1e-6
        ]
        self.assertTrue(vertical_cut_edges)
        self.assertGreater(top_cut.Volume, baseline.Volume)

        stair.TopCutDistance = 1000000.0
        self.document.recompute()
        full_top = stair.ConcreteGeometry.Shape
        self.assertTrue(full_top.isValid())
        self.assertEqual(len(full_top.Solids), 1)
        self.assertGreater(full_top.BoundBox.XMax, top_cut.BoundBox.XMax)
        self.assertEqual(
            (
                stair.FloorHeight.Value,
                stair.RiserHeight.Value,
                stair.TreadWidth.Value,
                stair.NumberOfTreads,
            ),
            dimensions,
        )

        balanced = make_stair(
            floor_height=1800.0,
            flight_length=1600.0,
            width=800.0,
            steps=10,
            stair_type="Concrete",
        )
        balanced.Proxy._updating = True
        try:
            flights = resize_flights(
                balanced,
                2,
                length=1600.0,
                width=800.0,
                rotations=["Left"],
            )
            flights[0].Angle = 0.0
            flights[1].Angle = 90.0
            balanced.BottomCutDistance = 100.0
            balanced.TopCutDistance = 100.0
        finally:
            balanced.Proxy._updating = False
        balanced.Proxy.rebuild(
            balanced,
            allow_structure_changes=True,
        )
        self.document.recompute()
        balanced_shape = balanced.ConcreteGeometry.Shape
        self.assertTrue(balanced_shape.isValid())
        self.assertEqual(len(balanced_shape.Solids), 1)
        self.assertAlmostEqual(balanced_shape.BoundBox.ZMin, -100.0)

        flights[1].FlightType = "Circular"
        self.document.recompute()
        circular_shape = balanced.ConcreteGeometry.Shape
        self.assertTrue(circular_shape.isValid())
        self.assertEqual(len(circular_shape.Solids), 1)
        self.assertAlmostEqual(circular_shape.BoundBox.ZMin, -100.0)

    def test_concrete_stair_can_have_finish_steps_and_risers(self):
        stair = make_stair(
            floor_height=2800.0,
            flight_length=3500.0,
            width=1000.0,
            steps=15,
            stair_type="Concrete",
        )
        self.document.recompute()
        structural_top = stair.ConcreteGeometry.Shape.BoundBox.ZMax
        self.assertFalse(stair.StepsEnabled)
        self.assertAlmostEqual(stair.StructureWidthOffset.Value, 0.0)
        self.assertIsNone(stair.StepsGroup)
        self.assertIn("Hidden", stair.getEditorMode("StepRiserOverlap"))

        stair.StepsEnabled = True
        self.document.recompute()
        treads = sorted(
            (
                child
                for child in stair.StepsGroup.Group
                if getattr(child, "StairDesignerRole", "") == "Tread"
            ),
            key=lambda child: child.Index,
        )
        risers = sorted(
            (
                child
                for child in stair.StepsGroup.Group
                if getattr(child, "StairDesignerRole", "") == "Riser"
            ),
            key=lambda child: child.Index,
        )
        self.assertEqual(len(treads), stair.NumberOfTreads)
        self.assertEqual(len(risers), stair.NumberOfTreads + 1)
        self.assertAlmostEqual(stair.StepRiserOverlap.Value, 0.0)
        self.assertAlmostEqual(
            stair.ConcreteGeometry.Shape.BoundBox.ZMax + stair.StepThickness.Value,
            structural_top,
        )
        self.assertAlmostEqual(treads[-1].Shape.BoundBox.ZMax, structural_top)
        self.assertAlmostEqual(risers[-1].Shape.BoundBox.ZMax, 2800.0)
        self.assertAlmostEqual(
            risers[-1].Shape.BoundBox.ZMin,
            treads[-1].Shape.BoundBox.ZMax,
        )
        self.assertAlmostEqual(risers[0].Shape.BoundBox.XMin, -18.0)
        self.assertAlmostEqual(risers[0].Shape.BoundBox.XMax, 0.0)
        self.assertAlmostEqual(
            risers[0].Shape.BoundBox.ZMax,
            stair.RiserHeight.Value - stair.StepThickness.Value,
        )
        self.assertTrue(all(tread.Shape.isValid() for tread in treads))
        self.assertTrue(all(riser.Shape.isValid() for riser in risers))

        full_width_volume = stair.ConcreteGeometry.Shape.Volume
        stair.StructureWidthOffset = 30.0
        self.document.recompute()
        self.assertAlmostEqual(
            stair.ConcreteGeometry.Shape.BoundBox.YMin,
            30.0,
        )
        self.assertAlmostEqual(
            stair.ConcreteGeometry.Shape.BoundBox.YMax,
            970.0,
        )
        self.assertLess(
            stair.ConcreteGeometry.Shape.Volume,
            full_width_volume,
        )
        self.assertAlmostEqual(treads[0].Shape.BoundBox.YMin, 0.0)
        self.assertAlmostEqual(treads[0].Shape.BoundBox.YMax, 1000.0)

        stair.PriorityToRiser = True
        self.document.recompute()
        treads = sorted(
            (
                child
                for child in stair.StepsGroup.Group
                if getattr(child, "StairDesignerRole", "") == "Tread"
            ),
            key=lambda child: child.Index,
        )
        risers = sorted(
            (
                child
                for child in stair.StepsGroup.Group
                if getattr(child, "StairDesignerRole", "") == "Riser"
            ),
            key=lambda child: child.Index,
        )
        self.assertAlmostEqual(
            treads[0].Shape.BoundBox.XMax,
            stair.TreadWidth.Value - stair.RiserThickness.Value,
        )
        self.assertAlmostEqual(
            risers[1].Shape.BoundBox.ZMin,
            stair.RiserHeight.Value - stair.StepThickness.Value,
        )
        self.assertAlmostEqual(
            risers[1].Shape.BoundBox.ZMax,
            2.0 * stair.RiserHeight.Value - stair.StepThickness.Value,
        )
        self.assertAlmostEqual(
            risers[-1].Shape.BoundBox.ZMin,
            treads[-1].Shape.BoundBox.ZMin,
        )

        stair.PriorityToRiser = False
        stair.EndWithRiser = False
        self.document.recompute()
        treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        ]
        risers = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Riser"
        ]
        self.assertEqual(len(treads), 15)
        self.assertEqual(len(risers), 15)
        self.assertAlmostEqual(treads[-1].Shape.BoundBox.ZMax, 2800.0)
        self.assertAlmostEqual(
            stair.ConcreteGeometry.Shape.BoundBox.ZMax,
            2800.0 - stair.StepThickness.Value,
        )

        stair.RisersEnabled = False
        self.document.recompute()
        self.assertFalse(
            any(
                getattr(child, "StairDesignerRole", "") == "Riser"
                for child in stair.StepsGroup.Group
            )
        )
        self.assertEqual(
            sum(
                getattr(child, "StairDesignerRole", "") == "Tread"
                for child in stair.StepsGroup.Group
            ),
            15,
        )

        stair.StepsEnabled = False
        self.document.recompute()
        self.assertIsNone(stair.StepsGroup)
        self.assertAlmostEqual(
            stair.ConcreteGeometry.Shape.BoundBox.ZMax,
            2800.0,
        )
        self.assertAlmostEqual(
            stair.ConcreteGeometry.Shape.BoundBox.YLength,
            1000.0,
        )

        balanced = make_stair(
            floor_height=1800.0,
            flight_length=1600.0,
            width=800.0,
            steps=10,
            stair_type="Concrete",
        )
        balanced.Proxy._updating = True
        try:
            flights = resize_flights(
                balanced,
                2,
                length=1600.0,
                width=800.0,
                rotations=["Left"],
            )
            flights[0].Angle = 0.0
            flights[1].Angle = 90.0
            balanced.StepsEnabled = True
        finally:
            balanced.Proxy._updating = False
        balanced.Proxy.rebuild(
            balanced,
            allow_structure_changes=True,
        )
        self.document.recompute()
        self.assertTrue(balanced.ConcreteGeometry.Shape.isValid())
        self.assertEqual(len(balanced.ConcreteGeometry.Shape.Solids), 1)
        self.assertTrue(
            all(
                child.Shape.isValid() and len(child.Shape.Solids) == 1
                for child in balanced.StepsGroup.Group
                if hasattr(child, "Shape")
            )
        )

        balanced_full_width_volume = balanced.ConcreteGeometry.Shape.Volume
        balanced_tread_bounds = [
            (
                child.Shape.BoundBox.XMin,
                child.Shape.BoundBox.XMax,
                child.Shape.BoundBox.YMin,
                child.Shape.BoundBox.YMax,
            )
            for child in balanced.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        ]
        balanced.StructureWidthOffset = 30.0
        self.document.recompute()
        self.assertTrue(balanced.ConcreteGeometry.Shape.isValid())
        self.assertEqual(len(balanced.ConcreteGeometry.Shape.Solids), 1)
        self.assertLess(
            balanced.ConcreteGeometry.Shape.Volume,
            balanced_full_width_volume,
        )
        self.assertEqual(
            [
                (
                    child.Shape.BoundBox.XMin,
                    child.Shape.BoundBox.XMax,
                    child.Shape.BoundBox.YMin,
                    child.Shape.BoundBox.YMax,
                )
                for child in balanced.StepsGroup.Group
                if getattr(child, "StairDesignerRole", "") == "Tread"
            ],
            balanced_tread_bounds,
        )
        flights[1].FlightType = "Circular"
        self.document.recompute()
        self.assertTrue(balanced.ConcreteGeometry.Shape.isValid())
        self.assertEqual(len(balanced.ConcreteGeometry.Shape.Solids), 1)
        circular_side_radii = sorted(
            round(face.Surface.Radius, 3)
            for face in balanced.ConcreteGeometry.Shape.Faces
            if isinstance(face.Surface, Part.Cylinder)
        )
        self.assertIn(
            round(flights[1].InnerRadius.Value + 30.0, 3),
            circular_side_radii,
        )
        self.assertIn(
            round(flights[1].OuterRadius.Value - 30.0, 3),
            circular_side_radii,
        )
        self.assertTrue(
            all(
                child.Shape.isValid() and len(child.Shape.Solids) == 1
                for child in balanced.StepsGroup.Group
                if hasattr(child, "Shape")
            )
        )

    def test_structure_width_offset_applies_to_single_circular_flights(self):
        for rotation in ("Left", "Right"):
            stair = make_stair(
                floor_height=2800.0,
                flight_length=3500.0,
                width=1000.0,
                steps=15,
                stair_type="Concrete",
            )
            flight = get_flights(stair)[0]
            stair.Proxy._updating = True
            try:
                flight.FlightType = "Circular"
                flight.InnerRadius = 500.0
                flight.Angle = 90.0
                flight.Rotation = rotation
                stair.StepsEnabled = True
                stair.StructureWidthOffset = 30.0
            finally:
                stair.Proxy._updating = False
            stair.Proxy.rebuild(stair, allow_structure_changes=True)
            self.document.recompute()

            radii = sorted(
                round(face.Surface.Radius, 3)
                for face in stair.ConcreteGeometry.Shape.Faces
                if isinstance(face.Surface, Part.Cylinder)
            )
            self.assertIn(round(flight.InnerRadius.Value + 30.0, 3), radii)
            self.assertIn(round(flight.OuterRadius.Value - 30.0, 3), radii)
            for cut_distance in (89.0, 90.0, 1000000.0):
                stair.BottomCutDistance = cut_distance
                self.document.recompute()
                self.assertTrue(stair.ConcreteGeometry.Shape.isValid())
                self.assertEqual(
                    len(stair.ConcreteGeometry.Shape.Solids),
                    1,
                    (rotation, cut_distance),
                )
            stair.BottomCutDistance = 0.0
            for cut_distance in (89.0, 90.0, 1000000.0):
                stair.TopCutDistance = cut_distance
                self.document.recompute()
                self.assertTrue(stair.ConcreteGeometry.Shape.isValid())
                self.assertEqual(
                    len(stair.ConcreteGeometry.Shape.Solids),
                    1,
                    (rotation, cut_distance),
                )

    def test_concrete_stair_is_monolithic(self):
        stair = make_stair(stair_type="Concrete")
        self.document.recompute()

        self.assertEqual(stair.Label, "Stair")
        self.assertIsNone(stair.StepsGroup)
        self.assertIsNone(stair.StringersGroup)
        self.assertIsNone(stair.HandrailsGroup)
        self.assertIsNotNone(stair.ConcreteGeometry)
        self.assertFalse(stair.ConcreteGeometry.Shape.isNull())
        self.assertEqual(len(stair.ConcreteGeometry.Shape.Solids), 1)

        shape = stair.ConcreteGeometry.Shape
        self.assertTrue(shape.isValid())
        self.assertAlmostEqual(shape.BoundBox.ZMin, 0.0)
        self.assertAlmostEqual(
            shape.BoundBox.ZMax,
            stair.NumberOfTreads * stair.RiserHeight.Value,
        )
        sloped_edges = []
        for edge in shape.Edges:
            if len(edge.Vertexes) != 2:
                continue
            first = edge.Vertexes[0].Point
            last = edge.Vertexes[1].Point
            delta_x = abs(last.x - first.x)
            delta_z = abs(last.z - first.z)
            if delta_x > 1.0 and delta_z > 1.0 and edge.BoundBox.YLength < 0.01:
                sloped_edges.append((first, last))
        self.assertEqual(len(sloped_edges), 2)
        for first, last in sloped_edges:
            self.assertAlmostEqual(
                abs(last.z - first.z) / abs(last.x - first.x),
                stair.RiserHeight.Value / stair.TreadWidth.Value,
            )
            self.assertAlmostEqual(min(first.x, last.x), stair.TreadWidth.Value)
        enclosing_volume = shape.BoundBox.XLength * shape.BoundBox.YLength * shape.BoundBox.ZLength
        self.assertLess(shape.Volume, enclosing_volume * 0.7)
        initial_volume = shape.Volume

        stair.ConcreteThickness = 0.0
        self.document.recompute()
        minimum_shape = stair.ConcreteGeometry.Shape
        self.assertTrue(minimum_shape.isValid())
        minimum_slopes = []
        for edge in minimum_shape.Edges:
            if len(edge.Vertexes) != 2:
                continue
            first = edge.Vertexes[0].Point
            last = edge.Vertexes[1].Point
            if (
                abs(last.x - first.x) > 1.0
                and abs(last.z - first.z) > 1.0
                and edge.BoundBox.YLength < 0.01
            ):
                minimum_slopes.append((first, last))
        self.assertEqual(len(minimum_slopes), 2)
        for first, last in minimum_slopes:
            self.assertAlmostEqual(min(first.x, last.x), 0.0, delta=0.1)

        stair.ConcreteThickness = stair.RiserHeight.Value * (
            stair.TreadWidth.Value / (stair.TreadWidth.Value**2 + stair.RiserHeight.Value**2) ** 0.5
        )
        self.document.recompute()
        self.assertAlmostEqual(stair.ConcreteGeometry.Shape.Volume, initial_volume)

        stair.ConcreteThickness = stair.ConcreteThickness.Value + 60.0
        self.document.recompute()
        self.assertAlmostEqual(stair.ConcreteGeometry.Shape.BoundBox.ZMin, 0.0)
        self.assertGreater(stair.ConcreteGeometry.Shape.Volume, initial_volume)

        stair.ConcreteThickness = stair.ConcreteThickness.Value - 120.0
        self.document.recompute()
        thinner_shape = stair.ConcreteGeometry.Shape
        self.assertTrue(thinner_shape.isValid())
        self.assertAlmostEqual(thinner_shape.BoundBox.ZMin, 0.0)
        thinner_slopes = []
        for edge in thinner_shape.Edges:
            if len(edge.Vertexes) != 2:
                continue
            first = edge.Vertexes[0].Point
            last = edge.Vertexes[1].Point
            if (
                abs(last.x - first.x) > 1.0
                and abs(last.z - first.z) > 1.0
                and edge.BoundBox.YLength < 0.01
            ):
                thinner_slopes.append((first, last))
        self.assertEqual(len(thinner_slopes), 2)
        for first, last in thinner_slopes:
            self.assertLess(min(first.x, last.x), stair.TreadWidth.Value)
            self.assertAlmostEqual(
                abs(last.z - first.z) / abs(last.x - first.x),
                stair.RiserHeight.Value / stair.TreadWidth.Value,
            )

    def test_abort_transaction_removes_generated_tree(self):
        self.document.UndoMode = 1
        self.document.openTransaction("Create Stair Designer")
        stair = make_stair(
            floor_height=600.0,
            flight_length=800.0,
            width=700.0,
            steps=3,
        )
        resize_flights(stair, 2, length=800.0, width=700.0, rotations=["Right"])
        object_names = [obj.Name for obj in self.document.Objects]
        self.assertNotEqual(self.document.getBookedTransactionID(), 0)

        self.document.abortTransaction()
        self.document.recompute()

        for name in object_names:
            self.assertIsNone(self.document.getObject(name))
        self.assertEqual(self.document.Objects, [])
        self.assertEqual(self.document.getUniqueObjectName("Stair"), "Stair")

    def _joinery_stair(self, flights=1):
        stair = make_stair()
        stair.Proxy._updating = True
        stair.Nosing = 30
        stair.RisersEnabled = True
        stair.Proxy._updating = False
        if flights > 1:
            resize_flights(stair, flights, length=1800.0, width=1000.0)
        for flight in get_flights(stair):
            flight.Proxy._updating = True
            flight.LeftStringerType = "Housed stringer"
            flight.RightStringerType = "Housed stringer"
            flight.LeftHandrailEnabled = True
            flight.RightHandrailEnabled = True
            flight.Proxy._updating = False
        stair.Proxy.rebuild(stair)
        return stair

    def test_housed_stringers_house_steps_and_tenon_into_posts(self):
        stair = self._joinery_stair()
        stringers = list(stair.StringersGroup.Group)
        steps = [p for p in stair.StepsGroup.Group if p.StairDesignerRole in ("Tread", "Riser")]
        members = list(stair.HandrailsGroup.Group)
        posts = [p for p in members if p.StairDesignerRole == "HandrailPost"]
        pickets = [p for p in members if p.StairDesignerRole == "HandrailPicket"]
        self.assertEqual(len(stringers), 2)
        self.assertEqual(len(posts), 4)
        for stringer in stringers:
            shape = stringer.Shape
            shape.check(True)
            self.assertEqual(len(shape.Solids), 1)
            envelope = Part.makeBox(
                shape.BoundBox.XLength,
                shape.BoundBox.YLength,
                shape.BoundBox.ZLength,
                FreeCAD.Vector(shape.BoundBox.XMin, shape.BoundBox.YMin, shape.BoundBox.ZMin),
            )
            for part in steps + pickets + posts:
                # Nothing overlaps the board any more: every end is housed.
                self.assertLess(shape.common(part.Shape).Volume, 1e-3, part.Label)
            for step in steps:
                self.assertGreater(step.Shape.common(envelope).Volume, 1.0, step.Label)
            for picket in pickets:
                if picket.Side == stringer.Side:
                    self.assertGreater(picket.Shape.common(envelope).Volume, 1.0)
            for post in posts:
                if post.Side != stringer.Side:
                    continue
                bounds = post.Shape.BoundBox
                post_box = Part.makeBox(
                    bounds.XLength,
                    bounds.YLength,
                    bounds.ZLength,
                    FreeCAD.Vector(bounds.XMin, bounds.YMin, bounds.ZMin),
                )
                tenon = shape.common(post_box)
                mortise = post_box.cut(post.Shape)
                self.assertGreater(tenon.Volume, 1000.0)
                self.assertAlmostEqual(mortise.common(shape).Volume, tenon.Volume, delta=1.0)
                # The tenon is a stub: shoulders keep it inside the board.
                self.assertLess(tenon.BoundBox.YLength, stringer.Thickness.Value * 0.5)

    def test_housed_stringer_profile_is_square_and_level_cut(self):
        stair = self._joinery_stair()
        stringer = next(p for p in stair.StringersGroup.Group if p.Side == "Left")
        shape = stringer.Shape
        floor_faces = [
            face
            for face in shape.Faces
            if face.Surface.TypeId == "Part::GeomPlane"
            and abs(abs(face.normalAt(0, 0).z) - 1.0) < 1e-9
            and face.BoundBox.ZMax < 1e-6
        ]
        self.assertTrue(floor_faces, "The board foot must be a level floor cut")
        self.assertGreaterEqual(shape.BoundBox.ZMin, -1e-6)
        # The lower edge stays parallel to the pitch: no curl at the floor.
        spans = []
        for x in (600.0, 1500.0, 2400.0):
            probe = Part.makeLine(FreeCAD.Vector(x, 990.0, -10), FreeCAD.Vector(x, 990.0, 4000))
            spans.append(shape.common(probe).BoundBox)
        for span in spans[1:]:
            self.assertAlmostEqual(span.ZMin - spans[0].ZMin, span.ZMax - spans[0].ZMax, places=4)

    def test_winder_stringers_meet_without_overlap_or_hooks(self):
        stair = self._joinery_stair(flights=2)
        self.assertEqual(stair.GeometryStatus, "Balanced multi-flight stair")
        stringers = list(stair.StringersGroup.Group)
        self.assertEqual(len(stringers), 4)
        for stringer in stringers:
            stringer.Shape.check(True)
            self.assertEqual(len(stringer.Shape.Solids), 1, stringer.Label)
            # No profile hook at a newel: the board stays below the floor
            # height plus one board width.
            self.assertLess(
                stringer.Shape.BoundBox.ZMax,
                stair.FloorHeight.Value + stringer.Width.Value,
            )
            for other in stringers:
                if other is not stringer:
                    self.assertLess(stringer.Shape.common(other.Shape).Volume, 1e-3)
        outer = sorted((p for p in stringers if p.Side == "Right"), key=lambda p: p.FlightIndex)
        # The outer boards meet at the corner at one common height.
        corner = FreeCAD.Vector(outer[0].Shape.BoundBox.XMax, 0.0, 0.0)
        tops = []
        for board, offset in (
            (outer[0], FreeCAD.Vector(-60.0, 10.0, 0.0)),
            (outer[1], FreeCAD.Vector(-20.0, 60.0, 0.0)),
        ):
            point = corner + offset
            probe = Part.makeLine(point, point + FreeCAD.Vector(0.0, 0.0, 4000.0))
            tops.append(board.Shape.common(probe).BoundBox.ZMax)
        self.assertAlmostEqual(tops[0], tops[1], delta=60.0)

    def test_square_top_rail_keeps_its_thickness_on_any_pitch(self):
        from stairdesigner.geometry_handrails import make_handrail_top_rail_shape

        for rise in (0.0, 500.0, 1000.0):
            path = {
                "kind": "Linear",
                "start": (0.0, 0.0),
                "end": (1000.0, 0.0),
                "tangent": (1.0, 0.0),
                "length": 1000.0,
                "top_elevations": [900.0, 900.0 + rise],
                "support_elevations": [0.0, rise],
                "cell_count": 1,
            }
            # A 50 x 40 rail between the centres of two 70 mm posts.
            rail = make_handrail_top_rail_shape(path, "Square", 50.0, 40.0, 35.0, 70.0)
            self.assertTrue(rail.isValid())
            run = math.hypot(1000.0, rise)
            # 40 mm square to the rail: its section is 50 x 40 whatever
            # the pitch, and it stands deeper plumb as the pitch steepens.
            self.assertAlmostEqual(rail.Volume, 50.0 * 40.0 * run, places=3)
            probe = Part.makeLine(
                FreeCAD.Vector(500.0, 0.0, 0.0), FreeCAD.Vector(500.0, 0.0, 3000.0)
            )
            crossing = rail.common(probe).BoundBox
            self.assertAlmostEqual(crossing.ZMax, 900.0 + rise / 2.0, places=6)
            self.assertAlmostEqual(crossing.ZLength, 40.0 * run / 1000.0, places=6)

    def test_classic_rail_sections_stand_square_within_their_size(self):
        from stairdesigner.geometry_handrails import make_handrail_top_rail_shape
        from stairdesigner.geometry_profiles import RAIL_PROFILES

        path = {
            "kind": "Linear",
            "start": (0.0, 0.0),
            "end": (1000.0, 0.0),
            "tangent": (1.0, 0.0),
            "length": 1000.0,
            "top_elevations": [900.0, 1600.0],
            "support_elevations": [0.0, 700.0],
            "cell_count": 1,
        }
        run = math.hypot(1000.0, 700.0)
        # Section areas of a 60 x 45 rail, by construction.
        areas = {
            "Square": 60.0 * 45.0,
            "Circular": math.pi * 30.0**2,
            "Rounded": 60.0 * 45.0 - 2.0 * (1.0 - math.pi / 4.0) * 15.0**2,
            "Oval": math.pi * 30.0 * 22.5,
        }
        self.assertEqual(RAIL_PROFILES[:2], ("Square", "Circular"))
        for profile in RAIL_PROFILES:
            with self.subTest(profile=profile):
                rail = make_handrail_top_rail_shape(path, profile, 60.0, 45.0, 35.0, 70.0)
                self.assertTrue(rail.isValid())
                self.assertEqual(len(rail.Solids), 1)
                # Cut plumb at both ends where the square rail is, and no
                # wider than asked...
                if profile != "Circular":
                    for corner in (
                        FreeCAD.Vector(-500.0, -500.0, 0.0),
                        FreeCAD.Vector(1000.0, -500.0, 0.0),
                        FreeCAD.Vector(-500.0, -530.0, 0.0),
                        FreeCAD.Vector(-500.0, 30.0, 0.0),
                    ):
                        size = (500.0, 1000.0) if corner.y == -500.0 else (2000.0, 500.0)
                        outside = Part.makeBox(size[0], size[1], 3000.0, corner)
                        self.assertLess(rail.common(outside).Volume, 1e-3)
                    for x in (0.5, 999.5):
                        self.assertTrue(rail.slice(FreeCAD.Vector(1.0, 0.0, 0.0), x))
                # ... its top on the rail's line whatever the pitch ...
                probe = Part.makeLine(
                    FreeCAD.Vector(500.0, 0.0, 0.0), FreeCAD.Vector(500.0, 0.0, 3000.0)
                )
                crossing = rail.common(probe).BoundBox
                self.assertAlmostEqual(crossing.ZMax, 1250.0, places=4)
                # ... and its section square to the rail, inside its size.
                area = rail.Volume / run
                self.assertLess(area, math.pi * 30.0**2 + 1e-3)
                self.assertGreater(area, 0.7 * 60.0 * 45.0)
                if profile in areas:
                    self.assertAlmostEqual(area, areas[profile], delta=0.5)
                if profile != "Circular":
                    self.assertAlmostEqual(crossing.ZLength, 45.0 * run / 1000.0, places=3)

    def test_worked_posts_keep_square_blocks_for_their_joints(self):
        from stairdesigner.geometry_profiles import MEMBER_SHAPES, POST_CAPS, make_member_shape

        def section(shape, elevation):
            return Part.Face(shape.slice(FreeCAD.Vector(0.0, 0.0, 1.0), elevation)).Area

        plain = 70.0 * 70.0
        for shape in MEMBER_SHAPES:
            for cap in POST_CAPS:
                with self.subTest(shape=shape, cap=cap):
                    post = make_member_shape(
                        (0.0, 0.0), (1.0, 0.0), 0.0, 1300.0, shape, 70.0, 70.0, (350.0, 950.0), cap
                    )
                    self.assertTrue(post.isValid())
                    self.assertEqual(len(post.Solids), 1)
                    self.assertAlmostEqual(post.BoundBox.ZMin, 0.0, places=6)
                    full = math.pi * 35.0**2 if shape == "Circular" else plain
                    # Whole where the joints are cut, below and above.
                    self.assertAlmostEqual(section(post, 200.0), full, delta=1.0)
                    self.assertAlmostEqual(section(post, 1100.0), full, delta=1.0)
                    middle = section(post, 650.0)
                    if shape == "Chamfered":
                        # Four 14 mm chamfers along the shaft.
                        self.assertAlmostEqual(middle, plain - 2.0 * 14.0**2, delta=1.0)
                    elif shape == "Turned":
                        self.assertLess(middle, math.pi * 35.0**2)
                        self.assertGreater(middle, math.pi * 12.0**2)
                    else:
                        self.assertAlmostEqual(middle, full, delta=1.0)
                    # A ball or an acorn stands on the post; the other ends
                    # are worked within its length.
                    if cap in ("Ball", "Acorn"):
                        self.assertGreater(post.BoundBox.ZMax, 1300.0 + 50.0)
                    else:
                        self.assertLess(post.BoundBox.ZMax, 1300.0 + 1e-6)
                        tip = section(post, 1299.0)
                        if cap == "Flat":
                            self.assertAlmostEqual(tip, full, delta=1.0)
                        else:
                            self.assertLess(tip, 0.95 * full)
        # Too short for a shaft between its blocks, a member stays square.
        stub = make_member_shape(
            (0.0, 0.0), (1.0, 0.0), 0.0, 400.0, "Turned", 70.0, 70.0, (150.0, 250.0)
        )
        self.assertAlmostEqual(stub.Volume, plain * 400.0, places=3)

    def test_stair_takes_classic_handrail_shapes(self):
        stair = self._joinery_stair()
        self.assertEqual(stair.HandrailPostCap, "Flat")
        stair.Proxy.defer_rebuilds(stair, True)
        stair.HandrailPostShape = "Turned"
        stair.HandrailPostCap = "Ball"
        stair.HandrailPicketShape = "Chamfered"
        stair.HandrailTopRailShape = "Moulded"
        stair.Proxy.defer_rebuilds(stair, False)
        members = {}
        for part in stair.HandrailsGroup.Group:
            members.setdefault(part.StairDesignerRole, []).append(part.Shape)
            self.assertTrue(part.Shape.isValid(), part.Label)
            self.assertEqual(len(part.Shape.Solids), 1, part.Label)
        self.assertEqual(len(members["HandrailPost"]), 4)
        plain = stair.HandrailPostWidth.Value * stair.HandrailPostThickness.Value
        for post in members["HandrailPost"]:
            # Turned, with its ball above the rail.
            self.assertLess(post.Volume, 0.95 * plain * post.BoundBox.ZLength)
        for rail in members["HandrailTopRail"]:
            for post in members["HandrailPost"]:
                self.assertLess(rail.common(post).Volume, 1e-3)
            for picket in members["HandrailPicket"]:
                self.assertLess(rail.common(picket).Volume, 1e-3)
        for stringer in stair.StringersGroup.Group:
            self.assertTrue(stringer.Shape.isValid())
            self.assertEqual(len(stringer.Shape.Solids), 1)
            for post in members["HandrailPost"]:
                self.assertLess(stringer.Shape.common(post).Volume, 1e-3)

    def test_handrail_joinery_tenons_the_rail_and_houses_the_pickets(self):
        from stairdesigner.geometry_handrails import make_handrail_top_rail_shape
        from stairdesigner.geometry_joinery import make_handrail_joinery

        path = {
            "kind": "Linear",
            "start": (0.0, 0.0),
            "end": (1000.0, 0.0),
            "tangent": (1.0, 0.0),
            "length": 1000.0,
            "top_elevations": [900.0, 1400.0],
            "support_elevations": [0.0, 500.0],
            "cell_count": 1,
        }
        # A 50 x 40 rail entering 35 mm into two 70 mm posts, over two
        # 20 mm pickets standing 10 mm into it.
        rail = make_handrail_top_rail_shape(path, "Square", 50.0, 40.0, 35.0, 70.0)
        posts = [
            Part.makeBox(70.0, 70.0, 1600.0, FreeCAD.Vector(x - 35.0, -35.0, 0.0))
            for x in (0.0, 1000.0)
        ]
        plumb = 40.0 * math.hypot(1000.0, 500.0) / 1000.0
        pickets = [
            Part.makeBox(
                20.0, 20.0, 900.0 + 0.5 * x - plumb + 10.0, FreeCAD.Vector(x - 10.0, -10.0, 0.0)
            )
            for x in (330.0, 660.0)
        ]
        (jointed,), mortised = make_handrail_joinery([rail], posts, pickets)
        self.assertTrue(jointed.isValid())
        self.assertEqual(len(jointed.Solids), 1)
        for post, original in zip(mortised, posts):
            self.assertTrue(post.isValid())
            self.assertEqual(len(post.Solids), 1)
            self.assertLess(jointed.common(post).Volume, 1e-6)
            # A tenon a third of the rail's width, as long as the rail went
            # in, with a shoulder above and below it.
            tenon = jointed.common(original)
            self.assertAlmostEqual(tenon.BoundBox.YLength, 50.0 / 3.0, places=6)
            self.assertAlmostEqual(tenon.BoundBox.XLength, 35.0, places=6)
            self.assertAlmostEqual(tenon.Volume, 35.0 * 50.0 / 3.0 * plumb / 2.0, places=3)
            self.assertAlmostEqual(original.Volume - post.Volume, tenon.Volume, places=3)
        for picket in pickets:
            self.assertLess(jointed.common(picket).Volume, 1e-6)
            self.assertLess(jointed.distToShape(picket)[0], 1e-6)
        # A rail running right through a post is no end joint: left alone.
        through = Part.makeBox(70.0, 70.0, 1600.0, FreeCAD.Vector(465.0, -35.0, 0.0))
        (same,), (untouched,) = make_handrail_joinery([rail], [through], [])
        self.assertAlmostEqual(same.Volume, rail.Volume, places=6)
        self.assertAlmostEqual(untouched.Volume, through.Volume, places=6)

    def test_stair_joinery_finger_joints_boards_into_each_other(self):
        from stairdesigner.geometry_joinery import make_stair_joinery

        # A 40 mm board whose end butts the side of another 40 mm board,
        # both 250 mm high: seven courses, three of them fingers.
        incoming = Part.makeBox(1000.0, 40.0, 250.0)
        crossing = Part.makeBox(40.0, 1000.0, 250.0, FreeCAD.Vector(1000.0, -480.0, 0.0))
        (joined, housed), _posts, _steps = make_stair_joinery(
            [incoming, crossing], [], [], finger_joints=True
        )
        for shape in (joined, housed):
            self.assertTrue(shape.isValid())
            self.assertEqual(len(shape.Solids), 1)
        self.assertLess(joined.common(housed).Volume, 1e-6)
        fingers = joined.Volume - incoming.Volume
        self.assertAlmostEqual(fingers, 3.0 * 40.0 * 40.0 * 250.0 / 7.0, places=3)
        self.assertAlmostEqual(crossing.Volume - housed.Volume, fingers, places=3)
        # The fingers run right through the other board, which keeps the
        # top and the bottom of the joint.
        self.assertAlmostEqual(joined.BoundBox.XMax, 1040.0, places=6)
        course = 250.0 / 7.0
        for index in range(7):
            point = FreeCAD.Vector(1020.0, 20.0, (index + 0.5) * course)
            self.assertEqual(joined.isInside(point, 1e-6, False), index % 2 == 1, index)
            self.assertEqual(housed.isInside(point, 1e-6, False), index % 2 == 0, index)

    def test_finger_joint_type_interlocks_the_wall_boards(self):
        stair, flights = self._short_inner_side_stair("Housed stringer", handrails=False)
        self.assertEqual(stair.StringerJointType, "Mortise and tenon")
        thickness = stair.StringerThickness.Value

        def shares():
            # How much of the corner block at the flight seam each of the
            # two wall boards meeting there holds.
            left = [part for part in stair.StringersGroup.Group if part.Side == "Left"]
            first = next(part.Shape for part in left if part.SourceFlight == flights[0])
            second = max(
                (part.Shape for part in left if part.SourceFlight == flights[1]),
                key=lambda shape: shape.Volume,
            )
            for shape in (first, second):
                self.assertTrue(shape.isValid())
                self.assertEqual(len(shape.Solids), 1)
            self.assertLess(first.common(second).Volume, 1e-3)
            bounds = first.BoundBox
            square = Part.makeBox(
                thickness,
                bounds.YLength,
                5000.0,
                FreeCAD.Vector(bounds.XMax - thickness, bounds.YMin, -100.0),
            )
            held = [shape.common(square).Volume for shape in (first, second)]
            return [volume / sum(held) for volume in held]

        # A stub tenon is a small part of the block; fingers share it.
        self.assertLess(min(shares()), 0.25)
        stair.StringerJointType = "Finger joint"
        self.assertGreater(min(shares()), 0.3)

    def test_stair_joinery_butts_a_board_running_past_another(self):
        from stairdesigner.geometry_joinery import make_stair_joinery

        # The second board runs 20 mm past the first one, level with it:
        # cut at the first, that end stands free beyond it.
        first = Part.makeBox(1000.0, 40.0, 200.0, FreeCAD.Vector(0.0, 20.0, 200.0))
        second = Part.makeBox(40.0, 800.0, 200.0, FreeCAD.Vector(20.0, 0.0, 200.0))
        (kept, butted), _posts, _steps = make_stair_joinery([first, second], [], [])
        self.assertEqual(len(butted.Solids), 1)
        self.assertLess(kept.common(butted).Volume, 1e-6)
        # The end beyond the first board is gone: the second one stops at
        # it, but for its tenon.
        self.assertGreater(butted.BoundBox.YMin, 20.0 + 1.0)
        # A board crossing another mid-length is no butt joint: both of
        # its halves stay.
        crossing = Part.makeBox(40.0, 800.0, 200.0, FreeCAD.Vector(480.0, -380.0, 200.0))
        (_kept, crossed), _posts, _steps = make_stair_joinery([first, crossing], [], [])
        self.assertAlmostEqual(crossed.BoundBox.YMin, -380.0, places=6)
        self.assertAlmostEqual(crossed.BoundBox.YMax, 420.0, places=6)

    def test_notched_board_keeps_its_timber_under_winder_cuts(self):
        from stairdesigner.geometry_stringer_path import notched_stringer_throat
        from stairdesigner.geometry_stringer_shapes import make_notched_stringer_shape

        # What a 250 mm board leaves under the cuts of regular 200 x 250
        # steps with 40 mm treads, square to the board and plumb.
        pitch = math.hypot(250.0, 200.0) / 250.0
        throat = notched_stringer_throat(200.0, 250.0, 40.0, 250.0)
        self.assertAlmostEqual(throat, 250.0 - 240.0 / pitch, places=9)

        def board(stations):
            sections = [
                BalancedSection(
                    center=(station, 450.0),
                    tangent=(1.0, 0.0),
                    left=(station, 900.0),
                    right=(station, 0.0),
                    station=float(index) * 250.0,
                    width=900.0,
                    flight_index=0,
                    riser_index=index + 1,
                )
                for index, station in enumerate(stations)
            ]
            shape = make_notched_stringer_shape(
                sections, 200.0, 40.0, "Left", 40.0, 250.0, 20.0, 0.0, 0.0, throat
            )
            self.assertTrue(shape.isValid())
            self.assertEqual(len(shape.Solids), 1)
            # The timber plumb under the inner corner of each cut.
            return shape, [
                (index * 200.0 - 40.0) - self._bottom_at(shape, stations[index] - 0.5, 860.0)
                for index in range(1, len(stations) - 1)
            ]

        # Along a regular flight the lower edge stays the straight line the
        # board's width sets below the nosings.
        regular, timbers = board([250.0 * index for index in range(7)])
        self.assertTrue(all(edge.Curve.TypeId == "Part::GeomLine" for edge in regular.Edges))
        for timber in timbers:
            self.assertAlmostEqual(timber, throat * pitch + 0.5 * 0.8, places=6)

        # Winders stretch two steps to 450 and 500 mm along the board: a
        # straight edge would leave some 20 mm under their cuts. The edge
        # runs under them instead, keeping the regular step's timber.
        _winding, timbers = board([0.0, 250.0, 700.0, 1200.0, 1450.0, 1700.0])
        for timber in timbers:
            self.assertGreaterEqual(timber, throat - 1e-3)
            self.assertLess(timber, 2.0 * throat)

    def test_stub_tenons_keep_their_minimum_thickness(self):
        from stairdesigner.geometry_joinery import _tenon_from_face

        # A third of the board, but never under 12 mm on a thin one.
        for board, expected in ((27.0, 12.0), (45.0, 15.0)):
            corners = [
                FreeCAD.Vector(0.0, 0.0, 0.0),
                FreeCAD.Vector(0.0, board, 0.0),
                FreeCAD.Vector(0.0, board, 250.0),
                FreeCAD.Vector(0.0, 0.0, 250.0),
            ]
            end = Part.Face(Part.makePolygon(corners + corners[:1]))
            tenon = _tenon_from_face(end, FreeCAD.Vector(1.0, 0.0, 0.0), 45.0)
            self.assertAlmostEqual(tenon.BoundBox.YLength, expected, places=6)
            self.assertAlmostEqual(tenon.BoundBox.XLength, 45.0, places=6)
            self.assertAlmostEqual(tenon.BoundBox.Center.y, board / 2.0, places=6)

    def test_stair_joinery_tenons_board_into_abutting_board(self):
        from stairdesigner.geometry_joinery import make_stair_joinery

        # A 40 mm board whose end butts the side of another 40 mm board.
        incoming = Part.makeBox(1000.0, 40.0, 250.0)
        crossing = Part.makeBox(40.0, 1000.0, 250.0, FreeCAD.Vector(1000.0, -480.0, 0.0))
        (joined, housed), posts, _steps = make_stair_joinery([incoming, crossing], [], [])
        self.assertEqual(posts, [])
        tenon = joined.Volume - incoming.Volume
        self.assertGreater(tenon, 0.0)
        self.assertAlmostEqual(crossing.Volume - housed.Volume, tenon, places=3)
        self.assertLess(joined.common(housed).Volume, 1e-6)
        self.assertLess(joined.BoundBox.XMax, 1040.0 - 1.0)
