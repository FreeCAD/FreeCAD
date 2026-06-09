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

"""Generator-level unit tests for the Rings rotary-surface pattern.

These tests exercise ``rotary_rings.generate`` directly with a
hand-built radii grid; no FreeCAD document, OCL sampling, or fixture
file is required.
"""

import math

import Path
import Path.Base.Generator.rotary_rings as rotary_rings
from CAMTests.PathTestUtils import PathTestBase

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


def _build_constant_grid(n_x, n_t, radius=10.0):
    """Hand-build a flat radii grid with constant value `radius`."""
    return [[float(radius) for _ in range(n_t)] for _ in range(n_x)]


def _default_args(radii, xs, thetas, **overrides):
    """Sensible defaults shared across the tests; overridable per case."""
    args = dict(
        radii=radii,
        xs=xs,
        thetas=thetas,
        rotary_letter="A",
        rotary_axis="X",
        x_min=xs[0],
        x_max=xs[-1],
        theta_start=0.0,
        theta_end=2.0 * math.pi,
        axial_stepover=2.0,
        angular_resolution=math.radians(15.0),
        radial_stock_to_leave=0.0,
        cut_mode="Climb",
        safe_height=20.0,
        clearance_height=15.0,
        horiz_feed=500.0,
        vert_feed=200.0,
        horiz_rapid=5000.0,
        vert_rapid=5000.0,
        max_feed=10000.0,
        cutter_z_floor=None,
    )
    args.update(overrides)
    return args


