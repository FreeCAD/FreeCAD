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

"""Multi-post regression coverage for rotary surfacing G-code.

Verifies that a Rotary Surface operation's XYZA motion survives the
post-processor pipeline through the LinuxCNC and Grbl posts in both
compound (single ``G1 X Y Z A``) and split-move forms, with continuous
unwound rotary output preserved end-to-end.
"""

import re
import unittest

import FreeCAD
import Part

import Path
import Path.Main.Job as PathJob
import Path.Main.Stock as PathStock
from CAMTests.PathTestUtils import PathTestBase
from Path.Post.Processor import PostProcessorFactory
from Machine.models.machine import Machine, RotaryAxis, Toolhead, ToolheadType

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


# Match an axis word like "X12.345", "A-720.000". Captures the value.
_AXIS_RE = re.compile(r"([XYZABC])(-?\d+(?:\.\d+)?)")


def _axis_values(line, letter):
    """Return all values for axis letter found on a single g-code line."""
    return [float(v) for ax, v in _AXIS_RE.findall(line) if ax == letter]


def _parse_motion_lines(gcode):
    """Return (cmd, params_dict) tuples for G0/G1 motion lines only.

    Comment-only lines, modal commands, M-codes, and lines that do not
    start with G0/G1 are skipped — this isolates the path output from
    preamble/postamble noise.
    """
    out = []
    for raw in gcode.splitlines():
        line = raw.strip()
        if not line or line.startswith("(") or line.startswith(";"):
            continue
        # Strip leading line numbers like "N100 G1 ...".
        line = re.sub(r"^N\d+\s*", "", line)
        m = re.match(r"^(G0?[01])\b(.*)$", line)
        if not m:
            continue
        cmd = m.group(1)
        rest = m.group(2)
        params = {}
        for ax, val in _AXIS_RE.findall(rest):
            params[ax] = float(val)
        out.append((cmd, params, line))
    return out


def _split_compound_path(path):
    """Return a new Path with each XYZA-compound G1 split into A-then-XYZ.

    Compound moves with both linear (X/Y/Z) and rotary (A/B/C) tokens are
    rewritten as a rotary-only G0 followed by a linear-only G1. Pure-linear
    or pure-rotary moves are passed through unchanged. Feed (F) is kept on
    whichever resulting move retains it last so the controller still sees a
    feed value before the cut.
    """
    new_commands = []
    rotary_letters = ("A", "B", "C")
    for cmd in path.Commands:
        params = dict(cmd.Parameters)
        rot = {k: v for k, v in params.items() if k in rotary_letters}
        lin = {k: v for k, v in params.items() if k in ("X", "Y", "Z")}
        if cmd.Name in ("G1", "G01") and rot and lin:
            feed = params.get("F")
            rot_params = dict(rot)
            new_commands.append(Path.Command("G0", rot_params))
            lin_params = dict(lin)
            if feed is not None:
                lin_params["F"] = feed
            new_commands.append(Path.Command("G1", lin_params))
        else:
            new_commands.append(Path.Command(cmd.Name, params))
    return Path.Path(new_commands)


def _make_part(doc, length=40.0, r0=8.0, r1=12.0):
    cone = Part.makeCone(r0, r1, length)
    cone.Placement = FreeCAD.Placement(
        FreeCAD.Vector(-length / 2.0, 0, 0),
        FreeCAD.Rotation(FreeCAD.Vector(0, 1, 0), 90.0),
    )
    obj = doc.addObject("Part::Feature", "RotaryPart")
    obj.Shape = cone
    return obj


def _setup_cyl_stock(job, radius, length):
    placement = FreeCAD.Placement(
        FreeCAD.Vector(-length / 2.0, 0, 0),
        FreeCAD.Rotation(FreeCAD.Vector(0, 1, 0), 90.0),
    )
    new_stock = PathStock.CreateCylinder(job, radius=radius, height=length, placement=placement)
    if job.Stock and job.Stock is not new_stock:
        try:
            FreeCAD.ActiveDocument.removeObject(job.Stock.Name)
        except (AttributeError, RuntimeError):
            # Stock may already be removed; ignore cleanup errors
            pass
    job.Stock = new_stock
    return new_stock


