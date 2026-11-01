# SPDX-License-Identifier: LGPL-2.1-or-later

from types import SimpleNamespace
import math

import Path
from Path.Dressup.ProfileTrim import DressupProfileTrim

from CAMTests.PathTestUtils import PathTestBase


class TestDressupProfileTrim(PathTestBase):
    def testStartAndStopOffsetsTrimProfileButKeepPlungeAndRetract(self):
        base = SimpleNamespace(
            Path=Path.Path(
                [
                    Path.Command("G0", {"X": 0, "Y": 0, "Z": 5}),
                    Path.Command("G0", {"Z": 0}),
                    Path.Command("G1", {"X": 0, "Y": 0, "Z": -1}),
                    Path.Command("G1", {"X": 100}),
                    Path.Command("G1", {"Z": 5}),
                ]
            )
        )
        obj = SimpleNamespace(
            Base=base,
            StartOffset=SimpleNamespace(Value=10),
            StopOffset=SimpleNamespace(Value=20),
        )

        DressupProfileTrim().execute(obj)

        commands = obj.Path.Commands
        self.assertEqual(commands[0].Name, "G0")
        self.assertEqual(commands[0].Parameters["X"], 10)
        self.assertEqual(commands[1].Parameters, {"Z": 0})
        self.assertEqual(commands[2].Name, "G1")
        self.assertEqual(commands[2].Parameters["X"], 10)
        self.assertEqual(commands[2].Parameters["Z"], -1)
        self.assertEqual(commands[3].Name, "G1")
        self.assertEqual(commands[3].Parameters["X"], 80)
        self.assertEqual(commands[4].Parameters, {"Z": 5})

    def testArcMovesAreTrimmedAlongTheirArcLength(self):
        base = SimpleNamespace(
            Path=Path.Path(
                [
                    Path.Command("G0", {"X": 10, "Y": 0, "Z": 5}),
                    Path.Command("G1", {"Z": -1}),
                    Path.Command("G3", {"X": 0, "Y": 10, "I": -10, "J": 0}),
                    Path.Command("G1", {"Z": 5}),
                ]
            )
        )
        obj = SimpleNamespace(
            Base=base,
            StartOffset=SimpleNamespace(Value=2),
            StopOffset=SimpleNamespace(Value=2),
        )

        DressupProfileTrim().execute(obj)

        arc = next(command for command in obj.Path.Commands if command.Name == "G3")
        self.assertAlmostEqual(arc.Parameters["X"], 10 * math.cos(math.pi / 2 - 0.2))
        self.assertAlmostEqual(arc.Parameters["Y"], 10 * math.sin(math.pi / 2 - 0.2))
        self.assertAlmostEqual(arc.Parameters["I"], -10 * math.cos(0.2))
        self.assertAlmostEqual(arc.Parameters["J"], -10 * math.sin(0.2))
