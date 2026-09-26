# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2018 sliptonic <shopinthewoods@gmail.com>
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
import Path.Op.Deburr as PathDeburr
from Path.Tool.toolbit import ToolBit
from CAMTests import PathTestUtils
import Path.Tool.Controller as PathToolController

if FreeCAD.GuiUp:
    import Path.Main.Gui.Job as PathJobGui

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
# Path.Log.trackModule(Path.Log.thisModule())


class TestPathOpDeburr(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument()
        FreeCAD.setActiveDocument(self.doc.Name)

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def create_deburr_op(self, shape=None):
        """Add shape to self.doc and return a Deburr op on its upward-facing top face"""
        part = self.doc.addObject("Part::Feature", "Part")
        if shape is None:
            shape = Part.makeBox(10, 10, 10)
        part.Shape = shape

        for i, face in enumerate(shape.Faces):
            if face.BoundBox.ZMin == shape.BoundBox.ZMax:
                sub_name = f"Face{i + 1}"

        job = PathJob.Create(f"Job_{self._testMethodName}", [part], None)
        if FreeCAD.GuiUp:
            job.ViewObject.Proxy = PathJobGui.ViewProvider(job.ViewObject)

        chamfer_bit = ToolBit.from_file(
            FreeCAD.getHomePath() + "Mod/CAM/Tools/Bit/90degree_Vbit.fctb"
        )
        tool = chamfer_bit.attach_to_doc(doc=self.doc)
        job.Tools.Group[0].Tool = tool

        deburr = PathDeburr.Create("Deburr", parentJob=job)
        deburr.Base = [(part, [sub_name])]

        deburr.clearExpression("ClearanceHeight")
        deburr.ClearanceHeight = 25
        deburr.clearExpression("SafeHeight")
        deburr.SafeHeight = 23
        deburr.clearExpression("StartDepth")
        deburr.StartDepth = 21
        deburr.clearExpression("StepDown")
        deburr.StepDown = 999

        return deburr

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
        """Verify chamfer depth and offset for an end mill"""
        deburr = self.create_deburr_op()

        tool_attrs = {
            "name": "Tool1",
            "shape": "endmill.fcstd",
            "parameter": {"Diameter": 6.0},
            "attribute": {},
        }
        toolbit = ToolBit.from_dict(tool_attrs)
        tool = toolbit.attach_to_doc(doc=self.doc)
        tool.Label = "6mm_Endmill"

        tc = PathToolController.Create("TC_Endmill", tool, 1)
        deburr.ToolController = tc

        depth, offset = deburr.Proxy.toolDepthAndOffset(width=1, extraDepth=0.5, tool=tool)
        self.assertRoughly(0.5, depth)
        self.assertRoughly(2, offset)

    def test01(self):
        """Verify chamfer depth and offset for an v-bit"""
        deburr = self.create_deburr_op()

        tool_attrs = {
            "name": "Tool2",
            "shape": "v-bit.fcstd",
            "parameter": {"Diameter": 10.0, "TipDiameter": 0.1, "CuttingEdgeAngle": 90},
            "attribute": {},
        }
        toolbit = ToolBit.from_dict(tool_attrs)
        tool = toolbit.attach_to_doc(doc=self.doc)
        tool.Label = "v-bit"

        tc = PathToolController.Create("TC_V-bit", tool, 1)
        deburr.ToolController = tc

        depth, offset = deburr.Proxy.toolDepthAndOffset(width=1, extraDepth=0.5, tool=tool)
        self.assertRoughly(1.5, depth)
        self.assertRoughly(0.55, offset)

    def test10_deburr_cylinder(self):
        """Verify chamfer around cylinder"""
        cyl_radius = 10
        cyl_height = 20

        deburr = self.create_deburr_op(Part.makeCylinder(cyl_radius, cyl_height))
        deburr.Width = 1
        deburr.ExtraDepth = 1
        deburr.recompute()

        expected = """G0 Z25
G0 X11.050000 Y0
G0 Z23
G1 F0 Z18
G2 F0 I-11.050000 J0 K0 X-11.050000 Y0 Z18
G2 F0 I11.050000 J0 K0 X11.050000 Y0 Z18
G0 Z25
"""

        result, _ = self.getSimpleGcodeFromPath(deburr.Path)
        self.assertEqual(result.split(), expected.split())

    def test11_deburr_cube(self):
        """Verify chamfer around cube"""
        cube_side = 20

        deburr = self.create_deburr_op(Part.makeBox(cube_side, cube_side, cube_side))
        deburr.Width = 1
        deburr.ExtraDepth = 1
        deburr.recompute()

        expected = """G0 Z25
G0 X21.050000 Y0
G0 Z23
G1 F0 Z18
G2 F0 I-1.050000 J0 K0 X20 Y-1.050000 Z18
G1 F0 X0 Y-1.050000 Z18
G2 F0 I0 J1.050000 K0 X-1.050000 Y0 Z18
G1 F0 X-1.050000 Y20 Z18
G2 F0 I1.050000 J0 K0 X0 Y21.050000 Z18
G1 F0 X20 Y21.050000 Z18
G2 F0 I0 J-1.050000 K0 X21.050000 Y20 Z18
G1 F0 X21.050000 Y0 Z18
G0 Z25
"""

        result, _ = self.getSimpleGcodeFromPath(deburr.Path)
        self.assertEqual(result.split(), expected.split())
