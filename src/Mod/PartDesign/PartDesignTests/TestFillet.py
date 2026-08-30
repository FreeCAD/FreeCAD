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
from pathlib import Path
import unittest

import FreeCAD
import Part

FIXTURE_PATH = Path(__file__).parent / "Fixtures" / "issue_32231_fillets.FCStd"


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

    def _assert_dress_up_stops_at_z0(self, source, result, upper_region, lower_region, name):
        self.assertFalse(result.isNull(), name)
        self.assertTrue(result.isValid(), name)

        source_upper = source.common(upper_region)
        result_upper = result.common(upper_region)
        changed_above = source_upper.cut(result_upper).Volume
        changed_above += result_upper.cut(source_upper).Volume
        self.assertAlmostEqual(changed_above, 0.0, places=7, msg=name)

        source_lower = source.common(lower_region)
        result_lower = result.common(lower_region)
        changed_below = source_lower.cut(result_lower).Volume
        changed_below += result_lower.cut(source_lower).Volume
        self.assertGreater(changed_below, 1e-7, name)
        return changed_below

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

    def testDeletingPreviousFeatureDoesNotRelinkUnsafeBaseEdge(self):
        body, box, fillet = self._create_box_with_fillet()
        old_edge, _new_edge = self._find_edge_with_match_count(fillet.Shape, box.Shape, 0)

        followup = self.Doc.addObject("PartDesign::Fillet", "FollowupFillet")
        followup.Base = (fillet, [old_edge])
        followup.Radius = 0.25
        body.addObject(followup)
        self.Doc.recompute()

        body.removeObject(fillet)

        if followup.Base[0]:
            self.assertNotEqual(followup.Base[0].Name, box.Name)

    def testIssue32231FilletChainPreservesValidTopology(self):
        """Chained fillets must remain valid and stop at transverse planar endpoints."""
        FreeCAD.closeDocument(self.Doc.Name)
        self.Doc = FreeCAD.openDocument(str(FIXTURE_PATH))

        upper_region = Part.makeBox(100, 100, 20, FreeCAD.Vector(-50, -50, 1e-6))
        lower_region = Part.makeBox(100, 100, 50, FreeCAD.Vector(-50, -50, -50))

        for feature_name in ("Fillet001", "Fillet002", "Fillet003"):
            feature = self.Doc.getObject(feature_name)
            feature.touch()
            self.Doc.recompute()
            self.assertTrue(feature.Shape.isValid(), feature_name)

            if feature_name != "Fillet001":
                self._assert_dress_up_stops_at_z0(
                    feature.Base[0].Shape,
                    feature.Shape,
                    upper_region,
                    lower_region,
                    feature_name,
                )

    def testIssue32231EndpointFilletMatrix(self):
        """Endpoint clipping must work on either axis, together, and across usable radii."""
        FreeCAD.closeDocument(self.Doc.Name)
        self.Doc = FreeCAD.openDocument(str(FIXTURE_PATH))
        source = self.Doc.getObject("Fillet001")
        source.touch()
        self.Doc.recompute()
        source_shape = source.Shape.copy(noElementMap=True)
        FreeCAD.closeDocument(self.Doc.Name)
        self.Doc = FreeCAD.newDocument("Issue32231EndpointFillets")

        upper_region = Part.makeBox(100, 100, 20, FreeCAD.Vector(-50, -50, 1e-6))
        lower_region = Part.makeBox(100, 100, 50, FreeCAD.Vector(-50, -50, -50))
        changes = {}
        for radius in (0.08, 0.1, 0.2, 0.4, 0.475):
            for axis, edge_names in (
                ("X", ["Edge23"]),
                ("Y", ["Edge5"]),
                ("XY", ["Edge23", "Edge5"]),
            ):
                case_name = "Fillet{}_{:03d}".format(axis, round(radius * 1000))
                body = self.Doc.addObject("PartDesign::Body", case_name + "Body")
                base = self.Doc.addObject("PartDesign::Feature", case_name + "Base")
                body.addObject(base)
                base.Shape = source_shape
                fillet = self.Doc.addObject("PartDesign::Fillet", case_name)
                body.addObject(fillet)
                fillet.Base = (base, edge_names)
                fillet.Radius = radius
                self.Doc.recompute()
                changes[(radius, axis)] = self._assert_dress_up_stops_at_z0(
                    base.Shape,
                    fillet.Shape,
                    upper_region,
                    lower_region,
                    case_name,
                )

        for radius in (0.1, 0.2, 0.4):
            self.assertAlmostEqual(changes[(radius, "X")], changes[(radius, "Y")], delta=1e-6)
            self.assertAlmostEqual(
                changes[(radius, "XY")],
                changes[(radius, "X")] + changes[(radius, "Y")],
                delta=1e-6,
            )

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument(self.Doc.Name)
        # print ("omit closing document for debugging")
