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
# import Part
# import Sketcher
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

        # Make sure we have elementMaps for the sketches
        self.assertEqual(self.PadSketch.Shape.ElementMapSize, 12)
        self.assertEqual(self.PadSketch1.Shape.ElementMapSize, 12)
        self.assertEqual(self.PadSketch2.Shape.ElementMapSize, 12)
        # Make sure we don't have elementMaps for the pads.
        self.assertEqual(self.Pad.Shape.ElementMapSize, 0)
        self.assertEqual(self.Pad1.Shape.ElementMapSize, 0)
        self.assertEqual(self.Pad2.Shape.ElementMapSize, 0)
        # could also check .Shape.BoundBox.   AttachmentSupport will always be consistent.

        # Act
        # Move the second pad ( the sketch attachment point )
        self.PadSketch1.AttachmentOffset = App.Placement(
            App.Vector(0.5000000000, 0.0000000000, 0.0000000000),
            App.Rotation(0.0000000000, 0.0000000000, 0.0000000000))
        self.Doc.recompute()

        # Todo: Helper method for comparing BoundBoxes ( exists in PartTests )
        # Assert everything is valid.
        # self.assertEqual(self.Pad1.Shape.BoundBox.XMax,1.5)
        # self.assertEqual(self.Pad2.Shape.BoundBox.XMax,1.5)
        self.assertTrue(self.Pad.isValid())
        self.assertTrue(self.Pad1.isValid())
        # self.assertTrue(self.Pad2.isValid())  # Todo: Use this instead of the prints
        if self.Pad2.isValid():
            print("Topological Naming Problem is not present.")
        else:
            print("TOPOLOGICAL NAMING PROBLEM IS PRESENT.")

    def testSketchObject(self):
        # Arrange
        self.Body = self.Doc.addObject('PartDesign::Body', 'Body')
        # Act
        self.Sketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.Sketch)
        TestSketcherApp.CreateRectangleSketch(self.Sketch, (0, 0), (1, 1))
        self.Doc.recompute()
        # Assert    Note that sketch already calls the makeElement versions, so no guard if needed
        # if self.Sketch.Shape.ElementMapVersion != "":
        self.assertEqual(self.Sketch.Shape.ElementMapSize, 12)

    def testShapeBinder(self):
        # Arrange
        body = self.Doc.addObject('PartDesign::Body', 'Body3')
        box = self.Doc.addObject('PartDesign::AdditiveBox', 'Box')
        body.addObject(box)

        box.Length=10.00000
        box.Width=10.00000
        box.Height=10.00000

        # Act
        binder = body.newObject('PartDesign::SubShapeBinder','Binder')
        binder.Support=(box, "")
        self.Doc.recompute()

        # Assert
        self.assertAlmostEqual(binder.Shape.Length, 240)
        self.assertEqual(box.Shape.ElementMapSize,0)
        if box.Shape.ElementMapVersion != "":
            self.assertEqual(binder.Shape.ElementMapSize,26)


    def tearDown(self):
        """ Close our test document """
        App.closeDocument("PartDesignTestTNP")
