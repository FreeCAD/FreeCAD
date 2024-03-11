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
import Path.Base.Generator.helix as generator
import Tests.PathTestUtils as PathTestUtils


Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


def _resetArgs():
    v1 = FreeCAD.Vector(5, 5, 20)
    v2 = FreeCAD.Vector(5, 5, 18)

    edg = Part.makeLine(v1, v2)

    return {
        "edge": edg,
        "hole_radius": 10.0,
        "step_down": 1.0,
        "step_over": 0.5,
        "tool_diameter": 5.0,
        "inner_radius": 0.0,
        "direction": "CW",
        "startAt": "Inside",
    }


class TestPathHelixGenerator(PathTestUtils.PathTestBase):

    expectedHelixGCode = "G0 X7.500000 Y5.000000\
G1 Z20.000000\
G2 I-2.500000 J0.000000 X2.500000 Y5.000000 Z19.500000\
G2 I2.500000 J0.000000 X7.500000 Y5.000000 Z19.000000\
G2 I-2.500000 J0.000000 X2.500000 Y5.000000 Z18.500000\
G2 I2.500000 J0.000000 X7.500000 Y5.000000 Z18.000000\
G2 I-2.500000 J0.000000 X2.500000 Y5.000000 Z18.000000\
G2 I2.500000 J0.000000 X7.500000 Y5.000000 Z18.000000\
G0 X5.000000 Y5.000000 Z18.000000\
G0 Z20.000000G0 X10.000000 Y5.000000\
G1 Z20.000000\
G2 I-5.000000 J0.000000 X0.000000 Y5.000000 Z19.500000\
G2 I5.000000 J0.000000 X10.000000 Y5.000000 Z19.000000\
G2 I-5.000000 J0.000000 X0.000000 Y5.000000 Z18.500000\
G2 I5.000000 J0.000000 X10.000000 Y5.000000 Z18.000000\
G2 I-5.000000 J0.000000 X0.000000 Y5.000000 Z18.000000\
G2 I5.000000 J0.000000 X10.000000 Y5.000000 Z18.000000\
G0 X5.000000 Y5.000000 Z18.000000\
G0 Z20.000000G0 X12.500000 Y5.000000\
G1 Z20.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z19.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z19.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z18.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z18.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z18.000000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z18.000000\
G0 X5.000000 Y5.000000 Z18.000000G0 Z20.000000"

    def test00(self):
        """Test Basic Helix Generator Return"""
        args = _resetArgs()
        result = generator.generate(**args)
        self.assertTrue(type(result) is list)
        self.assertTrue(type(result[0]) is Path.Command)

        gcode = "".join([r.toGCode() for r in result])
        print(gcode)
        self.assertTrue(
            gcode == self.expectedHelixGCode, "Incorrect helix g-code generated"
        )

    def test01(self):
        """Test Value and Type checking"""
        args = _resetArgs()
        args["hole_radius"] = "10"
        self.assertRaises(TypeError, generator.generate, **args)

        args["hole_radius"] = -10.0
        self.assertRaises(ValueError, generator.generate, **args)

        args = _resetArgs()
        args["inner_radius"] = "2"
        self.assertRaises(TypeError, generator.generate, **args)

        args = _resetArgs()
        args["tool_diameter"] = "5"
        self.assertRaises(TypeError, generator.generate, **args)

        # require tool fit 2: hole diameter not greater than tool diam
        # with zero inner radius
        args = _resetArgs()
        args["hole_radius"] = 2.0
        args["inner_radius"] = 0.0
        args["tool_diameter"] = 5.0
        self.assertRaises(ValueError, generator.generate, **args)

        # require tool fit: actual hole diameter after taking Extra Offset into account >= tool diameter
        # 1. Extra Offset just small enough to leave room for tool should not raise an error
        args = _resetArgs()
        designed_hole_diameter = 10.0
        extra_offset = 2.49
        args["hole_radius"] = designed_hole_diameter / 2 - extra_offset
        args["inner_radius"] = extra_offset
        args["tool_diameter"] = 5.0
        result = generator.generate(**args)
        self.assertTrue(result)

        # 2. Extra Offset does not leave room for tool, should raise an error
        args = _resetArgs()
        designed_hole_diameter = 10.0
        extra_offset = 2.50
        args["hole_radius"] = designed_hole_diameter / 2 - extra_offset
        args["inner_radius"] = extra_offset
        args["tool_diameter"] = 5.0
        self.assertRaises(ValueError, generator.generate, **args)

        # step_over is a percent value between 0 and 1
        args = _resetArgs()
        args["step_over"] = 50
        self.assertRaises(ValueError, generator.generate, **args)
        args["step_over"] = "50"
        self.assertRaises(TypeError, generator.generate, **args)

        # Other argument testing
        args = _resetArgs()
        args["startAt"] = "Other"
        self.assertRaises(ValueError, generator.generate, **args)

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
        v1 = FreeCAD.Vector(5, 5, 18)
        v2 = FreeCAD.Vector(5, 5, 20)
        edg = Part.makeLine(v1, v2)
        args["edge"] = edg

        self.assertRaises(ValueError, generator.generate, **args)

    def test10(self):
        """Test Helix Retraction"""

        # if center is clear, the second to last move should be a rapid away
        # from the wall
        args = _resetArgs()
        v1 = FreeCAD.Vector(0, 0, 20)
        v2 = FreeCAD.Vector(0, 0, 18)
        edg = Part.makeLine(v1, v2)
        args["edge"] = edg
        args["inner_radius"] = 0.0
        args["tool_diameter"] = 5.0
        result = generator.generate(**args)
        self.assertTrue(result[-2].Name == "G0")

        # if center is not clear, retraction is one straight up on the last
        # move. the second to last move should be a G2
        args["inner_radius"] = 2.0
        result = generator.generate(**args)
        self.assertTrue(result[-2].Name == "G2")
