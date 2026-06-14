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

from Path.Base.Generator.ramp_entry_helix import HelixRamp


def _resetArgs():
    commands = [
        Path.Command("G0", {"Z": 15}),
        Path.Command("G0", {"X": 52, "Y": 25}),
        Path.Command("G0", {"Z": 13}),
        Path.Command("G1", {"X": 52, "Y": 25, "Z": 7, "F": 500}),
        Path.Command("G1", {"X": 52, "Y": 0, "Z": 7, "F": 500}),
        Path.Command("G2", {"X": 50, "Y": -2, "Z": 7, "I": -2, "J": -0, "F": 500}),
        Path.Command("G1", {"X": 0, "Y": -2, "Z": 7, "F": 500}),
        Path.Command("G2", {"X": -2, "Y": 0, "Z": 7, "I": -0, "J": 2, "F": 500}),
        Path.Command("G1", {"X": -2, "Y": 50, "Z": 7, "F": 500}),
        Path.Command("G2", {"X": 0, "Y": 52, "Z": 7, "I": 2, "J": 0, "F": 500}),
        Path.Command("G1", {"X": 50, "Y": 52, "Z": 7, "F": 500}),
        Path.Command("G2", {"X": 52, "Y": 50, "Z": 7, "I": 0, "J": -2, "F": 500}),
        Path.Command("G1", {"X": 52, "Y": 25, "Z": 7, "F": 500}),
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

    return {"commands": commands, "tc": tc, "maxStepDown": 10, "ignoreAbove": 7}


class TestPathRampEntryHelixGenerator(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathToolController")
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")

    def test00(self):
        """Test Basic Return"""
        args = _resetArgs()
        result = HelixRamp(**args).generate()
        self.assertTrue(isinstance(result, list))
        self.assertTrue(isinstance(result[0], Path.Command))
        self.assertTrue(isinstance(result[-1], Path.Command))
        self.assertGreater(len(result), 30)

    def test01(self):
        """Test Value and Type checking"""
        args = _resetArgs()
        args["maxStepDown"] = "10"
        self.assertRaises(TypeError, HelixRamp, **args)

        args["maxStepDown"] = -10
        self.assertRaises(ValueError, HelixRamp, **args)

        args = _resetArgs()
        args["ignoreAbove"] = "10"
        self.assertRaises(TypeError, HelixRamp, **args)

        args = _resetArgs()
        args["tc"] = "toolcontroller"
        self.assertRaises(TypeError, HelixRamp, **args)

        args = _resetArgs()
        args["tc"].VertFeed = 0
        self.assertRaises(ValueError, HelixRamp, **args)

        args = _resetArgs()
        args["tc"].HorizFeed = 0
        self.assertRaises(ValueError, HelixRamp, **args)

        args = _resetArgs()
        args["tc"].RampFeed = 0
        self.assertRaises(ValueError, HelixRamp, **args)

    def test02(self):
        """Test maxStepDown"""
        args = _resetArgs()
        result1 = HelixRamp(**args).generate()
        args["maxStepDown"] = 1
        result2 = HelixRamp(**args).generate()
        self.assertLess(2 * len(result1), len(result2))

        args["maxStepDown"] = 10
        result2 = HelixRamp(**args).generate()
        self.assertEqual(len(result1), len(result2))

    def test03(self):
        """Test ignoreAbove"""
        args = _resetArgs()
        args["ignoreAbove"] = 3
        result = HelixRamp(**args).generate()
        resultGcode = Path.Path(result).toGCode()
        inputGcode = Path.Path(args["commands"]).toGCode()
        self.assertEqual(resultGcode[:20], inputGcode[:20])
