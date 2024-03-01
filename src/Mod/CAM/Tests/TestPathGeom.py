# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

import Part
import Path
import math

from FreeCAD import Vector
from Tests.PathTestUtils import PathTestBase


class TestPathGeom(PathTestBase):
    """Test Path <-> Wire conversion."""

    def test00(self):
        """Verify getAngle functionality."""
        self.assertRoughly(Path.Geom.getAngle(Vector(1, 0, 0)), 0)
        self.assertRoughly(Path.Geom.getAngle(Vector(1, 1, 0)), math.pi / 4)
        self.assertRoughly(Path.Geom.getAngle(Vector(0, 1, 0)), math.pi / 2)
        self.assertRoughly(Path.Geom.getAngle(Vector(-1, 1, 0)), 3 * math.pi / 4)
        self.assertRoughly(Path.Geom.getAngle(Vector(-1, 0, 0)), math.pi)
        self.assertRoughly(Path.Geom.getAngle(Vector(-1, -1, 0)), -3 * math.pi / 4)
        self.assertRoughly(Path.Geom.getAngle(Vector(0, -1, 0)), -math.pi / 2)
        self.assertRoughly(Path.Geom.getAngle(Vector(1, -1, 0)), -math.pi / 4)

    def test01(self):
        """Verify diffAngle functionality."""
        self.assertRoughly(
            Path.Geom.diffAngle(0, +0 * math.pi / 4, "CW") / math.pi, 0 / 4.0
        )
        self.assertRoughly(
            Path.Geom.diffAngle(0, +3 * math.pi / 4, "CW") / math.pi, 5 / 4.0
        )
        self.assertRoughly(
            Path.Geom.diffAngle(0, -3 * math.pi / 4, "CW") / math.pi, 3 / 4.0
        )
        self.assertRoughly(
            Path.Geom.diffAngle(0, +4 * math.pi / 4, "CW") / math.pi, 4 / 4.0
        )
        self.assertRoughly(
            Path.Geom.diffAngle(0, +0 * math.pi / 4, "CCW") / math.pi, 0 / 4.0
        )
        self.assertRoughly(
            Path.Geom.diffAngle(0, +3 * math.pi / 4, "CCW") / math.pi, 3 / 4.0
        )
        self.assertRoughly(
            Path.Geom.diffAngle(0, -3 * math.pi / 4, "CCW") / math.pi, 5 / 4.0
        )
        self.assertRoughly(
            Path.Geom.diffAngle(0, +4 * math.pi / 4, "CCW") / math.pi, 4 / 4.0
        )

        self.assertRoughly(
            Path.Geom.diffAngle(+math.pi / 4, +0 * math.pi / 4, "CW") / math.pi, 1 / 4.0
        )
        self.assertRoughly(
            Path.Geom.diffAngle(+math.pi / 4, +3 * math.pi / 4, "CW") / math.pi, 6 / 4.0
        )
        self.assertRoughly(
            Path.Geom.diffAngle(+math.pi / 4, -1 * math.pi / 4, "CW") / math.pi, 2 / 4.0
        )
        self.assertRoughly(
            Path.Geom.diffAngle(-math.pi / 4, +0 * math.pi / 4, "CW") / math.pi, 7 / 4.0
        )
        self.assertRoughly(
            Path.Geom.diffAngle(-math.pi / 4, +3 * math.pi / 4, "CW") / math.pi, 4 / 4.0
        )
        self.assertRoughly(
            Path.Geom.diffAngle(-math.pi / 4, -1 * math.pi / 4, "CW") / math.pi, 0 / 4.0
        )

        self.assertRoughly(
            Path.Geom.diffAngle(+math.pi / 4, +0 * math.pi / 4, "CCW") / math.pi,
            7 / 4.0,
        )
        self.assertRoughly(
            Path.Geom.diffAngle(+math.pi / 4, +3 * math.pi / 4, "CCW") / math.pi,
            2 / 4.0,
        )
        self.assertRoughly(
            Path.Geom.diffAngle(+math.pi / 4, -1 * math.pi / 4, "CCW") / math.pi,
            6 / 4.0,
        )
        self.assertRoughly(
            Path.Geom.diffAngle(-math.pi / 4, +0 * math.pi / 4, "CCW") / math.pi,
            1 / 4.0,
        )
        self.assertRoughly(
            Path.Geom.diffAngle(-math.pi / 4, +3 * math.pi / 4, "CCW") / math.pi,
            4 / 4.0,
        )
        self.assertRoughly(
            Path.Geom.diffAngle(-math.pi / 4, -1 * math.pi / 4, "CCW") / math.pi,
            0 / 4.0,
        )

    def test02(self):
        """Verify isVertical/isHorizontal for Vector"""
        self.assertTrue(Path.Geom.isVertical(Vector(0, 0, 1)))
        self.assertTrue(Path.Geom.isVertical(Vector(0, 0, -1)))
        self.assertFalse(Path.Geom.isVertical(Vector(1, 0, 1)))
        self.assertFalse(Path.Geom.isVertical(Vector(1, 0, -1)))

        self.assertTrue(Path.Geom.isHorizontal(Vector(1, 0, 0)))
        self.assertTrue(Path.Geom.isHorizontal(Vector(-1, 0, 0)))
        self.assertTrue(Path.Geom.isHorizontal(Vector(0, 1, 0)))
        self.assertTrue(Path.Geom.isHorizontal(Vector(0, -1, 0)))
        self.assertTrue(Path.Geom.isHorizontal(Vector(1, 1, 0)))
        self.assertTrue(Path.Geom.isHorizontal(Vector(-1, 1, 0)))
        self.assertTrue(Path.Geom.isHorizontal(Vector(1, -1, 0)))
        self.assertTrue(Path.Geom.isHorizontal(Vector(-1, -1, 0)))

        self.assertFalse(Path.Geom.isHorizontal(Vector(0, 1, 1)))
        self.assertFalse(Path.Geom.isHorizontal(Vector(0, -1, 1)))
        self.assertFalse(Path.Geom.isHorizontal(Vector(0, 1, -1)))
        self.assertFalse(Path.Geom.isHorizontal(Vector(0, -1, -1)))

    def test03(self):
        """Verify isVertical/isHorizontal for Edges"""

        # lines
        self.assertTrue(
            Path.Geom.isVertical(
                Part.Edge(Part.LineSegment(Vector(-1, -1, -1), Vector(-1, -1, 8)))
            )
        )
        self.assertFalse(
            Path.Geom.isVertical(
                Part.Edge(Part.LineSegment(Vector(-1, -1, -1), Vector(1, -1, 8)))
            )
        )
        self.assertFalse(
            Path.Geom.isVertical(
                Part.Edge(Part.LineSegment(Vector(-1, -1, -1), Vector(-1, 1, 8)))
            )
        )

        self.assertTrue(
            Path.Geom.isHorizontal(
                Part.Edge(Part.LineSegment(Vector(1, -1, -1), Vector(-1, -1, -1)))
            )
        )
        self.assertTrue(
            Path.Geom.isHorizontal(
                Part.Edge(Part.LineSegment(Vector(-1, 1, -1), Vector(-1, -1, -1)))
            )
        )
        self.assertTrue(
            Path.Geom.isHorizontal(
                Part.Edge(Part.LineSegment(Vector(1, 1, -1), Vector(-1, -1, -1)))
            )
        )
        self.assertFalse(
            Path.Geom.isHorizontal(
                Part.Edge(Part.LineSegment(Vector(1, -1, -1), Vector(1, -1, 8)))
            )
        )
        self.assertFalse(
            Path.Geom.isHorizontal(
                Part.Edge(Part.LineSegment(Vector(-1, 1, -1), Vector(-1, 1, 8)))
            )
        )

        # circles
        self.assertTrue(
            Path.Geom.isVertical(
                Part.Edge(Part.makeCircle(4, Vector(), Vector(0, 1, 0)))
            )
        )
        self.assertTrue(
            Path.Geom.isVertical(
                Part.Edge(Part.makeCircle(4, Vector(), Vector(1, 0, 0)))
            )
        )
        self.assertTrue(
            Path.Geom.isVertical(
                Part.Edge(Part.makeCircle(4, Vector(), Vector(1, 1, 0)))
            )
        )
        self.assertFalse(
            Path.Geom.isVertical(
                Part.Edge(Part.makeCircle(4, Vector(), Vector(1, 1, 1)))
            )
        )

        self.assertTrue(
            Path.Geom.isHorizontal(
                Part.Edge(Part.makeCircle(4, Vector(), Vector(0, 0, 1)))
            )
        )
        self.assertFalse(
            Path.Geom.isHorizontal(
                Part.Edge(Part.makeCircle(4, Vector(), Vector(0, 1, 1)))
            )
        )
        self.assertFalse(
            Path.Geom.isHorizontal(
                Part.Edge(Part.makeCircle(4, Vector(), Vector(1, 0, 1)))
            )
        )
        self.assertFalse(
            Path.Geom.isHorizontal(
                Part.Edge(Part.makeCircle(4, Vector(), Vector(1, 1, 1)))
            )
        )

        # bezier curves
        # ml: I know nothing about bezier curves, so this might be bollocks
        # and now I disable the tests because they seem to fail on OCE
        # bezier = Part.BezierCurve()
        # bezier.setPoles([Vector(), Vector(1,1,0), Vector(2,1,0), Vector(2,2,0)])
        # self.assertTrue(Path.Geom.isHorizontal(Part.Edge(bezier)))
        # self.assertFalse(Path.Geom.isVertical(Part.Edge(bezier)))
        # bezier.setPoles([Vector(), Vector(1,1,1), Vector(2,1,0), Vector(2,2,0)])
        # self.assertFalse(Path.Geom.isHorizontal(Part.Edge(bezier)))
        # self.assertFalse(Path.Geom.isVertical(Part.Edge(bezier)))
        # bezier.setPoles([Vector(), Vector(1,1,0), Vector(2,1,1), Vector(2,2,0)])
        # self.assertFalse(Path.Geom.isHorizontal(Part.Edge(bezier)))
        # self.assertFalse(Path.Geom.isVertical(Part.Edge(bezier)))
        # bezier.setPoles([Vector(), Vector(1,1,0), Vector(2,1,0), Vector(2,2,1)])
        # self.assertFalse(Path.Geom.isHorizontal(Part.Edge(bezier)))
        # self.assertFalse(Path.Geom.isVertical(Part.Edge(bezier)))
        #
        # bezier.setPoles([Vector(), Vector(1,1,1), Vector(2,2,2), Vector(0,0,3)])
        # self.assertFalse(Path.Geom.isHorizontal(Part.Edge(bezier)))
        # self.assertTrue(Path.Geom.isVertical(Part.Edge(bezier)))

    def test04(self):
        """Verify isVertical/isHorizontal for faces"""

        # planes
        xPlane = Part.makePlane(100, 100, Vector(), Vector(1, 0, 0))
        yPlane = Part.makePlane(100, 100, Vector(), Vector(0, 1, 0))
        zPlane = Part.makePlane(100, 100, Vector(), Vector(0, 0, 1))
        xyPlane = Part.makePlane(100, 100, Vector(), Vector(1, 1, 0))
        xzPlane = Part.makePlane(100, 100, Vector(), Vector(1, 0, 1))
        yzPlane = Part.makePlane(100, 100, Vector(), Vector(0, 1, 1))

        self.assertTrue(Path.Geom.isVertical(xPlane))
        self.assertTrue(Path.Geom.isVertical(yPlane))
        self.assertFalse(Path.Geom.isVertical(zPlane))
        self.assertTrue(Path.Geom.isVertical(xyPlane))
        self.assertFalse(Path.Geom.isVertical(xzPlane))
        self.assertFalse(Path.Geom.isVertical(yzPlane))

        self.assertFalse(Path.Geom.isHorizontal(xPlane))
        self.assertFalse(Path.Geom.isHorizontal(yPlane))
        self.assertTrue(Path.Geom.isHorizontal(zPlane))
        self.assertFalse(Path.Geom.isHorizontal(xyPlane))
        self.assertFalse(Path.Geom.isHorizontal(xzPlane))
        self.assertFalse(Path.Geom.isHorizontal(yzPlane))

        # cylinders
        xCylinder = [
            f
            for f in Part.makeCylinder(1, 1, Vector(), Vector(1, 0, 0)).Faces
            if type(f.Surface) == Part.Cylinder
        ][0]
        yCylinder = [
            f
            for f in Part.makeCylinder(1, 1, Vector(), Vector(0, 1, 0)).Faces
            if type(f.Surface) == Part.Cylinder
        ][0]
        zCylinder = [
            f
            for f in Part.makeCylinder(1, 1, Vector(), Vector(0, 0, 1)).Faces
            if type(f.Surface) == Part.Cylinder
        ][0]
        xyCylinder = [
            f
            for f in Part.makeCylinder(1, 1, Vector(), Vector(1, 1, 0)).Faces
            if type(f.Surface) == Part.Cylinder
        ][0]
        xzCylinder = [
            f
            for f in Part.makeCylinder(1, 1, Vector(), Vector(1, 0, 1)).Faces
            if type(f.Surface) == Part.Cylinder
        ][0]
        yzCylinder = [
            f
            for f in Part.makeCylinder(1, 1, Vector(), Vector(0, 1, 1)).Faces
            if type(f.Surface) == Part.Cylinder
        ][0]

        self.assertTrue(Path.Geom.isHorizontal(xCylinder))
        self.assertTrue(Path.Geom.isHorizontal(yCylinder))
        self.assertFalse(Path.Geom.isHorizontal(zCylinder))
        self.assertTrue(Path.Geom.isHorizontal(xyCylinder))
        self.assertFalse(Path.Geom.isHorizontal(xzCylinder))
        self.assertFalse(Path.Geom.isHorizontal(yzCylinder))

    def test07(self):
        """Verify speed interpolation works for different pitches"""
        # horizontal
        self.assertRoughly(
            100, Path.Geom.speedBetweenPoints(Vector(), Vector(1, 1, 0), 100, 50)
        )
        self.assertRoughly(
            100, Path.Geom.speedBetweenPoints(Vector(1, 1, 0), Vector(), 100, 50)
        )
        # vertical
        self.assertRoughly(
            50, Path.Geom.speedBetweenPoints(Vector(), Vector(0, 0, 1), 100, 50)
        )
        self.assertRoughly(
            50, Path.Geom.speedBetweenPoints(Vector(0, 0, 1), Vector(), 100, 50)
        )
        # 45°
        self.assertRoughly(
            75, Path.Geom.speedBetweenPoints(Vector(), Vector(1, 0, 1), 100, 50)
        )
        self.assertRoughly(
            75, Path.Geom.speedBetweenPoints(Vector(), Vector(0, 1, 1), 100, 50)
        )
        self.assertRoughly(
            75,
            Path.Geom.speedBetweenPoints(Vector(), Vector(0.707, 0.707, 1), 100, 50),
            0.01,
        )
        self.assertRoughly(
            75, Path.Geom.speedBetweenPoints(Vector(1, 0, 1), Vector(), 100, 50)
        )
        self.assertRoughly(
            75, Path.Geom.speedBetweenPoints(Vector(0, 1, 1), Vector(), 100, 50)
        )
        self.assertRoughly(
            75,
            Path.Geom.speedBetweenPoints(Vector(0.707, 0.707, 1), Vector(), 100, 50),
            0.01,
        )
        # 30°
        self.assertRoughly(
            66.66,
            Path.Geom.speedBetweenPoints(Vector(), Vector(0.5774, 0, 1), 100, 50),
            0.01,
        )
        self.assertRoughly(
            66.66,
            Path.Geom.speedBetweenPoints(Vector(), Vector(0, 0.5774, 1), 100, 50),
            0.01,
        )
        self.assertRoughly(
            66.66,
            Path.Geom.speedBetweenPoints(Vector(0.5774, 0, 1), Vector(), 100, 50),
            0.01,
        )
        self.assertRoughly(
            66.66,
            Path.Geom.speedBetweenPoints(Vector(0, 0.5774, 1), Vector(), 100, 50),
            0.01,
        )
        # 60°
        self.assertRoughly(
            83.33,
            Path.Geom.speedBetweenPoints(Vector(), Vector(1, 0, 0.5774), 100, 50),
            0.01,
        )
        self.assertRoughly(
            83.33,
            Path.Geom.speedBetweenPoints(Vector(), Vector(0, 1, 0.5774), 100, 50),
            0.01,
        )
        self.assertRoughly(
            83.33,
            Path.Geom.speedBetweenPoints(Vector(1, 0, 0.5774), Vector(), 100, 50),
            0.01,
        )
        self.assertRoughly(
            83.33,
            Path.Geom.speedBetweenPoints(Vector(0, 1, 0.5774), Vector(), 100, 50),
            0.01,
        )

    def test08(self):
        """Verify speed interpolation works for different pitches if vSpeed > hSpeed"""
        # horizontal
        self.assertRoughly(
            50, Path.Geom.speedBetweenPoints(Vector(), Vector(1, 1, 0), 50, 100)
        )
        self.assertRoughly(
            50, Path.Geom.speedBetweenPoints(Vector(1, 1, 0), Vector(), 50, 100)
        )
        # vertical
        self.assertRoughly(
            100, Path.Geom.speedBetweenPoints(Vector(), Vector(0, 0, 1), 50, 100)
        )
        self.assertRoughly(
            100, Path.Geom.speedBetweenPoints(Vector(0, 0, 1), Vector(), 50, 100)
        )
        # 45°
        self.assertRoughly(
            75, Path.Geom.speedBetweenPoints(Vector(), Vector(1, 0, 1), 50, 100)
        )
        self.assertRoughly(
            75, Path.Geom.speedBetweenPoints(Vector(), Vector(0, 1, 1), 50, 100)
        )
        self.assertRoughly(
            75,
            Path.Geom.speedBetweenPoints(Vector(), Vector(0.707, 0.707, 1), 50, 100),
            0.01,
        )
        self.assertRoughly(
            75, Path.Geom.speedBetweenPoints(Vector(1, 0, 1), Vector(), 50, 100)
        )
        self.assertRoughly(
            75, Path.Geom.speedBetweenPoints(Vector(0, 1, 1), Vector(), 50, 100)
        )
        self.assertRoughly(
            75,
            Path.Geom.speedBetweenPoints(Vector(0.707, 0.707, 1), Vector(), 50, 100),
            0.01,
        )
        # 30°
        self.assertRoughly(
            83.33,
            Path.Geom.speedBetweenPoints(Vector(), Vector(0.5774, 0, 1), 50, 100),
            0.01,
        )
        self.assertRoughly(
            83.33,
            Path.Geom.speedBetweenPoints(Vector(), Vector(0, 0.5774, 1), 50, 100),
            0.01,
        )
        self.assertRoughly(
            83.33,
            Path.Geom.speedBetweenPoints(Vector(0.5774, 0, 1), Vector(), 50, 100),
            0.01,
        )
        self.assertRoughly(
            83.33,
            Path.Geom.speedBetweenPoints(Vector(0, 0.5774, 1), Vector(), 50, 100),
            0.01,
        )
        # 60°
        self.assertRoughly(
            66.66,
            Path.Geom.speedBetweenPoints(Vector(), Vector(1, 0, 0.5774), 50, 100),
            0.01,
        )
        self.assertRoughly(
            66.66,
            Path.Geom.speedBetweenPoints(Vector(), Vector(0, 1, 0.5774), 50, 100),
            0.01,
        )
        self.assertRoughly(
            66.66,
            Path.Geom.speedBetweenPoints(Vector(1, 0, 0.5774), Vector(), 50, 100),
            0.01,
        )
        self.assertRoughly(
            66.66,
            Path.Geom.speedBetweenPoints(Vector(0, 1, 0.5774), Vector(), 50, 100),
            0.01,
        )

    def test10(self):
        """Verify proper geometry objects for G1 and G01 commands are created."""
        spt = Vector(1, 2, 3)
        self.assertLine(
            Path.Geom.edgeForCmd(Path.Command("G1", {"X": 7, "Y": 2, "Z": 3}), spt),
            spt,
            Vector(7, 2, 3),
        )
        self.assertLine(
            Path.Geom.edgeForCmd(Path.Command("G01", {"X": 1, "Y": 3, "Z": 5}), spt),
            spt,
            Vector(1, 3, 5),
        )

    def test20(self):
        """Verify proper geometry for arcs in the XY-plane are created."""
        p1 = Vector(0, -1, 2)
        p2 = Vector(-1, 0, 2)
        self.assertArc(
            Path.Geom.edgeForCmd(
                Path.Command(
                    "G2", {"X": p2.x, "Y": p2.y, "Z": p2.z, "I": 0, "J": 1, "K": 0}
                ),
                p1,
            ),
            p1,
            p2,
            "CW",
        )
        self.assertArc(
            Path.Geom.edgeForCmd(
                Path.Command(
                    "G3", {"X": p1.x, "Y": p1.y, "z": p1.z, "I": -1, "J": 0, "K": 0}
                ),
                p2,
            ),
            p2,
            p1,
            "CCW",
        )

    def test30(self):
        """Verify proper geometry for arcs with rising and fall ing Z-axis are created."""
        # print("------ rising helix -------")
        p1 = Vector(0, 1, 0)
        p2 = Vector(1, 0, 2)
        self.assertCurve(
            Path.Geom.edgeForCmd(
                Path.Command(
                    "G2", {"X": p2.x, "Y": p2.y, "Z": p2.z, "I": 0, "J": -1, "K": 1}
                ),
                p1,
            ),
            p1,
            Vector(1 / math.sqrt(2), 1 / math.sqrt(2), 1),
            p2,
        )
        p1 = Vector(-1, 0, 0)
        p2 = Vector(0, -1, 2)
        self.assertCurve(
            Path.Geom.edgeForCmd(
                Path.Command(
                    "G3", {"X": p2.x, "Y": p2.y, "Z": p2.z, "I": 1, "J": 0, "K": 1}
                ),
                p1,
            ),
            p1,
            Vector(-1 / math.sqrt(2), -1 / math.sqrt(2), 1),
            p2,
        )

        # print("------ falling helix -------")
        p1 = Vector(0, -1, 2)
        p2 = Vector(-1, 0, 0)
        self.assertCurve(
            Path.Geom.edgeForCmd(
                Path.Command(
                    "G2", {"X": p2.x, "Y": p2.y, "Z": p2.z, "I": 0, "J": 1, "K": -1}
                ),
                p1,
            ),
            p1,
            Vector(-1 / math.sqrt(2), -1 / math.sqrt(2), 1),
            p2,
        )
        p1 = Vector(-1, 0, 2)
        p2 = Vector(0, -1, 0)
        self.assertCurve(
            Path.Geom.edgeForCmd(
                Path.Command(
                    "G3", {"X": p2.x, "Y": p2.y, "Z": p2.z, "I": 1, "J": 0, "K": -1}
                ),
                p1,
            ),
            p1,
            Vector(-1 / math.sqrt(2), -1 / math.sqrt(2), 1),
            p2,
        )

    def test40(self):
        """Verify arc results in proper G2/3 command."""
        p1 = Vector(0, -10, 0)
        p2 = Vector(-10, 0, 0)
        p3 = Vector(0, +10, 0)
        p4 = Vector(+10, 0, 0)

        def cmds(pa, pb, pc, flip):
            return Path.Geom.cmdsForEdge(Part.Edge(Part.Arc(pa, pb, pc)), flip)[0]

        def cmd(g, end, off):
            return Path.Command(
                g,
                {
                    "X": end.x,
                    "Y": end.y,
                    "Z": end.z,
                    "I": off.x,
                    "J": off.y,
                    "K": off.z,
                },
            )

        self.assertCommandEqual(
            cmds(p1, p2, p3, False), cmd("G2", p3, Vector(0, 10, 0))
        )
        self.assertCommandEqual(
            cmds(p1, p4, p3, False), cmd("G3", p3, Vector(0, 10, 0))
        )

        self.assertCommandEqual(
            cmds(p1, p2, p3, True), cmd("G3", p1, Vector(0, -10, 0))
        )
        self.assertCommandEqual(
            cmds(p1, p4, p3, True), cmd("G2", p1, Vector(0, -10, 0))
        )

    def test41(self):
        """Verify circle results in proper G2/G3 commands."""

        def cmds(center, radius, up=True):
            norm = Vector(0, 0, 1) if up else Vector(0, 0, -1)
            return Path.Geom.cmdsForEdge(Part.Edge(Part.Circle(center, norm, radius)))[
                0
            ]

        def cmd(g, end, off):
            return Path.Command(
                g,
                {
                    "X": end.x,
                    "Y": end.y,
                    "Z": end.z,
                    "I": off.x,
                    "J": off.y,
                    "K": off.z,
                },
            )

        center = Vector(10, 10, 0)
        radius = 5

        self.assertCommandEqual(
            cmds(center, radius), cmd("G3", Vector(15, 10, 0), Vector(-5, 0, 0))
        )

    def test42(self):
        """Verify ellipsis results in a proper segmentation of G1 commands."""
        ellipse = Part.Edge(Part.Ellipse())
        cmds = Path.Geom.cmdsForEdge(ellipse)
        # let's make sure all commands are G1 and there are more than 20 of those
        self.assertGreater(len(cmds), 20)
        self.assertTrue(all([cmd.Name == "G1" for cmd in cmds]))

    def test50(self):
        """Verify proper wire(s) aggregation from a Path."""
        commands = []
        commands.append(Path.Command("G1", {"X": 1}))
        commands.append(Path.Command("G1", {"Y": 1}))
        commands.append(Path.Command("G0", {"X": 0}))
        commands.append(Path.Command("G1", {"Y": 0}))

        wire, rapid = Path.Geom.wireForPath(Path.Path(commands))
        self.assertEqual(len(wire.Edges), 4)
        self.assertLine(wire.Edges[0], Vector(0, 0, 0), Vector(1, 0, 0))
        self.assertLine(wire.Edges[1], Vector(1, 0, 0), Vector(1, 1, 0))
        self.assertLine(wire.Edges[2], Vector(1, 1, 0), Vector(0, 1, 0))
        self.assertLine(wire.Edges[3], Vector(0, 1, 0), Vector(0, 0, 0))
        self.assertEqual(len(rapid), 1)
        self.assertTrue(Path.Geom.edgesMatch(rapid[0], wire.Edges[2]))

        wires = Path.Geom.wiresForPath(Path.Path(commands))
        self.assertEqual(len(wires), 2)
        self.assertEqual(len(wires[0].Edges), 2)
        self.assertLine(wires[0].Edges[0], Vector(0, 0, 0), Vector(1, 0, 0))
        self.assertLine(wires[0].Edges[1], Vector(1, 0, 0), Vector(1, 1, 0))
        self.assertEqual(len(wires[1].Edges), 1)
        self.assertLine(wires[1].Edges[0], Vector(0, 1, 0), Vector(0, 0, 0))

    def test60(self):
        """Verify arcToHelix returns proper helix."""
        p1 = Vector(10, -10, 0)
        p2 = Vector(0, 0, 0)
        p3 = Vector(10, 10, 0)

        e = Path.Geom.arcToHelix(Part.Edge(Part.Arc(p1, p2, p3)), 0, 2)
        self.assertCurve(e, p1, p2 + Vector(0, 0, 1), p3 + Vector(0, 0, 2))

        e = Path.Geom.arcToHelix(Part.Edge(Part.Arc(p1, p2, p3)), 3, 7)
        self.assertCurve(
            e, p1 + Vector(0, 0, 3), p2 + Vector(0, 0, 5), p3 + Vector(0, 0, 7)
        )

        e = Path.Geom.arcToHelix(Part.Edge(Part.Arc(p1, p2, p3)), 9, 1)
        self.assertCurve(
            e, p1 + Vector(0, 0, 9), p2 + Vector(0, 0, 5), p3 + Vector(0, 0, 1)
        )

        dz = Vector(0, 0, 3)
        p11 = p1 + dz
        p12 = p2 + dz
        p13 = p3 + dz

        e = Path.Geom.arcToHelix(Part.Edge(Part.Arc(p11, p12, p13)), 0, 8)
        self.assertCurve(e, p1, p2 + Vector(0, 0, 4), p3 + Vector(0, 0, 8))

        e = Path.Geom.arcToHelix(Part.Edge(Part.Arc(p11, p12, p13)), 2, -2)
        self.assertCurve(e, p1 + Vector(0, 0, 2), p2, p3 + Vector(0, 0, -2))

        p1 = Vector(10, -10, 1)
        p2 = Vector(10 - 10 * math.sin(math.pi / 4), -10 * math.cos(math.pi / 4), 1)
        p3 = Vector(0, 0, 1)
        e = Path.Geom.arcToHelix(Part.Edge(Part.Arc(p1, p2, p3)), 0, 5)
        self.assertCurve(
            e, Vector(10, -10, 0), Vector(p2.x, p2.y, 2.5), Vector(0, 0, 5)
        )

    def test62(self):
        """Verify splitArcAt returns proper subarcs."""
        p1 = Vector(10, -10, 0)
        p2 = Vector(0, 0, 0)
        p3 = Vector(10, 10, 0)

        arc = Part.Edge(Part.Arc(p1, p2, p3))

        o = 10 * math.sin(math.pi / 4)
        p12 = Vector(10 - o, -o, 0)
        p23 = Vector(10 - o, +o, 0)

        e = Path.Geom.splitArcAt(arc, p2)
        self.assertCurve(e[0], p1, p12, p2)
        self.assertCurve(e[1], p2, p23, p3)

        p34 = Vector(
            10 - 10 * math.sin(1 * math.pi / 8), -10 * math.cos(1 * math.pi / 8), 0
        )
        p45 = Vector(
            10 - 10 * math.sin(5 * math.pi / 8), -10 * math.cos(5 * math.pi / 8), 0
        )

        e = Path.Geom.splitArcAt(arc, p12)
        self.assertCurve(e[0], p1, p34, p12)
        self.assertCurve(e[1], p12, p45, p3)

    def test65(self):
        """Verify splitEdgeAt."""

        # split a line segment
        e = Path.Geom.splitEdgeAt(
            Part.Edge(Part.LineSegment(Vector(), Vector(2, 4, 6))), Vector(1, 2, 3)
        )
        self.assertLine(e[0], Vector(), Vector(1, 2, 3))
        self.assertLine(e[1], Vector(1, 2, 3), Vector(2, 4, 6))

        # split an arc
        p1 = Vector(10, -10, 1)
        p2 = Vector(0, 0, 1)
        p3 = Vector(10, 10, 1)
        arc = Part.Edge(Part.Arc(p1, p2, p3))
        e = Path.Geom.splitEdgeAt(arc, p2)
        o = 10 * math.sin(math.pi / 4)
        p12 = Vector(10 - o, -o, 1)
        p23 = Vector(10 - o, +o, 1)
        self.assertCurve(e[0], p1, p12, p2)
        self.assertCurve(e[1], p2, p23, p3)

        # split a helix
        p1 = Vector(10, -10, 0)
        p2 = Vector(0, 0, 5)
        p3 = Vector(10, 10, 10)
        h = Path.Geom.arcToHelix(arc, 0, 10)
        self.assertCurve(h, p1, p2, p3)

        e = Path.Geom.splitEdgeAt(h, p2)
        o = 10 * math.sin(math.pi / 4)
        p12 = Vector(10 - o, -o, 2.5)
        p23 = Vector(10 - o, +o, 7.5)
        self.assertCurve(e[0], p1, p12, p2)
        self.assertCurve(e[1], p2, p23, p3)

    def test66(self):
        """Split arc real world sample"""

        af = Vector(421.55, 378.41, 1)
        am = Vector(459.51, 372.61, 1)
        al = Vector(491.75, 351.75, 1)
        arc = Part.Edge(Part.ArcOfCircle(af, am, al))
        ac = arc.Curve.Center

        s = Vector(434.54, 378.26, 1)
        head, tail = Path.Geom.splitEdgeAt(arc, s)

        # make sure the arcs connect as they should
        self.assertCoincide(
            arc.valueAt(arc.FirstParameter), head.valueAt(head.FirstParameter), 0.005
        )
        self.assertCoincide(s, head.valueAt(head.LastParameter), 0.005)
        self.assertCoincide(s, tail.valueAt(tail.FirstParameter), 0.005)
        i = arc.valueAt(arc.LastParameter)
        j = tail.valueAt(tail.LastParameter)
        print(
            "(%.2f, %.2f, %.2f) vs. (%.2f, %.2f, %.2f)" % (i.x, i.y, i.z, j.x, j.y, j.z)
        )
        self.assertCoincide(
            arc.valueAt(arc.LastParameter), tail.valueAt(tail.LastParameter), 0.005
        )

        # make sure the radii match
        self.assertRoughly(arc.Curve.Radius, head.Curve.Radius, 0.001)
        self.assertRoughly(arc.Curve.Radius, tail.Curve.Radius, 0.001)

        # also, all arcs should have the same center
        self.assertCoincide(arc.Curve.Center, head.Curve.Center, 0.001)
        self.assertCoincide(arc.Curve.Center, tail.Curve.Center, 0.001)

    def test70(self):
        """Flip a line."""
        edge = Part.Edge(Part.Line(Vector(0, 0, 0), Vector(3, 2, 1)))
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))
        edge = Part.Edge(Part.Line(Vector(0, 0, 0), Vector(-3, -2, -1)))
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

    def test71(self):
        """Flip a line segment."""
        edge = Part.Edge(Part.LineSegment(Vector(0, 0, 0), Vector(3, 2, 1)))
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))
        edge = Part.Edge(Part.LineSegment(Vector(4, 2, 1), Vector(-3, -7, 9)))
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

        edge = Part.makeLine(Vector(1, 0, 3), Vector(3, 2, 1))
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

    def test72(self):
        """Flip a circle"""
        edge = Part.makeCircle(3, Vector(1, 3, 2), Vector(0, 0, 1))
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

        edge = Part.makeCircle(3, Vector(1, 3, 2), Vector(0, 0, -1))
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

    def test73(self):
        """Flip an arc"""
        # make sure all 4 quadrants work
        edge = Part.makeCircle(3, Vector(1, 3, 2), Vector(0, 0, 1), 45, 90)
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

        edge = Part.makeCircle(3, Vector(1, 3, 2), Vector(0, 0, 1), 100, 170)
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

        edge = Part.makeCircle(3, Vector(1, 3, 2), Vector(0, 0, 1), 200, 250)
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

        edge = Part.makeCircle(3, Vector(1, 3, 2), Vector(0, 0, 1), 300, 340)
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

        # and the other way around too
        edge = Part.makeCircle(3, Vector(1, 3, 2), Vector(0, 0, -1), 45, 90)
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))
        edge = Part.makeCircle(3, Vector(1, 3, 2), Vector(0, 0, -1), 100, 170)
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))
        edge = Part.makeCircle(3, Vector(1, 3, 2), Vector(0, 0, -1), 200, 250)
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))
        edge = Part.makeCircle(3, Vector(1, 3, 2), Vector(0, 0, -1), 300, 340)
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

    def test74(self):
        """Flip a rotated arc"""
        # oh yes ...
        edge = Part.makeCircle(3, Vector(1, 3, 2), Vector(0, 0, 1), 45, 90)
        edge.rotate(edge.Curve.Center, Vector(0, 0, 1), -90)
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

    def test75(self):
        """Flip a B-spline"""
        spline = Part.BSplineCurve()
        spline.interpolate(
            [Vector(1, 2, 3), Vector(-3, 0, 7), Vector(-3, 1, 9), Vector(1, 3, 5)]
        )
        edge = Part.Edge(spline)
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

        edge = Part.Edge(spline, 4, 12)
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

        edge = Part.Edge(
            Part.BSplineCurve(
                [
                    Vector(-8, 4, 0),
                    Vector(1, -5, 0),
                    Vector(5, 11, 0),
                    Vector(12, -5, 0),
                ],
                weights=[2, 3, 5, 7],
            )
        )
        self.assertEdgeShapesMatch(edge, Path.Geom.flipEdge(edge))

    def test76(self):
        """Flip an offset wire"""

        e0 = Part.Edge(
            Part.BSplineCurve(
                [
                    Vector(-8, 4, 0),
                    Vector(1, -5, 0),
                    Vector(5, 11, 0),
                    Vector(12, -5, 0),
                ],
                weights=[2, 3, 5, 7],
            )
        )
        e1 = Part.Edge(Part.LineSegment(Vector(12, -5, 0), Vector(0, -7, 0)))
        e2 = Part.Edge(Part.LineSegment(Vector(0, -7, 0), Vector(-8, 4, 0)))
        w0 = Part.Wire([e0, e1, e2])
        w1 = w0.makeOffset2D(1)
        w2 = Path.Geom.flipWire(w1)
        # do some sanity checks
        self.assertTrue(w2.isValid())
        self.assertTrue(w2.isClosed())
