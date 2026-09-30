# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
#   Copyright (c) 2026 FreeCAD Project Association
#
#   This file is part of the FreeCAD CAx development system.
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public License
#   as published by the Free Software Foundation; either version 2.1 of
#   the License, or (at your option) any later version.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#   Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library; see the file COPYING.LIB. If not,
#   write to the Free Software Foundation, Inc., 59 Temple Place,
#   Suite 330, Boston, MA 02111-1307, USA
# ***************************************************************************

import os
import tempfile
import unittest

import FreeCAD as App
import Part


class TestLinkArrayCircular(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestLinkArrayCircular")
        source = self.doc.addObject("Part::Box", "Source")
        self.array = self.doc.addObject("Part::LinkArrayCircular", "Array")
        self.array.LinkedObject = source
        self.array.ShowElement = False
        self.array.RadialDistance = 10
        self.array.TangentialDistance = 10
        self.array.NumberCircles = 3
        self.array.Symmetry = 4

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def testSuppressionProtectsGeneratedElements(self):
        source = self.doc.addObject("Part::Box", "SuppressionSource")
        for kind in ("Polar", "Circular", "Path", "Point"):
            with self.subTest(kind=kind):
                array = self.doc.addObject("Part::LinkArray" + kind, "SuppressionArray")
                array.LinkedObject = source
                if kind in ("Path", "Point"):
                    path = self.doc.addObject("Part::Feature", "SuppressionPath")
                    path.Shape = Part.makePolygon([App.Vector(), App.Vector(20, 0, 0)])
                    setattr(array, "Path" if kind == "Path" else "PointObject", path)
                self.doc.recompute()
                array.ElementList[1].Suppressed = True
                self.doc.recompute()
                with self.assertRaises(AttributeError):
                    array.ShowElement = False
                self.assertTrue(array.ShowElement)
                self.assertTrue(array.ElementList[1].Suppressed)

    def testExcessiveOccurrenceCountIsRejected(self):
        self.array.NumberCircles = 2
        self.array.RadialDistance = 1000000
        self.array.TangentialDistance = 0.001
        self.doc.recompute()
        self.assertIn("10000", self.array.getStatusString())

    def testRingPopulationIsRoundedToSymmetry(self):
        self.assertEqual(self.array.getTypeIdOfProperty("RadialDistance"), "App::PropertyLength")
        self.assertEqual(
            self.array.getTypeIdOfProperty("TangentialDistance"), "App::PropertyLength"
        )
        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
        # floor(2*pi*10/10) -> 6 -> 4, floor(2*pi*20/10) -> 12.
        self.assertEqual(self.array.ElementCount, 1 + 4 + 12)
        self.assertEqual(len(self.array.PlacementList), self.array.ElementCount)
        self.assertEqual(self.array.PlacementList[0], App.Placement())
        self.assertAlmostEqual(self.array.PlacementList[1].Base.Length, 10)
        self.assertAlmostEqual(self.array.PlacementList[5].Base.Length, 20)

    def testAxisReferenceOrientsTheCircles(self):
        axis = self.doc.addObject("Part::Feature", "Axis")
        axis.Shape = Part.makeLine(App.Vector(5, 0, 0), App.Vector(5, 10, 0))
        self.array.Axis = (axis, ["Edge1"])

        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
        for placement in self.array.PlacementList:
            self.assertAlmostEqual(placement.Base.y, 0)

        # The selected edge supplies the center as well as the axis direction.
        self.assertAlmostEqual(self.array.PlacementList[2].Base.x, -5)

    def testSketchEdgeOrientsTheCircles(self):
        # Part2DObject is the base of Sketcher::SketchObject; it must not force
        # an explicit edge reference to use the sketch normal.
        sketch = self.doc.addObject("Part::Part2DObject", "Sketch")
        sketch.Shape = Part.makeLine(App.Vector(5, 0, 0), App.Vector(5, 10, 0))
        self.array.Axis = (sketch, ["Edge1"])
        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
        for placement in self.array.PlacementList:
            self.assertAlmostEqual(placement.Base.y, 0)
        self.assertAlmostEqual(self.array.PlacementList[2].Base.x, -5)

    def testSketchNamedAxesStillOrientTheCircles(self):
        sketch = self.doc.addObject("Part::Part2DObject", "Sketch")
        for axis, coordinate in (("H_Axis", "x"), ("V_Axis", "y"), ("N_Axis", "z")):
            with self.subTest(axis=axis):
                self.array.Axis = (sketch, [axis])
                self.doc.recompute()
                self.assertEqual(self.array.getStatusString(), "Valid")
                for placement in self.array.PlacementList:
                    self.assertAlmostEqual(getattr(placement.Base, coordinate), 0)

    def testObjectAxesSurviveSaveRestore(self):
        source = self.doc.getObject("Source")
        linear = self.doc.addObject("Part::LinkArrayLinear", "Linear")
        polar = self.doc.addObject("Part::LinkArrayPolar", "Polar")
        linear.LinkedObject = source
        polar.LinkedObject = source
        linear.ShowElement = polar.ShowElement = False
        linear.Direction = (None, ["Z_Axis"])
        linear.Direction2 = (None, ["X_Axis"])
        linear.Occurrences2 = 3
        polar.Axis = (None, ["Y_Axis"])
        self.array.Axis = (None, ["X_Axis"])
        self.doc.recompute()
        references = {
            "Linear": {"Direction": linear.Direction, "Direction2": linear.Direction2},
            "Polar": {"Axis": polar.Axis},
            "Array": {"Axis": self.array.Axis},
        }
        placements = {name: list(self.doc.getObject(name).PlacementList) for name in references}
        with tempfile.TemporaryDirectory() as directory:
            filename = os.path.join(directory, "ObjectAxes.FCStd")
            self.doc.saveAs(filename)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(filename)
            self.doc.recompute()
            for name, properties in references.items():
                with self.subTest(array=name):
                    array = self.doc.getObject(name)
                    self.assertEqual(array.getStatusString(), "Valid")
                    for prop, reference in properties.items():
                        self.assertEqual(getattr(array, prop), reference)
                    self.assertEqual(list(array.PlacementList), placements[name])
