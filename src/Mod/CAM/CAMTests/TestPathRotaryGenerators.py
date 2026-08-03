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
import unittest

import numpy

import Path
from CAMTests import PathTestUtils

try:
    import ocl

    HAVE_OCL = True
except ImportError:
    try:
        import opencamlib as ocl

        HAVE_OCL = True
    except ImportError:
        HAVE_OCL = False

if HAVE_OCL:
    from Path.Base.Generator import rotary_dropcutter

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


def _add_quad(stl, p_l1, p_r1, p_r2, p_l2):
    """Add two triangles forming a quad to an OCL STLSurf."""
    stl.addTriangle(ocl.Triangle(p_l1, p_r1, p_r2))
    stl.addTriangle(ocl.Triangle(p_l1, p_r2, p_l2))


def _build_cylinder_stl(radius, x_min, x_max, n_seg=128):
    """Cylinder around +X axis with caps."""
    stl = ocl.STLSurf()
    for i in range(n_seg):
        a1 = 2 * math.pi * i / n_seg
        a2 = 2 * math.pi * (i + 1) / n_seg
        y1, z1 = radius * math.cos(a1), radius * math.sin(a1)
        y2, z2 = radius * math.cos(a2), radius * math.sin(a2)
        _add_quad(
            stl,
            ocl.Point(x_min, y1, z1),
            ocl.Point(x_max, y1, z1),
            ocl.Point(x_max, y2, z2),
            ocl.Point(x_min, y2, z2),
        )
        # End caps so OCL drop-cutter can find the surface from above
        # along the full x range without ray-edge artifacts.
        stl.addTriangle(
            ocl.Triangle(
                ocl.Point(x_min, 0, 0),
                ocl.Point(x_min, y1, z1),
                ocl.Point(x_min, y2, z2),
            )
        )
        stl.addTriangle(
            ocl.Triangle(
                ocl.Point(x_max, 0, 0),
                ocl.Point(x_max, y2, z2),
                ocl.Point(x_max, y1, z1),
            )
        )
    return stl


def _build_sphere_stl(radius, n_lat=32, n_lon=64):
    """Unit sphere centered at origin, axis-aligned with X.

    Built as a UV sphere whose 'poles' lie on the +X / -X axis so the
    rotation around X does not see seam artifacts.
    """
    stl = ocl.STLSurf()
    for i in range(n_lat):
        # latitude bands measured along +X axis
        phi1 = math.pi * (i / n_lat - 0.5)
        phi2 = math.pi * ((i + 1) / n_lat - 0.5)
        x1 = radius * math.sin(phi1)
        r1 = radius * math.cos(phi1)
        x2 = radius * math.sin(phi2)
        r2 = radius * math.cos(phi2)
        for j in range(n_lon):
            t1 = 2 * math.pi * j / n_lon
            t2 = 2 * math.pi * (j + 1) / n_lon
            p1 = ocl.Point(x1, r1 * math.cos(t1), r1 * math.sin(t1))
            p2 = ocl.Point(x2, r2 * math.cos(t1), r2 * math.sin(t1))
            p3 = ocl.Point(x2, r2 * math.cos(t2), r2 * math.sin(t2))
            p4 = ocl.Point(x1, r1 * math.cos(t2), r1 * math.sin(t2))
            stl.addTriangle(ocl.Triangle(p1, p2, p3))
            stl.addTriangle(ocl.Triangle(p1, p3, p4))
    return stl


def _build_cone_stl(r0, r1, x_min, x_max, n_seg=128):
    """Truncated cone along +X. r linear in x: r(x_min)=r0, r(x_max)=r1."""
    stl = ocl.STLSurf()
    for i in range(n_seg):
        a1 = 2 * math.pi * i / n_seg
        a2 = 2 * math.pi * (i + 1) / n_seg
        c1, s1 = math.cos(a1), math.sin(a1)
        c2, s2 = math.cos(a2), math.sin(a2)
        _add_quad(
            stl,
            ocl.Point(x_min, r0 * c1, r0 * s1),
            ocl.Point(x_max, r1 * c1, r1 * s1),
            ocl.Point(x_max, r1 * c2, r1 * s2),
            ocl.Point(x_min, r0 * c2, r0 * s2),
        )
        # End caps
        stl.addTriangle(
            ocl.Triangle(
                ocl.Point(x_min, 0, 0),
                ocl.Point(x_min, r0 * c1, r0 * s1),
                ocl.Point(x_min, r0 * c2, r0 * s2),
            )
        )
        stl.addTriangle(
            ocl.Triangle(
                ocl.Point(x_max, 0, 0),
                ocl.Point(x_max, r1 * c2, r1 * s2),
                ocl.Point(x_max, r1 * c1, r1 * s1),
            )
        )
    return stl


