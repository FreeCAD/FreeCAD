# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 FreeCAD Project Association                        *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENSE text file.                                 *
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

    def test00_retract_height_defaults_to_safe_height(self):
        """RetractHeight (the G83/G81 'R' value) should default to SafeHeight, not StartDepth."""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController

        self.assertTrue(hasattr(operation, "RetractHeight"))
        self.assertRoughly(operation.SafeHeight.Value, operation.RetractHeight.Value)

    def test01_retract_height_drives_canned_cycle_r(self):
        """The R parameter of the generated G83 peck cycle must come from RetractHeight,
        not from StartDepth (see FreeCAD/FreeCAD#32201)."""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController
        operation.Strategy = "Drilling"
        operation.Locations = [FreeCAD.Vector(10, 10, 0)]
        operation.StartDepth = 5.0
        operation.FinalDepth = -10.0
        operation.PeckEnabled = True
        operation.PeckDepth = 2.0
        # Explicitly override the default (job-linked) retract height, the way a user
        # would via the "Peck Retract" field, mimicking a deep-hole peck cycle that
        # should not fully retract out of the material.
        operation.setExpression("RetractHeight", None)
        operation.RetractHeight = 1.0

        operation.Proxy.execute(operation)

        commands = {command.Name: command for command in operation.Path.Commands}
        self.assertIn("G83", commands)
        g83 = commands["G83"]

        self.assertRoughly(1.0, g83.Parameters["R"])
        self.assertNotEqual(operation.StartDepth.Value, g83.Parameters["R"])
