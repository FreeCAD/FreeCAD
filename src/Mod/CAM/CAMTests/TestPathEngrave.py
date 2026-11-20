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

from CAMTests import PathTestUtils
import FreeCAD
import Part
import Path
import Path.Base.SetupSheetOpPrototype as PathSetupSheetOpPrototype
import Path.Main.Job as PathJob
from Path.Op import Engrave
import Path.Op.Util as PathOpUtil

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestPathEngrave(PathTestUtils.PathTestBase):
    def setUp(self):
        self.maxDiff = None
        self.clone = None
        self.doc = FreeCAD.newDocument("test")
        box = self.doc.addObject("Part::Box", "TestBox")
        self.job = PathJob.Create("Job", [box])
        self.job.Stock.ExtZpos = 0

    def _make_op(self):
        """Create Engrave operation"""
        op = Engrave.Create("Engrave")

        op.Approximation = False
        op.Reverse = False
        op.SortingMode = "Manual"
        op.CutPattern = "Bidirectional"
        op.StartVertex = 0

        op.clearExpression("ClearanceHeight")
        op.ClearanceHeight = 15
        op.clearExpression("SafeHeight")
        op.SafeHeight = 13
        op.clearExpression("StepDown")
        op.StepDown = 5

        return op

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test00(self):
        """Verify op does not throw an exception."""

        op = self._make_op()
        op.Proxy.execute(op)

    def test_offset_spline_wire_reaches_both_ends(self):
        """Offset spline arcs must remain connected at the job geometry tolerance."""
        curve = Part.BSplineCurve()
        curve.buildFromPolesMultsKnots(
            [
                FreeCAD.Vector(10, 30, 10),
                FreeCAD.Vector(25.55890846, 22.20499229, 10),
                FreeCAD.Vector(31.42255974, 29, 10),
                FreeCAD.Vector(45.34870911, 23.96408844, 10),
                FreeCAD.Vector(60, 30, 10),
            ],
            [4, 1, 4],
            [0, 1, 2],
            False,
            3,
        )
        wire = Part.Wire([curve.toShape()])
        tol = self.job.GeometryTolerance.Value
        base = Part.makeBox(100, 100, 10)
        wires = PathOpUtil.offsetWireCompat(wire, base, 0.55, None, tol)
        self.assertEqual(len(wires), 1)
        self.assertGreater(len(wires[0].Edges), 1)

        op = self._make_op()
        op.Proxy.execute(op)
        op.Proxy.commandlist = []
        op.Proxy.buildpathocc(op, wires, [0, -1], forward=True)
        cuts = [c for c in op.Proxy.commandlist if c.Name in ("G2", "G3")]
        self.assertEqual(len(cuts), 2 * len(wires[0].Edges))
        points = [wires[0].Edges[0].Vertexes[0].Point, wires[0].Edges[-1].Vertexes[-1].Point]
        for cmd in (cuts[len(wires[0].Edges) - 1], cuts[-1]):
            point = FreeCAD.Vector(cmd.Parameters["X"], cmd.Parameters["Y"], 10)
            self.assertLess(min(point.distanceToPoint(p) for p in points), tol)
        self.assertGreater(
            FreeCAD.Vector(cuts[-1].Parameters["X"], cuts[-1].Parameters["Y"], 10).distanceToPoint(
                FreeCAD.Vector(cuts[0].Parameters["X"], cuts[0].Parameters["Y"], 10)
            ),
            1,
        )

        # A gap larger than the configured tolerance must not leave a usable
        # partial or stale path on the operation.
        self.job.GeometryTolerance = 0.000001
        op.Path = Path.Path("G1 X5")
        op.Proxy.commandlist = []
        with self.assertRaisesRegex(ValueError, "do not connect"):
            op.Proxy.buildpathocc(op, wires, [0], forward=True)
        self.assertEqual(len(op.Path.Commands), 0)

    def testCreateWithPrototype(self):
        """Verify a op can be created on a SetupSheet's prototype instead of a real document object"""

        ptt = PathSetupSheetOpPrototype.OpPrototype("Engrave")
        Engrave.Create("OpPrototype.Engrave", ptt)

    def test01(self):
        """Verify Engrave generates correct path for two lines, directional"""

        op = self._make_op()
        wire1 = self.doc.addObject("Part::Feature", "wire1")
        wire1.Shape = Part.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0))
        wire2 = self.doc.addObject("Part::Feature", "wire2")
        wire2.Shape = Part.makeLine(FreeCAD.Vector(5, 5, 0), FreeCAD.Vector(10, 10, 0))
        op.Base = [wire1, wire2]
        op.CutPattern = "Directional"
        op.recompute()

        expected = """(Engrave)
G0 Z15.000000
G0 X10.000000 Y0.000000
G0 Z13.000000
G1 F0.000000 Z5.000000
G1 F0.000000 X0.000000 Y0.000000 Z5.000000
G0 X0.000000 Y0.000000 Z13.000000
G0 X10.000000 Y0.000000 Z13.000000
G1 F0.000000 Z0.000000
G1 F0.000000 X0.000000 Y0.000000 Z0.000000
G0 X0.000000 Y0.000000 Z13.000000
G0 X10.000000 Y10.000000 Z13.000000
G1 F0.000000 Z5.000000
G1 F0.000000 X5.000000 Y5.000000 Z5.000000
G0 X5.000000 Y5.000000 Z13.000000
G0 X10.000000 Y10.000000 Z13.000000
G1 F0.000000 Z0.000000
G1 F0.000000 X5.000000 Y5.000000 Z0.000000
G0 Z15.000000
"""

        # remove Annotations from result
        current = "\n".join([cmd.toGCode().split(";")[0] for cmd in op.Path.Commands])
        self.assertEqual(expected.strip(), current.strip())

    def test02(self):
        """Verify Engrave generates correct path for two lines, biderectional"""

        op = self._make_op()
        wire1 = self.doc.addObject("Part::Feature", "wire1")
        wire1.Shape = Part.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0))
        wire2 = self.doc.addObject("Part::Feature", "wire2")
        wire2.Shape = Part.makeLine(FreeCAD.Vector(5, 5, 0), FreeCAD.Vector(10, 10, 0))
        op.Base = [wire1, wire2]
        op.recompute()

        expected = """(Engrave)
G0 Z15.000000
G0 X10.000000 Y0.000000
G0 Z13.000000
G1 F0.000000 Z5.000000
G1 F0.000000 X0.000000 Y0.000000 Z5.000000
G1 F0.000000 Z0.000000
G1 F0.000000 X10.000000 Y0.000000 Z0.000000
G0 X10.000000 Y0.000000 Z13.000000
G0 X10.000000 Y10.000000 Z13.000000
G1 F0.000000 Z5.000000
G1 F0.000000 X5.000000 Y5.000000 Z5.000000
G1 F0.000000 Z0.000000
G1 F0.000000 X10.000000 Y10.000000 Z0.000000
G0 Z15.000000
"""

        current = "\n".join([cmd.toGCode().split(";")[0] for cmd in op.Path.Commands])
        self.assertEqual(expected.strip(), current.strip())

    def test03(self):
        """Verify Engrave generates correct path for two lines with Reverse"""

        op = self._make_op()
        wire1 = self.doc.addObject("Part::Feature", "wire_line_1")
        wire1.Shape = Part.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0))
        wire2 = self.doc.addObject("Part::Feature", "wire_line_2")
        wire2.Shape = Part.makeLine(FreeCAD.Vector(5, 5, 0), FreeCAD.Vector(10, 10, 0))
        op.Base = [wire1, wire2]
        op.Reverse = True
        op.recompute()

        expected = """(Engrave)
G0 Z15.000000
G0 X0.000000 Y0.000000
G0 Z13.000000
G1 F0.000000 Z5.000000
G1 F0.000000 X10.000000 Y0.000000 Z5.000000
G1 F0.000000 Z0.000000
G1 F0.000000 X0.000000 Y0.000000 Z0.000000
G0 X0.000000 Y0.000000 Z13.000000
G0 X5.000000 Y5.000000 Z13.000000
G1 F0.000000 Z5.000000
G1 F0.000000 X10.000000 Y10.000000 Z5.000000
G1 F0.000000 Z0.000000
G1 F0.000000 X5.000000 Y5.000000 Z0.000000
G0 Z15.000000
"""

        current = "\n".join([cmd.toGCode().split(";")[0] for cmd in op.Path.Commands])
        self.assertEqual(expected.strip(), current.strip())

    def test04(self):
        """Verify Engrave generates correct path for rectangle"""

        op = self._make_op()
        wire = self.doc.addObject("Part::Feature", "wire_rectangle")
        edge1 = Part.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0))
        edge2 = Part.makeLine(FreeCAD.Vector(10, 0, 0), FreeCAD.Vector(10, 10, 0))
        edge3 = Part.makeLine(FreeCAD.Vector(10, 10, 0), FreeCAD.Vector(0, 10, 0))
        edge4 = Part.makeLine(FreeCAD.Vector(0, 10, 0), FreeCAD.Vector(0, 0, 0))
        wire.Shape = Part.Wire([edge1, edge2, edge3, edge4])
        op.Base = [wire]
        op.recompute()

        expected = """(Engrave)
G0 Z15.000000
G0 X0.000000 Y0.000000
G0 Z13.000000
G1 F0.000000 Z5.000000
G1 F0.000000 X0.000000 Y10.000000 Z5.000000
G1 F0.000000 X10.000000 Y10.000000 Z5.000000
G1 F0.000000 X10.000000 Y0.000000 Z5.000000
G1 F0.000000 X0.000000 Y0.000000 Z5.000000
G1 F0.000000 Z0.000000
G1 F0.000000 X0.000000 Y10.000000 Z0.000000
G1 F0.000000 X10.000000 Y10.000000 Z0.000000
G1 F0.000000 X10.000000 Y0.000000 Z0.000000
G1 F0.000000 X0.000000 Y0.000000 Z0.000000
G0 Z15.000000
"""

        current = "\n".join([cmd.toGCode().split(";")[0] for cmd in op.Path.Commands])
        self.assertEqual(expected.strip(), current.strip())

    def test05(self):
        """Verify Engrave generates correct path for rectangle with reverse"""

        op = self._make_op()
        wire = self.doc.addObject("Part::Feature", "wire_rectangle")
        edge1 = Part.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0))
        edge2 = Part.makeLine(FreeCAD.Vector(10, 0, 0), FreeCAD.Vector(10, 10, 0))
        edge3 = Part.makeLine(FreeCAD.Vector(10, 10, 0), FreeCAD.Vector(0, 10, 0))
        edge4 = Part.makeLine(FreeCAD.Vector(0, 10, 0), FreeCAD.Vector(0, 0, 0))
        wire.Shape = Part.Wire([edge1, edge2, edge3, edge4])
        op.Base = [wire]
        op.Reverse = True
        op.recompute()

        expected = """(Engrave)
G0 Z15.000000
G0 X0.000000 Y0.000000
G0 Z13.000000
G1 F0.000000 Z5.000000
G1 F0.000000 X10.000000 Y0.000000 Z5.000000
G1 F0.000000 X10.000000 Y10.000000 Z5.000000
G1 F0.000000 X0.000000 Y10.000000 Z5.000000
G1 F0.000000 X0.000000 Y0.000000 Z5.000000
G1 F0.000000 Z0.000000
G1 F0.000000 X10.000000 Y0.000000 Z0.000000
G1 F0.000000 X10.000000 Y10.000000 Z0.000000
G1 F0.000000 X0.000000 Y10.000000 Z0.000000
G1 F0.000000 X0.000000 Y0.000000 Z0.000000
G0 Z15.000000
"""

        current = "\n".join([cmd.toGCode().split(";")[0] for cmd in op.Path.Commands])
        self.assertEqual(expected.strip(), current.strip())

    def test06(self):
        """Verify Engrave generates correct path for rectangle with changed start index"""

        op = self._make_op()
        wire = self.doc.addObject("Part::Feature", "wire_rectangle")
        edge1 = Part.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0))
        edge2 = Part.makeLine(FreeCAD.Vector(10, 0, 0), FreeCAD.Vector(10, 10, 0))
        edge3 = Part.makeLine(FreeCAD.Vector(10, 10, 0), FreeCAD.Vector(0, 10, 0))
        edge4 = Part.makeLine(FreeCAD.Vector(0, 10, 0), FreeCAD.Vector(0, 0, 0))
        wire.Shape = Part.Wire([edge1, edge2, edge3, edge4])
        op.Base = [wire]
        op.StartVertex = 1
        op.recompute()

        expected = """(Engrave)
G0 Z15.000000
G0 X0.000000 Y10.000000
G0 Z13.000000
G1 F0.000000 Z5.000000
G1 F0.000000 X10.000000 Y10.000000 Z5.000000
G1 F0.000000 X10.000000 Y0.000000 Z5.000000
G1 F0.000000 X0.000000 Y0.000000 Z5.000000
G1 F0.000000 X0.000000 Y10.000000 Z5.000000
G1 F0.000000 Z0.000000
G1 F0.000000 X10.000000 Y10.000000 Z0.000000
G1 F0.000000 X10.000000 Y0.000000 Z0.000000
G1 F0.000000 X0.000000 Y0.000000 Z0.000000
G1 F0.000000 X0.000000 Y10.000000 Z0.000000
G0 Z15.000000
"""

        current = "\n".join([cmd.toGCode().split(";")[0] for cmd in op.Path.Commands])
        self.assertEqual(expected.strip(), current.strip())
