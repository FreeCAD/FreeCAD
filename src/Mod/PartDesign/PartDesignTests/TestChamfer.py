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

import FreeCAD
import Part
import TestSketcherApp


class TestChamfer(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestChamfer")

    def testChamferCubeToOctahedron(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.Box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        self.Body.addObject(self.Box)
        self.Box.Length = 10.00
        self.Box.Width = 10.00
        self.Box.Height = 10.00
        self.Doc.recompute()
        self.Chamfer = self.Doc.addObject("PartDesign::Chamfer", "Chamfer")
        self.Chamfer.Base = (self.Box, ["Face" + str(i + 1) for i in range(6)])
        self.Chamfer.Size = 4.999999
        self.Body.addObject(self.Chamfer)
        self.Doc.recompute()
        self.MajorFaces = [face for face in self.Chamfer.Shape.Faces if face.Area > 1e-3]
        self.assertEqual(len(self.MajorFaces), 8)
        # test UseAllEdges property
        self.Chamfer.UseAllEdges = True
        self.Chamfer.Base = (self.Box, [""])  # no subobjects, should still work
        self.Doc.recompute()
        self.MajorFaces = [face for face in self.Chamfer.Shape.Faces if face.Area > 1e-3]
        self.assertEqual(len(self.MajorFaces), 8)
        self.Chamfer.Base = (self.Box, ["Face50"])  # non-existent face, test topo naming resilience
        self.Doc.recompute()
        self.MajorFaces = [face for face in self.Chamfer.Shape.Faces if face.Area > 1e-3]
        self.assertEqual(len(self.MajorFaces), 8)
        self.Chamfer.UseAllEdges = False
        self.Chamfer.Base = (self.Box, ["Face1"])
        self.Doc.recompute()
        self.MajorFaces = [face for face in self.Chamfer.Shape.Faces if face.Area > 1e-3]
        self.assertEqual(len(self.MajorFaces), 9)

    def testInsertPadBeforeChamferPreservesBaseThroughEdit(self):
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = body.newObject("PartDesign::AdditiveBox", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        self.Doc.recompute()
        chamfer = body.newObject("PartDesign::Chamfer", "Chamfer")
        chamfer.Base = (box, ["Edge1"])
        chamfer.Size = 1
        self.Doc.recompute()
        self.assertTrue(chamfer.isValid())
        original_volume = chamfer.Shape.Volume

        body.Tip = box
        sketch = body.newObject("Sketcher::SketchObject", "InsertedSketch")
        sketch.Placement.Base = FreeCAD.Vector(0, 0, 2)
        TestSketcherApp.CreateRectangleSketch(sketch, (10, 3), (2, 4))
        pad = body.newObject("PartDesign::Pad", "InsertedPad")
        pad.Profile = sketch
        pad.Length = 4

        self.assertTrue(pad.Shape.isNull())
        self.assertEqual(chamfer.BaseFeature.Name, pad.Name)
        self.assertEqual(chamfer.Base[0].Name, pad.Name)
        self.Doc.recompute()
        self.assertTrue(pad.isValid())
        self.assertTrue(chamfer.isValid())

        chamfer.Base = (chamfer.Base[0], list(chamfer.Base[1]))
        body.Tip = chamfer
        self.Doc.recompute()
        self.assertEqual(chamfer.BaseFeature.Name, pad.Name)
        self.assertTrue(chamfer.isValid())
        self.assertAlmostEqual(body.Shape.BoundBox.XMax, 12)
        self.assertGreater(body.Shape.Volume, original_volume)

    def _create_blade_on_hub(self):
        def bezier(points):
            curve = Part.BezierCurve()
            curve.setPoles([FreeCAD.Vector(x, y, 0) for x, y in points])
            return curve.toShape()

        a, b, c, d = (8.5, 1.25), (50.9, 19.4), (53.1, 18.1), (8.5, -1.25)
        profile = Part.Wire(
            [
                bezier([a, (25.6, 1.0), (41.3, 3.7), b]),
                Part.makeLine(FreeCAD.Vector(*b, 0), FreeCAD.Vector(*c, 0)),
                bezier([c, (42.9, 1.6), (26.6, -1.5), d]),
                Part.makeLine(FreeCAD.Vector(*d, 0), FreeCAD.Vector(*a, 0)),
            ]
        )
        blade = Part.Face(profile).extrude(FreeCAD.Vector(0, 0, 16))
        hub = Part.makeCylinder(9, 22, FreeCAD.Vector(0, 0, -7))
        shape = self.Doc.addObject("Part::Feature", "Shape")
        shape.Shape = hub.fuse(blade).Solids[0]
        body = self.Doc.addObject("PartDesign::Body", "Body")
        body.BaseFeature = shape
        self.Doc.recompute()
        base = body.Group[0]
        root = FreeCAD.BoundBox(8.4, 1.2, 14.9, 8.6, 1.3, 16.1)
        for index, edge in enumerate(base.Shape.Edges):
            if root.isInside(edge.BoundBox):
                return body, base, "Edge" + str(index + 1)
        self.skipTest("Test model did not contain the blade root edge")

    def _tolerances(self, shape):
        return [element.Tolerance for element in shape.Edges + shape.Vertexes]

    def testInvalidChamferKeepsBaseTolerances(self):
        body, base, edge = self._create_blade_on_hub()
        tolerances = self._tolerances(base.Shape)
        chamfer = body.newObject("PartDesign::Chamfer", "Chamfer")
        chamfer.Base = (base, [edge])
        chamfer.Size = 0.5
        self.Doc.recompute()
        self.assertTrue(chamfer.isValid())
        self.assertEqual(self._tolerances(base.Shape), tolerances)

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument("PartDesignTestChamfer")
        # print ("omit closing document for debugging")
