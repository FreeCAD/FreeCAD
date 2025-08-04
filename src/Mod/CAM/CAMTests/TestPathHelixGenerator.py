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
import CAMTests.PathTestUtils as PathTestUtils
import math  # for one use of pi

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


def _resetArgs():
    v1 = FreeCAD.Vector(5, 5, 20)
    v2 = FreeCAD.Vector(5, 5, 18)

    return {
        "edge": Part.makeLine(v1, v2),
        "hole_radius": 10.0,
        "step_down": 1.0,
        "step_over": 0.5,
        "tool_diameter": 5.0,
        "inner_radius": 0.0,
        "direction": "CW",
        "startAt": "Inside",
        "feedRateAdj": False,
    }


class TestPathHelixGenerator(PathTestUtils.PathTestBase):

    # DR values pass helix descent rate to blend v,h feeds. Don't appear in o/p
    expectedHelixGCode = "G0 X7.500000 Y5.000000\
G1 Z20.000000\
G2 DR0.063662 I-2.500000 J0.000000 X2.500000 Y5.000000 Z19.500000\
G2 DR0.063662 I2.500000 J0.000000 X7.500000 Y5.000000 Z19.000000\
G2 DR0.063662 I-2.500000 J0.000000 X2.500000 Y5.000000 Z18.500000\
G2 DR0.063662 I2.500000 J0.000000 X7.500000 Y5.000000 Z18.000000\
G2 DR0.000000 I-2.500000 J0.000000 X2.500000 Y5.000000 Z18.000000\
G2 DR0.000000 I2.500000 J0.000000 X7.500000 Y5.000000 Z18.000000\
G1 X5.000000 Y5.000000\
G0 Z20.000000\
G0 X10.000000 Y5.000000\
G1 Z20.000000\
G2 DR0.031831 I-5.000000 J0.000000 X0.000000 Y5.000000 Z19.500000\
G2 DR0.031831 I5.000000 J0.000000 X10.000000 Y5.000000 Z19.000000\
G2 DR0.031831 I-5.000000 J0.000000 X0.000000 Y5.000000 Z18.500000\
G2 DR0.031831 I5.000000 J0.000000 X10.000000 Y5.000000 Z18.000000\
G2 DR0.000000 I-5.000000 J0.000000 X0.000000 Y5.000000 Z18.000000\
G2 DR0.000000 I5.000000 J0.000000 X10.000000 Y5.000000 Z18.000000\
G1 X8.750000 Y5.000000G0 Z20.000000\
G0 X12.500000 Y5.000000\
G1 Z20.000000\
G2 DR0.021221 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z19.500000\
G2 DR0.021221 I7.500000 J0.000000 X12.500000 Y5.000000 Z19.000000\
G2 DR0.021221 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z18.500000\
G2 DR0.021221 I7.500000 J0.000000 X12.500000 Y5.000000 Z18.000000\
G2 DR0.000000 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z18.000000\
G2 DR0.000000 I7.500000 J0.000000 X12.500000 Y5.000000 Z18.000000\
G1 X11.250000 Y5.000000\
G0 Z20.000000"

    def test00(self):
        """Test Basic Helix Generator Return"""
        args = _resetArgs()
        result = generator.generate(**args)
        self.assertTrue(type(result) is list)
        self.assertTrue(type(result[0]) is Path.Command)

        gcode = "".join([r.toGCode() for r in result])
        print(gcode)
        self.assertTrue(gcode == self.expectedHelixGCode, "Incorrect helix g-code generated")

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

    def test02(self):
        """Require tool fit: hole diameter not greater than tool diam
        # with zero inner radius"""
        args = _resetArgs()
        args["hole_radius"] = 2.0
        args["inner_radius"] = 0.0
        args["tool_diameter"] = 5.0
        self.assertRaises(ValueError, generator.generate, **args)

    def test03(self):
        """Require tool fits hole"""
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
        extra_offset = 2.501
        args["hole_radius"] = designed_hole_diameter / 2 - extra_offset
        args["inner_radius"] = extra_offset
        args["tool_diameter"] = 5.0
        self.assertRaises(ValueError, generator.generate, **args)

        # 3. Check plunge with endmill is not blocked. Not ideal but that's machinists choice.
        args["hole_radius"] = 5.0
        args["inner_radius"] = 0.0
        args["tool_diameter"] = 5.0
        result = generator.generate(**args)
        self.assertTrue(result)

    def test04(self):
        """Test step_over : a "percent" value between 0 and 1"""
        args = _resetArgs()
        args["step_over"] = 50
        self.assertRaises(ValueError, generator.generate, **args)
        args["step_over"] = "50"
        self.assertRaises(TypeError, generator.generate, **args)

    def test05(self):
        """Test some mistaken inputs are rejected"""
        args = _resetArgs()
        args["startAt"] = "Other"
        self.assertRaises(ValueError, generator.generate, **args)

        args = _resetArgs()
        args["direction"] = "clock"
        self.assertRaises(ValueError, generator.generate, **args)

    def test07(self):
        """Test Basic Helix Generator"""
        # verify linear edge is vertical: X
        args = _resetArgs()
        v1 = FreeCAD.Vector(5, 5, 20)
        v2 = FreeCAD.Vector(5.0001, 5, 10)
        args["edge"] = Part.makeLine(v1, v2)
        self.assertRaises(ValueError, generator.generate, **args)

        # verify linear edge is vertical: Y
        args = _resetArgs()
        v1 = FreeCAD.Vector(5, 5.0001, 20)
        v2 = FreeCAD.Vector(5, 5, 10)
        args["edge"] = Part.makeLine(v1, v2)
        self.assertRaises(ValueError, generator.generate, **args)

    def test08(self):
        """Test Helix Generator with horizontal edge"""
        args = _resetArgs()
        v1 = FreeCAD.Vector(10, 5, 5)
        v2 = FreeCAD.Vector(20, 5, 5)
        args["edge"] = Part.makeLine(v1, v2)
        self.assertRaises(ValueError, generator.generate, **args)

    def test09(self):
        """Test Helix Generator with inverted vertical edge"""
        args = _resetArgs()
        v1 = FreeCAD.Vector(5, 5, 18)
        v2 = FreeCAD.Vector(5, 5, 20)
        args["edge"] = Part.makeLine(v1, v2)

        self.assertRaises(ValueError, generator.generate, **args)

    def test10(self):
        """Test Helix Retraction"""

        # if center is clear, the second to last move should be a G1 away
        # from the wall
        args = _resetArgs()
        v1 = FreeCAD.Vector(0, 0, 20)
        v2 = FreeCAD.Vector(0, 0, 18)
        args["edge"] = Part.makeLine(v1, v2)
        args["tool_diameter"] = 5.0
        result = generator.generate(**args)
        self.assertTrue(result[-2].Name == "G1")

        # if center is not clear, multiple helical paths
        # retraction is one straight up on the last move.
        # the second to last move should be a G1 pulloff to a clear radius
        args["hole_radius"] = 14.0
        args["inner_radius"] = 2.0
        result = generator.generate(**args)
        self.assertTrue(result[-2].Name == "G1")

        # single annular slot, center not clear
        # retraction is one straight up on the last move.
        #  the second to last move should be a G2 since "CW"
        args["hole_radius"] = 7.0
        args["inner_radius"] = 2.0
        result = generator.generate(**args)
        self.assertTrue(result[-2].Name == "G2")

    def test12(self):
        """test "FeedFactor" pseudo-param when using feedRateAdj"""
        args = _resetArgs()
        v1 = FreeCAD.Vector(0, 0, 20)
        v2 = FreeCAD.Vector(0, 0, 18)
        args["edge"] = Part.makeLine(v1, v2)
        args["hole_radius"] = 7.0
        args["inner_radius"] = 2.0
        result = generator.generate(**args)
        # first check it's not present in default run:
        self.assertTrue("FR" not in result[-2].Parameters)

        # now recalc with feedRateAdj:
        args["feedRateAdj"] = True
        result = generator.generate(**args)
        self.assertTrue("FR" in result[-2].Parameters)

        # create a double helix path with feedRateAdj
        # default tool is 5mm diam
        args["hole_radius"] = 8.0
        args["inner_radius"] = 2.0
        args["feedRateAdj"] = True
        args["startAt"] = "Outside"
        tool_r = args["tool_diameter"] / 2
        # first cut is outside diam, so reduce spindle feedrate
        # first arc is in 3rd gcode command :result[3]
        path_r = args["hole_radius"] - tool_r
        result = generator.generate(**args)
        self.assertTrue("FR" in result[3].Parameters)
        FF = result[3].Parameters["FR"]
        self.assertTrue(FF == path_r / (path_r + tool_r))

        # final cut is inside diam, so increase spindle feedrate
        # last arc in inner helix 3rd gcode from end :result[-3]
        path_r = args["inner_radius"] + tool_r
        result = generator.generate(**args)
        self.assertTrue("FR" in result[-3].Parameters)
        FF = result[-3].Parameters["FR"]
        self.assertTrue(FF == path_r / (path_r - tool_r))

        # test neg.inner_radius: r = zero: cut-off max F-value=200
        args["hole_radius"] = 10.5
        args["tool_diameter"] = 5.0
        args["inner_radius"] = 0.01
        args["startAt"] = "Outside"
        result = generator.generate(**args)
        self.assertTrue("FR" in result[-3].Parameters)
        FR = result[-3].Parameters["FR"]
        self.assertTrue(FR == 200)

    def test14(self):
        """test "DescentRate" pseudo-param when using feedRateAdj"""
        args = _resetArgs()
        v1 = FreeCAD.Vector(0, 0, 20)
        v2 = FreeCAD.Vector(0, 0, 18)
        args["edge"] = Part.makeLine(v1, v2)
        # assert no DescentRate  pseudo-param on plunge holes (r=0)
        args["hole_radius"] = 5.0
        args["tool_diameter"] = args["hole_radius"] * 2
        args["inner_radius"] = -args["hole_radius"]
        args["feedRateAdj"] = True
        result = generator.generate(**args)
        self.assertTrue("DR" not in result[-3].Parameters)

        # test DescentRate pseudo-param in helical path (assert zero on final circular arcs)
        args["hole_radius"] = 6.0
        result = generator.generate(**args)
        self.assertTrue("DR" in result[-3].Parameters)
        DR = result[-3].Parameters["DR"]
        self.assertTrue(DR == 0)  # last two arcs are flat circle, no descent
        DR = result[-5].Parameters["DR"]  # last true helix arc
        self.assertTrue(
            DR
            == args["step_down"] / (2 * math.pi * (args["hole_radius"] - args["tool_diameter"] / 2))
        )
