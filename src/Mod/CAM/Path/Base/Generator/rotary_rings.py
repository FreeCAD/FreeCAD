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

# ***************************************************************************
"""
Rings rotary-surface toolpath generator.

Pure-function generator for the Rings cut pattern of the Rotary
Surface operation. Emits a stack of full-revolution rings at fixed
axial positions, advancing axially by `axial_stepover` between
rings. Within each ring the angular sweep covers
``theta_end - theta_start`` (typically 2*pi) at density
`angular_resolution`.

Inter-ring travel: this generator does NOT lift between rings. After
the final point of one ring is cut, the next ring begins at the same
radius and steps axially in a single cutting move. This keeps the
g-code compact and avoids unnecessary retract/plunge cycles for
surface-following rotary cuts. If a future caller needs a lift, that
behavior can be added under a new flag rather than changed here.

Output A is *unwound* (monotonic across rings): a full revolution per
ring means ring 0 sweeps theta_start..theta_start+2*pi, ring 1 sweeps
theta_start+2*pi..theta_start+4*pi, etc. The wrap strategy step
(``rotary_wrap.apply_wrap_strategy``) is applied by the caller.

This module has no FreeCAD-document dependency beyond `Path.Command`.
"""

import math

import Path

from Path.Base.Generator.rotary_spiral import (
    _bilinear,
    _radii_get,
    _world_xyz,
    _FeedClamp,
)