class TestPathRotaryRings(PathTestBase):
    """Unit tests for the Rings rotary-surface generator."""

    def test00_emits_xyza_commands(self):
        """Generator returns enough commands and the first cut has XYZA."""
        n_x, n_t = 5, 13
        xs = [i * 2.0 for i in range(n_x)]  # x_min=0, x_max=8
        thetas = [j * 2.0 * math.pi / (n_t - 1) for j in range(n_t)]
        grid = _build_constant_grid(n_x, n_t, radius=10.0)

        cmds = rotary_rings.generate(**_default_args(grid, xs, thetas))

        self.assertGreater(len(cmds), 10, "expected many commands, got {}".format(len(cmds)))

        cut_g1 = [c for c in cmds if c.Name == "G1" and "A" in c.Parameters]
        self.assertGreater(len(cut_g1), 5)
        first = cut_g1[0]
        for k in ("X", "Y", "Z", "A"):
            self.assertIn(k, first.Parameters, "missing {} on first cutting G1".format(k))

    def test01_ring_count_matches_formula(self):
        """Number of rings in output equals ceil(span/stepover)+1."""
        n_x, n_t = 5, 13
        xs = [i * 2.0 for i in range(n_x)]
        thetas = [j * 2.0 * math.pi / (n_t - 1) for j in range(n_t)]
        grid = _build_constant_grid(n_x, n_t, radius=10.0)

        x_min, x_max = 0.0, 8.0
        axial_stepover = 2.0
        expected_rings = int(math.ceil((x_max - x_min) / axial_stepover)) + 1

        cmds = rotary_rings.generate(
            **_default_args(
                grid,
                xs,
                thetas,
                x_min=x_min,
                x_max=x_max,
                axial_stepover=axial_stepover,
            )
        )

        # Count distinct X-positions among cutting moves; rotary X axis
        # so axial == X. Tolerance: round to 6 decimals.
        cut_g1 = [c for c in cmds if c.Name == "G1" and "A" in c.Parameters]
        x_set = sorted({round(c.Parameters["X"], 6) for c in cut_g1})
        self.assertEqual(
            len(x_set),
            expected_rings,
            "expected {} rings, got distinct X count {}".format(expected_rings, len(x_set)),
        )

    def test02_each_ring_sweeps_full_revolution(self):
        """Within each ring A spans ~theta_end - theta_start (== 2*pi)."""
        n_x, n_t = 5, 13
        xs = [i * 2.0 for i in range(n_x)]
        thetas = [j * 2.0 * math.pi / (n_t - 1) for j in range(n_t)]
        grid = _build_constant_grid(n_x, n_t, radius=10.0)

        cmds = rotary_rings.generate(**_default_args(grid, xs, thetas))
        cut_g1 = [c for c in cmds if c.Name == "G1" and "A" in c.Parameters]

        # Group cut moves by their X (which identifies the ring).
        rings = {}
        for c in cut_g1:
            xkey = round(c.Parameters["X"], 6)
            rings.setdefault(xkey, []).append(c.Parameters["A"])

        self.assertGreaterEqual(len(rings), 2)
        expected_span_deg = math.degrees(2.0 * math.pi)
        for xkey, a_vals in rings.items():
            span = max(a_vals) - min(a_vals)
            # Allow a small tolerance — last sub-step sits exactly at
            # the ring endpoint, so the sweep should be effectively full.
            self.assertAlmostEqual(
                span,
                expected_span_deg,
                delta=1e-3,
                msg="ring at X={} swept {:.3f} deg, expected {:.3f}".format(
                    xkey, span, expected_span_deg
                ),
            )

    def test03_x_monotonic_across_rings(self):
        """Each new ring's X is >= the previous ring's X (axial advance)."""
        n_x, n_t = 5, 13
        xs = [i * 2.0 for i in range(n_x)]
        thetas = [j * 2.0 * math.pi / (n_t - 1) for j in range(n_t)]
        grid = _build_constant_grid(n_x, n_t, radius=10.0)

        cmds = rotary_rings.generate(**_default_args(grid, xs, thetas))
        cut_g1 = [c for c in cmds if c.Name == "G1" and "A" in c.Parameters]

        # The X of each cutting move should be non-decreasing across the
        # entire path since rings are emitted in axial order.
        x_seq = [round(c.Parameters["X"], 6) for c in cut_g1]
        for i in range(1, len(x_seq)):
            self.assertGreaterEqual(
                x_seq[i],
                x_seq[i - 1],
                "X regressed from {} to {} at step {}".format(x_seq[i - 1], x_seq[i], i),
            )

    def test04_unwound_a_monotonic_across_rings(self):
        """A grows monotonically across rings (no 0/360 wrap)."""
        n_x, n_t = 5, 13
        xs = [i * 2.0 for i in range(n_x)]
        thetas = [j * 2.0 * math.pi / (n_t - 1) for j in range(n_t)]
        grid = _build_constant_grid(n_x, n_t, radius=10.0)

        cmds = rotary_rings.generate(**_default_args(grid, xs, thetas))
        cut_g1 = [c for c in cmds if c.Name == "G1" and "A" in c.Parameters]
        a_vals = [c.Parameters["A"] for c in cut_g1]

        # No single delta should exceed +/- 360 (would indicate a wrap).
        for i in range(1, len(a_vals)):
            d = a_vals[i] - a_vals[i - 1]
            self.assertLess(
                abs(d),
                360.0,
                "A delta {} indicates a 0/360 wrap rather than unwound output".format(d),
            )

        # A must be non-decreasing (monotonic) for Climb mode.
        for i in range(1, len(a_vals)):
            self.assertGreaterEqual(
                a_vals[i],
                a_vals[i - 1],
                "A regressed from {} to {} at step {}".format(a_vals[i - 1], a_vals[i], i),
            )

        # With multiple rings the total unwound sweep should exceed one
        # revolution.
        self.assertGreater(max(a_vals) - min(a_vals), 360.0)

    def test05_rejects_invalid_inputs(self):
        """Bad arguments raise ValueError, not silent garbage."""
        n_x, n_t = 4, 9
        xs = [i * 2.0 for i in range(n_x)]
        thetas = [j * 2.0 * math.pi / (n_t - 1) for j in range(n_t)]
        grid = _build_constant_grid(n_x, n_t, radius=10.0)

        with self.assertRaises(ValueError):
            rotary_rings.generate(**_default_args(grid, xs, thetas, rotary_axis="Z"))
        with self.assertRaises(ValueError):
            rotary_rings.generate(**_default_args(grid, xs, thetas, axial_stepover=0.0))
        with self.assertRaises(ValueError):
            rotary_rings.generate(**_default_args(grid, xs, thetas, angular_resolution=0.0))
        with self.assertRaises(ValueError):
            rotary_rings.generate(**_default_args(grid, xs, thetas, x_min=5.0, x_max=5.0))
