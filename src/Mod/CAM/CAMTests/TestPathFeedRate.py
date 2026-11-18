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
