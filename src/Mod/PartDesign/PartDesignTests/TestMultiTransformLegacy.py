# ***************************************************************************
# *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

import unittest

import FreeCAD
import Part
import Sketcher
import TestSketcherApp

App = FreeCAD


class TestMultiTransformLegacy(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestMultiTransformLegacy")

    def testMultiTransform(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.Body_Group = []
        self.Sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        self.geo0 = self.Sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(0.0, 0.0, 0.0), FreeCAD.Vector(0.0, 23.837982, 0.0)
            )
        )
        self.geo1 = self.Sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(0.0, 23.837982, 0.0),
                FreeCAD.Vector(44.55508400000001, 23.837982, 0.0),
            )
        )
        self.geo2 = self.Sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(44.55508400000001, 23.837982, 0.0),
                FreeCAD.Vector(60.491337, 0.0, 0.0),
            )
        )
        self.geo3 = self.Sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(60.491337, 0.0, 0.0), FreeCAD.Vector(0.0, 0.0, 0.0)
            )
        )
        self.Sketch.addConstraint(
            Sketcher.Constraint("Coincident", -1, 1, self.geo0, 1)
        )
        self.Sketch.addConstraint(
            Sketcher.Constraint("PointOnObject", self.geo0, 2, -2)
        )
        self.Sketch.addConstraint(
            Sketcher.Constraint("Coincident", self.geo0, 2, self.geo1, 1)
        )
        self.Sketch.addConstraint(Sketcher.Constraint("Horizontal", self.geo1))
        self.Sketch.addConstraint(
            Sketcher.Constraint("Coincident", self.geo1, 2, self.geo2, 1)
        )
        self.Sketch.addConstraint(
            Sketcher.Constraint("PointOnObject", self.geo2, 2, -1)
        )
        self.Sketch.addConstraint(
            Sketcher.Constraint("Coincident", self.geo2, 2, self.geo3, 1)
        )
        self.Sketch.addConstraint(
            Sketcher.Constraint("Coincident", self.geo3, 2, self.geo0, 1)
        )
        self.Sketch.MapMode = "FlatFace"
        self.Sketch.Support = [(self.Doc.XY_Plane, (""))]
        self.Sketch.Visibility = False
        self.Body_Group.append(self.Sketch)
        self.Sketch001 = self.Doc.addObject("Sketcher::SketchObject", "Sketch001")
        self.geo0 = self.Sketch001.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(23.837981999999997, 2.9e-15, 0.0),
                FreeCAD.Vector(0.0, 45.000000000000014, 0.0),
            )
        )
        self.geo1 = self.Sketch001.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(0.0, 45.000000000000014, 0.0),
                FreeCAD.Vector(40.116829, 51.757961, 0.0),
            )
        )
        self.geo2 = self.Sketch001.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(40.116829, 51.757961, 0.0),
                FreeCAD.Vector(23.837981999999997, 0.0, 0.0),
            )
        )
        self.Sketch001.addConstraint(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        self.Sketch001.addConstraint(Sketcher.Constraint("Coincident", 0, 1, 2, 2))
        self.Sketch001.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        self.Sketch001.addConstraint(Sketcher.Constraint("PointOnObject", 0, 2, -2))
        self.Sketch001.addConstraint(Sketcher.Constraint("PointOnObject", 0, 1, -1))
        self.Sketch001.addConstraint(
            Sketcher.Constraint("DistanceY", -1, 1, 0, 2, 45.000000)
        )
        self.Sketch001.setDatum(5, App.Units.Quantity("45.000000 mm"))
        self.Sketch001.addConstraint(
            Sketcher.Constraint("DistanceY", -1, 1, 1, 2, 51.757961)
        )
        self.Sketch001.setDatum(6, App.Units.Quantity("51.760000 mm"))
        self.Sketch001.addConstraint(
            Sketcher.Constraint("DistanceX", -1, 1, 0, 1, 23.837982)
        )
        self.Sketch001.setDatum(7, App.Units.Quantity("23.840000 mm"))
        self.Sketch001.addConstraint(
            Sketcher.Constraint("DistanceX", -1, 1, 1, 2, 40.116829)
        )
        self.Sketch001.setDatum(8, App.Units.Quantity("40.120000 mm"))
        self.Sketch001.MapMode = "FlatFace"
        self.Sketch001.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0.00, 0.00, 0.00), FreeCAD.Rotation(0.5, 0.5, 0.5, 0.5)
        )
        self.Sketch001.Support = [(self.Doc.YZ_Plane, (""))]
        self.Sketch001.Visibility = False
        self.Body_Group.append(self.Sketch001)

        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Pad.Direction = FreeCAD.Vector(0.00, 0.00, 1.00)
        self.Pad.Length = 45.0
        self.Pad.Profile = (self.Sketch, [])
        self.Pad.Visibility = False
        self.Body_Group.append(self.Pad)

        self.Pocket = self.Doc.addObject("PartDesign::Pocket", "Pocket")
        self.Pocket.AllowMultiFace = False
        self.Pocket.BaseFeature = self.Pad
        self.Pocket.Direction = FreeCAD.Vector(-1.00, -0.00, -0.00)
        self.Pocket.Profile = (self.Sketch001, [])
        self.Pocket.Reversed = True
        self.Pocket.Type = "ThroughAll"
        self.Pocket.Visibility = False
        self.Body_Group.append(self.Pocket)

        self.DatumPoint = self.Doc.addObject("PartDesign::Point", "DatumPoint")
        self.DatumPoint.MapMode = "OnEdge"
        self.DatumPoint.MapPathParameter = 0.40
        self.DatumPoint.Placement = FreeCAD.Placement(
            FreeCAD.Vector(36.29, 0.00, 45.00),
            FreeCAD.Rotation(
                FreeCAD.Vector(
                    0.5773502691896257, 0.5773502691896257, 0.5773502691896257
                ),
                120,
            ),
        )
        self.DatumPoint.Support = [(self.Pocket, ("Edge10"))]
        self.Body_Group.append(self.DatumPoint)
        self.DatumPoint.Visibility = False

        self.DatumPlane = self.Doc.addObject("PartDesign::Plane", "DatumPlane")
        self.DatumPlane.Length = 131.30239095262064
        self.DatumPlane.MapMode = "ThreePointsPlane"
        self.DatumPlane.Placement = FreeCAD.Placement(
            FreeCAD.Vector(47.11, 7.95, 15.00),
            FreeCAD.Rotation(
                0.25639002138112243,
                0.4798095739282281,
                0.7400452731042549,
                0.39544901499314516,
            ),
        )
        self.DatumPlane.Support = [
            (self.DatumPoint, ("")),
            (self.Sketch, ("Vertex4", "Vertex3")),
        ]
        self.DatumPlane.Visibility = False
        self.DatumPlane.Width = 138.78827744889375
        self.Body_Group.append(self.DatumPlane)

        self.Pocket001 = self.Doc.addObject("PartDesign::Pocket", "Pocket001")
        self.Pocket001.BaseFeature = self.Pocket
        self.Pocket001.Direction = FreeCAD.Vector(-0.76, -0.51, -0.41)
        self.Pocket001.Profile = (self.DatumPlane, [])
        self.Pocket001.Reversed = True
        self.Pocket001.Type = "ThroughAll"
        self.Pocket001.Visibility = False
        self.Body_Group.append(self.Pocket001)

        self.Mirrored = self.Doc.addObject("PartDesign::Mirrored", "Mirrored")
        self.Mirrored.MirrorPlane = (self.Doc.YZ_Plane, [""])
        self.Mirrored.Visibility = False
        self.Body_Group.append(self.Mirrored)

        self.Mirrored001 = self.Doc.addObject("PartDesign::Mirrored", "Mirrored001")
        self.Mirrored001.MirrorPlane = (self.Doc.XZ_Plane, [""])
        self.Mirrored001.Visibility = False
        self.Body_Group.append(self.Mirrored001)

        self.MultiTransform = self.Doc.addObject(
            "PartDesign::MultiTransform", "MultiTransform"
        )
        self.MultiTransform.BaseFeature = self.Pocket001
        self.MultiTransform.Originals = [self.Pad, self.Pocket, self.Pocket001]
        self.MultiTransform.Transformations = [self.Mirrored, self.Mirrored001]
        self.Body_Group.append(self.MultiTransform)
        self.Body.Group = self.Body_Group
        self.Body.Tip = self.MultiTransform
        self.MultiTransform.Refine = True
        self.Doc.recompute()

        self.Doc.XY_Plane.Visibility = False
        self.Doc.XZ_Plane.Visibility = False
        self.Doc.YZ_Plane.Visibility = False
        self.Doc.X_Axis.Visibility = False
        self.Doc.Y_Axis.Visibility = False
        self.Doc.Z_Axis.Visibility = False

        self.assertAlmostEqual(self.MultiTransform.Shape.Volume, 101086.27728525076)
        self.assertEqual(len(self.MultiTransform.Shape.Faces), 9)
        self.assertEqual(len(self.MultiTransform.Shape.Solids), 1)

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument("PartDesignTestMultiTransformLegacy")
        # print ("omit closing document for debugging")
