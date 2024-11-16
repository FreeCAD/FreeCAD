#***************************************************************************
#*   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import unittest

import FreeCAD
from FreeCAD import Base
import TestSketcherApp

class TestPad(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestPad")

    def testBoxCase(self):
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject','SketchPad')
        TestSketcherApp.CreateSlotPlateSet(self.PadSketch)
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad","Pad")
        self.Pad.Profile = self.PadSketch
        self.Doc.recompute()
        self.assertEqual(len(self.Pad.Shape.Faces), 6)

    def testSketchOnBasePlane(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject','SketchPad')
        self.PadSketch.AttachmentSupport = (self.Doc.XY_Plane, [''])
        self.PadSketch.MapMode = 'FlatFace'
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateSlotPlateSet(self.PadSketch)
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad","Pad")
        self.Pad.Profile = self.PadSketch
        self.Body.addObject(self.Pad)
        self.Doc.recompute()
        self.assertEqual(len(self.Pad.Shape.Faces), 6)

    def testSketchOnDatumPlane(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.DatumPlane = self.Doc.addObject('PartDesign::Plane','DatumPlane')
        self.DatumPlane.AttachmentSupport = (self.Doc.XY_Plane, [''])
        self.DatumPlane.MapMode = 'FlatFace'
        self.Body.addObject(self.DatumPlane)
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject','SketchPad')
        self.PadSketch.AttachmentSupport = (self.DatumPlane, [''])
        self.PadSketch.MapMode = 'FlatFace'
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateSlotPlateSet(self.PadSketch)
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad","Pad")
        self.Pad.Profile = self.PadSketch
        self.Body.addObject(self.Pad)
        self.Doc.recompute()
        self.assertEqual(len(self.Pad.Shape.Faces), 6)

    def testPadToFirstCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 1), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Doc.recompute()
        # Make second pad on different plane and pad to first
        self.PadSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad1')
        self.Body.addObject(self.PadSketch1)
        self.PadSketch1.MapMode = 'FlatFace'
        self.PadSketch1.AttachmentSupport = (self.Doc.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PadSketch1, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.PadSketch1
        self.Pad1.Type = 2
        self.Pad1.Reversed = 1
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pad1.Shape.Volume, 2.0)

    def testPadtoLastCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0.5, 1), (0.5, 2))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Doc.recompute()
        # Make second pad on different plane and pad to first
        self.PadSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad1')
        self.Body.addObject(self.PadSketch1)
        self.PadSketch1.MapMode = 'FlatFace'
        self.PadSketch1.AttachmentSupport = (self.Doc.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PadSketch1, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.PadSketch1
        self.Pad1.Type = 1
        self.Pad1.Reversed = 1
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pad1.Shape.Volume, 3.0)

    def testPadToFaceCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 1), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Doc.recompute()
        # Make second pad on different plane and pad to face on first
        self.PadSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad1')
        self.Body.addObject(self.PadSketch1)
        self.PadSketch1.MapMode = 'FlatFace'
        self.PadSketch1.AttachmentSupport = (self.Doc.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PadSketch1, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.PadSketch1
        self.Pad1.Type = 3
        self.Pad1.UpToFace = (self.Pad, ["Face3"])
        self.Pad1.Reversed = 1
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pad1.Shape.Volume, 2.0)

    def testPadTwoDimensionsCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 1), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Doc.recompute()
        # Make second pad on different plane and pad to face on first
        self.PadSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad1')
        self.Body.addObject(self.PadSketch1)
        self.PadSketch1.MapMode = 'FlatFace'
        self.PadSketch1.AttachmentSupport = (self.Doc.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PadSketch1, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.PadSketch1
        self.Pad1.Type = 4
        self.Pad1.Length = 1.0
        self.Pad1.Length2 = 2.0
        self.Pad1.Reversed = 1
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pad1.Shape.Volume, 4.0)

    def testPadToConcaveCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make a half revolution
        self.RevolutionSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.RevolutionSketch)
        TestSketcherApp.CreateRectangleSketch(self.RevolutionSketch, (9, 0), (10, 5))
        self.Doc.recompute()
        self.Revolution = self.Doc.addObject("PartDesign::Revolution", "Revolution")
        self.Body.addObject(self.Revolution)
        self.Revolution.Profile = self.RevolutionSketch
        self.Revolution.ReferenceAxis = (self.RevolutionSketch, ['V_Axis'])
        self.Revolution.Angle = 180
        self.Doc.recompute()
        # Make a sketch and pad to first
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Type = 2
        self.Pad.Reversed = True
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pad.Shape.Volume, 2208.0963, places=4)

    def testPadToShapeCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 1), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Doc.recompute()
        # Make second pad on different plane and pad to first
        self.PadSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad1')
        self.Body.addObject(self.PadSketch1)
        self.PadSketch1.MapMode = 'FlatFace'
        self.PadSketch1.AttachmentSupport = (self.Doc.XZ_Plane, [''])
        self.PadSketch1.AttachmentOffset.Rotation.Axis = Base.Vector(0,1,0)
        self.PadSketch1.AttachmentOffset.Rotation.Angle =  0.436332 # 25°
        self.PadSketch1.AttachmentOffset.Base.z = 1
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PadSketch1, (1, 0), (1, 1))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.PadSketch1
        self.Pad1.Type = 5
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pad1.Shape.Volume, 2.58787, places=4)

    def testPadToPlaneCustomDir(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 1), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Type = 3
        self.Doc.Pad.UseCustomVector = True
        self.Doc.Pad.Direction = Base.Vector(0,1,1)
        self.Pad.UpToFace = (self.Doc.XZ_Plane, [''])
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pad.Shape.Volume, 1.5)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestPad")
        #print ("omit closing document for debugging")

