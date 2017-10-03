# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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

import FreeCAD
import Part
import Path
import PathScripts
import math
import unittest

from FreeCAD import Vector
#from PathScripts.PathDressupHoldingTags import *
from PathScripts.PathGeom import PathGeom
from PathTests.PathTestUtils import PathTestBase

class TestPathGeom(PathTestBase):
    """Test Path <-> Wire conversion."""

    def test00(self):
        """Verify getAngle functionality."""
        self.assertRoughly(PathGeom.getAngle(Vector(1, 0, 0)), 0)
        self.assertRoughly(PathGeom.getAngle(Vector(1, 1, 0)), math.pi/4)
        self.assertRoughly(PathGeom.getAngle(Vector(0, 1, 0)), math.pi/2)
        self.assertRoughly(PathGeom.getAngle(Vector(-1, 1, 0)), 3*math.pi/4)
        self.assertRoughly(PathGeom.getAngle(Vector(-1, 0, 0)), math.pi)
        self.assertRoughly(PathGeom.getAngle(Vector(-1, -1, 0)), -3*math.pi/4)
        self.assertRoughly(PathGeom.getAngle(Vector(0, -1, 0)), -math.pi/2)
        self.assertRoughly(PathGeom.getAngle(Vector(1, -1, 0)), -math.pi/4)

    def test01(self):
        """Verify diffAngle functionality."""
        self.assertRoughly(PathGeom.diffAngle(0, +0*math.pi/4, 'CW') / math.pi, 0/4.)
        self.assertRoughly(PathGeom.diffAngle(0, +3*math.pi/4, 'CW') / math.pi, 5/4.)
        self.assertRoughly(PathGeom.diffAngle(0, -3*math.pi/4, 'CW') / math.pi, 3/4.)
        self.assertRoughly(PathGeom.diffAngle(0, +4*math.pi/4, 'CW') / math.pi, 4/4.)
        self.assertRoughly(PathGeom.diffAngle(0, +0*math.pi/4, 'CCW')/ math.pi, 0/4.)
        self.assertRoughly(PathGeom.diffAngle(0, +3*math.pi/4, 'CCW')/ math.pi, 3/4.)
        self.assertRoughly(PathGeom.diffAngle(0, -3*math.pi/4, 'CCW')/ math.pi, 5/4.)
        self.assertRoughly(PathGeom.diffAngle(0, +4*math.pi/4, 'CCW')/ math.pi, 4/4.)

        self.assertRoughly(PathGeom.diffAngle(+math.pi/4, +0*math.pi/4,  'CW') / math.pi, 1/4.)
        self.assertRoughly(PathGeom.diffAngle(+math.pi/4, +3*math.pi/4,  'CW') / math.pi, 6/4.)
        self.assertRoughly(PathGeom.diffAngle(+math.pi/4, -1*math.pi/4,  'CW') / math.pi, 2/4.)
        self.assertRoughly(PathGeom.diffAngle(-math.pi/4, +0*math.pi/4,  'CW') / math.pi, 7/4.)
        self.assertRoughly(PathGeom.diffAngle(-math.pi/4, +3*math.pi/4,  'CW') / math.pi, 4/4.)
        self.assertRoughly(PathGeom.diffAngle(-math.pi/4, -1*math.pi/4,  'CW') / math.pi, 0/4.)

        self.assertRoughly(PathGeom.diffAngle(+math.pi/4, +0*math.pi/4, 'CCW') / math.pi, 7/4.)
        self.assertRoughly(PathGeom.diffAngle(+math.pi/4, +3*math.pi/4, 'CCW') / math.pi, 2/4.)
        self.assertRoughly(PathGeom.diffAngle(+math.pi/4, -1*math.pi/4, 'CCW') / math.pi, 6/4.)
        self.assertRoughly(PathGeom.diffAngle(-math.pi/4, +0*math.pi/4, 'CCW') / math.pi, 1/4.)
        self.assertRoughly(PathGeom.diffAngle(-math.pi/4, +3*math.pi/4, 'CCW') / math.pi, 4/4.)
        self.assertRoughly(PathGeom.diffAngle(-math.pi/4, -1*math.pi/4, 'CCW') / math.pi, 0/4.)

    def test02(self):
        """Verify isVertical/isHorizontal for Vector"""
        self.assertTrue(PathGeom.isVertical(Vector(0, 0, 1)))
        self.assertTrue(PathGeom.isVertical(Vector(0, 0, -1)))
        self.assertFalse(PathGeom.isVertical(Vector(1, 0, 1)))
        self.assertFalse(PathGeom.isVertical(Vector(1, 0, -1)))

        self.assertTrue(PathGeom.isHorizontal(Vector( 1,  0, 0)))
        self.assertTrue(PathGeom.isHorizontal(Vector(-1,  0, 0)))
        self.assertTrue(PathGeom.isHorizontal(Vector( 0,  1, 0)))
        self.assertTrue(PathGeom.isHorizontal(Vector( 0, -1, 0)))
        self.assertTrue(PathGeom.isHorizontal(Vector( 1,  1, 0)))
        self.assertTrue(PathGeom.isHorizontal(Vector(-1,  1, 0)))
        self.assertTrue(PathGeom.isHorizontal(Vector( 1, -1, 0)))
        self.assertTrue(PathGeom.isHorizontal(Vector(-1, -1, 0)))

        self.assertFalse(PathGeom.isHorizontal(Vector(0,  1,  1)))
        self.assertFalse(PathGeom.isHorizontal(Vector(0, -1,  1)))
        self.assertFalse(PathGeom.isHorizontal(Vector(0,  1, -1)))
        self.assertFalse(PathGeom.isHorizontal(Vector(0, -1, -1)))

    def test03(self):
        """Verify isVertical/isHorizontal for Edges"""

        # lines
        self.assertTrue(PathGeom.isVertical(Part.Edge(Part.LineSegment(Vector(-1, -1, -1), Vector(-1, -1, 8)))))
        self.assertFalse(PathGeom.isVertical(Part.Edge(Part.LineSegment(Vector(-1, -1, -1), Vector(1, -1, 8)))))
        self.assertFalse(PathGeom.isVertical(Part.Edge(Part.LineSegment(Vector(-1, -1, -1), Vector(-1, 1, 8)))))

        self.assertTrue(PathGeom.isHorizontal(Part.Edge(Part.LineSegment(Vector(1, -1, -1), Vector(-1, -1, -1)))))
        self.assertTrue(PathGeom.isHorizontal(Part.Edge(Part.LineSegment(Vector(-1, 1, -1), Vector(-1, -1, -1)))))
        self.assertTrue(PathGeom.isHorizontal(Part.Edge(Part.LineSegment(Vector(1, 1, -1), Vector(-1, -1, -1)))))
        self.assertFalse(PathGeom.isHorizontal(Part.Edge(Part.LineSegment(Vector(1, -1, -1), Vector(1, -1, 8)))))
        self.assertFalse(PathGeom.isHorizontal(Part.Edge(Part.LineSegment(Vector(-1, 1, -1), Vector(-1, 1, 8)))))

        # circles
        self.assertTrue(PathGeom.isVertical(Part.Edge(Part.makeCircle(4, Vector(), Vector(0, 1, 0)))))
        self.assertTrue(PathGeom.isVertical(Part.Edge(Part.makeCircle(4, Vector(), Vector(1, 0, 0)))))
        self.assertTrue(PathGeom.isVertical(Part.Edge(Part.makeCircle(4, Vector(), Vector(1, 1, 0)))))
        self.assertFalse(PathGeom.isVertical(Part.Edge(Part.makeCircle(4, Vector(), Vector(1, 1, 1)))))

        self.assertTrue(PathGeom.isHorizontal(Part.Edge(Part.makeCircle(4, Vector(), Vector(0, 0, 1)))))
        self.assertFalse(PathGeom.isHorizontal(Part.Edge(Part.makeCircle(4, Vector(), Vector(0, 1, 1)))))
        self.assertFalse(PathGeom.isHorizontal(Part.Edge(Part.makeCircle(4, Vector(), Vector(1, 0, 1)))))
        self.assertFalse(PathGeom.isHorizontal(Part.Edge(Part.makeCircle(4, Vector(), Vector(1, 1, 1)))))

        # bezier curves
        # ml: I know nothing about bezier curves, so this might be bollocks
        # and now I disable the tests because they seem to fail on OCE 
        #bezier = Part.BezierCurve()
        #bezier.setPoles([Vector(), Vector(1,1,0), Vector(2,1,0), Vector(2,2,0)])
        #self.assertTrue(PathGeom.isHorizontal(Part.Edge(bezier)))
        #self.assertFalse(PathGeom.isVertical(Part.Edge(bezier)))
        #bezier.setPoles([Vector(), Vector(1,1,1), Vector(2,1,0), Vector(2,2,0)])
        #self.assertFalse(PathGeom.isHorizontal(Part.Edge(bezier)))
        #self.assertFalse(PathGeom.isVertical(Part.Edge(bezier)))
        #bezier.setPoles([Vector(), Vector(1,1,0), Vector(2,1,1), Vector(2,2,0)])
        #self.assertFalse(PathGeom.isHorizontal(Part.Edge(bezier)))
        #self.assertFalse(PathGeom.isVertical(Part.Edge(bezier)))
        #bezier.setPoles([Vector(), Vector(1,1,0), Vector(2,1,0), Vector(2,2,1)])
        #self.assertFalse(PathGeom.isHorizontal(Part.Edge(bezier)))
        #self.assertFalse(PathGeom.isVertical(Part.Edge(bezier)))
        #
        #bezier.setPoles([Vector(), Vector(1,1,1), Vector(2,2,2), Vector(0,0,3)])
        #self.assertFalse(PathGeom.isHorizontal(Part.Edge(bezier)))
        #self.assertTrue(PathGeom.isVertical(Part.Edge(bezier)))


    def test04(self):
        """Verify isVertical/isHorizontal for faces"""

        # planes
        xPlane = Part.makePlane(100, 100, FreeCAD.Vector(), FreeCAD.Vector(1, 0, 0))
        yPlane = Part.makePlane(100, 100, FreeCAD.Vector(), FreeCAD.Vector(0, 1, 0))
        zPlane = Part.makePlane(100, 100, FreeCAD.Vector(), FreeCAD.Vector(0, 0, 1))
        xyPlane = Part.makePlane(100, 100, FreeCAD.Vector(), FreeCAD.Vector(1, 1, 0))
        xzPlane = Part.makePlane(100, 100, FreeCAD.Vector(), FreeCAD.Vector(1, 0, 1))
        yzPlane = Part.makePlane(100, 100, FreeCAD.Vector(), FreeCAD.Vector(0, 1, 1))

        self.assertTrue(PathGeom.isVertical(xPlane))
        self.assertTrue(PathGeom.isVertical(yPlane))
        self.assertFalse(PathGeom.isVertical(zPlane))
        self.assertTrue(PathGeom.isVertical(xyPlane))
        self.assertFalse(PathGeom.isVertical(xzPlane))
        self.assertFalse(PathGeom.isVertical(yzPlane))

        self.assertFalse(PathGeom.isHorizontal(xPlane))
        self.assertFalse(PathGeom.isHorizontal(yPlane))
        self.assertTrue(PathGeom.isHorizontal(zPlane))
        self.assertFalse(PathGeom.isHorizontal(xyPlane))
        self.assertFalse(PathGeom.isHorizontal(xzPlane))
        self.assertFalse(PathGeom.isHorizontal(yzPlane))

        # cylinders
        xCylinder = [f for f in Part.makeCylinder(1, 1, FreeCAD.Vector(), FreeCAD.Vector(1, 0, 0)).Faces if type(f.Surface) == Part.Cylinder][0]
        yCylinder = [f for f in Part.makeCylinder(1, 1, FreeCAD.Vector(), FreeCAD.Vector(0, 1, 0)).Faces if type(f.Surface) == Part.Cylinder][0]
        zCylinder = [f for f in Part.makeCylinder(1, 1, FreeCAD.Vector(), FreeCAD.Vector(0, 0, 1)).Faces if type(f.Surface) == Part.Cylinder][0]
        xyCylinder = [f for f in Part.makeCylinder(1, 1, FreeCAD.Vector(), FreeCAD.Vector(1, 1, 0)).Faces if type(f.Surface) == Part.Cylinder][0]
        xzCylinder = [f for f in Part.makeCylinder(1, 1, FreeCAD.Vector(), FreeCAD.Vector(1, 0, 1)).Faces if type(f.Surface) == Part.Cylinder][0]
        yzCylinder = [f for f in Part.makeCylinder(1, 1, FreeCAD.Vector(), FreeCAD.Vector(0, 1, 1)).Faces if type(f.Surface) == Part.Cylinder][0]

        self.assertTrue(PathGeom.isHorizontal(xCylinder))
        self.assertTrue(PathGeom.isHorizontal(yCylinder))
        self.assertFalse(PathGeom.isHorizontal(zCylinder))
        self.assertTrue(PathGeom.isHorizontal(xyCylinder))
        self.assertFalse(PathGeom.isHorizontal(xzCylinder))
        self.assertFalse(PathGeom.isHorizontal(yzCylinder))

    def test10(self):
        """Verify proper geometry objects for G1 and G01 commands are created."""
        spt = Vector(1,2,3)
        self.assertLine(PathGeom.edgeForCmd(Path.Command('G1',  {'X': 7, 'Y': 2, 'Z': 3}), spt), spt, Vector(7, 2, 3))
        self.assertLine(PathGeom.edgeForCmd(Path.Command('G01', {'X': 1, 'Y': 3, 'Z': 5}), spt), spt, Vector(1, 3, 5))

    def test20(self):
        """Verfiy proper geometry for arcs in the XY-plane are created."""
        p1 = Vector(0, -1, 2)
        p2 = Vector(-1, 0, 2)
        self.assertArc(
                PathGeom.edgeForCmd(
                    Path.Command('G2', {'X': p2.x, 'Y': p2.y, 'Z': p2.z, 'I': 0, 'J': 1, 'K': 0}), p1),
                p1, p2, 'CW')
        self.assertArc(
                PathGeom.edgeForCmd(
                    Path.Command('G3', {'X': p1.x, 'Y': p1.y, 'z': p1.z, 'I': -1, 'J': 0, 'K': 0}), p2),
                p2, p1, 'CCW')

    def test30(self):
        """Verify proper geometry for arcs with rising and fall ing Z-axis are created."""
        #print("------ rising helix -------")
        p1 = Vector(0, 1, 0)
        p2 = Vector(1, 0, 2)
        self.assertCurve(
                PathGeom.edgeForCmd(
                    Path.Command('G2', {'X': p2.x, 'Y': p2.y, 'Z': p2.z, 'I': 0, 'J': -1, 'K': 1}), p1),
                p1, Vector(1/math.sqrt(2), 1/math.sqrt(2), 1), p2)
        p1 = Vector(-1, 0, 0)
        p2 = Vector(0, -1, 2)
        self.assertCurve(
                PathGeom.edgeForCmd(
                    Path.Command('G3', {'X': p2.x, 'Y': p2.y, 'Z': p2.z, 'I': 1, 'J': 0, 'K': 1}), p1),
                p1, Vector(-1/math.sqrt(2), -1/math.sqrt(2), 1), p2)

        #print("------ falling helix -------")
        p1 = Vector(0, -1, 2)
        p2 = Vector(-1, 0, 0)
        self.assertCurve(
                PathGeom.edgeForCmd(
                    Path.Command('G2', {'X': p2.x, 'Y': p2.y, 'Z': p2.z, 'I': 0, 'J': 1, 'K': -1}), p1),
                p1, Vector(-1/math.sqrt(2), -1/math.sqrt(2), 1), p2)
        p1 = Vector(-1, 0, 2)
        p2 = Vector(0, -1, 0)
        self.assertCurve(
                PathGeom.edgeForCmd(
                    Path.Command('G3', {'X': p2.x, 'Y': p2.y, 'Z': p2.z, 'I': 1, 'J': 0, 'K': -1}), p1),
                p1, Vector(-1/math.sqrt(2), -1/math.sqrt(2), 1), p2)

    def test40(self):
        """Verify arc results in proper G2/3 command."""
        p1 = Vector(  0, -10, 0)
        p2 = Vector(-10,   0, 0)
        p3 = Vector(  0, +10, 0)
        p4 = Vector(+10,   0, 0)

        def cmds(pa, pb, pc, flip):
            return PathGeom.cmdsForEdge(Part.Edge(Part.Arc(pa, pb, pc)), flip)[0]
        def cmd(c, end, off):
            return Path.Command(c, {'X': end.x, 'Y': end.y, 'Z': end.z, 'I': off.x, 'J': off.y, 'K': off.z})

        self.assertCommandEqual(cmds(p1, p2, p3, False), cmd('G2', p3, Vector(0,  10, 0)))
        self.assertCommandEqual(cmds(p1, p4, p3, False), cmd('G3', p3, Vector(0,  10, 0)))

        self.assertCommandEqual(cmds(p1, p2, p3,  True), cmd('G3', p1, Vector(0, -10, 0)))
        self.assertCommandEqual(cmds(p1, p4, p3,  True), cmd('G2', p1, Vector(0, -10, 0)))


    def test50(self):
        """Verify proper wire(s) aggregation from a Path."""
        commands = []
        commands.append(Path.Command('G1', {'X': 1}))
        commands.append(Path.Command('G1', {'Y': 1}))
        commands.append(Path.Command('G0', {'X': 0}))
        commands.append(Path.Command('G1', {'Y': 0}))

        wire,rapid = PathGeom.wireForPath(Path.Path(commands))
        self.assertEqual(len(wire.Edges), 4)
        self.assertLine(wire.Edges[0], Vector(0,0,0), Vector(1,0,0))
        self.assertLine(wire.Edges[1], Vector(1,0,0), Vector(1,1,0))
        self.assertLine(wire.Edges[2], Vector(1,1,0), Vector(0,1,0))
        self.assertLine(wire.Edges[3], Vector(0,1,0), Vector(0,0,0))
        self.assertEqual(len(rapid), 1)
        self.assertTrue(PathGeom.edgesMatch(rapid[0], wire.Edges[2]))

        wires = PathGeom.wiresForPath(Path.Path(commands))
        self.assertEqual(len(wires), 2)
        self.assertEqual(len(wires[0].Edges), 2)
        self.assertLine(wires[0].Edges[0], Vector(0,0,0), Vector(1,0,0))
        self.assertLine(wires[0].Edges[1], Vector(1,0,0), Vector(1,1,0))
        self.assertEqual(len(wires[1].Edges), 1)
        self.assertLine(wires[1].Edges[0], Vector(0,1,0), Vector(0,0,0))


    def test60(self):
        """Verify arcToHelix returns proper helix."""
        p1 = Vector(10,-10,0)
        p2 = Vector(0,0,0)
        p3 = Vector(10,10,0)

        e = PathGeom.arcToHelix(Part.Edge(Part.Arc(p1, p2, p3)), 0, 2)
        self.assertCurve(e, p1, p2 + Vector(0,0,1), p3 + Vector(0,0,2))

        e = PathGeom.arcToHelix(Part.Edge(Part.Arc(p1, p2, p3)), 3, 7)
        self.assertCurve(e, p1 + Vector(0,0,3), p2 + Vector(0,0,5), p3 + Vector(0,0,7))

        e = PathGeom.arcToHelix(Part.Edge(Part.Arc(p1, p2, p3)), 9, 1)
        self.assertCurve(e, p1 + Vector(0,0,9), p2 + Vector(0,0,5), p3 + Vector(0,0,1))

        dz = Vector(0,0,3)
        p11 = p1 + dz
        p12 = p2 + dz
        p13 = p3 + dz

        e = PathGeom.arcToHelix(Part.Edge(Part.Arc(p11, p12, p13)), 0, 8)
        self.assertCurve(e, p1, p2 + Vector(0,0,4), p3 + Vector(0,0,8))

        e = PathGeom.arcToHelix(Part.Edge(Part.Arc(p11, p12, p13)), 2, -2)
        self.assertCurve(e, p1 + Vector(0,0,2), p2, p3 + Vector(0,0,-2))

        o = 10*math.sin(math.pi/4)
        p1 = Vector(10, -10, 1)
        p2 = Vector(10 - 10*math.sin(math.pi/4), -10*math.cos(math.pi/4), 1)
        p3 = Vector(0, 0, 1)
        e = PathGeom.arcToHelix(Part.Edge(Part.Arc(p1, p2, p3)), 0, 5)
        self.assertCurve(e, Vector(10,-10,0), Vector(p2.x,p2.y,2.5), Vector(0, 0, 5))


    def test62(self):
        """Verify splitArcAt returns proper subarcs."""
        p1 = Vector(10,-10,0)
        p2 = Vector(0,0,0)
        p3 = Vector(10,10,0)

        arc = Part.Edge(Part.Arc(p1, p2, p3))

        o = 10*math.sin(math.pi/4)
        p12 = Vector(10 - o, -o, 0)
        p23 = Vector(10 - o, +o, 0)

        e = PathGeom.splitArcAt(arc, p2)
        self.assertCurve(e[0], p1, p12, p2)
        self.assertCurve(e[1], p2, p23, p3)

        p34 = Vector(10 - 10*math.sin(1*math.pi/8), -10*math.cos(1*math.pi/8), 0)
        p45 = Vector(10 - 10*math.sin(5*math.pi/8), -10*math.cos(5*math.pi/8), 0)

        e = PathGeom.splitArcAt(arc, p12)
        self.assertCurve(e[0], p1, p34, p12)
        self.assertCurve(e[1], p12, p45, p3)


    def test65(self):
        """Verify splitEdgeAt."""
        e = PathGeom.splitEdgeAt(Part.Edge(Part.LineSegment(Vector(), Vector(2, 4, 6))), Vector(1, 2, 3))
        self.assertLine(e[0], Vector(), Vector(1,2,3))
        self.assertLine(e[1], Vector(1,2,3), Vector(2,4,6))

        # split an arc
        p1 = Vector(10,-10,1)
        p2 = Vector(0,0,1)
        p3 = Vector(10,10,1)
        arc = Part.Edge(Part.Arc(p1, p2, p3))
        e = PathGeom.splitEdgeAt(arc, p2)
        o = 10*math.sin(math.pi/4)
        p12 = Vector(10 - o, -o, 1)
        p23 = Vector(10 - o, +o, 1)
        self.assertCurve(e[0], p1, p12, p2)
        self.assertCurve(e[1], p2, p23, p3)


        # split a helix
        p1 = Vector(10,-10,0)
        p2 = Vector(0,0,5)
        p3 = Vector(10,10,10)
        h = PathGeom.arcToHelix(arc, 0, 10)
        self.assertCurve(h, p1, p2, p3)

        e = PathGeom.splitEdgeAt(h, p2)
        o = 10*math.sin(math.pi/4)
        p12 = Vector(10 - o, -o, 2.5)
        p23 = Vector(10 - o, +o, 7.5)
        pf = e[0].valueAt((e[0].FirstParameter + e[0].LastParameter)/2)
        pl = e[1].valueAt((e[1].FirstParameter + e[1].LastParameter)/2)
        self.assertCurve(e[0], p1, p12, p2)
        self.assertCurve(e[1], p2, p23, p3)
