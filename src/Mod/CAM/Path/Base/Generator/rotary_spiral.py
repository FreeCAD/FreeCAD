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
Spiral rotary-surface toolpath generator.

Pure-function generator for the v1 Spiral cut pattern of the Rotary
Surface operation. Walks a continuous helix from x_min to x_max with
one revolution per AxialStepover, sampling the radius at each point
from a pre-computed (xs, thetas) grid. Emits XYZA (or XYZB) Path
commands with monotonic, *unwound* angular values so the rotary axis
never reverses across a 0/360 boundary.

This module has no FreeCAD-document dependency beyond `Path.Command`.
The OCL drop-cutter sampling is done by the caller (typically the
Rotary Surface operation), giving this generator a clean, testable
interface.
"""

import math

import Path

__title__ = "Rotary Spiral Toolpath Generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Generate a spiral rotary-surface toolpath."

__all__ = ["generate"]


def _bilinear(radii, xs, thetas, x, theta_mod):
    """Bilinear-interp the radius at (x, theta_mod) on the (xs, thetas) grid.

    `theta_mod` must already be reduced into the [thetas[0], thetas[-1]]
    range. Returns NaN if any of the four corner samples are NaN.
    """
    n_x = len(xs)
    n_t = len(thetas)

    # Locate x bracket
    if x <= xs[0]:
        i0 = 0
        i1 = min(1, n_x - 1)
        u = 0.0
    elif x >= xs[-1]:
        i0 = n_x - 2 if n_x >= 2 else 0
        i1 = n_x - 1
        u = 1.0
    else:
        i1 = 1
        while i1 < n_x - 1 and xs[i1] < x:
            i1 += 1
        i0 = i1 - 1
        span = xs[i1] - xs[i0]
        u = (x - xs[i0]) / span if span > 0 else 0.0

    # Locate theta bracket
    if theta_mod <= thetas[0]:
        j0 = 0
        j1 = min(1, n_t - 1)
        v = 0.0
    elif theta_mod >= thetas[-1]:
        j0 = n_t - 2 if n_t >= 2 else 0
        j1 = n_t - 1
        v = 1.0
    else:
        j1 = 1
        while j1 < n_t - 1 and thetas[j1] < theta_mod:
            j1 += 1
        j0 = j1 - 1
        span = thetas[j1] - thetas[j0]
        v = (theta_mod - thetas[j0]) / span if span > 0 else 0.0

    r00 = radii[i0][j0]
    r01 = radii[i0][j1]
    r10 = radii[i1][j0]
    r11 = radii[i1][j1]
    if any(math.isnan(r) for r in (r00, r01, r10, r11)):
        return float("nan")

    return (1 - u) * (1 - v) * r00 + (1 - u) * v * r01 + u * (1 - v) * r10 + u * v * r11


def _radii_get(radii, i, j):
    """Helper to read radii whether a list-of-lists or a numpy array."""
    if hasattr(radii, "shape"):
        return float(radii[i, j])
    return float(radii[i][j])


def _world_xyz(rotary_axis, axial, r):
    """Map (axial, r) onto a world-frame cutter position with Y or X = 0.

    For rotary_axis='X': world = (axial, 0, r).
    For rotary_axis='Y': world = (0, axial, r).
    """
    if rotary_axis == "X":
        return axial, 0.0, r
    if rotary_axis == "Y":
        return 0.0, axial, r
    raise ValueError("rotary_axis must be 'X' or 'Y'; got %r" % rotary_axis)


class _FeedClamp:
    """Track centerline-feed-clamp events while generating a path.

    Holds the running count of clamp events, the worst-case radius at
    which a clamp fired, and the worst-case effective rotary-feed rate
    that would have been emitted without clamping. The operation logs
    a single summary at end of generate() rather than per-step spam.
    """

    def __init__(self):
        self.events = 0
        self.min_r = None
        self.max_effective = 0.0

    def feed_for(self, horiz_feed, r, max_feed, feed_mode="AxialOnly"):
        """Return the F value to emit for a cut move at radius r.

        ``feed_mode == "AxialOnly"`` returns ``horiz_feed`` unchanged
        on every move (the controller's own feed math determines how
        the rotary keeps up). ``feed_mode == "SurfaceSpeed"`` scales F
        per move so the rotary holds a constant surface feed at the
        cutter contact: ``F = horiz_feed * 360 / (2π·r)``. As r→0 this
        diverges; when it exceeds ``max_feed`` the value is clamped at
        ``max_feed`` and the event is recorded for the end-of-generate
        summary.

        ``max_feed`` also caps ``AxialOnly``: if ``horiz_feed`` itself
        would imply an effective rotary rate greater than ``max_feed``
        (very small r), F is scaled down so the controller-side rotary
        rate stays at ``max_feed``.
        """
        if feed_mode == "SurfaceSpeed":
            r_safe = max(float(r), 1e-9)
            effective = horiz_feed * 360.0 / (2.0 * math.pi * r_safe)
            if max_feed and max_feed > 0.0 and effective > max_feed:
                self.events += 1
                if self.min_r is None or r_safe < self.min_r:
                    self.min_r = r_safe
                if effective > self.max_effective:
                    self.max_effective = effective
                return float(max_feed)
            return effective

        # AxialOnly: F=horiz_feed, but still clamp the controller-side
        # rotary rate so the rotary servo isn't asked to outrun max_feed.
        if max_feed is None or max_feed <= 0.0:
            return horiz_feed
        r_safe = max(float(r), 1e-9)
        effective = horiz_feed * 360.0 / (2.0 * math.pi * r_safe)
        if effective > max_feed:
            self.events += 1
            if self.min_r is None or r_safe < self.min_r:
                self.min_r = r_safe
            if effective > self.max_effective:
                self.max_effective = effective
            return horiz_feed * (max_feed / effective)
        return horiz_feed


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
    """Build a Spiral rotary-surface toolpath.

    Parameters
    ----------
    radii : 2-D array-like, shape (len(xs), len(thetas))
        Pre-sampled cutter-location radii from `rotary_dropcutter.sample`.
        Cells may be NaN; spiral points that interpolate to NaN are
        clamped to the maximum non-NaN radius (a conservative retract
        for that pass).
    xs, thetas : 1-D sequences
        The grid coordinates that `radii` was sampled on. `thetas` are in
        radians, monotonically increasing in [0, 2*pi].
    rotary_letter : str
        Axis letter to emit in G-code: 'A', 'B', or 'C'.
    rotary_axis : str
        Either 'X' or 'Y' — which world axis the rotary lies along. This
        determines the cutter's (X, Y) position: rotary X cuts at
        (axial, 0, r), rotary Y cuts at (0, axial, r).
    x_min, x_max : float
        Axial extents of the spiral.
    theta_start, theta_end : float
        Angular extents in radians, *unwound* (theta_end may be > 2*pi
        for multi-revolution spirals). theta_end is generally derived
        from x_min/x_max and axial_stepover; passing it explicitly keeps
        the API uniform with future patterns.
    axial_stepover : float
        Axial distance per full 2*pi revolution.
    angular_resolution : float
        Angular spacing in radians between successive points along the
        spiral. Smaller = smoother & more G-code lines.
    radial_stock_to_leave : float
        Finish allowance added to every output radius.
    cut_mode : str
        'Climb' or 'Conventional'. Inverts the spiral's angular direction.
    safe_height, clearance_height : float
        Z-values for retracts. Both are absolute.
    horiz_feed, vert_feed : float
        Cutting feed rates (mm/min) in the cutting plane and along the
        approach direction respectively.
    horiz_rapid, vert_rapid : float
        Rapid rates (mm/min) for non-cutting moves.
    max_feed : float or None
        Effective-feed clamp. None disables clamping.
    cutter_z_floor : float or None
        Per-layer radial-depth target. The emitted cutter Z is clamped to
        `max(surface_r + radial_stock_to_leave, cutter_z_floor)`, so the
        cutter never dives below `cutter_z_floor` even where the surface is
        deeper. None disables the clamp (single-pass / surface-follow).

    Returns
    -------
    list of Path.Command
    """
    if rotary_letter not in ("A", "B", "C"):
        raise ValueError("rotary_letter must be 'A', 'B' or 'C'; got %r" % rotary_letter)
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

    # Build a normalized 2-D radii view (list of lists of floats) so the
    # rest of the function is array-agnostic.
    grid = [[_radii_get(radii, i, j) for j in range(n_t)] for i in range(n_x)]

    # Conservative fallback for NaN cells: replace with the max valid
    # radius observed anywhere on the grid. This keeps the cutter
    # safely above missing surface data.
    valid = [v for row in grid for v in row if not math.isnan(v)]
    if not valid:
        raise ValueError("radii grid is entirely NaN; cannot build path")
    max_r = max(valid)
    for i in range(n_x):
        for j in range(n_t):
            if math.isnan(grid[i][j]):
                grid[i][j] = max_r

    # Spiral parameterization. Direction inverts for Conventional cut.
    direction = 1.0 if cut_mode == "Climb" else -1.0
    n_revs = (x_max - x_min) / axial_stepover
    total_theta_span = direction * n_revs * 2.0 * math.pi
    n_steps = max(1, int(math.ceil(abs(total_theta_span) / angular_resolution)))

    commands = []

    # Tracks centerline-feed clamp events for a single end-of-generate
    # warning rather than per-step log spam.
    feed_clamp = _FeedClamp()

    # Track the current machine position so every emitted move can be
    # fully qualified with X, Y, Z, and the rotary axis word, regardless
    # of which axes actually changed.
    x0, y0, _ = _world_xyz(rotary_axis, x_min, max_r + radial_stock_to_leave + 5.0)
    a_start_deg = math.degrees(theta_start)
    cur_x, cur_y, cur_z, cur_a = float(x0), float(y0), float(clearance_height), float(a_start_deg)

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

    # Approach: rapid to clearance, position, rotate to start, plunge to r0.
    _emit("G0", z=clearance_height, feed=vert_rapid)
    _emit("G0", x=x0, y=y0, feed=horiz_rapid)
    _emit("G0", a=a_start_deg, feed=horiz_rapid)

    # Drop to the radius at the very first sample.
    theta_mod0 = theta_start % (2.0 * math.pi)
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
    _emit("G1", z=r0, feed=vert_feed)

    # Spiral cut.
    last_r = r0
    for k in range(1, n_steps + 1):
        t = k / n_steps
        x = x_min + (x_max - x_min) * t
        theta_unwound = theta_start + total_theta_span * t

        # Reduce into grid range for interpolation.
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

        wx, wy, _ = _world_xyz(rotary_axis, x, r)
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
