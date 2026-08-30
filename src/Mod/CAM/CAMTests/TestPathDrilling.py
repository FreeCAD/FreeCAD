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

import FreeCAD
import Part
import Path
import Path.Main.Job as PathJob
import Path.Op.Drilling as PathDrilling
import Path.Tool.Controller as PathToolController
from Path.Tool.toolbit import ToolBit
import CAMTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TestPathDrilling(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathDrilling")
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

    def test00_peck_retract_defaults_to_safe_height(self):
        """PeckRetract (the G83/G81 'R' value) should default to SafeHeight, not StartDepth."""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController

        self.assertTrue(hasattr(operation, "PeckRetract"))
        self.assertRoughly(operation.SafeHeight.Value, operation.PeckRetract.Value)

    def test01_peck_retract_drives_canned_cycle_r(self):
        """The R parameter of the generated G83 peck cycle must come from PeckRetract,
        not from StartDepth (see FreeCAD/FreeCAD#32201)."""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController
        operation.Strategy = "Drilling"
        operation.Locations = [FreeCAD.Vector(10, 10, 0)]
        operation.setExpression("StartDepth", None)
        operation.StartDepth = 5.0
        operation.setExpression("FinalDepth", None)
        operation.FinalDepth = -10.0
        operation.PeckEnabled = True
        operation.PeckDepth = 2.0
        # Explicitly override the default (job-linked) retract height, the way a user
        # would via the "Peck Retract" field, mimicking a deep-hole peck cycle that
        # should not fully retract out of the material.
        operation.setExpression("PeckRetract", None)
        operation.PeckRetract = 1.0

        operation.Proxy.execute(operation)

        commands = {command.Name: command for command in operation.Path.Commands}
        self.assertIn("G83", commands)
        g83 = commands["G83"]

        self.assertRoughly(1.0, g83.Parameters["R"])
        self.assertNotEqual(operation.StartDepth.Value, g83.Parameters["R"])

    def test02_peck_retract_ignored_when_not_pecking(self):
        """PeckRetract only applies to peck cycles. A plain (non-peck) G81 cycle
        should retract to SafeHeight regardless of any stale PeckRetract value."""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController
        operation.Strategy = "Drilling"
        operation.Locations = [FreeCAD.Vector(10, 10, 0)]
        operation.setExpression("StartDepth", None)
        operation.StartDepth = 5.0
        operation.setExpression("FinalDepth", None)
        operation.FinalDepth = -10.0
        operation.PeckEnabled = False
        # A leftover/stale PeckRetract value (e.g. from a previous Peck session)
        # must not leak into a non-peck cycle.
        operation.setExpression("PeckRetract", None)
        operation.PeckRetract = 1.0

        operation.Proxy.execute(operation)

        commands = {command.Name: command for command in operation.Path.Commands}
        self.assertIn("G81", commands)
        g81 = commands["G81"]

        self.assertRoughly(operation.SafeHeight.Value, g81.Parameters["R"])
        self.assertNotEqual(1.0, g81.Parameters["R"])

    def test03_low_peck_retract_climbs_to_safe_before_next_hole(self):
        """In KeepToolDown (G99) mode with a PeckRetract below SafeHeight (e.g. a
        low peck retract deep inside a hole), the tool must not silently traverse to
        the next hole at the low R height. It should climb to SafeHeight first."""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController
        operation.Strategy = "Drilling"
        operation.Locations = [FreeCAD.Vector(2, 2, 0), FreeCAD.Vector(18, 18, 0)]
        operation.setExpression("StartDepth", None)
        operation.StartDepth = 5.0
        operation.setExpression("FinalDepth", None)
        operation.FinalDepth = -10.0
        operation.PeckEnabled = True
        operation.PeckDepth = 2.0
        operation.KeepToolDown = True
        operation.setExpression("PeckRetract", None)
        operation.PeckRetract = -9.0  # well below SafeHeight, deep in the hole

        operation.Proxy.execute(operation)

        commands = operation.Path.Commands
        g83_indices = [i for i, c in enumerate(commands) if c.Name == "G83"]
        self.assertEqual(2, len(g83_indices))

        between = commands[g83_indices[0] + 1 : g83_indices[1]]
        climbs = [
            c
            for c in between
            if c.Name == "G0"
            and "Z" in c.Parameters
            and Path.Geom.isRoughly(c.Parameters["Z"], operation.SafeHeight.Value)
        ]
        self.assertTrue(
            climbs, "expected an explicit climb to SafeHeight between the two peck cycles"
        )
