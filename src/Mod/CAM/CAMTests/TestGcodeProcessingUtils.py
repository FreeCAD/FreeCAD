# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 sliptonic <shopinthewoods@gmail.com>               *
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

import unittest

from Path.Post.GcodeProcessingUtils import (
    insert_line_numbers,
    suppress_redundant_axes_words,
    filter_inefficient_moves,
    deduplicate_repeated_commands,
    NumberGenerator,
    NO_COLLAPSE_MARKER,
)


class TestInsertLineNumbers(unittest.TestCase):
    """Test the insert_line_numbers function."""

    def test_empty_list(self):
        """Test with empty list."""
        result = insert_line_numbers([])
        self.assertEqual(result, [])

    def test_single_line(self):
        """Test with single G-code line."""
        gcode = ["G0 X10 Y20"]
        result = insert_line_numbers(gcode)
        expected = ["N10 G0 X10 Y20"]
        self.assertEqual(result, expected)

    def test_multiple_lines(self):
        """Test with multiple G-code lines."""
        gcode = ["G0 X0 Y0 Z0", "G1 X10 Y20 Z5", "G0 Z10"]
        result = insert_line_numbers(gcode)
        expected = ["N10 G0 X0 Y0 Z0", "N20 G1 X10 Y20 Z5", "N30 G0 Z10"]
        self.assertEqual(result, expected)

    def test_skip_comments(self):
        """Test that comments are not numbered."""
        gcode = ["(Header comment)", "G0 X0 Y0", "(Inline comment)", "G1 X10 Y10"]
        result = insert_line_numbers(gcode)
        expected = ["(Header comment)", "N10 G0 X0 Y0", "(Inline comment)", "N20 G1 X10 Y10"]
        self.assertEqual(result, expected)

    def test_skip_empty_lines(self):
        """Test that empty lines are not numbered."""
        gcode = ["", "G0 X0 Y0", "   ", "G1 X10 Y10"]
        result = insert_line_numbers(gcode)
        expected = ["", "N10 G0 X0 Y0", "   ", "N20 G1 X10 Y10"]
        self.assertEqual(result, expected)


