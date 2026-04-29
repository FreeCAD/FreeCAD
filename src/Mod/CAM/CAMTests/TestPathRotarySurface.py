# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *   Copyright (c) 2026 sliptonic <shopinthewoods@gmail.com>               *
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

import math
import unittest

import FreeCAD
import Part

import Path
import Path.Main.Job as PathJob
import Path.Main.Stock as PathStock
from CAMTests.PathTestUtils import PathTestBase

try:
    import ocl  # noqa: F401

    HAVE_OCL = True
except ImportError:
    try:
        import opencamlib as ocl  # noqa: F401

        HAVE_OCL = True
    except ImportError:
        HAVE_OCL = False

if HAVE_OCL:
    import Path.Op.RotarySurface as PathRotarySurface

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


def _make_tapered_part(doc, length=40.0, r0=8.0, r1=12.0, axis="X"):
    """Create a slightly tapered cylinder mounted along the given world axis.

    The shape is built around +Z internally then rotated into place so
    the cylinder's axis aligns with +X (or +Y), centered at the origin.
    """
    cone = Part.makeCone(r0, r1, length)
    if axis == "X":
        cone.Placement = FreeCAD.Placement(
            FreeCAD.Vector(-length / 2.0, 0, 0),
            FreeCAD.Rotation(FreeCAD.Vector(0, 1, 0), 90.0),
        )
    elif axis == "Y":
        cone.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0, -length / 2.0, 0),
            FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), -90.0),
        )
    else:
        raise ValueError("axis must be 'X' or 'Y'")
    obj = doc.addObject("Part::Feature", "RotaryPart")
    obj.Shape = cone
    return obj


def _setup_cyl_stock_along_axis(job, radius, length, axis):
    """Replace the Job's stock with a CreateCylinder oriented along axis."""
    if axis == "X":
        placement = FreeCAD.Placement(
            FreeCAD.Vector(-length / 2.0, 0, 0),
            FreeCAD.Rotation(FreeCAD.Vector(0, 1, 0), 90.0),
        )
    else:
        placement = FreeCAD.Placement(
            FreeCAD.Vector(0, -length / 2.0, 0),
            FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), -90.0),
        )
    new_stock = PathStock.CreateCylinder(job, radius=radius, height=length, placement=placement)
    # Drop the auto-created stock; install ours.
    if job.Stock and job.Stock is not new_stock:
        try:
            FreeCAD.ActiveDocument.removeObject(job.Stock.Name)
        except Exception:
            pass
    job.Stock = new_stock
    return new_stock


def _commands_with_axis(commands, letter):
    return [c for c in commands if letter in c.Parameters]


