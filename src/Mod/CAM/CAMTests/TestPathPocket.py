# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2024 FreeCAD Project                                    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

import FreeCAD
import Part
import Path.Op.Pocket as PathPocket
import Path.Main.Job as PathJob
import Path.Geom
from CAMTests.PathTestUtils import PathTestBase
from CAMTests.TestPathAdaptive import getGcodeMoves

if FreeCAD.GuiUp:
    import Path.Main.Gui.Job as PathJobGui
    import Path.Op.Gui.Pocket as PathPocketGui


def countOffsetLoops(commands, pocket_depth):
    """Count the number of distinct offset loops by counting plunge moves.

    A plunge move is a G1 command that changes Z to the cutting depth.
    We detect this by looking for G1 commands that go to pocket_depth
    after being at a higher Z (after a rapid move).

    Args:
        commands: List of G-code commands
        pocket_depth: The Z height where cutting occurs

    Returns:
        Number of distinct offset loops
    """
    plunge_count = 0
    prev_z = None
    for cmd in commands:
        params = cmd.Parameters
        if "Z" in params:
            current_z = params["Z"]
            # Check if this is a plunge: G1 command going to pocket depth
            # from a higher Z position
            if cmd.Name == "G1" and abs(current_z - pocket_depth) < 0.01:
                if prev_z is not None and prev_z > pocket_depth + 0.5:
                    plunge_count += 1
            prev_z = current_z
    return plunge_count


class TestPathPocket(PathTestBase):
    """Unit tests for the Pocket operation."""

    def setUp(self):
        """setUp()...
        This method is called prior to each test() method. Add code and objects here
        that are needed for multiple test() methods.
        """
        # Create a new document for each test
        self.doc = FreeCAD.newDocument("TestPocket")

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        FreeCAD.closeDocument(self.doc.Name)

    def createPocketOperation(self, part_obj, pocket_bottom_z, label, tool_diameter, **kwargs):
        """Create a pocket operation with the given parameters.

        Args:
            part_obj: The part object containing the geometry
            pocket_bottom_z: Z height of the pocket bottom
            label: Label for the pocket operation (job name will be "Job_<label>")
            tool_diameter: Diameter of the cutting tool
            **kwargs: Properties to set on the pocket operation
                     (e.g., StepOver=10, ClearingPattern="Offset", StartAt="Edge")

        Returns:
            The created pocket operation object
        """
        # Compute job name from label
        job_name = "Job_{}".format(label)

        # Create Job with the geometry
        job = PathJob.Create(job_name, [part_obj])
        if FreeCAD.GuiUp:
            job.ViewObject.Proxy = PathJobGui.ViewProvider(job.ViewObject)

        # Instantiate a Pocket operation
        pocket = PathPocket.Create("Pocket1")

        # Set tool diameter
        pocket.ToolController.Tool.Diameter = tool_diameter

        # Find all faces within tolerance of pocket bottom Z
        tolerance = 0.1
        pocket_faces = []

        for i, face in enumerate(part_obj.Shape.Faces):
            # Calculate distance from pocket bottom Z
            face_z = (face.BoundBox.ZMin + face.BoundBox.ZMax) / 2.0
            distance = abs(face_z - pocket_bottom_z)
            if distance < tolerance:
                pocket_faces.append("Face{}".format(i + 1))

        if not pocket_faces:
            raise ValueError("Could not find faces near Z={}".format(pocket_bottom_z))

        pocket.Base = [(part_obj, pocket_faces)]
        pocket.Label = label

        # Set any properties from kwargs
        for key, value in kwargs.items():
            if hasattr(pocket, key):
                setattr(pocket, key, value)
            else:
                FreeCAD.Console.PrintWarning(
                    "Property '{}' not found on pocket operation\n".format(key)
                )

        _addViewProvider(pocket)

        # Generate the toolpath
        pocket.recompute()

        return pocket

    # Unit tests
    def test_pocket_square_offset(self):
        """test_pocket_square_offset() Verify pocket operation with square geometry and offset clearing."""

        # Test geometry constants
        pocket_size = 50.0
        box_margin = 10.0
        outer_box_size = pocket_size + box_margin
        outer_box_height = 20.0
        pocket_depth_amount = 1.0
        pocket_bottom_z = outer_box_height - pocket_depth_amount
        pocket_offset_xy = (outer_box_size - pocket_size) / 2.0

        # Tool and operation constants
        tool_diameter = 5.0
        stepover_percent = 10

        # Create a box with a square pocket (extrusion with cutout)
        # Pocket is 1mm deep, from Z=19 to Z=20
        outer = Part.makeBox(outer_box_size, outer_box_size, outer_box_height)
        inner = Part.makeBox(
            pocket_size,
            pocket_size,
            pocket_depth_amount,
            FreeCAD.Vector(pocket_offset_xy, pocket_offset_xy, pocket_bottom_z),
        )
        pocket_solid = outer.cut(inner)

        part_obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "PocketPart")
        part_obj.Shape = pocket_solid

        # Create pocket operation with specified parameters
        pocket = self.createPocketOperation(
            part_obj,
            pocket_bottom_z,
            "pocket_square_offset",
            tool_diameter,
            StepOver=stepover_percent,
            ClearingPattern="Offset",
            StartAt="Edge",
        )

        # Count offset loops using two different methods
        # Actual: Count plunge moves from generated G-code
        actual_num_loops = countOffsetLoops(pocket.Path.Commands, pocket_bottom_z)

        # Expected: Calculate from geometry and stepover
        # Each offset loop moves inward by stepover distance on each side
        # Available clearance = (pocket_size - tool_diameter) / 2
        # Number of loops = available_clearance / stepover_distance
        stepover_distance = tool_diameter * (stepover_percent / 100.0)
        available_clearance = (pocket_size - tool_diameter) / 2.0
        expected_num_loops = int(available_clearance / stepover_distance)

        # Verify actual loop count matches expected loop count
        self.assertEqual(actual_num_loops, expected_num_loops)


def _addViewProvider(pocketOp):
    if FreeCAD.GuiUp:
        PathOpGui = PathPocketGui.PathOpGui
        cmdRes = PathPocketGui.Command.res
        pocketOp.ViewObject.Proxy = PathOpGui.ViewProvider(pocketOp.ViewObject, cmdRes)