class TestSuppressRedundantAxesWords(unittest.TestCase):
    """Test the suppress_redundant_axes_words function."""

    def test_empty_list(self):
        """Test with empty list."""
        result = suppress_redundant_axes_words([])
        self.assertEqual(result, [])

    def test_no_duplicates(self):
        """Test with no redundant axes (same as input)."""
        gcode = ["G1 X10 Y20 Z5"]
        result = suppress_redundant_axes_words(gcode)
        expected = ["G1 X10 Y20 Z5"]
        self.assertEqual(result, expected)

    def test_suppress_redundant_axes(self):
        """Test suppressing redundant axis values based on current position."""
        gcode = [
            "G0 X0 Y0 Z0",  # Set initial position
            "G1 X0 Y10 Z0",  # X is redundant, Y changes
            "G1 X0 Y10 Z5",  # X and Y redundant, Z changes
            "G1 X10 Y10 Z5",  # Only X changes
        ]
        result = suppress_redundant_axes_words(gcode)
        expected = [
            "G0 X0 Y0 Z0",  # All axes are new
            "G1 Y10",  # X redundant, Y changes
            "G1 Z5",  # X and Y redundant, Z changes
            "G1 X10",  # Only X changes
        ]
        self.assertEqual(result, expected)

    def test_suppress_redundant_feed_rates(self):
        """Test suppressing redundant feed rate values."""
        gcode = [
            "G0 X0 Y0 Z0 F1000",  # Set initial feed rate
            "G1 X10 Y0 Z0 F1000",  # Feed rate redundant
            "G1 X20 Y0 Z0 F2000",  # Feed rate changes
            "G1 X30 Y0 Z0 F2000",  # Feed rate redundant again
        ]
        result = suppress_redundant_axes_words(gcode)
        expected = [
            "G0 X0 Y0 Z0 F1000",  # Feed rate is new
            "G1 X10",  # Feed rate redundant
            "G1 X20 F2000",  # Feed rate changes
            "G1 X30",  # Feed rate redundant
        ]
        self.assertEqual(result, expected)

    def test_mixed_axes_and_feed_suppression(self):
        """Test suppressing both redundant axes and feed rates."""
        gcode = [
            "G0 X0 Y0 Z0 F1000",  # Set initial state
            "G1 X0 Y10 Z0 F1000",  # X and F redundant, Y changes
            "G1 X0 Y10 Z5 F1000",  # X, Y, F redundant, Z changes
            "G1 X10 Y10 Z5 F2000",  # X, Y, Z redundant, F changes
        ]
        result = suppress_redundant_axes_words(gcode)
        expected = [
            "G0 X0 Y0 Z0 F1000",  # All new
            "G1 Y10",  # X and F redundant
            "G1 Z5",  # X, Y, F redundant
            "G1 X10 F2000",  # X, Y, Z redundant
        ]
        self.assertEqual(result, expected)

    def test_different_axes(self):
        """Test with different axes (should keep all)."""
        gcode = ["G0 X0 Y0 Z0", "G1 X10 Y20 Z5 A30 B40"]
        result = suppress_redundant_axes_words(gcode)
        expected = ["G0 X0 Y0 Z0", "G1 X10 Y20 Z5 A30 B40"]
        self.assertEqual(result, expected)

    def test_skip_comments(self):
        """Test that comments are unchanged."""
        gcode = ["(Header comment)", "G0 X0 Y0 Z0", "G1 X0 Y10 Z0", "(Inline comment)"]
        result = suppress_redundant_axes_words(gcode)
        expected = ["(Header comment)", "G0 X0 Y0 Z0", "G1 Y10", "(Inline comment)"]
        self.assertEqual(result, expected)

    def test_blockdelete_slash_preservation(self):
        """Test that leading slashes (blockdelete mode) are preserved."""
        gcode = [
            "G0 X0 Y0 Z0",  # Normal line
            "/G1 X0 Y10 Z0",  # Blockdelete line
            "/G1 X0 Y10 Z5",  # Blockdelete with redundant axes
            "G1 X10 Y10 Z5",  # Normal line
        ]
        result = suppress_redundant_axes_words(gcode)
        expected = [
            "G0 X0 Y0 Z0",  # Normal
            "/G1 Y10",  # Blockdelete preserved, X redundant
            "/G1 Z5",  # Blockdelete preserved, X,Y redundant
            "G1 X10",  # Normal, X changes
        ]
        self.assertEqual(result, expected)


