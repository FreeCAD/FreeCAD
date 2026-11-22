# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors
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

"""Planar tests for Part::FaceMakerUnified.

All test geometry is defined on the XY plane, then transformed to the
target plane via a class-level placement. Each test mixin is instantiated
for every plane in _PLANES, so every test runs on XY, XZ, and a 45-deg
tilted plane automatically.
"""

import math
import os
import unittest

import FreeCAD
import Part

Vec = FreeCAD.Vector


# =========================================================================
# Helpers — produce geometry on the XY plane
# =========================================================================


def rect_wire(x0, y0, x1, y1):
    return Part.Wire(
        Part.makePolygon([Vec(x0, y0), Vec(x1, y0), Vec(x1, y1), Vec(x0, y1), Vec(x0, y0)])
    )


def rect_face(x0, y0, x1, y1):
    return Part.Face(rect_wire(x0, y0, x1, y1))


def circle_wire(cx, cy, r):
    return Part.Wire(Part.makeCircle(r, Vec(cx, cy, 0)))


def triangle_wire(p1, p2, p3):
    return Part.Wire(Part.makePolygon([Vec(*p1, 0), Vec(*p2, 0), Vec(*p3, 0), Vec(*p1, 0)]))


def line_wire(*points):
    return Part.Wire(Part.makePolygon([Vec(*p) if len(p) >= 2 else p for p in points]))


def total_area(faces):
    return sum(f.Area for f in faces)


def union_area(*face_shapes):
    result = face_shapes[0]
    for s in face_shapes[1:]:
        result = result.fuse(s)
    return sum(f.Area for f in result.Faces)


def faces_overlap(faces):
    for i, f1 in enumerate(faces):
        for f2 in faces[i + 1 :]:
            if f1.common(f2).Faces:
                return True
    return False


# =========================================================================
# Base mixin — transforms XY geometry to the target plane before testing
# =========================================================================


class _PlaneTestBase:
    """Mixin providing unified() that transforms wires to a target plane.

    Subclasses set ``placement`` as a class attribute. Areas and face counts
    are invariant under rigid transforms, so assertions stay the same.
    """

    placement = FreeCAD.Placement()

    def unified(self, wires):
        if isinstance(wires, Part.Wire):
            wires = [wires]
        mat = self.placement.toMatrix()
        transformed = [Part.Wire(w.transformed(mat).Edges) for w in wires]
        return Part.makeFace(Part.Compound(transformed), "Part::FaceMakerUnified").Faces


# =========================================================================
# Test mixins (no unittest.TestCase — concrete classes generated below)
# pylint: disable=no-member
# =========================================================================


class _SingleShape(_PlaneTestBase):
    def test_rectangle(self):
        faces = self.unified(rect_wire(0, 0, 10, 10))
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, 100.0, places=3)

    def test_circle(self):
        faces = self.unified(circle_wire(0, 0, 10))
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, math.pi * 100, places=1)

    def test_triangle(self):
        faces = self.unified(triangle_wire((0, 0), (10, 0), (5, 10)))
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, 50.0, places=3)

    def test_semicircle(self):
        r = 10
        arc = Part.ArcOfCircle(Part.Circle(Vec(0, 0, 0), Vec(0, 0, 1), r), 0, math.pi)
        line = Part.LineSegment(Vec(-r, 0, 0), Vec(r, 0, 0))
        w = Part.Wire([arc.toShape(), line.toShape()])
        faces = self.unified(w)
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, math.pi * r * r / 2, places=1)


class _SeparateShapes(_PlaneTestBase):
    def test_two_rectangles(self):
        faces = self.unified([rect_wire(0, 0, 5, 5), rect_wire(20, 20, 25, 25)])
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_area(faces), 50.0, places=3)

    def test_three_mixed_shapes(self):
        faces = self.unified(
            [
                rect_wire(0, 0, 5, 5),
                circle_wire(20, 0, 3),
                triangle_wire((40, 0), (50, 0), (45, 8)),
            ]
        )
        self.assertEqual(len(faces), 3)

    def test_shared_edge(self):
        faces = self.unified([rect_wire(0, 0, 10, 10), rect_wire(10, 0, 20, 10)])
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_area(faces), 200.0, places=3)

    def test_shared_corner(self):
        faces = self.unified([rect_wire(0, 0, 10, 10), rect_wire(10, 10, 20, 20)])
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_area(faces), 200.0, places=3)


