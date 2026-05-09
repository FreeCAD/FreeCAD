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

import math

import Path
import Path.Base.Generator.rotary_parallel as rotary_parallel
from CAMTests.PathTestUtils import PathTestBase

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


def _const_radii(n_x, n_t, value):
    """Build a (n_x, n_t) list-of-lists radii grid with a constant value."""
    return [[float(value) for _ in range(n_t)] for _ in range(n_x)]


def _build_grid(x_min, x_max, n_x, n_t, radius):
    xs = [x_min + (x_max - x_min) * i / (n_x - 1) for i in range(n_x)]
    thetas = [2.0 * math.pi * j / (n_t - 1) for j in range(n_t)]
    radii = _const_radii(n_x, n_t, radius)
    return xs, thetas, radii


class TestPathRotaryParallel(PathTestBase):
    """Generator-level unit tests for the Parallel rotary cut pattern."""

    def _generate(
        self,
        x_min=0.0,
        x_max=20.0,
        theta_start=0.0,
        theta_end=2.0 * math.pi,
        axial_stepover=2.0,
        radius=10.0,
        cut_mode="Climb",
        rotary_letter="A",
        rotary_axis="X",
        max_feed=5000.0,
    ):
        xs, thetas, radii = _build_grid(x_min, x_max, n_x=8, n_t=12, radius=radius)
        return rotary_parallel.generate(
            radii=radii,
            xs=xs,
            thetas=thetas,
            rotary_letter=rotary_letter,
            rotary_axis=rotary_axis,
            x_min=x_min,
            x_max=x_max,
            theta_start=theta_start,
            theta_end=theta_end,
            axial_stepover=axial_stepover,
            angular_resolution=math.radians(5.0),
            radial_stock_to_leave=0.0,
            cut_mode=cut_mode,
            safe_height=25.0,
            clearance_height=20.0,
            horiz_feed=500.0,
            vert_feed=200.0,
            horiz_rapid=5000.0,
            vert_rapid=5000.0,
            max_feed=max_feed,
            cutter_z_floor=None,
        )

    # ------------------------------------------------------------------

    def test00_emits_motion_commands(self):
        """Generator produces a non-trivial number of motion commands.

        For a 20mm axial range, 2mm axial_stepover, radius 10:
            angular_step = 2 / 10 = 0.2 rad
            n_passes ~ ceil(2*pi / 0.2) + 1 ~ 33
            samples/pass = ceil(20 / (2*0.25)) + 1 = 41
        So there should be ~33 * 41 cutting G1s plus approach/retract.
        """
        commands = self._generate()
        self.assertGreater(
            len(commands),
            100,
            "expected many commands for a parallel rotary path, got {}".format(len(commands)),
        )

    def test01_first_cutting_g1_has_xyza(self):
        """The first cutting G1 must qualify X, Y, Z, and the rotary letter."""
        commands = self._generate()
        cut_g1 = [c for c in commands if c.Name == "G1" and "A" in c.Parameters]
        self.assertTrue(len(cut_g1) > 0, "expected at least one cutting G1 with A")
        first = cut_g1[0]
        for key in ("X", "Y", "Z", "A"):
            self.assertIn(key, first.Parameters, "first cutting G1 missing {}".format(key))

    def test02_a_traverses_requested_range(self):
        """A values span [theta_start, theta_end] within stepover tolerance."""
        theta_start = 0.0
        theta_end = 2.0 * math.pi
        commands = self._generate(theta_start=theta_start, theta_end=theta_end)
        a_values = [c.Parameters["A"] for c in commands if c.Name == "G1" and "A" in c.Parameters]
        self.assertGreater(len(a_values), 0, "expected at least one cutting G1 with A")
        a_min = min(a_values)
        a_max = max(a_values)
        # Stepover at radius 10 with axial_stepover 2 -> 0.2 rad ~= 11.46 deg.
        # A values should reach within one stepover of both extents.
        self.assertLessEqual(a_min, math.degrees(theta_start) + 12.0)
        self.assertGreaterEqual(a_max, math.degrees(theta_end) - 12.0)

    def test03_pass_count_matches_angular_stepover(self):
        """Distinct A values in cutting G1s match the expected pass count."""
        x_min, x_max = 0.0, 20.0
        radius = 10.0
        axial_stepover = 2.0
        theta_start = 0.0
        theta_end = 2.0 * math.pi
        commands = self._generate(
            x_min=x_min,
            x_max=x_max,
            theta_start=theta_start,
            theta_end=theta_end,
            axial_stepover=axial_stepover,
            radius=radius,
        )
        # Expected: angular_step = axial_stepover / radius
        expected_angular_step = axial_stepover / radius
        expected_passes = int(math.ceil(abs(theta_end - theta_start) / expected_angular_step)) + 1

        a_values = [c.Parameters["A"] for c in commands if c.Name == "G1" and "A" in c.Parameters]
        # Distinct A values within float tolerance.
        distinct = []
        for v in a_values:
            if not any(abs(v - u) < 1e-6 for u in distinct):
                distinct.append(v)
        self.assertEqual(
            len(distinct),
            expected_passes,
            "expected {} passes (distinct A values), got {}".format(expected_passes, len(distinct)),
        )

    def test04_zigzag_x_direction_alternates(self):
        """Consecutive passes traverse X in opposite directions."""
        commands = self._generate()
        cut_g1 = [c for c in commands if c.Name == "G1" and "A" in c.Parameters]
        # Group cutting G1s by A value (each pass is one A).
        passes = []
        current_a = None
        current = []
        for c in cut_g1:
            a = c.Parameters["A"]
            if current_a is None or abs(a - current_a) > 1e-6:
                if current:
                    passes.append(current)
                current = [c]
                current_a = a
            else:
                current.append(c)
        if current:
            passes.append(current)

        # Need at least 3 passes to confirm sign flip across two boundaries.
        self.assertGreaterEqual(len(passes), 3)

        # For each pass, the dominant deltaX sign is the pass direction.
        signs = []
        for grp in passes:
            xs = [c.Parameters["X"] for c in grp]
            if len(xs) < 2:
                signs.append(0)
                continue
            dx = xs[-1] - xs[0]
            signs.append(1 if dx > 0 else -1 if dx < 0 else 0)

        # Adjacent passes must have opposite sign (zig-zag).
        for i in range(len(signs) - 1):
            self.assertNotEqual(
                signs[i],
                signs[i + 1],
                "expected zig-zag: pass {} sign {} vs pass {} sign {}".format(
                    i, signs[i], i + 1, signs[i + 1]
                ),
            )

    def test05_y_axis_emits_rotary_letter(self):
        """Y rotary axis: rotary letter is honored as the parameter key."""
        commands = self._generate(rotary_axis="Y", rotary_letter="B")
        b_g1 = [c for c in commands if c.Name == "G1" and "B" in c.Parameters]
        self.assertTrue(
            len(b_g1) > 5,
            "expected B-axis cutting commands for Y rotary, got {}".format(len(b_g1)),
        )
        # And conversely, no A.
        a_g1 = [c for c in commands if c.Name == "G1" and "A" in c.Parameters]
        self.assertEqual(len(a_g1), 0)

    def test06_conventional_inverts_theta_direction(self):
        """Conventional cut walks A in the opposite direction from Climb."""
        climb = self._generate(cut_mode="Climb")
        conv = self._generate(cut_mode="Conventional")
        a_climb = [c.Parameters["A"] for c in climb if c.Name == "G1" and "A" in c.Parameters]
        a_conv = [c.Parameters["A"] for c in conv if c.Name == "G1" and "A" in c.Parameters]
        self.assertGreater(max(a_climb), min(a_climb))
        self.assertGreater(max(a_conv), min(a_conv))
        # Climb increases (positive direction); Conventional decreases.
        self.assertGreater(a_climb[-1], a_climb[0])
        self.assertLess(a_conv[-1], a_conv[0])
