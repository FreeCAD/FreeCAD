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

        # Todo switch to actually asserting this and remove the printed lines as soon as
        #  the main branch is capable of passing this test.
        # self.assertTrue(self.Pad2.isValid())
        if self.Pad2.isValid():
            print("Topological Naming Problem is not present.")
        else:
            print("TOPOLOGICAL NAMING PROBLEM IS PRESENT.")

    def testPartDesignElementMapBox(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
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
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
            return
        # Act / Assert
        self.assertEqual(len(cylinder.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(cylinder.Shape.childShapes()), 1)
        self.assertEqual(cylinder.Shape.childShapes()[0].ElementMapSize, 8)
        body.addObject(cylinder)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 8)

    def testPartDesignElementMapSphere(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        sphere = self.Doc.addObject('PartDesign::AdditiveSphere', 'Sphere')
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
            return
        # Act / Assert
        self.assertEqual(len(sphere.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(sphere.Shape.childShapes()), 1)
        self.assertEqual(sphere.Shape.childShapes()[0].ElementMapSize, 6)
        body.addObject(sphere)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 6)

    def testPartDesignElementMapCone(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        cone = self.Doc.addObject('PartDesign::AdditiveCone', 'Cone')
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
            return
        # Act / Assert
        self.assertEqual(len(cone.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(cone.Shape.childShapes()), 1)
        self.assertEqual(cone.Shape.childShapes()[0].ElementMapSize, 8)
        body.addObject(cone)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 8)

    def testPartDesignElementMapEllipsoid(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        ellipsoid = self.Doc.addObject('PartDesign::AdditiveEllipsoid', 'Ellipsoid')
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
            return
        # Act / Assert
        self.assertEqual(len(ellipsoid.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(ellipsoid.Shape.childShapes()), 1)
        self.assertEqual(ellipsoid.Shape.childShapes()[0].ElementMapSize, 6)
        body.addObject(ellipsoid)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 6)

    def testPartDesignElementMapTorus(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        torus = self.Doc.addObject('PartDesign::AdditiveTorus', 'Torus')
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
            return
        # Act / Assert
        self.assertEqual(len(torus.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(torus.Shape.childShapes()), 1)
        self.assertEqual(torus.Shape.childShapes()[0].ElementMapSize, 4)
        body.addObject(torus)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 4)

    def testPartDesignElementMapPrism(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        prism = self.Doc.addObject('PartDesign::AdditivePrism', 'Prism')
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
            return
        # Act / Assert
        self.assertEqual(len(prism.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(prism.Shape.childShapes()), 1)
        self.assertEqual(prism.Shape.childShapes()[0].ElementMapSize, 38)
        body.addObject(prism)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 38)

    def testPartDesignElementMapWedge(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        wedge = self.Doc.addObject('PartDesign::AdditiveWedge', 'Wedge')
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
            return
        # Act / Assert
        self.assertEqual(len(wedge.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(wedge.Shape.childShapes()), 1)
        self.assertEqual(wedge.Shape.childShapes()[0].ElementMapSize, 26)
        body.addObject(wedge)
        self.assertEqual(len(body.Shape.childShapes()), 0)
        self.Doc.recompute()
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 26)

        # body.BaseFeature = box

    def testPartDesignElementMapSubBox(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        box.Length = 20
        box.Width = 20
        box.Height = 20
        body.addObject(box)
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
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
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
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
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
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
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
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
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
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
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
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
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
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
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
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
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
            return
        # Act
        pad = self.Doc.addObject('PartDesign::Pad', 'Pad')
        pad.Profile = sketch
        pad.BaseFeature = sketch
        body.addObject(sketch)
        body.addObject(pad)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 26)

    def testPartDesignElementMapRevolution(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        sketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
            return
        # Act
        revolution = self.Doc.addObject('PartDesign::Revolution', 'Revolution')
        revolution.ReferenceAxis = body.Origin.OriginFeatures[1]
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
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
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
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 26)

    def testPartDesignElementMapPipe(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        sketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (1, 1))
        sketch2 = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        TestSketcherApp.CreateRectangleSketch(sketch2, (0, 0), (2, 2))
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
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
        if not hasattr(body,"ElementMapVersion"):   # Skip without element maps.
            return
        # Act
        helix = self.Doc.addObject('PartDesign::AdditiveHelix', 'Helix')
        helix.Profile = sketch
        helix.ReferenceAxis = body.Origin.OriginFeatures[2]
        body.addObject(sketch)
        body.addObject(helix)
        self.Doc.recompute()
        # Assert
        self.assertEqual(len(body.Shape.childShapes()), 1)
        self.assertEqual(body.Shape.childShapes()[0].ElementMapSize, 26)

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
        if body.Shape.ElementMapVersion == "":   # Skip without element maps.
            return
        # Assert
        self.assertEqual(sketch.Shape.ElementMapSize, 12)
        self.assertEqual(pad.Shape.ElementMapSize, 30)  # The sketch plus the pad in the map
        # TODO:  differing results between main and LS3 on these values.  Does it matter?
        # self.assertEqual(body.Shape.ElementMapSize,0)   # 8?
        # self.Doc.recompute()
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
        if pad.Shape.ElementMapVersion == "":   # Skip without element maps.
            return
        # Assert
        self.assertEqual(plane.Shape.ElementMapSize, 0)
        self.assertEqual(pad.Shape.ElementMapSize, 26)

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
