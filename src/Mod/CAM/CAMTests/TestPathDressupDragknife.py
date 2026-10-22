# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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
from Path.Dressup.Gui.Dragknife import ObjectDressup

from CAMTests.PathTestUtils import PathTestBase

OFFSET = 0.2
PIVOT = 5.0
DEPTH = 0.0
SAFE = 10.0


class _TestBase:
    def __init__(self, commands):
        self.Path = Path.Path(commands)
        self.Name = "Profile"

    def isDerivedFrom(self, typ):
        return typ == "Path::Feature"


class _TestDressup:
    def __init__(self, base):
        self.Base = base
        self.Path = Path.Path()
        self.filterAngle = 20.0
        self.offset = OFFSET
        self.pivotheight = PIVOT

    def addProperty(self, typ, name, category, tip):
        setattr(self, name, None)


def _cut(*moves):
    """Wrap feed moves at DEPTH in an entry and an exit, the way an op emits them."""
    return (
        [
            Path.Command("G0", {"Z": SAFE}),
            Path.Command("G0", {"X": 0, "Y": 0}),
            Path.Command("G1", {"Z": DEPTH}),
        ]
        + list(moves)
        + [Path.Command("G0", {"Z": SAFE})]
    )


def _arcs(commands):
    """Return (start, center, end, sweep, name) for each arc, in XY."""
    pos = FreeCAD.Vector()
    arcs = []
    for cmd in commands:
        p = cmd.Parameters
        if cmd.Name in ("G2", "G3"):
            start = FreeCAD.Vector(pos.x, pos.y, 0)
            center = FreeCAD.Vector(pos.x + p.get("I", 0), pos.y + p.get("J", 0), 0)
            end = FreeCAD.Vector(p.get("X", pos.x), p.get("Y", pos.y), 0)
            a0 = math.atan2(start.y - center.y, start.x - center.x)
            a1 = math.atan2(end.y - center.y, end.x - center.x)
            if cmd.Name == "G2":
                sweep = (a0 - a1) % (2 * math.pi)
            else:
                sweep = (a1 - a0) % (2 * math.pi)
            arcs.append((start, center, end, sweep, cmd.Name))
        for k in "XYZ":
            if k in p:
                setattr(pos, k.lower(), p[k])
    return arcs


def _point(center, radius, degrees):
    a = math.radians(degrees)
    return FreeCAD.Vector(center.x + radius * math.cos(a), center.y + radius * math.sin(a), 0)


