# ***************************************************************************
# *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
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

"""Mock objects for postprocessor testing.

This module provides mock objects that simulate FreeCAD CAM job structure
without requiring disk I/O or loading actual FreeCAD documents.
"""

import Path


class MockTool:
    def __init__(self):
        self.ShapeName = "endmill"


class MockToolController:
    """Mock ToolController for operations."""

    def __init__(
        self,
        tool_number=1,
        label="TC: Default Tool",
        spindle_speed=1000,
        spindle_dir="Forward",
    ):
        self.Tool = MockTool()
        self.ToolNumber = tool_number
        self.Label = label
        self.SpindleSpeed = spindle_speed
        self.SpindleDir = spindle_dir
        self.Name = f"TC{tool_number}"

        # Create a simple path with tool change commands
        self.Path = Path.Path()
        self.Path.addCommands(
            [Path.Command(f"M6 T{tool_number}"), Path.Command(f"M3 S{spindle_speed}")]
        )

    def InList(self):
        return []


class MockOperation:
    """Mock Operation object for testing postprocessors."""

    def __init__(self, name="Operation", label=None, tool_controller=None, active=True):
        self.Name = name
        self.Label = label or name
        self.Active = active
        self.ToolController = tool_controller

        # Create an empty path by default
        self.Path = Path.Path()

    def InList(self):
        """Mock InList - operations belong to a job."""
        return []


class MockStock:
    """Mock Stock object with BoundBox."""

    def __init__(self, xmin=0.0, xmax=100.0, ymin=0.0, ymax=100.0, zmin=0.0, zmax=10.0):
        self.Shape = type(
            "obj",
            (object,),
            {
                "BoundBox": type(
                    "obj",
                    (object,),
                    {
                        "XMin": xmin,
                        "XMax": xmax,
                        "YMin": ymin,
                        "YMax": ymax,
                        "ZMin": zmin,
                        "ZMax": zmax,
                    },
                )()
            },
        )()


class MockSetupSheet:
    """Mock SetupSheet object."""

    def __init__(self, clearance_height=5.0, safe_height=3.0):
        self.ClearanceHeightOffset = type("obj", (object,), {"Value": clearance_height})()
        self.SafeHeightOffset = type("obj", (object,), {"Value": safe_height})()


class MockJob:
    """Mock Job object for testing postprocessors."""

    def __init__(self):
        # Create mock Stock with BoundBox
        self.Stock = MockStock()

        # Create mock SetupSheet
        self.SetupSheet = MockSetupSheet()

        # Create mock Operations group
        self.Operations = type("obj", (object,), {"Group": []})()

        # Create mock Tools group
        self.Tools = type("obj", (object,), {"Group": []})()

        # Create mock Model group
        self.Model = type("obj", (object,), {"Group": []})()

        # Basic properties
        self.Label = "MockJob"
        self.PostProcessor = ""
        self.PostProcessorArgs = ""
        self.PostProcessorOutputFile = ""
        self.Fixtures = ["G54"]
        self.OrderOutputBy = "Tool"
        self.SplitOutput = False

    def InList(self):
        """Mock InList for fixture setup."""
        return []


def create_default_job_with_operation():
    """Create a mock job with a default tool controller and operation.

    This is a convenience function for common test scenarios.
    Returns: (job, operation, tool_controller)
    """
    job = MockJob()

    # Create default tool controller
    tc = MockToolController(tool_number=1, label="TC: Default Tool", spindle_speed=1000)
    job.Tools.Group = [tc]

    # Create default operation
    op = MockOperation(name="Profile", label="Profile", tool_controller=tc)
    job.Operations.Group = [op]

    return job, op, tc
