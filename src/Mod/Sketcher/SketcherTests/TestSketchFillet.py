# **************************************************************************
#   Copyright (c) 2021 Emmanuel O'Brien                                   *
#                                                                         *
#   This file is part of the FreeCAD CAx development system.              *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU Lesser General Public License (LGPL)    *
#   as published by the Free Software Foundation; either version 2 of     *
#   the License, or (at your option) any later version.                   *
#   for detail see the LICENCE text file.                                 *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful,            *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU Library General Public License for more details.                  *
#                                                                         *
#   You should have received a copy of the GNU Library General Public     *
#   License along with FreeCAD; if not, write to the Free Software        *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#   USA                                                                   *
# **************************************************************************


import FreeCAD, math, os, sys, unittest, Part, Sketcher
from .TestSketcherSolver import CreateRectangleSketch

App = FreeCAD


def VShape(SketchFeature):
    # Simple inverted V shape
    SketchFeature.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(1, 1, 0)))
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 0, 1, 0, 2, 1))
    SketchFeature.addGeometry(Part.LineSegment(App.Vector(1, 1, 0), App.Vector(2, 0, 0)))
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 1, 1, 0, 2))
    SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 1, 1, 1, 2, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 1, 1, 1, 2, -1))
    SketchFeature.fillet(0, 2, 0.25, True, True)


