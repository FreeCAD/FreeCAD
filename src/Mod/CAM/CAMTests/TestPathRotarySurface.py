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


def _install_stub_machine(job, axis, min_limit=-360.0, max_limit=360.0):
    """Attach a single-rotary-axis stub Machine to the Job.

    The Rotary Surface op now requires a Machine with at least one
    rotary axis, so every test fixture installs one matching the
    fixture's world axis. ``axis`` is "X" (A around X) or "Y" (B
    around Y).
    """
    from Machine.models.machine import RotaryAxis

    if axis == "X":
        name = "A"
        rot_vec = FreeCAD.Vector(1, 0, 0)
    elif axis == "Y":
        name = "B"
        rot_vec = FreeCAD.Vector(0, 1, 0)
    else:
        raise ValueError("axis must be 'X' or 'Y'")

    class _StubMachine:
        rotary_axes = {
            name: RotaryAxis(
                name=name,
                rotation_vector=rot_vec,
                min_limit=float(min_limit),
                max_limit=float(max_limit),
            )
        }

    job.Proxy.getMachine = lambda: _StubMachine()


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
        _install_stub_machine(job, axis)
        self.doc.recompute()
        return job, part

    def _build_op(self, job, axis="X", radius=12.0, length=40.0):
        op = PathRotarySurface.Create("RotaryOp", parentJob=job)
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

    # ------------------------------------------------------------------
    # Hardening: warnings + feed clamp (Issue #4)
    # ------------------------------------------------------------------

    def _capture_warnings(self):
        """Context-manager-style helper that returns a list to append into.

        Replaces ``Path.Log.warning`` with a collector for the duration
        of a ``with``-block-equivalent. Caller uses ``addCleanup`` to
        restore the original.
        """
        captured = []
        original = Path.Log.warning

        def _capture(msg):
            captured.append(str(msg))
            return original(msg)

        Path.Log.warning = _capture
        self.addCleanup(lambda: setattr(Path.Log, "warning", original))
        return captured

    def test08_offcenter_part_warns(self):
        """Part centroid offset > LinearDeflection emits a warning."""
        warnings = self._capture_warnings()
        # Build the part directly, shift it 5 mm off the rotary axis,
        # then hand it to the Job so the Job's resource clone inherits
        # the off-center geometry.
        part = _make_tapered_part(self.doc, length=40.0, axis="X")
        part.Placement = FreeCAD.Placement(
            FreeCAD.Vector(part.Placement.Base.x, 5.0, part.Placement.Base.z),
            part.Placement.Rotation,
        )
        self.doc.recompute()
        job = PathJob.Create("Job_Rotary_Off", [part])
        _setup_cyl_stock_along_axis(job, radius=15.0, length=40.0, axis="X")
        _install_stub_machine(job, "X")
        self.doc.recompute()
        op = self._build_op(job, axis="X")
        op.Proxy.execute(op)
        msgs = "\n".join(warnings)
        self.assertIn("centroid", msgs.lower())

    def test09_angular_extents_warn_when_outside_limits(self):
        """Angular extents past machine rotary limits emit a warning.

        ``App::PropertyAngle`` clamps to ±360, so we exercise the
        warning by giving the Job a Machine whose rotary axis is
        narrower than that — a typical 4th-axis indexer with ±90°
        travel — and letting the default StopAngle=360 exceed it.
        """
        warnings = self._capture_warnings()
        job, _ = self._build_job(axis="X")
        op = self._build_op(job, axis="X")
        # Override the default ±360 stub from _build_job with a narrower
        # ±90 rotary so the default StopAngle=360 exceeds the limit.
        _install_stub_machine(job, "X", min_limit=-90.0, max_limit=90.0)
        op.Proxy.execute(op)
        msgs = "\n".join(warnings)
        self.assertIn("rotary limits", msgs.lower())

    def test11_parallel_pattern_emits_xyza(self):
        """CutPattern=Parallel produces an XYZA G1 path on the cone fixture."""
        job, _ = self._build_job(axis="X")
        op = self._build_op(job, axis="X")
        op.CutPattern = "Parallel"
        self.doc.recompute()
        commands = op.Path.Commands
        self.assertTrue(
            len(commands) > 10,
            "expected many commands for Parallel pattern, got {}".format(len(commands)),
        )
        cut_g1 = [c for c in commands if c.Name == "G1" and "A" in c.Parameters]
        self.assertTrue(
            len(cut_g1) > 5,
            "expected XYZA cutting commands for Parallel, got {}".format(len(cut_g1)),
        )
        for c in cut_g1[:3]:
            self.assertIn("X", c.Parameters)
            self.assertIn("Y", c.Parameters)
            self.assertIn("Z", c.Parameters)
            self.assertIn("A", c.Parameters)

    def test10_centerline_feed_clamp_logs(self):
        """rotary_spiral.generate logs when the rotary feed is clamped."""
        import Path.Base.Generator.rotary_spiral as rotary_spiral

        # 12-step grid, all radii ~0.5 mm — small enough that the
        # effective rotary feed F * 360/(2π·r) blows past max_feed.
        n_x, n_t = 4, 8
        xs = [0.0, 1.0, 2.0, 3.0]
        thetas = [i * 2.0 * math.pi / (n_t - 1) for i in range(n_t)]
        radii = [[0.5 for _ in range(n_t)] for _ in range(n_x)]

        warnings = self._capture_warnings()
        rotary_spiral.generate(
            radii=radii,
            xs=xs,
            thetas=thetas,
            rotary_letter="A",
            rotary_axis="X",
            x_min=0.0,
            x_max=3.0,
            theta_start=0.0,
            theta_end=2.0 * math.pi,
            axial_stepover=0.5,
            angular_resolution=0.524,
            radial_stock_to_leave=0.0,
            cut_mode="Climb",
            safe_height=20.0,
            clearance_height=15.0,
            horiz_feed=2000.0,  # high enough to force clamp at small r
            vert_feed=200.0,
            horiz_rapid=5000.0,
            vert_rapid=5000.0,
            max_feed=500.0,  # tight cap
            cutter_z_floor=None,
        )
        msgs = "\n".join(warnings)
        self.assertIn("clamped", msgs.lower())

    def test12_rings_pattern_emits_xyza(self):
        """CutPattern='Rings' produces XYZA G1 commands end-to-end."""
        job, _ = self._build_job(axis="X")
        op = self._build_op(job, axis="X")
        op.CutPattern = "Rings"
        self.doc.recompute()
        commands = op.Path.Commands
        self.assertTrue(len(commands) > 10, "expected many commands, got {}".format(len(commands)))

        cut_g1 = [c for c in commands if c.Name == "G1" and "A" in c.Parameters]
        self.assertTrue(
            len(cut_g1) > 5,
            "expected XYZA cutting commands for Rings pattern, got {}".format(len(cut_g1)),
        )
        for c in cut_g1[:3]:
            self.assertIn("X", c.Parameters)
            self.assertIn("Y", c.Parameters)
            self.assertIn("Z", c.Parameters)

    def test13_feedmode_axial_only_emits_constant_feed(self):
        """AxialOnly: every cut F equals the tool-controller HorizFeed."""
        job, _ = self._build_job(axis="X")
        op = self._build_op(job, axis="X")
        op.ToolController.HorizFeed = 500.0
        op.FeedMode = "AxialOnly"
        op.MaxFeed = 1.0e6  # avoid the centerline clamp masking the test
        self.doc.recompute()
        # Filter to true rotary-cut moves: G1s whose A advanced from
        # the previous move's A (excludes the plunge, which holds A
        # constant and uses VertFeed instead of HorizFeed).
        feeds = []
        prev_a = None
        for c in op.Path.Commands:
            a = c.Parameters.get("A")
            if c.Name == "G1" and a is not None and "F" in c.Parameters:
                if prev_a is not None and abs(a - prev_a) > 1e-6:
                    feeds.append(c.Parameters["F"])
                prev_a = a
            elif a is not None:
                prev_a = a
        self.assertGreater(len(feeds), 5)
        # All cut feeds in AxialOnly should agree to within float noise.
        self.assertAlmostEqual(min(feeds), max(feeds), places=2)

    def test14_feedmode_surface_speed_varies_feed(self):
        """SurfaceSpeed: F scales with 1/r, so tapered radii give different F."""
        job, _ = self._build_job(axis="X")
        op = self._build_op(job, axis="X")
        op.ToolController.HorizFeed = 500.0
        op.FeedMode = "SurfaceSpeed"
        op.MaxFeed = 1.0e6  # high enough that clamping won't flatten variation
        self.doc.recompute()
        # Filter to true rotary-cut moves (see test13).
        feeds = []
        prev_a = None
        for c in op.Path.Commands:
            a = c.Parameters.get("A")
            if c.Name == "G1" and a is not None and "F" in c.Parameters:
                if prev_a is not None and abs(a - prev_a) > 1e-6:
                    feeds.append(c.Parameters["F"])
                prev_a = a
            elif a is not None:
                prev_a = a
        self.assertGreater(len(feeds), 5)
        # The cone tapers from r0=8 to r1=12, so SurfaceSpeed should
        # produce F values spanning at least a 1.2x ratio between
        # smallest-radius and largest-radius cuts.
        self.assertGreater(max(feeds) / max(min(feeds), 1e-6), 1.2)
