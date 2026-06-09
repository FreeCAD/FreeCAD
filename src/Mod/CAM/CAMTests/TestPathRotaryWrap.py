# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 <shopinthewoods@gmail.com>
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

"""Unit tests for rotary-axis wrap-strategy rewrite."""

import unittest

import Path
from CAMTests.PathTestUtils import PathTestBase

import Path.Base.Generator.rotary_wrap as rotary_wrap

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


def _unwound_path(axis="A", n_revs=12, samples_per_rev=72, x0=-50.0, dx_per_step=0.1):
    """Synthetic monotonic-A spiral: n_revs full revolutions of motion."""
    commands = []
    n = n_revs * samples_per_rev
    for k in range(n):
        a = (k * 360.0) / samples_per_rev
        x = x0 + k * dx_per_step
        commands.append(Path.Command("G1", {"X": x, "Y": 0.0, "Z": 10.0, axis: a, "F": 100.0}))
    return commands


class TestRotaryWrapStrategy(PathTestBase):
    def test00_unwound_passthrough(self):
        cmds = _unwound_path(n_revs=3)
        out = rotary_wrap.apply_wrap_strategy(cmds, "A", "unwound")
        self.assertEqual(len(out), len(cmds))
        for src, dst in zip(cmds, out):
            self.assertEqual(src.Name, dst.Name)
            self.assertAlmostEqual(src.Parameters["A"], dst.Parameters["A"])

    def test01_modulo_values_in_range(self):
        cmds = _unwound_path(n_revs=12)
        out = rotary_wrap.apply_wrap_strategy(cmds, "A", "modulo")
        self.assertEqual(len(out), len(cmds))
        for c in out:
            a = c.Parameters["A"]
            self.assertGreaterEqual(a, 0.0)
            self.assertLess(a, 360.0)

    def test02_modulo_preserves_xyz(self):
        cmds = _unwound_path(n_revs=2)
        out = rotary_wrap.apply_wrap_strategy(cmds, "A", "modulo")
        for src, dst in zip(cmds, out):
            self.assertAlmostEqual(src.Parameters["X"], dst.Parameters["X"])
            self.assertAlmostEqual(src.Parameters["Z"], dst.Parameters["Z"])

    def test03_rezero_values_in_range(self):
        cmds = _unwound_path(n_revs=12)
        out = rotary_wrap.apply_wrap_strategy(cmds, "A", "rezero")
        for c in out:
            a = c.Parameters.get("A")
            if a is None:
                continue
            self.assertGreaterEqual(a, 0.0)
            self.assertLess(a, 360.0)

    def test04_rezero_emits_no_g92_commands(self):
        """ADR-002 forbids G92 in the internal command list."""
        cmds = _unwound_path(n_revs=5)
        out = rotary_wrap.apply_wrap_strategy(cmds, "A", "rezero")
        forbidden = [c for c in out if c.Name in ("G92", "G92.1")]
        self.assertEqual(forbidden, [], "rezero must annotate, not inject G92/G92.1 commands")
        # Output length matches input length: no extra commands inserted.
        self.assertEqual(len(out), len(cmds))

    def test05_rezero_annotates_boundary_crossings(self):
        """One rotary_rezero annotation per revolution boundary crossed."""
        cmds = _unwound_path(n_revs=5)
        out = rotary_wrap.apply_wrap_strategy(cmds, "A", "rezero")
        annotated = [c for c in out if c.Annotations.get("rotary_rezero") == "A"]
        # 5 revs of motion cross 4 internal 360° boundaries.
        self.assertGreaterEqual(len(annotated), 4)
        self.assertLessEqual(len(annotated), 5)

    def test06_rezero_no_annotations_under_one_rev(self):
        cmds = _unwound_path(n_revs=1, samples_per_rev=36)
        out = rotary_wrap.apply_wrap_strategy(cmds, "A", "rezero")
        annotated = [c for c in out if c.Annotations.get("rotary_rezero")]
        self.assertEqual(len(annotated), 0)

    def test07_rezero_direction_preserved(self):
        """Wrapped deltas are positive within each frame (between annotations)."""
        cmds = _unwound_path(n_revs=4, samples_per_rev=72)
        out = rotary_wrap.apply_wrap_strategy(cmds, "A", "rezero")

        last_a = None
        for c in out:
            a = c.Parameters.get("A")
            if a is None:
                continue
            if c.Annotations.get("rotary_rezero"):
                # Frame just rebased; the controller's modal A is 0
                # before this move, so compare against 0.
                self.assertGreaterEqual(a + 1e-6, 0.0)
                last_a = a
                continue
            if last_a is not None:
                self.assertGreaterEqual(
                    a + 1e-6,
                    last_a,
                    f"backward jump in rezeroed output: {last_a} -> {a}",
                )
            last_a = a

    def test08_non_rotary_letter_passthrough(self):
        """A 'C' axis path is unchanged when caller asks for 'A' wrap."""
        cmds = _unwound_path(axis="C", n_revs=3)
        out = rotary_wrap.apply_wrap_strategy(cmds, "A", "modulo")
        # The function only rewrites the requested letter; C values stay raw.
        self.assertEqual(len(out), len(cmds))
        for src, dst in zip(cmds, out):
            self.assertAlmostEqual(src.Parameters["C"], dst.Parameters["C"])

    def test09_unknown_strategy_raises(self):
        cmds = _unwound_path(n_revs=1)
        with self.assertRaises(ValueError):
            rotary_wrap.apply_wrap_strategy(cmds, "A", "bogus")
