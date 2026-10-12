# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Alan Grover <awgrover@gmail.com>                   *
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

from collections.abc import Iterable

import unittest
import Path
from Path.Post.PathOptimizationUtils import modal_gcode, modal_axis, collapse_g0


def _pathcommand_eq(a, b):
    # ad-hoc __eq__ since Path.Command doesn't have it
    if isinstance(a, Iterable):
        a = list(a)
        b = list(b)
        return len(a) == len(b) and not any(not _pathcommand_eq(*ab) for ab in zip(a, b))
    if a is None:
        return b is None
    else:
        return a.Name == b.Name and a.Parameters == b.Parameters and a.Annotations == b.Annotations


def _modal_gcode_list(
    commands: list[Path.Command], previous_command: Path.Command
) -> list[Path.Command]:
    """
    Convenience function for modal_gcode(), to dedup a list of Path.Commands.
    Correctly elides if consecutive commands are the same .Name and have no .Parameters

    Returns previous_command, new_list
        See modal_gcode()
    """
    new_list = []
    previous_command = None
    for command in commands:
        previous_command, deduped_command = modal_gcode(command, previous_command)
        if deduped_command:
            new_list.append(deduped_command)
    return previous_command, new_list


class TestModalAxis(unittest.TestCase):
    """Test the modal_axis function."""

    def _list_modal_axis(self, commands: list[Path.Command]) -> list[Path.Command]:
        """Convenience modal_axis(), for a list"""
        new_list = []
        previous_command = None  # FIXME: lift modal_gcode convenience
        for command in commands:
            previous_command, deduped_command = modal_axis(command, previous_command)
            if deduped_command:
                new_list.append(deduped_command)
        return previous_command, new_list

    def test_first(self):
        """Test that a single command is unaltered"""
        command = Path.Command("G1 X10 Y20 Z5")
        _, new_command = modal_axis(command, None)
        expected = Path.Command("G1 X10 Y20 Z5")
        self.assertTrue(
            _pathcommand_eq([new_command], [expected]),
            f"\nExpected\n{expected}\nSaw:\n{new_command}",
        )

    def test_suppress_redundant_axes(self):
        """Test suppressing redundant axis values based on current position."""
        commands = [
            Path.Command("G0 X0 Y0 Z0"),  # Set initial position
            Path.Command("G1 X0 Y10 Z0"),  # X is redundant, Y changes
            Path.Command("G1 Y11"),  # x,z unchanged, new y
            Path.Command("G1 X0 Y11 Z5"),  # X and Y redundant, Z changes
            Path.Command("G1 X10 Y11 Z5"),  # Only X changes
            Path.Command("", {"X": 12, "Y": 11, "Z": 5}),  # No .Name, Only X changes
        ]
        _, new_commands = self._list_modal_axis(commands)
        expected = [
            Path.Command("G0 X0 Y0 Z0"),  # All axes are new
            Path.Command("G1 Y10"),  # X redundant, Y changes
            Path.Command("G1 Y11"),  # x,z unchanged, new y
            Path.Command("G1 Z5"),  # X and Y redundant, Z changes
            Path.Command("G1 X10"),  # Only X changes
            Path.Command("", {"X": 12}),  # Only X changes
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_drill_doesnt_modal(self):
        """Canned-drill doesn't get axis removed"""
        commands = [
            Path.Command("G0 X0 Y0 Z0"),  # Set initial position
            Path.Command("G82 X0 Y2 Z0"),  # X is redundant, Y changes, but drill
            Path.Command("G1 X0 Y11 Z0"),  # x,z unchanged, new y : too bad, drill is barrier
        ]
        _, new_commands = self._list_modal_axis(commands)
        expected = [
            Path.Command("G0 X0 Y0 Z0"),  # All axes are new
            Path.Command("G82 X0 Y2 Z0"),  # X redundant, Y changes, but drill
            Path.Command("G1 X0 Y11 Z0"),  # x,z unchanged, new y : too bad, drill is barrier
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_suppress_redundant_feed_rates(self):
        """Test suppressing redundant feed rate values."""
        commands = [
            Path.Command("G0 X0 Y0 Z0 F1000"),  # Set initial feed rate
            Path.Command("G1 X10 Y0 Z0 F1000"),  # Feed rate redundant
            Path.Command("G1 X20 Y0 Z0 F2000"),  # Feed rate changes
            Path.Command("G1 X30 Y0 Z0 F2000"),  # Feed rate redundant again
        ]
        _, new_commands = self._list_modal_axis(commands)
        expected = [
            Path.Command("G0 X0 Y0 Z0 F1000"),  # Feed rate is new
            Path.Command("G1 X10"),  # Feed rate redundant
            Path.Command("G1 X20 F2000"),  # Feed rate changes
            Path.Command("G1 X30"),  # Feed rate redundant
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_mixed_axes_and_feed_suppression(self):
        """Test suppressing both redundant axes and feed rates."""
        commands = [
            Path.Command("G0 X0 Y0 Z0 F1000"),  # Set initial state
            Path.Command("G1 X0 Y10 Z0 F1000"),  # X and F redundant, Y changes
            Path.Command("G1 X0 Y10 Z5 F1000"),  # X, Y, F redundant, Z changes
            Path.Command("G1 X10 Y10 Z5 F2000"),  # X, Y, Z redundant, F changes
        ]
        _, new_commands = self._list_modal_axis(commands)
        expected = [
            Path.Command("G0 X0 Y0 Z0 F1000"),  # All new
            Path.Command("G1 Y10"),  # X and F redundant
            Path.Command("G1 Z5"),  # X, Y, F redundant
            Path.Command("G1 X10 F2000"),  # X, Y, Z redundant
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_different_axes(self):
        """Test with different axes (should keep all)."""
        commands = [
            Path.Command("G0 X0 Y0 Z0"),
            Path.Command("G1 X10 Y20 Z5 A30 B40"),
        ]
        _, new_commands = self._list_modal_axis(commands)
        expected = [
            Path.Command("G0 X0 Y0 Z0"),
            Path.Command("G1 X10 Y20 Z5 A30 B40"),
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_skip_comments(self):
        """Test that comments are unchanged."""
        commands = [
            Path.Command("(Header comment)"),
            Path.Command("G0 X0 Y0 Z0"),
            Path.Command("G1 X0 Y10 Z0"),
            Path.Command("(Inline comment)"),
        ]
        _, new_commands = self._list_modal_axis(commands)
        expected = [
            Path.Command("(Header comment)"),
            Path.Command("G0 X0 Y0 Z0"),
            Path.Command("G1 Y10"),
            Path.Command("(Inline comment)"),
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )


class TestCollapseG0(unittest.TestCase):
    """Test the collaps_g0 function."""

    def test_empty_list(self):
        """Test with empty list."""
        new_commands = collapse_g0([])
        self.assertEqual(list(new_commands), [])

    def test_at_least_3(self):
        """Need at least 1 establish + 2 g0's for collapse"""

        # establish
        gcode = ["G1 X1 Y2 Z3"]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [Path.Command(x) for x in gcode]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

        # 2x g1's
        gcode.extend(
            [
                "G1 X1 Y2 Z9",
                "G1 X1 Y2 Z10",
            ]
        )
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [Path.Command(x) for x in gcode]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

        # 1x g0
        gcode.append("G0 X1 Y2 Z4")
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [Path.Command(x) for x in gcode]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

        # Just 2x g0's
        gcode = [
            "G0 X1 Y2 Z3",
            "G0 X1 Y2 Z5",
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [Path.Command(x) for x in gcode]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_all_axis_same(self):
        # because count(diff) 0 is not the same as 1
        gcode = [
            "G1 X1 Y2 Z3",  # establish
            "G0 X1 Y2 Z3",
            "G0 X1 Y2 Z3",
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(x)
            for x in [
                "G1 X1 Y2 Z3",
                "G0 X1 Y2 Z3",
            ]
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_same_annotations(self):
        gcode = [
            "G0 X0 Y0 Z0",  # not start of chain, establishes XYZ
            "G0 X10.0 Y0 Z0",  # start chain, only x changed
            "G0 X20.0 Y0 Z0; ANNOT:'1'",  # doesn't continue
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [Path.Command(g) for g in gcode]  # Only first and last position kept
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

        gcode = [
            "G1 X0 Y0 Z0",  # not start of chain, establishes XYZ
            "G0 X10.0 Y0 Z0",  # start chain, only x changed
            "G0 X20.0 Y0 Z0; ANNOT:'1'",  # doesn't continue, restart chain
            "G0 X30.0 Y0 Z0; ANNOT:'1'",  # does continue
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(g)
            for g in [
                "G1 X0 Y0 Z0",  # not start of chain, establishes XYZ
                "G0 X10.0 Y0 Z0",  # start chain, only x changed
                "G0 X30.0 Y0 Z0; ANNOT:'1'",  # doesn't continue
            ]
        ]  # Only first and last position kept
        self.assertTrue(
            _pathcommand_eq(new_commands, expected),
            f"\nExpected\n{[x.toGCode() for x in expected]}\nSaw:\n{[x.toGCode() for x in new_commands]}",
        )

    def test_keep_different_moves(self):
        """Test keeping moves to different positions."""
        commands = [
            Path.Command("G0 X0 Y0 Z0"),
            Path.Command("G1 X10 Y20 Z5"),
            Path.Command("G0 X20 Y30 Z10"),
        ]
        new_commands = list(collapse_g0(commands))
        expected = [
            Path.Command("G0 X0 Y0 Z0"),
            Path.Command("G1 X10 Y20 Z5"),
            Path.Command("G0 X20 Y30 Z10"),
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_filter_same_position_moves(self):
        """Test that same position moves are not filtered (only rapid chains are optimized)."""
        commands = [
            Path.Command("G0 X10 Y20 Z5"),
            Path.Command("G1 X10 Y20 Z5"),  # G1 to same position - kept (not a rapid move)
            Path.Command(
                "G0 X10 Y20 Z5"
            ),  # G0 to same position - would be redundant but not in a chain
        ]
        new_commands = list(collapse_g0(commands))
        expected = [
            Path.Command("G0 X10 Y20 Z5"),
            Path.Command("G1 X10 Y20 Z5"),  # G1 moves are preserved
            Path.Command("G0 X10 Y20 Z5"),  # Single G0 is preserved
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_keep_non_move_commands(self):
        """Test keeping non-move commands."""
        gcode = ["M3 S1000", "G0 X10 Y20 Z5", "M5", "G1 X10 Y20 Z5"]  # G1 to same position - kept
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(g) for g in ("M3 S1000", "G0 X10 Y20 Z5", "M5", "G1 X10 Y20 Z5")
        ]  # G1 moves are preserved
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_partial_position_changes(self):
        """Test moves that change only some axes."""
        gcode = [
            "G0 X0 Y0 Z0",
            "G1 X10 Y0 Z0",  # Changes X
            "G1 X10 Y20 Z0",  # Changes Y
            "G1 X10 Y20 Z0",  # No change - kept (not rapid)
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(g)
            for g in [
                "G0 X0 Y0 Z0",
                "G1 X10 Y0 Z0",
                "G1 X10 Y20 Z0",
                "G1 X10 Y20 Z0",  # G1 to same position is kept
            ]
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_skip_comments(self):
        """Test that comments are preserved."""
        gcode = [
            "(Start)",
            "G0 X0 Y0 Z0",
            "(Comment)",
            "G1 X0 Y0 Z0",  # G1 to same position - kept
            "(End)",
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(g)
            for g in [
                "(Start)",
                "G0 X0 Y0 Z0",
                "(Comment)",
                "G1 X0 Y0 Z0",  # G1 moves are preserved
                "(End)",
            ]
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_skip_empty_lines(self):
        """Test that empty lines are preserved."""
        gcode = ["", "G0 X10 Y20 Z5", "", "G1 X10 Y20 Z5"]  # G1 to same position - kept
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(g) for g in ["", "G0 X10 Y20 Z5", "", "G1 X10 Y20 Z5"]
        ]  # G1 moves are preserved
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    # Collapse can happen in these:

    def test_optimize_single_axis_collapse(self):
        """Test collapsing rapid chain with single-axis changes."""

        # need initial position, so keeps 1st g0
        gcode = [
            "G0 X0 Y0 Z0",  # not start of chain, establishes XYZ
            "G0 X10.0 Y0 Z0",  # start chain, only x changed
            "G0 X20.0 Y0 Z0",  # continue, only x change
            "G0 X30.0 Y0 Z0",  # continue, only x change
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(g) for g in ["G0 X0 Y0 Z0", "G0 X30.0 Y0 Z0"]
        ]  # Only first and last position kept
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

        # uses the g1, so 1st g0 can collapse
        gcode = ["G1 X0 Y0 Z0", "G0 X10.0 Y0 Z0", "G0 X20.0 Y0 Z0", "G0 X30.0 Y0 Z0"]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [Path.Command(g) for g in ["G1 X0 Y0 Z0", "G0 X30.0 Y0 Z0"]]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

        # Even though each g0 changes only one axis, the whole chain has to be the same axis
        gcode = ["G1 X0 Y0 Z0", "G0 X10.0 Y0 Z0", "G0 X10.0 Y20 Z0", "G0 X30.0 Y0 Z0"]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(g)
            for g in ["G1 X0 Y0 Z0", "G0 X10.0 Y0 Z0", "G0 X10.0 Y20 Z0", "G0 X30.0 Y0 Z0"]
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_optimize_multi_axis_no_collapse(self):
        """Test that multi-axis rapid chains within linear group DON'T collapse."""
        gcode = ["G0 X10.0 Y10.0", "G0 X20.0 Y20.0"]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(g) for g in ["G0 X10.0 Y10.0", "G0 X20.0 Y20.0"]
        ]  # Collapsed to final position (both X,Y in linear group)
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_optimize_with_side_effects(self):
        """Test no collapsing when side effects are present."""
        gcode = [
            "G0 Z10.0",
            "M6 T1",  # Tool change, has side effect
            "G0 Z5.0",
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(g)
            for g in [
                "G0 Z10.0",
                "M6 T1",  # Side effect should flush chain
                "G0 Z5.0",
            ]
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_optimize_with_fixture_side_effects(self):
        """Test no collapsing when fixture side effects are present."""
        gcode = [
            "G0 X10.0",
            "G56",  # Fixture change, has side effect
            "G0 X20.0",
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(g)
            for g in [
                "G0 X10.0",
                "G56",  # Side effect should flush chain
                "G0 X20.0",
            ]
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_optimize_single_command(self):
        """Test optimization with a single command."""
        gcode = ["G0 X10.0"]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [Path.Command(g) for g in ["G0 X10.0"]]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_optimize_mixed_sequence(self):
        """Test mixed sequence with rapid and side effect commands."""
        gcode = [
            "G0 X10.0",  # can't start a chain, can't know what previous XYZ was
            "G0 X20.0",
            "M3 S1000",  # Spindle on, side effect
            "G0 X30.0",
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [Path.Command(g) for g in gcode]  # no changes
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_optimize_linear_group_no_collapse(self):
        """Test collapsing rapid moves within linear axis group (X,Y,Z)."""
        gcode = [
            "G1 X5 Y5 Z5",
            "G0 X10.0 Y10.0 Z10.0",
            "G0 X20.0 Y20.0 Z20.0",  # All linear axes change
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [Path.Command(g) for g in gcode]  # same
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_optimize_rotary_group_collapse(self):
        """Test collapsing rapid moves within rotary axis group (A,B,C)."""
        gcode = [
            "G1 A0 B0 C0",
            "G0 A10.0 B10.0 C10.0",
            "G0 A20.0 B20.0 C20.0",  # All rotary axes change
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [Path.Command(g) for g in gcode]  # no change
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

        # only 1 axis changes
        gcode = [
            "G1 A0 B0 C0",
            "G0 A0 B10.0 C0",
            "G0 A0 B20.0 C0",  # one rotary axes change
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(g)
            for g in [
                "G1 A0 B0 C0",
                "G0 A0 B20.0 C0",
            ]
        ]  # no change
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_optimize_mixed_axes_no_collapse(self):
        """Test that mixed linear/rotary changes don't collapse."""
        gcode = [
            "G0 X10.0 A10.0",  # establishes XYZ, but not start of chain
            "G0 X20.0 A20.0",  # Changed 2 axis, no chain
            "G0 Y10.0 B10.0",  # Different axes
        ]
        new_commands = list(collapse_g0([Path.Command(g) for g in gcode]))
        expected = [
            Path.Command(g)
            for g in [
                "G0 X10.0 A10.0",
                "G0 X20.0 A20.0",
                "G0 Y10.0 B10.0",
            ]
        ]  # All kept since mixed axes across groups
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )


class TestModalGCode(unittest.TestCase):
    """Test the modal_gcode function"""

    def test_modal_consecutive_same_commands(self):
        """Test that consecutive same commands have command word removed (modal behavior)."""

        commands = [
            Path.Command("G1 X10.0 Y20.0"),
            Path.Command("G1 X30.0 Y40.0"),
            Path.Command("G1 X50.0 Y60.0"),
        ]
        _, new_commands = _modal_gcode_list(commands, None)

        expected = [
            Path.Command("G1 X10.0 Y20.0"),
            Path.Command("", {"X": 30, "Y": 40}),  # First G1 - full command
            Path.Command("", {"X": 50, "Y": 60}),  # G1 removed (modal)
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_modal_different_commands(self):
        """Test that different commands are output with full command word."""
        commands = [
            Path.Command("G1 X10.0"),
            Path.Command("G1 X20.0"),
            Path.Command("G0 Z5.0"),
            Path.Command("G0 Z10.0"),
        ]
        _, new_commands = _modal_gcode_list(commands, None)
        expected = [
            Path.Command("G1 X10.0"),  # First G1
            Path.Command("", {"X": 20}),  # G1 removed
            Path.Command("G0 Z5.0"),  # Different command - full
            Path.Command("", {"Z": 10}),  # G0 removed
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_modal_with_comments(self):
        """Test that comments are preserved and don't affect modal state."""
        commands = [
            Path.Command("G1 X10.0"),
            Path.Command("(Comment)"),
            Path.Command("G1 X20.0"),
            Path.Command("G1 X30.0"),
        ]
        _, new_commands = _modal_gcode_list(commands, None)
        expected = [
            Path.Command("G1 X10.0"),
            Path.Command("(Comment)"),
            Path.Command("", {"X": 20.0}),  # G1 removed (modal continues)
            Path.Command("", {"X": 30.0}),  # G1 removed
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_modal_with_empty_lines(self):
        """Test that empty lines are preserved."""
        commands = [
            Path.Command("G1 X10.0"),
            Path.Command(""),
            Path.Command("G1 X20.0"),
        ]
        _, new_commands = _modal_gcode_list(commands, None)
        expected = [
            Path.Command("G1 X10.0"),
            Path.Command(""),
            Path.Command("", {"X": 20}),  # G1 removed
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_modal_command_without_parameters(self):
        """Test commands without parameters."""
        commands = [
            Path.Command("G80"),
            Path.Command("G80"),
        ]
        _, new_commands = _modal_gcode_list(commands, None)
        expected = [
            Path.Command(
                "G80"
            )  # First one kept, second removed (no params to output)  # First one kept, second removed (no params to output)
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    def test_modal_mixed_commands(self):
        """Test realistic G-code with mixed commands."""
        commands = [
            Path.Command("G0 X0.0 Y0.0"),
            Path.Command("G0 Z5.0"),
            Path.Command("G1 X10.0 F100.0"),
            Path.Command("G1 Y10.0"),
            Path.Command("G1 X0.0"),
            Path.Command("G0 Z20.0"),
        ]
        _, new_commands = _modal_gcode_list(commands, None)
        expected = [
            Path.Command("G0 X0.0 Y0.0"),
            Path.Command("", {"Z": 5}),  # G0 removed
            Path.Command("G1 X10.0 F100.0"),
            Path.Command("", {"Y": 10}),  # G1 removed
            Path.Command("", {"X": 0}),  # G1 removed
            Path.Command("G0 Z20.0"),
        ]
        self.assertTrue(
            _pathcommand_eq(new_commands, expected), f"\nExpected\n{expected}\nSaw:\n{new_commands}"
        )

    @unittest.skip("FIXME")
    def test_modal_blockdelete(self):
        """Test that blockdelete annotation is handled correctly."""
        # Not implemented anywhere, should be using Constants.ANNOT_BLOCKDELETE