class TestFilterInefficientMoves(unittest.TestCase):
    """Test the filter_inefficient_moves function."""

    def test_empty_list(self):
        """Test with empty list."""
        result = filter_inefficient_moves([])
        self.assertEqual(result, [])

    def test_keep_different_moves(self):
        """Test keeping moves to different positions."""
        gcode = ["G0 X0 Y0 Z0", "G1 X10 Y20 Z5", "G0 X20 Y30 Z10"]
        result = filter_inefficient_moves(gcode)
        expected = ["G0 X0 Y0 Z0", "G1 X10 Y20 Z5", "G0 X20 Y30 Z10"]
        self.assertEqual(result, expected)

    def test_filter_same_position_moves(self):
        """Test that same position moves are not filtered (only rapid chains are optimized)."""
        gcode = [
            "G0 X10 Y20 Z5",
            "G1 X10 Y20 Z5",  # G1 to same position - kept (not a rapid move)
            "G0 X10 Y20 Z5",  # G0 to same position - would be redundant but not in a chain
        ]
        result = filter_inefficient_moves(gcode)
        expected = [
            "G0 X10 Y20 Z5",
            "G1 X10 Y20 Z5",  # G1 moves are preserved
            "G0 X10 Y20 Z5",  # Single G0 is preserved
        ]
        self.assertEqual(result, expected)

    def test_keep_non_move_commands(self):
        """Test keeping non-move commands."""
        gcode = ["M3 S1000", "G0 X10 Y20 Z5", "M5", "G1 X10 Y20 Z5"]  # G1 to same position - kept
        result = filter_inefficient_moves(gcode)
        expected = ["M3 S1000", "G0 X10 Y20 Z5", "M5", "G1 X10 Y20 Z5"]  # G1 moves are preserved
        self.assertEqual(result, expected)

    def test_partial_position_changes(self):
        """Test moves that change only some axes."""
        gcode = [
            "G0 X0 Y0 Z0",
            "G1 X10 Y0 Z0",  # Changes X
            "G1 X10 Y20 Z0",  # Changes Y
            "G1 X10 Y20 Z0",  # No change - kept (not rapid)
        ]
        result = filter_inefficient_moves(gcode)
        expected = [
            "G0 X0 Y0 Z0",
            "G1 X10 Y0 Z0",
            "G1 X10 Y20 Z0",
            "G1 X10 Y20 Z0",  # G1 to same position is kept
        ]
        self.assertEqual(result, expected)

    def test_skip_comments(self):
        """Test that comments are preserved."""
        gcode = [
            "(Start)",
            "G0 X0 Y0 Z0",
            "(Comment)",
            "G1 X0 Y0 Z0",  # G1 to same position - kept
            "(End)",
        ]
        result = filter_inefficient_moves(gcode)
        expected = [
            "(Start)",
            "G0 X0 Y0 Z0",
            "(Comment)",
            "G1 X0 Y0 Z0",  # G1 moves are preserved
            "(End)",
        ]
        self.assertEqual(result, expected)

    def test_skip_empty_lines(self):
        """Test that empty lines are preserved."""
        gcode = ["", "G0 X10 Y20 Z5", "   ", "G1 X10 Y20 Z5"]  # G1 to same position - kept
        result = filter_inefficient_moves(gcode)
        expected = ["", "G0 X10 Y20 Z5", "   ", "G1 X10 Y20 Z5"]  # G1 moves are preserved
        self.assertEqual(result, expected)

    def test_optimize_single_axis_collapse(self):
        """Test collapsing a same-direction rapid chain on one axis."""
        gcode = ["G1 X0.0 F100", "G0 X10.0", "G0 X20.0", "G0 X30.0"]
        result = filter_inefficient_moves(gcode)
        expected = ["G1 X0.0 F100", "G0 X30.0"]  # Only last position kept
        self.assertEqual(result, expected)

    def test_unknown_start_prevents_collapse(self):
        """Test no collapse when the starting position is unknown.

        Without a known start, a direction reversal (e.g. an initial
        retract to safe height) cannot be ruled out.
        """
        gcode = ["G0 X10.0", "G0 X20.0", "G0 X30.0"]
        result = filter_inefficient_moves(gcode)
        expected = ["G0 X10.0", "G0 X20.0", "G0 X30.0"]
        self.assertEqual(result, expected)

    def test_optimize_multi_axis_no_collapse(self):
        """Test that multi-axis rapid chains do NOT collapse.

        Collapsing them would replace two path segments with a single direct
        move through different territory.
        """
        gcode = ["G0 X10.0 Y10.0", "G0 X20.0 Y20.0"]
        result = filter_inefficient_moves(gcode)
        expected = ["G0 X10.0 Y10.0", "G0 X20.0 Y20.0"]
        self.assertEqual(result, expected)

    def test_optimize_with_side_effects(self):
        """Test no collapsing when side effects are present."""
        gcode = [
            "G0 Z10.0",
            "M6 T1",  # Tool change, has side effect
            "G0 Z5.0",
        ]
        result = filter_inefficient_moves(gcode)
        expected = [
            "G0 Z10.0",
            "M6 T1",  # Side effect should flush chain
            "G0 Z5.0",
        ]
        self.assertEqual(result, expected)

    def test_optimize_with_fixture_side_effects(self):
        """Test no collapsing when fixture side effects are present."""
        gcode = [
            "G0 X10.0",
            "G56",  # Fixture change, has side effect
            "G0 X20.0",
        ]
        result = filter_inefficient_moves(gcode)
        expected = [
            "G0 X10.0",
            "G56",  # Side effect should flush chain
            "G0 X20.0",
        ]
        self.assertEqual(result, expected)

    def test_optimize_empty_list(self):
        """Test optimization with empty command list."""
        result = filter_inefficient_moves([])
        self.assertEqual(result, [])

    def test_optimize_single_command(self):
        """Test optimization with a single command."""
        gcode = ["G0 X10.0"]
        result = filter_inefficient_moves(gcode)
        expected = ["G0 X10.0"]
        self.assertEqual(result, expected)

    def test_optimize_mixed_sequence(self):
        """Test mixed sequence with rapid and side effect commands."""
        gcode = [
            "G1 X0.0 F100",
            "G0 X10.0",
            "G0 X20.0",
            "M3 S1000",  # Spindle on, side effect
            "G0 X30.0",
        ]
        result = filter_inefficient_moves(gcode)
        expected = [
            "G1 X0.0 F100",
            "G0 X20.0",  # First chain collapses to last position
            "M3 S1000",  # Side effect
            "G0 X30.0",  # New move after side effect
        ]
        self.assertEqual(result, expected)

    def test_optimize_linear_group_no_collapse(self):
        """Test that rapids changing several linear axes are preserved."""
        gcode = [
            "G0 X10.0 Y10.0 Z10.0",
            "G0 X20.0 Y20.0 Z20.0",  # All linear axes change
        ]
        result = filter_inefficient_moves(gcode)
        expected = ["G0 X10.0 Y10.0 Z10.0", "G0 X20.0 Y20.0 Z20.0"]
        self.assertEqual(result, expected)

    def test_optimize_rotary_group_no_collapse(self):
        """Test that rapids changing several rotary axes are preserved."""
        gcode = [
            "G0 A10.0 B10.0 C10.0",
            "G0 A20.0 B20.0 C20.0",  # All rotary axes change
        ]
        result = filter_inefficient_moves(gcode)
        expected = ["G0 A10.0 B10.0 C10.0", "G0 A20.0 B20.0 C20.0"]
        self.assertEqual(result, expected)

    def test_retract_then_traverse_preserved(self):
        """Test that a retract rapid followed by a traverse rapid is preserved.

        This is the standard between-operations pattern; dropping the Z
        retract would drag the tool through the stock at cutting depth.
        """
        gcode = [
            "G1 X50.0 Y50.0 Z-2.0 F500",
            "G0 Z15.0",  # retract
            "G0 X0.0 Y0.0",  # traverse at safe height
        ]
        result = filter_inefficient_moves(gcode)
        self.assertEqual(result, gcode)

    def test_single_axis_runs_collapse_independently(self):
        """Test that a chain splits into single-axis runs, each collapsed."""
        gcode = [
            "G1 X0.0 Y0.0 Z0.0 F100",
            "G0 X10.0",
            "G0 X20.0",
            "G0 Y5.0",
            "G0 Y15.0",
        ]
        result = filter_inefficient_moves(gcode)
        expected = ["G1 X0.0 Y0.0 Z0.0 F100", "G0 X20.0", "G0 Y15.0"]
        self.assertEqual(result, expected)

    def test_direction_reversal_preserved(self):
        """Test that a single-axis direction reversal is never collapsed.

        An excursion is intentional motion; its purpose is the excursion
        itself, not its endpoint.
        """
        gcode = ["G1 X0.0 F100", "G0 X10.0", "G0 X30.0", "G0 X20.0"]
        result = filter_inefficient_moves(gcode)
        expected = ["G1 X0.0 F100", "G0 X10.0", "G0 X30.0", "G0 X20.0"]
        self.assertEqual(result, expected)

    def test_peck_drill_retracts_preserved(self):
        """Test that expanded peck-drilling rapids are preserved.

        The drill cycle expander converts G83 into feed and rapid moves;
        the rapid out of the hole clears chips and the rapid back in
        resumes the peck. Both reverse direction on Z and must survive.
        """
        gcode = [
            "G0 X10.0 Y10.0",
            "G0 Z2.0",
            "G1 Z-5.0 F100",  # first peck
            "G0 Z2.0",  # rapid out to clear chips
            "G0 Z-4.5",  # rapid back to just above the bottom
            "G1 Z-10.0 F100",  # second peck
            "G0 Z2.0",  # final retract
        ]
        result = filter_inefficient_moves(gcode)
        self.assertEqual(result, gcode)

    def test_endpoint_word_required_on_survivor(self):
        """Test no collapse when the last line lacks the changing axis word."""
        gcode = [
            "G1 X0.0 Y0.0 F100",
            "G0 X10.0",
            "G0 Y0.0",  # redundant Y word; X endpoint not stated here
        ]
        result = filter_inefficient_moves(gcode)
        expected = ["G1 X0.0 Y0.0 F100", "G0 X10.0", "G0 Y0.0"]
        self.assertEqual(result, expected)

    def test_unknown_baseline_prevents_collapse(self):
        """Test that axes with unknown starting position count as changing."""
        gcode = ["G0 X10.0 Y5.0", "G0 X20.0"]  # Y start unknown
        result = filter_inefficient_moves(gcode)
        expected = ["G0 X10.0 Y5.0", "G0 X20.0"]
        self.assertEqual(result, expected)

    def test_incremental_mode_disables_collapsing(self):
        """Test that G91 suspends collapsing; repeated words are real motion."""
        gcode = ["G91", "G0 X10.0", "G0 X10.0"]
        result = filter_inefficient_moves(gcode)
        expected = ["G91", "G0 X10.0", "G0 X10.0"]
        self.assertEqual(result, expected)

    def test_modal_rapid_chain_collapses_with_motion_word(self):
        """Test that bare modal rapids collapse and keep a G0 word."""
        gcode = ["G1 X0.0 F100", "G0 X10.0", "X20.0", "X30.0"]
        result = filter_inefficient_moves(gcode)
        expected = ["G1 X0.0 F100", "G0 X30.0"]
        self.assertEqual(result, expected)

    def test_no_op_rapids_keep_only_last(self):
        """Test that rapids to the current known position collapse to one."""
        gcode = [
            "G1 X10.0 Y10.0 F100",
            "G0 X10.0 Y10.0",
            "G0 X10.0 Y10.0",
        ]
        result = filter_inefficient_moves(gcode)
        expected = ["G1 X10.0 Y10.0 F100", "G0 X10.0 Y10.0"]
        self.assertEqual(result, expected)

    def test_no_collapse_annotation_blocks_collapse(self):
        """Test that an annotated rapid is never collapsed, even when the
        run is monotonic, and that the marker is stripped from output."""
        gcode = [
            "G1 X0.0 F100",
            f"G0 X10.0 {NO_COLLAPSE_MARKER}",
            "G0 X20.0",
            "G0 X30.0",
        ]
        result = filter_inefficient_moves(gcode)
        expected = [
            "G1 X0.0 F100",
            "G0 X10.0",  # protected, marker stripped
            "G0 X30.0",  # remaining unprotected run still collapses
        ]
        self.assertEqual(result, expected)

    def test_no_collapse_marker_stripped_everywhere(self):
        """Test that the marker is stripped even outside of chains."""
        gcode = [f"G0 Z2.0 {NO_COLLAPSE_MARKER}", f"G1 Z-5.0 F100 {NO_COLLAPSE_MARKER}"]
        result = filter_inefficient_moves(gcode)
        expected = ["G0 Z2.0", "G1 Z-5.0 F100"]
        self.assertEqual(result, expected)

    def test_blockdelete_flushes_chain(self):
        """Test that block-deleted lines are passed through and end a chain."""
        gcode = ["G0 X10.0", "/G0 X20.0", "G0 X30.0"]
        result = filter_inefficient_moves(gcode)
        expected = ["G0 X10.0", "/G0 X20.0", "G0 X30.0"]
        self.assertEqual(result, expected)

    def test_optimize_mixed_axes_no_collapse(self):
        """Test that mixed linear/rotary changes don't collapse."""
        gcode = [
            "G0 X10.0 A10.0",
            "G0 X20.0 A20.0",
            "G0 Y10.0 B10.0",  # Different axes
        ]
        result = filter_inefficient_moves(gcode)
        expected = [
            "G0 X10.0 A10.0",
            "G0 X20.0 A20.0",
            "G0 Y10.0 B10.0",
        ]  # All kept since mixed axes across groups
        self.assertEqual(result, expected)


