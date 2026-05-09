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

"""
Parallel rotary-surface toolpath generator.

Pure-function generator for the Parallel cut pattern of the Rotary
Surface operation. The cutter walks the rotary stock along the axial
direction (the "fast" axis) at a fixed angular position, then steps
the rotary by an angular increment and walks back the other way --
classic axial zig-zag with the rotary axis providing the stepover.

Each axial pass alternates direction so the cutter never lifts; the
operation downstream (rotary_wrap) handles 0/360 wrapping policy.
The radii grid is consumed exactly as `rotary_spiral.generate` does:
the caller (typically the Rotary Surface op) pre-samples (xs, thetas)
via `rotary_dropcutter.sample` and passes the 2-D grid in.

Output A is *unwound*: the angular value increases monotonically as
passes accumulate so the rotary axis never reverses across a 0/360
boundary in a single emitted path.
"""

import math

import Path

from Path.Base.Generator.rotary_spiral import (
    _bilinear,
    _radii_get,
    _world_xyz,
    _FeedClamp,
)

__title__ = "Rotary Parallel Toolpath Generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Generate a parallel (axial zig-zag) rotary-surface toolpath."

__all__ = ["generate"]


def generate(
    radii,
    xs,
    thetas,
    rotary_letter,
    rotary_axis,
    x_min,
    x_max,
    theta_start,
    theta_end,
    axial_stepover,
    angular_resolution,
    radial_stock_to_leave,
    cut_mode,
    safe_height,
    clearance_height,
    horiz_feed,
    vert_feed,
    horiz_rapid,
    vert_rapid,
    max_feed=None,
    cutter_z_floor=None,
    feed_mode="AxialOnly",
):
    """Build a Parallel (axial zig-zag) rotary-surface toolpath.

    The rotary indexes by an angular stepover between passes. Each pass
    is a continuous axial sweep from x_min to x_max (or x_max to x_min
    on alternate passes for true bidirectional zig-zag) at constant A.
    Direction across the angular range is set by `cut_mode`: Climb
    advances theta with positive sign, Conventional reverses it.

    Angular stepover derivation
    ---------------------------
    Unlike the Spiral pattern, "axial stepover" is not the natural unit
    here -- there *is* no spiral pitch. Instead, `axial_stepover` is
    interpreted as the desired arc-length stepover at the largest stock
    radius, and converted to an angular step:

        angular_step = axial_stepover / max_r        [radians]

    where max_r is the maximum non-NaN radius observed on the grid.
    This keeps the surface stepover roughly comparable to the Spiral
    pattern at the same `axial_stepover` setting, without requiring a
    new user-facing property. (If/when `obj.AngularStepover` is added
    later, the op can pass that in via this same parameter slot.)

    Axial sample density
    --------------------
    Each pass samples X at `axial_stepover/4` spacing (matching the
    Spiral pattern's grid x-step), giving ~4 samples per nominal
    stepover for smooth surface following.

    Parameters
    ----------
    radii : 2-D array-like, shape (len(xs), len(thetas))
        Pre-sampled cutter-location radii from `rotary_dropcutter.sample`.
    xs, thetas : 1-D sequences
        The grid coordinates that `radii` was sampled on. `thetas` are in
        radians.
    rotary_letter : str
        Axis letter to emit in G-code: 'A', 'B', or 'C'.
    rotary_axis : str
        Either 'X' or 'Y' -- which world axis the rotary lies along.
    x_min, x_max : float
        Axial extents of the path.
    theta_start, theta_end : float
        Angular extents in radians, *unwound*. The pattern walks from
        theta_start toward theta_end inclusive.
    axial_stepover : float
        Repurposed here as the target arc-length stepover at max radius.
        Converted to radians as `axial_stepover / max_r`.
    angular_resolution : float
        Unused for this pattern (axial sample density is set from
        axial_stepover). Kept in the signature for parity with the
        other rotary generators.
    radial_stock_to_leave : float
        Finish allowance added to every output radius.
    cut_mode : str
        'Climb' (theta advances positive) or 'Conventional' (negative).
    safe_height, clearance_height : float
        Z-values for retracts. Both are absolute.
    horiz_feed, vert_feed : float
        Cutting feed rates (mm/min).
    horiz_rapid, vert_rapid : float
        Rapid rates (mm/min) for non-cutting moves.
    max_feed : float or None
        Effective-feed clamp. None disables clamping.
    cutter_z_floor : float or None
        Per-layer radial-depth target. The emitted cutter Z is clamped to
        `max(surface_r + radial_stock_to_leave, cutter_z_floor)`.

    Returns
    -------
    list of Path.Command
    """
    if rotary_axis not in ("X", "Y"):
        raise ValueError("rotary_axis must be 'X' or 'Y'; got %r" % rotary_axis)
    if axial_stepover <= 0.0:
        raise ValueError("axial_stepover must be positive")
    if x_max <= x_min:
        raise ValueError("x_max must exceed x_min")

    n_x = len(xs)
    n_t = len(thetas)

    # Normalize radii grid to a list-of-lists view.
    grid = [[_radii_get(radii, i, j) for j in range(n_t)] for i in range(n_x)]

    valid = [v for row in grid for v in row if not math.isnan(v)]
    if not valid:
        raise ValueError("radii grid is entirely NaN; cannot build path")
    max_r = max(valid)
    for i in range(n_x):
        for j in range(n_t):
            if math.isnan(grid[i][j]):
                grid[i][j] = max_r

    # Direction inverts for Conventional cut.
    direction = 1.0 if cut_mode == "Climb" else -1.0

    # Angular stepover: arc-length stepover at the largest radius.
    # max_r is guaranteed > 0 by the valid-grid check above (radii are
    # cutter-location distances from the spin axis; zero would only
    # happen if the part is a perfect line on the axis, which the
    # sampling step would reject upstream).
    max_r_safe = max(max_r, 1e-6)
    angular_step = axial_stepover / max_r_safe

    # Axial sample density: same convention as the Spiral pattern's
    # grid x-step. Approximately 4 samples per axial_stepover.
    x_sample_step = max(axial_stepover * 0.25, 1e-3)
    n_x_samples = max(2, int(math.ceil((x_max - x_min) / x_sample_step)) + 1)

    # Angular pass count. Always include the start; the end is included
    # if it falls (within tolerance) on a stepover boundary.
    total_theta_span = direction * abs(theta_end - theta_start)
    n_passes = max(1, int(math.ceil(abs(total_theta_span) / angular_step)) + 1)

    commands = []
    feed_clamp = _FeedClamp()

    # Track current machine position so every emitted move is fully
    # qualified with X, Y, Z, and the rotary letter.
    cur_x = float(x_min)
    cur_y = 0.0
    cur_z = float(clearance_height)
    cur_a = math.degrees(theta_start)

    def _emit(name, *, x=None, y=None, z=None, a=None, feed):
        nonlocal cur_x, cur_y, cur_z, cur_a
        if x is not None:
            cur_x = float(x)
        if y is not None:
            cur_y = float(y)
        if z is not None:
            cur_z = float(z)
        if a is not None:
            cur_a = float(a)
        params = {
            "X": cur_x,
            "Y": cur_y,
            "Z": cur_z,
            rotary_letter: cur_a,
            "F": float(feed),
        }
        commands.append(Path.Command(name, params))

    def _radius_at(x, theta_unwound):
        """Bilinear-interpolated radius at (x, theta), with stock-to-leave."""
        twopi = 2.0 * math.pi
        theta_mod = theta_unwound % twopi
        if theta_mod < 0:
            theta_mod += twopi
        if theta_mod < thetas[0]:
            theta_mod = thetas[0]
        if theta_mod > thetas[-1]:
            theta_mod = thetas[-1]
        r = _bilinear(grid, list(xs), list(thetas), x, theta_mod)
        if math.isnan(r):
            r = max_r
        r += radial_stock_to_leave
        if cutter_z_floor is not None and r < cutter_z_floor:
            r = cutter_z_floor
        return r

    # Approach: rapid to clearance.
    _emit("G0", z=clearance_height, feed=vert_rapid)

    last_r = max_r + radial_stock_to_leave

    for p in range(n_passes):
        # Unwound angular position for this pass.
        theta_p = theta_start + direction * p * angular_step
        # Clamp the final pass to the far edge of the requested span,
        # signed to match `direction` so Conventional walks A in the
        # opposite direction from Climb without a discontinuity.
        if p == n_passes - 1:
            theta_p = theta_start + direction * abs(theta_end - theta_start)
        a_deg = math.degrees(theta_p)

        # Zig-zag: even passes go x_min -> x_max, odd passes reverse.
        if p % 2 == 0:
            xs_pass = [x_min + (x_max - x_min) * k / (n_x_samples - 1) for k in range(n_x_samples)]
        else:
            xs_pass = [x_max - (x_max - x_min) * k / (n_x_samples - 1) for k in range(n_x_samples)]

        # Approach the start of this pass: rapid X/Y, set rotary, then
        # plunge. The first pass plunges from clearance; subsequent
        # passes lift to clearance, reposition, and plunge again so
        # the cutter never drags between passes.
        x_start = xs_pass[0]
        wx, wy, _ = _world_xyz(rotary_axis, x_start, last_r + 5.0)
        # Lift off the previous pass's last radius before the rapid.
        if p > 0:
            _emit("G1", z=last_r + 5.0, feed=vert_feed)
            _emit("G0", z=clearance_height, feed=vert_rapid)
        _emit("G0", x=wx, y=wy, feed=horiz_rapid)
        _emit("G0", a=a_deg, feed=horiz_rapid)

        # Plunge to first-sample radius on this pass.
        r0 = _radius_at(x_start, theta_p)
        _emit("G1", z=r0, feed=vert_feed)
        last_r = r0

        # Cut along the axial direction at constant theta_p.
        for x in xs_pass[1:]:
            r = _radius_at(x, theta_p)
            wx, wy, _ = _world_xyz(rotary_axis, x, r)
            _emit(
                "G1",
                x=wx,
                y=wy,
                z=r,
                a=a_deg,
                feed=feed_clamp.feed_for(horiz_feed, r, max_feed, feed_mode),
            )
            last_r = r

    # Final retract.
    _emit("G1", z=last_r + 5.0, feed=vert_feed)
    _emit("G0", z=safe_height, feed=vert_rapid)

    if feed_clamp.events:
        Path.Log.warning(
            "Rotary Surface (Parallel): centerline feed clamped {n} time(s); "
            "min radius {r:.3f}, peak rotary feed {peak:.1f} (capped at {cap:.1f}).".format(
                n=feed_clamp.events,
                r=feed_clamp.min_r if feed_clamp.min_r is not None else 0.0,
                peak=feed_clamp.max_effective,
                cap=float(max_feed) if max_feed else 0.0,
            )
        )

    return commands