def _stl_copy(stl):
    """Triangle-by-triangle copy of an OCL STLSurf."""
    new = ocl.STLSurf()
    for tri in stl.getTriangles():
        pts = tri.getPoints()
        new.addTriangle(
            ocl.Triangle(
                ocl.Point(pts[0].x, pts[0].y, pts[0].z),
                ocl.Point(pts[1].x, pts[1].y, pts[1].z),
                ocl.Point(pts[2].x, pts[2].y, pts[2].z),
            )
        )
    return new


def _reference_sample(stl, axis_vec, xs, thetas):
    """Slow reference: rebuild a fresh rotated STL for each theta.

    Used to verify that the production implementation's incremental
    rotation accumulates negligible error.
    """
    n_x = len(xs)
    n_t = len(thetas)
    radii = numpy.full((n_x, n_t), float("nan"), dtype=float)
    for j, theta in enumerate(thetas):
        fresh = _stl_copy(stl)
        # Use the production implementation on a single-theta call
        # but with a fresh STL, then read the column.
        col = rotary_dropcutter.sample(fresh, axis_vec, xs, [theta])
        radii[:, j] = col[:, 0]
    return radii


@unittest.skipUnless(HAVE_OCL, "OpenCamLib not available")
class TestPathRotaryGenerators(PathTestUtils.PathTestBase):
    """Analytic-shape verification of the rotary drop-cutter sampler."""

    def assertRadiiClose(self, expected, actual, tol):
        """Assert two radii grids are close, with NaN-aware comparison."""
        self.assertEqual(
            expected.shape,
            actual.shape,
            msg="Shape mismatch: expected {}, got {}".format(expected.shape, actual.shape),
        )
        # NaN-aware comparison; expected may have NaN where actual must too
        for i in range(actual.shape[0]):
            for j in range(actual.shape[1]):
                e = expected[i, j]
                a = actual[i, j]
                if math.isnan(e):
                    self.assertTrue(
                        math.isnan(a),
                        msg="[{}, {}] expected NaN, got {}".format(i, j, a),
                    )
                else:
                    self.assertFalse(
                        math.isnan(a),
                        msg="[{}, {}] expected {}, got NaN".format(i, j, e),
                    )
                    self.assertAlmostEqual(
                        a,
                        e,
                        delta=tol,
                        msg="[{}, {}] expected {}, got {}".format(i, j, e, a),
                    )

    def test00_cylinder_constant_radius(self):
        """Cylinder of radius 10 around +X: r is 10 for all (x, theta)."""
        stl = _build_cylinder_stl(10.0, -5.0, 5.0)
        xs = [-3.0, -1.0, 0.0, 1.0, 3.0]
        thetas = [0.0, 0.5, 1.0, 2.0, math.pi, math.pi + 1.0]
        radii = rotary_dropcutter.sample(stl, (1.0, 0.0, 0.0), xs, thetas)
        expected = numpy.full((len(xs), len(thetas)), 10.0)
        self.assertRadiiClose(expected, radii, tol=0.05)

    def test01_sphere_radius_varies_with_x(self):
        """Unit sphere around origin: r(x) = sqrt(1 - x^2)."""
        stl = _build_sphere_stl(1.0)
        xs = [-0.8, -0.5, 0.0, 0.3, 0.7]
        thetas = [0.0, 1.0, 2.5]
        radii = rotary_dropcutter.sample(stl, (1.0, 0.0, 0.0), xs, thetas)
        expected = numpy.zeros((len(xs), len(thetas)))
        for i, x in enumerate(xs):
            for j in range(len(thetas)):
                expected[i, j] = math.sqrt(1.0 - x * x)
        self.assertRadiiClose(expected, radii, tol=0.02)

    def test02_cone_radius_linear_in_x(self):
        """Truncated cone: r(x_min)=2, r(x_max)=8 -> linear in x."""
        x_min, x_max = -3.0, 3.0
        r0, r1 = 2.0, 8.0
        stl = _build_cone_stl(r0, r1, x_min, x_max)
        xs = [-2.5, -1.0, 0.0, 1.0, 2.5]
        thetas = [0.0, math.pi / 4, math.pi, -math.pi / 3]
        radii = rotary_dropcutter.sample(stl, (1.0, 0.0, 0.0), xs, thetas)
        expected = numpy.zeros((len(xs), len(thetas)))
        for i, x in enumerate(xs):
            r_x = r0 + (r1 - r0) * (x - x_min) / (x_max - x_min)
            for j in range(len(thetas)):
                expected[i, j] = r_x
        self.assertRadiiClose(expected, radii, tol=0.05)

    def test03_y_axis_rotary(self):
        """Cylinder around +Y axis sampled with axis_vec=(0,1,0)."""
        stl = ocl.STLSurf()
        radius = 5.0
        y_min, y_max = -4.0, 4.0
        n_seg = 128
        for i in range(n_seg):
            a1 = 2 * math.pi * i / n_seg
            a2 = 2 * math.pi * (i + 1) / n_seg
            c1, s1 = math.cos(a1), math.sin(a1)
            c2, s2 = math.cos(a2), math.sin(a2)
            _add_quad(
                stl,
                ocl.Point(radius * c1, y_min, radius * s1),
                ocl.Point(radius * c1, y_max, radius * s1),
                ocl.Point(radius * c2, y_max, radius * s2),
                ocl.Point(radius * c2, y_min, radius * s2),
            )
            stl.addTriangle(
                ocl.Triangle(
                    ocl.Point(0, y_min, 0),
                    ocl.Point(radius * c2, y_min, radius * s2),
                    ocl.Point(radius * c1, y_min, radius * s1),
                )
            )
            stl.addTriangle(
                ocl.Triangle(
                    ocl.Point(0, y_max, 0),
                    ocl.Point(radius * c1, y_max, radius * s1),
                    ocl.Point(radius * c2, y_max, radius * s2),
                )
            )
        xs = [-3.0, 0.0, 3.0]
        thetas = [0.0, 1.0, 3.0]
        radii = rotary_dropcutter.sample(stl, (0.0, 1.0, 0.0), xs, thetas)
        expected = numpy.full((len(xs), len(thetas)), radius)
        self.assertRadiiClose(expected, radii, tol=0.05)

    def test04_incremental_matches_fresh(self):
        """Production (incremental rotation) matches fresh-STL reference."""
        stl = _build_sphere_stl(1.0)
        xs = [-0.6, -0.2, 0.1, 0.5]
        thetas = [0.0, 0.7, 1.4, 2.1, 2.8, math.pi - 0.05]
        production = rotary_dropcutter.sample(stl, (1.0, 0.0, 0.0), xs, thetas)
        reference = _reference_sample(stl, (1.0, 0.0, 0.0), xs, thetas)
        self.assertRadiiClose(reference, production, tol=1e-6)

    def test05_restore_stl_after_call(self):
        """sample() leaves the STL in (approximately) its original pose."""
        stl = _build_cylinder_stl(10.0, -5.0, 5.0)
        bb_before = (
            stl.bb.minpt.x,
            stl.bb.minpt.y,
            stl.bb.minpt.z,
            stl.bb.maxpt.x,
            stl.bb.maxpt.y,
            stl.bb.maxpt.z,
        )
        rotary_dropcutter.sample(stl, (1.0, 0.0, 0.0), [-2.0, 0.0, 2.0], [0.0, 1.5, 3.0])
        bb_after = (
            stl.bb.minpt.x,
            stl.bb.minpt.y,
            stl.bb.minpt.z,
            stl.bb.maxpt.x,
            stl.bb.maxpt.y,
            stl.bb.maxpt.z,
        )
        for a, b in zip(bb_before, bb_after):
            self.assertAlmostEqual(a, b, delta=1e-3)

    def test06_rejects_non_axis_aligned_axis(self):
        """Non-axis-aligned rotary axis raises NotImplementedError."""
        stl = _build_cylinder_stl(5.0, -1.0, 1.0)
        with self.assertRaises(NotImplementedError):
            rotary_dropcutter.sample(stl, (1.0, 1.0, 0.0), [0.0], [0.0])

    def test07_rejects_zero_axis(self):
        """Zero vector axis raises ValueError."""
        stl = _build_cylinder_stl(5.0, -1.0, 1.0)
        with self.assertRaises(ValueError):
            rotary_dropcutter.sample(stl, (0.0, 0.0, 0.0), [0.0], [0.0])

    def test08_empty_inputs_return_empty(self):
        """Empty xs or thetas returns appropriately-shaped empty array."""
        stl = _build_cylinder_stl(5.0, -1.0, 1.0)
        out = rotary_dropcutter.sample(stl, (1.0, 0.0, 0.0), [], [0.0])
        self.assertEqual(out.shape, (0, 1))
        out = rotary_dropcutter.sample(stl, (1.0, 0.0, 0.0), [0.0], [])
        self.assertEqual(out.shape, (1, 0))