class TestNumberGenerator(unittest.TestCase):
    """Test the NumberGenerator class."""

    def test010_default_initialization(self):
        """Test NumberGenerator initializes with default parameters."""
        gen = NumberGenerator()

        self.assertEqual(gen._template, "{}")
        self.assertEqual(gen._start, 1)
        self.assertEqual(gen._increment, 1)
        self.assertEqual(gen._current, 1)

    def test020_custom_initialization(self):
        """Test NumberGenerator with custom parameters."""
        gen = NumberGenerator(template="N{:04d}", start=100, increment=10)

        self.assertEqual(gen._template, "N{:04d}")
        self.assertEqual(gen._start, 100)
        self.assertEqual(gen._increment, 10)
        self.assertEqual(gen._current, 100)

    def test030_get_sequence_default(self):
        """Test get() method with default parameters."""
        gen = NumberGenerator()

        # First call
        self.assertEqual(gen.get(), "1")
        self.assertEqual(gen._current, 2)

        # Second call
        self.assertEqual(gen.get(), "2")
        self.assertEqual(gen._current, 3)

        # Third call
        self.assertEqual(gen.get(), "3")
        self.assertEqual(gen._current, 4)

    def test040_get_sequence_custom_template(self):
        """Test get() method with custom template."""
        gen = NumberGenerator(template="N{:03d}")

        self.assertEqual(gen.get(), "N001")
        self.assertEqual(gen.get(), "N002")
        self.assertEqual(gen.get(), "N003")

    def test050_get_sequence_custom_start_increment(self):
        """Test get() method with custom start and increment."""
        gen = NumberGenerator(start=100, increment=5)

        self.assertEqual(gen.get(), "100")
        self.assertEqual(gen.get(), "105")
        self.assertEqual(gen.get(), "110")

    def test060_reset_functionality(self):
        """Test reset() method."""
        gen = NumberGenerator(start=10, increment=2)

        # Generate some numbers
        self.assertEqual(gen.get(), "10")
        self.assertEqual(gen.get(), "12")
        self.assertEqual(gen.get(), "14")

        # Reset
        gen.reset()
        self.assertEqual(gen._current, 10)

        # Generate again from start
        self.assertEqual(gen.get(), "10")
        self.assertEqual(gen.get(), "12")

    def test070_gcode_line_numbers(self):
        """Test typical G-code line number generation."""
        gen = NumberGenerator(template="N{:04d}", start=100, increment=10)

        self.assertEqual(gen.get(), "N0100")
        self.assertEqual(gen.get(), "N0110")
        self.assertEqual(gen.get(), "N0120")
        self.assertEqual(gen.get(), "N0130")

    def test080_zero_start(self):
        """Test with zero start value."""
        gen = NumberGenerator(start=0)

        self.assertEqual(gen.get(), "0")
        self.assertEqual(gen.get(), "1")
        self.assertEqual(gen.get(), "2")

    def test090_negative_values(self):
        """Test with negative start and increment."""
        gen = NumberGenerator(start=-10, increment=-1)

        self.assertEqual(gen.get(), "-10")
        self.assertEqual(gen.get(), "-11")
        self.assertEqual(gen.get(), "-12")

    def test100_large_numbers(self):
        """Test with large numbers."""
        gen = NumberGenerator(start=10000, increment=1000)

        self.assertEqual(gen.get(), "10000")
        self.assertEqual(gen.get(), "11000")
        self.assertEqual(gen.get(), "12000")


