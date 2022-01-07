# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathLog as PathLog
import Generators.helix_generator as generator
import PathTests.PathTestUtils as PathTestUtils


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())


def _resetArgs():
    v1 = FreeCAD.Vector(5, 5, 20)
    v2 = FreeCAD.Vector(5, 5, 10)

    edg = Part.makeLine(v1, v2)

    return {
        "edge": edg,
        "hole_radius": 10.0,
        "step_down": 1.0,
        "step_over": 5.0,
        "tool_diameter": 5.0,
        "inner_radius": 0.0,
        "direction": "CW",
        "startAt": "Inside",
    }


class TestPathHelixGenerator(PathTestUtils.PathTestBase):
    expectedHelixGCode = "G0 X12.500000 Y5.000000\
G1 Z20.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z19.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z19.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z18.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z18.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z17.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z17.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z16.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z16.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z15.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z15.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z14.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z14.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z13.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z13.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z12.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z12.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z11.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z11.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z10.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z10.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z10.000000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z10.000000\
G0 X5.000000 Y5.000000 Z20.000000"

    def test00(self):
        """Test Basic Helix Generator Return"""
        args = _resetArgs()
        result = generator.generate(**args)
        self.assertTrue(type(result) is list)
        self.assertTrue(type(result[0]) is Path.Command)

        gcode = "".join([r.toGCode() for r in result])
        self.assertTrue(
            gcode == self.expectedHelixGCode, "Incorrect helix g-code generated"
        )

    def test01(self):
        """Test Basic Helix Generator hole_radius is float > 0"""
        args = _resetArgs()
        args["hole_radius"] = 10
        self.assertRaises(ValueError, generator.generate, **args)

        args["hole_radius"] = -10.0
        self.assertRaises(ValueError, generator.generate, **args)

    def test02(self):
        """Test Basic Helix Generator inner_radius is float"""
        args = _resetArgs()
        args["inner_radius"] = 2
        self.assertRaises(ValueError, generator.generate, **args)

    def test03(self):
        """Test Basic Helix Generator tool_diameter is float"""
        args = _resetArgs()
        args["tool_diameter"] = 5
        self.assertRaises(ValueError, generator.generate, **args)

    def test04(self):
        """Test Basic Helix Generator tool fit with radius difference less than tool diameter"""
        args = _resetArgs()
        # require tool fit 1: radius diff less than tool diam
        args["hole_radius"] = 10.0
        args["inner_radius"] = 6.0
        args["tool_diameter"] = 5.0
        self.assertRaises(ValueError, generator.generate, **args)

        # require tool fit 2: hole radius less than tool diam with zero inner radius
        args["hole_radius"] = 4.5
        args["inner_radius"] = 0.0
        args["tool_diameter"] = 5.0
        self.assertRaises(ValueError, generator.generate, **args)

    def test05(self):
        """Test Basic Helix Generator validate the startAt enumeration value"""
        args = _resetArgs()
        args["startAt"] = "Other"
        self.assertRaises(ValueError, generator.generate, **args)

    def test06(self):
        """Test Basic Helix Generator validate the direction enumeration value"""
        args = _resetArgs()
        args["direction"] = "clock"
        self.assertRaises(ValueError, generator.generate, **args)

    def test07(self):
        """Test Basic Helix Generator verify linear edge is vertical"""
        # verify linear edge is vertical: X
        args = _resetArgs()
        v1 = FreeCAD.Vector(5, 5, 20)
        v2 = FreeCAD.Vector(5.0001, 5, 10)
        edg = Part.makeLine(v1, v2)
        args["edge"] = edg
        self.assertRaises(ValueError, generator.generate, **args)

        # verify linear edge is vertical: Y
        args = _resetArgs()
        v1 = FreeCAD.Vector(5, 5.0001, 20)
        v2 = FreeCAD.Vector(5, 5, 10)
        edg = Part.makeLine(v1, v2)
        args["edge"] = edg
        self.assertRaises(ValueError, generator.generate, **args)

    def test08(self):
        """Test Helix Generator with horizontal edge"""
        args = _resetArgs()
        v1 = FreeCAD.Vector(10, 5, 5)
        v2 = FreeCAD.Vector(20, 5, 5)
        edg = Part.makeLine(v1, v2)
        args["edge"] = edg
        self.assertRaises(ValueError, generator.generate, **args)

    def test09(self):
        """Test Helix Generator with inverted vertical edge"""
        args = _resetArgs()
        v1 = FreeCAD.Vector(5, 5, 10)
        v2 = FreeCAD.Vector(5, 5, 20)
        edg = Part.makeLine(v1, v2)
        args["edge"] = edg

        result = generator.generate(**args)

        self.assertTrue(type(result) is list)
        self.assertTrue(type(result[0]) is Path.Command)

        gcode = "".join([r.toGCode() for r in result])
        self.assertTrue(
            gcode == self.expectedHelixGCode, "Incorrect helix g-code generated"
        )