class TestDressupDragknife(PathTestBase):
    """Unit tests for the Dragknife dressup corner actions."""

    def dress(self, commands):
        obj = _TestDressup(_TestBase(commands))
        ObjectDressup.execute(ObjectDressup.__new__(ObjectDressup), obj)
        return obj.Path.Commands

    def assertArcsValid(self, commands):
        """Each arc has the same radius at its start and its end. The twist uses
        the arc length of the offset, not the chord, so allow that difference."""
        for start, center, end, _, _ in _arcs(commands):
            self.assertRoughly((start - center).Length, (end - center).Length, 1e-5)

    def assertTwistInto(self, commands, center, radius, startdeg, enddeg, cw):
        """The twist ends on the destination arc, OFFSET along it from the corner,
        and the rest of the arc is cut in the original direction."""
        arcs = _arcs(commands)
        # the first arc around the center is the rest of the destination arc;
        # the exit move adds a second one, the extension past its end
        dest = [a for a in arcs if (a[1] - center).Length < 1e-6]
        self.assertGreater(len(dest), 0, "destination arc missing")
        start, _, end, sweep, name = dest[0]

        theta = math.degrees(OFFSET / radius)
        expected = startdeg - theta if cw else startdeg + theta
        self.assertRoughly((start - _point(center, radius, expected)).Length, 0.0, 1e-6)
        self.assertRoughly((end - _point(center, radius, enddeg)).Length, 0.0, 1e-6)
        self.assertRoughly(math.degrees(sweep), abs(enddeg - startdeg) - theta, 1e-6)
        self.assertEqual(name, "G2" if cw else "G3")

    def assertTwistShort(self, commands, corner, cw):
        """The twist around the corner turns the short way, in the direction of the turn."""
        twists = [a for a in _arcs(commands) if (a[1] - corner).Length < 1e-6]
        self.assertEqual(len(twists), 1, "twist arc missing")
        _, _, _, sweep, name = twists[0]
        self.assertEqual(name, "G2" if cw else "G3")
        self.assertLess(math.degrees(sweep), 180.0)

    def test00(self):
        """A line into a G2 arc: the twist and the rest of the arc stay on the arc (#21800)."""
        center = FreeCAD.Vector(-10, 0, 0)
        end = _point(center, 20, -20)
        commands = self.dress(
            _cut(
                Path.Command("G1", {"X": 10, "Y": 0}),
                Path.Command("G2", {"X": end.x, "Y": end.y, "I": -20, "J": 0}),
            )
        )
        self.assertArcsValid(commands)
        self.assertTwistInto(commands, center, 20, 0, -20, cw=True)
        self.assertTwistShort(commands, FreeCAD.Vector(10, 0, 0), cw=True)

    def test01(self):
        """A line into a G3 arc: the twist goes forward along the arc, not back."""
        center = FreeCAD.Vector(-10, 0, 0)
        end = _point(center, 20, 20)
        commands = self.dress(
            _cut(
                Path.Command("G1", {"X": 10, "Y": 0}),
                Path.Command("G3", {"X": end.x, "Y": end.y, "I": -20, "J": 0}),
            )
        )
        self.assertArcsValid(commands)
        self.assertTwistInto(commands, center, 20, 0, 20, cw=False)
        self.assertTwistShort(commands, FreeCAD.Vector(10, 0, 0), cw=False)

    def test02(self):
        """A G2 arc into a G2 arc: the direction comes from the destination arc."""
        center = FreeCAD.Vector(10, -20, 0)
        end = _point(center, 20, 70)
        commands = self.dress(
            [
                Path.Command("G0", {"Z": SAFE}),
                Path.Command("G0", {"X": 0, "Y": 10}),
                Path.Command("G1", {"Z": DEPTH}),
                Path.Command("G2", {"X": 10, "Y": 0, "I": 0, "J": -10}),
                Path.Command("G2", {"X": end.x, "Y": end.y, "I": 0, "J": -20}),
                Path.Command("G0", {"Z": SAFE}),
            ]
        )
        self.assertArcsValid(commands)
        self.assertTwistInto(commands, center, 20, 90, 70, cw=True)
        # the knife moves along -Y at the corner and turns left to +X
        self.assertTwistShort(commands, FreeCAD.Vector(10, 0, 0), cw=False)

    def test03(self):
        """A G2 arc into a line: the twist turns the short way (left)."""
        commands = self.dress(
            [
                Path.Command("G0", {"Z": SAFE}),
                Path.Command("G0", {"X": 0, "Y": 10}),
                Path.Command("G1", {"Z": DEPTH}),
                Path.Command("G2", {"X": 10, "Y": 0, "I": 0, "J": -10}),
                Path.Command("G1", {"X": 20, "Y": 0}),
                Path.Command("G0", {"Z": SAFE}),
            ]
        )
        self.assertArcsValid(commands)
        self.assertTwistShort(commands, FreeCAD.Vector(10, 0, 0), cw=False)

    def test04(self):
        """A G3 arc into a line: the twist turns the short way (right)."""
        commands = self.dress(
            [
                Path.Command("G0", {"Z": SAFE}),
                Path.Command("G0", {"X": 0, "Y": -10}),
                Path.Command("G1", {"Z": DEPTH}),
                Path.Command("G3", {"X": 10, "Y": 0, "I": 0, "J": 10}),
                Path.Command("G1", {"X": 20, "Y": 0}),
                Path.Command("G0", {"Z": SAFE}),
            ]
        )
        self.assertArcsValid(commands)
        self.assertTwistShort(commands, FreeCAD.Vector(10, 0, 0), cw=True)
