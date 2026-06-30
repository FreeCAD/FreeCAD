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

import Path
import CAMTests.PathTestUtils as PathTestUtils
import Path.Base.Generator.leadinout as leadinout


def _resetArgs(close=True):
    if close:
        commands = [
            Path.Command("G0", {"Z": 10}),
            Path.Command("G0", {"X": 0, "Y": 0}),
            Path.Command("G0", {"Z": 5}),
            Path.Command("G1", {"Z": 0, "F": 10}),
            Path.Command("G1", {"X": 50, "F": 20}),
            Path.Command("G1", {"Y": 50, "F": 20}),
            Path.Command("G1", {"X": 0, "F": 20}),
            Path.Command("G1", {"Y": 0, "F": 20}),
            Path.Command("G0", {"Z": 10}),
        ]
    else:
        commands = [
            Path.Command("G0", {"Z": 10}),
            Path.Command("G0", {"X": 0, "Y": 0}),
            Path.Command("G0", {"Z": 0}),
            Path.Command("G1", {"X": 50, "F": 20}),
            Path.Command("G0", {"Z": 10}),
            Path.Command("G0", {"X": 60, "Y": 0}),
            Path.Command("G0", {"Z": 0}),
            Path.Command("G1", {"X": 100, "F": 20}),
            Path.Command("G0", {"Z": 10}),
        ]
    return {
        "path": Path.Path(commands),
        "side": "Outside",
        "direction": "CCW",
        "styleIn": "Arc",
        "styleOut": "Line",
        "angleIn": 90,
        "angleOut": 45,
        "radiusIn": 10,
        "radiusOut": 5,
        "offsetIn": 0,
        "offsetOut": 0,
        "extendLeadIn": 5,
        "extendLeadOut": 5,
        "invertIn": False,
        "invertOut": False,
        "rapidPlunge": False,
        "retractThreshold": 0,
        "horizFeed": 20,
        "vertFeed": 10,
        "entranceFeed": 15,
        "exitFeed": 15,
        "clearanceHeight": 10,
        "safeHeight": 5,
    }


class TestPathLeadInOutGenerator(PathTestUtils.PathTestBase):

    def test00(self):
        """Test Basic Generator Return"""
        expected_gcode = """G0 Z10.000000
G0 X-10.000000 Y-15.000000
G0 Z5.000000
G1 F10.000000 Z0.000000
G1 F15.000000 X-10.000000 Y-10.000000 Z0.000000
G2 F15.000000 I10.000000 J0.000000 X0.000000 Y0.000000 Z0.000000
G1 F20.000000 X50.000000
G1 F20.000000 Y50.000000
G1 F20.000000 X0.000000
G1 F20.000000 Y0.000000
G1 F15.000000 X-7.071068 Y-7.071068 Z0.000000
G0 Z10.000000
"""
        args = _resetArgs()
        pp = leadinout.LeadInOut(**args).generate()
        self.assertTrue(pp.toGCode() == expected_gcode, "Incorrect g-code generated: basic")

    def test10(self):
        """Test offset"""
        args = _resetArgs()
        args["styleIn"] = "Vertical"
        args["styleOut"] = "Vertical"
        pp = leadinout.LeadInOut(**args).generate()
        length = pp.Length

        # positive offset in
        offset = 5
        args["offsetIn"] = offset
        args["offsetOut"] = 0
        pp = leadinout.LeadInOut(**args).generate()
        self.assertRoughly(pp.Length, length + 2 * offset)

        # positive offset out
        args["offsetIn"] = 0
        args["offsetOut"] = offset
        pp = leadinout.LeadInOut(**args).generate()
        self.assertRoughly(pp.Length, length + offset)

        # negative offset in
        args["offsetIn"] = -offset
        args["offsetOut"] = 0
        pp = leadinout.LeadInOut(**args).generate()
        self.assertRoughly(pp.Length, length)

        # negative offset out
        args["offsetIn"] = 0
        args["offsetOut"] = -offset
        pp = leadinout.LeadInOut(**args).generate()
        self.assertRoughly(pp.Length, length - offset)

        # negative offset in longer than path
        args["offsetIn"] = -length
        args["offsetOut"] = 0
        pp = leadinout.LeadInOut(**args).generate()
        self.assertRoughly(pp.Length, 0)

        # negative offset out longer than path
        args["offsetIn"] = 0
        args["offsetOut"] = -length
        pp = leadinout.LeadInOut(**args).generate()
        self.assertRoughly(pp.Length, 30.1)

    def test20(self):
        """Test invert"""
        args = _resetArgs()
        args["invertIn"] = True
        args["invertOut"] = True
        pp = leadinout.LeadInOut(**args).generate()
        self.assertEqual(pp.Commands[1].toGCode(), "G0 X-10.000000 Y15.000000")
        self.assertEqual(pp.Commands[-2].toGCode(), "G1 F15.000000 X7.071068 Y-7.071068 Z0.000000")

    def test30(self):
        """Test rapid plunge"""
        args = _resetArgs()
        args["rapidPlunge"] = True
        pp = leadinout.LeadInOut(**args).generate()
        self.assertEqual(pp.Commands[2].z, 0)

    def test40(self):
        """Test retract threshold"""
        expected_gcode = """G0 Z10.000000
G0 X0.000000 Y0.000000
G0 Z5.000000
G1 F10.000000 Z0.000000
G1 F20.000000 X50.000000
G1 F10.000000 X60.000000 Y0.000000 Z0.000000
G1 F20.000000 X100.000000
G0 Z10.000000
"""
        args = _resetArgs(close=False)
        args["styleIn"] = "Vertical"
        args["styleOut"] = "Vertical"
        args["retractThreshold"] = 10
        pp = leadinout.LeadInOut(**args).generate()
        self.assertTrue(pp.toGCode() == expected_gcode, "Incorrect g-code generated: threshold")
