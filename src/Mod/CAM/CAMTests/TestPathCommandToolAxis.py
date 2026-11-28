# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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

"""Tests for the Tool Axis attribute of Path.Command."""

import math
import os
import shutil
import tempfile

import FreeCAD
import Path
import Path.Base.Util as PathUtil
from CAMTests.PathTestUtils import PathTestBase


class TestPathCommandToolAxis(PathTestBase):
    """The Tool Axis is optional data on a command; no operation uses it yet."""

    def test00(self):
        """A new command has no Tool Axis."""
        self.assertIsNone(Path.Command().ToolAxis)
        self.assertIsNone(Path.Command("G1", {"X": 1}).ToolAxis)
        self.assertIsNone(Path.Command("G1 X1 Y2").ToolAxis)

    def test01(self):
        """An assigned vector is normalized; None removes it."""
        c = Path.Command("G1", {"X": 1})
        c.ToolAxis = FreeCAD.Vector(0, 3, 4)
        self.assertCoincide(c.ToolAxis, FreeCAD.Vector(0, 0.6, 0.8), 1e-12)
        c.ToolAxis = None
        self.assertIsNone(c.ToolAxis)

    def test02(self):
        """A vector with no length and a value that is not a vector are refused."""
        c = Path.Command("G1", {"X": 1})
        with self.assertRaises(ValueError):
            c.ToolAxis = FreeCAD.Vector(0, 0, 0)
        with self.assertRaises(TypeError):
            c.ToolAxis = (0, 0, 1)
        self.assertIsNone(c.ToolAxis)

    def test03(self):
        """The Tool Axis does not change the G-code text, the name or the parameters."""
        plain = Path.Command("G1", {"X": 1, "Y": 2, "F": 100})
        tilted = Path.Command("G1", {"X": 1, "Y": 2, "F": 100})
        tilted.ToolAxis = FreeCAD.Vector(1, 0, 1)
        self.assertEqual(tilted.toGCode(), plain.toGCode())
        self.assertEqual(tilted.Parameters, plain.Parameters)
        self.assertEqual(tilted.Annotations, {})

    def test04(self):
        """A Toolpath keeps the Tool Axis of each command."""
        a = Path.Command("G1", {"X": 1})
        a.ToolAxis = FreeCAD.Vector(0, 0, 1)
        b = Path.Command("G1", {"X": 2})
        path = Path.Path([a, b])
        back = path.Commands
        self.assertCoincide(back[0].ToolAxis, FreeCAD.Vector(0, 0, 1), 1e-12)
        self.assertIsNone(back[1].ToolAxis)
        self.assertCoincide(path.copy().Commands[0].ToolAxis, FreeCAD.Vector(0, 0, 1), 1e-12)

    def test05(self):
        """setFromGCode() removes the Tool Axis, as it removes the annotations."""
        c = Path.Command("G1", {"X": 1})
        c.ToolAxis = FreeCAD.Vector(0, 0, 1)
        c.setFromGCode("G1 X5")
        self.assertIsNone(c.ToolAxis)

    def test10(self):
        """transform() rotates the Tool Axis and does not translate it."""
        c = Path.Command("G1", {"X": 1, "Y": 0, "Z": 0})
        c.ToolAxis = FreeCAD.Vector(0, 0, 1)
        placement = FreeCAD.Placement(
            FreeCAD.Vector(10, 20, 30), FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 90)
        )
        moved = c.transform(placement)
        self.assertCoincide(moved.ToolAxis, FreeCAD.Vector(0, -1, 0), 1e-9)
        self.assertRoughly(moved.ToolAxis.Length, 1.0, 1e-12)
        self.assertIsNone(Path.Command("G1", {"X": 1}).transform(placement).ToolAxis)

    def test11(self):
        """applyPlacementToPath() gives the Tool Axis in the target frame."""
        c = Path.Command("G1", {"X": 1, "Y": 2, "Z": 3})
        c.ToolAxis = FreeCAD.Vector(0, 0, 1)
        placement = FreeCAD.Placement(
            FreeCAD.Vector(5, 0, 0), FreeCAD.Rotation(FreeCAD.Vector(0, 1, 0), 90)
        )
        result = PathUtil.applyPlacementToPath(placement, Path.Path([c])).Commands[0]
        self.assertCoincide(result.ToolAxis, FreeCAD.Vector(1, 0, 0), 1e-9)
        expected = placement.multVec(FreeCAD.Vector(1, 2, 3))
        self.assertRoughly(result.Parameters["X"], expected.x, 1e-9)
        self.assertRoughly(result.Parameters["Y"], expected.y, 1e-9)
        self.assertRoughly(result.Parameters["Z"], expected.z, 1e-9)

    def test20(self):
        """The Tool Axis survives a document save and restore."""
        tilt = math.radians(30)
        commands = [
            Path.Command("G0", {"X": 0, "Y": 0, "Z": 5}),
            Path.Command("G1", {"X": 1, "Y": 0, "Z": 0, "F": 100}),
            Path.Command("G1", {"X": 2, "Y": 0, "Z": 0}),
        ]
        commands[1].ToolAxis = FreeCAD.Vector(0, 0, 1)
        commands[2].ToolAxis = FreeCAD.Vector(math.sin(tilt), 0, math.cos(tilt))

        folder = tempfile.mkdtemp()
        try:
            doc = FreeCAD.newDocument("ToolAxisSave")
            obj = doc.addObject("Path::Feature", "Stored")
            obj.Path = Path.Path(commands)
            fileName = os.path.join(folder, "axis.FCStd")
            doc.saveAs(fileName)
            FreeCAD.closeDocument(doc.Name)
            doc = FreeCAD.openDocument(fileName)
            try:
                back = doc.Stored.Path.Commands
                self.assertEqual(len(back), 3)
                self.assertIsNone(back[0].ToolAxis)
                self.assertCoincide(back[1].ToolAxis, FreeCAD.Vector(0, 0, 1), 1e-6)
                self.assertCoincide(
                    back[2].ToolAxis, FreeCAD.Vector(math.sin(tilt), 0, math.cos(tilt)), 1e-6
                )
                self.assertRoughly(back[2].Parameters["X"], 2.0, 1e-6)
            finally:
                FreeCAD.closeDocument(doc.Name)
        finally:
            shutil.rmtree(folder, ignore_errors=True)
