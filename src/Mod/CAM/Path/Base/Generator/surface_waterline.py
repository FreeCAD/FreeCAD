# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
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

"""Waterline (constant-Z contour) generation using OpenCamLib.

Wraps OCL's Waterline (push-cutter + Weave) and AdaptiveWaterline
algorithms.  Also provides a fallback slice-based waterline using
FreeCAD's shape.slice() for when OCL is not available.
"""

import Path
import time

__title__ = "Surface Waterline Generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


from Path.Base.Generator.surface_common import _get_ocl


# ---------------------------------------------------------------------------
# OCL Waterline (push-cutter + Weave)
# ---------------------------------------------------------------------------


def waterline(stl, cutter, sampling, z, threads=None, timer=None):
    """Run OCL Waterline (push-cutter + Weave) at a single Z-height.

    This is the primary waterline algorithm.  It uses BatchPushCutter
    internally to push the cutter along X and Y fibers, then builds
    a Weave graph and extracts loops via face-traversal.

    This replaces the ~400-line custom topo-map code in the old
    Waterline.py "OCL Dropcutter" mode.

    Args:
        stl: ``ocl.STLSurf`` mesh.
        cutter: OCL cutter object.
        sampling: Fiber spacing (mm).
        z: Z-height for this waterline slice.
        threads: Number of OpenMP threads (*None* = auto-detect).
        timer: Optional ``timer(stage_name, elapsed_seconds)`` callback.

    Returns:
        List of loops.  Each loop is a list of ``(x, y, z)`` tuples.
    """
    ocl = _get_ocl()

    wl = ocl.Waterline()
    wl.setSTL(stl)
    wl.setCutter(cutter)
    wl.setSampling(sampling)
    wl.setZ(z)

    if threads is not None and threads > 0:
        wl.setThreads(threads)

    t0 = time.time()
    wl.run()
    t1 = time.time()

    if timer:
        timer("waterline_z{:.3f}".format(z), t1 - t0)

    raw_loops = wl.getLoops()
    loops = []
    for raw_loop in raw_loops:
        loop = [(pt.x, pt.y, pt.z) for pt in raw_loop]
        loops.append(loop)

    Path.Log.debug("waterline z={:.3f}: {} loops in {:.3f}s".format(z, len(loops), t1 - t0))

    return loops


# ---------------------------------------------------------------------------
# OCL AdaptiveWaterline
# ---------------------------------------------------------------------------


def adaptive_waterline(stl, cutter, sampling, z, min_sampling=None, threads=None, timer=None):
    """Run OCL AdaptiveWaterline at a single Z-height.

    Adaptive-sampling variant that refines fiber density where the
    contour changes rapidly.  Better quality than fixed-sampling
    Waterline but potentially slower.

    Args:
        stl: ``ocl.STLSurf`` mesh.
        cutter: OCL cutter object.
        sampling: Base fiber spacing (mm).
        z: Z-height for this waterline slice.
        min_sampling: Minimum sampling interval for adaptive refinement.
                      If *None*, defaults to ``sampling / 10``.
        threads: Number of OpenMP threads.
        timer: Optional callback.

    Returns:
        List of loops.  Each loop is a list of ``(x, y, z)`` tuples.
    """
    ocl = _get_ocl()

    if min_sampling is None:
        min_sampling = sampling / 10.0

    awl = ocl.AdaptiveWaterline()
    awl.setSTL(stl)
    awl.setCutter(cutter)
    awl.setSampling(sampling)
    awl.setMinSampling(min_sampling)
    awl.setZ(z)

    if threads is not None and threads > 0:
        awl.setThreads(threads)

    t0 = time.time()
    awl.run()
    t1 = time.time()

    if timer:
        timer("adaptive_waterline_z{:.3f}".format(z), t1 - t0)

    raw_loops = awl.getLoops()
    loops = []
    for raw_loop in raw_loops:
        loop = [(pt.x, pt.y, pt.z) for pt in raw_loop]
        loops.append(loop)

    Path.Log.debug(
        "adaptive_waterline z={:.3f}: {} loops in {:.3f}s".format(z, len(loops), t1 - t0)
    )

    return loops


