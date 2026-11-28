# SPDX-License-Identifier: LGPL-2.1-or-later

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
import math

import FreeCAD
from FreeCAD import Base
import Part
import Sketcher


class TestShapeBinder(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestShapeBinder")

    def testTwoBodyShapeBinderCase(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.Box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        self.Box.Length = 1
        self.Box.Width = 1
        self.Box.Height = 1
        self.Body.addObject(self.Box)
        self.Doc.recompute()
        self.Body001 = self.Doc.addObject("PartDesign::Body", "Body001")
        self.ShapeBinder = self.Doc.addObject("PartDesign::ShapeBinder", "ShapeBinder")
        self.ShapeBinder.Support = [(self.Box, "Face1")]
        self.Body001.addObject(self.ShapeBinder)
        self.Doc.recompute()
        self.assertIn("Box", self.ShapeBinder.OutList[0].Label)
        self.assertIn("Body001", self.ShapeBinder.InList[0].Label)

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument("PartDesignTestShapeBinder")
        # print ("omit closing document for debugging")


class TestSubShapeBinder(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestSubShapeBinder")

    def tearDown(self):
        FreeCAD.closeDocument("PartDesignTestSubShapeBinder")

    def testOffsetBinder(self):
        # See PR 7445
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)

        box.Length = 10.00000
        box.Width = 10.00000
        box.Height = 10.00000

        binder = body.newObject("PartDesign::SubShapeBinder", "Binder")
        binder.Support = [(box, ("Edge2", "Edge12", "Edge6", "Edge10"))]
        self.Doc.recompute()

        self.assertAlmostEqual(binder.Shape.Length, 40)

        binder.OffsetJoinType = "Tangent"
        binder.Offset = 5.00000
        self.Doc.recompute()

        self.assertAlmostEqual(binder.Shape.Length, 80)

    def testBinderBeforeOrAfterPad(self):
        """Test case for PR #8763"""
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = body.newObject("Sketcher::SketchObject", "Sketch")
        sketch.AttachmentSupport = (self.Doc.XZ_Plane, [""])
        sketch.MapMode = "FlatFace"
        self.Doc.recompute()

        geoList = []
        geoList.append(
            Part.LineSegment(
                Base.Vector(-21.762587, 19.904083, 0), Base.Vector(32.074337, 19.904083, 0)
            )
        )
        geoList.append(
            Part.LineSegment(
                Base.Vector(32.074337, 19.904083, 0), Base.Vector(32.074337, -27.458027, 0)
            )
        )
        geoList.append(
            Part.LineSegment(
                Base.Vector(32.074337, -27.458027, 0), Base.Vector(-21.762587, -27.458027, 0)
            )
        )
        geoList.append(
            Part.LineSegment(
                Base.Vector(-21.762587, -27.458027, 0), Base.Vector(-21.762587, 19.904083, 0)
            )
        )
        sketch.addGeometry(geoList, False)

        conList = []
        conList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        conList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        conList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        conList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        conList.append(Sketcher.Constraint("Horizontal", 0))
        conList.append(Sketcher.Constraint("Horizontal", 2))
        conList.append(Sketcher.Constraint("Vertical", 1))
        conList.append(Sketcher.Constraint("Vertical", 3))
        sketch.addConstraint(conList)
        del geoList, conList

        self.Doc.recompute()

        binder1 = body.newObject("PartDesign::SubShapeBinder", "Binder")
        binder1.Support = sketch
        self.Doc.recompute()
        pad = body.newObject("PartDesign::Pad", "Pad")
        pad.Profile = sketch
        pad.Length = 10
        self.Doc.recompute()
        pad.ReferenceAxis = (sketch, ["N_Axis"])
        sketch.Visibility = False
        self.Doc.recompute()

        binder2 = body.newObject("PartDesign::SubShapeBinder", "Binder001")
        binder2.Support = [pad, "Sketch."]
        self.Doc.recompute()

        self.assertAlmostEqual(binder1.Shape.BoundBox.XLength, binder2.Shape.BoundBox.XLength, 2)
        self.assertAlmostEqual(binder1.Shape.BoundBox.YLength, binder2.Shape.BoundBox.YLength, 2)
        self.assertAlmostEqual(binder1.Shape.BoundBox.ZLength, binder2.Shape.BoundBox.ZLength, 2)

        nor1 = binder1.Shape.Face1.normalAt(0, 0)
        nor2 = binder2.Shape.Face1.normalAt(0, 0)
        self.assertAlmostEqual(nor1.getAngle(nor2), 0.0, 2)

    def testBinderWithRevolution(self):
        doc = self.Doc
        body = doc.addObject("PartDesign::Body", "Body")
        doc.recompute()
        sketch = body.newObject("Sketcher::SketchObject", "Sketch")
        geoList = []
        geoList.append(Part.LineSegment(Base.Vector(10, 10, 0), Base.Vector(30, 10, 0)))
        geoList.append(Part.LineSegment(Base.Vector(30, 10, 0), Base.Vector(30, 15, 0)))
        geoList.append(Part.LineSegment(Base.Vector(30, 15, 0), Base.Vector(10, 15, 0)))
        geoList.append(Part.LineSegment(Base.Vector(10, 15, 0), Base.Vector(10, 10, 0)))
        sketch.addGeometry(geoList, False)
        del geoList
        constraintList = []
        constraintList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        constraintList.append(Sketcher.Constraint("Horizontal", 0))
        constraintList.append(Sketcher.Constraint("Horizontal", 2))
        constraintList.append(Sketcher.Constraint("Vertical", 1))
        constraintList.append(Sketcher.Constraint("Vertical", 3))
        sketch.addConstraint(constraintList)
        del constraintList
        doc.recompute()
        binder = body.newObject("PartDesign::SubShapeBinder", "Binder")
        binder.Support = sketch
        revolution = body.newObject("PartDesign::Revolution", "Revolution")
        revolution.Profile = (
            binder,
            [
                "",
            ],
        )
        revolution.ReferenceAxis = (doc.getObject("Y_Axis"), [""])
        revolution.Angle = 360.0
        revolution.Reversed = 1
        doc.recompute()
        revolution.Angle2 = 60.0
        doc.recompute()
        self.assertAlmostEqual(binder.Shape.Area, 100)
        volume = 100 * math.pi * 2 * 20
        self.assertAlmostEqual(revolution.Shape.Volume, volume)

    def _addSketch(self, body, plane=None, mapReversed=False, placement=None):
        sketch = body.newObject("Sketcher::SketchObject", "Sketch")
        if plane:
            origin = [f for f in body.Origin.OriginFeatures if f.Role == plane][0]
            sketch.AttachmentSupport = (origin, [""])
            sketch.MapMode = "FlatFace"
            sketch.MapReversed = mapReversed
        if placement:
            sketch.Placement = placement
        points = [
            Base.Vector(0, 0, 0),
            Base.Vector(10, 0, 0),
            Base.Vector(10, 6, 0),
            Base.Vector(0, 6, 0),
        ]
        for start, end in zip(points, points[1:] + points[:1]):
            sketch.addGeometry(Part.LineSegment(start, end), False)
        return sketch

    def _addBinder(self, body, support):
        binder = body.newObject("PartDesign::SubShapeBinder", "Binder")
        binder.Support = support
        return binder

    def _sketchNormal(self, sketch):
        return sketch.Placement.Rotation.multVec(Base.Vector(0, 0, 1))

    def _assertFaceNormal(self, binder, normal):
        self.assertTrue(binder.Shape.Faces)
        for face in binder.Shape.Faces:
            self.assertAlmostEqual(face.normalAt(0, 0).dot(normal), 1.0, places=6)

    def testBinderFaceFollowsSketchOnOriginPlanes(self):
        for plane in ("XY_Plane", "XZ_Plane", "YZ_Plane"):
            for mapReversed in (False, True):
                with self.subTest(plane=plane, mapReversed=mapReversed):
                    body = self.Doc.addObject("PartDesign::Body", "Body")
                    sketch = self._addSketch(body, plane, mapReversed)
                    binder = self._addBinder(body, [(sketch, "")])
                    pad = body.newObject("PartDesign::Pad", "Pad")
                    pad.Profile = binder
                    pad.Length = 7
                    self.Doc.recompute()
                    normal = self._sketchNormal(sketch)
                    self._assertFaceNormal(binder, normal)
                    travel = pad.Shape.CenterOfMass - binder.Shape.CenterOfMass
                    self.assertAlmostEqual(travel.dot(normal), 3.5, places=6)
                    self.assertEqual(binder._Version, 4)

    def testBinderFaceFollowsTiltedSketch(self):
        for axis in (Base.Vector(1, 0, 0), Base.Vector(0, 1, 0), Base.Vector(1, 1, 1)):
            for angle in (30, 44, 46, 120, 136, 200, 300):
                with self.subTest(axis=axis, angle=angle):
                    body = self.Doc.addObject("PartDesign::Body", "Body")
                    placement = FreeCAD.Placement(
                        Base.Vector(3, -2, 5), FreeCAD.Rotation(axis, angle)
                    )
                    sketch = self._addSketch(body, placement=placement)
                    binder = self._addBinder(body, [(sketch, "")])
                    self.Doc.recompute()
                    self._assertFaceNormal(binder, self._sketchNormal(sketch))

    def testBinderFaceFollowsSketchWithOffset(self):
        for offset in (1.0, -1.0):
            with self.subTest(offset=offset):
                body = self.Doc.addObject("PartDesign::Body", "Body")
                sketch = self._addSketch(body, "XZ_Plane")
                binder = self._addBinder(body, [(sketch, "")])
                binder.Offset = offset
                self.Doc.recompute()
                self._assertFaceNormal(binder, self._sketchNormal(sketch))

    def testBinderFaceFollowsSketchElements(self):
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = self._addSketch(body, "XZ_Plane")
        binder = self._addBinder(body, [(sketch, ["Edge1", "Edge2", "Edge3", "Edge4"])])
        self.Doc.recompute()
        self._assertFaceNormal(binder, self._sketchNormal(sketch))

    def testBinderFaceFollowsSketchInsidePad(self):
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = self._addSketch(body, "XZ_Plane")
        pad = body.newObject("PartDesign::Pad", "Pad")
        pad.Profile = sketch
        pad.Length = 7
        binder = self._addBinder(body, [(pad, sketch.Name + ".")])
        self.Doc.recompute()
        self._assertFaceNormal(binder, self._sketchNormal(sketch))

    def testBinderFaceFollowsSketchAcrossParts(self):
        source = self.Doc.addObject("App::Part", "Source")
        source.Placement.Rotation = FreeCAD.Rotation(Base.Vector(0, 1, 0), 40)
        sourceBody = self.Doc.addObject("PartDesign::Body", "SourceBody")
        source.addObject(sourceBody)
        sketch = self._addSketch(sourceBody, "XZ_Plane")
        target = self.Doc.addObject("App::Part", "Target")
        target.Placement.Rotation = FreeCAD.Rotation(Base.Vector(0, 0, 1), 70)
        targetBody = self.Doc.addObject("PartDesign::Body", "TargetBody")
        target.addObject(targetBody)
        binder = self._addBinder(targetBody, [(source, "SourceBody." + sketch.Name + ".")])
        self.Doc.recompute()
        normal = sketch.getGlobalPlacement().Rotation.multVec(Base.Vector(0, 0, 1))
        self._assertFaceNormal(binder, target.Placement.Rotation.inverted().multVec(normal))

    def testBinderFaceFollowsLinkedSketch(self):
        for linkTransform in (False, True):
            for angle in (0, 120, 180):
                with self.subTest(linkTransform=linkTransform, angle=angle):
                    sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
                    sketch.Placement.Rotation = FreeCAD.Rotation(Base.Vector(1, 0, 0), angle)
                    sketch.addGeometry(
                        [
                            Part.LineSegment(Base.Vector(0, 0, 0), Base.Vector(10, 0, 0)),
                            Part.LineSegment(Base.Vector(10, 0, 0), Base.Vector(10, 6, 0)),
                            Part.LineSegment(Base.Vector(10, 6, 0), Base.Vector(0, 0, 0)),
                        ],
                        False,
                    )
                    link = self.Doc.addObject("App::Link", "Link")
                    link.LinkedObject = sketch
                    link.LinkTransform = linkTransform
                    link.Placement.Rotation = FreeCAD.Rotation(Base.Vector(0, 1, 0), 150)
                    body = self.Doc.addObject("PartDesign::Body", "Body")
                    binder = self._addBinder(body, [(link, "")])
                    self.Doc.recompute()
                    rotation = link.Placement.Rotation
                    if linkTransform:
                        rotation = rotation.multiply(sketch.Placement.Rotation)
                    self._assertFaceNormal(binder, rotation.multVec(Base.Vector(0, 0, 1)))

    def testBinderFaceFollowsLinkedSketchElements(self):
        for parent in (False, True):
            with self.subTest(parent=parent):
                sketch = self._addSketch(self.Doc.addObject("PartDesign::Body", "Source"))
                sketch.Placement.Rotation = FreeCAD.Rotation(Base.Vector(1, 0, 0), 180)
                link = self.Doc.addObject("App::Link", "Link")
                link.LinkedObject = sketch
                link.LinkTransform = True
                link.Placement.Rotation = FreeCAD.Rotation(Base.Vector(0, 1, 0), 150)
                edges = ["Edge1", "Edge2", "Edge3", "Edge4"]
                support = (link, edges)
                if parent:
                    part = self.Doc.addObject("App::Part", "Part")
                    part.addObject(link)
                    support = (part, [link.Name + "." + edge for edge in edges])
                body = self.Doc.addObject("PartDesign::Body", "Body")
                binder = self._addBinder(body, [support])
                self.Doc.recompute()
                rotation = link.Placement.Rotation.multiply(sketch.Placement.Rotation)
                self._assertFaceNormal(binder, rotation.multVec(Base.Vector(0, 0, 1)))

    def testBinderFaceKeepsSideForMirroredLink(self):
        for scale in (Base.Vector(-1, 1, 1), Base.Vector(1, -1, 1)):
            with self.subTest(scale=scale):
                normals = []
                for version in (3, 4):
                    body = self.Doc.addObject("PartDesign::Body", "Body")
                    sketch = self._addSketch(body)
                    link = self.Doc.addObject("App::Link", "Link")
                    link.LinkedObject = sketch
                    link.ScaleVector = scale
                    binder = self._addBinder(self.Doc.addObject("PartDesign::Body", "Other"), [])
                    binder._Version = version
                    binder.Support = [(link, "")]
                    self.Doc.recompute()
                    normals.append(binder.Shape.Faces[0].normalAt(0, 0))
                self.assertAlmostEqual(normals[0].dot(normals[1]), 1.0, places=6)

    def testBinderFaceWithoutRelativeStaysLocal(self):
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = self._addSketch(body, "XY_Plane", True)
        binder = self._addBinder(body, [(sketch, "")])
        binder.Relative = False
        self.Doc.recompute()
        self._assertFaceNormal(binder, Base.Vector(0, 0, 1))

    def testBinderFaceFollowsSketchesWithSameNormal(self):
        body = self.Doc.addObject("PartDesign::Body", "Body")
        first = self._addSketch(body, "XZ_Plane")
        second = self._addSketch(body, "XZ_Plane")
        second.AttachmentOffset.Base = Base.Vector(20, 0, 0)
        binder = self._addBinder(body, [(first, ""), (second, "")])
        self.Doc.recompute()
        self.assertEqual(len(binder.Shape.Faces), 2)
        self._assertFaceNormal(binder, self._sketchNormal(first))

    def testBinderFaceKeepsSideForOppositeSketches(self):
        normals = []
        for version in (3, 4):
            body = self.Doc.addObject("PartDesign::Body", "Body")
            first = self._addSketch(body, "XZ_Plane")
            second = self._addSketch(body, "XZ_Plane", True)
            second.AttachmentOffset.Base = Base.Vector(20, 0, 0)
            binder = self._addBinder(body, [(first, ""), (second, "")])
            binder._Version = version
            self.Doc.recompute()
            normals.append(binder.Shape.Faces[0].normalAt(0, 0))
        self.assertAlmostEqual(normals[0].dot(normals[1]), 1.0, places=6)

    def testOlderBinderKeepsFaceSide(self):
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = self._addSketch(body, "XZ_Plane")
        binder = self._addBinder(body, [(sketch, "")])
        binder._Version = 3
        self.Doc.recompute()
        face = Part.makeFace(sketch.Shape.Wires, "Part::FaceMakerBuildFace")
        self._assertFaceNormal(binder, face.Faces[0].normalAt(0, 0))
