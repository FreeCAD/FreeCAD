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

"""Drop-cutter surface generation using OpenCamLib.

Wraps OCL's BatchDropCutter, PathDropCutter, and AdaptivePathDropCutter
algorithms.  Includes CL-point filtering via LineCLFilter and G-code
generation from drop-cutter results.
"""

import Path
import time

__title__ = "Surface Drop-Cutter Generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


from Path.Base.Generator.surface_common import _get_ocl

# ---------------------------------------------------------------------------
# BatchDropCutter — grid-based surface scan
# ---------------------------------------------------------------------------


def batch_dropcutter(stl, cutter, polylines, min_z, threads=None, timer=None):
    """Run OCL BatchDropCutter on a list of pre-discretized polylines.

    Uses KDTree acceleration and OpenMP parallelism (via ``setThreads()``).
    This is the preferred method for complex, dense patterns (Spiral, Offset, etc.) as
    it leverages OCL's internal KD-Tree for massive spatial acceleration.

    Args:
        stl: ``ocl.STLSurf`` mesh.
        cutter: OCL cutter object.
        polylines: A list of scan lines, where each line is a list of (x,y,z) tuples.
        min_z: Minimum Z depth.
        threads: Number of OpenMP threads (*None* = auto-detect).
        timer: Optional ``timer(stage_name, elapsed_seconds)`` callback.

    Returns:
        A list of (x, y, z) tuples with Z-heights set by the drop-cutter
    """
    ocl = _get_ocl()

    bdc = ocl.BatchDropCutter()
    bdc.setSTL(stl)
    bdc.setCutter(cutter)

    if threads is not None and threads > 0:
        bdc.setThreads(threads)

    # Flatten the list of lines into a single stream of points.
    total_points = 0
    for line in polylines:
        for pt in line:
            bdc.appendPoint(ocl.CLPoint(pt[0], pt[1], min_z))
            total_points += 1

    t0 = time.time()
    bdc.run()
    t1 = time.time()

    if timer:
        timer("batch_dropcutter", t1 - t0)

    Path.Log.debug("batch_dropcutter: {} points in {:.3f}s".format(total_points, t1 - t0))

    # Extract results
    results = []
    for cl in bdc.getCLPoints():
        results.append((cl.x, cl.y, cl.z))

    return results


# ---------------------------------------------------------------------------
# PathDropCutter — path-based surface scan
# ---------------------------------------------------------------------------


def path_dropcutter(stl, cutter, ocl_path, min_z, sampling, timer=None):
    """Run OCL PathDropCutter along a path (Line/Arc segments).

    Fixed-interval sampling.  Use for circular/arc scan paths where
    BatchDropCutter is not applicable.

    Args:
        stl: ``ocl.STLSurf`` mesh.
        cutter: OCL cutter object.
        ocl_path: ``ocl.Path`` (sequence of Line/Arc).
        min_z: Minimum Z depth.
        sampling: Sampling interval along the path (mm).
        timer: Optional callback.

    Returns:
        List of ``(x, y, z)`` tuples.
    """
    ocl = _get_ocl()

    pdc = ocl.PathDropCutter()
    pdc.setSTL(stl)
    pdc.setCutter(cutter)
    pdc.setZ(min_z)
    pdc.setSampling(sampling)
    pdc.setPath(ocl_path)

    t0 = time.time()
    pdc.run()
    t1 = time.time()

    if timer:
        timer("path_dropcutter", t1 - t0)

    Path.Log.debug("path_dropcutter: {:.3f}s".format(t1 - t0))

    return [(cl.x, cl.y, cl.z) for cl in pdc.getCLPoints()]


# ---------------------------------------------------------------------------
# AdaptivePathDropCutter — adaptive-sampling path scan
# ---------------------------------------------------------------------------


def adaptive_path_dropcutter(stl, cutter, ocl_path, min_z, sampling, min_sampling=None, timer=None):
    """Run OCL AdaptivePathDropCutter along a path.

    Automatically refines sampling in areas of high curvature.
    Preferred over fixed-interval PathDropCutter for quality-critical
    finishing.

    Args:
        stl: ``ocl.STLSurf`` mesh.
        cutter: OCL cutter object.
        ocl_path: ``ocl.Path`` (sequence of Line/Arc).
        min_z: Minimum Z depth.
        sampling: Base sampling interval (mm).
        min_sampling: Minimum sampling interval for adaptive refinement.
                      If *None*, defaults to ``sampling / 10``.
        timer: Optional callback.

    Returns:
        List of ``(x, y, z)`` tuples.
    """
    ocl = _get_ocl()

    if min_sampling is None:
        min_sampling = sampling / 10.0

    apdc = ocl.AdaptivePathDropCutter()
    apdc.setSTL(stl)
    apdc.setCutter(cutter)
    apdc.setZ(min_z)
    apdc.setSampling(sampling)
    apdc.setMinSampling(min_sampling)
    apdc.setPath(ocl_path)

    t0 = time.time()
    apdc.run()
    t1 = time.time()

    if timer:
        timer("adaptive_path_dropcutter", t1 - t0)

    Path.Log.debug("adaptive_path_dropcutter: {:.3f}s".format(t1 - t0))

    return [(cl.x, cl.y, cl.z) for cl in apdc.getCLPoints()]


# ---------------------------------------------------------------------------
# G-code generation from drop-cutter data
# ---------------------------------------------------------------------------


