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
        operation.StartDepth = 11.0  # stock top
        operation.setExpression("FinalDepth", None)
        operation.FinalDepth = 0.0
        operation.PeckEnabled = True
        operation.PeckDepth = 2.0
        # Override the default (job-linked) retract, the way a user would via the
        # "Peck Retract" field: just clear of the surface, so the cycle's rapid down
        # to R stays in open air, but well below SafeHeight.
        operation.setExpression("PeckRetract", None)
        operation.PeckRetract = 12.0

        operation.Proxy.execute(operation)

        commands = {command.Name: command for command in operation.Path.Commands}
        self.assertIn("G83", commands)
        g83 = commands["G83"]

        self.assertRoughly(12.0, g83.Parameters["R"])
        self.assertNotEqual(operation.StartDepth.Value, g83.Parameters["R"])

    def test02_peck_retract_ignored_when_not_pecking(self):
        """PeckRetract only applies to peck cycles. A plain (non-peck) G81 cycle
        should retract to SafeHeight regardless of any stale PeckRetract value."""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController
        operation.Strategy = "Drilling"
        operation.Locations = [FreeCAD.Vector(10, 10, 0)]
        operation.setExpression("StartDepth", None)
        operation.StartDepth = 11.0  # stock top
        operation.setExpression("FinalDepth", None)
        operation.FinalDepth = 0.0
        operation.PeckEnabled = False
        # A leftover/stale PeckRetract value (e.g. from a previous Peck session)
        # must not leak into a non-peck cycle.
        operation.setExpression("PeckRetract", None)
        operation.PeckRetract = 12.0

        operation.Proxy.execute(operation)

        commands = {command.Name: command for command in operation.Path.Commands}
        self.assertIn("G81", commands)
        g81 = commands["G81"]

        self.assertRoughly(operation.SafeHeight.Value, g81.Parameters["R"])
        self.assertNotEqual(12.0, g81.Parameters["R"])

    def test03_low_peck_retract_climbs_to_safe_before_next_hole(self):
        """In KeepToolDown (G99) mode with a PeckRetract below SafeHeight (e.g. a
        low peck retract deep inside a hole), the tool must not silently traverse to
        the next hole at the low R height. It should climb to SafeHeight first."""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController
        operation.Strategy = "Drilling"
        operation.Locations = [FreeCAD.Vector(2, 2, 0), FreeCAD.Vector(18, 18, 0)]
        operation.setExpression("StartDepth", None)
        operation.StartDepth = 11.0  # stock top
        operation.setExpression("FinalDepth", None)
        operation.FinalDepth = 0.0
        operation.PeckEnabled = True
        operation.PeckDepth = 2.0
        operation.KeepToolDown = True
        operation.setExpression("PeckRetract", None)
        operation.PeckRetract = 12.0  # clear of the surface, but below SafeHeight

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

    def _twoHolePeckOp(self, peck_retract, strategy=None):
        """A two-hole G99 peck op at machine-runnable depths, optionally on a
        geometry-aware collision strategy."""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController
        operation.Strategy = "Drilling"
        operation.Locations = [FreeCAD.Vector(2, 2, 0), FreeCAD.Vector(18, 18, 0)]
        operation.setExpression("StartDepth", None)
        operation.StartDepth = 11.0  # stock top
        operation.setExpression("FinalDepth", None)
        operation.FinalDepth = 0.0
        operation.PeckEnabled = True
        operation.PeckDepth = 2.0
        operation.KeepToolDown = True
        operation.setExpression("PeckRetract", None)
        operation.PeckRetract = peck_retract
        if strategy:
            operation.CollisionAvoidanceStrategy = strategy
            # CollisionClearance is expression-bound to OpToolDiameter and would be
            # re-evaluated by execute(); pin it so the margin is explicit.
            operation.setExpression("CollisionClearance", None)
            operation.CollisionClearance = 1.0
        operation.Proxy.execute(operation)
        return operation

    def _betweenCycles(self, operation):
        """The commands emitted between the two canned cycles."""
        commands = operation.Path.Commands
        cycles = [i for i, c in enumerate(commands) if c.Name == "G83"]
        self.assertEqual(2, len(cycles))
        return commands[cycles[0] + 1 : cycles[1]]

    def test04_geometry_strategy_keeps_tool_down_between_holes(self):
        """With a strategy that actually checks geometry, a G99 retract that clears
        the stock needs no climb -- the modal cycle carries the tool across at R.
        That is the whole point of KeepToolDown."""
        operation = self._twoHolePeckOp(12.0, strategy="Line of Sight")

        between = self._betweenCycles(operation)
        climbs = [c for c in between if c.Name == "G0" and "Z" in c.Parameters]
        self.assertFalse(
            climbs,
            f"a clear traverse at R should emit no linking moves, got: {climbs}",
        )

    def test05_geometry_strategy_links_when_traverse_blocked(self):
        """Same strategy, but a retract down inside the stock: the traverse to the
        next hole is not clear, so explicit linking moves must be emitted rather
        than letting the modal cycle drag the tool through material."""
        operation = self._twoHolePeckOp(5.0, strategy="Line of Sight")

        between = self._betweenCycles(operation)
        climbs = [
            c for c in between if c.Name == "G0" and "Z" in c.Parameters and c.Parameters["Z"] > 5.0
        ]
        self.assertTrue(climbs, "expected linking moves when the traverse at R is blocked by stock")

    def test06_extra_offset_migration_renames_drill_tip(self):
        """An old document's ExtraOffset selection must survive the enum rename.
        Enumeration::setEnums keeps the stored string and setValue() silently falls
        back to index 0 when it is gone, so without the migration "Drill Tip" would
        come back as "None" and the hole would be drilled short."""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController

        for old, expected in (
            ("Drill Tip", "Tool Tip"),
            ("2x Drill Tip", "2x Tool Tip"),
            ("None", "None"),
        ):
            # Rebuild the pre-migration enum list and selection, as a restore would.
            operation.ExtraOffset = ["None", "Drill Tip", "2x Drill Tip"]
            operation.ExtraOffset = old

            operation.Proxy.opOnDocumentRestored(operation)

            self.assertEqual(expected, operation.ExtraOffset)

    def test07_retract_height_migrates_to_peck_retract(self):
        """Documents saved while the property was named RetractHeight must come back
        as PeckRetract, keeping their value."""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController
        operation.setExpression("PeckRetract", None)
        operation.PeckRetract = 12.0
        operation.renameProperty("PeckRetract", "RetractHeight")
        self.assertFalse(hasattr(operation, "PeckRetract"))

        operation.Proxy.opOnDocumentRestored(operation)

        self.assertTrue(hasattr(operation, "PeckRetract"))
        self.assertFalse(hasattr(operation, "RetractHeight"))
        self.assertRoughly(12.0, operation.PeckRetract.Value)