class _Nesting(_PlaneTestBase):
    def test_circle_inside_rectangle(self):
        faces = self.unified([rect_wire(-20, -20, 20, 20), circle_wire(0, 0, 5)])
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(total_area(faces), 1600.0 - math.pi * 25, places=1)

    def test_concentric_circles(self):
        faces = self.unified([circle_wire(0, 0, 10), circle_wire(0, 0, 5)])
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(total_area(faces), math.pi * 75, places=1)

    def test_triple_nesting(self):
        faces = self.unified(
            [rect_wire(0, 0, 30, 30), rect_wire(5, 5, 25, 25), rect_wire(10, 10, 20, 20)]
        )
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_area(faces), 600.0, places=3)

    def test_four_concentric_circles(self):
        r1, r2, r3, r4 = 25.8, 15.4, 6.4, 2.5
        faces = self.unified(
            [
                circle_wire(0, 0, r1),
                circle_wire(0, 0, r2),
                circle_wire(0, 0, r3),
                circle_wire(0, 0, r4),
            ]
        )
        expected = math.pi * (r1**2 - r2**2 + r3**2 - r4**2)
        self.assertAlmostEqual(total_area(faces), expected, delta=1.0)


class _TwoOverlapping(_PlaneTestBase):
    def test_two_circles(self):
        r, d = 10, 12
        faces = self.unified([circle_wire(0, 0, r), circle_wire(d, 0, r)])
        self.assertEqual(len(faces), 3)
        cos_arg = d / (2 * r)
        lens = 2 * r * r * math.acos(cos_arg) - (d / 2) * math.sqrt(4 * r * r - d * d)
        self.assertAlmostEqual(total_area(faces), 2 * math.pi * r * r - lens, places=1)
        self.assertFalse(faces_overlap(faces))

    def test_two_rectangles(self):
        faces = self.unified([rect_wire(0, 0, 20, 20), rect_wire(10, 10, 30, 30)])
        expected = union_area(rect_face(0, 0, 20, 20), rect_face(10, 10, 30, 30))
        self.assertAlmostEqual(total_area(faces), expected, delta=1.0)
        self.assertFalse(faces_overlap(faces))


