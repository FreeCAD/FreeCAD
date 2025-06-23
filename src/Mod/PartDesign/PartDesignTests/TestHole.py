#**************************************************************************
#   Copyright (c) 2017 Kurt Kremitzki <kkremitzki@gmail.com>              *
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
#**************************************************************************
from math import pi
import unittest

import FreeCAD
import TestSketcherApp

App = FreeCAD

class TestHole(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestHole")
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Box.Length=10
        self.Box.Width=10
        self.Box.Height=10
        self.Body.addObject(self.Box)
        self.Doc.recompute()
        self.HoleSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchHole')
        self.HoleSketch.AttachmentSupport = (self.Doc.XY_Plane, [''])
        self.HoleSketch.MapMode = 'FlatFace'
        self.HoleSketch.MapReversed = True
        self.Body.addObject(self.HoleSketch)
        TestSketcherApp.CreateCircleSketch(self.HoleSketch, (-5, 5),  1)
        self.Doc.recompute()
        self.Hole = self.Doc.addObject("PartDesign::Hole", "Hole")
        self.Hole.Profile = self.HoleSketch
        self.Body.addObject(self.Hole)
        self.Doc.recompute()

    def testPlainHole(self):
        self.Hole.Diameter = 6
        self.Hole.Depth = 10
        # self.Hole.DrillPointAngle = 118.000000
        # self.Hole.TaperedAngle = 90
        self.Hole.ThreadType = 0
        self.Hole.HoleCutType = 0 # 1 = Counterbore, 2 = Countersink
        # self.Hole.HoleCutDiameter = 5
        # self.Hole.HoleCutCountersinkAngle = 90
        # self.Hole.HoleCutDepth = 2 # Counterbore
        self.Hole.DepthType = 0 # 1 = Through all
        self.Hole.DrillPoint = 0 # 1 = Angled
        self.Hole.Tapered = 0 # On/off
        self.Doc.recompute()
        self.assertAlmostEqual(self.Hole.Shape.Volume, 10**3 - pi * 3**2 * 10)

    def testTaperedHole(self):
        self.Hole.Diameter = 6
        self.Hole.Depth = 5
        self.Hole.TaperedAngle = 60
        self.Hole.ThreadType = 0
        self.Hole.HoleCutType = 0
        self.Hole.DepthType = 0
        self.Hole.DrillPoint = 0
        self.Hole.Tapered = 1
        self.Doc.recompute()
        self.assertEqual(len(self.Hole.Shape.Faces), 8)

    def testAngledDrillHole(self):
        self.Hole.Diameter = 6
        self.Hole.Depth = 10
        self.Hole.DrillPointAngle = 118
        self.Hole.ThreadType = 0
        self.Hole.HoleCutType = 0
        self.Hole.DepthType = 0
        self.Hole.DrillPoint = 1
        self.Hole.Tapered = 0
        self.Hole.DrillForDepth = 1
        self.Doc.recompute()
        self.assertEqual(len(self.Hole.Shape.Faces), 8)

    def testCounterboreHole(self):
        self.Hole.Diameter = 6
        self.Hole.Depth = 10
        self.Hole.ThreadType = 0
        self.Hole.HoleCutType = 1
        self.Hole.HoleCutDiameter = 8
        self.Hole.HoleCutDepth = 5
        self.Hole.DepthType = 0
        self.Hole.DrillPoint = 0
        self.Hole.Tapered = 0
        self.Doc.recompute()
        self.assertAlmostEqual(self.Hole.Shape.Volume, 10**3 - pi * 3**2 * 5 - pi * 4**2 * 5)

    def testCountersinkHole(self):
        self.Hole.Diameter = 6
        self.Hole.Depth = 10
        self.Hole.ThreadType = 0
        self.Hole.HoleCutType = 2
        self.Hole.HoleCutDiameter = 9
        self.Hole.HoleCutCountersinkAngle = 90
        self.Hole.DepthType = 0
        self.Hole.DrillPoint = 0
        self.Hole.Tapered = 0
        self.Doc.recompute()
        self.assertAlmostEqual(self.Hole.Shape.Volume, 10**3 - pi * 3**2 * 10 - 24.7400421)

    def testNoRefineHole(self):
        # Add a second box to get a shape with more faces
        self.Box2 = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Box2.Length=10
        self.Box2.Width=10
        self.Box2.Height=10
        self.Box2.AttacherEngine = u"Engine 3D"
        self.Box2.AttachmentOffset = App.Placement(
            App.Vector(1.0000000000, 0.0000000000, 0.0000000000),
            App.Rotation(0.0000000000, 0.0000000000, 0.0000000000),
        )
        self.Box2.MapReversed = False
        self.Box2.AttachmentSupport = self.Doc.getObject('XY_Plane')
        self.Box2.MapPathParameter = 0.000000
        self.Box2.MapMode = 'FlatFace'

        # Set the Refine option to False, otherwise adding the second box would be useless
        self.Box2.Refine = False
        self.Body.addObject(self.Box2)
        self.Doc.recompute()

        # Move the Hole on top of the Body
        self.Body.removeObject(self.Hole)
        self.Body.insertObject(self.Hole,self.Box2, True)
        self.Body.Tip = self.Hole
        self.Hole.Diameter = 6
        self.Hole.Depth = 10
        self.Hole.ThreadType = 0
        self.Hole.HoleCutType = 0
        self.Hole.DepthType = 0
        self.Hole.DrillPoint = 0
        self.Hole.Tapered = 0
        self.Hole.Visibility = True

        # Test the number of faces with the Refine option set to False
        self.Hole.Refine = False
        self.Doc.recompute()
        self.assertEqual(len(self.Hole.Shape.Faces), 15)

    def testRefineHole(self):
        # Add a second box to get a shape with more faces
        self.Box2 = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Box2.Length=10
        self.Box2.Width=10
        self.Box2.Height=10
        self.Box2.AttacherEngine = u"Engine 3D"
        self.Box2.AttachmentOffset = App.Placement(
            App.Vector(1.0000000000, 0.0000000000, 0.0000000000),
            App.Rotation(0.0000000000, 0.0000000000, 0.0000000000),
        )
        self.Box2.MapReversed = False
        self.Box2.AttachmentSupport = self.Doc.getObject('XY_Plane')
        self.Box2.MapPathParameter = 0.000000
        self.Box2.MapMode = 'FlatFace'

        # Set the Refine option to False, otherwise adding the second box would be useless
        self.Box2.Refine = False
        self.Body.addObject(self.Box2)
        self.Doc.recompute()

        # Move the Hole on top of the Body
        self.Body.removeObject(self.Hole)
        self.Body.insertObject(self.Hole,self.Box2, True)
        self.Body.Tip = self.Hole
        self.Hole.Diameter = 6
        self.Hole.Depth = 10
        self.Hole.ThreadType = 0
        self.Hole.HoleCutType = 0
        self.Hole.DepthType = 0
        self.Hole.DrillPoint = 0
        self.Hole.Tapered = 0
        self.Hole.Visibility = True

        # Test the number of faces with the Refine option set to True
        self.Hole.Refine = True
        self.Doc.recompute()
        self.assertEqual(len(self.Hole.Shape.Faces), 7)


    def testThreadEnums(self):
        """Test thread enums for correct order"""
        # Due to the savefile use of indexes and not strings
        # The correct mapping needs to be ensured to not break savefiles
        # The order of the arrays and elements is critical
        thread_types = {
            'ISOMetricProfile': [
                "M1x0.25",   "M1.1x0.25", "M1.2x0.25", "M1.4x0.3",  "M1.6x0.35",
                "M1.8x0.35", "M2x0.4",    "M2.2x0.45", "M2.5x0.45", "M3x0.5",
                "M3.5x0.6",  "M4x0.7",    "M4.5x0.75", "M5x0.8",    "M6x1.0",
                "M7x1.0",    "M8x1.25",   "M9x1.25",   "M10x1.5",   "M11x1.5",
                "M12x1.75",  "M14x2.0",   "M16x2.0",   "M18x2.5",   "M20x2.5",
                "M22x2.5",   "M24x3.0",   "M27x3.0",   "M30x3.5",   "M33x3.5",
                "M36x4.0",   "M39x4.0",   "M42x4.5",   "M45x4.5",   "M48x5.0",
                "M52x5.0",   "M56x5.5",   "M60x5.5",   "M64x6.0",   "M68x6.0",
            ],
            'ISOMetricFineProfile': [
                "M1x0.2",      "M1.1x0.2",    "M1.2x0.2",    "M1.4x0.2",
                "M1.6x0.2",    "M1.8x0.2",    "M2x0.25",     "M2.2x0.25",
                "M2.5x0.35",   "M3x0.35",     "M3.5x0.35",
                "M4x0.5",      "M4.5x0.5",    "M5x0.5",      "M5.5x0.5",
                "M6x0.75",     "M7x0.75",     "M8x0.75",     "M8x1.0",
                "M9x0.75",     "M9x1.0",      "M10x0.75",    "M10x1.0",
                "M10x1.25",    "M11x0.75",    "M11x1.0",     "M12x1.0",
                "M12x1.25",    "M12x1.5",     "M14x1.0",     "M14x1.25",
                "M14x1.5",     "M15x1.0",     "M15x1.5",     "M16x1.0",
                "M16x1.5",     "M17x1.0",     "M17x1.5",     "M18x1.0",
                "M18x1.5",     "M18x2.0",     "M20x1.0",     "M20x1.5",
                "M20x2.0",     "M22x1.0",     "M22x1.5",     "M22x2.0",
                "M24x1.0",     "M24x1.5",     "M24x2.0",     "M25x1.0",
                "M25x1.5",     "M25x2.0",     "M27x1.0",     "M27x1.5",
                "M27x2.0",     "M28x1.0",     "M28x1.5",     "M28x2.0",
                "M30x1.0",     "M30x1.5",     "M30x2.0",     "M30x3.0",
                "M32x1.5",     "M32x2.0",     "M33x1.5",     "M33x2.0",
                "M33x3.0",     "M35x1.5",     "M35x2.0",     "M36x1.5",
                "M36x2.0",     "M36x3.0",     "M39x1.5",     "M39x2.0",
                "M39x3.0",     "M40x1.5",     "M40x2.0",     "M40x3.0",
                "M42x1.5",     "M42x2.0",     "M42x3.0",     "M42x4.0",
                "M45x1.5",     "M45x2.0",     "M45x3.0",     "M45x4.0",
                "M48x1.5",     "M48x2.0",     "M48x3.0",     "M48x4.0",
                "M50x1.5",     "M50x2.0",     "M50x3.0",     "M52x1.5",
                "M52x2.0",     "M52x3.0",     "M52x4.0",     "M55x1.5",
                "M55x2.0",     "M55x3.0",     "M55x4.0",     "M56x1.5",
                "M56x2.0",     "M56x3.0",     "M56x4.0",     "M58x1.5",
                "M58x2.0",     "M58x3.0",     "M58x4.0",     "M60x1.5",
                "M60x2.0",     "M60x3.0",     "M60x4.0",     "M62x1.5",
                "M62x2.0",     "M62x3.0",     "M62x4.0",     "M64x1.5",
                "M64x2.0",     "M64x3.0",     "M64x4.0",     "M65x1.5",
                "M65x2.0",     "M65x3.0",     "M65x4.0",     "M68x1.5",
                "M68x2.0",     "M68x3.0",     "M68x4.0",     "M70x1.5",
                "M70x2.0",     "M70x3.0",     "M70x4.0",     "M70x6.0",
                "M72x1.5",     "M72x2.0",     "M72x3.0",     "M72x4.0",
                "M72x6.0",     "M75x1.5",     "M75x2.0",     "M75x3.0",
                "M75x4.0",     "M75x6.0",     "M76x1.5",     "M76x2.0",
                "M76x3.0",     "M76x4.0",     "M76x6.0",     "M80x1.5",
                "M80x2.0",     "M80x3.0",     "M80x4.0",     "M80x6.0",
                "M85x2.0",     "M85x3.0",     "M85x4.0",     "M85x6.0",
                "M90x2.0",     "M90x3.0",     "M90x4.0",     "M90x6.0",
                "M95x2.0",     "M95x3.0",     "M95x4.0",     "M95x6.0",
                "M100x2.0",    "M100x3.0",    "M100x4.0",    "M100x6.0",
            ],
            'UNC': [
                "#1", "#2", "#3", "#4", "#5", "#6",
                "#8",  "#10", "#12",
                "1/4", "5/16", "3/8", "7/16", "1/2", "9/16",
                "5/8", "3/4", "7/8", "1", "1 1/8", "1 1/4",
                "1 3/8", "1 1/2", "1 3/4", "2", "2 1/4",
                "2 1/2", "2 3/4", "3", "3 1/4", "3 1/2",
                "3 3/4", "4",
            ],
            'UNF': [
                "#0", "#1", "#2", "#3", "#4", "#5", "#6",
                "#8", "#10", "#12",
                "1/4", "5/16", "3/8", "7/16", "1/2", "9/16",
                "5/8", "3/4", "7/8", "1", "1 1/8", "1 3/16", "1 1/4",
                "1 3/8", "1 1/2",
            ],
            'UNEF': [
                "#12", "1/4", "5/16", "3/8", "7/16", "1/2",
                "9/16", "5/8", "11/16", "3/4", "13/16", "7/8",
                "15/16", "1", "1 1/16", "1 1/8", "1 1/4",
                "1 5/16", "1 3/8", "1 7/16", "1 1/2", "1 9/16",
                "1 5/8", "1 11/16",
            ],
            'NPT': [
                "1/16", "1/8", "1/4", "3/8", "1/2", "3/4",
                "1", "1 1/4", "1 1/2",
                "2", "2 1/2",
                "3", "3 1/2",
                "4", "5", "6", "8", "10", "12",
            ],
            'BSP': [
                "1/16", "1/8", "1/4", "3/8", "1/2", "5/8", "3/4", "7/8",
                "1", "1 1/8", "1 1/4", "1 1/2", "1 3/4",
                "2", "2 1/4", "2 1/2", "2 3/4",
                "3", "3 1/2", "4", "4 1/2",
                "5", "5 1/2", "6",
            ],
            'BSW': [
                "1/8", "3/16", "1/4", "5/16", "3/8", "7/16",
                "1/2", "9/16", "5/8", "11/16", "3/4", "7/8",
                "1", "1 1/8", "1 1/4", "1 1/2", "1 3/4",
                "2", "2 1/4", "2 1/2", "2 3/4",
                "3", "3 1/4", "3 1/2", "3 3/4",
                "4", "4 1/2", "5", "5 1/2", "6",
            ],
            'BSF': [
                "3/16", "7/32", "1/4", "9/32", "5/16", "3/8", "7/16",
                "1/2", "9/16", "5/8", "11/16", "3/4", "7/8",
                "1", "1 1/8", "1 1/4", "1 3/8", "1 1/2", "1 5/8", "1 3/4",
                "2", "2 1/4", "2 1/2", "2 3/4",
                "3", "3 1/4", "3 1/2", "3 3/4",
                "4", "4 1/4",
            ],
            'ISOTyre': [
                "5v1", "5v2", "6v1", "8v1", "9v1", "10v2",
                "12v1", "13v1", "8v2", "10v1", "11v1", "13v2",
                "15v1", "16v1", "17v1", "17v2", "17v3", "19v1", "20v1",
            ],
        }
        allowed_types = self.Hole.getEnumerationsOfProperty("ThreadType")
        for type_index, thread_type in enumerate(thread_types.keys(), 1):
            if thread_type not in allowed_types:
                self._helperNotFoundMessage(thread_type, allowed_types)
            # Set by number like the saved files
            self.Hole.ThreadType = type_index
            self._helperNotCorrectMessage(self.Hole.ThreadType, thread_type)

            allowed_sizes = self.Hole.getEnumerationsOfProperty("ThreadSize")
            for size_index, designation in enumerate(thread_types[thread_type]):
                if designation not in allowed_sizes:
                    self._helperNotFoundMessage(designation, allowed_sizes)
                # Set by number like the saved files
                self.Hole.ThreadSize = size_index
                self._helperNotCorrectMessage(self.Hole.ThreadSize, designation)

    def _helperNotCorrectMessage(self, value, comparison):
        self.assertEqual(
            value, comparison,
            f"{comparison} is not in the correct position\n\n"
            "it will break compatibility with older saves"
        )

    def _helperNotFoundMessage(self, prop, allowed_props):
        raise AssertionError(
            "\n"
            f"{prop} is not in {allowed_props}\n\n"
            "Verify that the tested enums names are updated \n\n"
        )

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestHole")
        #print ("omit closing document for debugging")

