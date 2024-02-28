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

import FreeCAD
import Path
from PathTests.PathTestUtils import PathTestBase


class TestPathCore(PathTestBase):
    def test00(self):
        """Test Path command core functionality"""
        # create empty command
        c = Path.Command()
        self.assertIsInstance(c, Path.Command)

        # change name
        c.Name = "G1"
        self.assertEqual(c.Name, "G1")

        # Assign Parameters
        c.Parameters = {"X": 1, "Y": 0}
        self.assertEqual(c.Parameters, {"Y": 0.0, "X": 1.0})

        # change parameters
        c.Parameters = {"X": 1, "Y": 0.5}
        self.assertEqual(c.Parameters, {"Y": 0.5, "X": 1})

        # output gcode
        self.assertEqual(c.toGCode(), "G1 X1.000000 Y0.500000")

        # create and assign name in one
        c2 = Path.Command("G2")
        self.assertEqual(c2.Name, "G2")

        # Create Path and parameters in one
        c3 = Path.Command("G1", {"X": 34, "Y": 1.2})
        self.assertEqual(str(c3), "Command G1 [ X:34 Y:1.2 ]")
        c4 = Path.Command("G1X4Y5")
        self.assertEqual(str(c4), "Command G1 [ X:4 Y:5 ]")

        # use placement
        self.assertEqual(
            str(c3.Placement), "Placement [Pos=(34,1.2,0), Yaw-Pitch-Roll=(0,0,0)]"
        )
        self.assertEqual(c3.toGCode(), "G1 X34.000000 Y1.200000")
        p1 = FreeCAD.Placement()
        p1.Base = FreeCAD.Vector(3, 2, 1)
        self.assertEqual(str(p1), "Placement [Pos=(3,2,1), Yaw-Pitch-Roll=(0,0,0)]")
        c5 = Path.Command("g1", p1)
        self.assertEqual(str(c5), "Command G1 [ X:3 Y:2 Z:1 ]")
        p2 = FreeCAD.Placement()
        p2.Base = FreeCAD.Vector(5, 0, 0)

        # overwrite placement
        c5.Placement = p2
        self.assertEqual(str(c5), "Command G1 [ X:5 ]")
        self.assertEqual(c5.x, 5.0)

        # overwrite individual parameters
        c5.x = 10
        self.assertEqual(c5.x, 10.0)
        c5.y = 2
        self.assertEqual(str(c5), "Command G1 [ X:10 Y:2 ]")

        # set from gcode
        c3.setFromGCode("G1X1Y0")
        self.assertEqual(str(c3), "Command G1 [ X:1 Y:0 ]")

    def test10(self):
        """Test Path Object core functionality"""

        c1 = Path.Command("g1", {"x": 1, "y": 0})
        c2 = Path.Command("g1", {"x": 0, "y": 2})
        p = Path.Path([c1, c2])
        self.assertAlmostEqual(str(p), "Path [ size:2 length:3.2361 ]", places=4)

        self.assertEqual(
            str(p.Commands), "[Command G1 [ X:1 Y:0 ], Command G1 [ X:0 Y:2 ]]"
        )
        self.assertAlmostEqual(p.Length, 3.2361, places=4)
        p.addCommands(c1)
        self.assertEqual(
            p.toGCode(),
            "G1 X1.000000 Y0.000000\nG1 X0.000000 Y2.000000\nG1 X1.000000 Y0.000000\n",
        )

        lines = """
G0X-0.5905Y-0.3937S3000M03
G0Z0.125
G1Z-0.004F3
G1X0.9842Y-0.3937F14.17
G1X0.9842Y0.433
G1X-0.5905Y0.433
G1X-0.5905Y-0.3937
G0Z0.5
"""

        output = """G0 S3000.000000 X-0.590500 Y-0.393700
M03
G0 Z0.125000
G1 F3.000000 Z-0.004000
G1 F14.170000 X0.984200 Y-0.393700
G1 X0.984200 Y0.433000
G1 X-0.590500 Y0.433000
G1 X-0.590500 Y-0.393700
G0 Z0.500000
"""

        # create a path directly form a piece of gcode.
        p = Path.Path()
        p.setFromGCode(lines)
        self.assertEqual(p.toGCode(), output)

    def test50(self):
        """Test Path.Length calculation"""
        commands = []
        commands.append(Path.Command("G1", {"X": 1}))
        commands.append(Path.Command("G1", {"Y": 1}))
        path = Path.Path(commands)

        self.assertEqual(path.Length, 2)
