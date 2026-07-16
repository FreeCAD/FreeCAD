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

from types import SimpleNamespace
from unittest.mock import Mock

import Path
from Path.Op import Custom
from Path.Post.Processor import PostProcessor
from Path.Post.PostList import Postable
from Machine.models.machine import Machine, OutputUnits
from CAMTests import PathTestUtils
from CAMTests.PostTestMocks import MockJob, MockStock

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def _make_job(xmin=0.0, ymin=0.0, zmin=0.0, xmax=10.0, ymax=10.0, zmax=4.0):
    """A shared MockJob whose stock spans the requested bounding box."""
    job = MockJob()
    job.Stock = MockStock(xmin=xmin, xmax=xmax, ymin=ymin, ymax=ymax, zmin=zmin, zmax=zmax)
    return job


class TestPathCustomConverted(PathTestUtils.PathTestBase):
    """Test Custom through Processor.py's _convert_item_commands()"""

    @classmethod
    def _make_op(cls, gcode):
        """make the op and postable from lines of gcode or ["gcode",...] or [Path.Command...]"""
        op = Custom.Create(name="dumy", obj=Mock(), parentJob=cls.job).Proxy
        op.Active = True
        op.commandlist = []
        if isinstance(gcode, str):
            gcode = gcode.rstrip().split("\n")
        if isinstance(gcode, (list, tuple)):
            gcode = [(s if isinstance(s, str) else s.toGCode()) for s in gcode]
        custom_gcode = SimpleNamespace(Source="Text", Gcode=gcode)
        op.opExecute(custom_gcode)

        postable = Postable(
            label="test custom",
            item_type="operation",
            data={},
            path=Path.Path(op.commandlist),
            source=None,
        )
        return op, postable

    @classmethod
    def setUpClass(cls):

        cls.job = _make_job()
        cls.pp = PostProcessor(cls.job, "tooltip", "args", units="G21")

    def setUp(self):
        self.pp._machine = Machine()
        self.pp._machine.output.units = OutputUnits.METRIC
        self.pp._merge_machine_config()

    def test_supported(self):
        """Processor allows supported gcode"""
        _, postable = self._make_op("G1 X1")

        output = []
        self.pp._convert_item_commands(postable, output)
        self.assertEqual(
            "\n".join(output),
            """(Begin Custom)
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
            """(Begin Custom)
G666 X1.000
(End Custom)""",
        )
