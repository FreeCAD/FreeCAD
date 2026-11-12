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

from __future__ import division
from math import pi
import unittest

import FreeCAD
import TestSketcherApp


class TestFillet(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestFillet")

    def _create_box_with_fillet(self):
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 10.00
        box.Width = 10.00
        box.Height = 10.00
        body.addObject(box)
        self.Doc.recompute()

        fillet = self.Doc.addObject("PartDesign::Fillet", "Fillet")
        fillet.Base = (box, ["Edge1"])
        fillet.Radius = 1.0
        body.addObject(fillet)
        self.Doc.recompute()
        self.assertTrue(fillet.isValid())
        return body, box, fillet

    def _find_edge_with_match_count(self, source_shape, target_shape, match_count):
        for index in range(1, source_shape.countElement("Edge") + 1):
            source_name = "Edge" + str(index)
            source_edge = source_shape.getElement(source_name, True)
            matches = target_shape.findSubShapesWithSharedVertex(
                source_edge,
                needName=True,
                checkGeometry=True,
            )
            if len(matches) == match_count:
                return source_name, matches[0][0] if matches else None
        self.skipTest("Test model did not contain a suitable edge")

    def testFilletCubeToSphere(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.Box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        self.Body.addObject(self.Box)
        self.Box.Length = 10.00
        self.Box.Width = 10.00
        self.Box.Height = 10.00
        self.Doc.recompute()
        self.Fillet = self.Doc.addObject("PartDesign::Fillet", "Fillet")
        self.Fillet.Base = (self.Box, ["Face" + str(i + 1) for i in range(6)])
        self.Fillet.Radius = 4.999999
        self.Body.addObject(self.Fillet)
        self.Doc.recompute()
        self.assertAlmostEqual(self.Fillet.Shape.Volume, 4 / 3 * pi * 5**3, places=3)
        # test UseAllEdges property
        self.Fillet.UseAllEdges = True
        self.Fillet.Base = (self.Box, [""])  # no subobjects, should still work
        self.Doc.recompute()
        self.assertAlmostEqual(self.Fillet.Shape.Volume, 4 / 3 * pi * 5**3, places=3)
        self.Fillet.Base = (self.Box, ["Face50"])  # non-existent face, topo naming resilience
        self.Doc.recompute()
        self.assertAlmostEqual(self.Fillet.Shape.Volume, 4 / 3 * pi * 5**3, places=3)
        self.Fillet.UseAllEdges = False
        self.Fillet.Base = (self.Box, ["Face1"])
        self.Doc.recompute()
        self.assertNotAlmostEqual(self.Fillet.Shape.Volume, 4 / 3 * pi * 5**3, places=3)

    def testDeletingPreviousFeatureRelinksUniqueMatchingBaseEdge(self):
        body, box, fillet = self._create_box_with_fillet()
        old_edge, new_edge = self._find_edge_with_match_count(fillet.Shape, box.Shape, 1)

        followup = self.Doc.addObject("PartDesign::Fillet", "FollowupFillet")
        followup.Base = (fillet, [old_edge])
        followup.Radius = 0.25
        body.addObject(followup)
        self.Doc.recompute()
        self.assertTrue(followup.isValid())

        body.removeObject(fillet)

        self.assertEqual(followup.Base[0].Name, box.Name)
        self.assertEqual(list(followup.Base[1]), [new_edge])

    def testDeletingPreviousFeatureLeavesUnmatchedBaseEdgeUnresolved(self):
        body, box, fillet = self._create_box_with_fillet()
        old_edge, _new_edge = self._find_edge_with_match_count(fillet.Shape, box.Shape, 0)

        followup = self.Doc.addObject("PartDesign::Fillet", "FollowupFillet")
        followup.Base = (fillet, [old_edge])
        followup.Radius = 0.25
        body.addObject(followup)
        self.Doc.recompute()

        body.removeObject(fillet)

        self.assertEqual(followup.BaseFeature.Name, box.Name)
        self.assertEqual(followup.Base[0].Name, box.Name)
        self.assertEqual(len(followup.Base[1]), 1)
        self.assertTrue(all(ref.startswith("?") for ref in followup.Base[1]))
        self.Doc.recompute()
        self.assertFalse(followup.isValid())

    def testMovePadAfterFilletLeavesMissingEdgeWithoutDependencyCycle(self):
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = body.newObject("PartDesign::AdditiveBox", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        self.Doc.recompute()

        top_face = max(
            range(1, len(box.Shape.Faces) + 1),
            key=lambda index: box.Shape.Faces[index - 1].CenterOfMass.z,
        )
        sketch = body.newObject("Sketcher::SketchObject", "Sketch")
        sketch.AttachmentSupport = (box, [f"Face{top_face}"])
        sketch.MapMode = "FlatFace"
        TestSketcherApp.CreateRectangleSketch(sketch, (2, 2), (2, 2))
        pad = body.newObject("PartDesign::Pad", "Pad")
        pad.Profile = sketch
        pad.Length = 4
        self.Doc.recompute()
        side_edge = next(
            index
            for index, edge in enumerate(pad.Shape.Edges, 1)
            if edge.BoundBox.ZMin >= 10 - 1e-7
            and edge.BoundBox.ZLength > 1
            and edge.BoundBox.XLength < 1e-7
            and edge.BoundBox.YLength < 1e-7
        )
        fillet = body.newObject("PartDesign::Fillet", "Fillet")
        fillet.Base = (pad, [f"Edge{side_edge}"])
        fillet.Radius = 0.25
        self.Doc.recompute()
        self.assertTrue(fillet.isValid())

        body.removeObject(pad)
        body.insertObject(pad, fillet, True)

        self.assertEqual(fillet.BaseFeature.Name, box.Name)
        self.assertEqual(fillet.Base[0].Name, box.Name)
        self.assertEqual(pad.BaseFeature.Name, fillet.Name)
        self.assertNotIn(pad, fillet.OutList)
        self.assertEqual(len(fillet.Base[1]), 1)
        self.assertTrue(all(ref.startswith("?") for ref in fillet.Base[1]))
        self.Doc.recompute(None, False, True)
        self.assertFalse(fillet.isValid())

    def testInsertPadBeforeFilletPreservesBaseThroughEditAndUndo(self):
        body, box, fillet = self._create_box_with_fillet()
        original_volume = fillet.Shape.Volume

        self.Doc.openTransaction("Insert pad before fillet")
        body.Tip = box
        sketch = body.newObject("Sketcher::SketchObject", "InsertedSketch")
        sketch.Placement.Base = FreeCAD.Vector(0, 0, 2)
        TestSketcherApp.CreateRectangleSketch(sketch, (10, 3), (2, 4))
        pad = body.newObject("PartDesign::Pad", "InsertedPad")
        pad.Profile = sketch
        pad.Length = 4

        self.assertTrue(pad.Shape.isNull())
        self.assertEqual(fillet.BaseFeature.Name, pad.Name)
        self.assertEqual(fillet.Base[0].Name, pad.Name)
        self.Doc.recompute()
        self.assertTrue(pad.isValid())
        self.assertTrue(fillet.isValid())

        fillet.Base = (fillet.Base[0], list(fillet.Base[1]))
        body.Tip = fillet
        self.Doc.recompute()
        self.assertEqual(fillet.BaseFeature.Name, pad.Name)
        self.assertTrue(fillet.isValid())
        self.assertAlmostEqual(body.Shape.BoundBox.XMax, 12)
        self.assertGreater(body.Shape.Volume, original_volume)
        self.Doc.commitTransaction()

        self.Doc.undo()
        self.Doc.recompute()
        self.assertEqual(fillet.BaseFeature.Name, box.Name)
        self.assertEqual(fillet.Base[0].Name, box.Name)
        self.assertAlmostEqual(body.Shape.Volume, original_volume)

        self.Doc.redo()
        self.Doc.recompute()
        restored_pad = self.Doc.getObject("InsertedPad")
        self.assertIsNotNone(restored_pad)
        self.assertEqual(fillet.BaseFeature.Name, restored_pad.Name)
        self.assertEqual(fillet.Base[0].Name, restored_pad.Name)
        self.assertTrue(fillet.isValid())
        self.assertAlmostEqual(body.Shape.BoundBox.XMax, 12)

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument("PartDesignTestFillet")
        # print ("omit closing document for debugging")
