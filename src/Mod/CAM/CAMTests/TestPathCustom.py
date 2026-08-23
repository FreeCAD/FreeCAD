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

"""Unit tests for the CAM Custom operation (Path.Op.Custom).

These tests deliberately avoid loading documents from disk and avoid building
a full Job/Stock/ToolController graph. The operation is small
"""

import FreeCAD
import Path
import Path.Main.Job as PathJob
from Path.Op import Custom
from Path.Base.MachineState import MachineState
from Path.Post.Processor import PostProcessor
from Path.Post.PostList import Postable
from Machine.models.machine import Machine, OutputUnits
from CAMTests import PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TestPathCustomConverted(PathTestUtils.PathTestBase):
    """Test Custom through Processor.py's _convert_item_commands()"""

    @classmethod
    def _make_op(cls, gcode, process=True):
        """make the op and postable from lines of gcode or ["gcode",...] or [Path.Command...]"""
        if not cls.job.Operations.Group:
            op = Custom.Create(name="Custom", parentJob=cls.job)
        else:
            op = cls.job.Operations.Group[0]
        op.Active = True
        if isinstance(gcode, str):
            gcode = gcode.rstrip().split("\n")
        if isinstance(gcode, (list, tuple)):
            gcode = [(s if isinstance(s, str) else s.toGCode()) for s in gcode]
        op.Source = "Text"
        op.Gcode = gcode
        op.PostProcessOutput = process
        op.recompute()

        postable = Postable(
            label="test custom",
            item_type="operation",
            data={},
            path=op.Path,
            source=None,
        )
        return op, postable

    @classmethod
    def setUpClass(cls):
        cls.doc = FreeCAD.newDocument("test")
        box = cls.doc.addObject("Part::Box", "TestBox")
        cls.job = PathJob.Create("Job", [box])
        cls.job.Machine = "TestMachine"
        cls.pp = PostProcessor(cls.job, "tooltip", "args", units="G21")

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def setUp(self):
        self.pp._machine = Machine()
        self.pp._machine.name = "TestMachine"
        self.pp._machine.output.units = OutputUnits.METRIC
        self.pp.apply_configuration_bundle()
        self.pp.machine_state = MachineState()

    def test_supported(self):
        """Processor allows supported gcode"""
        _, postable = self._make_op("G1 X1")

        output = []
        self.pp._convert_item_commands(postable, output)
        self.assertEqual(
            "\n".join(output),
            """(Custom)
(Begin Custom)
G1 X1.000
(End Custom)""",
        )

    def test_unsupported(self):
        """Processor allows unsupported gcode"""
        _, postable = self._make_op("G666 X1")

        output = []
        self.pp._convert_item_commands(postable, output)
        self.assertEqual(
            "\n".join(output),
            """(Custom)
(Begin Custom)
G666 X1.000
(End Custom)""",
        )

    def test_as_is_one_line(self):
        """Processor allows add lines without processing"""
        _, postable = self._make_op("! G68 R#100 X#150 Y#151")

        output = []
        self.pp._convert_item_commands(postable, output)
        self.assertEqual(
            "\n".join(output),
            """(Custom)
(Begin Custom)
 G68 R#100 X#150 Y#151
(End Custom)""",
        )

    def test_as_is_all_line(self):
        """Processor allows add lines without processing"""
        _, postable = self._make_op(" ;EX1: REFERENCE LEFT 0\n  G68 R#100 X#150 Y#151", False)

        output = []
        self.pp._convert_item_commands(postable, output)
        self.assertEqual(
            "\n".join(output),
            """(Custom)
(Begin Custom)
 ;EX1: REFERENCE LEFT 0
  G68 R#100 X#150 Y#151
(End Custom)""",
        )