def BoxCircle(SketchFeature):
    # Square with a circle centered at the upper right corner
    top_edge = int(SketchFeature.GeometryCount)
    right_edge = top_edge + 1
    CreateRectangleSketch(SketchFeature, [0, 0], [2, 2])
    top_midpoint = App.Vector(1, 2, 0)
    right_midpoint = App.Vector(2, 1, 0)
    circle = int(SketchFeature.GeometryCount)
    SketchFeature.addGeometry(Part.Circle(App.Vector(0, 0, 0), App.Vector(0, 0, 1), 1), False)
    SketchFeature.addConstraint(Sketcher.Constraint("Radius", circle, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", top_edge, 2, circle, 3))
    # Since the circle center is coincident with the corner, there are three coincident points
    # and thus the simpler fillet() call would get confused. Instead we'll need to point at the two
    # lines and their midpoints
    SketchFeature.fillet(top_edge, right_edge, top_midpoint, right_midpoint, 0.25, True, True)


def PointOnObject(SketchFeature):
    # Square with the upper right corner touching the edge of a circle
    top_edge = int(SketchFeature.GeometryCount)
    right_edge = top_edge + 1
    CreateRectangleSketch(SketchFeature, [0, 0], [2, 2])
    circle = int(SketchFeature.GeometryCount)
    SketchFeature.addGeometry(Part.Circle(App.Vector(12, 3, 0), App.Vector(0, 0, 1), 1), False)
    SketchFeature.addConstraint(Sketcher.Constraint("Radius", circle, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("PointOnObject", top_edge, 2, circle))
    SketchFeature.fillet(top_edge, 2, 0.25, True, True)


class TestSketchFillet(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("TestSketchFillet")

    def testBasicFillet(self):
        SketchFeature = self.Doc.addObject("Sketcher::SketchObject", "BasicFillet")
        CreateRectangleSketch(SketchFeature, [0, 0], [2, 2])
        SketchFeature.fillet(0, 2, 0.25, True, True)
        self.assertAlmostEqual(SketchFeature.Geometry[4].Radius, 0.25)

    # Fillets can be made even between unconnected lines
    def testUnconnected(self):
        SketchFeature = self.Doc.addObject("Sketcher::SketchObject", "Unconnected")
        # Inverted open V
        SketchFeature.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(1, 1, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 0, 1, 0, 2, 1))
        SketchFeature.addGeometry(Part.LineSegment(App.Vector(2.1, 1, 0), App.Vector(3, 0, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 1, 1, 1, 2, 0.9))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 1, 1, 1, 2, -1))

        SketchFeature.addGeometry(Part.LineSegment(App.Vector(3, 3, 0), App.Vector(5, 3, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("Equal", 0, 2))
        self.Doc.recompute()
        self.assertAlmostEqual(SketchFeature.Geometry[2].length(), math.sqrt(2))

        SketchFeature.fillet(
            0, 1, App.Vector(0.5, 0.5, 0), App.Vector(2.55, 0.5, 0), 0.25, True, True
        )
        # Make sure a fillet was created
        self.assertAlmostEqual(SketchFeature.Geometry[3].Radius, 0.25)

        self.Doc.recompute()
        # Third line's length shouldn't have changed
        self.assertAlmostEqual(SketchFeature.Geometry[2].length(), math.sqrt(2))
        # First line should be shorter
        self.assertNotAlmostEqual(SketchFeature.Geometry[0].length(), math.sqrt(2))

    # Curved lines can also be filleted
    def testCurve(self):
        SketchFeature = self.Doc.addObject("Sketcher::SketchObject", "Curve")
        SketchFeature.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(1, 1, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 0, 1, 0, 2, 1))
        arc = Part.ArcOfCircle(Part.Circle(App.Vector(3, 0, 0), App.Vector(0, 0, 1), 3), 0, -1)
        SketchFeature.addGeometry(arc)
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 1, 3, -1, 1, -3))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 1, 3, -1, 1, 0))
        SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 1, 1, 0, 2))
        self.Doc.recompute()

        SketchFeature.fillet(
            0, 1, App.Vector(0.5, 0.5, 0), App.Vector(0.8, 0.3, 0), 0.25, True, True
        )
        self.assertAlmostEqual(SketchFeature.Geometry[2].Radius, 0.25)

    def testUnconnectedCurve(self):
        SketchFeature = self.Doc.addObject("Sketcher::SketchObject", "UnconnectedCurve")
        SketchFeature.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(1, 1, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 0, 1, 0, 2, 1))
        arc = Part.ArcOfCircle(
            Part.Circle(App.Vector(3, 1, 0), App.Vector(0, 0, 1), 1.75), -3.14, -2.17
        )
        SketchFeature.addGeometry(arc)
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", -1, 1, 1, 3, 3.0))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", -1, 1, 1, 3, 1.0))
        SketchFeature.addConstraint(Sketcher.Constraint("Distance", 0, 2, 1, 1, 0.25))
        self.Doc.recompute()

        # SketchFeature.fillet(0,1, App.Vector(0.75,0.75,0), App.Vector(1.22,0.66,0), 0.25, True, True)
        # Make sure the fillet happened
        # self.Doc.recompute()
        # self.assertAlmostEqual(SketchFeature.Geometry[2].Radius, 0.25)

    # The following tests are mostly about verifying that transferFilletConstraints
    # does the right thing with pre-existing constraints when a fillet is created.

    # Make sure the original corner is preserved when filleting
    def testOriginalCorner(self):
        SketchFeature = self.Doc.addObject("Sketcher::SketchObject", "OriginalCorner")
        VShape(SketchFeature)
        self.Doc.recompute()
        # If fillet() ever gets refactored such that the corner gets added after
        # the arc, then getPoint(3,1) might break.  A more general approach would
        # be to iterate over the geometry list and find the bare vertex.
        self.assertAlmostEqual(App.Vector(1, 1, 0), SketchFeature.getPoint(3, 1))

    # Make sure coincident constraints get moved to the old corner location
    def testCoincident(self):
        SketchFeature = self.Doc.addObject("Sketcher::SketchObject", "Coincident")
        BoxCircle(SketchFeature)
        self.Doc.recompute()
        # Make sure the circle center is still at the old corner
        self.assertAlmostEqual(
            App.Vector(2, 2, 0).distanceToPoint(SketchFeature.getPoint(4, 3)), 0.0
        )

    # Point-to-point horizontal and vertical constraints should also move to the old corner
    def testHorizontalVertical(self):
        SketchFeature = self.Doc.addObject("Sketcher::SketchObject", "HorizontalVertical")
        # Inverted V
        SketchFeature.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(1, 1, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 0, 1, 0, 2, 1))
        SketchFeature.addGeometry(Part.LineSegment(App.Vector(1, 1, 0), App.Vector(2, 0, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 1, 1, 0, 2))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 1, 1, 1, 2, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 1, 1, 1, 2, -1))

        SketchFeature.addGeometry(Part.Point(App.Vector(2, 1)))
        SketchFeature.addConstraint(Sketcher.Constraint("Horizontal", 0, 2, 2, 1))
        SketchFeature.addGeometry(Part.Point(App.Vector(1, 2)))
        SketchFeature.addConstraint(Sketcher.Constraint("Vertical", 1, 1, 3, 1))

        SketchFeature.fillet(0, 2, 0.25, True, True)

        # Verify the constraint moved to the original corner
        found_horizontal = False
        found_vertical = False
        for c in SketchFeature.Constraints:
            if (
                c.Type == "Horizontal"
                and c.First == 5
                and c.FirstPos == 1
                and c.Second == 2
                and c.SecondPos == 1
            ):
                found_horizontal = True
            elif (
                c.Type == "Vertical"
                and c.First == 5
                and c.FirstPos == 1
                and c.Second == 3
                and c.SecondPos == 1
            ):
                found_vertical = True

        self.assertTrue(found_horizontal)
        self.assertTrue(found_vertical)

    # Distance constraints to the old corner point should be preserved
    def testDistance(self):
        SketchFeature = self.Doc.addObject("Sketcher::SketchObject", "Distance")
        # We'll end up implicitly testing line length constraints as well since that's
        # what CreateRectangleSketch uses to enforce side length.  If the side length doesn't
        # switch to a point-to-point distance constraint with the original corner as expected,
        # the point won't end up at its expected destination.
        CreateRectangleSketch(SketchFeature, [0, 0], [2, 2])

        point = int(SketchFeature.GeometryCount)
        SketchFeature.addGeometry(Part.Point(App.Vector(3, 2, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 0, 2, point, 1, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 0, 2, point, 1, 0))

        self.Doc.recompute()
        self.assertTrue(SketchFeature.FullyConstrained)
        SketchFeature.fillet(0, 2, 0.25, True, True)
        SketchFeature.addConstraint(Sketcher.Constraint("Radius", 5, 0.25))
        self.Doc.recompute()
        # If any constraints disappeared then we won't be fully constrained
        self.assertTrue(SketchFeature.FullyConstrained)
        # Make sure the point is to the right of the original corner as expected
        self.assertAlmostEqual(SketchFeature.getPoint(point, 1), App.Vector(3, 2, 0))

    # Make sure point on object constraints get moved to the old corner location
    def testPointOnObject(self):
        SketchFeature = self.Doc.addObject("Sketcher::SketchObject", "PointOnObject")
        PointOnObject(SketchFeature)
        self.Doc.recompute()
        # Make sure the circle center is one radius away from the old corner
        self.assertAlmostEqual(
            App.Vector(2, 2, 0).distanceToPoint(SketchFeature.getPoint(4, 3)), 1.0
        )

    # Make sure colinearity doesn't get dropped
    def testTangent(self):
        SketchFeature = self.Doc.addObject("Sketcher::SketchObject", "Tangent")

        # Setup the geometry
        first_line = int(SketchFeature.GeometryCount)
        SketchFeature.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(1, 1, 0)))
        # Anchor at the origin
        SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", first_line, 2, -1, 1, -1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", first_line, 2, -1, 1, -1))

        second_line = int(SketchFeature.GeometryCount)
        SketchFeature.addGeometry(Part.LineSegment(App.Vector(1, 1, 0), App.Vector(2, 0, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", second_line, 2, -1, 1, -2))
        SketchFeature.addConstraint(
            Sketcher.Constraint("Coincident", first_line, 2, second_line, 1)
        )
        tangent_line = int(SketchFeature.GeometryCount)
        SketchFeature.addGeometry(Part.LineSegment(App.Vector(2, 2, 0), App.Vector(3, 3, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("Tangent", tangent_line, first_line))
        SketchFeature.addConstraint(Sketcher.Constraint("Distance", tangent_line, 1.41421356237))
        SketchFeature.fillet(first_line, 2, 0.25, True, True)

        self.Doc.recompute()
        # Move the tangent line and see if it's aimed right
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", tangent_line, 1, -1, 1, -4))
        self.Doc.recompute()

        # The first endpoint should now be at 4,4
        self.assertAlmostEqual(
            App.Vector(4, 4, 0).distanceToPoint(SketchFeature.getPoint(tangent_line, 1)),
            0.0,
        )

        # We expect the other end of the tangent line to be at 5,5, but I think 3,3 also satisfies
        # the colinearity constraint
        try:
            self.assertAlmostEqual(
                App.Vector(3, 3, 0).distanceToPoint(SketchFeature.getPoint(tangent_line, 2)),
                0.0,
            )
        except AssertionError:
            self.assertAlmostEqual(
                App.Vector(5, 5, 0).distanceToPoint(SketchFeature.getPoint(tangent_line, 2)),
                0.0,
            )

    def testSymmetric(self):
        SketchFeature = self.Doc.addObject("Sketcher::SketchObject", "Symmetric")
        # Inverted V
        SketchFeature.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(1, 1, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 0, 1, 0, 2, 1))
        SketchFeature.addGeometry(Part.LineSegment(App.Vector(1, 1, 0), App.Vector(2, 0, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 1, 1, 0, 2))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 1, 1, 1, 2, 1))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 1, 1, 1, 2, -1))

        # Mirror point
        SketchFeature.addGeometry(Part.Point(App.Vector(3, 2, 0)))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", -1, 1, 2, 1, 3))
        SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", -1, 1, 2, 1, 2))

        # Point that will mirror the apex of the V
        SketchFeature.addGeometry(Part.Point(App.Vector(4, 2, 0)))

        SketchFeature.addConstraint(Sketcher.Constraint("Symmetric", 0, 2, 3, 1, 2, 1))
        self.Doc.recompute()

        SketchFeature.fillet(0, 2, 0.25, True, True)
        self.Doc.recompute()
        self.assertAlmostEqual(
            App.Vector(5, 3, 0).distanceToPoint(SketchFeature.getPoint(3, 1)), 0.0
        )

    def tearDown(self):
        # comment out to omit closing document for debugging
        FreeCAD.closeDocument("TestSketchFillet")
        pass