class _Subdivision(_PlaneTestBase):
    def test_diagonal(self):
        faces = self.unified([rect_wire(0, 0, 10, 10), line_wire((0, 0), (10, 10))])
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_area(faces), 100.0, places=3)

    def test_cross_pattern(self):
        faces = self.unified([rect_wire(-5, -15, 5, 15), rect_wire(-15, -5, 15, 5)])
        self.assertEqual(len(faces), 5)
        self.assertAlmostEqual(total_area(faces), 500.0, places=2)
        self.assertFalse(faces_overlap(faces))

    def test_midpoint_cross(self):
        faces = self.unified(
            [rect_wire(0, 0, 10, 10), line_wire((0, 5), (10, 5)), line_wire((5, 0), (5, 10))]
        )
        self.assertEqual(len(faces), 4)
        self.assertAlmostEqual(total_area(faces), 100.0, places=3)
        self.assertFalse(faces_overlap(faces))
        for f in faces:
            self.assertAlmostEqual(f.Area, 25.0, places=2)

    def test_incomplete_intersection(self):
        faces = self.unified([rect_wire(0, 0, 10, 10), line_wire((5, 0), (5, 5))])
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, 100.0, places=3)

    def test_t_junction(self):
        faces = self.unified(
            [rect_wire(0, 0, 10, 10), line_wire((0, 0), (10, 10)), line_wire((5, 0), (5, 5))]
        )
        self.assertEqual(len(faces), 3)
        self.assertAlmostEqual(total_area(faces), 100.0, places=3)

    def test_dangling_chain(self):
        faces = self.unified(
            [
                rect_wire(0, 0, 10, 10),
                line_wire((5, 0), (5, 4)),
                line_wire((5, 4), (7, 6)),
                line_wire((7, 6), (4, 8)),
            ]
        )
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, 100.0, places=3)

    def test_floating_line(self):
        faces = self.unified([rect_wire(0, 0, 10, 10), line_wire((3, 3), (7, 7))])
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, 100.0, places=3)

    def test_diagonal_plus_dangling(self):
        faces = self.unified(
            [rect_wire(0, 0, 10, 10), line_wire((0, 0), (10, 10)), line_wire((10, 5), (7, 5))]
        )
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_area(faces), 100.0, places=3)

    def test_circle_with_dangling_radius(self):
        """Circle + line from center to outside: dangling line should be
        pruned, producing 1 circular face with a single wire."""
        r = 10
        faces = self.unified([circle_wire(0, 0, r), line_wire((0, 0), (r + 5, 0))])
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, math.pi * r * r, places=1)
        # Face should have 1 clean wire — no internal dangling edges
        self.assertEqual(len(faces[0].Wires), 1)

    def test_circle_with_line_through(self):
        """Circle + line from outside through interior to outside: the line
        crosses the boundary twice, creating a chord that splits the circle
        into 2 faces. No dangling segments should remain."""
        r = 10
        faces = self.unified([circle_wire(0, 0, r), line_wire((-r - 5, 0), (r + 5, 0))])
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_area(faces), math.pi * r * r, places=1)


class _MultipleOverlapping(_PlaneTestBase):
    def test_three_circles(self):
        r = 10
        faces = self.unified([circle_wire(0, 0, r), circle_wire(8, 0, r), circle_wire(4, 7, r)])
        self.assertEqual(len(faces), 7)
        c1 = Part.Face(circle_wire(0, 0, r))
        c2 = Part.Face(circle_wire(8, 0, r))
        c3 = Part.Face(circle_wire(4, 7, r))
        self.assertAlmostEqual(total_area(faces), union_area(c1, c2, c3), places=0)

    def test_four_circles(self):
        faces = self.unified(
            [
                circle_wire(0, 0, 10),
                circle_wire(8, 0, 10),
                circle_wire(0, 8, 10),
                circle_wire(8, 8, 10),
            ]
        )
        self.assertGreaterEqual(len(faces), 9)


class _OverlapWithHoles(_PlaneTestBase):
    def test_overlapping_rects_with_hole(self):
        faces = self.unified(
            [rect_wire(0, 0, 90, 80), rect_wire(-30, -10, 30, 20), rect_wire(10, 20, 30, 40)]
        )
        large = rect_face(0, 0, 90, 80)
        small = rect_face(-30, -10, 30, 20)
        hole = rect_face(10, 20, 30, 40)
        expected = sum(f.Area for f in large.fuse(small).cut(hole).Faces)
        self.assertAlmostEqual(total_area(faces), expected, delta=1.0)

    def test_overlapping_with_hole_and_separate(self):
        faces = self.unified(
            [
                rect_wire(-20, -20, 20, 20),
                rect_wire(10, 10, 30, 30),
                rect_wire(-5, -5, 5, 5),
                rect_wire(50, 0, 60, 10),
            ]
        )
        large = rect_face(-20, -20, 20, 20)
        overlap = rect_face(10, 10, 30, 30)
        hole = rect_face(-5, -5, 5, 5)
        main_a = sum(f.Area for f in large.fuse(overlap).cut(hole).Faces)
        self.assertAlmostEqual(total_area(faces), main_a + 100.0, delta=1.0)