# ---------------------------------------------------------------------------
# Multi-height waterline stack
# ---------------------------------------------------------------------------


def waterline_stack(
    stl,
    cutter,
    sampling,
    min_z,
    max_z,
    step_down,
    adaptive=False,
    min_sampling=None,
    threads=None,
    depth_offset=0.0,
    timer=None,
):
    """Generate waterline contours at multiple Z-heights.

    Convenience function that calls :func:`waterline` or
    :func:`adaptive_waterline` at each Z-level from *max_z* down to
    *min_z* in *step_down* increments.

    Args:
        stl: ``ocl.STLSurf`` mesh.
        cutter: OCL cutter object.
        sampling: Fiber spacing (mm).
        min_z: Final depth (lowest Z).
        max_z: Start depth (highest Z).
        step_down: Step-down increment between layers.
        adaptive: If True, use AdaptiveWaterline.
        min_sampling: Minimum sampling for adaptive mode.
        threads: OpenMP thread count.
        depth_offset: Z offset applied to all output points.
        timer: Optional callback.

    Returns:
        Ordered dict mapping ``z_height`` -> list of loops.
        Each loop is a list of ``(x, y, z)`` tuples.
    """
    from collections import OrderedDict

    # Compute Z-heights from max_z down to min_z
    z_heights = []
    z = max_z
    while z >= min_z - 1e-6:
        z_heights.append(z)
        z -= step_down

    # Ensure min_z is included
    if z_heights and abs(z_heights[-1] - min_z) > 1e-6:
        z_heights.append(min_z)

    result = OrderedDict()

    t0 = time.time()

    for zh in z_heights:
        if adaptive:
            loops = adaptive_waterline(
                stl,
                cutter,
                sampling,
                zh,
                min_sampling=min_sampling,
                threads=threads,
                timer=timer,
            )
        else:
            loops = waterline(
                stl,
                cutter,
                sampling,
                zh,
                threads=threads,
                timer=timer,
            )

        if not loops:
            Path.Log.debug("waterline_stack: no loops at z={:.3f}, skipping".format(zh))
            continue

        # Apply depth offset
        if abs(depth_offset) > 1e-9:
            offset_loops = []
            for loop in loops:
                offset_loops.append([(pt[0], pt[1], pt[2] + depth_offset) for pt in loop])
            result[zh] = offset_loops
        else:
            result[zh] = loops

    t1 = time.time()
    if timer:
        timer("waterline_stack_total", t1 - t0)

    Path.Log.debug(
        "waterline_stack: {} heights, {} total loops in {:.3f}s".format(
            len(z_heights),
            sum(len(v) for v in result.values()),
            t1 - t0,
        )
    )

    return result


# ---------------------------------------------------------------------------
# Slice-based waterline (no OCL required)
# ---------------------------------------------------------------------------


def slice_waterline(shape, min_z, max_z, step_down):
    """Extract waterlines using FreeCAD ``shape.slice()`` (no OCL required).

    Fallback for when OCL is not installed.  Uses OCCT's cross-section
    algorithm to slice the shape at each Z-height.

    Args:
        shape: A ``Part.Shape`` object.
        min_z: Final depth.
        max_z: Start depth.
        step_down: Step-down increment.

    Returns:
        Ordered dict mapping ``z_height`` -> list of wires.
        Each wire is a list of ``(x, y, z)`` tuples extracted from
        the wire vertices.
    """
    import FreeCAD
    from collections import OrderedDict

    z_heights = []
    z = max_z
    while z >= min_z - 1e-6:
        z_heights.append(z)
        z -= step_down
    if z_heights and abs(z_heights[-1] - min_z) > 1e-6:
        z_heights.append(min_z)

    result = OrderedDict()

    for zh in z_heights:
        try:
            wires = shape.slice(FreeCAD.Vector(0, 0, 1), zh)
        except Exception as e:
            Path.Log.warning("slice_waterline: failed at z={:.3f}: {}".format(zh, e))
            continue

        if not wires:
            continue

        loops = []
        for wire in wires:
            pts = []
            for vertex in wire.Vertexes:
                pts.append((vertex.X, vertex.Y, vertex.Z))
            # Close the loop if needed
            if pts and pts[0] != pts[-1]:
                pts.append(pts[0])
            loops.append(pts)

        if loops:
            result[zh] = loops

    return result


