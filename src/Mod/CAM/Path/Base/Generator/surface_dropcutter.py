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


def _build_ocl_path_from_polylines(ocl, polylines, min_z):
    """Build an ocl.Path from a list of polylines.

    Args:
        ocl: OCL module reference (already loaded).
        polylines: List of scan lines, each a list of ``(x, y, z)`` tuples.
        min_z: Z used for path points; the drop-cutter projects the
            actual surface Z anyway.

    Returns:
        ``ocl.Path`` containing one ``ocl.Line`` per polyline segment.
    """
    ocl_path = ocl.Path()
    for line in polylines:
        if len(line) < 2:
            continue
        for i in range(len(line) - 1):
            p1 = ocl.Point(line[i][0], line[i][1], min_z)
            p2 = ocl.Point(line[i + 1][0], line[i + 1][1], min_z)
            ocl_path.append(ocl.Line(p1, p2))
    return ocl_path


# ---------------------------------------------------------------------------
# BatchDropCutter — grid-based surface scan
# ---------------------------------------------------------------------------


def batch_dropcutter(stl, cutter, polylines, min_z, threads=None):
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

    Returns:
        A list of (x, y, z) tuples with Z-heights set by the drop-cutter
    """
    if stl is None or cutter is None:
        raise ValueError("stl and cutter must not be None")
    if not polylines:
        raise ValueError("polylines must be a non-empty list")
    if threads is not None and (not isinstance(threads, int) or threads <= 0):
        raise ValueError("threads must be a positive integer")

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

    Path.Log.debug("batch_dropcutter: {} points in {:.3f}s".format(total_points, t1 - t0))

    # Extract results
    results = []
    for cl in bdc.getCLPoints():
        results.append((cl.x, cl.y, cl.z))

    return results


# ---------------------------------------------------------------------------
# PathDropCutter — path-based surface scan
# ---------------------------------------------------------------------------


def path_dropcutter(stl, cutter, polylines, min_z, sampling):
    """Run OCL PathDropCutter along a path (Line/Arc segments).

    Fixed-interval sampling.  Use for circular/arc scan paths where
    BatchDropCutter is not applicable.

    Args:
        stl: ``ocl.STLSurf`` mesh.
        cutter: OCL cutter object.
        ocl_path: ``ocl.Path`` (sequence of Line/Arc).
        min_z: Minimum Z depth.
        sampling: Sampling interval along the path (mm).

    Returns:
        List of ``(x, y, z)`` tuples.
    """
    ocl = _get_ocl()

    ocl_path = _build_ocl_path_from_polylines(ocl, polylines, min_z)

    pdc = ocl.PathDropCutter()
    pdc.setSTL(stl)
    pdc.setCutter(cutter)
    pdc.setZ(min_z)
    pdc.setSampling(sampling)
    pdc.setPath(ocl_path)

    t0 = time.time()
    pdc.run()
    t1 = time.time()

    Path.Log.debug("path_dropcutter: {:.3f}s".format(t1 - t0))

    return [(cl.x, cl.y, cl.z) for cl in pdc.getCLPoints()]


# ---------------------------------------------------------------------------
# AdaptivePathDropCutter — adaptive-sampling path scan
# ---------------------------------------------------------------------------


def adaptive_path_dropcutter(stl, cutter, polylines, min_z, sampling, min_sampling=None):
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

    Returns:
        List of ``(x, y, z)`` tuples.
    """
    ocl = _get_ocl()

    ocl_path = _build_ocl_path_from_polylines(ocl, polylines, min_z)

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

    Path.Log.debug("adaptive_path_dropcutter: {:.3f}s".format(t1 - t0))

    return [(cl.x, cl.y, cl.z) for cl in apdc.getCLPoints()]
