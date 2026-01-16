# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import Path
import CAMTests.PathTestUtils as PathTestUtils
import Path.Tool.Controller as PathToolController
from CAMTests.TestPathAdaptive import getGcodeMoves

import math

from Path.Base.Generator.ramp_entry import RampEntry


def _resetArgs():
    commands = [
        Path.Command("G0", {"Z": 15}),
        Path.Command("G0", {"X": 52, "Y": 25}),
        Path.Command("G0", {"Z": 13}),
        Path.Command("G1", {"X": 52, "Y": 25, "Z": 6, "F": 500}),
        Path.Command("G1", {"X": 52, "Y": 0, "Z": 6, "F": 500}),
        Path.Command("G2", {"X": 50, "Y": -2, "Z": 6, "I": -2, "J": -0, "F": 500}),
        Path.Command("G1", {"X": 0, "Y": -2, "Z": 6, "F": 500}),
        Path.Command("G2", {"X": -2, "Y": 0, "Z": 6, "I": -0, "J": 2, "F": 500}),
        Path.Command("G1", {"X": -2, "Y": 50, "Z": 6, "F": 500}),
        Path.Command("G2", {"X": 0, "Y": 52, "Z": 6, "I": 2, "J": 0, "F": 500}),
        Path.Command("G1", {"X": 50, "Y": 52, "Z": 6, "F": 500}),
        Path.Command("G2", {"X": 52, "Y": 50, "Z": 6, "I": 0, "J": -2, "F": 500}),
        Path.Command("G1", {"X": 52, "Y": 25, "Z": 6, "F": 500}),
        Path.Command("G1", {"X": 52, "Y": 25, "Z": 3, "F": 500}),
        Path.Command("G1", {"X": 52, "Y": 0, "Z": 3, "F": 500}),
        Path.Command("G2", {"X": 50, "Y": -2, "Z": 3, "I": -2, "J": -0, "F": 500}),
        Path.Command("G1", {"X": 0, "Y": -2, "Z": 3, "F": 500}),
        Path.Command("G2", {"X": -2, "Y": 0, "Z": 3, "I": -0, "J": 2, "F": 500}),
        Path.Command("G1", {"X": -2, "Y": 50, "Z": 3, "F": 500}),
        Path.Command("G2", {"X": 0, "Y": 52, "Z": 3, "I": 2, "J": 0, "F": 500}),
        Path.Command("G1", {"X": 50, "Y": 52, "Z": 3, "F": 500}),
        Path.Command("G2", {"X": 52, "Y": 50, "Z": 3, "I": 0, "J": -2, "F": 500}),
        Path.Command("G1", {"X": 52, "Y": 25, "Z": 3, "F": 500}),
        Path.Command("G1", {"X": 52, "Y": 25, "Z": 0, "F": 500}),
        Path.Command("G1", {"X": 52, "Y": 0, "Z": 0, "F": 500}),
        Path.Command("G2", {"X": 50, "Y": -2, "Z": 0, "I": -2, "J": -0, "F": 500}),
        Path.Command("G1", {"X": 0, "Y": -2, "Z": 0, "F": 500}),
        Path.Command("G2", {"X": -2, "Y": 0, "Z": 0, "I": -0, "J": 2, "F": 500}),
        Path.Command("G1", {"X": -2, "Y": 50, "Z": 0, "F": 500}),
        Path.Command("G2", {"X": 0, "Y": 52, "Z": 0, "I": 2, "J": 0, "F": 500}),
        Path.Command("G1", {"X": 50, "Y": 52, "Z": 0, "F": 500}),
        Path.Command("G2", {"X": 52, "Y": 50, "Z": 0, "I": 0, "J": -2, "F": 500}),
        Path.Command("G1", {"X": 52, "Y": 25, "Z": 0, "F": 500}),
        Path.Command("G0", {"Z": 15}),
    ]

    tc = PathToolController.Create("TC0")
    tc.VertFeed = 500
    tc.RampFeed = 500
    tc.HorizFeed = 500
    tc.VertRapid = 0
    tc.HorizRapid = 0

    args = {
        "commands": commands,
        "method": 0,
        "angle_rad": math.radians(5),
        "pitch": 10,
        "tc": tc,
        "ignoreAbove": 9,
    }

    return args


