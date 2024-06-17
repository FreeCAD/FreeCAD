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
    """ Tests related to the Topological Naming Problem """

    # pylint: disable=attribute-defined-outside-init

    def setUp(self):
        """ Create a document for the test suite """
        self.Doc = App.newDocument("PartDesignTestTNP")

    def testPadsOnBaseObject(self):
        """ Simple TNP test case
            By creating three Pads dependent on each other in succession, and then moving the
            middle one we can determine if the last one breaks because of a broken reference
            to the middle one.  This is the essence of a TNP. Pretty much a duplicate of the
            steps at https://wiki.freecad.org/Topological_naming_problem """

        # Arrange
        self.Body = self.Doc.addObject('PartDesign::Body', 'Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Doc.recompute()

        # Attach a second pad to the top of the first.
        self.PadSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad1')
        self.Body.addObject(self.PadSketch1)
        self.PadSketch1.MapMode = 'FlatFace'
        self.PadSketch1.AttachmentSupport = [(self.Doc.getObject('Pad'), 'Face6')]
        TestSketcherApp.CreateRectangleSketch(self.PadSketch1, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.PadSketch1
        self.Pad1.Length = 1
        self.Doc.recompute()

        # Attach a third pad to the top of the second.
        self.PadSketch2 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad2')
        self.Body.addObject(self.PadSketch2)
        self.PadSketch2.MapMode = 'FlatFace'
        self.PadSketch2.AttachmentSupport = [(self.Doc.getObject('Pad1'), 'Face6')]
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
            App.Rotation(0.0000000000, 0.0000000000, 0.0000000000))
        self.Doc.recompute()

        # Assert everything is still valid.
        self.assertTrue(self.Pad.isValid())
        self.assertTrue(self.Pad1.isValid())

        if self.Body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            self.assertFalse(self.Pad2.isValid())   # TNP problem is present without ElementMaps
        else:
            self.assertTrue(self.Pad2.isValid())    # TNP problem is not present with ElementMaps

    def testPartDesignElementMapSketch(self):
        """ Test that creating a sketch results in a correct element map.  """
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        sketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        body.addObject(sketch)
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        # Act
        self.Doc.recompute()
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        reverseMap = sketch.Shape.ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        # Assert
        self.assertEqual(sketch.Shape.ElementMapSize,9)
        self.assertEqual(len(reverseMap),9)
        self.assertEqual(len(faces),1)
        self.assertEqual(len(edges),4)
        self.assertEqual(len(vertexes),4)

    def testPartDesignElementMapPad(self):
        """ Test that padding a sketch results in a correct element map.  Note that comprehensive testing
            of the geometric functionality of the Pad is in TestPad.py """
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        padSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
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
        self.assertEqual(pad.Shape.ElementMapSize,30)   # 4 duplicated Vertexes in here
        self.assertEqual(len(reverseMap),26)
        self.assertEqual(len(faces),6)
        self.assertEqual(len(edges),12)
        self.assertEqual(len(vertexes),8)

    def testPartDesignElementMapBox(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
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
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        cylinder = self.Doc.addObject('PartDesign::AdditiveCylinder', 'Cylinder')
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
        self.assertEqual(len(reverseMap),8)
        self.assertEqual(len(faces),3)
        self.assertEqual(len(edges),3)
        self.assertEqual(len(vertexes),2)

    def testPartDesignElementMapSphere(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        sphere = self.Doc.addObject('PartDesign::AdditiveSphere', 'Sphere')
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
        self.assertEqual(len(reverseMap),6)
        self.assertEqual(len(faces),1)
        self.assertEqual(len(edges),3)
        self.assertEqual(len(vertexes),2)

    def testPartDesignElementMapCone(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        cone = self.Doc.addObject('PartDesign::AdditiveCone', 'Cone')
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
        self.assertEqual(len(reverseMap),8)
        self.assertEqual(len(faces),3)
        self.assertEqual(len(edges),3)
        self.assertEqual(len(vertexes),2)

    def testPartDesignElementMapEllipsoid(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        ellipsoid = self.Doc.addObject('PartDesign::AdditiveEllipsoid', 'Ellipsoid')
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
        self.assertEqual(len(reverseMap),6)
        self.assertEqual(len(faces),1)
        self.assertEqual(len(edges),3)
        self.assertEqual(len(vertexes),2)

    def testPartDesignElementMapTorus(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        torus = self.Doc.addObject('PartDesign::AdditiveTorus', 'Torus')
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
        self.assertEqual(len(reverseMap),4)
        self.assertEqual(len(faces),1)
        self.assertEqual(len(edges),2)
        self.assertEqual(len(vertexes),1)

    def testPartDesignElementMapPrism(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        prism = self.Doc.addObject('PartDesign::AdditivePrism', 'Prism')
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
        self.assertEqual(len(reverseMap),38)
        self.assertEqual(len(faces),8)
        self.assertEqual(len(edges),18)
        self.assertEqual(len(vertexes),12)

    def testPartDesignElementMapWedge(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        wedge = self.Doc.addObject('PartDesign::AdditiveWedge', 'Wedge')
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
        self.assertEqual(len(reverseMap),26)
        self.assertEqual(len(faces),6)
        self.assertEqual(len(edges),12)
        self.assertEqual(len(vertexes),8)

        # body.BaseFeature = box

    def testPartDesignElementMapSubBox(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subbox = self.Doc.addObject('PartDesign::SubtractiveBox', 'Box')
        subbox.BaseFeature = box
        body.addObject(subbox)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 44)

    def testPartDesignElementMapSubCylinder(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subcylinder = self.Doc.addObject('PartDesign::SubtractiveCylinder', 'Cylinder')
        subcylinder.BaseFeature = box
        body.addObject(subcylinder)
        # Assert
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 38)

    def testPartDesignElementMapSubSphere(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subsphere = self.Doc.addObject('PartDesign::SubtractiveSphere', 'Sphere')
        subsphere.BaseFeature = box
        body.addObject(subsphere)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 33)

    def testPartDesignElementMapSubCone(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subcone = self.Doc.addObject('PartDesign::SubtractiveCone', 'Cone')
        subcone.BaseFeature = box
        body.addObject(subcone)
        # Assert
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 38)

    def testPartDesignElementMapSubEllipsoid(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subellipsoid = self.Doc.addObject('PartDesign::SubtractiveEllipsoid', 'Ellipsoid')
        subellipsoid.BaseFeature = box
        body.addObject(subellipsoid)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 33)

    def testPartDesignElementMapSubTorus(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subtorus = self.Doc.addObject('PartDesign::SubtractiveTorus', 'Torus')
        subtorus.BaseFeature = box
        body.addObject(subtorus)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 38)

    def testPartDesignElementMapSubPrism(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subprism = self.Doc.addObject('PartDesign::SubtractivePrism', 'Prism')
        subprism.BaseFeature = box
        body.addObject(subprism)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 44)

    def testPartDesignElementMapSubWedge(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        subwedge = self.Doc.addObject('PartDesign::SubtractiveWedge', 'Wedge')
        subwedge.BaseFeature = box
        body.addObject(subwedge)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 50)

    def testPartDesignElementMapSketch(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        sketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        pad = self.Doc.addObject('PartDesign::Pad', 'Pad')
        pad.Profile = sketch
        body.addObject(sketch)
        body.addObject(pad)
        self.Doc.recompute()
        # Assert
        # self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 30)
        self.assertEqual(body.Shape.ElementMapSize,30)
        self.assertEqual(sketch.Shape.ElementMapSize,12)
        self.assertEqual(pad.Shape.ElementMapSize,30)
        # Todo: Assert that the names in the ElementMap are good;
        #  in particular that they are hashed with a # starting

    def testPartDesignElementMapRevolution(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        sketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        revolution = self.Doc.addObject('PartDesign::Revolution', 'Revolution')
        revolution.ReferenceAxis = (self.Doc.getObject('Sketch'),['V_Axis'])
        revolution.Profile = sketch  # Causing segfault
        body.addObject(sketch)
        body.addObject(revolution)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 8)

    def testPartDesignElementMapLoft(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        sketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        sketch2 = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        TestSketcherApp.CreateRectangleSketch(sketch2, (0, 0), (2, 2))
        sketch2.Placement.move(App.Vector(0, 0, 3))
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        loft = self.Doc.addObject('PartDesign::AdditiveLoft', 'Loft')
        loft.Profile = sketch
        loft.Sections = [sketch2]
        body.addObject(sketch)
        body.addObject(sketch2)
        body.addObject(loft)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 30)

    def testPartDesignElementMapPipe(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        sketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        sketch2 = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        TestSketcherApp.CreateRectangleSketch(sketch2, (0, 0), (2, 2))
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        pipe = self.Doc.addObject('PartDesign::AdditivePipe', 'Pipe')
        pipe.Profile = sketch
        pipe.Spine = sketch2
        body.addObject(sketch)
        body.addObject(sketch2)
        body.addObject(pipe)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 64)

    def testPartDesignElementMapHelix(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        sketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Act
        helix = self.Doc.addObject('PartDesign::AdditiveHelix', 'Helix')
        helix.Profile = sketch
        helix.ReferenceAxis = (self.Doc.getObject('Sketch'),['V_Axis'])
        body.addObject(sketch)
        body.addObject(helix)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        # The next size can vary based on tOCCT version (26 or 30), so we accept having entries.
        self.assertGreaterEqual(body.Shape.childShapes()[0].ElementMapSize, 26)

    def testPartDesignElementMapPocket(self):
        pass  # TODO

    def testPartDesignElementMapHole(self):
        pass  # TODO

    def testPartDesignElementMapGroove(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        body.addObject(box)

        groove = self.Doc.addObject('PartDesign::Groove', 'Groove')
        body.addObject(groove)
        groove.ReferenceAxis = (self.Doc.getObject('Y_Axis'), [''])
        groove.Angle = 360.0
        groove.Profile = (box, ['Face6'])
        groove.ReferenceAxis = (box, ['Edge9'])
        groove.Midplane = 0
        groove.Reversed = 0
        groove.Base = App.Vector(0, 0, 0)
        self.Doc.recompute()
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Assert
        # print(groove.Shape.childShapes()[0].ElementMap)
        # TODO: Complete me as part of the subtractive features

    def testPartDesignElementMapSubLoft(self):
        pass  # TODO

    def testPartDesignElementMapSubPipe(self):
        pass  # TODO

    def testPartDesignElementMapSubHelix(self):
        pass  # TODO

    def testPartDesignElementMapChamfer(self):
        """ Test Chamfer ( and  FeatureDressup )"""
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        if body.Shape.ElementMapVersion == "":   # Skip without element maps.
            return
        chamfer = self.Doc.addObject('PartDesign::Chamfer', 'Chamfer')
        chamfer.Base = (box, ['Edge1',
                              'Edge2',
                              'Edge3',
                              'Edge4',
                              'Edge5',
                              'Edge6',
                              'Edge7',
                              'Edge8',
                              'Edge9',
                              'Edge10',
                              'Edge11',
                              'Edge12',
                              ])
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
        self.assertEqual(len(reverseMap),98)
        self.assertEqual(len(faces),26) # 6 Faces become 26 ( +8 + 2*6 )
        self.assertEqual(len(edges),48) # 12 Edges become 48
        self.assertEqual(len(vertexes),24) # 8 Vertices become 24

    def testPartDesignElementMapFillet(self):
        """ Test Fillet ( and  FeatureDressup )"""
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        if body.Shape.ElementMapVersion == "":   # Skip without element maps.
            return
        fillet = self.Doc.addObject('PartDesign::Fillet', 'Fillet')
        fillet.Base = (box, ['Edge1',
                              'Edge2',
                              'Edge3',
                              'Edge4',
                              'Edge5',
                              'Edge6',
                              'Edge7',
                              'Edge8',
                              'Edge9',
                              'Edge10',
                              'Edge11',
                              'Edge12',
                              ])
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
        self.assertEqual(len(reverseMap),106)
        self.assertEqual(len(faces),26) # 6 Faces become 26 ( +8 + 2*6 )
        self.assertEqual(len(edges),56) # 12 Edges become 56  Why?
        self.assertEqual(len(vertexes),24) # 8 Vertices become 24

    def testPartDesignElementMapTransform(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        if body.Shape.ElementMapVersion == "":   # Skip without element maps.
            return
        multitransform = self.Doc.addObject('PartDesign::MultiTransform', 'MultiTransform')
        scaled = self.Doc.addObject('PartDesign::Scaled', 'Scaled')
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
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        shapebinder = self.Doc.addObject('PartDesign::ShapeBinder', 'ShapeBinder')
        if body.Shape.ElementMapVersion == "":   # Skip without element maps.
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
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        subshapebinder = self.Doc.addObject('PartDesign::SubShapeBinder', 'SubShapeBinder')
        if body.Shape.ElementMapVersion == "":   # Skip without element maps.
            return
        # Act / Assert
        body.addObject(box)
        body.addObject(subshapebinder)
        subshapebinder.Support = [ (box, ["Face1"]) ]
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(subshapebinder.Shape.childShapes()[0].ElementMapSize, 9)

    def testSketchElementMap(self):
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        sketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        body.addObject(sketch)
        self.Doc.recompute()
        self.assertEqual(sketch.Shape.ElementMapSize, 12)
        pad = self.Doc.addObject('PartDesign::Pad', 'Pad')
        pad.Profile = sketch
        body.addObject(pad)
        self.Doc.recompute()
        if body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Assert
        self.assertEqual(sketch.Shape.ElementMapSize, 12)
        self.assertEqual(pad.Shape.ElementMapSize, 30)  # The sketch plus the pad in the map
        # TODO:  differing results between main and LS3 on these values.  Does it matter?
        # self.assertEqual(body.Shape.ElementMapSize,0)   # 8?
        # self.assertEqual(body.Shape.ElementMapSize,30) # 26

    def testPlaneElementMap(self):
        plane = self.Doc.addObject("Part::Plane", "Plane")
        plane.Length = 10
        plane.Width = 10
        self.Doc.recompute()
        self.assertEqual(plane.Shape.ElementMapSize, 0)
        pad = self.Doc.addObject('PartDesign::Pad', 'Pad')
        pad.Profile = plane
        self.Doc.recompute()
        if pad.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        # Assert
        self.assertEqual(plane.Shape.ElementMapSize, 0)
        self.assertEqual(pad.Shape.ElementMapSize, 26)

    def testChangeSketch(self):
        # Arrange
        self.Body = self.Doc.addObject('PartDesign::Body', 'Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (31.37, 25.2))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 10
        self.Doc.recompute()

        self.Sketch001 = self.Body.newObject('Sketcher::SketchObject','Sketch001')
        self.Sketch001.AttachmentSupport = (self.Doc.getObject('Pad'),['Face6',])
        self.Sketch001.MapMode = 'FlatFace'
        App.ActiveDocument.recompute()

        self.Sketch001.addExternal("Pad","Edge10")
        self.Sketch001.addExternal("Pad","Edge7")

        geoList = []
        geoList.append(Part.Circle(App.Vector(15.093666, 13.036922, 0.000000),
                                   App.Vector(0.000000, 0.000000, 1.000000), 5.000000))
        self.Sketch001.addGeometry(geoList,False)
        del geoList
        self.Sketch001.addConstraint(Sketcher.Constraint('Radius',0,5.000000))
        self.Sketch001.addConstraint(Sketcher.Constraint('Symmetric',-3,2,-4,1,0,3))
        App.ActiveDocument.recompute()
        self.Doc.recompute()

        self.Pad001 = self.Body.newObject('PartDesign::Pad','Pad001')
        self.Pad001.Profile = self.Doc.getObject('Sketch001')
        self.Pad001.Length = 10
        App.ActiveDocument.recompute()
        self.Pad001.ReferenceAxis = (self.Doc.getObject('Sketch001'),['N_Axis'])
        self.Sketch001.Visibility = False
        App.ActiveDocument.recompute()

        self.Pad001.Length = 10.000000
        self.Pad001.TaperAngle = 0.000000
        self.Pad001.UseCustomVector = 0
        self.Pad001.Direction = (0, 0, 1)
        self.Pad001.ReferenceAxis = (self.Doc.getObject('Sketch001'), ['N_Axis'])
        self.Pad001.AlongSketchNormal = 1
        self.Pad001.Type = 0
        self.Pad001.UpToFace = None
        self.Pad001.Reversed = 0
        self.Pad001.Midplane = 0
        self.Pad001.Offset = 0
        self.Doc.recompute()
        self.Doc.getObject('Pad').Visibility = False

        self.Doc.getObject('Sketch001').Visibility = False

        # Modify the original sketch to generate TNP issue
        geoList = []
        geoList.append(Part.LineSegment(App.Vector(2.510468, 22.837425, 0.000000),
                                        App.Vector(2.510468, 19.933617, 0.000000)))
        geoList.append(Part.LineSegment(App.Vector(2.510468, 19.933617, 0.000000),
                                        App.Vector(4.869811, 19.933617, 0.000000)))
        geoList.append(Part.LineSegment(App.Vector(4.869811, 19.933617, 0.000000),
                                        App.Vector(4.869811, 22.837425, 0.000000)))
        geoList.append(Part.LineSegment(App.Vector(4.869811, 22.837425, 0.000000),
                                        App.Vector(2.510468, 22.837425, 0.000000)))
        self.PadSketch.addGeometry(geoList,False)
        del geoList

        constraintList = []
        constraintList.append(Sketcher.Constraint('Coincident', 4, 2, 5, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 5, 2, 6, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 6, 2, 7, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 7, 2, 4, 1))
        constraintList.append(Sketcher.Constraint('Vertical', 4))
        constraintList.append(Sketcher.Constraint('Vertical', 6))
        constraintList.append(Sketcher.Constraint('Horizontal', 5))
        constraintList.append(Sketcher.Constraint('Horizontal', 7))
        self.PadSketch.addConstraint(constraintList)
        del constraintList
        self.Doc.recompute()
        # Assert
        if self.Body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        self.assertEqual(self.Body.Shape.BoundBox.XMin,0)
        self.assertEqual(self.Body.Shape.BoundBox.YMin,0)
        self.assertEqual(self.Body.Shape.BoundBox.ZMin,0)
        self.assertEqual(self.Body.Shape.BoundBox.XMax,31.37)
        self.assertEqual(self.Body.Shape.BoundBox.YMax,25.2)
        self.assertEqual(self.Body.Shape.BoundBox.ZMax,20)

    def testApplyFillet(self):
        # Arrange
        self.Body = self.Doc.addObject('PartDesign::Body', 'Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (31.37, 25.2))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 10
        self.Doc.recompute()

        self.Sketch001 = self.Body.newObject('Sketcher::SketchObject','Sketch001')
        self.Sketch001.AttachmentSupport = (self.Doc.getObject('Pad'),['Face6',])
        self.Sketch001.MapMode = 'FlatFace'
        App.ActiveDocument.recompute()

        self.Sketch001.addExternal("Pad","Edge10")
        self.Sketch001.addExternal("Pad","Edge7")

        geoList = []
        geoList.append(Part.Circle(App.Vector(15.093666, 13.036922, 0.000000),
                                   App.Vector(0.000000, 0.000000, 1.000000), 5.000000))
        self.Sketch001.addGeometry(geoList,False)
        del geoList
        self.Sketch001.addConstraint(Sketcher.Constraint('Radius',0,5.000000))
        self.Sketch001.addConstraint(Sketcher.Constraint('Symmetric',-3,2,-4,1,0,3))
        App.ActiveDocument.recompute()
        self.Doc.recompute()

        self.Pad001 = self.Body.newObject('PartDesign::Pad','Pad001')
        self.Pad001.Profile = self.Doc.getObject('Sketch001')
        self.Pad001.Length = 10
        App.ActiveDocument.recompute()
        self.Pad001.ReferenceAxis = (self.Doc.getObject('Sketch001'),['N_Axis'])
        self.Sketch001.Visibility = False
        App.ActiveDocument.recompute()

        self.Pad001.Length = 10.000000
        self.Pad001.TaperAngle = 0.000000
        self.Pad001.UseCustomVector = 0
        self.Pad001.Direction = (0, 0, 1)
        self.Pad001.ReferenceAxis = (self.Doc.getObject('Sketch001'), ['N_Axis'])
        self.Pad001.AlongSketchNormal = 1
        self.Pad001.Type = 0
        self.Pad001.UpToFace = None
        self.Pad001.Reversed = 0
        self.Pad001.Midplane = 0
        self.Pad001.Offset = 0
        self.Doc.recompute()
        self.Doc.getObject('Pad').Visibility = False

        self.Doc.getObject('Sketch001').Visibility = False

        area1 = self.Pad.Shape.Area
        # Act
        self.Doc.getObject('Sketch').fillet(2,3,
                                            App.Vector(6.673934,25.000000,0),
                                            App.Vector(0.000000,21.980343,0),
                                            4.740471,True,True,False)
        self.Doc.recompute()
        area2 = self.Pad.Shape.Area

        # Assert
        if self.Body.Shape.ElementMapVersion == "":  # Should be '4' as of Mar 2023.
            return
        self.assertEqual(self.Body.Shape.BoundBox.XMin,0)
        self.assertEqual(self.Body.Shape.BoundBox.YMin,0)
        self.assertEqual(self.Body.Shape.BoundBox.ZMin,0)
        self.assertEqual(self.Body.Shape.BoundBox.XMax,31.37)
        self.assertAlmostEqual(self.Body.Shape.BoundBox.YMax,25.2)
        self.assertEqual(self.Body.Shape.BoundBox.ZMax,20)
        self.assertNotEquals(area1, area2)

    def testShapeBinder(self):
        doc = self.Doc
        self.Body = doc.addObject('PartDesign::Body', 'TNP_Test_Body_SubShape')
        doc.getObject('TNP_Test_Body_SubShape').Label = 'TNP_Test_Body_SubShape'

        doc.recompute()
        doc.getObject('TNP_Test_Body_SubShape').newObject('Sketcher::SketchObject', 'Sketch')
        doc.Sketch.AttachmentSupport = (doc.getObject('XY_Plane'), [''])
        doc.Sketch.MapMode = 'FlatFace'
        doc.recompute()

        geoList = []
        geoList.append(
            Part.LineSegment(App.Vector(0.000000, 0.000000, 0.000000),
                             App.Vector(35.000000, 0.000000, 0.000000)))
        geoList.append(
            Part.LineSegment(App.Vector(35.000000, 0.000000, 0.000000),
                             App.Vector(35.000000, 25.000000, 0.000000)))
        geoList.append(
            Part.LineSegment(App.Vector(35.000000, 25.000000, 0.000000),
                             App.Vector(0.000000, 25.000000, 0.000000)))
        geoList.append(
            Part.LineSegment(App.Vector(0.000000, 25.000000, 0.000000),
                             App.Vector(0.000000, 0.000000, 0.000000)))
        doc.Sketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        constraintList.append(Sketcher.Constraint('Coincident', 0, 2, 1, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 1, 2, 2, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 2, 2, 3, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 3, 2, 0, 1))
        constraintList.append(Sketcher.Constraint('Horizontal', 0))
        constraintList.append(Sketcher.Constraint('Horizontal', 2))
        constraintList.append(Sketcher.Constraint('Vertical', 1))
        constraintList.append(Sketcher.Constraint('Vertical', 3))
        doc.Sketch.addConstraint(constraintList)
        del constraintList

        doc.Sketch.addConstraint(Sketcher.Constraint('Distance', 1, 1, 3, 2, 35.000000))
        doc.Sketch.addConstraint(Sketcher.Constraint('Distance', 0, 1, 2, 2, 25.000000))
        doc.Sketch.addConstraint(Sketcher.Constraint('Coincident', 0, 1, -1, 1))

        doc.recompute()

        doc.getObject('TNP_Test_Body_SubShape').newObject('PartDesign::Pad', 'Pad')
        doc.Pad.Profile = doc.Sketch
        doc.Pad.Length = 10
        doc.recompute()
        doc.Pad.ReferenceAxis = (doc.Sketch, ['N_Axis'])
        doc.Sketch.Visibility = False
        doc.recompute()
        doc.Pad.Length = 10.000000
        doc.Pad.TaperAngle = 0.000000
        doc.Pad.UseCustomVector = 0
        doc.Pad.Direction = (0, 0, 1)
        doc.Pad.ReferenceAxis = (doc.Sketch, ['N_Axis'])
        doc.Pad.AlongSketchNormal = 1
        doc.Pad.Type = 0
        doc.Pad.UpToFace = None
        doc.Pad.Reversed = 0
        doc.Pad.Midplane = 0
        doc.Pad.Offset = 0
        doc.recompute()
        doc.Sketch.Visibility = False

        doc.addObject('PartDesign::Body', 'TNP_Test_Body_Second')
        doc.getObject('TNP_Test_Body_Second').Label = 'TNP_Test_Body_Second'
        doc.recompute()
        obj = doc.getObject('TNP_Test_Body_Second').newObject('PartDesign::ShapeBinder',
                                                              'ShapeBinder')
        obj.Support = (doc.getObject("TNP_Test_Body_SubShape"), [u'Face6'])
        doc.recompute()
        doc.getObject('TNP_Test_Body_Second').newObject('Sketcher::SketchObject', 'Sketch001')
        doc.getObject('Sketch001').AttachmentSupport = (doc.getObject('ShapeBinder'), [''])
        doc.getObject('Sketch001').MapMode = 'FlatFace'
        doc.recompute()

        geoList = []
        geoList.append(
            Part.Circle(App.Vector(14.725412, 16.666899, 0.000000),
                        App.Vector(0.000000, 0.000000, 1.000000), 2.162720))
        doc.getObject('Sketch001').addGeometry(geoList, False)
        del geoList

        doc.recompute()
        doc.getObject('TNP_Test_Body_Second').newObject('PartDesign::Pad', 'Pad001')
        doc.getObject('Pad001').Profile = doc.getObject('Sketch001')
        doc.getObject('Pad001').Length = 10
        doc.recompute()
        doc.getObject('Pad001').ReferenceAxis = (doc.getObject('Sketch001'), ['N_Axis'])
        doc.getObject('Sketch001').Visibility = False
        doc.recompute()
        doc.getObject('Pad001').Length = 10.000000
        doc.getObject('Pad001').TaperAngle = 0.000000
        doc.getObject('Pad001').UseCustomVector = 0
        doc.getObject('Pad001').Direction = (0, 0, 1)
        doc.getObject('Pad001').ReferenceAxis = (doc.getObject('Sketch001'), ['N_Axis'])
        doc.getObject('Pad001').AlongSketchNormal = 1
        doc.getObject('Pad001').Type = 0
        doc.getObject('Pad001').UpToFace = None
        doc.getObject('Pad001').Reversed = 0
        doc.getObject('Pad001').Midplane = 0
        doc.getObject('Pad001').Offset = 0
        doc.recompute()
        doc.getObject('ShapeBinder').Visibility = False
        doc.getObject('Sketch001').Visibility = False

        geoList = []
        geoList.append(
            Part.LineSegment(App.Vector(28.380075, 21.486303, 0.000000),
                             App.Vector(28.380075, 15.462212, 0.000000)))
        geoList.append(
            Part.LineSegment(App.Vector(28.380075, 15.462212, 0.000000),
                             App.Vector(32.797741, 15.462212, 0.000000)))
        geoList.append(
            Part.LineSegment(App.Vector(32.797741, 15.462212, 0.000000),
                             App.Vector(32.797741, 21.486303, 0.000000)))
        geoList.append(
            Part.LineSegment(App.Vector(32.797741, 21.486303, 0.000000),
                             App.Vector(28.380075, 21.486303, 0.000000)))
        doc.Sketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        constraintList.append(Sketcher.Constraint('Coincident', 4, 2, 5, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 5, 2, 6, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 6, 2, 7, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 7, 2, 4, 1))
        constraintList.append(Sketcher.Constraint('Vertical', 4))
        constraintList.append(Sketcher.Constraint('Vertical', 6))
        constraintList.append(Sketcher.Constraint('Horizontal', 5))
        constraintList.append(Sketcher.Constraint('Horizontal', 7))
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
        self.Body = doc.addObject('PartDesign::Body', 'Body')
        doc.Body.Label = 'Body'
        doc.recompute()
        doc.Body.newObject('Sketcher::SketchObject', 'Sketch')
        doc.Sketch.AttachmentSupport = (doc.getObject('XY_Plane'), [''])
        doc.Sketch.MapMode = 'FlatFace'
        doc.recompute()

        geoList = []
        geoList.append(
            Part.LineSegment(App.Vector(0.000000, 0.000000, 0.000000),
                             App.Vector(35.000000, 0.000000, 0.000000)))
        geoList.append(
            Part.LineSegment(App.Vector(35.000000, 0.000000, 0.000000),
                             App.Vector(35.000000, 25.000000, 0.000000)))
        geoList.append(
            Part.LineSegment(App.Vector(35.000000, 25.000000, 0.000000),
                             App.Vector(0.000000, 25.000000, 0.000000)))
        geoList.append(
            Part.LineSegment(App.Vector(0.000000, 25.000000, 0.000000),
                             App.Vector(0.000000, 0.000000, 0.000000)))
        doc.Sketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        constraintList.append(Sketcher.Constraint('Coincident', 0, 2, 1, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 1, 2, 2, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 2, 2, 3, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 3, 2, 0, 1))
        constraintList.append(Sketcher.Constraint('Horizontal', 0))
        constraintList.append(Sketcher.Constraint('Horizontal', 2))
        constraintList.append(Sketcher.Constraint('Vertical', 1))
        constraintList.append(Sketcher.Constraint('Vertical', 3))
        doc.Sketch.addConstraint(constraintList)
        del constraintList

        doc.Sketch.addConstraint(Sketcher.Constraint('Distance', 1, 1, 3, 2, 35.000000))
        doc.Sketch.addConstraint(Sketcher.Constraint('Distance', 0, 1, 2, 2, 25.000000))
        doc.Sketch.addConstraint(Sketcher.Constraint('Coincident', 0, 1, -1, 1))

        doc.recompute()
        doc.Body.newObject('PartDesign::Pad', 'Pad')
        doc.Pad.Profile = doc.Sketch
        doc.Pad.Length = 10
        doc.recompute()
        doc.Pad.ReferenceAxis = (doc.Sketch, ['N_Axis'])
        doc.Sketch.Visibility = False
        doc.Pad.Length = 10.000000
        doc.Pad.TaperAngle = 0.000000
        doc.Pad.UseCustomVector = 0
        doc.Pad.Direction = (0, 0, 1)
        doc.Pad.ReferenceAxis = (doc.Sketch, ['N_Axis'])
        doc.Pad.AlongSketchNormal = 1
        doc.Pad.Type = 0
        doc.Pad.UpToFace = None
        doc.Pad.Reversed = 0
        doc.Pad.Midplane = 0
        doc.Pad.Offset = 0
        doc.recompute()
        doc.Sketch.Visibility = False
        doc.addObject('PartDesign::Body', 'Body001')
        doc.getObject('Body001').Label = 'Body'
        doc.recompute()
        binder = doc.getObject('Body001').newObject('PartDesign::SubShapeBinder', 'Binder')
        binder.Support = self.Body
        doc.getObject('Body001').newObject('Sketcher::SketchObject', 'Sketch001')
        doc.getObject('Sketch001').AttachmentSupport = (doc.getObject('Binder'), ['Face6', ])
        doc.getObject('Sketch001').MapMode = 'FlatFace'
        doc.recompute()

        geoList = []
        geoList.append(
            Part.Circle(App.Vector(16.566162, 13.537925, 0.000000),
                        App.Vector(0.000000, 0.000000, 1.000000), 2.197371))
        doc.getObject('Sketch001').addGeometry(geoList, False)
        del geoList

        doc.recompute()
        ### Begin command PartDesign_Pad
        doc.getObject('Body001').newObject('PartDesign::Pad', 'Pad001')
        doc.Pad001.Profile = doc.getObject('Sketch001')
        doc.Pad001.Length = 10
        doc.recompute()
        doc.Pad001.ReferenceAxis = (doc.getObject('Sketch001'), ['N_Axis'])
        doc.getObject('Sketch001').Visibility = False
        doc.recompute()
        doc.Pad001.Length = 10.000000
        doc.Pad001.TaperAngle = 0.000000
        doc.Pad001.UseCustomVector = 0
        doc.Pad001.Direction = (0, 0, 1)
        doc.Pad001.ReferenceAxis = (doc.getObject('Sketch001'), ['N_Axis'])
        doc.Pad001.AlongSketchNormal = 1
        doc.Pad001.Type = 0
        doc.Pad001.UpToFace = None
        doc.Pad001.Reversed = 0
        doc.Pad001.Midplane = 0
        doc.Pad001.Offset = 0
        doc.recompute()
        doc.getObject('Binder').Visibility = False
        doc.getObject('Sketch001').Visibility = False

        geoList = []
        geoList.append(
            Part.LineSegment(App.Vector(30.009926, 21.026653, 0.000000),
                             App.Vector(30.009926, 16.425089, 0.000000)))
        geoList.append(
            Part.LineSegment(App.Vector(30.009926, 16.425089, 0.000000),
                             App.Vector(31.994911, 16.425089, 0.000000)))
        geoList.append(
            Part.LineSegment(App.Vector(31.994911, 16.425089, 0.000000),
                             App.Vector(31.994911, 21.026653, 0.000000)))
        geoList.append(
            Part.LineSegment(App.Vector(31.994911, 21.026653, 0.000000),
                             App.Vector(30.009926, 21.026653, 0.000000)))
        doc.Sketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        constraintList.append(Sketcher.Constraint('Coincident', 4, 2, 5, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 5, 2, 6, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 6, 2, 7, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 7, 2, 4, 1))
        constraintList.append(Sketcher.Constraint('Vertical', 4))
        constraintList.append(Sketcher.Constraint('Vertical', 6))
        constraintList.append(Sketcher.Constraint('Horizontal', 5))
        constraintList.append(Sketcher.Constraint('Horizontal', 7))
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
        """ Test Chamfer """
        # Arrange
        doc = self.Doc
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        body.addObject(box)
        self.Doc.recompute()
        volume1 = body.Shape.Volume
        chamfer = self.Doc.addObject('PartDesign::Chamfer', 'Chamfer')
        chamfer.Base = (box, ['Edge1',
                              'Edge5',
                              'Edge7',
                              ])
        chamfer.Size = 1
        body.addObject(chamfer)
        self.Doc.recompute()
        volume2 = body.Shape.Volume

        doc.Body.newObject('Sketcher::SketchObject', 'Sketch')
        doc.Sketch.AttachmentSupport = (chamfer, "Face8")
        doc.Sketch.MapMode = 'FlatFace'
        doc.recompute()

        x1, x2, y1, y2 = 10 / math.sqrt(2) - math.sqrt(2), 10 / math.sqrt(2) + math.sqrt(2), 6, 11
        geoList = []
        geoList.append(
            Part.LineSegment(App.Vector(x1, y1, 0.0 ),
                             App.Vector(x1, y2, 0.0 )))
        geoList.append(
            Part.LineSegment(App.Vector(x1, y2, 0.0),
                             App.Vector(x2, y2, 0.0)))
        geoList.append(
            Part.LineSegment(App.Vector(x2, y2, 0.0),
                             App.Vector(x2, y1, 0.0)))
        geoList.append(
            Part.LineSegment(App.Vector(x2, y1, 0.0),
                             App.Vector(x1, y1, 0.0)))
        doc.Sketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        constraintList.append(Sketcher.Constraint('Coincident', 0, 2, 1, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 1, 2, 2, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 2, 2, 3, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 3, 2, 0, 1))
        constraintList.append(Sketcher.Constraint('Horizontal', 0))
        constraintList.append(Sketcher.Constraint('Horizontal', 2))
        constraintList.append(Sketcher.Constraint('Vertical', 1))
        constraintList.append(Sketcher.Constraint('Vertical', 3))
        doc.Sketch.addConstraint(constraintList)
        del constraintList
        body.addObject(doc.Sketch)

        pocket = self.Doc.addObject('PartDesign::Pocket', 'Pocket')
        pocket.Type = "Length"
        pocket.Length = 3
        pocket.Direction = App.Vector(-0.710000000,0.7100000000, 0.0000000000)
        pocket.Profile = doc.Sketch
        body.addObject(pocket)
        self.Doc.recompute()
        volume3 = body.Shape.Volume
        # Change the chamfered edges, potentially triggering TNP
        chamfer.Base = (box, ['Edge5',
                              'Edge7',
                              ])
        self.Doc.recompute()
        volume4 = body.Shape.Volume
        # Assert
        if body.Shape.ElementMapVersion == "":   # Skip without element maps.
            return
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 62)
        self.assertEqual(len(reverseMap),62)
        self.assertEqual(len(faces),12)
        self.assertEqual(len(edges),30)
        self.assertEqual(len(vertexes),20)
        boxVolume = 10 * 10 * 10
        chamferVolume = 1 * 1 * 0.5 * 10
        # cut area is rectangle with sqrt(2) as one side minus 2 isosceles right triangles
        cutArea = (2 * math.sqrt(2)) * 3 - ((math.sqrt(2)/2 * math.sqrt(2)/2)/2)*2
        cutVolume = cutArea * 4 # height is 4  ( 11-6 with a limit of 10 from the box )
        self.assertAlmostEqual(volume1, boxVolume )
        self.assertAlmostEqual(volume2, boxVolume - 3 * chamferVolume)
        self.assertAlmostEqual(volume3, boxVolume - 3 * chamferVolume - cutVolume, 4)
        self.assertAlmostEqual(volume4, boxVolume - 2 * chamferVolume - cutVolume, 4)

    def testPartDesignTNPFillet(self):
        """ Test Fillet """
        # Arrange
        doc = self.Doc
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        body.addObject(box)
        self.Doc.recompute()
        volume1 = body.Shape.Volume
        fillet = self.Doc.addObject('PartDesign::Fillet', 'Fillet')
        fillet.Base = (box, ['Edge1',
                              'Edge5',
                              'Edge7',
                              ])
        # fillet.Size = 1
        body.addObject(fillet)
        self.Doc.recompute()
        volume2 = body.Shape.Volume

        doc.Body.newObject('Sketcher::SketchObject', 'Sketch')
        doc.Sketch.AttachmentSupport = (fillet, "Face2")
        doc.Sketch.MapMode = 'FlatFace'
        doc.recompute()

        x1, x2, y1, y2 = 4,6 , 6, 11
        geoList = []
        geoList.append(
            Part.LineSegment(App.Vector(x1, y1, 0.0 ),
                             App.Vector(x1, y2, 0.0 )))
        geoList.append(
            Part.LineSegment(App.Vector(x1, y2, 0.0),
                             App.Vector(x2, y2, 0.0)))
        geoList.append(
            Part.LineSegment(App.Vector(x2, y2, 0.0),
                             App.Vector(x2, y1, 0.0)))
        geoList.append(
            Part.LineSegment(App.Vector(x2, y1, 0.0),
                             App.Vector(x1, y1, 0.0)))
        doc.Sketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        constraintList.append(Sketcher.Constraint('Coincident', 0, 2, 1, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 1, 2, 2, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 2, 2, 3, 1))
        constraintList.append(Sketcher.Constraint('Coincident', 3, 2, 0, 1))
        constraintList.append(Sketcher.Constraint('Horizontal', 0))
        constraintList.append(Sketcher.Constraint('Horizontal', 2))
        constraintList.append(Sketcher.Constraint('Vertical', 1))
        constraintList.append(Sketcher.Constraint('Vertical', 3))
        doc.Sketch.addConstraint(constraintList)
        del constraintList
        body.addObject(doc.Sketch)

        pocket = self.Doc.addObject('PartDesign::Pocket', 'Pocket')
        pocket.Type = "Length"
        pocket.Length = 3
        pocket.Direction = App.Vector(-0.710000000,0.7100000000, 0.0000000000)
        pocket.Profile = doc.Sketch
        # pocket.Reversed = False
        body.addObject(pocket)
        self.Doc.recompute()
        volume3 = body.Shape.Volume
        # Change the filleted edges, potentially triggering TNP
        fillet.Base = (box, ['Edge5',
                              'Edge7',
                              ])
        self.Doc.recompute()
        volume4 = body.Shape.Volume
        # Assert
        if body.Shape.ElementMapVersion == "":   # Skip without element maps.
            return
        reverseMap = body.Shape.childShapes()[0].ElementReverseMap
        faces = [name for name in reverseMap.keys() if name.startswith("Face")]
        edges = [name for name in reverseMap.keys() if name.startswith("Edge")]
        vertexes = [name for name in reverseMap.keys() if name.startswith("Vertex")]
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 62)
        self.assertEqual(len(reverseMap),62)
        self.assertEqual(len(faces),12)
        self.assertEqual(len(edges),30)
        self.assertEqual(len(vertexes),20)
        boxVolume = 10 * 10 * 10
        # Full prism minus the rounded triangle prism.
        filletVolume = 1 * 1 * 10 - 1 * 1 * math.pi / 4 * 10 #0.5 * 10
        cutVolume = 24
        self.assertAlmostEqual(volume1, boxVolume )
        self.assertAlmostEqual(volume2, boxVolume - 3 * filletVolume)
        self.assertAlmostEqual(volume3, boxVolume - 3 * filletVolume - cutVolume, 4)
        self.assertAlmostEqual(volume4, boxVolume - 2 * filletVolume - cutVolume, 4)

    def create_t_sketch(self):
        self.Doc.getObject('Body').newObject('Sketcher::SketchObject', 'Sketch')
        geo_list = [
            Part.LineSegment(App.Vector(0, 0, 0), App.Vector(20, 0, 0)),
            Part.LineSegment(App.Vector(20, 0, 0), App.Vector(20, 10, 0)),
            Part.LineSegment(App.Vector(20, 10, 0), App.Vector(10, 10, 0)),
            Part.LineSegment(App.Vector(10, 10, 0), App.Vector(10, 20, 0)),
            Part.LineSegment(App.Vector(10, 20, 0), App.Vector(0, 20, 0)),
            Part.LineSegment(App.Vector(0, 20, 0), App.Vector(0, 0, 0))]
        self.Doc.getObject('Sketch').addGeometry(geo_list, False)
        con_list = [
            Sketcher.Constraint('Coincident', 0, 2, 1, 1),
            Sketcher.Constraint('Coincident', 1, 2, 2, 1),
            Sketcher.Constraint('Coincident', 2, 2, 3, 1),
            Sketcher.Constraint('Coincident', 3, 2, 4, 1),
            Sketcher.Constraint('Coincident', 4, 2, 5, 1),
            Sketcher.Constraint('Coincident', 5, 2, 0, 1),
            Sketcher.Constraint('Horizontal', 0),
            Sketcher.Constraint('Horizontal', 2),
            Sketcher.Constraint('Horizontal', 4),
            Sketcher.Constraint('Vertical', 1),
            Sketcher.Constraint('Vertical', 3),
            Sketcher.Constraint('Vertical', 5)]
        self.Doc.getObject('Sketch').addConstraint(con_list)
        del geo_list, con_list
        self.Doc.recompute()

    def tearDown(self):
        """ Close our test document """
        App.closeDocument("PartDesignTestTNP")
