# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic
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

"""
Test suite for Tapping
"""

import unittest
import Path

from Path.Post.Processor import PostProcessor
from Machine.models.machine import OutputUnits, Machine
from CAMTests.PostTestMocks import MockJob


class TestPostTapping(unittest.TestCase):
    """Test special behavior of Tapping (G84 G74)"""

    @classmethod
    def setUpClass(cls):
        """needed for each test"""
        cls.job = MockJob()
        cls.pp = PostProcessor(cls.job, "tooltip", "args", units="G21")

        # we are only testing the _convert_drill_cycle(), so no need to worry about expand, etc.
        cls.pp._machine = Machine()

    def settings(self, units):
        """Configure for the settings"""
        # self.job.Tools.Group[0].spindle_speed = 0 # not used
        self.pp._machine.output.units = units
        self.pp._merge_machine_config()

    def test_F_to_speed(self):
        """The Tapping operation generates an F that is the pitch, not speed, convert to speed"""

        # mm -> min, speed needs *60 -> units/min
        conv = {OutputUnits.IMPERIAL: 1 / 25.4, OutputUnits.METRIC: 1}

        # Values to make it more obvious what went wrong in calculation
        z = 10  # mm/sec
        pitch = 2  # mm/thread (.1mm = 1/10mm per thread)
        s = 3  # rev/min

        for units in [OutputUnits.METRIC, OutputUnits.IMPERIAL]:
            unit_conversion = conv[units]
            self.settings(units=units)

            for direction in ["G84", "G74"]:
                # As if from the tapping operation (i.e. f is pitch)
                rez = self.pp._convert_drill_cycle(
                    Path.Command(direction, {"F": pitch, "S": s, "Z": z}, {"operation": "tapping"})
                )

                # Unit analysis:
                # rev/min * mm/thread * thread/rev = mm/min
                # * unit/mm = unit/min # nb: we want per-minute speeds
                expected = s * pitch * 1 * unit_conversion

                self.assertEqual(
                    f"{direction} Z{z*unit_conversion:.3f} F{expected:.3f} S{s:.0f}",
                    rez,
                    f"For {direction}, {units}",
                )