class _Degenerate(_PlaneTestBase):
    def test_crossing_edges(self):
        w = Part.Wire(
            Part.makePolygon([Vec(-10, -5), Vec(10, 5), Vec(10, -5), Vec(-10, 5), Vec(-10, -5)])
        )
        faces = self.unified(w)
        self.assertGreaterEqual(len(faces), 2)
        self.assertGreater(total_area(faces), 0)

    def test_bowtie(self):
        w1 = Part.Wire(Part.makePolygon([Vec(-10, -5), Vec(-10, 5), Vec(0, 0), Vec(-10, -5)]))
        w2 = Part.Wire(Part.makePolygon([Vec(0, 0), Vec(10, 5), Vec(10, -5), Vec(0, 0)]))
        faces = self.unified([w1, w2])
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_area(faces), 100.0, places=1)

    def test_bspline_with_separate_line(self):
        """Self-intersecting BSpline + separate non-touching line:
        BSpline should produce 2 lobes, line should be pruned."""
        poles = [
            Vec(0, 26.06),
            Vec(14.6, 15.54),
            Vec(0.51, 0),
            Vec(-16.13, -17.23),
            Vec(0, -24.02),
            Vec(17.15, -9.42),
            Vec(-16.64, 15.20),
            Vec(0, 26.06),
        ]
        bs = Part.BSplineCurve(poles, None, None, False, 3, [1] * len(poles), False)
        separate_line = line_wire((500, 500), (600, 600))
        faces = self.unified([Part.Wire([bs.toShape()]), separate_line])
        self.assertGreaterEqual(len(faces), 2)
        self.assertGreater(total_area(faces), 0)

    def test_bspline_with_overlapping_circles(self):
        """Self-intersecting BSpline must not affect separate overlapping
        circles. The circles should still produce the correct Venn regions."""
        poles = [
            Vec(-60, 0),
            Vec(-45, -15),
            Vec(-55, -25),
            Vec(-65, -10),
            Vec(-50, 8),
            Vec(-60, 20),
            Vec(-70, 10),
            Vec(-60, 0),
        ]
        bs = Part.BSplineCurve(poles, None, None, False, 3, [1] * len(poles), False)
        r = 10
        faces = self.unified(
            [Part.Wire([bs.toShape()]), circle_wire(0, 0, r), circle_wire(8, 0, r)]
        )
        # BSpline: 2 lobes + circles: 3 Venn regions = 5 total
        self.assertGreaterEqual(len(faces), 5)
        # Verify the circle area is preserved (union of two r=10 circles at d=8)
        c1 = Part.Face(circle_wire(0, 0, r))
        c2 = Part.Face(circle_wire(8, 0, r))
        circle_union_area = union_area(c1, c2)
        bspline_area = total_area(faces) - circle_union_area
        self.assertGreater(bspline_area, 0)

    def test_figure_8_polygon(self):
        w = Part.Wire(
            Part.makePolygon(
                [
                    Vec(5, 0),
                    Vec(0, 5),
                    Vec(-5, 0),
                    Vec(0, -5),
                    Vec(5, 0),
                    Vec(10, 5),
                    Vec(15, 0),
                    Vec(10, -5),
                    Vec(5, 0),
                ]
            )
        )
        faces = self.unified(w)
        self.assertGreaterEqual(len(faces), 1)

    def test_figure_8_bspline(self):
        """Single self-intersecting BSpline forming a figure-8 -> 2 faces."""
        poles = [
            Vec(0, 26.06),
            Vec(14.6, 15.54),
            Vec(0.51, 0),
            Vec(-16.13, -17.23),
            Vec(0, -24.02),
            Vec(17.15, -9.42),
            Vec(-16.64, 15.20),
            Vec(0, 26.06),
        ]
        bs = Part.BSplineCurve(poles, None, None, False, 3, [1] * len(poles), False)
        w = Part.Wire([bs.toShape()])
        faces = self.unified(w)
        self.assertGreaterEqual(len(faces), 2)
        self.assertGreater(total_area(faces), 0)


# =========================================================================
# Standalone tests (plane-independent, run once)
# =========================================================================


