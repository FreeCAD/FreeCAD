# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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
import Path.Base.Generator.threadmilling as threadmilling
import math

from Tests.PathTestUtils import PathTestBase


def radii(internal, major, minor, toolDia, toolCrest):
    """test radii function for simple testing"""
    return (minor, major)


class TestPathThreadMillingGenerator(PathTestBase):
    """Test thread milling generator."""

    def test00(self):
        """Verify thread commands for a single thread"""

        center = FreeCAD.Vector()
        cmd = "G2"
        zStart = 0
        zFinal = 1
        pitch = 1
        radius = 3
        leadInOut = False
        elevator = 2

        path, start = threadmilling.generate(
            center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None
        )

        gcode = [
            "G0 X0.000000 Y2.000000",
            "G0 Z0.000000",
            "G1 Y3.000000",
            "G2 J-3.000000 Y-3.000000 Z0.500000",
            "G2 J3.000000 Y3.000000 Z1.000000",
            "G1 X0.000000 Y2.000000",
        ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(0, 2, zFinal))

    def test01(self):
        """Verify thread commands for a thwo threads"""

        center = FreeCAD.Vector()
        cmd = "G2"
        zStart = 0
        zFinal = 2
        pitch = 1
        radius = 3
        leadInOut = False
        elevator = 2

        path, start = threadmilling.generate(
            center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None
        )

        gcode = [
            "G0 X0.000000 Y2.000000",
            "G0 Z0.000000",
            "G1 Y3.000000",
            "G2 J-3.000000 Y-3.000000 Z0.500000",
            "G2 J3.000000 Y3.000000 Z1.000000",
            "G2 J-3.000000 Y-3.000000 Z1.500000",
            "G2 J3.000000 Y3.000000 Z2.000000",
            "G1 X0.000000 Y2.000000",
        ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(0, 2, zFinal))

    def test02(self):
        """Verify thread commands for a one and a half threads"""

        center = FreeCAD.Vector()
        cmd = "G2"
        zStart = 0
        zFinal = 1.5
        pitch = 1
        radius = 3
        leadInOut = False
        elevator = 2

        path, start = threadmilling.generate(
            center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None
        )

        gcode = [
            "G0 X0.000000 Y2.000000",
            "G0 Z0.000000",
            "G1 Y3.000000",
            "G2 J-3.000000 Y-3.000000 Z0.500000",
            "G2 J3.000000 Y3.000000 Z1.000000",
            "G2 J-3.000000 Y-3.000000 Z1.500000",
            "G1 X0.000000 Y-2.000000",
        ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(0, -2, zFinal))

    def test03(self):
        """Verify thread commands for a one and 3 quarter threads"""

        center = FreeCAD.Vector()
        cmd = "G2"
        zStart = 0
        zFinal = 1.75
        pitch = 1
        radius = 3
        leadInOut = False
        elevator = 2

        path, start = threadmilling.generate(
            center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None
        )

        gcode = [
            "G0 X0.000000 Y2.000000",
            "G0 Z0.000000",
            "G1 Y3.000000",
            "G2 J-3.000000 Y-3.000000 Z0.500000",
            "G2 J3.000000 Y3.000000 Z1.000000",
            "G2 J-3.000000 Y-3.000000 Z1.500000",
            #'(------- finish-thread -------)',
            "G2 J3.000000 X-3.000000 Y0.000000 Z1.750000",
            #'(------- finish-thread -------)',
            "G1 X-2.000000 Y0.000000",
        ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(-2, 0, zFinal))

    def test04(self):
        """Verify thread commands for a one and 3 quarter threads - CCW"""

        center = FreeCAD.Vector()
        cmd = "G3"
        zStart = 0
        zFinal = 1.75
        pitch = 1
        radius = 3
        leadInOut = False
        elevator = 2

        path, start = threadmilling.generate(
            center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None
        )

        gcode = [
            "G0 X0.000000 Y2.000000",
            "G0 Z0.000000",
            "G1 Y3.000000",
            "G3 J-3.000000 Y-3.000000 Z0.500000",
            "G3 J3.000000 Y3.000000 Z1.000000",
            "G3 J-3.000000 Y-3.000000 Z1.500000",
            #'(------- finish-thread -------)',
            "G3 J3.000000 X3.000000 Y0.000000 Z1.750000",
            #'(------- finish-thread -------)',
            "G1 X2.000000 Y0.000000",
        ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(2, 0, zFinal))

    def test10(self):
        """Verify lead in/out commands for a single thread"""

        center = FreeCAD.Vector()
        cmd = "G2"
        zStart = 0
        zFinal = 1
        pitch = 1
        radius = 3
        leadInOut = True
        elevator = 2

        path, start = threadmilling.generate(
            center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None
        )

        gcode = [
            "G0 X0.000000 Y2.000000",
            "G0 Z0.000000",
            #'(------- lead-in -------)',
            "G2 J0.500000 Y3.000000",
            #'(------- lead-in -------)',
            "G2 J-3.000000 Y-3.000000 Z0.500000",
            "G2 J3.000000 Y3.000000 Z1.000000",
            #'(------- lead-out -------)',
            "G2 I0.000000 J-0.500000 X0.000000 Y2.000000",
            #'(------- lead-out -------)',
        ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(0, 2, zFinal))

    def test11(self):
        """Verify lead in/out commands for one and a half threads"""

        center = FreeCAD.Vector()
        cmd = "G2"
        zStart = 0
        zFinal = 1.5
        pitch = 1
        radius = 3
        leadInOut = True
        elevator = 2

        path, start = threadmilling.generate(
            center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None
        )

        gcode = [
            "G0 X0.000000 Y2.000000",
            "G0 Z0.000000",
            #'(------- lead-in -------)',
            "G2 J0.500000 Y3.000000",
            #'(------- lead-in -------)',
            "G2 J-3.000000 Y-3.000000 Z0.500000",
            "G2 J3.000000 Y3.000000 Z1.000000",
            "G2 J-3.000000 Y-3.000000 Z1.500000",
            #'(------- lead-out -------)',
            "G2 I0.000000 J0.500000 X0.000000 Y-2.000000",
            #'(------- lead-out -------)',
        ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(0, -2, zFinal))