class TestPathRampEntryGenerator(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathToolController")
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")

    def test00(self):
        """Test Basic Return"""
        args = _resetArgs()
        result = RampEntry(**args).generate()
        self.assertTrue(isinstance(result, list))
        self.assertTrue(isinstance(result[0], Path.Command))
        self.assertTrue(isinstance(result[-1], Path.Command))
        self.assertGreater(len(result), 30)

    def test01(self):
        """Test Value and Type checking"""
        args = _resetArgs()
        args["method"] = "3"
        self.assertRaises(TypeError, RampEntry, **args)

        args = _resetArgs()
        args["method"] = 5
        self.assertRaises(ValueError, RampEntry, **args)

        args = _resetArgs()
        args["angle_rad"] = "10"
        self.assertRaises(TypeError, RampEntry, **args)

        args = _resetArgs()
        args["angle_rad"] = -10
        self.assertRaises(ValueError, RampEntry, **args)

        args = _resetArgs()
        args["pitch"] = "10"
        self.assertRaises(TypeError, RampEntry, **args)

        args = _resetArgs()
        args["pitch"] = -10
        self.assertRaises(ValueError, RampEntry, **args)

        args = _resetArgs()
        args["ignoreAbove"] = "10"
        self.assertRaises(TypeError, RampEntry, **args)

        args = _resetArgs()
        args["tc"] = "toolcontroller"
        self.assertRaises(TypeError, RampEntry, **args)

        args = _resetArgs()
        args["tc"].VertFeed = 0
        self.assertRaises(ValueError, RampEntry, **args)

        args = _resetArgs()
        args["tc"].HorizFeed = 0
        self.assertRaises(ValueError, RampEntry, **args)

        args = _resetArgs()
        args["tc"].RampFeed = 0
        self.assertRaises(ValueError, RampEntry, **args)

    def test10(self):
        """Test ramp entry method 0 - Helix"""

        expected_gcode = (
            "G1 X52.0 Y25.0 Z9.0;  G1 X52.0 Y0.0 Z8.65;  "
            "G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z8.6;  G1 X0.0 Y-2.0 Z7.9;  "
            "G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z7.85;  G1 X-2.0 Y50.0 Z7.15;  "
            "G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z7.1;  G1 X50.0 Y52.0 Z6.4;  "
            "G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z6.35;  G1 X52.0 Y25.0 Z6.0;  "
            "G1 X52.0 Y0.0 Z5.65;  G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z5.6;  "
            "G1 X0.0 Y-2.0 Z4.9;  G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z4.85;  "
            "G1 X-2.0 Y50.0 Z4.15;  G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z4.1;  "
            "G1 X50.0 Y52.0 Z3.4;  G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z3.35;  "
            "G1 X52.0 Y25.0 Z3.0;  G1 X52.0 Y0.0 Z2.65;  "
            "G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z2.6;  G1 X0.0 Y-2.0 Z1.9;  "
            "G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z1.85;  G1 X-2.0 Y50.0 Z1.15;  "
            "G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z1.1;  G1 X50.0 Y52.0 Z0.4;  "
            "G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z0.35;  G1 X52.0 Y25.0 Z0.0;  "
            "G1 X52.0 Y0.0 Z0.0;  G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z0.0;  "
            "G1 X0.0 Y-2.0 Z0.0;  G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z0.0;  "
            "G1 X-2.0 Y50.0 Z0.0;  G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z0.0;  "
            "G1 X50.0 Y52.0 Z0.0;  G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z0.0;  "
            "G1 X52.0 Y25.0 Z0.0"
        )

        args = _resetArgs()
        args["method"] = 0
        args["angle_rad"] = math.radians(5)
        args["pitch"] = None
        args["ignoreAbove"] = 9
        result = RampEntry(**args).generate()
        moves = getGcodeMoves(result, includeRapids=False)
        operationMoves = ";  ".join(moves)
        self.assertTrue(
            expected_gcode == operationMoves,
            "Incorrect g-code generated by method 0 (Helix)",
        )

    def test11(self):
        """Test ramp entry method 1"""

        expected_gcode = (
            "G1 X52.0 Y25.0 Z9.0;  G1 X52.0 Y19.8 Z6.0;  "
            "G1 X52.0 Y25.0 Z6.0;  G1 X52.0 Y0.0 Z6.0;  "
            "G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z6.0;  G1 X0.0 Y-2.0 Z6.0;  "
            "G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z6.0;  G1 X-2.0 Y50.0 Z6.0;  "
            "G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z6.0;  G1 X50.0 Y52.0 Z6.0;  "
            "G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z6.0;  G1 X52.0 Y25.0 Z6.0;  "
            "G1 X52.0 Y19.8 Z3.0;  G1 X52.0 Y25.0 Z3.0;  "
            "G1 X52.0 Y0.0 Z3.0;  G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z3.0;  "
            "G1 X0.0 Y-2.0 Z3.0;  G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z3.0;  "
            "G1 X-2.0 Y50.0 Z3.0;  G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z3.0;  "
            "G1 X50.0 Y52.0 Z3.0;  G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z3.0;  "
            "G1 X52.0 Y25.0 Z3.0;  G1 X52.0 Y19.8 Z0.0;  "
            "G1 X52.0 Y25.0 Z0.0;  G1 X52.0 Y0.0 Z0.0;  "
            "G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z0.0;  G1 X0.0 Y-2.0 Z0.0;  "
            "G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z0.0;  G1 X-2.0 Y50.0 Z0.0;  "
            "G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z0.0;  G1 X50.0 Y52.0 Z0.0;  "
            "G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z0.0;  G1 X52.0 Y25.0 Z0.0"
        )

        args = _resetArgs()
        args["method"] = 1
        args["angle_rad"] = math.radians(30)
        args["pitch"] = None
        args["ignoreAbove"] = 9
        result = RampEntry(**args).generate()
        moves = getGcodeMoves(result, includeRapids=False)
        operationMoves = ";  ".join(moves)
        self.assertTrue(expected_gcode == operationMoves, "Incorrect g-code generated be method 1")

    def test12(self):
        """Test ramp entry method 2"""

        expected_gcode = (
            "G1 X52.0 Y25.0 Z9.0;  G1 X52.0 Y19.8 Z9.0;  "
            "G1 X52.0 Y25.0 Z6.0;  G1 X52.0 Y0.0 Z6.0;  "
            "G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z6.0;  G1 X0.0 Y-2.0 Z6.0;  "
            "G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z6.0;  G1 X-2.0 Y50.0 Z6.0;  "
            "G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z6.0;  G1 X50.0 Y52.0 Z6.0;  "
            "G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z6.0;  G1 X52.0 Y25.0 Z6.0;  "
            "G1 X52.0 Y19.8 Z6.0;  G1 X52.0 Y25.0 Z3.0;  "
            "G1 X52.0 Y0.0 Z3.0;  G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z3.0;  "
            "G1 X0.0 Y-2.0 Z3.0;  G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z3.0;  "
            "G1 X-2.0 Y50.0 Z3.0;  G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z3.0;  "
            "G1 X50.0 Y52.0 Z3.0;  G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z3.0;  "
            "G1 X52.0 Y25.0 Z3.0;  G1 X52.0 Y19.8 Z3.0;  "
            "G1 X52.0 Y25.0 Z0.0;  G1 X52.0 Y0.0 Z0.0;  "
            "G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z0.0;  G1 X0.0 Y-2.0 Z0.0;  "
            "G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z0.0;  G1 X-2.0 Y50.0 Z0.0;  "
            "G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z0.0;  G1 X50.0 Y52.0 Z0.0;  "
            "G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z0.0;  G1 X52.0 Y25.0 Z0.0"
        )

        args = _resetArgs()
        args["method"] = 2
        args["angle_rad"] = math.radians(30)
        args["pitch"] = None
        args["ignoreAbove"] = 9
        result = RampEntry(**args).generate()
        moves = getGcodeMoves(result, includeRapids=False)
        operationMoves = ";  ".join(moves)
        self.assertTrue(expected_gcode == operationMoves, "Incorrect g-code generated by method 2")

    def test13(self):
        """Test ramp entry method 3"""

        expected_gcode = (
            "G1 X52.0 Y25.0 Z9.0;  G1 X52.0 Y22.4 Z7.5;  "
            "G1 X52.0 Y25.0 Z6.0;  G1 X52.0 Y0.0 Z6.0;  "
            "G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z6.0;  G1 X0.0 Y-2.0 Z6.0;  "
            "G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z6.0;  G1 X-2.0 Y50.0 Z6.0;  "
            "G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z6.0;  G1 X50.0 Y52.0 Z6.0;  "
            "G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z6.0;  G1 X52.0 Y25.0 Z6.0;  "
            "G1 X52.0 Y22.4 Z4.5;  G1 X52.0 Y25.0 Z3.0;  "
            "G1 X52.0 Y0.0 Z3.0;  G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z3.0;  "
            "G1 X0.0 Y-2.0 Z3.0;  G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z3.0;  "
            "G1 X-2.0 Y50.0 Z3.0;  G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z3.0;  "
            "G1 X50.0 Y52.0 Z3.0;  G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z3.0;  "
            "G1 X52.0 Y25.0 Z3.0;  G1 X52.0 Y22.4 Z1.5;  "
            "G1 X52.0 Y25.0 Z0.0;  G1 X52.0 Y0.0 Z0.0;  "
            "G2 I-2.0 J0.0 K0.0 X50.0 Y-2.0 Z0.0;  G1 X0.0 Y-2.0 Z0.0;  "
            "G2 I0.0 J2.0 K0.0 X-2.0 Y0.0 Z0.0;  G1 X-2.0 Y50.0 Z0.0;  "
            "G2 I2.0 J0.0 K0.0 X0.0 Y52.0 Z0.0;  G1 X50.0 Y52.0 Z0.0;  "
            "G2 I0.0 J-2.0 K0.0 X52.0 Y50.0 Z0.0;  G1 X52.0 Y25.0 Z0.0"
        )

        args = _resetArgs()
        args["method"] = 3
        args["angle_rad"] = math.radians(30)
        args["pitch"] = None
        args["ignoreAbove"] = 9
        result = RampEntry(**args).generate()
        moves = getGcodeMoves(result, includeRapids=False)
        operationMoves = ";  ".join(moves)
        self.assertTrue(expected_gcode == operationMoves, "Incorrect g-code generated by method 3")
