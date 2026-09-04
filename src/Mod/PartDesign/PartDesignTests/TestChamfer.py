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
from pathlib import Path

import FreeCAD
import Part

FIXTURE_PATH = Path(__file__).parent / "Fixtures" / "issue_32231_fillets.FCStd"


class TestChamfer(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestChamfer")

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
        self.assertGreater(changed_below, 1e-5, name)

    def _find_edge(self, shape, first_point, last_point):
        target = (FreeCAD.Vector(*first_point), FreeCAD.Vector(*last_point))
        for index, edge in enumerate(shape.Edges, 1):
            points = [vertex.Point for vertex in edge.Vertexes]
            if len(points) != 2:
                continue
            direct = (points[0] - target[0]).Length + (points[1] - target[1]).Length
            reverse = (points[0] - target[1]).Length + (points[1] - target[0]).Length
            if min(direct, reverse) < 1e-6:
                return "Edge" + str(index)
        raise AssertionError("Expected endpoint edge was not found")

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

    def testIssue32231ChamferChainPreservesValidTopology(self):
        """Both chained endpoint chamfers must remain valid and stop at z=0."""
        FreeCAD.closeDocument(self.Doc.Name)
        self.Doc = FreeCAD.openDocument(str(FIXTURE_PATH))
        source = self.Doc.getObject("Fillet001")
        source.touch()
        self.Doc.recompute()
        source_shape = source.Shape.copy(noElementMap=True)
        FreeCAD.closeDocument(self.Doc.Name)
        self.Doc = FreeCAD.newDocument("Issue32231ChamferChain")

        body = self.Doc.addObject("PartDesign::Body", "Body")
        base = self.Doc.addObject("PartDesign::Feature", "Base")
        body.addObject(base)
        base.Shape = source_shape
        first = self.Doc.addObject("PartDesign::Chamfer", "Chamfer")
        body.addObject(first)
        first.Base = (base, ["Edge23"])
        first.Size = 0.4
        self.Doc.recompute()

        upper_region = Part.makeBox(100, 100, 20, FreeCAD.Vector(-50, -50, 1e-6))
        lower_region = Part.makeBox(100, 100, 50, FreeCAD.Vector(-50, -50, -50))
        self._assert_dress_up_stops_at_z0(
            base.Shape, first.Shape, upper_region, lower_region, first.Name
        )

        second = self.Doc.addObject("PartDesign::Chamfer", "Chamfer001")
        body.addObject(second)
        second.Base = (
            first,
            [self._find_edge(first.Shape, (0, -33.4, -1.45), (0, -33.4, 0))],
        )
        second.Size = 0.4
        self.Doc.recompute()

        self._assert_dress_up_stops_at_z0(
            first.Shape, second.Shape, upper_region, lower_region, second.Name
        )

    def testIssue32231ChamfersStopAtPlanarEndpoint(self):
        """Every chamfer mode must preserve the base beyond a transverse endpoint plane."""
        FreeCAD.closeDocument(self.Doc.Name)
        fixture = FreeCAD.openDocument(str(FIXTURE_PATH))
        source_shape = fixture.getObject("Fillet").Shape.copy(noElementMap=True)
        FreeCAD.closeDocument(fixture.Name)
        self.Doc = FreeCAD.newDocument("Issue32231EndpointChamfers")

        upper_region = Part.makeBox(200, 200, 100, FreeCAD.Vector(-100, -100, 1e-6))
        lower_region = Part.makeBox(200, 200, 100, FreeCAD.Vector(-100, -100, -100))
        cases = (
            ("EqualTiny", "Equal distance", 0.02, 0.02, 45.0, False),
            ("EqualSmall", "Equal distance", 0.075, 0.075, 45.0, False),
            ("EqualUser", "Equal distance", 1.0, 1.0, 45.0, False),
            ("TwoDistances", "Two distances", 0.7, 0.35, 45.0, False),
            ("DistanceAngle", "Distance and Angle", 0.7, 1.0, 30.0, False),
            ("EqualFlipped", "Equal distance", 0.7, 0.7, 45.0, True),
        )
        for case_name, chamfer_type, size, size2, angle, flipped in cases:
            body = self.Doc.addObject("PartDesign::Body", case_name + "Body")
            base = self.Doc.addObject("PartDesign::Feature", case_name + "Base")
            body.addObject(base)
            base.Shape = source_shape
            chamfer = self.Doc.addObject("PartDesign::Chamfer", case_name)
            body.addObject(chamfer)
            chamfer.Base = (base, ["Edge6"])
            chamfer.ChamferType = chamfer_type
            chamfer.Size = size
            chamfer.Size2 = size2
            chamfer.Angle = angle
            chamfer.FlipDirection = flipped
            self.Doc.recompute()

            self._assert_dress_up_stops_at_z0(
                base.Shape,
                chamfer.Shape,
                upper_region,
                lower_region,
                case_name,
            )

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument(self.Doc.Name)
        # print ("omit closing document for debugging")
