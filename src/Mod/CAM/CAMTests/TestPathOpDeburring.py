# SPDX-License-Identifier: LGPL-2.1-or-later
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
import Path.Op.Deburring as PathDeburring
from Path.Tool.toolbit import ToolBit
from CAMTests import PathTestUtils
import Path.Tool.Controller as PathToolController

if FreeCAD.GuiUp:
    import Path.Main.Gui.Job as PathJobGui

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
# Path.Log.trackModule(Path.Log.thisModule())


class TestPathOpDeburring(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument()
        FreeCAD.setActiveDocument(self.doc.Name)

        part = self.doc.addObject("Part::Feature", "Part")
        shape = Part.makeBox(10, 10, 10)
        part.Shape = shape

        self.job = PathJob.Create(f"Job_{self._testMethodName}", [part], None)
        if FreeCAD.GuiUp:
            self.job.ViewObject.Proxy = PathJobGui.ViewProvider(self.job.ViewObject)

        self.job.Tools.Group[0].Tool.Diameter = 5

        for i, face in enumerate(shape.Faces):
            if face.BoundBox.ZMin == shape.BoundBox.ZMax:
                sub_name = f"Face{i + 1}"

        self.deburr = PathDeburring.Create("Deburring", parentJob=self.job)
        self.deburr.Base = [(part, [sub_name])]
        self.deburr.Direction = "CW"
        self.deburr.Width = 1
        self.deburr.ExtraDepth = 0
        self.deburr.Angle = 45

        self.deburr.clearExpression("ClearanceHeight")
        self.deburr.ClearanceHeight = 25
        self.deburr.clearExpression("SafeHeight")
        self.deburr.SafeHeight = 23
        self.deburr.clearExpression("StartDepth")
        self.deburr.StartDepth = 21
        self.deburr.clearExpression("StepDown")
        self.deburr.StepDown = 999

        tool_attrs = {
            "name": "Tool2",
            "shape": "v-bit.fcstd",
            "parameter": {"Diameter": 10.0, "TipDiameter": 0.1, "CuttingEdgeAngle": 90},
            "attribute": {},
        }
        toolbit = ToolBit.from_dict(tool_attrs)
        tool = toolbit.attach_to_doc(doc=self.doc)
        tool.Label = "v-bit"
        self.tc1 = PathToolController.Create("TC_V-bit", tool, 2)
        self.tc1.HorizFeed = 6000
        self.tc1.VertFeed = 3000
        self.job.Proxy.addToolController(self.tc1)

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def getSimpleGcodeFromPath(self, path):
        """Returns string (gcode) without decimals and annotations to simplify comparing result"""
        lines = []
        annotation = None
        for cmd in path.Commands:
            line = cmd.toGCode()
            if line.startswith("("):
                continue
            line = line.replace(".000000", "")
            lst = line.split(";")
            lines.append(lst[0])
            if len(lst) > 1:
                annotation = lst[1]

        return "\n".join(lines), annotation

    def test00(self):
        """Verify chamfer depths and offsets with V-bit"""
        self.deburr.ToolController = self.job.Tools.Group[1]
        self.deburr.Width = 1
        self.deburr.ExtraDepth = 0.5
        self.deburr.recompute()

        depths, offsets = self.deburr.Proxy.toolDepthAndOffset(self.deburr)

        self.assertEqual(len(depths), 1)
        self.assertEqual(len(offsets), 1)
        self.assertRoughly(-1.5, depths[0])
        self.assertRoughly(0.55, offsets[0])

    def test01(self):
        """Verify chamfer depths and offsets with Endmill"""
        self.deburr.Width = 1
        self.deburr.Angle = 45
        self.deburr.StepDown = 0.2
        self.deburr.recompute()

        depths, offsets = self.deburr.Proxy.toolDepthAndOffset(self.deburr)

        self.assertListEqual([-0.2, -0.4, -0.6, -0.8], [round(x, 2) for x in depths])
        self.assertListEqual([1.7, 1.9, 2.1, 2.3], [round(x, 2) for x in offsets])

    def test10_deburr_cylinder(self):
        """Verify chamfer around cylinder with V-bit"""
        cyl_radius = 10
        cyl_height = 20
        shape = Part.makeCylinder(cyl_radius, cyl_height)

        for i, face in enumerate(shape.Faces):
            if face.BoundBox.ZMin == shape.BoundBox.ZMax:
                sub_name = f"Face{i + 1}"

        part = self.doc.addObject("Part::Feature", "Part")
        part.Shape = shape

        self.deburr.Base = [(part, [sub_name])]
        self.deburr.Width = 1
        self.deburr.ExtraDepth = 1
        self.deburr.ToolController = self.tc1
        self.deburr.recompute()

        expected = """G0 Z25
G0 X11.050000 Y0
G0 Z23
G1 F3000 X11.050000 Y0 Z18
G2 F6000 I-11.050000 J0 K0 X-11.050000 Y0 Z18
G2 F6000 I11.050000 J0 K0 X11.050000 Y0 Z18
G0 Z25
"""

        result, _ = self.getSimpleGcodeFromPath(self.deburr.Path)
        self.assertEqual(result.split(), expected.split())

    def test11_deburr_cube(self):
        """Verify chamfer around cube with V-bit"""
        cube_side = 20
        shape = Part.makeBox(cube_side, cube_side, cube_side)

        for i, face in enumerate(shape.Faces):
            if face.BoundBox.ZMin == shape.BoundBox.ZMax:
                sub_name = f"Face{i + 1}"

        part = self.doc.addObject("Part::Feature", "Part")
        part.Shape = shape

        self.deburr.Base = [(part, [sub_name])]
        self.deburr.Width = 1
        self.deburr.ExtraDepth = 1
        self.deburr.ToolController = self.tc1
        self.deburr.recompute()

        expected = """G0 Z25
G0 X-1.050000 Y0
G0 Z23
G1 F3000 X-1.050000 Y0 Z18
G1 F6000 X-1.050000 Y20 Z18
G2 F6000 I1.050000 J0 K0 X0 Y21.050000 Z18
G1 F6000 X20 Y21.050000 Z18
G2 F6000 I0 J-1.050000 K0 X21.050000 Y20 Z18
G1 F6000 X21.050000 Y0 Z18
G2 F6000 I-1.050000 J0 K0 X20 Y-1.050000 Z18
G1 F6000 X0 Y-1.050000 Z18
G2 F6000 I0 J1.050000 K0 X-1.050000 Y0 Z18
G0 Z25
"""

        result, _ = self.getSimpleGcodeFromPath(self.deburr.Path)
        self.assertEqual(result.split(), expected.split())