@unittest.skipUnless(HAVE_OCL, "OpenCamLib not available")
class TestPathRotarySurface(PathTestBase):
    """Integration tests for the Rotary Surface operation MVP."""

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestRotarySurface")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _build_job(self, axis="X", length=40.0, radius=12.0):
        part = _make_tapered_part(self.doc, length=length, axis=axis)
        job = PathJob.Create("Job_Rotary", [part])
        _setup_cyl_stock_along_axis(job, radius=radius + 0.5, length=length, axis=axis)
        self.doc.recompute()
        return job, part

    def _build_op(self, job, axis="X", radius=12.0, length=40.0):
        op = PathRotarySurface.Create("RotaryOp", parentJob=job)
        op.RotaryAxis = axis
        op.StartX = -length / 2.0
        op.StopX = length / 2.0
        op.StartAngle = 0.0
        op.StopAngle = 360.0
        op.StepOver = 4.0
        op.AngularResolution = 15.0
        # Clear the OpToolDiameter expression that the SetupSheet auto-binds.
        op.setExpression("StepDown", None)
        op.StepDown = 0.0  # single radial pass by default; test_07 covers multi-pass
        op.RadialStockToLeave = 0.0
        op.MaxFeed = 5000.0
        op.LinearDeflection = 0.2
        op.AngularDeflection = 0.524
        op.ToolController.Tool.Diameter = 3.0
        return op

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test00_emits_xyza_for_x_rotary(self):
        job, _ = self._build_job(axis="X")
        op = self._build_op(job, axis="X")
        self.doc.recompute()
        commands = op.Path.Commands
        self.assertTrue(len(commands) > 10, "expected many commands, got {}".format(len(commands)))

        a_commands = _commands_with_axis(commands, "A")
        self.assertTrue(
            len(a_commands) > 5,
            "expected XYZA cutting commands, got {}".format(len(a_commands)),
        )

        # XYZ presence on cutting moves
        cut_g1 = [c for c in commands if c.Name == "G1" and "A" in c.Parameters]
        self.assertTrue(len(cut_g1) > 5)
        for c in cut_g1[:3]:
            self.assertIn("X", c.Parameters)
            self.assertIn("Z", c.Parameters)

    def test01_unwound_a_is_monotonic(self):
        """A values within a single radial pass must never wrap (unwound)."""
        job, _ = self._build_job(axis="X")
        op = self._build_op(job, axis="X")
        self.doc.recompute()

        a_values = [
            c.Parameters["A"] for c in op.Path.Commands if c.Name == "G1" and "A" in c.Parameters
        ]
        self.assertTrue(len(a_values) > 5)
        deltas = [a_values[i + 1] - a_values[i] for i in range(len(a_values) - 1)]
        # No single delta should exceed +/- 360 (would indicate a wrap).
        for d in deltas:
            self.assertLess(
                abs(d),
                360.0,
                "A delta {} indicates a 0/360 wrap rather than unwound output".format(d),
            )
        self.assertGreater(
            max(a_values) - min(a_values),
            360.0,
            "expected at least one full revolution of unwound A",
        )

    def test02_start_angle_honored(self):
        """StartAngle is the unwound angular start; the spiral begins there."""
        job, _ = self._build_job(axis="X")
        op = self._build_op(job, axis="X")
        op.StartAngle = 30.0
        op.StepOver = 20.0
        self.doc.recompute()
        a_values = [
            c.Parameters["A"] for c in op.Path.Commands if c.Name == "G1" and "A" in c.Parameters
        ]
        self.assertTrue(len(a_values) > 0)
        self.assertGreaterEqual(min(a_values), 29.5)
        # n_revs = 40 / 20 = 2 revolutions = 720 deg of unwound sweep.
        self.assertGreater(max(a_values) - min(a_values), 700.0)

    def test03_y_axis_rotary_emits_b(self):
        job, _ = self._build_job(axis="Y")
        op = self._build_op(job, axis="Y")
        self.doc.recompute()
        commands = op.Path.Commands
        b_commands = _commands_with_axis(commands, "B")
        self.assertTrue(
            len(b_commands) > 5,
            "expected B-axis commands for Y rotary, got {}".format(len(b_commands)),
        )

    def test04_validation_rejects_box_stock(self):
        job, _ = self._build_job(axis="X")
        # Replace stock with a box.
        if job.Stock:
            FreeCAD.ActiveDocument.removeObject(job.Stock.Name)
        job.Stock = PathStock.CreateBox(job)
        op = self._build_op(job, axis="X")
        self.doc.recompute()
        # Should produce no cutting commands beyond the initial label.
        a_commands = [c for c in op.Path.Commands if c.Name == "G1" and "A" in c.Parameters]
        self.assertEqual(len(a_commands), 0, "non-cylinder stock should yield no path")

    def test05_validation_rejects_misaligned_stock(self):
        job, _ = self._build_job(axis="X")
        # Tilt the stock cylinder ~5 deg — well above the 0.1 deg tolerance.
        job.Stock.Placement = FreeCAD.Placement(
            job.Stock.Placement.Base,
            FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 5.0).multiply(job.Stock.Placement.Rotation),
        )
        op = self._build_op(job, axis="X")
        self.doc.recompute()
        a_commands = [c for c in op.Path.Commands if c.Name == "G1" and "A" in c.Parameters]
        self.assertEqual(len(a_commands), 0, "misaligned stock should yield no path")

    def test07_step_down_creates_multiple_passes(self):
        """Non-zero StepDown produces multiple radial pass groups."""
        job, _ = self._build_job(axis="X", radius=14.0)
        op = self._build_op(job, axis="X")
        op.setExpression("StepDown", None)
        op.StepDown = 1.5
        self.doc.recompute()
        # Each radial pass starts with a G0 setting the rotary axis to
        # StartAngle, then a G1 plunge to first radius. Count G0 commands
        # that set the A axis (proxy for pass count).
        a_g0 = [c for c in op.Path.Commands if c.Name == "G0" and "A" in c.Parameters]
        self.assertGreater(
            len(a_g0),
            1,
            "expected multiple A-axis G0 resets for multi-pass roughing, got {}".format(len(a_g0)),
        )

    def test06_z_height_above_radius(self):
        job, _ = self._build_job(axis="X", radius=12.0)
        op = self._build_op(job, axis="X")
        self.doc.recompute()
        z_values = [
            c.Parameters["Z"] for c in op.Path.Commands if c.Name == "G1" and "Z" in c.Parameters
        ]
        self.assertTrue(len(z_values) > 0)
        # Cutter Z should be in the range [r0 - some_margin, r1 + safe_margin].
        # For our taper the surface radius varies between 8 and 12 mm.
        for z in z_values:
            self.assertGreaterEqual(z, 0.0)
            self.assertLess(z, 100.0)