@unittest.skipUnless(HAVE_OCL, "OpenCamLib not available")
class TestPathRotaryPostRegression(PathTestBase):
    """Multi-post regression for rotary surfacing output (Issue #3)."""

    @classmethod
    def setUpClass(cls):
        cls.doc = FreeCAD.newDocument("TestRotaryPostRegression")
        cls.length = 40.0
        cls.radius = 12.0
        part = _make_part(cls.doc, length=cls.length, r0=8.0, r1=cls.radius)
        cls.job = PathJob.Create("Job_RotaryPost", [part])
        _setup_cyl_stock(cls.job, radius=cls.radius + 0.5, length=cls.length)

        # Rotary Surface requires a Machine with at least one rotary
        # axis. Pin a stub A-around-X axis on the Job so the resolver
        # picks it up.
        class _StubMachine:
            rotary_axes = {
                "A": RotaryAxis(
                    name="A",
                    rotation_vector=FreeCAD.Vector(1, 0, 0),
                    min_limit=-360.0,
                    max_limit=360.0,
                )
            }

        cls.job.Proxy.getMachine = lambda: _StubMachine()
        cls.doc.recompute()

        cls.op = PathRotarySurface.Create("RotaryOp", parentJob=cls.job)
        cls.op.StartX = -cls.length / 2.0
        cls.op.StopX = cls.length / 2.0
        cls.op.StartAngle = 0.0
        cls.op.StopAngle = 360.0
        cls.op.StepOver = 4.0
        cls.op.AngularResolution = 15.0
        cls.op.setExpression("StepDown", None)
        cls.op.StepDown = 0.0
        cls.op.RadialStockToLeave = 0.0
        cls.op.MaxFeed = 5000.0
        cls.op.LinearDeflection = 0.2
        cls.op.AngularDeflection = 0.524
        cls.op.ToolController.Tool.Diameter = 3.0
        cls.doc.recompute()

        # Snapshot the compound path; tests that need the split form rebuild
        # it from this snapshot so each test starts from the same baseline.
        cls.compound_path = Path.Path(list(cls.op.Path.Commands))

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)

    def setUp(self):
        self.maxDiff = None
        # Restore the compound path in case a previous test mutated op.Path.
        self.op.Path = Path.Path(list(self.compound_path.Commands))

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _make_post(self, post_name):
        """Build a post-processor with a permissive 4-axis machine config.

        Mirrors the setUp pattern in TestLinuxCNCPost: instantiate the post,
        pin a fresh Machine onto it, and silence comments/header so the
        regex-based assertions below do not have to skip preamble noise.
        """
        post = PostProcessorFactory.get_post_processor(self.job, post_name)
        post.reinitialize()
        machine = Machine.create_3axis_config()
        machine.name = f"Test {post_name} Rotary Machine"
        machine.toolheads = [
            Toolhead(
                name="Default Toolhead",
                toolhead_type=ToolheadType.ROTARY,
                min_rpm=0,
                max_rpm=24000,
                max_power_kw=1.0,
            )
        ]
        machine.output.comments.enabled = False
        machine.output.output_header = False
        # Keep duplicate axis values in output so every motion line
        # carries the full XYZA word set; otherwise the post strips
        # unchanged axes via modal-state suppression and the round-trip
        # comparison loses its 1:1 mapping.
        machine.output.duplicates.parameters = True
        post._machine = machine
        return post

    def _export_gcode(self, post_name):
        post = self._make_post(post_name)
        sections = post.export2()
        self.assertIsNotNone(sections, f"{post_name} export2 returned None")
        return "\n".join(g for _, g in sections)

    def _assert_unwound(self, a_values):
        """Assert that successive A values never wrap (delta < 360°)."""
        self.assertGreater(len(a_values), 5, "expected several rotary samples")
        deltas = [a_values[i + 1] - a_values[i] for i in range(len(a_values) - 1)]
        for d in deltas:
            self.assertLess(
                abs(d),
                360.0,
                f"A delta {d} indicates a 0/360 wrap rather than unwound output",
            )
        self.assertGreater(
            max(a_values) - min(a_values),
            360.0,
            "expected at least one full revolution of unwound A in posted G-code",
        )

    # ------------------------------------------------------------------
    # Compound-mode tests (default Op output)
    # ------------------------------------------------------------------

    def test00_linuxcnc_compound_emits_xyza(self):
        """LinuxCNC must not drop A-axis words from compound moves."""
        gcode = self._export_gcode("linuxcnc")
        motion = _parse_motion_lines(gcode)
        compound = [(c, p, line) for c, p, line in motion if "A" in p and "X" in p]
        self.assertGreater(
            len(compound),
            5,
            "expected several compound G1 X.. Y.. Z.. A.. lines in LinuxCNC output",
        )

    def test01_linuxcnc_compound_unwound_a(self):
        """Posted A values within a pass remain unwound (no 0/360 wrap)."""
        gcode = self._export_gcode("linuxcnc")
        a_values = []
        for cmd, params, _ in _parse_motion_lines(gcode):
            if "A" in params:
                a_values.append(params["A"])
        self._assert_unwound(a_values)

    def test02_grbl_compound_emits_xyza(self):
        """Grbl preserves A on compound XYZA moves consistent with its caps."""
        gcode = self._export_gcode("grbl")
        motion = _parse_motion_lines(gcode)
        compound = [(c, p, line) for c, p, line in motion if "A" in p and "X" in p]
        self.assertGreater(
            len(compound),
            5,
            "expected several compound G1 X.. Y.. Z.. A.. lines in Grbl output",
        )

    def test03_grbl_compound_unwound_a(self):
        gcode = self._export_gcode("grbl")
        a_values = [p["A"] for _, p, _ in _parse_motion_lines(gcode) if "A" in p]
        self._assert_unwound(a_values)

    def test04_compound_round_trip_first_command(self):
        """Coordinate values on the first cutting move round-trip per post.

        Matches by A value across op.Path and posted G-code so it is
        robust to the post inserting/dropping non-cutting moves: pick
        the first G1 in op.Path with a non-zero A, look up the posted
        line carrying that same A, and compare X/Y/Z within format
        precision.
        """
        ref = None
        for cmd in self.op.Path.Commands:
            p = cmd.Parameters
            if cmd.Name == "G1" and all(ax in p for ax in ("X", "Y", "Z", "A")):
                if abs(p["A"]) > 1e-3:
                    ref = (cmd.Name, p)
                    break
        self.assertIsNotNone(ref, "no compound G1 XYZA cut move in op path — fixture broken")

        target_a = ref[1]["A"]
        for post_name in ("linuxcnc", "grbl"):
            gcode = self._export_gcode(post_name)
            motion = _parse_motion_lines(gcode)
            match = None
            for _, p, _ in motion:
                if all(ax in p for ax in ("X", "Y", "Z", "A")) and abs(p["A"] - target_a) < 1e-2:
                    match = p
                    break
            self.assertIsNotNone(
                match,
                f"{post_name}: no posted line matching A={target_a:.4f}",
            )
            for ax in ("X", "Y", "Z", "A"):
                self.assertAlmostEqual(
                    match[ax],
                    ref[1][ax],
                    places=2,
                    msg=f"{post_name}: {ax} differs between op and posted G-code",
                )

    # ------------------------------------------------------------------
    # Split-mode tests (compound_moves=False simulation)
    # ------------------------------------------------------------------
    #
    # The Machine model already exposes ``compound_moves`` (default True).
    # Wiring that flag to a Processor expansion stage is forward work; for
    # regression today we exercise the split-move shape by pre-splitting
    # compound XYZA into rotary-only G0 + linear-only G1 before posting and
    # confirming each post echoes both forms cleanly.

    def test05_linuxcnc_split_emits_separate_axis_lines(self):
        """LinuxCNC posts split moves without merging or dropping the A line."""
        self.op.Path = _split_compound_path(self.compound_path)
        try:
            gcode = self._export_gcode("linuxcnc")
        finally:
            self.op.Path = Path.Path(list(self.compound_path.Commands))

        motion = _parse_motion_lines(gcode)
        a_only = [
            p for _, p, _ in motion if "A" in p and not any(ax in p for ax in ("X", "Y", "Z"))
        ]
        xyz_only = [
            p for _, p, _ in motion if "X" in p and not any(ax in p for ax in ("A", "B", "C"))
        ]
        self.assertGreater(
            len(a_only),
            5,
            "expected several A-only motion lines after splitting",
        )
        self.assertGreater(
            len(xyz_only),
            5,
            "expected several XYZ-only motion lines after splitting",
        )

    def test06_grbl_split_emits_separate_axis_lines(self):
        self.op.Path = _split_compound_path(self.compound_path)
        try:
            gcode = self._export_gcode("grbl")
        finally:
            self.op.Path = Path.Path(list(self.compound_path.Commands))

        motion = _parse_motion_lines(gcode)
        a_only = [
            p for _, p, _ in motion if "A" in p and not any(ax in p for ax in ("X", "Y", "Z"))
        ]
        xyz_only = [
            p for _, p, _ in motion if "X" in p and not any(ax in p for ax in ("A", "B", "C"))
        ]
        self.assertGreater(len(a_only), 5)
        self.assertGreater(len(xyz_only), 5)

    def test07_split_unwound_a_preserved(self):
        """Even when emitted on dedicated lines, A is still unwound."""
        self.op.Path = _split_compound_path(self.compound_path)
        try:
            gcode = self._export_gcode("linuxcnc")
        finally:
            self.op.Path = Path.Path(list(self.compound_path.Commands))

        a_values = [p["A"] for _, p, _ in _parse_motion_lines(gcode) if "A" in p]
        self._assert_unwound(a_values)

    def test08_split_round_trip_axis_pairs(self):
        """Split A move + paired XYZ move round-trip the original compound."""
        # Build a parallel reference list of (A, X, Y, Z) tuples from the
        # source compound path; the split path must produce the same tuples
        # in the posted output, just spread across two lines.
        ref_tuples = []
        for cmd in self.compound_path.Commands:
            p = cmd.Parameters
            if cmd.Name == "G1" and all(ax in p for ax in ("X", "Y", "Z", "A")):
                ref_tuples.append((p["A"], p["X"], p["Y"], p["Z"]))

        self.assertGreater(len(ref_tuples), 5, "need several compound moves to round-trip")

        self.op.Path = _split_compound_path(self.compound_path)
        try:
            gcode = self._export_gcode("linuxcnc")
        finally:
            self.op.Path = Path.Path(list(self.compound_path.Commands))

        # Walk the posted motion lines and pair each A-only line with the
        # next XYZ line (they were emitted as G0 A then G1 X Y Z by the
        # splitter). Compare the first few pairs to the reference.
        motion = _parse_motion_lines(gcode)
        pairs = []
        i = 0
        while i < len(motion) - 1:
            _, p1, _ = motion[i]
            _, p2, _ = motion[i + 1]
            is_a_only = "A" in p1 and not any(ax in p1 for ax in ("X", "Y", "Z"))
            is_xyz = all(ax in p2 for ax in ("X", "Y", "Z")) and "A" not in p2
            if is_a_only and is_xyz:
                pairs.append((p1["A"], p2["X"], p2["Y"], p2["Z"]))
                i += 2
            else:
                i += 1

        self.assertGreaterEqual(
            len(pairs),
            5,
            "expected several A-then-XYZ pairs in posted G-code",
        )
        for ref, got in zip(ref_tuples[: len(pairs)], pairs):
            for label, r, g in zip(("A", "X", "Y", "Z"), ref, got):
                self.assertAlmostEqual(
                    r,
                    g,
                    places=2,
                    msg=f"split mode: {label} differs between op ({r}) and posted ({g})",
                )
