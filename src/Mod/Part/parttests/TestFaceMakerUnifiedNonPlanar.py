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

"""Non-planar and multi-plane tests for Part::FaceMakerUnified.

Planar geometry on non-XY planes is already covered by
TestFaceMakerUnifiedPlanar.py (which transforms every test to XZ and
tilted planes). This file tests cases that require different code paths:
- Multiple wires on different planes (can't use single-plane transform)
- Analytical surfaces (cylinder, cone)
- Freeform non-planar surfaces (BRepFill_Filling)
"""

import math
import unittest

import FreeCAD
import Part

Vec = FreeCAD.Vector


def make_polygon(*points):
    vecs = [Vec(*p) if isinstance(p, (list, tuple)) else p for p in points]
    if vecs[0] != vecs[-1]:
        vecs.append(vecs[0])
    return Part.Wire(Part.makePolygon(vecs))


def unified(wires):
    if isinstance(wires, Part.Wire):
        wires = [wires]
    return Part.makeFace(Part.Compound(wires), "Part::FaceMakerUnified").Faces


def total_area(faces):
    return sum(f.Area for f in faces)


# =========================================================================
# 1. Multiple wires on different planes
# =========================================================================


class TestDifferentPlanes(unittest.TestCase):
    def test_two_rects_xy_and_xz(self):
        w1 = make_polygon((0, 0, 0), (10, 0, 0), (10, 10, 0), (0, 10, 0))
        w2 = make_polygon((20, 0, 0), (30, 0, 0), (30, 0, 10), (20, 0, 10))
        faces = unified([w1, w2])
        self.assertEqual(len(faces), 2)
        self.assertAlmostEqual(total_area(faces), 200.0, places=3)

    def test_two_circles_different_normals(self):
        c1 = Part.Wire(Part.makeCircle(8, Vec(0, 0, 0), Vec(0, 0, 1)))
        c2 = Part.Wire(Part.makeCircle(8, Vec(30, 0, 0), Vec(0, 1, 0)))
        faces = unified([c1, c2])
        self.assertEqual(len(faces), 2)


# =========================================================================
# 2. Analytical surfaces
# =========================================================================


class TestAnalyticalSurfaces(unittest.TestCase):
    def test_cylinder_lateral(self):
        cyl = Part.makeCylinder(10, 20)
        lateral = [f for f in cyl.Faces if f.Surface.TypeId == "Part::GeomCylinder"][0]
        faces = unified(lateral.OuterWire)
        self.assertEqual(len(faces), 1)
        self.assertGreater(faces[0].Area, 0)

    def test_cone_lateral(self):
        cone = Part.makeCone(10, 5, 20)
        lateral = [f for f in cone.Faces if f.Surface.TypeId == "Part::GeomCone"][0]
        faces = unified(lateral.OuterWire)
        self.assertEqual(len(faces), 1)
        self.assertGreater(faces[0].Area, 0)


# =========================================================================
# 3. Freeform non-planar (BRepFill_Filling)
# =========================================================================


class TestFreeformSurfaces(unittest.TestCase):
    def test_twisted_quad(self):
        """Quad with one vertex off-plane."""
        w = make_polygon((0, 0, 0), (10, 0, 0), (10, 10, 5), (0, 10, 0))
        faces = unified(w)
        self.assertEqual(len(faces), 1)
        self.assertGreater(faces[0].Area, 100.0)

    def test_saddle_hexagon(self):
        """Hexagon with alternating Z (saddle shape)."""
        r = 10
        pts = []
        for i in range(6):
            a = i * math.pi / 3
            z = 3 * (1 if i % 2 == 0 else -1)
            pts.append((r * math.cos(a), r * math.sin(a), z))
        w = make_polygon(*pts)
        faces = unified(w)
        self.assertEqual(len(faces), 1)
        self.assertGreater(faces[0].Area, 0)

    def test_bspline_wavy(self):
        """Closed BSpline with non-coplanar control points."""
        pts = [Vec(0, 0, 0), Vec(10, 0, 5), Vec(10, 10, -3), Vec(0, 10, 4)]
        bs = Part.BSplineCurve()
        bs.interpolate(pts, PeriodicFlag=True)
        faces = unified(Part.Wire(bs.toShape()))
        self.assertEqual(len(faces), 1)
        self.assertGreater(faces[0].Area, 0)

    def test_steep_fold(self):
        """Wire with a steep fold in Z."""
        w = make_polygon((0, 0, 0), (10, 0, 0), (10, 5, 20), (10, 10, 0), (0, 10, 0))
        faces = unified(w)
        self.assertEqual(len(faces), 1)
        self.assertGreater(faces[0].Area, 0)

    def test_two_separate_freeform(self):
        """Two independent non-planar wires -> 2 filled faces."""
        w1 = make_polygon((0, 0, 0), (10, 0, 0), (10, 10, 5), (0, 10, 0))
        w2 = make_polygon((20, 0, 0), (30, 0, 3), (30, 10, 0), (20, 10, 4))
        faces = unified([w1, w2])
        self.assertEqual(len(faces), 2)

    def test_mixed_planar_and_freeform(self):
        """Planar circle + freeform quad in one call -> 2 faces."""
        planar = Part.Wire(Part.makeCircle(5, Vec(0, 0, 0)))
        freeform = make_polygon((20, 0, 0), (30, 0, 0), (30, 10, 5), (20, 10, 0))
        faces = unified([planar, freeform])
        self.assertEqual(len(faces), 2)


# =========================================================================
# 4. Edge cases
# =========================================================================


class TestEdgeCases(unittest.TestCase):
    def test_xz_rectangle(self):
        """Rectangle on XZ plane — regression test for gp_Dir2d bug."""
        w = make_polygon((0, 0, 0), (1, 0, 0), (1, 0, 1), (0, 0, 1))
        faces = unified(w)
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, 1.0, places=3)

    def test_near_planar(self):
        """Z offset below tolerance -> treated as planar."""
        w = make_polygon((0, 0, 0), (10, 0, 0), (10, 10, 1e-8), (0, 10, 0))
        faces = unified(w)
        self.assertEqual(len(faces), 1)
        self.assertAlmostEqual(faces[0].Area, 100.0, places=1)

    def test_bspline_coplanar_non_xy(self):
        """BSpline whose points are coplanar on a tilted plane."""
        pts = [Vec(0, 0, 10), Vec(5, 5, 5), Vec(10, 0, 0), Vec(5, -5, 5)]
        bs = Part.BSplineCurve()
        bs.interpolate(pts, PeriodicFlag=True)
        faces = unified(Part.Wire(bs.toShape()))
        self.assertEqual(len(faces), 1)
        self.assertGreater(faces[0].Area, 0)
