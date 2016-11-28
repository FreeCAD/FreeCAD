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
from PathScripts.PathDressupHoldingTags import *
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