def points_to_gcode(
    cl_points,
    horiz_feed,
    vert_rapid,
    horiz_rapid,
    safe_z,
    clearance_z,
    depth_offset=0.0,
):
    """Convert drop-cutter CL-point data to ``Path.Command`` list.

    Generates G1 cutting moves for the scan data, with G0 rapids for
    travel moves between disconnected segments.

    Note: Linear path optimization should be handled upstream by
    :func:`filter_cl_points` using OCL's LineCLFilter, not reimplemented
    here.

    Args:
        cl_points: List of ``(x, y, z)`` tuples (already filtered).
        horiz_feed: Horizontal feed rate (mm/min).
        vert_rapid: Vertical rapid feed rate (mm/min).
        horiz_rapid: Horizontal rapid feed rate (mm/min).
        safe_z: Safe height for travel moves.
        clearance_z: Clearance height.
        depth_offset: Optional Z offset applied to all points.

    Returns:
        List of ``Path.Command``.
    """
    if not cl_points:
        return []

    commands = []

    # Move to first point
    first = cl_points[0]
    commands.append(Path.Command("G0", {"Z": clearance_z, "F": vert_rapid}))
    commands.append(Path.Command("G0", {"X": first[0], "Y": first[1], "F": horiz_rapid}))

    # Cut through all points
    for pt in cl_points:
        z = pt[2] + depth_offset
        commands.append(Path.Command("G1", {"X": pt[0], "Y": pt[1], "Z": z, "F": horiz_feed}))

    return commands


def scan_lines_to_gcode(
    scan_lines,
    horiz_feed,
    vert_rapid,
    horiz_rapid,
    safe_z,
    clearance_z,
    depth_offset=0.0,
    optimize_transitions=False,
    safe_stl=None,
    cutter=None,
):
    """Convert multiple scan lines of CL-points to G-code with transitions.

    Each scan line is a list of ``(x, y, z)`` tuples.  Travel moves are
    inserted between scan lines.

    Args:
        scan_lines: List of scan lines, each a list of ``(x, y, z)`` tuples.
        horiz_feed: Horizontal feed rate.
        vert_rapid: Vertical rapid feed rate.
        horiz_rapid: Horizontal rapid feed rate.
        safe_z: Safe height.
        clearance_z: Clearance height.
        depth_offset: Optional Z offset.
        optimize_transitions: If True, short transitions (≤ 2× cutter
                              diameter) use a direct G1 feed move instead
                              of retracting.  Longer transitions fall back
                              to a full safe_z retract.
        safe_stl: Optional ``ocl.STLSurf`` for transition optimization.
        cutter: Optional OCL cutter for transition optimization.

    Returns:
        List of ``Path.Command``.
    """
    from Path.Base.Generator.surface_common import optimize_travel, _make_safe_pdc

    if not scan_lines:
        return []

    # Create the PDC once and reuse it for all transitions
    safe_pdc = None
    if optimize_transitions and safe_stl is not None and cutter is not None:
        safe_pdc = _make_safe_pdc(safe_stl, cutter, safe_z)

    commands = []
    commands.append(Path.Command("G0", {"Z": clearance_z, "F": vert_rapid}))

    last_point = None

    for line_idx, line in enumerate(scan_lines):
        if not line:
            continue

        first_pt = line[0]

        # Travel to start of this scan line
        if last_point is not None:
            if optimize_transitions:
                travel_cmds = optimize_travel(
                    last_point,
                    first_pt,
                    safe_z,
                    clearance_z,
                    horiz_feed,
                    horiz_rapid,
                    vert_rapid,
                    safe_pdc,
                    cutter,
                )
            else:
                travel_cmds = [
                    Path.Command("G0", {"Z": safe_z, "F": vert_rapid}),
                    Path.Command(
                        "G0",
                        {"X": first_pt[0], "Y": first_pt[1], "F": horiz_rapid},
                    ),
                ]
            commands.extend(travel_cmds)
        else:
            commands.append(
                Path.Command(
                    "G0",
                    {"X": first_pt[0], "Y": first_pt[1], "F": horiz_rapid},
                )
            )

        # Cut along this scan line
        for pt in line:
            z = pt[2] + depth_offset
            commands.append(Path.Command("G1", {"X": pt[0], "Y": pt[1], "Z": z, "F": horiz_feed}))

        last_point = line[-1]

    return commands


def apply_multipass(scan_lines, start_depth, final_depth, step_down):
    """
    Splits and clamps full-depth scan lines into multiple Z-levels for roughing.

    Args:
        scan_lines: List of scan lines, each a list of (x,y,z) tuples at full depth.
        start_depth: Top Z level to start cutting.
        final_depth: Final target Z level.
        step_down: Maximum depth per pass.

    Returns:
        A flat list of scan lines ordered by layer, suitable for scan_lines_to_gcode.
    """
    if step_down <= 0.0 or final_depth >= start_depth:
        return scan_lines

    # Generate depth levels from top to bottom
    depthparams = []
    curr_z = start_depth - step_down
    while curr_z > final_depth + 1e-5:
        depthparams.append(curr_z)
        curr_z -= step_down

    # Ensure final depth is always the last pass
    if not depthparams or abs(depthparams[-1] - final_depth) > 1e-5:
        depthparams.append(final_depth)

    all_multipass_lines = []

    for i, layDep in enumerate(depthparams):
        prvDep = start_depth if i == 0 else depthparams[i - 1]

        for line in scan_lines:
            current_segment = []
            for pt in line:
                x, y, z = pt

                # Check if the point is above the already-cleared material (adding a tiny
                # tolerance to prevent floating point noise from falsely lifting the tool)
                if z > prvDep + 1e-4:
                    if current_segment:
                        # End the current cutting segment because we hit the "air"
                        all_multipass_lines.append(current_segment)
                        current_segment = []
                else:
                    # Point is cutting material. Clamp to the current layer depth
                    new_z = layDep if z < layDep else z
                    current_segment.append((x, y, new_z))

            if current_segment:
                all_multipass_lines.append(current_segment)

    return all_multipass_lines
