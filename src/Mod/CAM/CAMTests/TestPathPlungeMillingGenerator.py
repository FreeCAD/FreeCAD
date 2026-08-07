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
import Path.Base.Generator.plunge_milling as plunge_milling
from CAMTests.TestPathAdaptive import getGcodeMoves


class TestPathPlungeMillingGenerator(PathTestUtils.PathTestBase):

    def test00(self):
        """Test Basic Generator Return"""
        expected_gcode = (
            "G0 X0.0 Y0.0 Z10.0;  G0 X0.0 Y0.0 Z10.0;  "
            "G0 X0.0 Y0.0 Z5.0;  G0 X0.0 Y0.0 Z5.0;  G1 X0.0 Y0.0 Z0.0;  "
            "G0 X0.0 Y0.0 Z10.0;  G0 X3.0 Y0.0 Z10.0;  G1 X3.0 Y0.0 Z0.0;  "
            "G0 X3.0 Y0.0 Z10.0;  G0 X6.0 Y0.0 Z10.0;  G1 X6.0 Y0.0 Z0.0;  "
            "G0 X6.0 Y0.0 Z10.0;  G0 X9.0 Y0.0 Z10.0;  G1 X9.0 Y0.0 Z0.0;  "
            "G0 X9.0 Y0.0 Z10.0;  G0 X9.0 Y0.0 Z10.0"
        )

        baseCmds = [
            Path.Command("G0", {"Z": 10}),
            Path.Command("G0", {"X": 0, "Y": 0, "Z": 10}),
            Path.Command("G0", {"Z": 5}),
            Path.Command("G1", {"Z": 0, "F": 10}),
            Path.Command("G1", {"X": 9, "F": 20}),
            Path.Command("G0", {"Z": 10}),
        ]
        basePath = Path.Path(baseCmds)
        step = 3
        step_min = step / 2
        retract_height = 10
        vert_feed = 10
        pp = plunge_milling.get_path(
            path=basePath,
            step=step,
            step_min=step_min,
            retract_height=retract_height,
            vert_feed=vert_feed,
        )

        moves = getGcodeMoves(pp.Commands)
        operationMoves = ";  ".join(moves)
        self.assertTrue(expected_gcode == operationMoves, "Incorrect g-code generated")