# ---------------------------------------------------------------------------
# G-code generation from waterline data
# ---------------------------------------------------------------------------


def waterline_to_gcode(
    waterline_data,
    horiz_feed,
    vert_rapid,
    horiz_rapid,
    safe_z,
    clearance_z,
    cut_climb=False,
):
    """Convert waterline contour data to ``Path.Command`` list.

    Handles single-pass and multi-pass layer modes, climb/conventional
    cut direction.

    Args:
        waterline_data: Ordered dict from :func:`waterline_stack` or
                        :func:`slice_waterline`.  Maps z_height -> list
                        of loops, where each loop is a list of
                        ``(x, y, z)`` tuples.
        horiz_feed: Horizontal feed rate (mm/min).
        vert_rapid: Vertical rapid feed rate (mm/min).
        horiz_rapid: Horizontal rapid feed rate (mm/min).
        safe_z: Safe height for travel moves.
        clearance_z: Clearance height.
        cut_climb: If True, reverse loop direction for climb cutting.

    Returns:
        List of ``Path.Command``.
    """
    commands = []
    commands.append(Path.Command("G0", {"Z": clearance_z, "F": vert_rapid}))

    for z_height, loops in waterline_data.items():
        for loop in loops:
            if not loop:
                continue

            # Optionally reverse for climb cutting
            pts = list(loop)
            if cut_climb:
                pts.reverse()

            first = pts[0]

            # Rapid to start of loop
            commands.append(Path.Command("G0", {"Z": safe_z, "F": vert_rapid}))
            commands.append(Path.Command("G0", {"X": first[0], "Y": first[1], "F": horiz_rapid}))

            # Cut the loop
            for pt in pts:
                commands.append(
                    Path.Command(
                        "G1",
                        {"X": pt[0], "Y": pt[1], "Z": pt[2], "F": horiz_feed},
                    )
                )

            # Retract after loop
            commands.append(Path.Command("G0", {"Z": safe_z, "F": vert_rapid}))

    return commands


def loop_to_gcode(loop, horiz_feed, vert_rapid, horiz_rapid, safe_z, cut_climb=False):
    """Convert a single waterline loop to ``Path.Command`` list.

    Args:
        loop: List of ``(x, y, z)`` tuples forming a closed contour.
        horiz_feed: Horizontal feed rate.
        vert_rapid: Vertical rapid feed rate.
        horiz_rapid: Horizontal rapid feed rate.
        safe_z: Safe height for travel moves.
        cut_climb: If True, reverse loop direction.

    Returns:
        List of ``Path.Command``.
    """
    if not loop:
        return []

    pts = list(loop)
    if cut_climb:
        pts.reverse()

    commands = []
    first = pts[0]

    # Rapid to start
    commands.append(Path.Command("G0", {"X": first[0], "Y": first[1], "F": horiz_rapid}))

    # Cut the loop
    for pt in pts:
        commands.append(
            Path.Command(
                "G1",
                {"X": pt[0], "Y": pt[1], "Z": pt[2], "F": horiz_feed},
            )
        )

    # Retract
    commands.append(Path.Command("G0", {"Z": safe_z, "F": vert_rapid}))

    return commands
