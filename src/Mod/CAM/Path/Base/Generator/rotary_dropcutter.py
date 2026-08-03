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
Rotary drop-cutter sampling.

Sample radii r(x, theta) for a part mounted on a single rotary axis.

The rotary axis is rotated into +X canonical pose, then for each theta
the STL is rotated by -theta around X so the angular position 'theta'
on the part faces +Z. OCL's BatchDropCutter is used to drop a cutter
from above onto the (x, 0) probe points; the contact Z values are the
surface radii at (x, theta).

This is the deep, FreeCAD-free core of the Rotary Surface operation.
The single public function `sample` takes an OCL STLSurf and plain
numeric arrays; it returns a numpy array of radii.
"""

import math
import numpy

try:
    import ocl
except ImportError:
    import opencamlib as ocl


__title__ = "Rotary Drop-Cutter Sampler"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Sample r(x, theta) for a part on a rotary axis."

__all__ = ["sample"]


_AXES = {
    (1.0, 0.0, 0.0): (0.0, 0.0, 0.0),
    (-1.0, 0.0, 0.0): (0.0, 0.0, math.pi),
    (0.0, 1.0, 0.0): (0.0, 0.0, -math.pi / 2.0),
    (0.0, -1.0, 0.0): (0.0, 0.0, math.pi / 2.0),
    (0.0, 0.0, 1.0): (0.0, math.pi / 2.0, 0.0),
    (0.0, 0.0, -1.0): (0.0, -math.pi / 2.0, 0.0),
}


def _normalize(v):
    n = math.sqrt(v[0] ** 2 + v[1] ** 2 + v[2] ** 2)
    if n < 1e-12:
        raise ValueError("axis_vec must be a non-zero vector")
    return (v[0] / n, v[1] / n, v[2] / n)


def _canonical_rotation(axis_vec):
    """Return (rx, ry, rz) Euler angles to rotate axis_vec onto +X.

    OCL's STLSurf.rotate applies rotations in X-then-Y-then-Z order. v1
    supports only the six axis-aligned rotary directions; non-aligned
    inputs raise NotImplementedError.
    """
    v = _normalize(axis_vec)
    for axis, angles in _AXES.items():
        if all(abs(a - b) < 1e-6 for a, b in zip(v, axis)):
            return angles
    raise NotImplementedError(
        "v1 supports only axis-aligned rotary axes (+/- X, Y, Z); got %r" % (v,)
    )


def _rotate_stl(stl, rx, ry, rz):
    if rx == 0.0 and ry == 0.0 and rz == 0.0:
        return
    stl.rotate(rx, ry, rz)


def _default_point_cutter():
    return ocl.CylCutter(0.001, 1.0)


def _drop_floor(stl):
    """A Z value safely below the stock so 'no contact' is detectable."""
    bb = stl.bb
    span = max(
        abs(bb.maxpt.x - bb.minpt.x),
        abs(bb.maxpt.y - bb.minpt.y),
        abs(bb.maxpt.z - bb.minpt.z),
    )
    return bb.minpt.z - max(span * 10.0, 1000.0)


def sample(stl, axis_vec, xs, thetas, cutter=None):
    """Sample surface radii r(x, theta) for a part on a single rotary axis.

    Parameters
    ----------
    stl : ocl.STLSurf
        Tessellated part surface in world coordinates. Mutated and
        restored during the call (rotated in place, then rotated back).
    axis_vec : sequence of 3 floats
        Rotary-axis direction in world coordinates. v1 supports the six
        axis-aligned directions (+/- X, Y, Z) only.
    xs : sequence of floats
        Axial probe coordinates, in the canonical +X frame.
    thetas : sequence of floats
        Angular probe coordinates in radians, right-hand rule about the
        rotary axis.
    cutter : ocl cutter, optional
        OCL cutter for the drop. Default is a near-zero-radius
        cylindrical cutter that returns the bare surface radius.

    Returns
    -------
    radii : numpy.ndarray of shape (len(xs), len(thetas))
        Surface radii at each (xs[i], thetas[j]). NaN where no surface
        contact was found.
    """
    xs_arr = numpy.asarray(xs, dtype=float)
    thetas_arr = numpy.asarray(thetas, dtype=float)
    if xs_arr.ndim != 1 or thetas_arr.ndim != 1:
        raise ValueError("xs and thetas must be 1-D")

    n_x = xs_arr.size
    n_t = thetas_arr.size
    radii = numpy.full((n_x, n_t), float("nan"), dtype=float)
    if n_x == 0 or n_t == 0:
        return radii

    if cutter is None:
        cutter = _default_point_cutter()

    rx_can, ry_can, rz_can = _canonical_rotation(axis_vec)
    _rotate_stl(stl, rx_can, ry_can, rz_can)

    floor = _drop_floor(stl)
    cumulative = 0.0
    try:
        for j, theta in enumerate(thetas_arr):
            target = -float(theta)
            delta = target - cumulative
            _rotate_stl(stl, delta, 0.0, 0.0)
            cumulative = target

            bdc = ocl.BatchDropCutter()
            bdc.setSTL(stl)
            bdc.setCutter(cutter)
            for x in xs_arr:
                bdc.appendPoint(ocl.CLPoint(float(x), 0.0, floor))
            bdc.run()
            results = bdc.getCLPoints()

            for i, p in enumerate(results):
                if i >= n_x:
                    break
                z = p.z
                if z <= floor + 1e-9:
                    radii[i, j] = float("nan")
                else:
                    radii[i, j] = z
    finally:
        if cumulative != 0.0:
            _rotate_stl(stl, -cumulative, 0.0, 0.0)
        if rz_can != 0.0:
            _rotate_stl(stl, 0.0, 0.0, -rz_can)
        if ry_can != 0.0:
            _rotate_stl(stl, 0.0, -ry_can, 0.0)
        if rx_can != 0.0:
            _rotate_stl(stl, -rx_can, 0.0, 0.0)

    return radii
