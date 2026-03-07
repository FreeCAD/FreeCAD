# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

"""
Testing functions for VCarve operation module
"""

# ***************************************************************************
# *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
import Part
import Path
import Path.Main.Job as PathJob
import Path.Op.Vcarve as PathVcarve
import math
from CAMTests.PathTestUtils import PathTestWithAssets

# pylint: disable=too-few-public-methods, protected-access


class VbitTool:
    """Faked out vcarve tool"""

    def __init__(self, dia, angle, tipDia):
        self.Diameter = FreeCAD.Units.Quantity(dia, FreeCAD.Units.Length)
        self.CuttingEdgeAngle = FreeCAD.Units.Quantity(angle, FreeCAD.Units.Angle)
        self.TipDiameter = FreeCAD.Units.Quantity(tipDia, FreeCAD.Units.Length)


Scale45 = 2.414214
Scale60 = math.sqrt(3)


class TestPathVcarve(PathTestWithAssets):
    """Test Vcarve milling basics."""

    def tearDown(self):
        super().tearDown()
        if hasattr(self, "doc"):
            FreeCAD.closeDocument(self.doc.Name)

    def testFinishingPass(self):
        """Check if enabling finishing pass adds another path with required z-depth"""
        self.doc = FreeCAD.newDocument()
        part1 = FreeCAD.ActiveDocument.addObject("Part::Feature", "TestShape")
        part2 = FreeCAD.ActiveDocument.addObject("Part::Feature", "TestShape")
        rect = Part.makePolygon(
            [(20, 20, 10), (25, 20, 10), (25, 30, 10), (20, 30, 10), (20, 20, 10)]
        )
        box = Part.makeBox(100, 100, 10)
        part1.Shape = box
        part2.Shape = Part.makeFace(rect, "Part::FaceMakerSimple")
        job = PathJob.Create("Job", [part1, part2])

        toolbit = self.assets.get("toolbit://60degree_Vbit")
        loaded_tool = toolbit.attach_to_doc(doc=job.Document)
        job.Tools.Group[0].Tool = loaded_tool

        op = PathVcarve.Create("TestVCarve")
        op.BaseShapes = job.Model.Group[1]

        op.FinishingPass = False
        op.Proxy.execute(op)
        min_z_no_finish = op.Path.BoundBox.ZMin

        finishing_offset = -0.1
        op.FinishingPass = True
        op.FinishingPassZOffset = finishing_offset
        op.Proxy.execute(op)
        min_z_with_finish = op.Path.BoundBox.ZMin

        self.assertRoughly(min_z_with_finish - min_z_no_finish, finishing_offset, 1.0)

    def test00(self):
        """Verify 90 deg depth calculation"""
        tool = VbitTool(10, 90, 0)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -10)
        self.assertRoughly(geom.start, 0)
        self.assertRoughly(geom.stop, -5)
        self.assertRoughly(geom.scale, 1)

    def test01(self):
        """Verify 90 deg depth limit"""
        tool = VbitTool(10, 90, 0)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -3)
        self.assertRoughly(geom.start, 0)
        self.assertRoughly(geom.stop, -3)
        self.assertRoughly(geom.scale, 1)

    def test02(self):
        """Verify 60 deg depth calculation"""
        tool = VbitTool(10, 60, 0)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -10)
        self.assertRoughly(geom.start, 0)
        self.assertRoughly(geom.stop, -5 * Scale60)
        self.assertRoughly(geom.scale, Scale60)

    def test03(self):
        """Verify 60 deg depth limit"""
        tool = VbitTool(10, 60, 0)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -3)
        self.assertRoughly(geom.start, 0)
        self.assertRoughly(geom.stop, -3)
        self.assertRoughly(geom.scale, Scale60)

    def test10(self):
        """Verify 90 deg with tip dia depth calculation"""
        tool = VbitTool(10, 90, 2)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -10)
        # in order for the width to be correct the height needs to be shifted
        self.assertRoughly(geom.start, 1)
        self.assertRoughly(geom.stop, -4)
        self.assertRoughly(geom.scale, 1)

    def test11(self):
        """Verify 90 deg with tip dia depth limit calculation"""
        tool = VbitTool(10, 90, 2)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -3)
        # in order for the width to be correct the height needs to be shifted
        self.assertRoughly(geom.start, 1)
        self.assertRoughly(geom.stop, -3)
        self.assertRoughly(geom.scale, 1)

    def test12(self):
        """Verify 45 deg with tip dia depth calculation"""
        tool = VbitTool(10, 45, 2)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -10)
        # in order for the width to be correct the height needs to be shifted
        self.assertRoughly(geom.start, Scale45)
        self.assertRoughly(geom.stop, -4 * Scale45)
        self.assertRoughly(geom.scale, Scale45)

    def test13(self):
        """Verify 45 deg with tip dia depth limit calculation"""
        tool = VbitTool(10, 45, 2)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -3)
        # in order for the width to be correct the height needs to be shifted
        self.assertRoughly(geom.start, Scale45)
        self.assertRoughly(geom.stop, -3)
        self.assertRoughly(geom.scale, Scale45)

    def test14(self):
        """Verify if max dept is calculated properly when step-down is disabled"""

        tool = VbitTool(10, 45, 2)
        geom = PathVcarve._Geometry.FromTool(tool, zStart=0, zFinal=-3, zStepDown=0)

        self.assertEqual(geom.maximumDepth, -3)
        self.assertEqual(geom.maximumDepth, geom.stop)

    def test15(self):
        """Verify if step-down sections match final max depth"""

        tool = VbitTool(10, 45, 2)
        geom = PathVcarve._Geometry.FromTool(tool, zStart=0, zFinal=-3, zStepDown=0.13)

        while geom.incrementStepDownDepth(maximumUsableDepth=-3):
            pass

        self.assertEqual(geom.maximumDepth, -3)

    def test16(self):
        """Verify 90 deg with tip dia depth calculation with step-down enabled"""
        tool = VbitTool(10, 90, 2)
        geom = PathVcarve._Geometry.FromTool(tool, zStart=0, zFinal=-10, zStepDown=0.13)

        while geom.incrementStepDownDepth(maximumUsableDepth=-10):
            pass

        # in order for the width to be correct the height needs to be shifted
        self.assertRoughly(geom.start, 1)
        self.assertRoughly(geom.stop, -4)
        self.assertRoughly(geom.scale, 1)
        self.assertRoughly(geom.maximumDepth, -4)

    def test17(self):
        """Verify if canSkipRepositioning allows to skip if new point is < 0.5 mm"""

        positionHistory = [
            FreeCAD.Base.Vector(0, 0, 0),  # previous position
            FreeCAD.Base.Vector(0, 1, 5),  # current position
        ]

        newPosition = FreeCAD.Base.Vector(0, 1.4, 3)
        assert PathVcarve.canSkipRepositioning(positionHistory, newPosition, 0.01)

        newPosition = FreeCAD.Base.Vector(0, 1.7, 3)
        assert not PathVcarve.canSkipRepositioning(positionHistory, newPosition, 0.01)

    def test18(self):
        """Verify if canSkipRepositioning allows to skip if new edge ends in current position"""

        defaultTolerance = Path.Preferences.defaultGeometryTolerance()

        positionHistory = [
            FreeCAD.Base.Vector(0, 0, 0),  # previous position
            FreeCAD.Base.Vector(0, 1, 5),  # current position
        ]

        # new position is same as previous position so we can G1 from (0,1,5) to (0,0,0) because
        # we already travelled this path before and it's carved - no need to raise toolbit
        newPosition = FreeCAD.Base.Vector(0, 0, 0)
        assert PathVcarve.canSkipRepositioning(positionHistory, newPosition, defaultTolerance)

        # same but should fail because we are out of tolerance
        newPosition = FreeCAD.Base.Vector(0, 0.1, 0)
        assert not PathVcarve.canSkipRepositioning(positionHistory, newPosition, defaultTolerance)

        # same but is OK because we are within tolerance
        newPosition = FreeCAD.Base.Vector(0, 0.1, 0)
        assert PathVcarve.canSkipRepositioning(positionHistory, newPosition, 0.1)

    def test19(self):
        """Verify virtualBackTrackEdges() various scenarios"""

        defaultTolerance = Path.Preferences.defaultGeometryTolerance()

        # test scenario 1 - refer to function comments for explanation

        positionHistory = [
            FreeCAD.Base.Vector(0, 0, 0),  # previous position
            FreeCAD.Base.Vector(0, 1, 5),  # current position
        ]

        # new edge ends at current position
        newEdge = Part.Edge(
            Part.LineSegment(FreeCAD.Base.Vector(1, 2, 3), FreeCAD.Base.Vector(0, 1, 5))
        )

        virtualEdges = PathVcarve.generateVirtualBackTrackEdges(
            positionHistory, newEdge, defaultTolerance
        )

        assert len(virtualEdges) == 1

        virtualEdge = virtualEdges[0]
        # virtualEdge is essentially a reversed newEdge
        assert virtualEdge.valueAt(virtualEdge.FirstParameter) == newEdge.valueAt(
            newEdge.LastParameter
        )
        assert virtualEdge.valueAt(virtualEdge.LastParameter) == newEdge.valueAt(
            newEdge.FirstParameter
        )

        # test scenario 2 - refer to function comments for explanation

        positionHistory = [
            FreeCAD.Base.Vector(0, 0, 0),  # previous position
            FreeCAD.Base.Vector(0, 1, 5),  # current position
        ]

        # new edge ends at previous position
        newEdge = Part.Edge(
            Part.LineSegment(FreeCAD.Base.Vector(1, 2, 3), FreeCAD.Base.Vector(0, 0, 0))
        )

        virtualEdges = PathVcarve.generateVirtualBackTrackEdges(
            positionHistory, newEdge, defaultTolerance
        )

        assert len(virtualEdges) == 2

        virtualEdge1 = virtualEdges[0]
        virtualEdge2 = virtualEdges[1]

        # 2 virtual edges (current position, previous position) and
        # (previous position, new edge start)

        assert virtualEdge1.valueAt(virtualEdge1.FirstParameter) == positionHistory[-1]
        assert virtualEdge1.valueAt(virtualEdge1.LastParameter) == positionHistory[-2]

        assert virtualEdge2.valueAt(virtualEdge2.FirstParameter) == positionHistory[-2]
        assert virtualEdge2.valueAt(virtualEdge2.LastParameter) == newEdge.valueAt(
            newEdge.FirstParameter
        )