__title__ = "Rotary Rings Toolpath Generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Generate a rings rotary-surface toolpath."

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
    """Build a Rings rotary-surface toolpath.

    Parameters
    ----------
    radii : 2-D array-like, shape (len(xs), len(thetas))
        Pre-sampled cutter-location radii from `rotary_dropcutter.sample`.
        NaN cells are replaced with the maximum valid radius (a
        conservative retract).
    xs, thetas : 1-D sequences
        Grid coordinates that `radii` was sampled on. `thetas` are in
        radians, monotonically increasing in [0, 2*pi].
    rotary_letter : str
        Axis letter to emit ('A', 'B', or 'C').
    rotary_axis : str
        World axis the rotary lies along ('X' or 'Y').
    x_min, x_max : float
        Axial extents of the rings stack.
    theta_start, theta_end : float
        Per-ring angular extents in radians, *unwound*. Typically a
        full revolution: ``theta_end - theta_start == 2*pi``.
    axial_stepover : float
        Axial distance between consecutive rings.
    angular_resolution : float
        Angular spacing in radians between successive points within a
        ring. Smaller = smoother & more G-code lines.
    radial_stock_to_leave : float
        Finish allowance added to every output radius.
    cut_mode : str
        'Climb' or 'Conventional'. Inverts the angular direction of each
        ring's sweep.
    safe_height, clearance_height : float
        Z-values for retracts. Both are absolute.
    horiz_feed, vert_feed : float
        Cutting feed rates (mm/min).
    horiz_rapid, vert_rapid : float
        Rapid rates (mm/min).
    max_feed : float or None
        Effective-feed clamp. None disables clamping.
    cutter_z_floor : float or None
        Per-layer radial-depth target. The emitted cutter Z is clamped
        to ``max(surface_r + radial_stock_to_leave, cutter_z_floor)``.
        None disables the clamp (single-pass / surface-follow).
    feed_mode : str, default 'AxialOnly'
        Controls how cutting feed is computed. 'AxialOnly' uses only the
        axial (X) component for feed calculation; 'Combined' includes the
        angular/rotary component so effective feed accounts for both axial
        and circumferential motion. Affects how horiz_feed/vert_feed and
        max_feed are applied.

    Returns
    -------
    list of Path.Command
        Flat list of commands. A is unwound (monotonic across rings),
        so post-processing wrap strategy can be applied by the caller.

    Notes
    -----
    Ring count formula::

        n_rings = int(math.ceil((x_max - x_min) / axial_stepover)) + 1

    The +1 ensures both endpoints (x_min and x_max) get a ring; rings
    are evenly distributed between the two via interpolation by `t` in
    [0, 1].
    """
    if rotary_axis not in ("X", "Y"):
        raise ValueError("rotary_axis must be 'X' or 'Y'; got %r" % rotary_axis)
    if axial_stepover <= 0.0:
        raise ValueError("axial_stepover must be positive")
    if angular_resolution <= 0.0:
        raise ValueError("angular_resolution must be positive")
    if x_max <= x_min:
        raise ValueError("x_max must exceed x_min")

    n_x = len(xs)
    n_t = len(thetas)

    # Normalize radii to a list-of-lists view.
    grid = [[_radii_get(radii, i, j) for j in range(n_t)] for i in range(n_x)]

    # Replace NaN cells with the max valid radius (conservative).
    valid = [v for row in grid for v in row if not math.isnan(v)]
    if not valid:
        raise ValueError("radii grid is entirely NaN; cannot build path")
    max_r = max(valid)
    for i in range(n_x):
        for j in range(n_t):
            if math.isnan(grid[i][j]):
                grid[i][j] = max_r

    # Ring count: include both endpoints.
    n_rings = int(math.ceil((x_max - x_min) / axial_stepover)) + 1
    if n_rings < 2:
        n_rings = 2

    # Per-ring angular-step direction inverts for Conventional cut.
    direction = 1.0 if cut_mode == "Climb" else -1.0
    theta_span = theta_end - theta_start  # typically 2*pi
    # Number of angular sub-steps within a ring.
    n_steps_per_ring = max(1, int(math.ceil(abs(theta_span) / angular_resolution)))

    commands = []
    feed_clamp = _FeedClamp()

    # Track current machine position so every emitted move can be fully
    # qualified with X, Y, Z, and the rotary-axis word.
    x0_world, y0_world, _ = _world_xyz(rotary_axis, x_min, max_r + radial_stock_to_leave + 5.0)
    a_start_deg = math.degrees(theta_start)
    cur_x, cur_y, cur_z, cur_a = (
        float(x0_world),
        float(y0_world),
        float(clearance_height),
        float(a_start_deg),
    )

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

    # Approach: rapid to clearance, position, rotate to start, plunge.
    _emit("G0", z=clearance_height, feed=vert_rapid)
    _emit("G0", x=x0_world, y=y0_world, feed=horiz_rapid)
    _emit("G0", a=a_start_deg, feed=horiz_rapid)

    # Drop to the radius at the very first sample (ring 0, theta_start).
    twopi = 2.0 * math.pi
    theta_mod0 = theta_start % twopi
    if theta_mod0 < 0:
        theta_mod0 += twopi
    if theta_mod0 < thetas[0]:
        theta_mod0 = thetas[0]
    if theta_mod0 > thetas[-1]:
        theta_mod0 = thetas[-1]

    r0 = _bilinear(grid, list(xs), list(thetas), x_min, theta_mod0)
    if math.isnan(r0):
        r0 = max_r
    r0 += radial_stock_to_leave
    if cutter_z_floor is not None and r0 < cutter_z_floor:
        r0 = cutter_z_floor

    # Move to the start X position (in world coords) and plunge.
    wx0, wy0, _ = _world_xyz(rotary_axis, x_min, r0)
    _emit("G1", x=wx0, y=wy0, z=r0, a=a_start_deg, feed=vert_feed)

    last_r = r0

    # Outer loop: rings (axial position).
    for ring_idx in range(n_rings):
        if n_rings == 1:
            t_axial = 0.0
        else:
            t_axial = ring_idx / (n_rings - 1)
        x_axial = x_min + (x_max - x_min) * t_axial

        # Inner loop: angular sweep within this ring. Each ring spans
        # exactly ``theta_span`` of unwound A. For ring k starting at
        # ``theta_start + k * direction * theta_span`` we step in
        # `n_steps_per_ring` sub-intervals; both endpoints are emitted.
        ring_a_start = theta_start + ring_idx * direction * theta_span
        # Inner step inverts in the same direction as `direction`.
        for k in range(n_steps_per_ring + 1):
            # Skip the very first inner sample of subsequent rings:
            # ring_idx>=1 starts at ring_a_start which equals the
            # previous ring's last A — we already emitted that point at
            # the previous ring's final iteration. Re-emitting would add
            # a zero-length move at a different X (axial slide handled
            # below via the inner k=0 emission).
            if ring_idx > 0 and k == 0:
                # Slide axially to this ring's start while holding A
                # constant. This is the inter-ring travel: a single
                # cutting move along the rotary-axis direction.
                # We still need a fresh radius lookup at the new X.
                theta_unwound = ring_a_start
                theta_mod = theta_unwound % twopi
                if theta_mod < 0:
                    theta_mod += twopi
                if theta_mod < thetas[0]:
                    theta_mod = thetas[0]
                if theta_mod > thetas[-1]:
                    theta_mod = thetas[-1]
                r = _bilinear(grid, list(xs), list(thetas), x_axial, theta_mod)
                if math.isnan(r):
                    r = max_r
                r += radial_stock_to_leave
                if cutter_z_floor is not None and r < cutter_z_floor:
                    r = cutter_z_floor
                wx, wy, _ = _world_xyz(rotary_axis, x_axial, r)
                _emit(
                    "G1",
                    x=wx,
                    y=wy,
                    z=r,
                    a=math.degrees(theta_unwound),
                    feed=feed_clamp.feed_for(horiz_feed, r, max_feed, feed_mode),
                )
                last_r = r
                continue

            t_ring = k / n_steps_per_ring
            theta_unwound = ring_a_start + direction * theta_span * t_ring

            # Reduce into grid range for interpolation.
            theta_mod = theta_unwound % twopi
            if theta_mod < 0:
                theta_mod += twopi
            if theta_mod < thetas[0]:
                theta_mod = thetas[0]
            if theta_mod > thetas[-1]:
                theta_mod = thetas[-1]

            r = _bilinear(grid, list(xs), list(thetas), x_axial, theta_mod)
            if math.isnan(r):
                r = max_r
            r += radial_stock_to_leave
            if cutter_z_floor is not None and r < cutter_z_floor:
                r = cutter_z_floor

            wx, wy, _ = _world_xyz(rotary_axis, x_axial, r)
            _emit(
                "G1",
                x=wx,
                y=wy,
                z=r,
                a=math.degrees(theta_unwound),
                feed=feed_clamp.feed_for(horiz_feed, r, max_feed, feed_mode),
            )
            last_r = r

    # Retract.
    _emit("G1", z=last_r + 5.0, feed=vert_feed)
    _emit("G0", z=safe_height, feed=vert_rapid)

    if feed_clamp.events:
        Path.Log.warning(
            "Rotary Surface: centerline feed clamped {n} time(s); "
            "min radius {r:.3f}, peak rotary feed {peak:.1f} (capped at {cap:.1f}).".format(
                n=feed_clamp.events,
                r=feed_clamp.min_r if feed_clamp.min_r is not None else 0.0,
                peak=feed_clamp.max_effective,
                cap=float(max_feed) if max_feed else 0.0,
            )
        )

    return commands
