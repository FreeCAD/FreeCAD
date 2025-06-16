# SPDX-License-Identifier: LGPL-2.1-or-later

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


Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


def _resetArgs():
    v1 = FreeCAD.Vector(5, 5, 20)
    v2 = FreeCAD.Vector(5, 5, 18)

    edg = Part.makeLine(v1, v2)

    return {
        "edge": edg,
        "outer_radius": 7.5,
        "step_down": 1.0,
        "step": 2.5,
        "tool_diameter": 5.0,
        "inner_radius": 2.5,
        "retract_height": 23,
        "direction": "CW",
        "startAt": "Inside",
    }


class TestPathHelixGenerator(PathTestUtils.PathTestBase):

    expectedHelixGCode = "G0 Z23.000000\
G0 X7.500000 Y5.000000\
G1 Z20.000000\
G2 I-2.500000 J0.000000 X2.500000 Y5.000000 Z19.500000\
G2 I2.500000 J0.000000 X7.500000 Y5.000000 Z19.000000\
G2 I-2.500000 J0.000000 X2.500000 Y5.000000 Z18.500000\
G2 I2.500000 J0.000000 X7.500000 Y5.000000 Z18.000000\
G2 I-2.500000 J0.000000 X2.500000 Y5.000000 Z18.000000\
G2 I2.500000 J0.000000 X7.500000 Y5.000000 Z18.000000\
G0 X6.250000 Y5.000000 Z23.000000\
G0 X10.000000 Y5.000000\
G1 Z20.000000\
G2 I-5.000000 J0.000000 X0.000000 Y5.000000 Z19.500000\
G2 I5.000000 J0.000000 X10.000000 Y5.000000 Z19.000000\
G2 I-5.000000 J0.000000 X0.000000 Y5.000000 Z18.500000\
G2 I5.000000 J0.000000 X10.000000 Y5.000000 Z18.000000\
G2 I-5.000000 J0.000000 X0.000000 Y5.000000 Z18.000000\
G2 I5.000000 J0.000000 X10.000000 Y5.000000 Z18.000000\
G0 X8.750000 Y5.000000 Z23.000000\
G0 X12.500000 Y5.000000\
G1 Z20.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z19.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z19.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z18.500000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z18.000000\
G2 I-7.500000 J0.000000 X-2.500000 Y5.000000 Z18.000000\
G2 I7.500000 J0.000000 X12.500000 Y5.000000 Z18.000000\
G0 X11.250000 Y5.000000 Z23.000000"

    def test00(self):
        """Test Basic Helix Generator Return"""
        args = _resetArgs()
        result = generator.generate(**args)
        self.assertTrue(isinstance(result, list))
        self.assertTrue(isinstance(result[0], Path.Command))

        gcode = "".join([r.toGCode() for r in result])
        print()
        print("\n".join([str(r.toGCode()) + "\\" for r in result]))
        self.assertTrue(gcode == self.expectedHelixGCode, "Incorrect helix g-code generated")

    def test01(self):
        """Test Value and Type checking"""
        args = _resetArgs()
        args["outer_radius"] = "7.5"
        self.assertRaises(TypeError, generator.generate, **args)

        args["outer_radius"] = -7.5
        self.assertRaises(ValueError, generator.generate, **args)

        args = _resetArgs()
        args["inner_radius"] = "2"
        self.assertRaises(TypeError, generator.generate, **args)

        args = _resetArgs()
        args["tool_diameter"] = "5"
        self.assertRaises(TypeError, generator.generate, **args)

        # step is a length and can not be negative
        args = _resetArgs()
        args["step"] = -50
        self.assertRaises(ValueError, generator.generate, **args)
        args["step"] = "50"
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

        # if one helix and center is clear,
        # retraction is inclined line
        args = _resetArgs()
        v1 = FreeCAD.Vector(0, 0, 20)
        v2 = FreeCAD.Vector(0, 0, 18)
        edg = Part.makeLine(v1, v2)
        args["edge"] = edg
        args["outer_radius"] = 2.5
        args["inner_radius"] = 2.5
        args["tool_diameter"] = 5.0
        result = generator.generate(**args)
        self.assertEqual(result[-2].Name, "G2")
        self.assertEqual(result[-1].Name, "G0")
        self.assertTrue(result[-1].x is not None and result[-1].y is not None)

        # if one helix and center is not clear,
        # retraction is vertical line
        args = _resetArgs()
        v1 = FreeCAD.Vector(0, 0, 20)
        v2 = FreeCAD.Vector(0, 0, 18)
        edg = Part.makeLine(v1, v2)
        args["edge"] = edg
        args["outer_radius"] = 3.5
        args["inner_radius"] = 3.5
        args["tool_diameter"] = 5.0
        result = generator.generate(**args)
        self.assertEqual(result[-2].Name, "G2")
        self.assertEqual(result[-1].Name, "G0")
        self.assertTrue(result[-1].x is None and result[-1].y is None)

        # if several helices and start at Inside,
        # last retraction is inclined line
        args = _resetArgs()
        v1 = FreeCAD.Vector(0, 0, 20)
        v2 = FreeCAD.Vector(0, 0, 18)
        edg = Part.makeLine(v1, v2)
        args["edge"] = edg
        args["outer_radius"] = 10.0
        args["inner_radius"] = 2.5
        args["tool_diameter"] = 5.0
        args["startAt"] = "Inside"
        result = generator.generate(**args)
        self.assertEqual(result[-2].Name, "G2")
        self.assertEqual(result[-1].Name, "G0")
        self.assertTrue(result[-1].x is not None and result[-1].y is not None)

        # if several helices, start at Outside and center is clear
        # last retraction is vertical line
        args = _resetArgs()
        v1 = FreeCAD.Vector(0, 0, 20)
        v2 = FreeCAD.Vector(0, 0, 18)
        edg = Part.makeLine(v1, v2)
        args["edge"] = edg
        args["outer_radius"] = 10.0
        args["inner_radius"] = 2.5
        args["tool_diameter"] = 5.0
        args["startAt"] = "Outside"
        result = generator.generate(**args)
        self.assertEqual(result[-2].Name, "G2")
        self.assertEqual(result[-1].Name, "G0")
        self.assertTrue(result[-1].x is None and result[-1].y is None)

        # if several helices, start at Outside and center is not clear
        # last retraction is inclinde line
        args = _resetArgs()
        v1 = FreeCAD.Vector(0, 0, 20)
        v2 = FreeCAD.Vector(0, 0, 18)
        edg = Part.makeLine(v1, v2)
        args["edge"] = edg
        args["outer_radius"] = 10.0
        args["inner_radius"] = 3.5
        args["tool_diameter"] = 5.0
        args["startAt"] = "Outside"
        result = generator.generate(**args)
        self.assertEqual(result[-2].Name, "G2")
        self.assertEqual(result[-1].Name, "G0")
        self.assertTrue(result[-1].x is not None and result[-1].y is not None)
