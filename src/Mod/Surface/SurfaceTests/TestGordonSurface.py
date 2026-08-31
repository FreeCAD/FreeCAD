# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2026 Andrew Shkolik <shkolik@gmail.com>                 *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "Gordon surface unit tests"
__author__ = "Andrew Shkolik"
__url__ = "https://www.freecad.org"

import unittest
import FreeCAD
from FreeCAD import Base
import Surface
import Part

vec = Base.Vector


class TestGordonSurface(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("GordonTest")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _make_grid(self):
        """Create 4 Part::Feature objects forming a simple 2x2 curve network.

        Profiles (V): lines along Y at X=0 and X=10.
        Guides (U):   lines along X at Y=0 and Y=10.

        Returns (obj_p1, obj_p2, obj_g1, obj_g2).
        """
        p1 = self.doc.addObject("Part::Feature", "Profile1")
        p1.Shape = Part.makeLine(vec(0, 0, 0), vec(0, 10, 0))

        p2 = self.doc.addObject("Part::Feature", "Profile2")
        p2.Shape = Part.makeLine(vec(10, 0, 0), vec(10, 10, 0))

        g1 = self.doc.addObject("Part::Feature", "Guide1")
        g1.Shape = Part.makeLine(vec(0, 0, 0), vec(10, 0, 0))

        g2 = self.doc.addObject("Part::Feature", "Guide2")
        g2.Shape = Part.makeLine(vec(0, 10, 0), vec(10, 10, 0))

        return p1, p2, g1, g2

    def test_create_gordon_surface(self):
        """Minimal Gordon surface — 2 profiles × 2 guides forming a grid."""
        p1, p2, g1, g2 = self._make_grid()

        gordon = self.doc.addObject("Surface::GordonSurface", "Gordon")
        gordon.ProfileEdges = [(p1, ["Edge1"]), (p2, ["Edge1"])]
        gordon.ProfileDirections = [False, False]
        gordon.GuideEdges = [(g1, ["Edge1"]), (g2, ["Edge1"])]
        gordon.GuideDirections = [False, False]

        self.doc.recompute()
        status = gordon.getStatusString()
        print("Status:", status)
        self.assertTrue(gordon.isValid(), "Gordon surface is not valid: " + status)
        self.assertIsNotNone(gordon.Shape, "Gordon surface has no Shape")
        self.assertFalse(gordon.Shape.isNull(), "Gordon surface Shape is null")

        first_area = gordon.Shape.Area
        # A valid face should have a positive area
        self.assertGreater(first_area, 0, "Gordon surface area should be positive")

        # check recomputation stability
        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status after recompute:", status)

        self.assertTrue(gordon.isValid(), "Gordon surface is not valid: " + status)
        self.assertIsNotNone(gordon.Shape, "Gordon surface has no Shape")
        self.assertFalse(gordon.Shape.isNull(), "Gordon surface Shape is null")
        self.assertAlmostEqual(gordon.Shape.Area, first_area)

    def test_insufficient_profiles(self):
        """Gordon surface should fail when less than 2 profiles are provided."""
        p1, p2, g1, g2 = self._make_grid()

        gordon = self.doc.addObject("Surface::GordonSurface", "Gordon")

        # Only one profile - invalid
        gordon.ProfileEdges = [(p1, ["Edge1"])]
        gordon.ProfileDirections = [False]

        gordon.GuideEdges = [(g1, ["Edge1"]), (g2, ["Edge1"])]
        gordon.GuideDirections = [False, False]

        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status:", status)

        self.assertFalse(
            gordon.isValid(), "Gordon surface should be invalid with insufficient profiles"
        )
        self.assertIn("at least 2 profiles", status)

    def test_insufficient_guides(self):
        """Gordon surface should fail when less than 2 guides are provided."""
        p1, p2, g1, g2 = self._make_grid()

        gordon = self.doc.addObject("Surface::GordonSurface", "Gordon")

        gordon.ProfileEdges = [(p1, ["Edge1"]), (p2, ["Edge1"])]
        gordon.ProfileDirections = [False, False]

        # Only one guide - invalid
        gordon.GuideEdges = [(g1, ["Edge1"])]
        gordon.GuideDirections = [False]

        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status:", status)

        self.assertFalse(
            gordon.isValid(), "Gordon surface should be invalid with insufficient guides"
        )
        self.assertIn("at least 2 guides", status)

    def test_invalid_edge_reference(self):
        """Gordon surface should fail when an edge reference does not exist."""
        p1, p2, g1, g2 = self._make_grid()

        gordon = self.doc.addObject("Surface::GordonSurface", "Gordon")

        # Invalid sub-element name
        gordon.ProfileEdges = [(p1, ["Edge999"]), (p2, ["Edge1"])]
        gordon.ProfileDirections = [False, False]

        gordon.GuideEdges = [(g1, ["Edge1"]), (g2, ["Edge1"])]
        gordon.GuideDirections = [False, False]

        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status:", status)

        self.assertFalse(
            gordon.isValid(), "Gordon surface should be invalid with missing edge reference"
        )
        self.assertIn("out of bound", status)

    def test_direction_count_mismatch(self):
        """Gordon surface should fail when direction count does not match edge count."""
        p1, p2, g1, g2 = self._make_grid()

        gordon = self.doc.addObject("Surface::GordonSurface", "Gordon")

        # Two profiles but only one direction flag - invalid
        gordon.ProfileEdges = [(p1, ["Edge1"]), (p2, ["Edge1"])]
        gordon.ProfileDirections = [False]

        gordon.GuideEdges = [(g1, ["Edge1"]), (g2, ["Edge1"])]
        gordon.GuideDirections = [False, False]

        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status:", status)

        self.assertFalse(
            gordon.isValid(), "Gordon surface should be invalid with mismatched direction count"
        )
        self.assertIn("Inconsistent number of edges", status)

    def _make_closed_bspline(self, z, radius=10):
        """Create a closed BSpline circle-like curve."""
        points = [
            vec(radius, 0, z),
            vec(0, radius, z),
            vec(-radius, 0, z),
            vec(0, -radius, z),
            vec(radius, 0, z),
        ]

        curve = Part.BSplineCurve()
        curve.interpolate(points)

        return curve.toShape()

    def _make_closed_bspline_shifted(self, z, start_index, radius=10):
        """Create a closed BSpline circle-like curve with a different seam."""
        points = [
            vec(radius, 0, z),
            vec(0, radius, z),
            vec(-radius, 0, z),
            vec(0, -radius, z),
        ]

        points = points[start_index:] + points[:start_index]
        points.append(points[0])

        curve = Part.BSplineCurve()
        curve.interpolate(points)

        return curve.toShape()

    def _make_periodic_bspline_circle(self, z, seam_shift=0, radius=10):
        """Create a periodic C2 BSpline circle-like curve with adjustable seam."""

        points = []
        count = 8

        import math

        for i in range(count):
            a = 2.0 * math.pi * (i + seam_shift) / count
            points.append(vec(radius * math.cos(a), radius * math.sin(a), z))

        curve = Part.BSplineCurve()

        # Periodic interpolation is the important part here
        curve.interpolate(points, True, 1.0e-7)  # periodic

        return curve.toShape()

    def test_create_gordon_surface_with_closed_profiles(self):
        """Gordon surface with two closed BSpline profiles and two guides."""

        profile1 = self.doc.addObject("Part::Feature", "Profile1")
        profile1.Shape = self._make_periodic_bspline_circle(0)

        profile2 = self.doc.addObject("Part::Feature", "Profile2")
        profile2.Shape = self._make_periodic_bspline_circle(10)

        # Create guides connecting matching seam points.
        guide1 = self.doc.addObject("Part::Feature", "Guide1")
        guide1.Shape = Part.makeLine(vec(10, 0, 0), vec(10, 0, 10))

        guide2 = self.doc.addObject("Part::Feature", "Guide2")
        guide2.Shape = Part.makeLine(vec(-10, 0, 0), vec(-10, 0, 10))

        gordon = self.doc.addObject("Surface::GordonSurface", "Gordon")

        gordon.ProfileEdges = [(profile1, ["Edge1"]), (profile2, ["Edge1"])]
        gordon.ProfileDirections = [False, False]

        gordon.GuideEdges = [(guide1, ["Edge1"]), (guide2, ["Edge1"])]
        gordon.GuideDirections = [False, False]

        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status:", status)

        self.assertTrue(gordon.isValid(), "Gordon surface is not valid: " + status)

        self.assertIsNotNone(gordon.Shape, "Gordon surface has no Shape")

        self.assertFalse(gordon.Shape.isNull(), "Gordon surface Shape is null")

        first_area = gordon.Shape.Area

        self.assertGreater(first_area, 0, "Gordon surface area should be positive")

        # Regression: recompute must produce the same result
        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status after recompute:", status)

        self.assertTrue(gordon.isValid(), "Gordon surface is not valid after recompute: " + status)

        self.assertAlmostEqual(gordon.Shape.Area, first_area)

    def test_create_gordon_surface_with_closed_profiles_different_seams(self):
        """Gordon surface with closed profiles where only one profile seam matches guides."""

        profile1 = self.doc.addObject("Part::Feature", "Profile1")
        profile1.Shape = self._make_closed_bspline(0)

        # Seam shifted 90 degrees compared to profile1.
        profile2 = self.doc.addObject("Part::Feature", "Profile2")
        profile2.Shape = self._make_closed_bspline_shifted(10, 1)

        # Guides intersect profile1 at its seam:
        # profile1 seam is (10,0,z)
        guide1 = self.doc.addObject("Part::Feature", "Guide1")
        guide1.Shape = Part.makeLine(vec(10, 0, 0), vec(10, 0, 10))

        # Opposite side guide
        guide2 = self.doc.addObject("Part::Feature", "Guide2")
        guide2.Shape = Part.makeLine(vec(-10, 0, 0), vec(-10, 0, 10))

        gordon = self.doc.addObject("Surface::GordonSurface", "Gordon")

        gordon.ProfileEdges = [(profile1, ["Edge1"]), (profile2, ["Edge1"])]
        gordon.ProfileDirections = [False, True]

        gordon.GuideEdges = [(guide1, ["Edge1"]), (guide2, ["Edge1"])]
        gordon.GuideDirections = [False, False]

        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status:", status)

        # should fail - cannot move seam
        self.assertFalse(gordon.isValid(), "Gordon surface is not valid: " + status)

        self.assertTrue(status == "Cannot move seam of BSpline with tangent discontinuity.")

    def test_create_gordon_surface_with_closed_periodic_profiles_different_seams(self):
        """Gordon surface with closed profiles where only one profile seam matches guides."""

        profile1 = self.doc.addObject("Part::Feature", "Profile1")
        profile1.Shape = self._make_periodic_bspline_circle(0)

        # Seam shifted 90 degrees compared to profile1.
        profile2 = self.doc.addObject("Part::Feature", "Profile2")
        profile2.Shape = self._make_periodic_bspline_circle(10, 2)

        # Guides intersect profile1 at its seam:
        # profile1 seam is (10,0,z)
        guide1 = self.doc.addObject("Part::Feature", "Guide1")
        guide1.Shape = Part.makeLine(vec(10, 0, 0), vec(10, 0, 10))

        # Opposite side guide
        guide2 = self.doc.addObject("Part::Feature", "Guide2")
        guide2.Shape = Part.makeLine(vec(-10, 0, 0), vec(-10, 0, 10))

        gordon = self.doc.addObject("Surface::GordonSurface", "Gordon")

        gordon.ProfileEdges = [(profile1, ["Edge1"]), (profile2, ["Edge1"])]
        gordon.ProfileDirections = [False, False]

        gordon.GuideEdges = [(guide1, ["Edge1"]), (guide2, ["Edge1"])]
        gordon.GuideDirections = [False, False]

        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status:", status)

        self.assertTrue(gordon.isValid(), "Gordon surface is not valid: " + status)

        self.assertIsNotNone(gordon.Shape)
        self.assertFalse(gordon.Shape.isNull())

        first_area = gordon.Shape.Area
        self.assertGreater(first_area, 0)

        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status after recompute:", status)

        self.assertTrue(gordon.isValid(), "Gordon surface is not valid after recompute: " + status)

        self.assertAlmostEqual(gordon.Shape.Area, first_area)

        # flip profiles - still valid surface but with self intersection
        gordon.ProfileDirections = [False, True]

        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status:", status)

        self.assertTrue(gordon.isValid(), "Gordon surface is not valid: " + status)

        self.assertGreater(gordon.Shape.Area, 0)

        self.assertFalse(
            gordon.Shape.Area == first_area,
            "Flipped surface area should not match original surface area.",
        )

    def test_create_native_gordon_surface_with_closed_periodic_profiles_different_seams(self):
        """Gordon surface with closed profiles where only one profile seam matches guides."""

        profile1 = self.doc.addObject("Part::Feature", "Profile1")
        profile1.Shape = self._make_periodic_bspline_circle(0)

        # Seam shifted 90 degrees compared to profile1.
        profile2 = self.doc.addObject("Part::Feature", "Profile2")
        profile2.Shape = self._make_periodic_bspline_circle(10, 2)

        # Guides intersect profile1 at its seam:
        # profile1 seam is (10,0,z)
        guide1 = self.doc.addObject("Part::Feature", "Guide1")
        guide1.Shape = Part.makeLine(vec(10, 0, 0), vec(10, 0, 10))

        # Opposite side guide
        guide2 = self.doc.addObject("Part::Feature", "Guide2")
        guide2.Shape = Part.makeLine(vec(-10, 0, 0), vec(-10, 0, 10))

        gordon = self.doc.addObject("Surface::GordonSurface", "Gordon")

        gordon.ProfileEdges = [(profile1, ["Edge1"]), (profile2, ["Edge1"])]
        gordon.ProfileDirections = [False, False]

        gordon.GuideEdges = [(guide1, ["Edge1"]), (guide2, ["Edge1"])]
        gordon.GuideDirections = [False, False]

        # test native OCCT algo
        gordon.UseNativeAlgorithm = True

        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status:", status)

        self.assertTrue(gordon.isValid(), "Gordon surface is not valid: " + status)

        self.assertIsNotNone(gordon.Shape)
        self.assertFalse(gordon.Shape.isNull())

        first_area = gordon.Shape.Area
        self.assertGreater(first_area, 0)

        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status after recompute:", status)

        self.assertTrue(gordon.isValid(), "Gordon surface is not valid after recompute: " + status)

        self.assertAlmostEqual(gordon.Shape.Area, first_area)

        # flip profiles - still valid surface but with self intersection
        gordon.ProfileDirections = [False, True]

        self.doc.recompute()

        status = gordon.getStatusString()
        print("Status:", status)

        self.assertTrue(gordon.isValid(), "Gordon surface is not valid: " + status)

        self.assertGreater(gordon.Shape.Area, 0)

        self.assertFalse(
            gordon.Shape.Area == first_area,
            "Flipped surface area should not match original surface area.",
        )
