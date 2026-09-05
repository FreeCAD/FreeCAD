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

"""Regression coverage for the "legacy" (non-machine, non-Processor.py) post
scripts and the G98/G99 retract-mode gap.

Path.Op.Drilling only *annotates* each canned-cycle command with its retract
mode (RetractMode: "G98"/"G99") for cannedCycleTerminator's benefit. These
scripts never call cannedCycleTerminator and only ever learn the retract mode
from a literal G98/G99 command in the stream -- so a KeepToolDown=True
(G99) Drilling operation was silently treated as G98 by every one of them.
Fixed by running cannedCycleTerminator() over the path before their own
per-command scan, so a literal G98/G99 precedes each cycle group.

dynapath_legacy_post is exercised separately: unlike the others, its
export() never returns the gcode string for filename="-" (a separate,
pre-existing bug) -- it only writes to a real file, so its test writes to a
temp file and reads it back.
"""

import os
import tempfile
from importlib import reload

import FreeCAD
import Part
import Path
import Path.Main.Job as PathJob
import Path.Op.Drilling as PathDrilling
import Path.Tool.Controller as PathToolController
from Path.Tool.toolbit import ToolBit
import CAMTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TestLegacyPostRetractMode(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestLegacyPostRetractMode")
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

        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = toolController
        operation.Strategy = "Drilling"
        operation.Locations = [FreeCAD.Vector(2, 2, 0)]
        operation.setExpression("StartDepth", None)
        operation.StartDepth = 5.0
        operation.setExpression("FinalDepth", None)
        operation.FinalDepth = -10.0
        operation.PeckEnabled = True
        operation.PeckDepth = 2.0
        operation.KeepToolDown = True  # G99 -- the mode that was getting lost
        operation.Proxy.execute(operation)
        self.operation = operation

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _assertG99InOutput(self, module_name):
        module = __import__(f"Path.Post.scripts.{module_name}", fromlist=[module_name])
        reload(module)
        gcode = module.export([self.operation], "-", "")
        self.assertIsNotNone(gcode, f"{module_name}.export() returned None")
        self.assertIn(
            "G99",
            gcode,
            f"{module_name}: KeepToolDown=True (G99) never reached the output:\n{gcode}",
        )

    def test_linuxcnc_legacy(self):
        self._assertG99InOutput("linuxcnc_legacy_post")

    def test_grbl_legacy(self):
        self._assertG99InOutput("grbl_legacy_post")

    def test_marlin_legacy(self):
        self._assertG99InOutput("marlin_legacy_post")

    def test_estlcam_legacy(self):
        self._assertG99InOutput("estlcam_legacy_post")

    def test_rrf_legacy(self):
        self._assertG99InOutput("rrf_legacy_post")

    def test_fanuc_legacy(self):
        self._assertG99InOutput("fanuc_legacy_post")

    def test_uccnc_legacy(self):
        self._assertG99InOutput("uccnc_legacy_post")

    def test_dynapath_legacy(self):
        # dynapath_legacy_post.export() never returns the gcode string for
        # filename="-" like the others do (a separate, pre-existing bug) --
        # it only writes to a real file. Work around that here instead of
        # letting it block coverage of the fix we're actually testing.
        module = __import__(
            "Path.Post.scripts.dynapath_legacy_post", fromlist=["dynapath_legacy_post"]
        )
        reload(module)
        with tempfile.NamedTemporaryFile(suffix=".ncc", delete=False) as f:
            path = f.name
        try:
            module.export([self.operation], path, "")
            with open(path) as f:
                gcode = f.read()
            self.assertIn(
                "G99",
                gcode,
                f"dynapath_legacy_post: KeepToolDown=True (G99) never reached the output:\n{gcode}",
            )
        finally:
            os.unlink(path)
