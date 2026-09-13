# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Dimitrios Pana <dimitriospana75@gmail.com>
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

"""Waterline (constant-Z contour) generation using OpenCamLib.

Wraps OCL's Waterline (push-cutter + Weave) and AdaptiveWaterline
algorithms.  Also provides a fallback slice-based waterline using
FreeCAD's shape.slice() for when OCL is not available.
"""

import Path
import time
import math

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


def adaptive_waterline(stl, cutter, sampling, min_sampling, z, threads=None, timer=None):
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
        threads: Number of OpenMP threads.
        timer: Optional callback.

    Returns:
        List of loops.  Each loop is a list of ``(x, y, z)`` tuples.
    """
    ocl = _get_ocl()

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
    min_sampling,
    min_z,
    max_z,
    step_down,
    adaptive=False,
    depth_offset=0.0,
    threads=None,
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
        min_sampling: Minimum sampling for adaptive mode.
        min_z: Final depth (lowest Z).
        max_z: Start depth (highest Z).
        step_down: Step-down increment between layers.
        adaptive: If True, use AdaptiveWaterline.
        threads: OpenMP thread count.
        depth_offset: Z offset applied to all output points.
        timer: Optional callback.

    Returns:
        Ordered dict mapping ``z_height`` -> list of loops.
        Each loop is a list of ``(x, y, z)`` tuples.
    """
    from collections import OrderedDict

    # Add a small epsilon to the slicing Z-level to avoid Z-fighting
    # with simplified planar meshes.
    epsilon = 0.001

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

    for z in z_heights:
        zh = z + epsilon
        if adaptive:
            loops = adaptive_waterline(
                stl,
                cutter,
                sampling,
                min_sampling,
                zh,
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
# G-code generation from waterline data
# ---------------------------------------------------------------------------


def _reorient_loop_start(loop, start_pt):
    """
    Cycles a raw list of (x,y,z) points so the first point is closest
    to the given start_pt (x, y).
    """
    if not loop or not start_pt:
        return loop

    min_dist = float("inf")
    closest_idx = 0

    for i, pt in enumerate(loop):
        dist = math.hypot(pt[0] - start_pt[0], pt[1] - start_pt[1])
        if dist < min_dist:
            min_dist = dist
            closest_idx = i

    if closest_idx == 0:
        return loop

    # Cycle the list
    return loop[closest_idx:] + loop[:closest_idx]


def waterline_to_gcode(
    waterline_data,
    horiz_feed,
    vert_feed,
    horiz_rapid,
    vert_rapid,
    safe_z,
    clearance_z,
    cut_climb=False,
):
    """Convert waterline contour data to ``Path.Command`` list.

    Handles single-pass and multi-pass layer modes, climb/conventional
    cut direction, and features a TSP optimizer to sort loops and align
    start seams.

    Includes Smart Retraction logic: Tool will stay engaged in the material
    between loops if the next starting coordinate is microscopic or vertical.

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

    current_tool_pos = None
    is_retracted = True  # Tracks if the tool is currently safely above the material

    # Tolerance (mm) to determine if the next loop starts at the exact same location
    STAY_DOWN_TOLERANCE = 0.01

    for raw_loops in waterline_data.values():
        # Copy the list so we can pop loops out as we process them
        remaining_loops = list(raw_loops)

        while remaining_loops:
            chosen_loop = None

            # 1. Loop sorting: Find the nearest loop to the tool
            if current_tool_pos:
                best_idx = 0
                best_dist = float("inf")

                for i, loop in enumerate(remaining_loops):
                    for pt in loop:
                        d = math.hypot(pt[0] - current_tool_pos[0], pt[1] - current_tool_pos[1])
                        if d < best_dist:
                            best_dist = d
                            best_idx = i

                chosen_loop = remaining_loops.pop(best_idx)
            else:
                chosen_loop = remaining_loops.pop(0)

            # 2. Seam alignment: Rotate the points to start near the tool
            if current_tool_pos:
                chosen_loop = _reorient_loop_start(chosen_loop, current_tool_pos)

            pts = list(chosen_loop)

            # Connect the last and the first points to close the loop
            if math.hypot(pts[0][0] - pts[-1][0], pts[0][1] - pts[-1][1]) > 1e-5:
                pts.append(pts[0])

            # Reverse for climb cutting
            if cut_climb:
                pts.reverse()

            first = pts[0]

            # 3. Smart retraction and transit
            if current_tool_pos:
                dist = math.hypot(first[0] - current_tool_pos[0], first[1] - current_tool_pos[1])
            else:
                dist = float("inf")

            # Only retract and rapid if the tool is moving to a new XY location
            if dist > STAY_DOWN_TOLERANCE:
                if not is_retracted:
                    commands.append(Path.Command("G0", {"Z": safe_z, "F": vert_rapid}))
                    is_retracted = True

                commands.append(
                    Path.Command("G0", {"X": first[0], "Y": first[1], "F": horiz_rapid})
                )

            # 4. Cut the loop
            for i, pt in enumerate(pts):
                # If we stayed down, this first command acts as a vertical G1 plunge to the new Z-depth
                feed = vert_feed if i == 0 else horiz_feed
                commands.append(
                    Path.Command(
                        "G1",
                        {"X": pt[0], "Y": pt[1], "Z": pt[2], "F": feed},
                    )
                )

            # 5. Update magnet state
            current_tool_pos = (pts[-1][0], pts[-1][1])
            is_retracted = False  # The tool is now down in the material

    # 6. Final retract
    if not is_retracted:
        commands.append(Path.Command("G0", {"Z": safe_z, "F": vert_rapid}))

    return commands
