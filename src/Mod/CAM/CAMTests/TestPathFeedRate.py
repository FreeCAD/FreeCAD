# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Bill Warner bill.warner@gmail.com
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

import math
import FreeCAD
import Path
import Path.Base.FeedRate as PathFeedRate

from CAMTests.PathTestUtils import PathTestBase


class _Quantity:
    """Minimal stand-in for a FreeCAD Quantity exposing .Value."""

    def __init__(self, value):
        self.Value = value


class _TestToolController:
    """Tool controller with the feeds a tapping tool typically leaves at zero."""

    def __init__(self, vert=0.0, horiz=0.0):
        self.VertFeed = _Quantity(vert)
        self.HorizFeed = _Quantity(horiz)
        self.VertRapid = _Quantity(0.0)
        self.HorizRapid = _Quantity(0.0)


class TestPathFeedRate(PathTestBase):
    """Test feed rate assignment across command types."""

    def test00(self):
        """Verify a tapping cycle keeps its pitch in F.

        On G84/G74 the F word carries the thread pitch, not a feed rate.
        setFeedRate must leave it alone; a tap tool commonly has VertFeed
        of zero because the feed is derived as pitch x RPM.
        """
        for name in ("G84", "G74"):
            cmd = Path.Command(name, {"X": 0.0, "Y": 0.0, "Z": -10.0, "R": 2.0, "F": 1.41224})
            PathFeedRate.setFeedRate([cmd], _TestToolController(vert=0.0))
            self.assertEqual(cmd.Parameters["F"], 1.41224)

    def test01(self):
        """Verify tapping pitch survives even when the tool has feeds set."""
        cmd = Path.Command("G84", {"X": 0.0, "Y": 0.0, "Z": -10.0, "R": 2.0, "F": 1.41224})
        PathFeedRate.setFeedRate([cmd], _TestToolController(vert=100.0, horiz=200.0))
        self.assertEqual(cmd.Parameters["F"], 1.41224)

    def test02(self):
        """Verify standard drill cycles still get the vertical feed."""
        cmd = Path.Command("G83", {"X": 0.0, "Y": 0.0, "Z": -10.0, "R": 2.0, "F": 0.0})
        PathFeedRate.setFeedRate([cmd], _TestToolController(vert=123.0))
        self.assertEqual(cmd.Parameters["F"], 123.0)

    def test03(self):
        """Verify ordinary feed moves still get a feed rate."""
        cmd = Path.Command("G1", {"X": 10.0, "Y": 0.0, "Z": 0.0})
        PathFeedRate.setFeedRate([cmd], _TestToolController(vert=50.0, horiz=250.0))
        self.assertEqual(cmd.Parameters["F"], 250.0)

    def testAutomaticRampFeed(self):
        tc = _TestToolController(vert=2.0, horiz=10.0)
        tc.RampFeedMode = "Automatic"
        start = FreeCAD.Vector()
        for horizontal, vertical, expected in (
            (10.0, 1.0, math.hypot(10.0, 1.0)),
            (1.0, 10.0, 2.0 * math.hypot(1.0, 10.0) / 10.0),
            (10.0, 0.0, 10.0),
            (0.0, 10.0, 2.0),
            (0.0, 0.0, 10.0),
        ):
            for direction in (-1, 1):
                command = Path.Command("G1", {"X": horizontal, "Z": direction * vertical})
                self.assertAlmostEqual(PathFeedRate.rampFeedRate(tc, command, start), expected)

        tc.VertFeed.Value = 0.0
        self.assertEqual(
            PathFeedRate.rampFeedRate(tc, Path.Command("G1", {"X": 10, "Z": -1}), start),
            0.0,
        )

    def testAutomaticHelixFeed(self):
        """Full circles must use arc length, even with coincident XY endpoints."""
        tc = _TestToolController(vert=2.0, horiz=10.0)
        tc.RampFeedMode = "Automatic"
        start = FreeCAD.Vector(5, 0, 0)
        for name in ("G2", "G3"):
            for height in (1.0, 100.0):
                command = Path.Command(name, {"X": 5, "Y": 0, "Z": -height, "I": -5})
                horizontal = 10 * math.pi
                length = math.hypot(horizontal, height)
                expected = min(10 * length / horizontal, 2 * length / height)
                self.assertAlmostEqual(PathFeedRate.rampFeedRate(tc, command, start), expected)
                self.assertEqual(command.Parameters["Z"], -height)

        # Quarter-circle travel is longer than its chord, too.
        command = Path.Command("G3", {"X": 0, "Y": 5, "Z": -10, "I": -5, "K": -5})
        horizontal = 2.5 * math.pi
        length = math.hypot(horizontal, 10)
        self.assertAlmostEqual(
            PathFeedRate.rampFeedRate(tc, command, start),
            min(10 * length / horizontal, 2 * length / 10),
        )
        # Same endpoints, opposite direction: this is a three-quarter arc.
        command = Path.Command("G2", command.Parameters)
        horizontal = 7.5 * math.pi
        length = math.hypot(horizontal, 10)
        self.assertAlmostEqual(
            PathFeedRate.rampFeedRate(tc, command, start),
            min(10 * length / horizontal, 2 * length / 10),
        )

    def testCustomRampFeed(self):
        tc = _TestToolController(vert=2.0, horiz=10.0)
        tc.RampFeedMode = "Custom"
        tc.RampFeed = _Quantity(123.0)
        command = Path.Command("G1", {"X": 1, "Z": -10})
        self.assertEqual(PathFeedRate.rampFeedRate(tc, command, FreeCAD.Vector()), 123.0)

    def testRampFeedTracksPosition(self):
        tc = _TestToolController(vert=2.0, horiz=10.0)
        tc.RampFeedMode = "Automatic"
        commands = [
            Path.Command("G0", {"X": 5, "Y": 0, "Z": 10, "F": 99}),
            Path.Command("G3", {"Z": 9, "I": -5}),
            Path.Command("G1", {"X": 15, "Z": 8}),
        ]
        PathFeedRate.setRampFeedRate(commands, tc)
        self.assertEqual(commands[0].Parameters["F"], 99)
        self.assertAlmostEqual(
            commands[1].Parameters["F"], 10 * math.hypot(10 * math.pi, 1) / (10 * math.pi)
        )
        self.assertAlmostEqual(commands[2].Parameters["F"], math.hypot(10, 1))
