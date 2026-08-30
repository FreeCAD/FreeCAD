# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
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

"""Integration tests for the Drilling operation's PeckRetract/linking fix
(FreeCAD/FreeCAD#32201) through real post-processors.

TestPathDrilling.py checks the Path.Command stream Drilling.py produces.
This file checks what actually reaches the machine after a real post-processor
has had a chance to reinterpret that stream two different ways:

  * LinuxCNC: canned cycles (G81/G82/G83/G73) pass straight through.
  * Grbl, with translate_drill_cycles=True: canned cycles never appear at all --
    DrillCycleExpander rewrites them into plain G0/G1 moves before the
    per-postprocessor conversion ever runs.

Both must honor PeckRetract (not StartDepth) for the R/retract level, and
both must preserve the explicit SafeHeight climb inserted between holes when
PeckRetract sits below SafeHeight.
"""

import re

import FreeCAD
import Part
import Path
import Path.Main.Job as PathJob
import Path.Op.Drilling as PathDrilling
import Path.Tool.Controller as PathToolController
from Path.Tool.toolbit import ToolBit
from Path.Post.Processor import PostProcessorFactory
from Machine.models.machine import Machine, Toolhead, ToolheadType
import CAMTests.PathTestUtils as PathTestUtils
import CAMTests.PostTestMocks as PostTestMocks

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

PECK_RETRACT = -9.0  # well below SafeHeight -- exercises the climb-first fix
Z_RE = re.compile(r"Z(-?\d+(?:\.\d+)?)")
R_RE = re.compile(r"R(-?\d+(?:\.\d+)?)")


