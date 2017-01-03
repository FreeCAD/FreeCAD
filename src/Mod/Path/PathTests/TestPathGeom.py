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

    def test50(self):
        """Verify proper wire(s) aggregation from a Path."""
        commands = []
        commands.append(Path.Command('G1', {'X': 1}))
        commands.append(Path.Command('G1', {'Y': 1}))
        commands.append(Path.Command('G0', {'X': 0}))
        commands.append(Path.Command('G1', {'Y': 0}))

        wire = PathGeom.wireForPath(Path.Path(commands))
        self.assertEqual(len(wire.Edges), 4)
        self.assertLine(wire.Edges[0], Vector(0,0,0), Vector(1,0,0))
        self.assertLine(wire.Edges[1], Vector(1,0,0), Vector(1,1,0))
        self.assertLine(wire.Edges[2], Vector(1,1,0), Vector(0,1,0))
        self.assertLine(wire.Edges[3], Vector(0,1,0), Vector(0,0,0))

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