class TestDeduplicateRepeatedCommands(unittest.TestCase):
    """Test the deduplicate_repeated_commands function for modal G-code output."""

    def test_modal_consecutive_same_commands(self):
        """Test that consecutive same commands have command word removed (modal behavior)."""
        gcode = ["G1 X10.0 Y20.0", "G1 X30.0 Y40.0", "G1 X50.0 Y60.0"]
        result = deduplicate_repeated_commands(gcode)
        expected = [
            "G1 X10.0 Y20.0",  # First G1 - full command
            "X30.0 Y40.0",  # G1 removed (modal)
            "X50.0 Y60.0",  # G1 removed (modal)
        ]
        self.assertEqual(result, expected)

    def test_modal_different_commands(self):
        """Test that different commands are output with full command word."""
        gcode = ["G1 X10.0", "G1 X20.0", "G0 Z5.0", "G0 Z10.0"]
        result = deduplicate_repeated_commands(gcode)
        expected = [
            "G1 X10.0",  # First G1
            "X20.0",  # G1 removed
            "G0 Z5.0",  # Different command - full
            "Z10.0",  # G0 removed
        ]
        self.assertEqual(result, expected)

    def test_modal_with_comments(self):
        """Test that comments are preserved and don't affect modal state."""
        gcode = ["G1 X10.0", "(Comment)", "G1 X20.0", "G1 X30.0"]
        result = deduplicate_repeated_commands(gcode)
        expected = [
            "G1 X10.0",
            "(Comment)",
            "X20.0",  # G1 removed (modal continues)
            "X30.0",  # G1 removed
        ]
        self.assertEqual(result, expected)

    def test_modal_with_empty_lines(self):
        """Test that empty lines are preserved."""
        gcode = ["G1 X10.0", "", "G1 X20.0"]
        result = deduplicate_repeated_commands(gcode)
        expected = ["G1 X10.0", "", "X20.0"]  # G1 removed
        self.assertEqual(result, expected)

    def test_modal_command_without_parameters(self):
        """Test commands without parameters."""
        gcode = ["G80", "G80"]
        result = deduplicate_repeated_commands(gcode)
        expected = ["G80"]  # First one kept, second removed (no params to output)
        self.assertEqual(result, expected)

    def test_modal_mixed_commands(self):
        """Test realistic G-code with mixed commands."""
        gcode = ["G0 X0.0 Y0.0", "G0 Z5.0", "G1 X10.0 F100.0", "G1 Y10.0", "G1 X0.0", "G0 Z20.0"]
        result = deduplicate_repeated_commands(gcode)
        expected = [
            "G0 X0.0 Y0.0",
            "Z5.0",  # G0 removed
            "G1 X10.0 F100.0",
            "Y10.0",  # G1 removed
            "X0.0",  # G1 removed
            "G0 Z20.0",
        ]
        self.assertEqual(result, expected)

    @unittest.skip("FIXME")
    def test_modal_blockdelete(self):
        """Test that blockdelete prefix is handled correctly."""
        gcode = ["/G1 X10.0", "/G1 X20.0"]
        result = deduplicate_repeated_commands(gcode)
        # Blockdelete commands should still follow modal rules
        expected = ["/G1 X10.0", "/G1 X20.0"]  # Full line kept (blockdelete handling)
        self.assertEqual(result, expected)