class TestDrillingPostIntegration(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestDrillingPostIntegration")
        base = self.doc.addObject("Part::Feature", "Base")
        base.Shape = Part.makeBox(20, 20, 10)
        self.job = PathJob.Create("Job", [base], None)

        tool = ToolBit.from_shape_id("drill.fcstd").attach_to_doc(doc=self.doc)
        toolController = PathToolController.Create("DrillTool", tool, 1)
        toolController.HorizFeed = 100
        toolController.VertFeed = 100
        toolController.HorizRapid = 200
        toolController.VertRapid = 200
        self.job.Tools.Group = [toolController]
        self.toolController = toolController

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _make_deep_peck_operation(self):
        """Two holes, KeepToolDown on, PeckRetract well below SafeHeight --
        the scenario the inter-hole 'climb to SafeHeight first' fix protects."""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController
        operation.Strategy = "Drilling"
        operation.Locations = [FreeCAD.Vector(2, 2, 0), FreeCAD.Vector(18, 18, 0)]
        # StartDepth/FinalDepth are expression-bound to Op*Depth (derived from the
        # stock geometry), same as PeckRetract -- clear the expression first or a
        # plain assignment gets silently overwritten on the next recompute/execute.
        operation.setExpression("StartDepth", None)
        operation.StartDepth = 5.0
        operation.setExpression("FinalDepth", None)
        operation.FinalDepth = -10.0
        operation.PeckEnabled = True
        operation.PeckDepth = 2.0
        operation.KeepToolDown = True
        operation.setExpression("PeckRetract", None)
        operation.PeckRetract = PECK_RETRACT

        operation.Proxy.execute(operation)
        return operation

    def _export(self, operation, postname, translate_drill_cycles):
        """Run `operation` through a real post-processor and return the gcode text."""
        mockjob = PostTestMocks.MockJob()
        mockjob.Tools.Group = [self.toolController]
        mockjob.Operations.Group = [operation]

        post = PostProcessorFactory.get_post_processor(mockjob, postname)
        post._machine = Machine.create_3axis_config()
        post._machine.name = f"Test {postname}"
        # Must be set before apply_configuration_bundle(): it snapshots
        # machine.processing.* into post.values and only runs once per post.
        post._machine.processing.translate_drill_cycles = translate_drill_cycles
        post.apply_configuration_bundle()
        post._machine.toolheads = [
            Toolhead(
                name="Default Toolhead",
                toolhead_type=ToolheadType.ROTARY,
                min_rpm=0,
                max_rpm=24000,
                max_power_kw=1.0,
            )
        ]

        sections = post.export2()
        self.assertIsNotNone(sections, "export2() returned None")
        return sections[0][1]

    # ------------------------------------------------------------------
    # LinuxCNC: native canned cycles (G81/G82/G83/G73 pass through as-is)
    # ------------------------------------------------------------------

    def test_linuxcnc_g83_r_matches_peck_retract(self):
        operation = self._make_deep_peck_operation()
        gcode = self._export(operation, "linuxcnc", translate_drill_cycles=False)

        g83_lines = [line for line in gcode.splitlines() if line.strip().startswith("G83")]
        self.assertTrue(g83_lines, f"expected G83 cycles in gcode:\n{gcode}")
        for line in g83_lines:
            m = R_RE.search(line)
            self.assertIsNotNone(m, f"G83 line missing R parameter: {line}")
            self.assertAlmostEqual(
                PECK_RETRACT,
                float(m.group(1)),
                places=2,
                msg=f"G83 R should be PeckRetract, not StartDepth: {line}",
            )

    def test_linuxcnc_climbs_to_safe_height_between_holes(self):
        operation = self._make_deep_peck_operation()
        gcode = self._export(operation, "linuxcnc", translate_drill_cycles=False)
        lines = gcode.splitlines()

        g83_idx = [i for i, line in enumerate(lines) if line.strip().startswith("G83")]
        self.assertEqual(2, len(g83_idx), f"expected 2 peck cycles in gcode:\n{gcode}")

        between = lines[g83_idx[0] + 1 : g83_idx[1]]
        safe_height = operation.SafeHeight.Value
        climbs = [
            line
            for line in between
            if line.strip().startswith("G0")
            and Z_RE.search(line)
            and abs(float(Z_RE.search(line).group(1)) - safe_height) < 1e-2
        ]
        self.assertTrue(
            climbs,
            f"expected an explicit G0 climb to SafeHeight ({safe_height}) between "
            f"the two peck cycles, found none in:\n{gcode}",
        )

    # ------------------------------------------------------------------
    # Grbl w/ translate_drill_cycles=True: no canned cycles at all --
    # DrillCycleExpander rewrites everything into plain G0/G1 moves.
    # ------------------------------------------------------------------

    def test_grbl_expansion_has_no_canned_cycles(self):
        operation = self._make_deep_peck_operation()
        gcode = self._export(operation, "grbl", translate_drill_cycles=True)

        for cycle in ("G81", "G82", "G83", "G73", "G98", "G99"):
            self.assertNotIn(
                f"{cycle}\n".strip(),
                [w for line in gcode.splitlines() for w in line.split()],
                f"{cycle} should not appear once drill cycles are expanded:\n{gcode}",
            )

    def test_grbl_expansion_retracts_to_peck_retract(self):
        operation = self._make_deep_peck_operation()
        gcode = self._export(operation, "grbl", translate_drill_cycles=True)

        z_values = [float(m.group(1)) for m in Z_RE.finditer(gcode)]
        matches = [z for z in z_values if abs(z - PECK_RETRACT) < 1e-2]
        self.assertTrue(
            matches,
            f"expected expanded moves to retract to PeckRetract "
            f"({PECK_RETRACT}), not StartDepth, in:\n{gcode}",
        )

    def test_grbl_expansion_still_climbs_to_safe_height_between_holes(self):
        operation = self._make_deep_peck_operation()
        gcode = self._export(operation, "grbl", translate_drill_cycles=True)

        safe_height = operation.SafeHeight.Value
        z_values = [float(m.group(1)) for m in Z_RE.finditer(gcode)]
        self.assertTrue(
            any(abs(z - safe_height) < 1e-2 for z in z_values),
            f"expected the explicit climb to SafeHeight ({safe_height}) to survive "
            f"drill-cycle expansion in:\n{gcode}",
        )
