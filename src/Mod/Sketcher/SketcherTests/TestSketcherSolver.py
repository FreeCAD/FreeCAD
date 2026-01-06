# **************************************************************************
#   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


import os, tempfile, unittest
import FreeCAD, Part, Sketcher
from Part import Precision

App = FreeCAD

xy_normal = FreeCAD.Vector(0, 0, 1)


def vec(x, y):
    """Shorthand to create a vector in the XY-plane"""
    return FreeCAD.Vector(x, y, 0)


def CreateRectangleSketch(SketchFeature, corner, lengths):
    hmin, hmax = corner[0], corner[0] + lengths[0]
    vmin, vmax = corner[1], corner[1] + lengths[1]

    # add the geometry and grab the count offset
    i = int(SketchFeature.GeometryCount)
    SketchFeature.addGeometry(
        Part.LineSegment(FreeCAD.Vector(hmin, vmax), FreeCAD.Vector(hmax, vmax, 0))
    )
    SketchFeature.addGeometry(
        Part.LineSegment(FreeCAD.Vector(hmax, vmax, 0), FreeCAD.Vector(hmax, vmin, 0))
    )
    SketchFeature.addGeometry(
        Part.LineSegment(FreeCAD.Vector(hmax, vmin, 0), FreeCAD.Vector(hmin, vmin, 0))
    )
    SketchFeature.addGeometry(
        Part.LineSegment(FreeCAD.Vector(hmin, vmin, 0), FreeCAD.Vector(hmin, vmax, 0))
    )

    # add the rectangular constraints
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", i + 0, 2, i + 1, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", i + 1, 2, i + 2, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", i + 2, 2, i + 3, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", i + 3, 2, i + 0, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Horizontal", i + 0))
    SketchFeature.addConstraint(Sketcher.Constraint("Horizontal", i + 2))
    SketchFeature.addConstraint(Sketcher.Constraint("Vertical", i + 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Vertical", i + 3))

    # Fix the bottom left corner of the rectangle
    SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", i + 2, 2, corner[0]))
    SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", i + 2, 2, corner[1]))

    # add dimensions
    if lengths[0] == lengths[1]:
        SketchFeature.addConstraint(Sketcher.Constraint("Equal", i + 2, i + 3))
        SketchFeature.addConstraint(Sketcher.Constraint("Distance", i + 0, hmax - hmin))
    else:
        SketchFeature.addConstraint(Sketcher.Constraint("Distance", i + 1, vmax - vmin))
        SketchFeature.addConstraint(Sketcher.Constraint("Distance", i + 0, hmax - hmin))


def CreateCircleSketch(SketchFeature, center, radius):
    i = int(SketchFeature.GeometryCount)
    SketchFeature.addGeometry(Part.Circle(App.Vector(*center), App.Vector(0, 0, 1), radius), False)
    SketchFeature.addConstraint(Sketcher.Constraint("Radius", i, radius))
    SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", i, 3, center[0]))
    SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", i, 3, center[1]))


def CreateBoxSketchSet(SketchFeature):
    SketchFeature.addGeometry(
        Part.LineSegment(
            FreeCAD.Vector(-99.230339, 36.960674, 0),
            FreeCAD.Vector(69.432587, 36.960674, 0),
        )
    )
    SketchFeature.addGeometry(
        Part.LineSegment(
            FreeCAD.Vector(69.432587, 36.960674, 0),
            FreeCAD.Vector(69.432587, -53.196629, 0),
        )
    )
    SketchFeature.addGeometry(
        Part.LineSegment(
            FreeCAD.Vector(69.432587, -53.196629, 0),
            FreeCAD.Vector(-99.230339, -53.196629, 0),
        )
    )
    SketchFeature.addGeometry(
        Part.LineSegment(
            FreeCAD.Vector(-99.230339, -53.196629, 0),
            FreeCAD.Vector(-99.230339, 36.960674, 0),
        )
    )
    # add the constraints
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Horizontal", 0))
    SketchFeature.addConstraint(Sketcher.Constraint("Horizontal", 2))
    SketchFeature.addConstraint(Sketcher.Constraint("Vertical", 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Vertical", 3))
    # add dimensions
    SketchFeature.addConstraint(Sketcher.Constraint("Distance", 1, 81.370787))
    SketchFeature.addConstraint(Sketcher.Constraint("Distance", 0, 187.573036))


def CreateSlotPlateSet(SketchFeature):
    SketchFeature.addGeometry(
        Part.LineSegment(
            App.Vector(60.029362, -30.279360, 0), App.Vector(-120.376335, -30.279360, 0)
        )
    )
    SketchFeature.addConstraint(Sketcher.Constraint("Horizontal", 0))
    SketchFeature.addGeometry(
        Part.LineSegment(
            App.Vector(-120.376335, -30.279360, 0), App.Vector(-70.193062, 38.113884, 0)
        )
    )
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
    SketchFeature.addGeometry(
        Part.LineSegment(App.Vector(-70.193062, 38.113884, 0), App.Vector(60.241116, 37.478645, 0))
    )
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Horizontal", 2))
    SketchFeature.addGeometry(
        Part.ArcOfCircle(
            Part.Circle(App.Vector(60.039921, 3.811391, 0), App.Vector(0, 0, 1), 35.127132),
            -1.403763,
            1.419522,
        )
    )
    SketchFeature.addConstraint(Sketcher.Constraint("Tangent", 2, 2, 3, 2))
    SketchFeature.addConstraint(Sketcher.Constraint("Tangent", 0, 1, 3, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Angle", 0, 2, 1, 1, 0.947837))
    SketchFeature.addConstraint(Sketcher.Constraint("Distance", 0, 184.127425))
    SketchFeature.setDatum(7, 200.000000)
    SketchFeature.addConstraint(Sketcher.Constraint("Radius", 3, 38.424808))
    SketchFeature.setDatum(8, 40.000000)
    SketchFeature.setDatum(6, 0.872665)
    SketchFeature.addConstraint(Sketcher.Constraint("DistanceX", 0, 2, 0.0))
    SketchFeature.setDatum(9, 0.000000)
    SketchFeature.moveGeometry(0, 2, App.Vector(-0.007829, -33.376450, 0))
    SketchFeature.moveGeometry(0, 2, App.Vector(-0.738149, -10.493386, 0))
    SketchFeature.moveGeometry(0, 2, App.Vector(-0.007829, 2.165328, 0))
    SketchFeature.addConstraint(Sketcher.Constraint("DistanceY", 0, 2, 2.165328))
    SketchFeature.setDatum(10, 0.000000)


def CreateSlotPlateInnerSet(SketchFeature):
    SketchFeature.addGeometry(
        Part.Circle(App.Vector(195.055893, 39.562252, 0), App.Vector(0, 0, 1), 29.846098)
    )
    SketchFeature.addGeometry(
        Part.LineSegment(App.Vector(150.319031, 13.449363, 0), App.Vector(36.700474, 13.139774, 0))
    )
    SketchFeature.addConstraint(Sketcher.Constraint("Horizontal", 5))
    SketchFeature.addGeometry(
        Part.LineSegment(App.Vector(36.700474, 13.139774, 0), App.Vector(77.566010, 63.292927, 0))
    )
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 5, 2, 6, 1))
    SketchFeature.addGeometry(
        Part.LineSegment(App.Vector(77.566010, 63.292927, 0), App.Vector(148.151917, 63.602505, 0))
    )
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 6, 2, 7, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Horizontal", 7))
    SketchFeature.addConstraint(Sketcher.Constraint("Parallel", 1, 6))
    SketchFeature.addGeometry(
        Part.ArcOfCircle(
            Part.Circle(App.Vector(192.422913, 38.216347, 0), App.Vector(0, 0, 1), 45.315174),
            2.635158,
            3.602228,
        )
    )
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 7, 2, 8, 1))
    SketchFeature.addConstraint(Sketcher.Constraint("Coincident", 8, 2, 5, 1))


def CreateThreeLinesWithCommonPoint(SketchFeature):
    SketchFeature.addGeometry(
        Part.LineSegment(
            App.Vector(-55.965607, -9.864289, 0), App.Vector(-55.600571, -9.387639, 0)
        ),
        False,
    )
    SketchFeature.addGeometry(
        Part.LineSegment(
            App.Vector(-55.735817, -9.067246, 0), App.Vector(-55.600571, -9.387639, 0)
        ),
        False,
    )
    SketchFeature.addGeometry(
        Part.LineSegment(
            App.Vector(-55.600571, -9.387639, 0), App.Vector(-55.058266, -9.677831, 0)
        ),
        False,
    )


# ---------------------------------------------------------------------------
# define the test cases to test the FreeCAD Sketcher module
# ---------------------------------------------------------------------------


class TestSketcherSolver(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("SketchSolverTest")

    def testBoxCase(self):
        self.Box = self.Doc.addObject("Sketcher::SketchObject", "SketchBox")
        CreateBoxSketchSet(self.Box)
        self.Doc.recompute()
        # moving a point of the sketch
        self.Box.moveGeometry(0, 2, App.Vector(88.342697, 28.174158, 0))
        # fully constrain
        self.Box.addConstraint(Sketcher.Constraint("DistanceX", 1, 2, 90.0))
        self.Box.addConstraint(Sketcher.Constraint("DistanceY", 1, 2, -50.0))
        self.Doc.recompute()

    def testSlotCase(self):
        self.Slot = self.Doc.addObject("Sketcher::SketchObject", "SketchSlot")
        CreateSlotPlateSet(self.Slot)
        self.Doc.recompute()
        # test if all edges created
        self.assertTrue(len(self.Slot.Shape.Edges) == 4)
        CreateSlotPlateInnerSet(self.Slot)
        self.Doc.recompute()
        self.assertTrue(len(self.Slot.Shape.Edges) == 9)

    def testIssue3245(self):
        self.Doc2 = FreeCAD.newDocument("Issue3245")
        self.Doc2.addObject("Sketcher::SketchObject", "Sketch")
        self.Doc2.Sketch.Placement = App.Placement(
            App.Vector(0.000000, 0.000000, 0.000000),
            App.Rotation(0.000000, 0.000000, 0.000000, 1.000000),
        )
        self.Doc2.Sketch.MapMode = "Deactivated"
        self.Doc2.Sketch.addGeometry(
            Part.LineSegment(
                App.Vector(-1.195999, 56.041161, 0), App.Vector(60.654316, 56.382877, 0)
            ),
            False,
        )
        self.Doc2.Sketch.addConstraint(Sketcher.Constraint("PointOnObject", 0, 1, -2))
        self.Doc2.Sketch.addConstraint(Sketcher.Constraint("Horizontal", 0))
        self.Doc2.Sketch.addGeometry(
            Part.LineSegment(
                App.Vector(0.512583, 32.121155, 0), App.Vector(60.654316, 31.779440, 0)
            ),
            False,
        )
        self.Doc2.Sketch.addConstraint(Sketcher.Constraint("Horizontal", 1))
        self.Doc2.Sketch.addGeometry(
            Part.LineSegment(
                App.Vector(0.170867, 13.326859, 0), App.Vector(61.679455, 13.326859, 0)
            ),
            False,
        )
        self.Doc2.Sketch.addConstraint(Sketcher.Constraint("PointOnObject", 2, 1, -2))
        self.Doc2.Sketch.addConstraint(Sketcher.Constraint("Horizontal", 2))
        self.Doc2.Sketch.addConstraint(Sketcher.Constraint("PointOnObject", 1, 1, -2))
        self.Doc2.Sketch.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 60.654316))
        self.Doc2.Sketch.setExpression("Constraints[6]", "60")
        self.Doc2.Sketch.addConstraint(Sketcher.Constraint("DistanceX", 1, 1, 1, 2, 60.654316))
        self.Doc2.Sketch.setExpression("Constraints[7]", "65")
        self.Doc2.Sketch.addConstraint(Sketcher.Constraint("DistanceX", 2, 1, 2, 2, 61.679455))
        self.Doc2.Sketch.setExpression("Constraints[8]", "70")
        self.Doc2.recompute()
        self.Doc2.Sketch.delGeometry(2)
        values = d = {key: value for (key, value) in self.Doc2.Sketch.ExpressionEngine}
        self.assertTrue(values["Constraints[4]"] == "60")
        self.assertTrue(values["Constraints[5]"] == "65")
        FreeCAD.closeDocument("Issue3245")

    def testIssue3245_2(self):
        self.Doc2 = FreeCAD.newDocument("Issue3245")
        ActiveSketch = self.Doc2.addObject("Sketcher::SketchObject", "Sketch")
        ActiveSketch.Placement = App.Placement(
            App.Vector(0.000000, 0.000000, 0.000000),
            App.Rotation(0.000000, 0.000000, 0.000000, 1.000000),
        )
        ActiveSketch.MapMode = "Deactivated"
        geoList = []
        geoList.append(
            Part.LineSegment(
                App.Vector(-23.574591, 42.399727, 0),
                App.Vector(81.949776, 42.399727, 0),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(81.949776, 42.399727, 0),
                App.Vector(81.949776, -19.256901, 0),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(81.949776, -19.256901, 0),
                App.Vector(-23.574591, -19.256901, 0),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(-23.574591, -19.256901, 0),
                App.Vector(-23.574591, 42.399727, 0),
            )
        )
        ActiveSketch.addGeometry(geoList, False)
        conList = []
        conList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        conList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        conList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        conList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        conList.append(Sketcher.Constraint("Horizontal", 0))
        conList.append(Sketcher.Constraint("Horizontal", 2))
        conList.append(Sketcher.Constraint("Vertical", 1))
        conList.append(Sketcher.Constraint("Vertical", 3))
        ActiveSketch.addConstraint(conList)
        ActiveSketch.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 105.524367))
        ActiveSketch.setExpression("Constraints[8]", "10 + 10")
        ActiveSketch.addConstraint(Sketcher.Constraint("DistanceY", 3, 1, 3, 2, 61.656628))
        ActiveSketch.setDatum(9, App.Units.Quantity("5.000000 mm"))
        ActiveSketch.delConstraint(8)
        values = d = {key: value for (key, value) in self.Doc2.Sketch.ExpressionEngine}
        self.Doc2.recompute()
        self.assertTrue(len(values) == 0)
        FreeCAD.closeDocument("Issue3245")

    def testBlockConstraintEllipse(self):
        self.Doc3 = FreeCAD.newDocument("BlockConstraintTests")
        ActiveSketch = self.Doc3.addObject("Sketcher::SketchObject", "Sketch")
        ActiveSketch.Placement = App.Placement(
            App.Vector(0.000000, 0.000000, 0.000000),
            App.Rotation(0.000000, 0.000000, 0.000000, 1.000000),
        )
        ActiveSketch.MapMode = "Deactivated"
        ActiveSketch.addGeometry(
            Part.Ellipse(
                App.Vector(-19.129438, 14.345055, 0),
                App.Vector(-33.806261, 12.085921, 0),
                App.Vector(-30.689360, 7.107538, 0),
            ),
            False,
        )
        ActiveSketch.solve()
        ActiveSketch.exposeInternalGeometry(0)
        ActiveSketch.solve()
        ActiveSketch.moveGeometry(0, 0, App.Vector(-26.266434, 14.345055, 0), 0)
        ActiveSketch.solve()
        ActiveSketch.addConstraint(Sketcher.Constraint("Block", 0))  # Block the Ellipse in place
        ActiveSketch.addConstraint(
            Sketcher.Constraint("Block", 1)
        )  # Block the major axis in place (on purpose)
        status = ActiveSketch.solve()
        self.assertTrue(status == 0)  # no redundants/conflicts/convergence issues
        ActiveSketch.addConstraint(
            Sketcher.Constraint("Distance", 1, 27.277350)
        )  # Length of major axis
        ActiveSketch.setDriving(
            6, True
        )  # ensure length is driving (because pre-existing block constraint on major axis)
        ActiveSketch.setDatum(6, App.Units.Quantity("28.000000 mm"))
        status = ActiveSketch.solve()
        self.assertTrue(status == 0)  # no redundants/conflicts/convergence issues
        ActiveSketch.addConstraint(
            Sketcher.Constraint("Distance", 2, 11.747233)
        )  # Length of minor axis
        ActiveSketch.setDatum(7, App.Units.Quantity("15.000000 mm"))
        ActiveSketch.solve()
        self.assertTrue(status == 0)  # no redundants/conflicts/convergence issues
        ActiveSketch.addConstraint(Sketcher.Constraint("Block", 2))
        ActiveSketch.solve()
        self.assertTrue(status == 0)  # no redundants/conflicts/convergence issues
        ActiveSketch.addConstraint(
            Sketcher.Constraint("Horizontal", 1)
        )  # Make major axis horizontal (together with horizontal and length driving constraints)
        ActiveSketch.solve()
        self.assertTrue(status == 0)  # no redundants/conflicts/convergence issues
        ActiveSketch.addConstraint(
            Sketcher.Constraint("DistanceX", 0, 3, -1, 1, 27.655024)
        )  # Locate Ellipse center
        ActiveSketch.addConstraint(Sketcher.Constraint("DistanceY", 0, 3, -1, 1, -20.877021))
        ActiveSketch.setDatum(10, App.Units.Quantity("25.000000 mm"))
        ActiveSketch.setDatum(11, App.Units.Quantity("-20.000000 mm"))
        ActiveSketch.solve()
        self.assertTrue(status == 0)  # no redundants/conflicts/convergence issues
        FreeCAD.closeDocument(self.Doc3.Name)

    def testThreeLinesWithCoincidences_1(self):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        CreateThreeLinesWithCommonPoint(sketch)
        sketch.addConstraint(Sketcher.Constraint("Coincident", 1, 2, 0, 2))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 2, 1, 0, 2))
        self.assertEqual(sketch.detectMissingPointOnPointConstraints(0.0001), 0)

    # Same as in testThreeLinesWithCoincidences_1 but set the constraints on
    # different lines
    def testThreeLinesWithCoincidences_2(self):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        CreateThreeLinesWithCommonPoint(sketch)
        sketch.addConstraint(Sketcher.Constraint("Coincident", 1, 2, 0, 2))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 2, 1, 1, 2))
        self.assertEqual(sketch.detectMissingPointOnPointConstraints(0.0001), 0)

    def testCircleToLineDistance_Driving_Passant(self):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        radius = 20
        circle = Part.Circle(vec(0, 0), xy_normal, radius)
        line = Part.LineSegment(vec(-radius, 2 * radius), vec(radius, 2 * radius))
        c_idx = sketch.addGeometry(circle)
        l_idx = sketch.addGeometry(line)
        # use a positive distance, other than the initial distance of the line
        wanted_distance = radius / 2
        sketch.addConstraint(Sketcher.Constraint("Distance", c_idx, l_idx, wanted_distance))
        self.assertSuccessfulSolve(sketch)
        c_shape = sketch.Geometry[c_idx].toShape()
        l_shape = sketch.Geometry[l_idx].toShape()
        self.assertShapeDistance(c_shape, l_shape, wanted_distance)

    @unittest.skip("Support for secants still under discussion, see comments in PR 9044")
    def testCircleToLineDistance_Driving_Secant(self):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        radius = 20
        c_idx = sketch.addGeometry(Part.Circle(vec(0, 0), xy_normal, radius))
        l_idx = sketch.addGeometry(
            Part.LineSegment(vec(-radius, 2 * radius), vec(radius, 2 * radius))
        )
        # use a negative distance to tell "line is within the circle"
        wanted_distance = -radius / 2
        sketch.addConstraint(Sketcher.Constraint("Distance", c_idx, l_idx, wanted_distance))
        self.assertSuccessfulSolve(sketch)
        c_shape = sketch.Geometry[c_idx].toShape()
        l_shape = sketch.Geometry[l_idx].toShape()
        self.assertShapeDistance(c_shape, l_shape, 0)  # secant intersects circle, thus no distance

    def testCircleToLineDistance_Reference_Secant(self):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        radius = 20
        c_idx = sketch.addGeometry(Part.Circle(vec(0, 0), xy_normal, radius))
        l_idx = sketch.addGeometry(
            Part.LineSegment(vec(-radius, radius / 2), vec(radius, radius / 2))
        )
        # The block constraints are required to ensure the geometry does not move.
        # Without this, the solver may find another valid solution than what we assert.
        sketch.addConstraint(
            [Sketcher.Constraint("Block", c_idx), Sketcher.Constraint("Block", l_idx)]
        )
        expected_distance = radius / 2  # note that we don't set this in the constraint below!
        # TODO: addConstraint(constraint) triggers a solve (for good reasons) however, this way
        # one cannot add non-driving constraints. In contrast, addConstraint(list(constraint))
        # does not solve automatically, thus we use this "overload".
        # Much nicer would be an addConstraint(constraint, isReference=False), like addGeometry
        dist_idx = sketch.addConstraint([Sketcher.Constraint("Distance", c_idx, l_idx, 0)])[0]
        sketch.setDriving(dist_idx, False)
        self.assertSuccessfulSolve(sketch)
        actual_distance = sketch.Constraints[dist_idx].Value
        self.assertAlmostEqual(
            expected_distance,
            actual_distance,
            delta=Precision.confusion(),
            msg="Reference constraint did not return the expected distance.",
        )

    def testCircleToLineDistance_Legacy_Negative(self):
        # compare a driving negative distance to an expected positive reference one
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        radius = 20
        c_idx = sketch.addGeometry(Part.Circle(vec(0, 0), xy_normal, radius))
        l_idx = sketch.addGeometry(
            Part.LineSegment(vec(-radius, radius / 2), vec(radius, radius / 2))
        )
        sketch.addConstraint(
            [Sketcher.Constraint("Block", c_idx), Sketcher.Constraint("Block", l_idx)]
        )
        expected_distance = radius / 2
        ref_idx = sketch.addConstraint(
            [
                Sketcher.Constraint("Distance", c_idx, l_idx, -expected_distance),
                Sketcher.Constraint("Distance", c_idx, l_idx, 0),
            ]
        )[1]
        sketch.setDriving(ref_idx, False)
        self.assertSuccessfulSolve(sketch)
        actual_distance = sketch.Constraints[ref_idx].Value
        self.assertAlmostEqual(
            expected_distance,
            actual_distance,
            delta=Precision.confusion(),
            msg="Negative length constraint did not return the expected distance.",
        )

    def testCircleToLineDistanceOriented(self):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        radius = 20

        c_idx = sketch.addGeometry(Part.Circle(vec(0, 0), xy_normal, radius))
        l_left_ext = sketch.addGeometry(
            Part.LineSegment(vec(-2 * radius, -20), vec(-2 * radius, 20))
        )
        l_left_int = sketch.addGeometry(
            Part.LineSegment(vec(-0.5 * radius, -20), vec(-0.5 * radius, 20))
        )
        l_right_ext = sketch.addGeometry(
            Part.LineSegment(vec(2 * radius, -20), vec(2 * radius, 20))
        )
        l_right_int = sketch.addGeometry(
            Part.LineSegment(vec(0.5 * radius, -20), vec(0.5 * radius, 20))
        )

        radius_idx = sketch.addConstraint(
            [
                Sketcher.Constraint("Coincident", c_idx, 3, -1, 1),  # constrain circle to origin
                Sketcher.Constraint("Vertical", l_left_ext),
                Sketcher.Constraint("Vertical", l_left_int),
                Sketcher.Constraint("Vertical", l_right_ext),
                Sketcher.Constraint("Vertical", l_right_int),
                Sketcher.Constraint("Distance", c_idx, l_left_ext, 0.25 * radius),
                Sketcher.Constraint("Distance", c_idx, l_left_int, 0.25 * radius),
                Sketcher.Constraint("Distance", c_idx, l_right_ext, 0.25 * radius),
                Sketcher.Constraint("Distance", c_idx, l_right_int, 0.25 * radius),
                Sketcher.Constraint("Radius", c_idx, radius),
            ]
        )[-1]

        self.assertSuccessfulSolve(sketch)

        self.assertLess(sketch.Geometry[l_left_ext].StartPoint.x, -radius)
        self.assertLess(sketch.Geometry[l_left_int].StartPoint.x, 0)
        self.assertGreater(sketch.Geometry[l_left_int].StartPoint.x, -radius)

        self.assertGreater(sketch.Geometry[l_right_ext].StartPoint.x, radius)
        self.assertGreater(sketch.Geometry[l_right_int].StartPoint.x, 0)
        self.assertLess(sketch.Geometry[l_right_int].StartPoint.x, radius)

        # Try to flip the sketch
        radius = 200.0
        sketch.setDatum(radius_idx, App.Units.Quantity("{} mm".format(radius)))

        self.assertSuccessfulSolve(sketch)

        self.assertLess(sketch.Geometry[l_left_ext].StartPoint.x, -radius)
        self.assertLess(sketch.Geometry[l_left_int].StartPoint.x, 0)
        self.assertGreater(sketch.Geometry[l_left_int].StartPoint.x, -radius)

        self.assertGreater(sketch.Geometry[l_right_ext].StartPoint.x, radius)
        self.assertGreater(sketch.Geometry[l_right_int].StartPoint.x, 0)
        self.assertLess(sketch.Geometry[l_right_int].StartPoint.x, radius)

    def testPointToLineDistanceSigned(self):
        # Test the signed p2l constraint by trying to flip the sketch
        # with a big constraint datum change

        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")

        lv_idx = sketch.addGeometry(Part.LineSegment(vec(0, -25), vec(0, 0)))
        lh_idx = sketch.addGeometry(Part.LineSegment(vec(0, 0), vec(25, 0)))
        ln_idx = sketch.addGeometry(Part.LineSegment(vec(0, -25), vec(25, 0)))
        pt_idx = sketch.addGeometry(Part.Point(vec(20, -20)))

        linepose_idx = sketch.addConstraint(
            [
                Sketcher.Constraint("PointOnObject", lv_idx, 1, -2),
                Sketcher.Constraint("Coincident", lv_idx, 2, -1, 1),
                Sketcher.Constraint("Coincident", lh_idx, 1, lv_idx, 2),
                Sketcher.Constraint("PointOnObject", lh_idx, 2, -1),
                Sketcher.Constraint("Coincident", ln_idx, 1, lv_idx, 1),
                Sketcher.Constraint("Coincident", ln_idx, 2, lh_idx, 2),
                Sketcher.Constraint("Equal", lv_idx, lh_idx),
                Sketcher.Constraint("DistanceX", lh_idx, 1, lh_idx, 2, 25.0),
                Sketcher.Constraint("Distance", pt_idx, 1, ln_idx, 5.0),
            ]
        )[7]

        self.assertSuccessfulSolve(sketch)

        sketch.setDatum(linepose_idx, App.Units.Quantity("100.000000 mm"))

        self.assertSuccessfulSolve(sketch)

        A = sketch.Geometry[pt_idx]
        ln = sketch.Geometry[ln_idx]
        B = ln.StartPoint
        C = ln.EndPoint

        ccw = B.x * C.y - B.y * C.x - A.X * C.y + A.Y * C.x + A.X * B.y - A.Y * B.x > 0.0

        self.assertEqual(ccw, False)

    def testCircleToCircleDistanceOriented(self):
        # Make a set of two circles a small and a big
        # the small is fully contained in the big
        # add a distance between the small and the big
        # then set the small's diameter to a big value
        # it should remain smaller
        # tests concentric and non concentric and both distance orders (small-big, big-small)

        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")

        sm1_idx = sketch.addGeometry(
            Part.Circle(App.Vector(-27.105705, 20.597332, 0.000000), xy_normal, 9.964653)
        )
        bg1_idx = sketch.addGeometry(
            Part.Circle(App.Vector(-27.105705, 20.597332, 0.000000), xy_normal, 19.924598)
        )

        sm2_idx = sketch.addGeometry(
            Part.Circle(App.Vector(-21.745829, -21.056572, 0.000000), xy_normal, 9.019634)
        )
        bg2_idx = sketch.addGeometry(
            Part.Circle(App.Vector(-27.871407, -22.434828, 0.000000), xy_normal, 18.524801)
        )

        sm3_idx = sketch.addGeometry(
            Part.Circle(App.Vector(27.565046, 20.750475, 0.000000), xy_normal, 12.809815)
        )
        bg3_idx = sketch.addGeometry(
            Part.Circle(App.Vector(27.565046, 20.750475, 0.000000), xy_normal, 22.553629)
        )

        sm4_idx = sketch.addGeometry(
            Part.Circle(App.Vector(20.980043, -21.822269, 0.000000), xy_normal, 10.010452)
        )
        bg4_idx = sketch.addGeometry(
            Part.Circle(App.Vector(29.249569, -24.272499, 0.000000), xy_normal, 22.875263)
        )

        sm_diam = sketch.addConstraint(
            [
                Sketcher.Constraint("Coincident", bg1_idx, 3, sm1_idx, 3),
                Sketcher.Constraint("Coincident", sm3_idx, 3, bg3_idx, 3),
                Sketcher.Constraint("Equal", sm1_idx, sm2_idx),
                Sketcher.Constraint("Equal", sm2_idx, sm3_idx),
                Sketcher.Constraint("Equal", sm3_idx, sm4_idx),
                Sketcher.Constraint("Diameter", sm1_idx, 20.0),
                Sketcher.Constraint("Distance", sm2_idx, bg2_idx, 3.0),
                Sketcher.Constraint("Distance", sm4_idx, bg4_idx, 4.0),
                Sketcher.Constraint("Distance", bg3_idx, sm3_idx, 5.0),
                Sketcher.Constraint("Distance", bg1_idx, sm1_idx, 6.0),
            ]
        )[5]

        self.assertSuccessfulSolve(sketch)

        # Change the small radiuses by a big amount

        sketch.setDatum(sm_diam, App.Units.Quantity("100.00 mm"))

        self.assertSuccessfulSolve(sketch)

        sm1 = sketch.Geometry[sm1_idx]
        bg1 = sketch.Geometry[bg1_idx]

        sm2 = sketch.Geometry[sm2_idx]
        bg2 = sketch.Geometry[bg2_idx]

        sm3 = sketch.Geometry[sm3_idx]
        bg3 = sketch.Geometry[bg3_idx]

        sm4 = sketch.Geometry[sm4_idx]
        bg4 = sketch.Geometry[bg4_idx]

        self.assertGreater(bg1.Radius, sm1.Radius)
        self.assertGreater(bg2.Radius, sm2.Radius)
        self.assertGreater(bg3.Radius, sm3.Radius)
        self.assertGreater(bg4.Radius, sm4.Radius)

    def testRemovedExternalGeometryReference(self):
        if "BUILD_PARTDESIGN" in FreeCAD.__cmake__:
            body = self.Doc.addObject("PartDesign::Body", "Body")
            sketch = body.newObject("Sketcher::SketchObject", "Sketch")
            CreateRectangleSketch(sketch, (0, 0), (30, 30))
            pad = body.newObject("PartDesign::Pad", "Pad")
            pad.Profile = sketch
            sketch1 = body.newObject("Sketcher::SketchObject", "Sketch1")
            CreateCircleSketch(sketch1, (15, 15), 0.25)
            self.Doc.recompute()
            self.assertEqual(len(pad.Shape.Edges), 12)

            hole = self.Doc.addObject("PartDesign::Hole", "Hole")
            hole.Refine = True
            hole.Reversed = True
            body.addObject(hole)
            hole.Profile = sketch1
            hole.DrillPointAngle = 118.000000
            hole.Diameter = 6.000000
            hole.TaperedAngle = 90.000000
            hole.Tapered = 0
            hole.Depth = 8.000000
            hole.Threaded = 1
            hole.ModelThread = 0
            hole.ThreadDepthType = 0
            hole.ThreadType = 1
            hole.ThreadSize = 16
            hole.ThreadClass = 0
            hole.ThreadDirection = 0
            hole.HoleCutType = 0
            hole.DepthType = 0
            hole.DrillPoint = 1
            hole.DrillForDepth = 0
            self.Doc.recompute()
            # 15 edges if it's passthrough-flat 17 if DrillPoint = 1
            self.assertEqual(len(hole.Shape.Edges), 17)

            hole.ModelThread = 1
            sketch2 = body.newObject("Sketcher::SketchObject", "Sketch2")
            CreateRectangleSketch(sketch2, (0, 0), (3, 3))
            self.Doc.recompute()
            self.assertGreater(len(hole.Shape.Edges), 17)
            # 77 edges for basic profile
            self.assertEqual(len(hole.Shape.Edges), 77)

            # Edges in the thread should disappear when we stop modeling thread
            sketch2.addExternal("Hole", "Edge29")
            hole.ModelThread = 0
            hole.Refine = 1
            self.Doc.recompute()
            self.assertEqual(len(hole.Shape.Edges), 17)
            self.assertEqual(len(sketch2.ExternalGeometry), 0)

    def testSaveLoadWithExternalGeometryReference(self):
        if "BUILD_PARTDESIGN" in FreeCAD.__cmake__:
            # Arrange
            body = self.Doc.addObject("PartDesign::Body", "Body")
            sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
            CreateRectangleSketch(sketch, (0, 0), (30, 30))
            pad = self.Doc.addObject("PartDesign::Pad", "Pad")
            pad.Profile = sketch
            sketch1 = self.Doc.addObject("Sketcher::SketchObject", "Sketch1")
            body.addObject(sketch)
            body.addObject(pad)
            body.addObject(sketch1)
            self.Doc.recompute()
            sketch1.addExternal("Pad", "Edge12")
            self.Doc.recompute()

            # Act: Try changing sketch before the save
            sketch = self.Doc.getObject("Sketch")
            g1 = sketch.Constraints[11].First
            d1 = sketch.Constraints[11].Value
            sketch.delConstraint(11)
            sketch.addConstraint(Sketcher.Constraint("Distance", g1, d1 - 1.0))
            self.Doc.recompute()

            # Act: Save and reload the file
            filename = tempfile.gettempdir() + os.sep + self.Doc.Name + ".FCStd"
            self.Doc.saveAs(filename)
            FreeCAD.closeDocument(self.Doc.Name)
            self.Doc = FreeCAD.openDocument(filename)
            pad = self.Doc.getObject("Pad")
            sketch1 = self.Doc.getObject("Sketch1")
            self.Doc.recompute()

            # Act:  change sketch after restore ( trigger missing references if there is a bug )
            sketch = self.Doc.getObject("Sketch")
            g1 = sketch.Constraints[11].First
            d1 = sketch.Constraints[11].Value
            sketch.delConstraint(11)
            sketch.addConstraint(Sketcher.Constraint("Distance", g1, d1 - 1.0))
            self.Doc.recompute()

            # Assert
            self.assertEqual(pad.Shape.ElementMapSize, 30)
            self.assertIn("Edge12", pad.Shape.ElementReverseMap)
            self.assertIn((pad, ("Edge12",)), sketch1.ExternalGeometry)  # Not "?Edge12"

    def testTNPExternalGeometryStored(self):
        # Arrange
        if "BUILD_PARTDESIGN" in FreeCAD.__cmake__:
            import xml.etree.ElementTree as ET

            sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
            CreateRectangleSketch(sketch, (0, 0), (30, 30))
            sketch1 = self.Doc.addObject("Sketcher::SketchObject", "Sketch1")
            pad = self.Doc.addObject("PartDesign::Pad", "Pad")
            pad.Profile = sketch
            self.Doc.recompute()
            sketch1.addExternal("Pad", "Edge12")
            self.Doc.recompute()
            # Act
            root = ET.fromstring("<all>" + sketch1.Content + "</all>")
            # Can't use != in an xpath because it wasn't added until python 3.10.
            # "*/*[@name='ExternalGeo']//*/[@type='Sketcher::ExternalGeometryExtension']/[@Ref!='']"
            extRefs = root.findall(
                "*/*[@name='ExternalGeo']//*/[@type='Sketcher::ExternalGeometryExtension']/[@Ref='']"
            )
            extRefsAll = root.findall(
                "*/*[@name='ExternalGeo']//*/[@type='Sketcher::ExternalGeometryExtension']/[@Ref]"
            )
            # Assert
            self.assertEqual(len(extRefs), 2)
            self.assertEqual(len(extRefsAll), 3)
            self.assertEqual(root.tag, "all")
            # Act
            filename = tempfile.gettempdir() + os.sep + self.Doc.Name + ".FCStd"
            self.Doc.saveAs(filename)
            FreeCAD.closeDocument(self.Doc.Name)
            self.Doc = FreeCAD.openDocument(filename)
            # Assert
            root = ET.fromstring("<all>" + self.Doc.getObject("Sketch1").Content + "</all>")
            extRefs = root.findall(
                "*/*[@name='ExternalGeo']//*/[@type='Sketcher::ExternalGeometryExtension']/[@Ref='']"
            )
            extRefsAll = root.findall(
                "*/*[@name='ExternalGeo']//*/[@type='Sketcher::ExternalGeometryExtension']/[@Ref]"
            )
            self.assertEqual(len(extRefs), 2)
            self.assertEqual(len(extRefsAll), 3)
            self.assertEqual(root.tag, "all")
            # Act to change the constraint
            sketch = self.Doc.getObject("Sketch")
            g1 = sketch.Constraints[11].First
            d1 = sketch.Constraints[11].Value
            sketch.delConstraint(11)
            sketch.addConstraint(Sketcher.Constraint("Distance", g1, d1 - 1.0))
            self.Doc.recompute()
            # Assert
            root = ET.fromstring("<all>" + self.Doc.getObject("Sketch1").Content + "</all>")
            extRefs = root.findall(
                "*/*[@name='ExternalGeo']//*/[@type='Sketcher::ExternalGeometryExtension']/[@Ref='']"
            )
            extRefsAll = root.findall(
                "*/*[@name='ExternalGeo']//*/[@type='Sketcher::ExternalGeometryExtension']/[@Ref]"
            )
            self.assertEqual(len(extRefs), 2)
            self.assertEqual(len(extRefsAll), 3)
            self.assertEqual(root.tag, "all")

    def testConstructionToggleTNP(self):
        """Bug 15484"""
        if "BUILD_PARTDESIGN" in FreeCAD.__cmake__:
            # Arrange
            import xml.etree.ElementTree as ET

            body = self.Doc.addObject("PartDesign::Body", "Body")
            sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
            CreateRectangleSketch(sketch, (0, 0), (30, 30))
            # Add zigsag geo as construction lines
            i = sketch.GeometryCount
            sketch.addGeometry(
                Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(-10, 10, 0)), True
            )
            sketch.addGeometry(
                Part.LineSegment(FreeCAD.Vector(-10, 10, 0), FreeCAD.Vector(-5, 20, 0)), True
            )
            sketch.addGeometry(
                Part.LineSegment(FreeCAD.Vector(-5, 20, 0), FreeCAD.Vector(-10, 25, 0)), True
            )
            sketch.addGeometry(
                Part.LineSegment(FreeCAD.Vector(-10, 25, 0), FreeCAD.Vector(0, 30, 0)), True
            )
            sketch.addConstraint(Sketcher.Constraint("Coincident", i + 0, 2, i + 1, 1))
            sketch.addConstraint(Sketcher.Constraint("Coincident", i + 1, 2, i + 2, 1))
            sketch.addConstraint(Sketcher.Constraint("Coincident", i + 2, 2, i + 3, 1))
            sketch.addConstraint(Sketcher.Constraint("Coincident", i + 3, 2, 0, 1))
            sketch.addConstraint(Sketcher.Constraint("Coincident", i + 0, 1, 2, 2))

            pad = self.Doc.addObject("PartDesign::Pad", "Pad")
            pad.Profile = sketch
            body.addObject(sketch)
            body.addObject(pad)
            self.Doc.recompute()
            sketch1 = self.Doc.addObject("Sketcher::SketchObject", "Sketch1")
            sketch1.AttachmentSupport = (pad, ("Face6"))
            sketch1.MapMode = "FlatFace"
            self.Doc.recompute()

            CreateCircleSketch(sketch1, (2, 2, 0), 1)
            CreateCircleSketch(sketch1, (6, 2, 0), 1)
            body.addObject(sketch1)
            self.Doc.recompute()
            # Act toggle construction lines on in sketch; pad now has 9 instead of 6 faces.
            sketch.setConstruction(4, False)
            sketch.setConstruction(5, False)
            sketch.setConstruction(6, False)
            sketch.setConstruction(7, False)
            sketch.setConstruction(3, True)
            self.Doc.recompute()
            # Assert
            # AttachmentSupport is a list of (object,(subobject list)) with 1 entry.  Get the
            # first and only subobject name in second part of that first tuple and see that it moved
            # from the Face6 we set above.
            self.assertEqual(sketch1.AttachmentSupport[0][1][0], "Face9")
            self.assertIn("Face6", pad.Shape.ElementReverseMap)  # different Face6 exists

    # TODO other tests:
    #  getHigherElement

    def assertSuccessfulSolve(self, sketch, msg=None):
        status = sketch.solve()
        # TODO: can we get the solver's messages somehow to improve the message?
        self.assertTrue(status == 0, msg=msg or "solver didn't converge")

    def assertShapeDistance(self, shape1, shape2, expected_distance, msg=None):
        distance, _, _ = shape1.distToShape(shape2)
        self.assertAlmostEqual(
            distance,
            expected_distance,
            delta=Precision.confusion(),
            msg=msg or "The given shapes are not spaced by the expected distance.",
        )

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument("SketchSolverTest")
        # print ("omit closing document for debugging")