class TestDegenerateInput(unittest.TestCase):
    def test_single_open_line(self):
        """An open wire cannot form a face."""
        self.assertRaises(
            ValueError,
            Part.makeFace,
            Part.Compound([line_wire((0, 0), (10, 0))]),
            "Part::FaceMakerUnified",
        )

    def test_empty_compound(self):
        """An empty compound cannot form a face."""
        self.assertRaises(
            ValueError,
            Part.makeFace,
            Part.Compound([]),
            "Part::FaceMakerUnified",
        )


# =========================================================================
# Text glyph tests — uses the font bundled with FreeCAD (osifont)
# =========================================================================

_FONT_PATH = os.path.join(FreeCAD.getResourceDir(), "Mod", "TechDraw", "Resources", "fonts")
_FONT_FILE = "osifont-lgpl3fe.ttf"
_FONT_AVAILABLE = os.path.isfile(os.path.join(_FONT_PATH, _FONT_FILE))


class TestTextGlyphs(unittest.TestCase):
    """Unified face making for actual font glyph wires."""

    def setUp(self):
        if not _FONT_AVAILABLE:
            self.skipTest("Bundled osifont not found")

    def _glyph_faces(self, char):
        wires = Part.makeWireString(char, _FONT_PATH + "/", _FONT_FILE, 10.0, 0.0)
        self.assertEqual(len(wires), 1, f"Expected 1 character, got {len(wires)}")
        comp = Part.Compound(wires[0])
        return Part.makeFace(comp, "Part::FaceMakerUnified").Faces

    def test_letter_A(self):
        """Letter A has an outer outline and a triangular hole — 1 face."""
        faces = self._glyph_faces("A")
        self.assertEqual(len(faces), 1)
        self.assertGreater(faces[0].Area, 0)

    def test_letter_B(self):
        """Letter B has an outer outline and two holes — 1 face."""
        faces = self._glyph_faces("B")
        self.assertEqual(len(faces), 1)
        self.assertGreater(faces[0].Area, 0)

    def test_letter_O(self):
        """Letter O has an outer and inner outline — 1 annular face."""
        faces = self._glyph_faces("O")
        self.assertEqual(len(faces), 1)
        self.assertGreater(faces[0].Area, 0)

    def test_letter_D(self):
        """Letter D has an outer outline and one hole — 1 face."""
        faces = self._glyph_faces("D")
        self.assertEqual(len(faces), 1)
        self.assertGreater(faces[0].Area, 0)

    def test_digit_8(self):
        """Digit 8 has an outer outline and two holes — 1 face."""
        faces = self._glyph_faces("8")
        self.assertEqual(len(faces), 1)
        self.assertGreater(faces[0].Area, 0)

    def test_multiple_chars(self):
        """Multiple characters each produce valid faces."""
        for char in "HELLO":
            wires = Part.makeWireString(char, _FONT_PATH + "/", _FONT_FILE, 10.0, 0.0)
            self.assertEqual(len(wires), 1)
            comp = Part.Compound(wires[0])
            faces = Part.makeFace(comp, "Part::FaceMakerUnified").Faces
            self.assertGreater(len(faces), 0, f"No faces for '{char}'")
            self.assertGreater(total_area(faces), 0, f"Zero area for '{char}'")


_MIXINS = [
    _SingleShape,
    _SeparateShapes,
    _Nesting,
    _TwoOverlapping,
    _Subdivision,
    _MultipleOverlapping,
    _OverlapWithHoles,
    _Degenerate,
]

_PLANES = {
    "XY": FreeCAD.Placement(),
    "XZ": FreeCAD.Placement(Vec(0, 0, 0), FreeCAD.Rotation(Vec(1, 0, 0), 90)),
    "Tilted": FreeCAD.Placement(
        Vec(10, -20, 15), FreeCAD.Rotation(Vec(1, 0, 0), 45) * FreeCAD.Rotation(Vec(0, 0, 1), 30)
    ),
}

for _plane_name, _placement in _PLANES.items():
    for _mixin in _MIXINS:
        _cls_name = f"{_mixin.__name__[1:]}_{_plane_name}"
        _cls = type(_cls_name, (_mixin, unittest.TestCase), {"placement": _placement})
        globals()[_cls_name] = _cls
