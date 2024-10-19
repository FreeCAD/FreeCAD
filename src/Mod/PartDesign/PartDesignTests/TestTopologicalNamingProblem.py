# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 bgbsww@gmail.com                                   *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

""" Tests related to the Topological Naming Problem """

import math
import unittest

import FreeCAD as App
import Part
import Sketcher
import TestSketcherApp


class TestTopologicalNamingProblem(unittest.TestCase):
    """Tests related to the Topological Naming Problem"""

    # pylint: disable=attribute-defined-outside-init

    def setUp(self):
        """Create a document for each test in the test suite"""
        self.Doc = App.newDocument("PartDesignTestTNP." + self._testMethodName)

    def countFacesEdgesVertexes(self, map):
        """Helper to return a tuple of the counts of Faces, Edges, and Vertexes in a map for
        easier test writing"""
        faces = [name for name in map.keys() if name.startswith("Face")]
        edges = [name for name in map.keys() if name.startswith("Edge")]
        vertexes = [name for name in map.keys() if name.startswith("Vertex")]
        return (len(faces), len(edges), len(vertexes))

    def testPadsOnBaseObject(self):
        """Simple TNP test case
        By creating three Pads dependent on each other in succession, and then moving the
        middle one we can determine if the last one breaks because of a broken reference
        to the middle one.  This is the essence of a TNP. Pretty much a duplicate of the
        steps at https://wiki.freecad.org/Topological_naming_problem"""

        # Arrange
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject("Sketcher::SketchObject", "SketchPad")
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Doc.recompute()

        # Attach a second pad to the top of the first.
        self.PadSketch1 = self.Doc.addObject("Sketcher::SketchObject", "SketchPad1")
        self.Body.addObject(self.PadSketch1)
        self.PadSketch1.MapMode = "FlatFace"
        self.PadSketch1.AttachmentSupport = [(self.Doc.getObject("Pad"), "Face6")]
        TestSketcherApp.CreateRectangleSketch(self.PadSketch1, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.PadSketch1
        self.Pad1.Length = 1
        self.Doc.recompute()

        # Attach a third pad to the top of the second.
        self.PadSketch2 = self.Doc.addObject("Sketcher::SketchObject", "SketchPad2")
        self.Body.addObject(self.PadSketch2)
        self.PadSketch2.MapMode = "FlatFace"
        self.PadSketch2.AttachmentSupport = [(self.Doc.getObject("Pad1"), "Face6")]
        TestSketcherApp.CreateRectangleSketch(self.PadSketch2, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad2 = self.Doc.addObject("PartDesign::Pad", "Pad2")
        self.Body.addObject(self.Pad2)
        self.Pad2.Profile = self.PadSketch2
        self.Pad2.Length = 1
        self.Doc.recompute()

        # Assert everything is valid
        self.assertTrue(self.Pad.isValid())
        self.assertTrue(self.Pad1.isValid())
        self.assertTrue(self.Pad2.isValid())

        # Act
        # Move the second pad ( the sketch attachment point )
        self.PadSketch1.AttachmentOffset = App.Placement(
            App.Vector(0.5000000000, 0.0000000000, 0.0000000000),
            App.Rotation(0.0000000000, 0.0000000000, 0.0000000000),
        )
        self.Doc.recompute()

        # Assert everything is still valid.
        self.assertTrue(self.Pad.isValid())
        self.assertTrue(self.Pad1.isValid())

        if self.Body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            self.assertFalse(
                self.Pad2.isValid()
            )  # TNP problem is present without ElementMaps
        else:
            self.assertTrue(
                self.Pad2.isValid()
            )  # TNP problem is not present with ElementMaps

    def testPartDesignElementMapSketch(self):
        """Test that creating a sketch results in a correct element map."""
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = self.Doc.addObject("Sketcher::SketchObject", "SketchPad")
        body.addObject(sketch)
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        # Act
        self.Doc.recompute()
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        reverseMap = sketch.Shape.ElementReverseMap
        reverseFaces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        # Assert
        self.assertEqual(sketch.Shape.ElementMapSize, 12)
        self.assertEqual(len(reverseMap), 8)
        self.assertEqual(len(reverseFaces), 0)
        self.assertEqual(len(edges), 4)
        self.assertEqual(len(vertexes), 4)

    def testPartDesignBasicFusion(self):
        """Test that a basic fusion creates an element map, and refine retains it"""
        # Arrange
        doc = self.Doc
        box1 = doc.addObject("Part::Box", "Box")
        if App.GuiUp:
            mat = App.Material()
            mat.AmbientColor = (128, 0, 0)
            box1.ViewObject.ShapeAppearance = (
                mat  # Change color ( material ) for at least one
            )
        box2 = doc.addObject("Part::Box", "Box001")
        box3 = doc.addObject("Part::Box", "Box002")
        cyl1 = doc.addObject("Part::Cylinder", "Cylinder")
        fuse1 = doc.addObject("Part::MultiFuse", "Fusion")
        doc.Fusion.Shapes = [box1, box2]
        fuse2 = doc.addObject("Part::MultiFuse", "Fusion001")
        doc.Fusion001.Shapes = [box3, cyl1]
        doc.recompute()
        # Assert
        self.assertEqual(fuse1.Shape.ElementMapSize, 26)
        self.assertEqual(fuse2.Shape.ElementMapSize, 44)
        # Act
        doc.Fusion.Refine = True  # activate refinement
        doc.Fusion001.Refine = True  # activate refinement
        doc.recompute()
        self.assertEqual(fuse1.Shape.ElementMapSize, 26)
        self.assertEqual(fuse2.Shape.ElementMapSize, 44)

    def testPartDesignElementMapPad(self):
        """Test that padding a sketch results in a correct element map.  Note that comprehensive
        testing of the geometric functionality of the Pad is in TestPad.py"""
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        padSketch = self.Doc.addObject("Sketcher::SketchObject", "SketchPad")
        pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        body.addObject(padSketch)
        body.addObject(pad)
        TestSketcherApp.CreateRectangleSketch(padSketch, (0, 0), (1, 1))
        pad.Profile = padSketch
        pad.Length = 1
        # Act
        self.Doc.recompute()
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        reverseMap = pad.Shape.ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        # Assert
        self.assertEqual(pad.Shape.ElementMapSize, 30)  # 4 duplicated Vertexes in here
        self.assertEqual(len(reverseMap), 26)
        self.assertEqual(len(faces), 6)
        self.assertEqual(len(edges), 12)
        self.assertEqual(len(vertexes), 8)

    def testPartDesignElementMapBox(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act / Assert
        self.assertEqual(len(box.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(box.Shape.childShapes()), 1)
        self.assertEqual(box.Shape.childShapes()[0].ElementMapSize, 26)
        body.addObject(box)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 26)

    def testPartDesignElementMapCylinder(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        cylinder = self.Doc.addObject("PartDesign::AdditiveCylinder", "Cylinder")
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act / Assert
        self.assertEqual(len(cylinder.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(cylinder.Shape.childShapes()), 1)
        self.assertEqual(cylinder.Shape.childShapes()[0].ElementMapSize, 8)
        body.addObject(cylinder)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 8)
        self.assertEqual(len(reverseMap), 8)
        self.assertEqual(len(faces), 3)
        self.assertEqual(len(edges), 3)
        self.assertEqual(len(vertexes), 2)

    def testPartDesignElementMapSphere(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sphere = self.Doc.addObject("PartDesign::AdditiveSphere", "Sphere")
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act / Assert
        self.assertEqual(len(sphere.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(sphere.Shape.childShapes()), 1)
        self.assertEqual(sphere.Shape.childShapes()[0].ElementMapSize, 6)
        body.addObject(sphere)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 6)
        self.assertEqual(len(reverseMap), 6)
        self.assertEqual(len(faces), 1)
        self.assertEqual(len(edges), 3)
        self.assertEqual(len(vertexes), 2)

    def testPartDesignElementMapCone(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        cone = self.Doc.addObject("PartDesign::AdditiveCone", "Cone")
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act / Assert
        self.assertEqual(len(cone.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(cone.Shape.childShapes()), 1)
        self.assertEqual(cone.Shape.childShapes()[0].ElementMapSize, 8)
        body.addObject(cone)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 8)
        self.assertEqual(len(reverseMap), 8)
        self.assertEqual(len(faces), 3)
        self.assertEqual(len(edges), 3)
        self.assertEqual(len(vertexes), 2)

    def testPartDesignElementMapEllipsoid(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        ellipsoid = self.Doc.addObject("PartDesign::AdditiveEllipsoid", "Ellipsoid")
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act / Assert
        self.assertEqual(len(ellipsoid.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(ellipsoid.Shape.childShapes()), 1)
        self.assertEqual(ellipsoid.Shape.childShapes()[0].ElementMapSize, 6)
        body.addObject(ellipsoid)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 6)
        self.assertEqual(len(reverseMap), 6)
        self.assertEqual(len(faces), 1)
        self.assertEqual(len(edges), 3)
        self.assertEqual(len(vertexes), 2)

    def testPartDesignElementMapTorus(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        torus = self.Doc.addObject("PartDesign::AdditiveTorus", "Torus")
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act / Assert
        self.assertEqual(len(torus.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(torus.Shape.childShapes()), 1)
        self.assertEqual(torus.Shape.childShapes()[0].ElementMapSize, 4)
        body.addObject(torus)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 4)
        self.assertEqual(len(reverseMap), 4)
        self.assertEqual(len(faces), 1)
        self.assertEqual(len(edges), 2)
        self.assertEqual(len(vertexes), 1)

    def testPartDesignElementMapPrism(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        prism = self.Doc.addObject("PartDesign::AdditivePrism", "Prism")
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act / Assert
        self.assertEqual(len(prism.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(prism.Shape.childShapes()), 1)
        self.assertEqual(prism.Shape.childShapes()[0].ElementMapSize, 38)
        body.addObject(prism)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 38)
        self.assertEqual(len(reverseMap), 38)
        self.assertEqual(len(faces), 8)
        self.assertEqual(len(edges), 18)
        self.assertEqual(len(vertexes), 12)

    def testPartDesignElementMapWedge(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        wedge = self.Doc.addObject("PartDesign::AdditiveWedge", "Wedge")
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act / Assert
        self.assertEqual(len(wedge.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(wedge.Shape.childShapes()), 1)
        self.assertEqual(wedge.Shape.childShapes()[0].ElementMapSize, 26)
        body.addObject(wedge)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 26)
        self.assertEqual(len(reverseMap), 26)
        self.assertEqual(len(faces), 6)
        self.assertEqual(len(edges), 12)
        self.assertEqual(len(vertexes), 8)

        # body.BaseFeature = box

    def testPartDesignElementMapSubBox(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subbox = self.Doc.addObject("PartDesign::SubtractiveBox", "Box")
        subbox.BaseFeature = box
        body.addObject(subbox)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 44)

    def testPartDesignElementMapSubCylinder(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subcylinder = self.Doc.addObject("PartDesign::SubtractiveCylinder", "Cylinder")
        subcylinder.BaseFeature = box
        body.addObject(subcylinder)
        # Assert
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 38)

    def testPartDesignElementMapSubSphere(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subsphere = self.Doc.addObject("PartDesign::SubtractiveSphere", "Sphere")
        subsphere.BaseFeature = box
        body.addObject(subsphere)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 33)

    def testPartDesignElementMapSubCone(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subcone = self.Doc.addObject("PartDesign::SubtractiveCone", "Cone")
        subcone.BaseFeature = box
        body.addObject(subcone)
        # Assert
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 38)

    def testPartDesignElementMapSubEllipsoid(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subellipsoid = self.Doc.addObject(
            "PartDesign::SubtractiveEllipsoid", "Ellipsoid"
        )
        subellipsoid.BaseFeature = box
        body.addObject(subellipsoid)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 33)

    def testPartDesignElementMapSubTorus(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subtorus = self.Doc.addObject("PartDesign::SubtractiveTorus", "Torus")
        subtorus.BaseFeature = box
        body.addObject(subtorus)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 38)

    def testPartDesignElementMapSubPrism(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subprism = self.Doc.addObject("PartDesign::SubtractivePrism", "Prism")
        subprism.BaseFeature = box
        body.addObject(subprism)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 44)

    def testPartDesignElementMapSubWedge(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subwedge = self.Doc.addObject("PartDesign::SubtractiveWedge", "Wedge")
        subwedge.BaseFeature = box
        body.addObject(subwedge)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 50)

    def testPartDesignElementPadSketch(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        pad.Profile = sketch
        body.addObject(sketch)
        body.addObject(pad)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 30)  # The pad
        self.assertEqual(body.Shape.ElementMapSize, 26)
        self.assertEqual(sketch.Shape.ElementMapSize, 12)
        self.assertEqual(
            pad.Shape.ElementMapSize, 30
        )  # pad has the 26 plus the 4 original
        self.assertNotEqual(
            pad.Shape.ElementReverseMap["Vertex1"], "Vertex1"
        )  # NewName, not OldName
        self.assertEqual(
            self.countFacesEdgesVertexes(pad.Shape.ElementReverseMap), (6, 12, 8)
        )

        # Todo: Offer a way to turn on hashing and check that with a # starting
        #  Pad -> Extrusion -> makes compounds and does booleans, thus the resulting newName element maps
        #  See if we can turn those off, or try them on the other types?

    def testPartDesignElementMapRevolution(self):
        # App.KeepTestDoc = True    # Uncomment this if you want to keep the test document to examine
        self.Doc.UseHasher = False
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        sketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 1), (3, 2))  # (pt), (w,l)
        body.addObject(sketch)
        self.Doc.recompute()
        pad = self.Doc.addObject('PartDesign::Pad', 'Pad')
        pad.Profile = sketch
        pad.Length = 3
        body.addObject(pad)
        self.Doc.recompute()

        sketch2 = self.Doc.addObject('Sketcher::SketchObject', 'Sketch001')
        TestSketcherApp.CreateRectangleSketch(sketch2, (2, -3), (1, 2))  # (pt), (w,l)
        sketch2.AttachmentSupport = (pad, ["Face5"])
        sketch2.MapMode = 'FlatFace'
        body.addObject(sketch2)
        self.Doc.recompute()
        revolution = self.Doc.addObject('PartDesign::Revolution', 'Revolution')
        revolution.ReferenceAxis = (sketch2, ['V_Axis'])
        revolution.Reversed = 1
        revolution.Profile = sketch2
        revolution.Angle=180
        revolution.Refine = True
        body.addObject(revolution)
        volume = (math.pi * 3 * 3 - math.pi * 2 * 2) * 2  / 2
        padVolume =  3 * 3 * 2 # 50.26548245743668
        # Act
        self.Doc.recompute()
        # Assert the Shape is correct
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertAlmostEqual(pad.Shape.Volume, padVolume)
        self.assertAlmostEqual(revolution.Shape.Volume, volume + padVolume)
        # Assert the element map is correct
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 46)
        self.assertEqual(revolution.Shape.ElementMapSize, 46)
        self.assertEqual(self.countFacesEdgesVertexes(revolution.Shape.ElementReverseMap),
                         (9, 21, 14))
        self.assertEqual( revolution.Shape.ElementReverseMap["Vertex9"][1].count(";"), 3)
        self.assertEqual( revolution.Shape.ElementReverseMap["Face9"].count(";"), 16)
        # Arrange for an UpToFace mode test
        revolution.Type = 3
        revolution.UpToFace = (pad, ("Face4"))
        revolution.Reversed = 1
        revolution.Midplane = 0
        volume = (math.pi * 3 * 3 - math.pi * 2 * 2) * 2  / 4 * 3
        # Act
        self.Doc.recompute()
        # Assert UpToFace shape is correct
        self.assertAlmostEqual(revolution.Shape.Volume, volume + padVolume)
        # Assert UpToFace element map is correct
        self.assertEqual(self.countFacesEdgesVertexes(revolution.Shape.ElementReverseMap),
                         (8, 18, 12))
        # Assertions modified/added while reviewing PR#17119 by CalligaroV
        # Previously the condition counted the number of ";" (element map operations prefix)
        # If the number of operations changes then the number of ";" will change accordingly
        #
        # However, it is more useful to count the number of times an elemement name is
        # present in the MappedName of an element (a MappedName is definined also using the
        # element names - "Vertex*", "Edge*", "Face*" - used by an OCCT operation to generate
        # output elements)
        self.assertEqual( revolution.Shape.ElementReverseMap["Face8"].count("Face8"), 3)
        self.assertEqual( revolution.Shape.ElementReverseMap["Face8"].count("Face10"), 3)

    def testPartDesignElementMapLoft(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        sketch2 = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        TestSketcherApp.CreateRectangleSketch(sketch2, (0, 0), (2, 2))
        sketch2.Placement.move(App.Vector(0, 0, 3))
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        loft = self.Doc.addObject("PartDesign::AdditiveLoft", "Loft")
        loft.Profile = sketch
        loft.Sections = [sketch2]
        body.addObject(sketch)
        body.addObject(sketch2)
        body.addObject(loft)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        # 6 face 12 edge 8 vertexes = 26
        # 4 edges are duplicated (the originals from the sketch loft profile, and then those in the loft)
        # 4 vertexes are quad dups for 12 more.  26 + 4 + 12 = 42
        # self.assertEqual(body.Shape.ElementMapSize, 42)
        revMap = body.Shape.ElementReverseMap
        self.assertNotEqual(loft.Shape.ElementReverseMap["Vertex1"], "Vertex1")
        self.assertNotEqual(revMap["Vertex1"], "Vertex1")
        self.assertEqual(
            self.countFacesEdgesVertexes(loft.Shape.ElementReverseMap), (6, 12, 8)
        )
        volume = 7.0
        self.assertAlmostEqual(loft.Shape.Volume, volume)

    def testPartDesignElementMapPipe(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        sketch2 = self.Doc.addObject("Sketcher::SketchObject", "Sketch001")
        sketch2.AttachmentSupport = (self.Doc.getObject("XZ_Plane"), [""])
        sketch2.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(0, 1, 0)))
        sketch2.Placement = App.Placement(
            App.Vector(0, 0, 0), App.Rotation(App.Vector(1.00, 0.00, 0.00), 90.00)
        )
        # Need to set sketch2 placement?
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        pipe = self.Doc.addObject("PartDesign::AdditivePipe", "Pipe")
        pipe.Profile = sketch
        pipe.Spine = sketch2
        body.addObject(sketch)
        body.addObject(sketch2)
        body.addObject(pipe)
        self.Doc.recompute()
        # Assert
        self.assertAlmostEqual(body.Shape.Volume, 1)
        self.assertAlmostEqual(body.Shape.BoundBox.XMin, 0)
        self.assertAlmostEqual(body.Shape.BoundBox.YMin, 0)
        self.assertAlmostEqual(body.Shape.BoundBox.ZMin, 0)
        self.assertAlmostEqual(body.Shape.BoundBox.XMax, 1)
        self.assertAlmostEqual(body.Shape.BoundBox.YMax, 1)
        self.assertAlmostEqual(body.Shape.BoundBox.ZMax, 1)
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 26)
        revMap = body.Shape.childShapes()[0].ElementReverseMap
        # revMap = pipe.Shape.ElementReverseMap
        # TODO: This is a child of the body and not the actual Pipe.
        #   1: is that okay and normal, or should the pipe have an element map
        self.assertEqual(self.countFacesEdgesVertexes(revMap), (6, 12, 8))

    def testPartDesignElementMapHelix(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        helix = self.Doc.addObject("PartDesign::AdditiveHelix", "Helix")
        helix.Profile = sketch
        helix.ReferenceAxis = (self.Doc.getObject("Sketch"), ["N_Axis"])
        # helix.Mode = 0
        body.addObject(sketch)
        body.addObject(helix)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertGreaterEqual(body.Shape.childShapes()[0].ElementMapSize, 26)
        revMap = body.Shape.childShapes()[0].ElementReverseMap
        self.assertEqual(self.countFacesEdgesVertexes(revMap), (14, 28, 16))
        Radius = 0  # Rectangle is on the axis, but wouldn't matter regardless here
        Area = Part.Face(sketch.Shape).Area
        # General helix formula; not actually used here since devolves to just the
        # height in this orientation.
        helixLength = (
            helix.Height.Value
            / helix.Pitch.Value
            * math.sqrt((math.pi * Radius) ** 2 + helix.Pitch.Value**2)
        )
        Volume = Area * helixLength
        self.assertAlmostEqual(Area, 1)
        self.assertAlmostEqual(helixLength, helix.Height.Value)
        self.assertAlmostEqual(helix.Shape.Volume, Volume, 2)
        self.assertEqual(body.Shape.ElementMapSize, 58)

    def testPartDesignElementMapPocket(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        sketch.AttachmentSupport = (box, "Face6")
        sketch.MapMode = "FlatFace"
        TestSketcherApp.CreateRectangleSketch(sketch, (1, 1), (1, 1))
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        pocket = self.Doc.addObject("PartDesign::Pocket", "Pocket")
        pocket.Profile = sketch
        pocket.Length = 5
        pocket.Direction = (0, 0, -1)
        pocket.ReferenceAxis = (sketch, ["N_Axis"])
        pocket.Refine = True

        body.addObject(sketch)
        body.addObject(pocket)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 51)
        self.assertEqual(body.Shape.ElementMapSize, 51)
        self.assertEqual(sketch.Shape.ElementMapSize, 12)
        self.assertEqual(pocket.Shape.ElementMapSize, 51)
        self.assertNotEqual(
            pocket.Shape.ElementReverseMap["Vertex1"], "Vertex1"
        )  # NewName, not OldName
        self.assertEqual(
            self.countFacesEdgesVertexes(pocket.Shape.ElementReverseMap), (11, 24, 16)
        )
        volume = 1000 - 5 * 1 * 1
        self.assertAlmostEqual(pocket.Shape.Volume, volume)

    def testPartDesignElementMapHole(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        sketch.AttachmentSupport = (box, "Face6")
        sketch.MapMode = "FlatFace"
        TestSketcherApp.CreateCircleSketch(sketch, (5, 5), 1)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        hole = self.Doc.addObject("PartDesign::Hole", "Hole")
        hole.Profile = sketch

        body.addObject(sketch)
        body.addObject(hole)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 32)
        self.assertEqual(body.Shape.ElementMapSize, 32)
        self.assertEqual(sketch.Shape.ElementMapSize, 2)
        self.assertEqual(hole.Shape.ElementMapSize, 32)
        # self.assertNotEqual(hole.Shape.ElementReverseMap['Vertex1'],"Vertex1")   # NewName, not OldName
        self.assertEqual(
            self.countFacesEdgesVertexes(hole.Shape.ElementReverseMap), (7, 15, 10)
        )
        volume = 1000 - 10 * math.pi * 3 * 3
        self.assertAlmostEqual(hole.Shape.Volume, volume)

    def testPartDesignElementMapGroove(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        groove = self.Doc.addObject("PartDesign::Groove", "Groove")
        body.addObject(groove)
        groove.ReferenceAxis = (self.Doc.getObject("Y_Axis"), [""])
        groove.Angle = 360.0
        groove.Profile = (box, ["Face6"])
        groove.ReferenceAxis = (box, ["Edge9"])
        groove.Midplane = 0
        groove.Reversed = 0
        groove.Base = App.Vector(0, 0, 0)
        self.Doc.recompute()
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Assert
        revMap = (
            groove.Shape.ElementReverseMap
        )  # body.Shape.childShapes()[0].ElementReverseMap
        self.assertEqual(self.countFacesEdgesVertexes(revMap), (5, 9, 6))
        volume = 785.3981633974482  # TODO:  math formula to calc this.  Maybe make a sketch as the Profile.
        self.assertAlmostEqual(groove.Shape.Volume, volume)

    def testPartDesignElementMapSubLoft(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        TestSketcherApp.CreateRectangleSketch(sketch, (1, 1), (1, 1))
        sketch2 = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        TestSketcherApp.CreateRectangleSketch(sketch2, (1, 1), (2, 2))
        sketch2.Placement.move(App.Vector(0, 0, 3))
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        loft = self.Doc.addObject("PartDesign::SubtractiveLoft", "SubLoft")
        loft.Profile = sketch
        loft.Sections = [sketch2]
        body.addObject(loft)
        self.Doc.recompute()
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Assert
        revMap = (
            loft.Shape.ElementReverseMap
        )  # body.Shape.childShapes()[0].ElementReverseMap
        self.assertEqual(self.countFacesEdgesVertexes(revMap), (11, 24, 16))
        volume = 993  # TODO:  math formula to calc this.
        self.assertAlmostEqual(loft.Shape.Volume, volume)

    def testPartDesignElementMapSubPipe(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        sketch2 = self.Doc.addObject("Sketcher::SketchObject", "Sketch001")
        sketch2.AttachmentSupport = (self.Doc.getObject("XZ_Plane"), [""])
        sketch2.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(0, 1, 0)))
        sketch2.Placement = App.Placement(
            App.Vector(0, 0, 0), App.Rotation(App.Vector(1.00, 0.00, 0.00), 90.00)
        )
        # Need to set sketch2 placement?
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        pipe = self.Doc.addObject("PartDesign::SubtractivePipe", "SubPipe")
        pipe.Profile = sketch
        pipe.Spine = sketch2
        body.addObject(sketch)
        body.addObject(sketch2)
        body.addObject(pipe)
        self.Doc.recompute()
        # Assert
        self.assertAlmostEqual(body.Shape.Volume, 999)
        self.assertAlmostEqual(body.Shape.BoundBox.XMin, 0)
        self.assertAlmostEqual(body.Shape.BoundBox.YMin, 0)
        self.assertAlmostEqual(body.Shape.BoundBox.ZMin, 0)
        self.assertAlmostEqual(body.Shape.BoundBox.XMax, 10)
        self.assertAlmostEqual(body.Shape.BoundBox.YMax, 10)
        self.assertAlmostEqual(body.Shape.BoundBox.ZMax, 10)
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 44)
        revMap = body.Shape.ElementReverseMap  #
        self.assertEqual(self.countFacesEdgesVertexes(revMap), (9, 21, 14))

    def testPartDesignElementMapSubHelix(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        TestSketcherApp.CreateRectangleSketch(sketch, (5, 5), (1, 1))
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        helix = self.Doc.addObject("PartDesign::SubtractiveHelix", "SubHelix")
        helix.Profile = sketch
        helix.ReferenceAxis = (self.Doc.getObject("Sketch"), ["V_Axis"])
        helix.Reversed = True
        body.addObject(sketch)
        body.addObject(helix)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 50)
        revMap = body.Shape.childShapes()[0].ElementReverseMap
        self.assertEqual(self.countFacesEdgesVertexes(revMap), (10, 24, 16))
        volume = 991.3606  # TODO:  math formula to calc this.
        self.assertAlmostEqual(helix.Shape.Volume, volume, 4)

    def testPartDesignElementMapChamfer(self):
        """Test Chamfer ( and  FeatureDressup )"""
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        if body.Shape.ElementMapVersion == "":  # Skip without element maps.
            return
        chamfer = self.Doc.addObject("PartDesign::Chamfer", "Chamfer")
        chamfer.Base = (
            box,
            [
                "Edge1",
                "Edge2",
                "Edge3",
                "Edge4",
                "Edge5",
                "Edge6",
                "Edge7",
                "Edge8",
                "Edge9",
                "Edge10",
                "Edge11",
                "Edge12",
            ],
        )
        chamfer.Size = 1
        chamfer.UseAllEdges = True
        # Act / Assert
        body.addObject(box)
        body.addObject(chamfer)
        self.Doc.recompute()
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 98)
        self.assertEqual(len(reverseMap), 98)
        self.assertEqual(len(faces), 26)  # 6 Faces become 26 ( +8 + 2*6 )
        self.assertEqual(len(edges), 48)  # 12 Edges become 48
        self.assertEqual(len(vertexes), 24)  # 8 Vertices become 24

    def testPartDesignElementMapFillet(self):
        """Test Fillet ( and  FeatureDressup )"""
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        if body.Shape.ElementMapVersion == "":  # Skip without element maps.
            return
        fillet = self.Doc.addObject("PartDesign::Fillet", "Fillet")
        fillet.Base = (
            box,
            [
                "Edge1",
                "Edge2",
                "Edge3",
                "Edge4",
                "Edge5",
                "Edge6",
                "Edge7",
                "Edge8",
                "Edge9",
                "Edge10",
                "Edge11",
                "Edge12",
            ],
        )
        # Act / Assert
        body.addObject(box)
        body.addObject(fillet)
        self.Doc.recompute()
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 106)
        self.assertEqual(len(reverseMap), 106)
        self.assertEqual(len(faces), 26)  # 6 Faces become 26 ( +8 + 2*6 )
        self.assertEqual(len(edges), 56)  # 12 Edges become 56  Why?
        self.assertEqual(len(vertexes), 24)  # 8 Vertices become 24

    def testPartDesignElementMapTransform(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        if body.Shape.ElementMapVersion == "":  # Skip without element maps.
            return
        multitransform = self.Doc.addObject(
            "PartDesign::MultiTransform", "MultiTransform"
        )
        scaled = self.Doc.addObject("PartDesign::Scaled", "Scaled")
        scaled.Factor = 2
        scaled.Occurrences = 2
        multitransform.Transformations = scaled
        multitransform.Shape = box.Shape

        # Act / Assert
        self.Doc.recompute()
        body.addObject(box)
        body.addObject(multitransform)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 26)

    def testPartDesignElementMapShapeBinder(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        shapebinder = self.Doc.addObject("PartDesign::ShapeBinder", "ShapeBinder")
        if body.Shape.ElementMapVersion == "":  # Skip without element maps.
            return
        # Act / Assert
        body.addObject(box)
        body.addObject(shapebinder)
        shapebinder.Support = [box]
        self.Doc.recompute()
        self.assertEqual(len(shapebinder.Shape.childShapes()), 1)
        self.assertEqual(shapebinder.Shape.childShapes()[0].ElementMapSize, 26)

    def testPartDesignElementMapSubShapeBinder(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        subshapebinder = self.Doc.addObject(
            "PartDesign::SubShapeBinder", "SubShapeBinder"
        )
        if body.Shape.ElementMapVersion == "":  # Skip without element maps.
            return
        # Act / Assert
        body.addObject(box)
        body.addObject(subshapebinder)
        subshapebinder.Support = [(box, ["Face1"])]
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(subshapebinder.Shape.childShapes()[0].ElementMapSize, 9)

    def testSketchElementMap(self):
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        body.addObject(sketch)
        self.Doc.recompute()
        self.assertEqual(sketch.Shape.ElementMapSize, 12)
        pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        pad.Profile = sketch
        body.addObject(pad)
        self.Doc.recompute()
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Assert
        self.assertEqual(sketch.Shape.ElementMapSize, 12)
        self.assertEqual(
            pad.Shape.ElementMapSize, 30
        )  # The sketch plus the pad in the map
        # TODO:  differing results between main and LS3 on these values.  Does it matter?
        # self.assertEqual(body.Shape.ElementMapSize,0)   # 8?
        # self.assertEqual(body.Shape.ElementMapSize,30) # 26

    def testPlaneElementMap(self):
        plane = self.Doc.addObject("Part::Plane", "Plane")
        plane.Length = 10
        plane.Width = 10
        self.Doc.recompute()
        self.assertEqual(plane.Shape.ElementMapSize, 0)
        pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        pad.Profile = plane
        self.Doc.recompute()
        if pad.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Assert
        self.assertEqual(plane.Shape.ElementMapSize, 0)
        self.assertEqual(pad.Shape.ElementMapSize, 26)

    def testChangeSketch(self):
        # Arrange
        doc = self.Doc

        body = doc.addObject("PartDesign::Body", "Body")
        # Make first offset cube Pad
        padSketch = doc.addObject("Sketcher::SketchObject", "Sketch")
        body.addObject(padSketch)
        TestSketcherApp.CreateRectangleSketch(padSketch, (0, 0), (31.37, 25.2))
        doc.recompute()
        pad = doc.addObject("PartDesign::Pad", "Pad")
        body.addObject(pad)
        pad.Profile = padSketch
        pad.Length = 10
        doc.recompute()

        sketch001 = body.newObject("Sketcher::SketchObject", "Sketch001")
        sketch001 = doc.Sketch001
        sketch001.AttachmentSupport = (
            doc.getObject("Pad"),
            [
                "Face6",
            ],
        )
        sketch001.MapMode = "FlatFace"
        App.ActiveDocument.recompute()

        geoList = []
        geoList.append(
            Part.Circle(
                App.Vector(15.093666, 13.036922, 0.000000),
                App.Vector(0.000000, 0.000000, 1.000000),
                5.000000,
            )
        )
        sketch001.addGeometry(geoList, False)
        del geoList
        sketch001.addConstraint(Sketcher.Constraint("Radius", 0, 5.000000))
        doc.recompute()

        pad001 = body.newObject("PartDesign::Pad", "Pad001")
        pad001.Profile = doc.getObject("Sketch001")
        pad001.Length = 10
        App.ActiveDocument.recompute()
        pad001.ReferenceAxis = (doc.getObject("Sketch001"), ["N_Axis"])
        sketch001.Visibility = False
        App.ActiveDocument.recompute()
        pad001.Length = 10.000000
        pad001.TaperAngle = 0.000000
        pad001.UseCustomVector = 0
        pad001.Direction = (0, 0, 1)
        pad001.ReferenceAxis = (doc.getObject("Sketch001"), ["N_Axis"])
        pad001.AlongSketchNormal = 1
        pad001.Type = 0
        pad001.UpToFace = None
        pad001.Reversed = 0
        pad001.Midplane = 0
        pad001.Offset = 0
        doc.recompute()
        doc.getObject("Pad").Visibility = False

        doc.getObject("Sketch001").Visibility = False

        # Modify the original sketch to generate TNP issue
        geoList = []
        geoList.append(
            Part.LineSegment(
                App.Vector(2.510468, 22.837425, 0.000000),
                App.Vector(2.510468, 19.933617, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(2.510468, 19.933617, 0.000000),
                App.Vector(4.869811, 19.933617, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(4.869811, 19.933617, 0.000000),
                App.Vector(4.869811, 22.837425, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(4.869811, 22.837425, 0.000000),
                App.Vector(2.510468, 22.837425, 0.000000),
            )
        )
        padSketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        constraintList.append(Sketcher.Constraint("Coincident", 4, 2, 5, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 5, 2, 6, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 6, 2, 7, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 7, 2, 4, 1))
        constraintList.append(Sketcher.Constraint("Vertical", 4))
        constraintList.append(Sketcher.Constraint("Vertical", 6))
        constraintList.append(Sketcher.Constraint("Horizontal", 5))
        constraintList.append(Sketcher.Constraint("Horizontal", 7))
        padSketch.addConstraint(constraintList)
        del constraintList
        doc.recompute()
        # Assert
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        self.assertEqual(body.Shape.BoundBox.XMin, 0)
        self.assertEqual(body.Shape.BoundBox.YMin, 0)
        self.assertEqual(body.Shape.BoundBox.ZMin, 0)
        self.assertEqual(body.Shape.BoundBox.XMax, 31.37)
        self.assertEqual(body.Shape.BoundBox.YMax, 25.2)
        self.assertEqual(body.Shape.BoundBox.ZMax, 20)

    def testApplyFillet(self):
        # Arrange
        doc = self.Doc
        body = doc.addObject("PartDesign::Body", "Body")
        # Make first offset cube Pad
        padSketch = doc.addObject("Sketcher::SketchObject", "Sketch")
        body.addObject(padSketch)
        TestSketcherApp.CreateRectangleSketch(padSketch, (0, 0), (31.37, 25.2))
        doc.recompute()
        pad = doc.addObject("PartDesign::Pad", "Pad")
        body.addObject(pad)
        pad.Profile = padSketch
        pad.Length = 10
        doc.recompute()

        sketch001 = body.newObject("Sketcher::SketchObject", "Sketch001")
        sketch001.AttachmentSupport = (
            doc.getObject("Pad"),
            [
                "Face6",
            ],
        )
        sketch001.MapMode = "FlatFace"

        geoList = []
        geoList.append(
            Part.Circle(
                App.Vector(15.093666, 13.036922, 0.000000),
                App.Vector(0.000000, 0.000000, 1.000000),
                5.000000,
            )
        )
        sketch001.addGeometry(geoList, False)
        del geoList
        sketch001.addConstraint(Sketcher.Constraint("Radius", 0, 5.000000))
        doc.recompute()

        pad001 = body.newObject("PartDesign::Pad", "Pad001")
        pad001.Profile = doc.getObject("Sketch001")
        pad001.Length = 10
        App.ActiveDocument.recompute()
        pad001.ReferenceAxis = (doc.getObject("Sketch001"), ["N_Axis"])
        sketch001.Visibility = False
        App.ActiveDocument.recompute()

        pad001.Length = 10.000000
        pad001.TaperAngle = 0.000000
        pad001.UseCustomVector = 0
        pad001.Direction = (0, 0, 1)
        pad001.ReferenceAxis = (doc.getObject("Sketch001"), ["N_Axis"])
        pad001.AlongSketchNormal = 1
        pad001.Type = 0
        pad001.UpToFace = None
        pad001.Reversed = 0
        pad001.Midplane = 0
        pad001.Offset = 0
        doc.recompute()
        doc.getObject("Pad").Visibility = False

        doc.getObject("Sketch001").Visibility = False

        area1 = pad.Shape.Area
        # Act
        doc.getObject("Sketch").fillet(
            2,
            3,
            App.Vector(6.673934, 25.000000, 0),
            App.Vector(0.000000, 21.980343, 0),
            4.740471,
            True,
            True,
            False,
        )
        doc.recompute()
        area2 = pad.Shape.Area

        # Assert
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        self.assertEqual(body.Shape.BoundBox.XMin, 0)
        self.assertEqual(body.Shape.BoundBox.YMin, 0)
        self.assertEqual(body.Shape.BoundBox.ZMin, 0)
        self.assertEqual(body.Shape.BoundBox.XMax, 31.37)
        self.assertAlmostEqual(body.Shape.BoundBox.YMax, 25.2)
        self.assertEqual(body.Shape.BoundBox.ZMax, 20)
        self.assertNotEqual(area1, area2)

    def testShapeBinder(self):
        doc = self.Doc
        self.Body = doc.addObject("PartDesign::Body", "TNP_Test_Body_SubShape")
        doc.getObject("TNP_Test_Body_SubShape").Label = "TNP_Test_Body_SubShape"

        doc.recompute()
        doc.getObject("TNP_Test_Body_SubShape").newObject(
            "Sketcher::SketchObject", "Sketch"
        )
        doc.Sketch.AttachmentSupport = (doc.getObject("XY_Plane"), [""])
        doc.Sketch.MapMode = "FlatFace"
        doc.recompute()

        geoList = []
        geoList.append(
            Part.LineSegment(
                App.Vector(0.000000, 0.000000, 0.000000),
                App.Vector(35.000000, 0.000000, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(35.000000, 0.000000, 0.000000),
                App.Vector(35.000000, 25.000000, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(35.000000, 25.000000, 0.000000),
                App.Vector(0.000000, 25.000000, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(0.000000, 25.000000, 0.000000),
                App.Vector(0.000000, 0.000000, 0.000000),
            )
        )
        doc.Sketch.addGeometry(geoList, False)
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
        doc.Sketch.addConstraint(constraintList)
        del constraintList

        doc.Sketch.addConstraint(Sketcher.Constraint("Distance", 1, 1, 3, 2, 35.000000))
        doc.Sketch.addConstraint(Sketcher.Constraint("Distance", 0, 1, 2, 2, 25.000000))
        doc.Sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))

        doc.recompute()

        doc.getObject("TNP_Test_Body_SubShape").newObject("PartDesign::Pad", "Pad")
        doc.Pad.Profile = doc.Sketch
        doc.Pad.Length = 10
        doc.recompute()
        doc.Pad.ReferenceAxis = (doc.Sketch, ["N_Axis"])
        doc.Sketch.Visibility = False
        doc.recompute()
        doc.Pad.Length = 10.000000
        doc.Pad.TaperAngle = 0.000000
        doc.Pad.UseCustomVector = 0
        doc.Pad.Direction = (0, 0, 1)
        doc.Pad.ReferenceAxis = (doc.Sketch, ["N_Axis"])
        doc.Pad.AlongSketchNormal = 1
        doc.Pad.Type = 0
        doc.Pad.UpToFace = None
        doc.Pad.Reversed = 0
        doc.Pad.Midplane = 0
        doc.Pad.Offset = 0
        doc.recompute()
        doc.Sketch.Visibility = False

        doc.addObject("PartDesign::Body", "TNP_Test_Body_Second")
        doc.getObject("TNP_Test_Body_Second").Label = "TNP_Test_Body_Second"
        doc.recompute()
        obj = doc.getObject("TNP_Test_Body_Second").newObject(
            "PartDesign::ShapeBinder", "ShapeBinder"
        )
        obj.Support = (doc.getObject("TNP_Test_Body_SubShape"), ["Face6"])
        doc.recompute()
        doc.getObject("TNP_Test_Body_Second").newObject(
            "Sketcher::SketchObject", "Sketch001"
        )
        doc.getObject("Sketch001").AttachmentSupport = (
            doc.getObject("ShapeBinder"),
            [""],
        )
        doc.getObject("Sketch001").MapMode = "FlatFace"
        doc.recompute()

        geoList = []
        geoList.append(
            Part.Circle(
                App.Vector(14.725412, 16.666899, 0.000000),
                App.Vector(0.000000, 0.000000, 1.000000),
                2.162720,
            )
        )
        doc.getObject("Sketch001").addGeometry(geoList, False)
        del geoList

        doc.recompute()
        doc.getObject("TNP_Test_Body_Second").newObject("PartDesign::Pad", "Pad001")
        doc.getObject("Pad001").Profile = doc.getObject("Sketch001")
        doc.getObject("Pad001").Length = 10
        doc.recompute()
        doc.getObject("Pad001").ReferenceAxis = (doc.getObject("Sketch001"), ["N_Axis"])
        doc.getObject("Sketch001").Visibility = False
        doc.recompute()
        doc.getObject("Pad001").Length = 10.000000
        doc.getObject("Pad001").TaperAngle = 0.000000
        doc.getObject("Pad001").UseCustomVector = 0
        doc.getObject("Pad001").Direction = (0, 0, 1)
        doc.getObject("Pad001").ReferenceAxis = (doc.getObject("Sketch001"), ["N_Axis"])
        doc.getObject("Pad001").AlongSketchNormal = 1
        doc.getObject("Pad001").Type = 0
        doc.getObject("Pad001").UpToFace = None
        doc.getObject("Pad001").Reversed = 0
        doc.getObject("Pad001").Midplane = 0
        doc.getObject("Pad001").Offset = 0
        doc.recompute()
        doc.getObject("ShapeBinder").Visibility = False
        doc.getObject("Sketch001").Visibility = False

        geoList = []
        geoList.append(
            Part.LineSegment(
                App.Vector(28.380075, 21.486303, 0.000000),
                App.Vector(28.380075, 15.462212, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(28.380075, 15.462212, 0.000000),
                App.Vector(32.797741, 15.462212, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(32.797741, 15.462212, 0.000000),
                App.Vector(32.797741, 21.486303, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(32.797741, 21.486303, 0.000000),
                App.Vector(28.380075, 21.486303, 0.000000),
            )
        )
        doc.Sketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        constraintList.append(Sketcher.Constraint("Coincident", 4, 2, 5, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 5, 2, 6, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 6, 2, 7, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 7, 2, 4, 1))
        constraintList.append(Sketcher.Constraint("Vertical", 4))
        constraintList.append(Sketcher.Constraint("Vertical", 6))
        constraintList.append(Sketcher.Constraint("Horizontal", 5))
        constraintList.append(Sketcher.Constraint("Horizontal", 7))
        doc.Sketch.addConstraint(constraintList)
        del constraintList

        doc.recompute()
        # Assert
        if self.Body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        self.assertEqual(self.Body.Shape.BoundBox.XMin, 0)
        self.assertEqual(self.Body.Shape.BoundBox.YMin, 0)
        self.assertEqual(self.Body.Shape.BoundBox.ZMin, 0)
        self.assertEqual(self.Body.Shape.BoundBox.XMax, 35)
        self.assertEqual(self.Body.Shape.BoundBox.YMax, 25)
        self.assertEqual(self.Body.Shape.BoundBox.ZMax, 10)

    def testSubShapeBinder(self):
        doc = self.Doc
        self.Body = doc.addObject("PartDesign::Body", "Body")
        doc.Body.Label = "Body"
        doc.recompute()
        doc.Body.newObject("Sketcher::SketchObject", "Sketch")
        doc.Sketch.AttachmentSupport = (doc.getObject("XY_Plane"), [""])
        doc.Sketch.MapMode = "FlatFace"
        doc.recompute()

        geoList = []
        geoList.append(
            Part.LineSegment(
                App.Vector(0.000000, 0.000000, 0.000000),
                App.Vector(35.000000, 0.000000, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(35.000000, 0.000000, 0.000000),
                App.Vector(35.000000, 25.000000, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(35.000000, 25.000000, 0.000000),
                App.Vector(0.000000, 25.000000, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(0.000000, 25.000000, 0.000000),
                App.Vector(0.000000, 0.000000, 0.000000),
            )
        )
        doc.Sketch.addGeometry(geoList, False)
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
        doc.Sketch.addConstraint(constraintList)
        del constraintList

        doc.Sketch.addConstraint(Sketcher.Constraint("Distance", 1, 1, 3, 2, 35.000000))
        doc.Sketch.addConstraint(Sketcher.Constraint("Distance", 0, 1, 2, 2, 25.000000))
        doc.Sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))

        doc.recompute()
        doc.Body.newObject("PartDesign::Pad", "Pad")
        doc.Pad.Profile = doc.Sketch
        doc.Pad.Length = 10
        doc.recompute()
        doc.Pad.ReferenceAxis = (doc.Sketch, ["N_Axis"])
        doc.Sketch.Visibility = False
        doc.Pad.Length = 10.000000
        doc.Pad.TaperAngle = 0.000000
        doc.Pad.UseCustomVector = 0
        doc.Pad.Direction = (0, 0, 1)
        doc.Pad.ReferenceAxis = (doc.Sketch, ["N_Axis"])
        doc.Pad.AlongSketchNormal = 1
        doc.Pad.Type = 0
        doc.Pad.UpToFace = None
        doc.Pad.Reversed = 0
        doc.Pad.Midplane = 0
        doc.Pad.Offset = 0
        doc.recompute()
        doc.Sketch.Visibility = False
        doc.addObject("PartDesign::Body", "Body001")
        doc.getObject("Body001").Label = "Body"
        doc.recompute()
        binder = doc.getObject("Body001").newObject(
            "PartDesign::SubShapeBinder", "Binder"
        )
        binder.Support = self.Body
        doc.getObject("Body001").newObject("Sketcher::SketchObject", "Sketch001")
        doc.getObject("Sketch001").AttachmentSupport = (
            doc.getObject("Binder"),
            [
                "Face6",
            ],
        )
        doc.getObject("Sketch001").MapMode = "FlatFace"
        doc.recompute()

        geoList = []
        geoList.append(
            Part.Circle(
                App.Vector(16.566162, 13.537925, 0.000000),
                App.Vector(0.000000, 0.000000, 1.000000),
                2.197371,
            )
        )
        doc.getObject("Sketch001").addGeometry(geoList, False)
        del geoList

        doc.recompute()
        ### Begin command PartDesign_Pad
        doc.getObject("Body001").newObject("PartDesign::Pad", "Pad001")
        doc.Pad001.Profile = doc.getObject("Sketch001")
        doc.Pad001.Length = 10
        doc.recompute()
        doc.Pad001.ReferenceAxis = (doc.getObject("Sketch001"), ["N_Axis"])
        doc.getObject("Sketch001").Visibility = False
        doc.recompute()
        doc.Pad001.Length = 10.000000
        doc.Pad001.TaperAngle = 0.000000
        doc.Pad001.UseCustomVector = 0
        doc.Pad001.Direction = (0, 0, 1)
        doc.Pad001.ReferenceAxis = (doc.getObject("Sketch001"), ["N_Axis"])
        doc.Pad001.AlongSketchNormal = 1
        doc.Pad001.Type = 0
        doc.Pad001.UpToFace = None
        doc.Pad001.Reversed = 0
        doc.Pad001.Midplane = 0
        doc.Pad001.Offset = 0
        doc.recompute()
        doc.getObject("Binder").Visibility = False
        doc.getObject("Sketch001").Visibility = False

        geoList = []
        geoList.append(
            Part.LineSegment(
                App.Vector(30.009926, 21.026653, 0.000000),
                App.Vector(30.009926, 16.425089, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(30.009926, 16.425089, 0.000000),
                App.Vector(31.994911, 16.425089, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(31.994911, 16.425089, 0.000000),
                App.Vector(31.994911, 21.026653, 0.000000),
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(31.994911, 21.026653, 0.000000),
                App.Vector(30.009926, 21.026653, 0.000000),
            )
        )
        doc.Sketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        constraintList.append(Sketcher.Constraint("Coincident", 4, 2, 5, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 5, 2, 6, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 6, 2, 7, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 7, 2, 4, 1))
        constraintList.append(Sketcher.Constraint("Vertical", 4))
        constraintList.append(Sketcher.Constraint("Vertical", 6))
        constraintList.append(Sketcher.Constraint("Horizontal", 5))
        constraintList.append(Sketcher.Constraint("Horizontal", 7))
        doc.Sketch.addConstraint(constraintList)
        del constraintList

        doc.recompute()
        # Assert
        if self.Body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        self.assertEqual(self.Body.Shape.BoundBox.XMin, 0)
        self.assertEqual(self.Body.Shape.BoundBox.YMin, 0)
        self.assertEqual(self.Body.Shape.BoundBox.ZMin, 0)
        self.assertEqual(self.Body.Shape.BoundBox.XMax, 35)
        self.assertEqual(self.Body.Shape.BoundBox.YMax, 25)
        self.assertEqual(self.Body.Shape.BoundBox.ZMax, 10)

    def testPartDesignTNPChamfer(self):
        """Test Chamfer"""
        # Arrange
        doc = self.Doc
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        self.Doc.recompute()
        volume1 = body.Shape.Volume
        chamfer = self.Doc.addObject("PartDesign::Chamfer", "Chamfer")
        chamfer.Base = (
            box,
            [
                "Edge1",
                "Edge5",
                "Edge7",
            ],
        )
        chamfer.Size = 1
        body.addObject(chamfer)
        self.Doc.recompute()
        volume2 = body.Shape.Volume

        doc.Body.newObject("Sketcher::SketchObject", "Sketch")
        doc.Sketch.AttachmentSupport = (chamfer, "Face8")
        doc.Sketch.MapMode = "FlatFace"
        doc.recompute()

        x1, x2, y1, y2 = (
            10 / math.sqrt(2) - math.sqrt(2),
            10 / math.sqrt(2) + math.sqrt(2),
            6,
            11,
        )
        geoList = []
        geoList.append(
            Part.LineSegment(App.Vector(x1, y1, 0.0), App.Vector(x1, y2, 0.0))
        )
        geoList.append(
            Part.LineSegment(App.Vector(x1, y2, 0.0), App.Vector(x2, y2, 0.0))
        )
        geoList.append(
            Part.LineSegment(App.Vector(x2, y2, 0.0), App.Vector(x2, y1, 0.0))
        )
        geoList.append(
            Part.LineSegment(App.Vector(x2, y1, 0.0), App.Vector(x1, y1, 0.0))
        )
        doc.Sketch.addGeometry(geoList, False)
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
        doc.Sketch.addConstraint(constraintList)
        del constraintList
        body.addObject(doc.Sketch)

        pocket = self.Doc.addObject("PartDesign::Pocket", "Pocket")
        pocket.Type = "Length"
        pocket.Length = 3
        pocket.Direction = App.Vector(-0.710000000, 0.7100000000, 0.0000000000)
        pocket.Profile = doc.Sketch
        body.addObject(pocket)
        self.Doc.recompute()
        volume3 = body.Shape.Volume
        # Change the chamfered edges, potentially triggering TNP
        chamfer.Base = (
            box,
            [
                "Edge5",
                "Edge7",
            ],
        )
        self.Doc.recompute()
        volume4 = body.Shape.Volume
        # Assert
        if body.Shape.ElementMapVersion == "":  # Skip without element maps.
            return
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 62)
        self.assertEqual(len(reverseMap), 62)
        self.assertEqual(len(faces), 12)
        self.assertEqual(len(edges), 30)
        self.assertEqual(len(vertexes), 20)
        boxVolume = 10 * 10 * 10
        chamferVolume = 1 * 1 * 0.5 * 10
        # cut area is rectangle with sqrt(2) as one side minus 2 isosceles right triangles
        cutArea = (2 * math.sqrt(2)) * 3 - (
            (math.sqrt(2) / 2 * math.sqrt(2) / 2) / 2
        ) * 2
        cutVolume = cutArea * 4  # height is 4  ( 11-6 with a limit of 10 from the box )
        self.assertAlmostEqual(volume1, boxVolume)
        self.assertAlmostEqual(volume2, boxVolume - 3 * chamferVolume)
        self.assertAlmostEqual(volume3, boxVolume - 3 * chamferVolume - cutVolume, 4)
        self.assertAlmostEqual(volume4, boxVolume - 2 * chamferVolume - cutVolume, 4)

    def testPartDesignTNPFillet(self):
        """Test Fillet"""
        # Arrange
        doc = self.Doc
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        self.Doc.recompute()
        volume1 = body.Shape.Volume
        fillet = self.Doc.addObject("PartDesign::Fillet", "Fillet")
        fillet.Refine = True
        fillet.Base = (
            box,
            [
                "Edge1",
                "Edge5",
                "Edge7",
            ],
        )
        # fillet.Size = 1
        body.addObject(fillet)
        self.Doc.recompute()
        volume2 = body.Shape.Volume

        doc.Body.newObject("Sketcher::SketchObject", "Sketch")
        doc.Sketch.AttachmentSupport = (fillet, "Face2")
        doc.Sketch.MapMode = "FlatFace"
        doc.recompute()

        x1, x2, y1, y2 = 4, 6, 6, 11
        geoList = []
        geoList.append(
            Part.LineSegment(App.Vector(x1, y1, 0.0), App.Vector(x1, y2, 0.0))
        )
        geoList.append(
            Part.LineSegment(App.Vector(x1, y2, 0.0), App.Vector(x2, y2, 0.0))
        )
        geoList.append(
            Part.LineSegment(App.Vector(x2, y2, 0.0), App.Vector(x2, y1, 0.0))
        )
        geoList.append(
            Part.LineSegment(App.Vector(x2, y1, 0.0), App.Vector(x1, y1, 0.0))
        )
        doc.Sketch.addGeometry(geoList, False)
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
        doc.Sketch.addConstraint(constraintList)
        del constraintList
        body.addObject(doc.Sketch)

        pocket = self.Doc.addObject("PartDesign::Pocket", "Pocket")
        pocket.Refine = True
        pocket.Type = "Length"
        pocket.Length = 3
        pocket.Direction = App.Vector(-0.710000000, 0.7100000000, 0.0000000000)
        pocket.Profile = doc.Sketch
        # pocket.Reversed = False
        body.addObject(pocket)
        self.Doc.recompute()
        volume3 = body.Shape.Volume
        # Change the filleted edges, potentially triggering TNP
        fillet.Base = (
            box,
            [
                "Edge5",
                "Edge7",
            ],
        )
        self.Doc.recompute()
        volume4 = body.Shape.Volume
        # Assert
        if body.Shape.ElementMapVersion == "":  # Skip without element maps.
            return
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 62)
        self.assertEqual(len(reverseMap), 62)
        self.assertEqual(len(faces), 12)
        self.assertEqual(len(edges), 30)
        self.assertEqual(len(vertexes), 20)
        boxVolume = 10 * 10 * 10
        # Full prism minus the rounded triangle prism.
        filletVolume = 1 * 1 * 10 - 1 * 1 * math.pi / 4 * 10  # 0.5 * 10
        cutVolume = 24
        self.assertAlmostEqual(volume1, boxVolume)
        self.assertAlmostEqual(volume2, boxVolume - 3 * filletVolume)
        self.assertAlmostEqual(volume3, boxVolume - 3 * filletVolume - cutVolume, 4)
        self.assertAlmostEqual(volume4, boxVolume - 2 * filletVolume - cutVolume, 4)

    # TODO: ENABLE THIS TEST WHEN MULTISOLIDS AND TNP PLAY NICELY
    # def testPD_TNPSketchPadMultipleSolids(self):
    #     """ Prove that a sketch with multiple wires works correctly"""
    #     doc = App.ActiveDocument
    #     App.activeDocument().addObject('PartDesign::Body','Body')
    #     doc.Body.newObject('Sketcher::SketchObject','Sketch')
    #     doc.Sketch.AttachmentSupport = (doc.XY_Plane,[''])
    #     doc.Sketch.MapMode = 'FlatFace'
    #     radius = 15
    #     geoList = []
    #     geoList.append(Part.Circle(App.Vector(-20, 20, 0.000000), App.Vector(0.000000, 0.000000, 1.000000), radius))
    #     doc.Sketch.addGeometry(geoList,False)
    #     geoList = []
    #     geoList.append(Part.Circle(App.Vector(20, 20, 0.000000), App.Vector(0.000000, 0.000000, 1.000000), radius))
    #     doc.Sketch.addGeometry(geoList,False)
    #     geoList = []
    #     geoList.append(Part.Circle(App.Vector(20, -20, 0.000000), App.Vector(0.000000, 0.000000, 1.000000), radius))
    #     doc.Sketch.addGeometry(geoList,False)
    #     geoList = []
    #     geoList.append(Part.Circle(App.Vector(-20,-20, 0.000000), App.Vector(0.000000, 0.000000, 1.000000), radius))
    #     doc.Sketch.addGeometry(geoList,False)
    #     del geoList
    #     doc.recompute()
    #     doc.Body.newObject('PartDesign::Pad','Pad')
    #     doc.Pad.Profile = (doc.Sketch, ['',])
    #     doc.Pad.Length = 10
    #     doc.Pad.ReferenceAxis = (doc.Sketch,['N_Axis'])
    #     doc.Sketch.Visibility = False
    #     doc.recompute()
    #     expected_volume = math.pi * radius * radius * 10 * 4 # Volume of 4 padded circles
    #     self.assertAlmostEqual(doc.Body.Shape.Volume, expected_volume )
    #   # Add additional code to attach another sketch, then change the original sketch and check TNP

    def testPD_TNPSketchPadTouching(self):
        """Prove that a sketch with touching wires works correctly"""
        doc = App.ActiveDocument
        App.activeDocument().addObject("PartDesign::Body", "Body")
        doc.Body.newObject("Sketcher::SketchObject", "Sketch")
        doc.Sketch.AttachmentSupport = (doc.XY_Plane, [""])
        doc.Sketch.MapMode = "FlatFace"
        radius = 20
        geoList = []
        geoList.append(
            Part.Circle(
                App.Vector(-20, 20, 0.000000),
                App.Vector(0.000000, 0.000000, 1.000000),
                radius,
            )
        )
        doc.Sketch.addGeometry(geoList, False)
        geoList = []
        geoList.append(
            Part.Circle(
                App.Vector(20, 20, 0.000000),
                App.Vector(0.000000, 0.000000, 1.000000),
                radius,
            )
        )
        doc.Sketch.addGeometry(geoList, False)
        geoList = []
        geoList.append(
            Part.Circle(
                App.Vector(20, -20, 0.000000),
                App.Vector(0.000000, 0.000000, 1.000000),
                radius,
            )
        )
        doc.Sketch.addGeometry(geoList, False)
        geoList = []
        geoList.append(
            Part.Circle(
                App.Vector(-20, -20, 0.000000),
                App.Vector(0.000000, 0.000000, 1.000000),
                radius,
            )
        )
        doc.Sketch.addGeometry(geoList, False)
        del geoList
        doc.recompute()
        doc.Body.newObject("PartDesign::Pad", "Pad")
        doc.Pad.Profile = (
            doc.Sketch,
            [
                "",
            ],
        )
        doc.Pad.Length = 10
        doc.Pad.ReferenceAxis = (doc.Sketch, ["N_Axis"])
        doc.Sketch.Visibility = False
        doc.recompute()
        expected_volume = (
            math.pi * radius * radius * 10 * 4
        )  # Volume of 4 padded circles
        # self.assertAlmostEqual(doc.Body.Shape.Volume, expected_volume ) # TODO ENABLE THIS ASSERTION WHEN IT PASSES

    #   # Add additional code to attach another sketch, then change the original sketch and check TNP

    # TODO ENABLE THIS TEST IF CODE IS WRITTEN TO SUPPORT SKETCHES WITH OVERLAPS.
    # def testPD_TNPSketchPadOverlapping(self):
    #     """ Prove that a sketch with overlapping wires works correctly"""
    #     doc = App.ActiveDocument
    #     App.activeDocument().addObject('PartDesign::Body','Body')
    #     doc.Body.newObject('Sketcher::SketchObject','Sketch')
    #     doc.Sketch.AttachmentSupport = (doc.XY_Plane,[''])
    #     doc.Sketch.MapMode = 'FlatFace'
    #     radius = 25
    #     geoList = []
    #     geoList.append(Part.Circle(App.Vector(-20, 20, 0.000000), App.Vector(0.000000, 0.000000, 1.000000), radius))
    #     doc.Sketch.addGeometry(geoList,False)
    #     geoList = []
    #     geoList.append(Part.Circle(App.Vector(20, 20, 0.000000), App.Vector(0.000000, 0.000000, 1.000000), radius))
    #     doc.Sketch.addGeometry(geoList,False)
    #     geoList = []
    #     geoList.append(Part.Circle(App.Vector(20, -20, 0.000000), App.Vector(0.000000, 0.000000, 1.000000), radius))
    #     doc.Sketch.addGeometry(geoList,False)
    #     geoList = []
    #     geoList.append(Part.Circle(App.Vector(-20,-20, 0.000000), App.Vector(0.000000, 0.000000, 1.000000), radius))
    #     doc.Sketch.addGeometry(geoList,False)
    #     del geoList
    #     doc.recompute()
    #     doc.Body.newObject('PartDesign::Pad','Pad')
    #     doc.Pad.Profile = (doc.Sketch, ['',])
    #     doc.Pad.Length = 10
    #     doc.Pad.ReferenceAxis = (doc.Sketch,['N_Axis'])
    #     doc.Sketch.Visibility = False
    #     doc.recompute()
    #     expected_volume = math.pi * radius * radius * 10 * 4 # Volume of 4 padded circles
    #     length = 12 # FIXME arbitrary guess, figure out the right value for either this or the angle.
    #     angle = 2 * math.asin(length/2*radius)
    #     expected_volume = expected_volume - 1/2 * radius * radius * (angle-math.sin(angle))  # Volume of the overlap areas
    #     self.assertAlmostEqual(doc.Body.Shape.Volume, expected_volume )
    #   # Add additional code to attach another sketch, then change the original sketch and check TNP

    def testPD_TNPSketchPadSketchMove(self):
        """Prove that a sketch attached to a padded sketch shape does not have a problem when the initial sketch has geometry move"""
        doc = App.ActiveDocument
        App.activeDocument().addObject("PartDesign::Body", "Body")
        doc.Body.newObject("Sketcher::SketchObject", "Sketch")
        doc.Sketch.AttachmentSupport = (doc.XY_Plane, [""])
        doc.Sketch.MapMode = "FlatFace"
        geoList = []
        geoList.append(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(40, 0, 0)))
        geoList.append(Part.LineSegment(App.Vector(40, 0, 0), App.Vector(40, 20, 0)))
        geoList.append(Part.LineSegment(App.Vector(40, 20, 0), App.Vector(0, 20, 0)))
        geoList.append(Part.LineSegment(App.Vector(0, 20, 0), App.Vector(0, 0, 0)))
        doc.Sketch.addGeometry(geoList, False)
        constraintList = []
        constraintList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        constraintList.append(Sketcher.Constraint("Horizontal", 0))
        constraintList.append(Sketcher.Constraint("Horizontal", 2))
        constraintList.append(Sketcher.Constraint("Vertical", 1))
        constraintList.append(Sketcher.Constraint("Vertical", 3))
        doc.Sketch.addConstraint(constraintList)
        doc.recompute()
        doc.Body.newObject("PartDesign::Pad", "Pad")
        doc.Pad.Profile = (
            doc.Sketch,
            [
                "",
            ],
        )
        doc.Pad.Length = 10
        doc.Pad.ReferenceAxis = (doc.Sketch, ["N_Axis"])
        doc.Sketch.Visibility = False
        doc.Pad.Length = 10.000000
        doc.Pad.TaperAngle = 0.000000
        doc.Pad.UseCustomVector = 0
        doc.Pad.Direction = (0, 0, 1)
        doc.Pad.ReferenceAxis = (doc.Sketch, ["N_Axis"])
        doc.Pad.AlongSketchNormal = 1
        doc.Pad.Type = 0
        doc.Pad.UpToFace = None
        doc.Pad.Reversed = 0
        doc.Pad.Midplane = 0
        doc.Pad.Offset = 0
        doc.recompute()
        doc.Sketch.Visibility = False
        doc.Body.newObject("Sketcher::SketchObject", "Sketch001")
        doc.Sketch001.AttachmentSupport = (
            doc.Pad,
            [
                "Face6",
            ],
        )
        doc.Sketch001.MapMode = "FlatFace"
        geoList = []
        geoList.append(Part.LineSegment(App.Vector(5, 5, 0), App.Vector(5, 10, 0)))
        geoList.append(Part.LineSegment(App.Vector(5, 10, 0), App.Vector(25, 10, 0)))
        geoList.append(Part.LineSegment(App.Vector(25, 10, 0), App.Vector(25, 5, 0)))
        geoList.append(Part.LineSegment(App.Vector(25, 5, 0), App.Vector(5, 5, 0)))
        doc.Sketch001.addGeometry(geoList, False)
        del geoList
        constraintList = []
        constraintList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        constraintList.append(Sketcher.Constraint("Vertical", 0))
        constraintList.append(Sketcher.Constraint("Vertical", 2))
        constraintList.append(Sketcher.Constraint("Horizontal", 1))
        constraintList.append(Sketcher.Constraint("Horizontal", 3))
        doc.Sketch001.addConstraint(constraintList)
        doc.recompute()
        doc.Body.newObject("PartDesign::Pad", "Pad001")
        doc.Pad001.Profile = (
            doc.Sketch001,
            [
                "",
            ],
        )
        doc.Pad001.Length = 10
        doc.Pad001.ReferenceAxis = (doc.Sketch001, ["N_Axis"])
        doc.Sketch001.Visibility = False
        doc.Pad001.Length = 10.000000
        doc.Pad001.TaperAngle = 0.000000
        doc.Pad001.UseCustomVector = 0
        doc.Pad001.Direction = (0, 0, 1)
        doc.Pad001.ReferenceAxis = (doc.Sketch001, ["N_Axis"])
        doc.Pad001.AlongSketchNormal = 1
        doc.Pad001.Type = 0
        doc.Pad001.UpToFace = None
        doc.Pad001.Reversed = 0
        doc.Pad001.Midplane = 0
        doc.Pad001.Offset = 0
        doc.recompute()
        doc.Pad.Visibility = False
        doc.Sketch001.Visibility = False
        doc.Sketch.movePoint(3, 0, App.Vector(-5, 0, 0), 1)
        doc.Sketch.movePoint(0, 0, App.Vector(0.000000, -5, 0), 1)
        doc.Sketch.movePoint(1, 0, App.Vector(-5, 0.000000, 0), 1)
        doc.Sketch.movePoint(2, 0, App.Vector(-0, -5, 0), 1)
        doc.recompute()
        # If Sketch001 is still at the right start point, we are good.
        self.assertTrue(doc.Sketch001.AttachmentOffset.Matrix == App.Matrix())
        matrix1 = App.Matrix()
        matrix1.A34 = 10  # Z offset by 10.
        self.assertTrue(doc.Sketch001.Placement.Matrix == matrix1)

    def testPD_TNPSketchPadSketchDelete(self):
        """Prove that a sketch attached to a padded sketch shape does not have a problem when the initial sketch has geometry deleted"""
        doc = App.ActiveDocument
        App.activeDocument().addObject("PartDesign::Body", "Body")
        doc.Body.newObject("Sketcher::SketchObject", "Sketch")
        doc.Sketch.AttachmentSupport = (doc.XY_Plane, [""])
        doc.Sketch.MapMode = "FlatFace"
        import ProfileLib.RegularPolygon

        ProfileLib.RegularPolygon.makeRegularPolygon(
            doc.Sketch,
            6,
            App.Vector(0.000000, 0.000000, 0),
            App.Vector(24, 12, 0),
            False,
        )
        doc.Sketch.addConstraint(Sketcher.Constraint("Coincident", 6, 3, -1, 1))
        doc.Sketch.addConstraint(Sketcher.Constraint("PointOnObject", 0, 2, -2))
        doc.recompute()
        doc.Body.newObject("PartDesign::Pad", "Pad")
        doc.Pad.Profile = (
            doc.Sketch,
            [
                "",
            ],
        )
        doc.Pad.Length = 10
        doc.Pad.ReferenceAxis = (doc.Sketch, ["N_Axis"])
        doc.Sketch.Visibility = False
        doc.Pad.Length = 10.000000
        doc.Pad.TaperAngle = 0.000000
        doc.Pad.UseCustomVector = 0
        doc.Pad.Direction = (0, 0, 1)
        doc.Pad.ReferenceAxis = (doc.Sketch, ["N_Axis"])
        doc.Pad.AlongSketchNormal = 1
        doc.Pad.Type = 0
        doc.Pad.UpToFace = None
        doc.Pad.Reversed = 0
        doc.Pad.Midplane = 0
        doc.Pad.Offset = 0
        doc.recompute()
        doc.Sketch.Visibility = False
        doc.Body.newObject("Sketcher::SketchObject", "Sketch001")
        doc.Sketch001.AttachmentSupport = (
            doc.Pad,
            [
                "Face8",
            ],
        )
        doc.Sketch001.MapMode = "FlatFace"
        geoList = []
        geoList.append(
            Part.LineSegment(App.Vector(-5, 5, 0.000000), App.Vector(-5, -5, 0.000000))
        )
        geoList.append(
            Part.LineSegment(App.Vector(-5, -5, 0.000000), App.Vector(5, -5, 0.000000))
        )
        geoList.append(
            Part.LineSegment(App.Vector(5, -5, 0.000000), App.Vector(5, 5, 0.000000))
        )
        geoList.append(
            Part.LineSegment(App.Vector(5, 5, 0.000000), App.Vector(-5, 5, 0.000000))
        )
        doc.Sketch001.addGeometry(geoList, False)
        del geoList
        constraintList = []
        constraintList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        constraintList.append(Sketcher.Constraint("Vertical", 0))
        constraintList.append(Sketcher.Constraint("Vertical", 2))
        constraintList.append(Sketcher.Constraint("Horizontal", 1))
        constraintList.append(Sketcher.Constraint("Horizontal", 3))
        doc.Sketch001.addConstraint(constraintList)
        constraintList = []
        doc.recompute()
        doc.Body.newObject("PartDesign::Pad", "Pad001")
        doc.Pad001.Profile = (
            doc.Sketch001,
            [
                "",
            ],
        )
        doc.Pad001.Length = 10
        doc.Pad001.ReferenceAxis = (doc.Sketch001, ["N_Axis"])
        doc.Sketch001.Visibility = False
        doc.Pad001.Length = 10.000000
        doc.Pad001.TaperAngle = 0.000000
        doc.Pad001.UseCustomVector = 0
        doc.Pad001.Direction = (0, 0, 1)
        doc.Pad001.ReferenceAxis = (doc.Sketch001, ["N_Axis"])
        doc.Pad001.AlongSketchNormal = 1
        doc.Pad001.Type = 0
        doc.Pad001.UpToFace = None
        doc.Pad001.Reversed = 0
        doc.Pad001.Midplane = 0
        doc.Pad001.Offset = 0
        doc.recompute()
        doc.Pad.Visibility = False
        doc.Sketch001.Visibility = False
        doc.Sketch.delGeometries([4])
        doc.Sketch.addConstraint(Sketcher.Constraint("Coincident", 3, 2, 4, 1))
        doc.Sketch.delConstraint(12)
        doc.recompute()
        # If Sketch001 is still at the right start point, we are good.
        self.assertTrue(doc.Sketch001.AttachmentOffset.Matrix == App.Matrix())
        matrix1 = App.Matrix()
        matrix1.A34 = 10  # Z offset by 10
        self.assertTrue(doc.Sketch001.Placement.Matrix == matrix1)

    def testPD_TNPSketchPadSketchConstructionChange(self):
        """Prove that a sketch attached to a padded sketch shape does not have a problem when the initial sketch has geometry changed from Construction"""
        pass  # TODO

    def testPD_TNPSketchPadSketchTrim(self):
        """Prove that a sketch attached to a padded sketch shape does not have a problem when the initial sketch has geometry trimmed"""
        pass  # TODO

    def testPD_TNPSketchPadSketchExternal(self):
        """Prove that a sketch attached to a padded sketch shape does not have a problem when the initial sketch has external geometry changed"""
        pass  # TODO

    def testPD_TNPSketchPadSketchTransform(self):
        """Prove that a sketch attached to a padded sketch shape does not have a problem when the initial sketch has a transformation applied"""
        pass  # TODO

    def testPD_TNPSketchPadSketchSymmetry(self):
        """Prove that a sketch attached to a padded sketch shape does not have a problem when the initial sketch has Symmetry applied"""
        pass  # TODO

    def testPD_TNPSketchPadSketchBSpline(self):
        """Prove that a sketch attached to a padded sketch shape does not have a problem when the initial sketch has BSpline changed"""
        pass  # TODO

    def testPD_TNPSketchRotSketchMove(self):
        """Prove that a sketch attached to a rotated sketch shape does not have a problem when the initial sketch has geometry moved"""
        pass  # TODO

    def testPD_TNPSketchPocketSketchMove(self):
        """Prove that a sketch attached to a pocketed sketch shape does not have a problem when the initial sketch has geometry moved"""
        pass  # TODO

    def testPD_TNPSketchLoftSketchMove(self):
        """Prove that a sketch attached to a lofted sketch shape does not have a problem when the initial sketch has geometry moved"""
        pass  # TODO

    def testPD_TNPSketchPipeSketchMove(self):
        """Prove that a sketch attached to a piped sketch shape does not have a problem when the initial sketch has geometry moved"""
        pass  # TODO

    def testSubelementNames(self):
        # Arrange
        doc = App.ActiveDocument
        plane = doc.addObject("Part::Plane", "Plane")
        plane.Length = 10
        plane.Width = 10
        extrude = doc.addObject("Part::Extrusion", "Extrude")
        extrude.Base = plane
        extrude.LengthFwd = 10
        doc.recompute()
        if not App.GuiUp:
            return
        # Act
        App.Gui.Selection.addSelection("", extrude.Name, "Face2")
        # Assert
        self.assertEqual(
            len(App.Gui.Selection.getSelectionEx("", 0)[0].SubElementNames), 1
        )
        if extrude.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            self.assertEqual(
                App.Gui.Selection.getSelectionEx("", 0)[0].SubElementNames[0], "Face2"
            )
        else:
            self.assertEqual(
                App.Gui.Selection.getSelectionEx("", 0)[0].SubElementNames[0][-8:],
                ",F.Face2",
            )

    def testGetElementFunctionality(self):
        # Arrange
        body = self.Doc.addObject("PartDesign::Body", "Body")
        padSketch = self.Doc.addObject("Sketcher::SketchObject", "SketchPad")
        pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        body.addObject(padSketch)
        body.addObject(pad)
        TestSketcherApp.CreateRectangleSketch(padSketch, (0, 0), (1, 1))
        pad.Profile = padSketch
        pad.Length = 1
        # Act
        self.Doc.recompute()
        if pad.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        map = pad.Shape.ElementMap
        # Assert
        self.assertGreater(pad.Shape.ElementMapSize, 0)
        for tnpName in map.keys():
            element1 = pad.Shape.getElement(tnpName)
            element2 = pad.Shape.getElement(map[tnpName])
            self.assertTrue(element1.isSame(element2))

    def testFileSaveRestore(self):
        # Arrange
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.create_t_sketch()
        self.assertEqual(self.Doc.Sketch.Shape.ElementMapSize, 18)
        filename = self.Doc.Name
        # Act
        self.Doc.saveAs(filename)
        App.closeDocument(filename)
        self.Doc = App.openDocument(filename + ".FCStd")
        self.Doc.recompute()
        # Assert
        self.assertEqual(self.Doc.Sketch.Shape.ElementMapSize, 18)

    def testBodySubShapeBinderElementMap(self):
        # Arrange
        doc = App.ActiveDocument
        doc.addObject("Part::Box", "Box")
        doc.ActiveObject.Label = "Cube"
        doc.addObject("Part::Box", "Box")
        doc.ActiveObject.Label = "Cube"
        doc.addObject("Part::MultiFuse", "Fusion")
        doc.Fusion.Refine = False
        doc.Fusion.Shapes = [
            doc.Box,
            doc.Box001,
        ]
        doc.recompute()
        self.assertEqual(doc.Fusion.Shape.ElementMapSize, 26)

        doc.addObject("PartDesign::Body", "Body")
        doc.Body.Label = "Body"

        doc.addObject("PartDesign::Body", "Body001")
        doc.Body001.Label = "Body001"

        # act
        # Set up the subshapebinder version
        binder = doc.Body.newObject("PartDesign::SubShapeBinder", "Binder")
        binder.Support = [(doc.Fusion, (""))]
        doc.recompute()

        # Set up the base feature version
        doc.Body001.BaseFeature = App.activeDocument().Fusion
        doc.recompute()

        # assert
        self.assertEqual(
            doc.Body.OutList[1].Shape.ElementMapSize, 26
        )  # subobjects ( subshapebinder here ) should have elementmap
        self.assertEqual(
            doc.Body.Shape.ElementMapSize, 0
        )  # TODO:  This is Sus, although LS3 passes.  Might be because
        # SubShapeBinder is different in LS3.
        self.assertEqual(
            doc.Body001.BaseFeature.Shape.ElementMapSize, 26
        )  # base feature lookup should have element map
        self.assertEqual(
            doc.Body001.Shape.ElementMapSize, 26
        )  # Body Shape should have element map

    def testBaseFeatureAttachmentSupport(self):
        # Arrange
        doc = App.ActiveDocument
        doc.addObject("Part::Box", "Box")
        doc.ActiveObject.Label = "Cube"
        doc.recompute()
        doc.addObject("Part::Box", "Box")
        doc.ActiveObject.Label = "Cube"
        doc.Box001.Placement = App.Placement(
            App.Vector(5.00, 5.00, 5.00),
            App.Rotation(App.Vector(0.00, 0.00, 1.00), 0.00),
        )
        doc.recompute()

        doc.addObject("Part::MultiFuse", "Fusion")
        doc.Fusion.Refine = False
        doc.Fusion.Shapes = [
            doc.Box,
            doc.Box001,
        ]

        doc.recompute()
        # doc.Box.Visibility = False
        # doc.Box001.Visibility = False
        # doc.recompute()

        doc.addObject("PartDesign::Body", "Body")
        doc.Body.Label = "Body"
        doc.Body.BaseFeature = App.activeDocument().Fusion
        doc.recompute()

        doc.Body.newObject("Sketcher::SketchObject", "Sketch")
        doc.Sketch.AttachmentSupport = (doc.getObject("BaseFeature"), ("Face8"))
        doc.Sketch.MapMode = "FlatFace"
        doc.recompute()
        geoList = []
        geoList.append(
            Part.LineSegment(
                App.Vector(12.0, 13.0, 0.000000), App.Vector(12.0, 11.0, 0.000000)
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(12.0, 11, 0.000000), App.Vector(14.0, 11.0, 0.000000)
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(14.0, 11, 0.000000), App.Vector(14.0, 13.0, 0.000000)
            )
        )
        geoList.append(
            Part.LineSegment(
                App.Vector(14.0, 13.0, 0.000000), App.Vector(12, 13.0, 0.000000)
            )
        )
        doc.Sketch.addGeometry(geoList, False)
        del geoList
        constraintList = []
        constraintList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        constraintList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        constraintList.append(Sketcher.Constraint("Vertical", 0))
        constraintList.append(Sketcher.Constraint("Vertical", 2))
        constraintList.append(Sketcher.Constraint("Horizontal", 1))
        constraintList.append(Sketcher.Constraint("Horizontal", 3))
        doc.Sketch.addConstraint(constraintList)
        del constraintList
        constraintList = []
        doc.recompute()

        # Assert that we have a sketch element map before proceeding
        self.assertEqual(doc.Sketch.Shape.ElementMapSize, 12)

        # Arrange
        doc.Body.newObject("PartDesign::Pad", "Pad")
        doc.Pad.Profile = (
            doc.Sketch,
            [
                "",
            ],
        )
        doc.Pad.Length = 10
        doc.recompute()
        doc.Pad.ReferenceAxis = (doc.Sketch, ["N_Axis"])
        doc.Sketch.Visibility = False
        doc.Pad.Length = 10.000000
        doc.Pad.TaperAngle = 0.000000
        doc.Pad.UseCustomVector = 0
        doc.Pad.Direction = (0, -1, 0)
        doc.Pad.ReferenceAxis = (doc.Sketch, ["N_Axis"])
        doc.Pad.AlongSketchNormal = 1
        doc.Pad.Type = 0
        doc.Pad.UpToFace = None
        doc.Pad.Reversed = 0
        doc.Pad.Midplane = 0
        doc.Pad.Offset = 0
        doc.BaseFeature.Visibility = False
        doc.Sketch.Visibility = False
        doc.recompute()

        # Act
        doc.Box001.Width = "3.00 mm"
        doc.Box001.Placement = App.Placement(
            App.Vector(5.00, 5.00, 5.00),
            App.Rotation(App.Vector(0.00, 0.00, 1.00), 0.00),
        )
        doc.recompute()
        # Assert
        self.assertEqual(
            len(doc.Body.Shape.Faces), 17
        )  # Check that the object seems right.
        self.assertEqual(len(doc.Body.Shape.Edges), 42)
        self.assertEqual(len(doc.Body.Shape.Vertexes), 28)
        self.assertEqual(len(doc.Body.Shape.Shells), 1)
        self.assertEqual(len(doc.Body.Shape.Solids), 1)
        self.assertEqual(
            doc.Sketch.AttachmentSupport[0][1][0], "Face9"
        )  # Attachment autochanged from Face8.
        # potentially check the .BoundBox ( calc seems off on this, Not applying sketch position to Pad object )

    def create_t_sketch(self):
        self.Doc.getObject("Body").newObject("Sketcher::SketchObject", "Sketch")
        geo_list = [
            Part.LineSegment(App.Vector(0, 0, 0), App.Vector(20, 0, 0)),
            Part.LineSegment(App.Vector(20, 0, 0), App.Vector(20, 10, 0)),
            Part.LineSegment(App.Vector(20, 10, 0), App.Vector(10, 10, 0)),
            Part.LineSegment(App.Vector(10, 10, 0), App.Vector(10, 20, 0)),
            Part.LineSegment(App.Vector(10, 20, 0), App.Vector(0, 20, 0)),
            Part.LineSegment(App.Vector(0, 20, 0), App.Vector(0, 0, 0)),
        ]
        self.Doc.getObject("Sketch").addGeometry(geo_list, False)
        con_list = [
            Sketcher.Constraint("Coincident", 0, 2, 1, 1),
            Sketcher.Constraint("Coincident", 1, 2, 2, 1),
            Sketcher.Constraint("Coincident", 2, 2, 3, 1),
            Sketcher.Constraint("Coincident", 3, 2, 4, 1),
            Sketcher.Constraint("Coincident", 4, 2, 5, 1),
            Sketcher.Constraint("Coincident", 5, 2, 0, 1),
            Sketcher.Constraint("Horizontal", 0),
            Sketcher.Constraint("Horizontal", 2),
            Sketcher.Constraint("Horizontal", 4),
            Sketcher.Constraint("Vertical", 1),
            Sketcher.Constraint("Vertical", 3),
            Sketcher.Constraint("Vertical", 5),
        ]
        self.Doc.getObject("Sketch").addConstraint(con_list)
        del geo_list, con_list
        self.Doc.recompute()

    def testRectanglewithArcChangeinGlobalCenter(self):
        # Arrange
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (-50, -25), (100, 50))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 10
        self.Doc.recompute()

        self.Sketch001 = self.Body.newObject("Sketcher::SketchObject", "Sketch001")
        self.Sketch001.AttachmentSupport = (
            self.Doc.getObject("Pad"),
            [
                "Face6",
            ],
        )
        self.Sketch001.MapMode = "FlatFace"

        TestSketcherApp.CreateRectangleSketch(self.Sketch001, (-40, -20), (80, 40))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.Sketch001
        self.Pad1.Length = 10
        self.Doc.recompute()

        self.geoList = []
        self.geoList.append(
            Part.ArcOfCircle(
                Part.Circle(
                    App.Vector(0.000000, -109.419670, 0.000000),
                    App.Vector(0.000000, 0.000000, 1.000000),
                    88.713871,
                ),
                1.258384,
                1.886112,
            )
        )
        self.PadSketch.addGeometry(self.geoList, False)
        del self.geoList

        self.constraintList = []
        self.constraintList.append(Sketcher.Constraint("PointOnObject", 4, 3, -2))
        self.constraintList.append(Sketcher.Constraint("PointOnObject", 4, 1, 2))
        self.constraintList.append(Sketcher.Constraint("PointOnObject", 4, 2, 2))
        self.PadSketch.addConstraint(self.constraintList)
        del self.constraintList

        self.PadSketch.trim(2, App.Vector(7.337847, -25.000000, 0))
        self.PadSketch.addConstraint(Sketcher.Constraint("Equal", 3, 1))
        self.PadSketch.addConstraint(Sketcher.Constraint("Horizontal", 5))
        self.PadSketch.addConstraint(Sketcher.Constraint("Radius", 4, 73.031111))
        self.PadSketch.setDatum(18, App.Units.Quantity("70.000000 mm"))
        self.PadSketch.addConstraint(
            Sketcher.Constraint("DistanceY", 4, 3, -1, 1, 88.867210)
        )
        self.PadSketch.setDatum(19, App.Units.Quantity("80.000000 mm"))

        self.Doc.recompute()
        self.assertTrue(self.Sketch001.isValid())

    def testRectanglewithArcChangeinGlobalUpperRight(self):
        # Arrange
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        # Make first offset cube Pad
        self.PadSketch = self.Body.newObject("Sketcher::SketchObject", "Sketch")
        self.PadSketch.AttachmentSupport = (self.Doc.getObject("XY_Plane"), [""])
        self.PadSketch.MapMode = "FlatFace"
        self.Doc.recompute()

        lastGeoId = len(self.PadSketch.Geometry)

        self.geoList = []
        self.geoList.append(
            Part.LineSegment(
                App.Vector(6.565019, 5.821458, 0.000000),
                App.Vector(131.750198, 5.821458, 0.000000),
            )
        )
        self.geoList.append(
            Part.LineSegment(
                App.Vector(131.750198, 5.821458, 0.000000),
                App.Vector(131.750198, 75.265900, 0.000000),
            )
        )
        self.geoList.append(
            Part.LineSegment(
                App.Vector(131.750198, 75.265900, 0.000000),
                App.Vector(6.565019, 75.265900, 0.000000),
            )
        )
        self.geoList.append(
            Part.LineSegment(
                App.Vector(6.565019, 75.265900, 0.000000),
                App.Vector(6.565019, 5.821458, 0.000000),
            )
        )
        self.PadSketch.addGeometry(self.geoList, False)
        del self.geoList
        self.constraintList = []
        self.constraintList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        self.constraintList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        self.constraintList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        self.constraintList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        self.constraintList.append(Sketcher.Constraint("Horizontal", 0))
        self.constraintList.append(Sketcher.Constraint("Horizontal", 2))
        self.constraintList.append(Sketcher.Constraint("Vertical", 1))
        self.constraintList.append(Sketcher.Constraint("Vertical", 3))
        self.PadSketch.addConstraint(self.constraintList)
        del self.constraintList

        self.Doc.recompute()

        self.Pad = self.Body.newObject("PartDesign::Pad", "Pad")
        self.Pad.Profile = (
            self.Doc.getObject("Sketch"),
            [
                "",
            ],
        )
        self.Pad.Length = 10
        self.Doc.recompute()
        self.Pad.ReferenceAxis = (self.Doc.getObject("Sketch"), ["N_Axis"])
        self.Doc.recompute()

        self.Pad.Length = 10.000000
        self.Pad.TaperAngle = 0.000000
        self.Pad.UseCustomVector = 0
        self.Pad.Direction = (0, 0, 1)
        self.Pad.ReferenceAxis = (self.Doc.getObject("Sketch"), ["N_Axis"])
        self.Pad.AlongSketchNormal = 1
        self.Pad.Type = 0
        self.Pad.UpToFace = None
        self.Pad.Reversed = 0
        self.Pad.Midplane = 0
        self.Pad.Offset = 0
        self.Doc.recompute()

        self.Sketch001 = self.Body.newObject("Sketcher::SketchObject", "Sketch001")
        self.Sketch001.AttachmentSupport = (
            self.Doc.getObject("Pad"),
            [
                "Face6",
            ],
        )
        self.Sketch001.MapMode = "FlatFace"
        self.Doc.recompute()

        lastGeoId = len(self.Sketch001.Geometry)

        self.geoList = []
        self.geoList.append(
            Part.LineSegment(
                App.Vector(33.048996, 24.872660, 0.000000),
                App.Vector(125.086029, 24.872660, 0.000000),
            )
        )
        self.geoList.append(
            Part.LineSegment(
                App.Vector(125.086029, 24.872660, 0.000000),
                App.Vector(125.086029, 72.835625, 0.000000),
            )
        )
        self.geoList.append(
            Part.LineSegment(
                App.Vector(125.086029, 72.835625, 0.000000),
                App.Vector(33.048996, 72.835625, 0.000000),
            )
        )
        self.geoList.append(
            Part.LineSegment(
                App.Vector(33.048996, 72.835625, 0.000000),
                App.Vector(33.048996, 24.872660, 0.000000),
            )
        )
        self.Sketch001.addGeometry(self.geoList, False)
        del self.geoList

        self.constraintList = []
        self.constraintList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        self.constraintList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        self.constraintList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        self.constraintList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        self.constraintList.append(Sketcher.Constraint("Horizontal", 0))
        self.constraintList.append(Sketcher.Constraint("Horizontal", 2))
        self.constraintList.append(Sketcher.Constraint("Vertical", 1))
        self.constraintList.append(Sketcher.Constraint("Vertical", 3))
        self.Sketch001.addConstraint(self.constraintList)
        del self.constraintList

        self.Doc.recompute()

        self.Pad1 = self.Body.newObject("PartDesign::Pad", "Pad001")
        self.Pad1.Profile = (
            self.Doc.getObject("Sketch001"),
            [
                "",
            ],
        )
        self.Pad1.Length = 10
        self.Doc.recompute()
        self.Pad1.ReferenceAxis = (self.Doc.getObject("Sketch001"), ["N_Axis"])

        self.Doc.recompute()

        self.Pad1.Length = 10.000000
        self.Pad1.TaperAngle = 0.000000
        self.Pad1.UseCustomVector = 0
        self.Pad1.Direction = (0, 0, 1)
        self.Pad1.ReferenceAxis = (self.Doc.getObject("Sketch001"), ["N_Axis"])
        self.Pad1.AlongSketchNormal = 1
        self.Pad1.Type = 0
        self.Pad1.UpToFace = None
        self.Pad1.Reversed = 0
        self.Pad1.Midplane = 0
        self.Pad1.Offset = 0
        self.Doc.recompute()

        lastGeoId = len(self.Sketch001.Geometry)

        self.geoList = []
        self.geoList.append(
            Part.ArcOfCircle(
                Part.Circle(
                    App.Vector(73.611900, -60.502949, 0.000000),
                    App.Vector(0.000000, 0.000000, 1.000000),
                    80.806032,
                ),
                0.997061,
                2.178808,
            )
        )
        self.PadSketch.addGeometry(self.geoList, False)
        del self.geoList

        self.constraintList = []
        self.constraintList.append(Sketcher.Constraint("PointOnObject", 4, 2, 0))
        self.constraintList.append(Sketcher.Constraint("PointOnObject", 4, 1, 0))
        self.PadSketch.addConstraint(self.constraintList)
        del self.constraintList
        self.Doc.recompute()
        self.PadSketch.trim(0, App.Vector(69.157609, 5.865876, 0))
        self.Doc.recompute()
        self.assertTrue(self.Sketch001.isValid())

    def testPadChange_UpToFirst_to_Dimension(self):
        # Arrange
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (-42.5, -42.5), (85, 85))
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (-37.5, -37.5), (75, 75))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 10
        self.Doc.recompute()

        self.Sketch001 = self.Body.newObject("Sketcher::SketchObject", "Sketch001")
        self.Sketch001.AttachmentSupport = (
            self.Doc.getObject("Pad"),
            [
                "Face5",
            ],
        )
        self.Sketch001.MapMode = "FlatFace"
        self.Doc.recompute()
        lastGeoId = len(self.Sketch001.Geometry)

        self.geoList = []
        self.geoList.append(
            Part.LineSegment(
                App.Vector(-33.953453, 7.680901, 0.000000),
                App.Vector(-33.953453, 2.543239, 0.000000),
            )
        )
        self.geoList.append(
            Part.LineSegment(
                App.Vector(-33.953453, 2.543239, 0.000000),
                App.Vector(33.282925, 2.543239, 0.000000),
            )
        )
        self.geoList.append(
            Part.LineSegment(
                App.Vector(33.282925, 2.543239, 0.000000),
                App.Vector(33.282925, 7.680901, 0.000000),
            )
        )
        self.geoList.append(
            Part.LineSegment(
                App.Vector(33.282925, 7.680901, 0.000000),
                App.Vector(-33.953453, 7.680901, 0.000000),
            )
        )
        self.Sketch001.addGeometry(self.geoList, False)
        del self.geoList

        self.constraintList = []
        self.constraintList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        self.constraintList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        self.constraintList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        self.constraintList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        self.constraintList.append(Sketcher.Constraint("Vertical", 0))
        self.constraintList.append(Sketcher.Constraint("Vertical", 2))
        self.constraintList.append(Sketcher.Constraint("Horizontal", 1))
        self.constraintList.append(Sketcher.Constraint("Horizontal", 3))
        self.Sketch001.addConstraint(self.constraintList)
        del self.constraintList

        self.Doc.recompute()
        self.Pad001 = self.Body.newObject("PartDesign::Pad", "Pad001")
        self.Pad001.Profile = (
            self.Doc.getObject("Sketch001"),
            [
                "",
            ],
        )
        self.Pad001.Length = 10
        self.Doc.recompute()
        self.Pad001.ReferenceAxis = (self.Doc.getObject("Sketch001"), ["N_Axis"])
        self.Doc.recompute()
        self.Pad001.UseCustomVector = 0
        self.Pad001.Direction = (0, -1, 0)
        self.Pad001.ReferenceAxis = (self.Doc.getObject("Sketch001"), ["N_Axis"])
        self.Pad001.AlongSketchNormal = 1
        self.Pad001.Type = 2
        self.Pad001.UpToFace = None
        self.Pad001.Reversed = 0
        self.Pad001.Midplane = 0
        self.Pad001.Offset = 0
        self.Doc.recompute()
        self.Sketch002 = self.Body.newObject("Sketcher::SketchObject", "Sketch002")
        self.Sketch002.AttachmentSupport = (
            self.Doc.getObject("Pad001"),
            [
                "Face11",
            ],
        )
        self.Sketch002.MapMode = "FlatFace"
        self.Doc.recompute()
        lastGeoId = len(self.Sketch002.Geometry)

        self.geoList = []
        self.geoList.append(
            Part.LineSegment(
                App.Vector(-30.826233, 35.070259, 0.000000),
                App.Vector(-30.826233, 30.602728, 0.000000),
            )
        )
        self.geoList.append(
            Part.LineSegment(
                App.Vector(-30.826233, 30.602728, 0.000000),
                App.Vector(30.602348, 30.602728, 0.000000),
            )
        )
        self.geoList.append(
            Part.LineSegment(
                App.Vector(30.602348, 30.602728, 0.000000),
                App.Vector(30.602348, 35.070259, 0.000000),
            )
        )
        self.geoList.append(
            Part.LineSegment(
                App.Vector(30.602348, 35.070259, 0.000000),
                App.Vector(-30.826233, 35.070259, 0.000000),
            )
        )
        self.Sketch002.addGeometry(self.geoList, False)
        del self.geoList

        self.constraintList = []
        self.constraintList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        self.constraintList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        self.constraintList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        self.constraintList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        self.constraintList.append(Sketcher.Constraint("Vertical", 0))
        self.constraintList.append(Sketcher.Constraint("Vertical", 2))
        self.constraintList.append(Sketcher.Constraint("Horizontal", 1))
        self.constraintList.append(Sketcher.Constraint("Horizontal", 3))
        self.Sketch002.addConstraint(self.constraintList)
        del self.constraintList

        self.Doc.recompute()

        self.Pad002 = self.Body.newObject("PartDesign::Pad", "Pad002")
        self.Pad002.Profile = (
            self.Doc.getObject("Sketch002"),
            [
                "",
            ],
        )
        self.Pad002.Length = 10
        self.Doc.recompute()
        self.Pad002.ReferenceAxis = (self.Doc.getObject("Sketch002"), ["N_Axis"])

        self.Doc.recompute()
        self.Pad002.Length = 10.000000
        self.Pad002.TaperAngle = 0.000000
        self.Pad002.UseCustomVector = 0
        self.Pad002.Direction = (0, 0, 1)
        self.Pad002.ReferenceAxis = (self.Doc.getObject("Sketch002"), ["N_Axis"])
        self.Pad002.AlongSketchNormal = 1
        self.Pad002.Type = 0
        self.Pad002.UpToFace = None
        self.Pad002.Reversed = 0
        self.Pad002.Midplane = 0
        self.Pad002.Offset = 0
        self.Doc.recompute()

        self.Pad001.Type = "Length"
        self.Pad001.Length = "4 mm"
        self.Doc.recompute()
        """Assumption: the warning <PropertyLinks> PropertyLinks.cpp(453):
         PartDesignTestTNP1#Sketch002.AttachmentSupport missing element
         reference PartDesignTestTNP1#Pad001 ;g815v1;SKT;:H976,V;:L#2;PSM;
        :H976:9,E;:L#8;PSM;:H976:9,F;:H-977,F.Face11 is only temporary and can be ignored."""
        self.assertTrue(self.Sketch002.AttachmentSupport[0][1][0] == "Face11")
        self.assertGreaterEqual(self.Body.Shape.Volume, 20126)

    def tearDown(self):
        """Clean up our test, optionally preserving the test document"""
        # This flag allows doing something like this:
        #   App.KeepTestDoc = True
        #   import TestApp
        #   TestApp.Test("TestPartDesignApp.TestTopologicalNamingProblem.testPadChange_UpToFirst_to_Dimension")
        # to leave the test document(s) around for further examination in an interactive setting.
        if hasattr(App, "KeepTestDoc") and App.KeepTestDoc:
            return
        App.closeDocument(self.Doc.Name)
