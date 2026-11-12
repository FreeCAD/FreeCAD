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
import Path.Op.MillFacing as PathMillFacing
from CAMTests import PathTestUtils

if FreeCAD.GuiUp:
    import Path.Main.Gui.Job as PathJobGui

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
# Path.Log.trackModule(Path.Log.thisModule())


class TestPathMillFacing(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument()
        FreeCAD.setActiveDocument(self.doc.Name)

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def create_millfacing_op(self, stock_shape=None):
        """Add shape to self.doc and return a MillFacing op on its upward-facing top face"""
        part = self.doc.addObject("Part::Feature", "Part")
        model_shape = Part.makeBox(20, 20, 20)
        part.Shape = model_shape

        if stock_shape is None:
            stock_shape = Part.makeBox(20, 20, 20)

        job = PathJob.Create(f"Job_{self._testMethodName}", [part], None)
        if FreeCAD.GuiUp:
            job.ViewObject.Proxy = PathJobGui.ViewProvider(job.ViewObject)

        job.Stock.Shape = stock_shape
        job.Tools.Group[0].HorizFeed = 6000
        job.Tools.Group[0].VertFeed = 3000
        job.Tools.Group[0].HorizRapid = 12000
        job.Tools.Group[0].VertRapid = 5000
        job.Tools.Group[0].Tool.Diameter = 10

        op = PathMillFacing.Create("MillFacing", parentJob=job)

        op.CutMode = "Climb"
        op.ClearingPattern = "ZigZag"
        op.Reverse = False

        op.clearExpression("ClearanceHeight")
        op.ClearanceHeight = 25
        op.clearExpression("SafeHeight")
        op.SafeHeight = 23
        op.clearExpression("StartDepth")
        op.StartDepth = 21
        op.clearExpression("StepDown")
        op.StepDown = 999

        op.AxialStockToLeave = 0
        op.PassExtension = 0
        op.StockExtension = 0
        op.Angle = 0
        op.StepOver = 50

        return op

    def getSimpleGcodeFromPath(self, path):
        """Returns string (gcode) without decimals and annotations to simplify comparing result"""
        lines = []
        annotation = None
        for cmd in path.Commands:
            if cmd.Name.startswith("("):
                continue
            params = cmd.Parameters
            for key, val in params.items():
                params[key] = int(val)
            cmd.Parameters = params
            line = cmd.toGCode()
            line = line.replace(".000000", "")
            lst = line.split(";")
            lines.append(lst[0])
            if len(lst) > 1:
                annotation = lst[1]

        return "\n".join(lines), annotation

    def test00_create_millfacing_op(self):
        """Create MillFacing operation"""
        op = self.create_millfacing_op()
        op.recompute()

        self.assertTrue(op.Path.Commands)

    def test10_spiral_rectangular(self):
        """Verify MillFacing operation with pattern Spiral Rectangular"""
        op = self.create_millfacing_op(Part.makeBox(30, 20, 20))
        op.ClearingPattern = "Spiral Rectangular"
        op.recompute()

        expected = """G0 F5000 Z25
G0 F12000 X30 Y20
G0 F5000 Z23
G1 F3000 Z20
G1 F6000 X30 Y0 Z20
G1 F6000 X0 Y0 Z20
G1 F6000 X0 Y20 Z20
G1 F6000 X25 Y20 Z20
G1 F6000 X25 Y5 Z20
G1 F6000 X5 Y5 Z20
G1 F6000 X5 Y15 Z20
G1 F6000 X25 Y15 Z20
G0 F5000 Z25
G0 Z25
"""

        result, _ = self.getSimpleGcodeFromPath(op.Path)
        self.assertEqual(result.split(), expected.split())

    def test20_zigzag(self):
        """Verify MillFacing operation with pattern ZigZag"""
        op = self.create_millfacing_op(Part.makeBox(30, 20, 20))
        op.ClearingPattern = "ZigZag"
        op.recompute()

        expected = """G0 F5000 Z25
G0 F12000 X35 Y0
G0 F5000 Z23
G1 F3000 Z20
G1 F6000 X-5 Y0 Z20
G2 F6000 I0 J2 K0 X-5 Y5 Z20
G1 F6000 X35 Y5 Z20
G3 F6000 I0 J2 K0 X35 Y10 Z20
G1 F6000 X-5 Y10 Z20
G2 F6000 I0 J2 K0 X-5 Y15 Z20
G1 F6000 X35 Y15 Z20
G0 F5000 Z25
G0 Z25
"""

        result, _ = self.getSimpleGcodeFromPath(op.Path)
        self.assertEqual(result.split(), expected.split())

    def test30_bidirectional(self):
        """Verify MillFacing operation with pattern Bidirectional"""
        op = self.create_millfacing_op(Part.makeBox(30, 20, 20))
        op.ClearingPattern = "Bidirectional"
        op.recompute()

        expected = """G0 F5000 Z25
G0 F12000 X35 Y0
G0 F5000 Z23
G1 F3000 Z20
G1 F6000 X-5 Y0 Z20
G0 F12000 X-5 Y20 Z20
G1 F6000 X35 Y20 Z20
G0 F12000 X35 Y5 Z20
G1 F6000 X-5 Y5 Z20
G0 F12000 X-5 Y15 Z20
G1 F6000 X35 Y15 Z20
G0 F5000 Z25
G0 Z25
"""

        result, _ = self.getSimpleGcodeFromPath(op.Path)
        self.assertEqual(result.split(), expected.split())

    def test40_directional(self):
        """Verify MillFacing operation with pattern Directional"""
        op = self.create_millfacing_op(Part.makeBox(30, 20, 20))
        op.ClearingPattern = "Directional"
        op.recompute()

        expected = """G0 F5000 Z25
G0 F12000 X35 Y0
G0 F5000 Z23
G1 F3000 Z20
G1 F6000 X-5 Y0 Z20
G0 F5000 X-5 Y0 Z23
G0 F12000 X35 Y5 Z23
G0 F5000 X35 Y5 Z20
G1 F6000 X-5 Y5 Z20
G0 F5000 X-5 Y5 Z23
G0 F12000 X35 Y10 Z23
G0 F5000 X35 Y10 Z20
G1 F6000 X-5 Y10 Z20
G0 F5000 X-5 Y10 Z23
G0 F12000 X35 Y15 Z23
G0 F5000 X35 Y15 Z20
G1 F6000 X-5 Y15 Z20
G0 F5000 Z25
G0 Z25
"""

        result, _ = self.getSimpleGcodeFromPath(op.Path)
        self.assertEqual(result.split(), expected.split())

    def test50_spiral_circular(self):
        """Verify MillFacing operation with pattern Spiral Circular"""
        cyl_radius = 10
        cyl_height = 20
        op = self.create_millfacing_op(Part.makeCylinder(cyl_radius, cyl_height))
        op.ClearingPattern = "Spiral Circular"
        op.recompute()

        expected = """G0 F5000 Z25
G0 F12000 X10 Y0
G0 F5000 Z23
G1 F3000 Z20
G2 F6000 I-9 J0 X4 Y-8 Z20
G2 F6000 I-4 J8 X-4 Y-7 Z20
G2 F6000 I4 J7 X-8 Y0 Z20
G2 F6000 I7 J0 X-3 Y6 Z20
G2 F6000 I3 J-6 X3 Y5 Z20
G2 F6000 I-3 J-5 X6 Y0 Z20
G2 F6000 I-5 J0 X2 Y-4 Z20
G2 F6000 I-2 J4 X-2 Y-4 Z20
G2 F6000 I2 J3 X-4 Y0 Z20
G2 F6000 I3 J0 X-1 Y3 Z20
G2 F6000 I1 J-3 X1 Y2 Z20
G2 F6000 I-1 J-2 X2 Y0 Z20
G2 F6000 I-2 J0 X-2 Y0 Z20
G2 F6000 I2 J0 X2 Y0 Z20
G0 F5000 Z25
G0 Z25
"""

        result, _ = self.getSimpleGcodeFromPath(op.Path)
        self.assertEqual(result.split(), expected.split())
