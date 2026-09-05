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

"""Unit tests for the CAM Array operation (Path.Op.Array)"""

import re
import FreeCAD
import Part
import Path
import Path.Main.Job as PathJob
from Path.Op import Array
from Path.Op import Custom
from Path.Post.Processor import PostProcessorFactory
import Path.Tool.Controller as PathToolController
from CAMTests import PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TestPathArrayOp(PathTestUtils.PathTestBase):
    """Unit tests for Array op"""

    def _resetArrayOp(self):
        if self.array is None:
            self.array = Array.Create([self.op1], name="Array")

        self.array.Active = True
        self.array.Base = [self.op1]
        self.array.ReverseDirection = False
        self.array.Combine = False
        self.array.ExpandArray = False

        # Linear1D pattern
        self.array.Type = "Linear1D"
        self.array.Copies = 1
        self.array.Offset.x = 0
        self.array.Offset.y = 0

        # Linear2D pattern
        self.array.CopiesX = 1
        self.array.CopiesY = 1
        self.array.SwapDirection = False

        # polar pattern
        self.array.Angle = 0
        self.array.Centre = FreeCAD.Vector()

        # points pattern
        self.array.PointsSource = None
        self.array.PointsOrigin = None

        # jitter
        self.array.UseJitter = False
        self.array.JitterMagnitude = FreeCAD.Vector()
        self.array.JitterSeed = 0
        self.array.JitterAngle = 0

    def _resetOp1(self):
        if self.op1 is None:
            self.op1 = Custom.Create(name="Custom1", parentJob=self.job)

        self.op1.CoolantMode = "None"
        self.op1.Source = "Text"
        self.op1.ToolController = self.job.Tools.Group[1]
        self.op1.Path = Path.Path([Path.Command("G1", {"X": 1, "Y": 1})])

    def _resetOp2(self):
        if self.op2 is None:
            self.op2 = Custom.Create(name="Custom2", parentJob=self.job)

        self.op2.CoolantMode = "None"
        self.op2.Source = "Text"
        self.op2.ToolController = self.job.Tools.Group[2]
        self.op2.Path = Path.Path([Path.Command("G1", {"X": 2, "Y": 2})])

    def setUp(self):
        self.doc = FreeCAD.newDocument("test")
        box = self.doc.addObject("Part::Box", "TestBox")
        self.job = PathJob.Create("Job", [box])

        tc1 = PathToolController.Create(name="TC1", toolNumber=1)
        self.job.Proxy.addToolController(tc1)
        tc2 = PathToolController.Create(name="TC2", toolNumber=2)
        self.job.Proxy.addToolController(tc2)

        self.op1 = None
        self.op2 = None
        self.array = None

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def test_01(self):
        """Verify Array op does not throw an exception."""
        self._resetOp1()
        self._resetArrayOp()

    def test_02(self):
        """Verify Linear1D array"""
        self._resetOp1()
        self._resetArrayOp()
        self.array.Type = "Linear1D"
        self.array.Copies = 3
        self.array.Offset.x = 10
        self.array.recompute()

        result = self.array.Path.toGCode().strip()
        expected = "G1 X11.000000 Y1.000000\nG1 X21.000000 Y1.000000"
        self.assertEqual(result, expected, "Error Linear1D pattern")

    def test_03(self):
        """Verify Linear2D YX array"""
        self._resetOp1()
        self._resetArrayOp()
        self.array.Type = "Linear2D"
        self.array.CopiesX = 2
        self.array.CopiesY = 2
        self.array.Offset.x = 10
        self.array.Offset.y = 10
        self.array.recompute()

        result = self.array.Path.toGCode().strip()
        expected = "G1 X1.000000 Y11.000000\nG1 X11.000000 Y11.000000\nG1 X11.000000 Y1.000000"
        self.assertEqual(result, expected, "Error Linear2D YX pattern")

    def test_04(self):
        """Verify Linear2D XY array"""
        self._resetOp1()
        self._resetArrayOp()
        self.array.Type = "Linear2D"
        self.array.SwapDirection = True
        self.array.CopiesX = 2
        self.array.CopiesY = 2
        self.array.Offset.x = 10
        self.array.Offset.y = 10
        self.array.recompute()

        result = self.array.Path.toGCode().strip()
        expected = "G1 X11.000000 Y1.000000\nG1 X11.000000 Y11.000000\nG1 X1.000000 Y11.000000"
        self.assertEqual(result, expected, "Error Linear2D XY pattern")

    def test_05(self):
        """Verify Polar array"""
        self._resetOp1()
        self._resetArrayOp()
        self.array.Type = "Polar"
        self.array.Copies = 2

        self.array.Angle = 180
        self.array.recompute()
        result = self.array.Path.toGCode().strip()
        expected = "G1 X-1.000000 Y-1.000000"
        self.assertEqual(result, expected, "Error Polar pattern 180 degrees")

        self.array.Angle = 360
        self.array.recompute()
        result = self.array.Path.toGCode().strip()
        expected = "G1 X-1.000000 Y-1.000000"
        self.assertEqual(result, expected, "Error Polar pattern 360 degrees")

    def test_06(self):
        """Verify Points array"""
        self._resetOp1()
        self._resetArrayOp()
        self.array.Type = "Points"
        source = Part.show(Part.Vertex(FreeCAD.Vector(5, 5, 0)))
        origin = Part.show(Part.Vertex(FreeCAD.Vector()))
        self.array.PointsSource = [source]
        self.array.PointsOrigin = (origin, ())
        self.array.recompute()

        result = self.array.Path.toGCode().strip()
        expected = "G1 X6.000000 Y6.000000"
        self.assertEqual(result, expected, "Error Points pattern")

    def test_07(self):
        """Verify Jitter with Linear1D array"""
        self._resetOp1()
        self._resetArrayOp()
        testCmds1 = [Path.Command("G1", {"X": 0, "Y": 0}), Path.Command("G1", {"X": 10})]
        self.op1.Path = Path.Path(testCmds1)
        self.array.Type = "Linear1D"
        self.array.Copies = 2
        self.array.Offset.x = 10
        self.array.Offset.y = 10

        self.array.UseJitter = True
        self.array.JitterSeed = 0
        self.array.JitterAngle = 45
        self.array.JitterMagnitude = FreeCAD.Vector(5, 5, 0)

        self.array.recompute()

        resultX = self.array.Path.Commands[0].x
        resultY = self.array.Path.Commands[0].y
        self.assertEqual(round(resultX, 1), 13.8, "Error jitter X")
        self.assertEqual(round(resultY, 1), 14.4, "Error jitter Y")

    def test_08(self):
        """Verify Linear1D array with ReverseDirection"""
        self._resetOp1()
        self._resetArrayOp()
        self.array.Type = "Linear1D"
        self.array.Copies = 3
        self.array.Offset.x = 10
        self.array.ReverseDirection = True
        self.array.recompute()

        result = self.array.Path.toGCode().strip()
        expected = "G1 X21.000000 Y1.000000\nG1 X11.000000 Y1.000000"
        self.assertEqual(result, expected, "Error Linear1D pattern with ReverseDirection")

    def test_10(self):
        """Verify Linear1D array with two base operations and same tool controller"""
        self._resetOp1()
        self._resetOp2()
        self.op2.ToolController = self.job.Tools.Group[1]

        self._resetArrayOp()
        self.array.Type = "Linear1D"
        self.array.Copies = 2
        self.array.Offset.x = 10
        self.array.Base = [self.op1, self.op2]
        self.array.recompute()

        result = self.array.Path.toGCode().strip()
        expected = "G1 X11.000000 Y1.000000\nG1 X12.000000 Y2.000000"
        self.assertEqual(
            result,
            expected,
            "Error Linear1D with two base operations and same tool controller",
        )

    def test_11(self):
        """Verify Linear1D array with two base operations, some tool controller and coolant"""
        self._resetOp1()
        self._resetOp2()
        self.op2.ToolController = self.job.Tools.Group[1]

        self.op1.CoolantMode = "Mist"
        self.op2.CoolantMode = "Mist"

        self._resetArrayOp()
        self.array.Type = "Linear1D"
        self.array.Copies = 2
        self.array.Offset.x = 10
        self.array.Base = [self.op1, self.op2]
        self.array.recompute()

        self.assertEqual(
            len(self.array.ArrayGroup),
            2,
            "Error with two base operations, same tool controller and coolant",
        )

    def test_12(self):
        """Verify Linear1D array with two base operations and different tool controllers"""
        self._resetOp1()
        self._resetOp2()

        self._resetArrayOp()
        self.array.Type = "Linear1D"
        self.array.Copies = 2
        self.array.Offset.x = 10
        self.array.Base = [self.op1, self.op2]
        self.array.recompute()

        self.assertEqual(
            len(self.array.ArrayGroup),
            2,
            "Error with two base operations and different tool controllers",
        )

    def test_13(self):
        """Verify post processing array with multiple tool controllers and order Operation"""
        self._resetOp1()
        self._resetOp2()
        self._resetArrayOp()
        self.array.Type = "Linear1D"
        self.array.Copies = 2
        self.array.Offset.x = 10
        self.array.Base = [self.op1, self.op2]
        self.array.recompute()

        self.job.OrderOutputBy = "Operation"
        self.job.Fixtures = ["G54"]
        post = PostProcessorFactory.get_post_processor(self.job, "generic")
        gcode = post.export()[0][1].split("\n")
        begin = "(Begin operation: TC"

        result = [int(re.search(r"(\d)", cmd).group(1)) for cmd in gcode if cmd.startswith(begin)]
        expected = [1, 2, 1, 2]

        self.assertEqual(result, expected, "Error export sequence with order Operation")

    def test_14(self):
        """Verify post processing array with multiple tool controllers and order Tool"""
        self._resetOp1()
        self._resetOp2()
        self._resetArrayOp()
        self.array.Type = "Linear1D"
        self.array.Copies = 2
        self.array.Offset.x = 10
        self.array.Base = [self.op1, self.op2]
        self.array.recompute()

        self.assertEqual(
            len(self.array.ArrayGroup),
            2,
            "Error with two base operations and different tool controllers",
        )

        self.job.OrderOutputBy = "Tool"
        self.job.Fixtures = ["G54"]
        post = PostProcessorFactory.get_post_processor(self.job, "generic")
        gcode = post.export()[0][1].split("\n")
        begin = "(Begin operation: TC"

        result = [int(re.search(r"(\d)", cmd).group(1)) for cmd in gcode if cmd.startswith(begin)]
        expected = [1, 2]

        self.assertEqual(result, expected, "Error export sequence with order Tool")

    def test_15(self):
        """Verify post processing array with multiple tool controllers and order Fixture"""
        self._resetOp1()
        self._resetOp2()
        self._resetArrayOp()
        self.array.Type = "Linear1D"
        self.array.Copies = 2
        self.array.Offset.x = 10
        self.array.Base = [self.op1, self.op2]
        self.array.recompute()

        self.assertEqual(
            len(self.array.ArrayGroup),
            2,
            "Error with two base operations and different tool controllers",
        )

        self.job.OrderOutputBy = "Fixture"
        self.job.Fixtures = ["G54", "G55"]
        post = PostProcessorFactory.get_post_processor(self.job, "generic")
        gcode = post.export()[0][1].split("\n")
        begin = "(Begin operation: TC"

        result = [int(re.search(r"(\d)", cmd).group(1)) for cmd in gcode if cmd.startswith(begin)]
        expected = [1, 2, 1, 2, 1, 2, 1, 2]

        self.assertEqual(result, expected, "Error export sequence with order Fixture")
