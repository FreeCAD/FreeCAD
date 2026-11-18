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
from CAMTests import PathTestUtils

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

    def createDrillingOp(self):
        """Helper method to create drilling op"""
        operation = PathDrilling.Create("Drilling", parentJob=self.job)
        operation.ToolController = self.toolController

        operation.Strategy = "Drilling"
        operation.Locations = [
            FreeCAD.Vector(5, 5, 0),
            FreeCAD.Vector(10, 10, 0),
            FreeCAD.Vector(20, 20, 0),
        ]

        operation.clearExpression("ClearanceHeight")
        operation.ClearanceHeight = 15.0
        operation.clearExpression("SafeHeight")
        operation.SafeHeight = 13.0
        operation.clearExpression("StartDepth")
        operation.StartDepth = 11.0
        operation.clearExpression("FinalDepth")
        operation.FinalDepth = 0.0

        operation.PeckEnabled = False
        operation.clearExpression("PeckRetract")
        operation.PeckRetract = 11.0
        operation.clearExpression("PeckDepth")
        operation.PeckDepth = 2.0

        operation.KeepToolDown = False

        operation.CollisionAvoidanceStrategy = "Retract Height"
        operation.clearExpression("CollisionClearance")
        operation.CollisionClearance = 1.0

        return operation

    def getSimpleGcodeFromPath(self, path):
        """Returns string (gcode) without decimals and annotations to simplify comparing result"""
        lines = []
        for cmd in path.Commands:
            line = cmd.toGCode()
            if line in ("(Drilling)", "(Begin Drilling)"):
                continue
            line = line.replace(".000000", "")
            lst = line.split(";")
            lines.append(lst[0])
            if len(lst) > 1:
                annotation = lst[1]

        return "\n".join(lines), annotation

    def test00_extra_offset_migration_renames_drill_tip(self):
        """An old document's ExtraOffset selection must survive the enum rename.
        Enumeration::setEnums keeps the stored string and setValue() silently falls
        back to index 0 when it is gone, so without the migration "Drill Tip" would
        come back as "None" and the hole would be drilled short."""
        operation = self.createDrillingOp()

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

    def test01_retract_height_migrates_to_peck_retract(self):
        """Documents saved while the property was named RetractHeight must come back
        as PeckRetract, keeping their value."""
        operation = self.createDrillingOp()
        operation.renameProperty("PeckRetract", "RetractHeight")
        self.assertFalse(hasattr(operation, "PeckRetract"))

        operation.Proxy.opOnDocumentRestored(operation)

        self.assertTrue(hasattr(operation, "PeckRetract"))
        self.assertFalse(hasattr(operation, "RetractHeight"))
        self.assertRoughly(11.0, operation.PeckRetract.Value)

    def test10_basic_drilling_op(self):
        """Test basic Drilling operation creation"""
        operation = self.createDrillingOp()
        operation.Proxy.execute(operation)

        expected = """G0 F200 Z15
G0 F200 X5 Y5
G0 F200 Z13
G81 F100 R13 X5 Y5 Z0
G81 F100 R13 X10 Y10 Z0
G81 F100 R13 X20 Y20 Z0
G0 Z15"""

        result, annotation = self.getSimpleGcodeFromPath(operation.Path)
        self.assertEqual(result, expected)
        self.assertIn("G98", annotation)

    def test11_collision_clearance(self):
        """Test Drilling operation
        - rapids at ClearanceHeight"""
        operation = self.createDrillingOp()
        operation.CollisionAvoidanceStrategy = "Clearance Height"
        operation.Proxy.execute(operation)

        expected = """G0 F200 Z15
G0 F200 X5 Y5
G0 F200 Z13
G81 F100 R13 X5 Y5 Z0
G0 F200 X5 Y5 Z15
G0 F200 X10 Y10 Z15
G0 F200 X10 Y10 Z13
G81 F100 R13 X10 Y10 Z0
G0 F200 X10 Y10 Z15
G0 F200 X20 Y20 Z15
G0 F200 X20 Y20 Z13
G81 F100 R13 X20 Y20 Z0
G0 Z15"""

        result, annotation = self.getSimpleGcodeFromPath(operation.Path)
        self.assertEqual(result, expected)
        self.assertIn("G98", annotation)

    def test12_collision_clearance_peck(self):
        """Test Drilling operation
        - peck
        - rapids at ClearanceHeight"""
        operation = self.createDrillingOp()
        operation.CollisionAvoidanceStrategy = "Clearance Height"
        operation.PeckEnabled = True
        operation.Proxy.execute(operation)

        expected = """G0 F200 Z15
G0 F200 X5 Y5
G0 F200 Z13
G83 F100 Q2 R11 X5 Y5 Z0
G0 F200 X5 Y5 Z15
G0 F200 X10 Y10 Z15
G0 F200 X10 Y10 Z13
G83 F100 Q2 R11 X10 Y10 Z0
G0 F200 X10 Y10 Z15
G0 F200 X20 Y20 Z15
G0 F200 X20 Y20 Z13
G83 F100 Q2 R11 X20 Y20 Z0
G0 Z15"""

        result, annotation = self.getSimpleGcodeFromPath(operation.Path)
        self.assertEqual(result, expected)
        self.assertIn("G98", annotation)

    def test13_collision_retract_peck(self):
        """Test Drilling operation
        - peck
        - rapids at SafeHeight"""
        operation = self.createDrillingOp()
        operation.CollisionAvoidanceStrategy = "Retract Height"
        operation.PeckEnabled = True
        operation.Proxy.execute(operation)

        expected = """G0 F200 Z15
G0 F200 X5 Y5
G0 F200 Z13
G83 F100 Q2 R11 X5 Y5 Z0
G83 F100 Q2 R11 X10 Y10 Z0
G83 F100 Q2 R11 X20 Y20 Z0
G0 Z15"""

        result, annotation = self.getSimpleGcodeFromPath(operation.Path)
        self.assertEqual(result, expected)
        self.assertIn("G98", annotation)

    def test14_collision_retract_peck_depth(self):
        """Test Drilling operation
        - peck
        - peck retract lower than top model face
        - rapids at SafeHeight"""
        operation = self.createDrillingOp()
        operation.CollisionAvoidanceStrategy = "Retract Height"
        operation.PeckEnabled = True
        operation.PeckRetract = 9.0
        operation.Proxy.execute(operation)

        expected = """G0 F200 Z15
G0 F200 X5 Y5
G0 F200 Z13
G83 F100 Q2 R9 X5 Y5 Z0
G83 F100 Q2 R9 X10 Y10 Z0
G83 F100 Q2 R9 X20 Y20 Z0
G0 Z15"""

        result, annotation = self.getSimpleGcodeFromPath(operation.Path)
        self.assertEqual(result, expected)
        self.assertIn("G98", annotation)

    def test15_collision_retract_peck_depth_keeptooldown(self):
        """Test Drilling operation
        - peck
        - peck retract lower than top model face
        - rapids at SafeHeight
        - keep tool down"""
        operation = self.createDrillingOp()
        operation.CollisionAvoidanceStrategy = "Retract Height"
        operation.PeckEnabled = True
        operation.PeckRetract = 9.0
        operation.KeepToolDown = True
        operation.Proxy.execute(operation)

        expected = """G0 F200 Z15
G0 F200 X5 Y5
G0 F200 Z13
G83 F100 Q2 R9 X5 Y5 Z0
G0 F200 X5 Y5 Z13
G0 F200 X10 Y10 Z13
G83 F100 Q2 R9 X10 Y10 Z0
G0 F200 X10 Y10 Z13
G0 F200 X20 Y20 Z13
G83 F100 Q2 R9 X20 Y20 Z0
G0 Z15"""

        result, annotation = self.getSimpleGcodeFromPath(operation.Path)
        self.assertEqual(result, expected)
        self.assertIn("G99", annotation)

    def test16_collision_line_peck_depth_keeptooldown(self):
        """Test Drilling operation
        - peck
        - peck retract lower than top model face
        - rapids at SafeHeight
        - keep tool down"""
        operation = self.createDrillingOp()
        operation.CollisionAvoidanceStrategy = "Line of Sight"
        operation.PeckEnabled = True
        operation.PeckRetract = 9.0
        operation.KeepToolDown = True
        operation.Proxy.execute(operation)

        expected = """G0 F200 Z15
G0 F200 X5 Y5
G0 F200 Z13
G83 F100 Q2 R9 X5 Y5 Z0
G0 F200 X5 Y5 Z13
G0 F200 X10 Y10 Z13
G83 F100 Q2 R9 X10 Y10 Z0
G0 F200 X10 Y10 Z13
G0 F200 X20 Y20 Z13
G83 F100 Q2 R9 X20 Y20 Z0
G0 Z15"""

        result, annotation = self.getSimpleGcodeFromPath(operation.Path)
        self.assertEqual(result, expected)
        self.assertIn("G99", annotation)
