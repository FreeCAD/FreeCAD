# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 David kaufman <davidgilkaufman@gmail.com>
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
import Path.Geom as PathGeom
import Path.Op.MillFace as PathMillFace
import Path.Op.Pocket as PathPocket
import Path.Main.Job as PathJob
import Path.Tool.Controller as PathToolController
import Constants as CAMConstants
from CAMTests.PathTestUtils import PathTestBase

import math

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


def getLoopDirection(commands, pocket_depth, loop_index=0):
    """Determine if a specific offset loop is clockwise or counter-clockwise.

    Uses the shoelace formula to calculate signed area from XY moves.
    Positive area = CCW, negative area = CW.

    Args:
        commands: List of G-code commands
        pocket_depth: The Z height where cutting occurs
        loop_index: Which loop to check (0 = first loop)

    Returns:
        "CCW" or "CW", or None if loop not found
    """
    # Find the loop by counting plunges
    current_loop = -1
    prev_z = None
    loop_points = []
    current_pos = FreeCAD.Vector(0, 0, 0)
    in_target_loop = False

    for cmd in commands:
        params = cmd.Parameters

        # Update current position
        if "X" in params:
            current_pos.x = params["X"]
        if "Y" in params:
            current_pos.y = params["Y"]
        if "Z" in params:
            current_z = params["Z"]
            current_pos.z = current_z

            # Check for plunge (start of new loop)
            if cmd.Name == "G1" and abs(current_z - pocket_depth) < 0.01:
                if prev_z is not None and prev_z > pocket_depth + 0.5:
                    current_loop += 1
                    if current_loop == loop_index:
                        in_target_loop = True
                        loop_points = [FreeCAD.Vector(current_pos.x, current_pos.y, 0)]
                    elif current_loop > loop_index:
                        # We've passed the target loop, stop collecting
                        break

            # Check for retract (end of loop)
            if (
                prev_z is not None
                and abs(prev_z - pocket_depth) < 0.01
                and current_z > pocket_depth + 0.5
            ):
                if in_target_loop:
                    break

            prev_z = current_z

        # Collect points for the target loop (only G1 moves at cutting depth)
        if in_target_loop and cmd.Name in ("G1", "G2", "G3"):
            if abs(current_pos.z - pocket_depth) < 0.01:
                loop_points.append(FreeCAD.Vector(current_pos.x, current_pos.y, 0))

    if len(loop_points) < 3:
        return None

    # Calculate signed area using shoelace formula
    signed_area = 0.0
    for i in range(len(loop_points) - 1):
        p1 = loop_points[i]
        p2 = loop_points[i + 1]
        signed_area += (p2.x - p1.x) * (p2.y + p1.y)

    return "CW" if signed_area > 0 else "CCW"


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

    def createPocketOperation(
        self, part_obj, pocket_bottom_z, label, tool_diameter, job=None, **kwargs
    ):
        """Create a pocket operation with the given parameters.

        Args:
            part_obj: The part object containing the geometry
            pocket_bottom_z: Z height of the pocket bottom
            label: Label for the pocket operation (job name will be "Job_<label>")
            tool_diameter: Diameter of the cutting tool
            job: Optional existing job to add the operation to. If None, a new job is created.
            **kwargs: Properties to set on the pocket operation
                     (e.g., StepOver=10, ClearingPattern="Offset", StartAt="Edge")

        Returns:
            The created pocket operation object
        """
        if job is None:
            job = PathJob.Create("Job_{}".format(label), [part_obj])
            if FreeCAD.GuiUp:
                job.ViewObject.Proxy = PathJobGui.ViewProvider(job.ViewObject)

        # Instantiate a Pocket operation
        pocket = PathPocket.Create(label, parentJob=job)

        # Create a dedicated tool controller for this operation and set its diameter
        tc = PathToolController.Create(name="TC: {}mm Endmill".format(tool_diameter))
        job.Proxy.addToolController(tc)
        tc.Tool.Diameter = tool_diameter
        pocket.ToolController = tc

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
                pocket.setExpression(key, None)
                setattr(pocket, key, value)
            else:
                FreeCAD.Console.PrintWarning(
                    "Property '{}' not found on pocket operation\n".format(key)
                )

        _addViewProvider(pocket)

        # Generate the toolpath
        # Note: PathPocket.Create() with parentJob already adds the operation
        # to job.Operations.Group via PathUtils.addToJob, so no need to call
        # job.addObject() here (which would move it out of Operations).
        self.doc.recompute()

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

        # Check that first offset loop is counterclockwise
        first_loop_direction = getLoopDirection(pocket.Path.Commands, pocket_bottom_z, 0)
        self.assertEqual(first_loop_direction, "CCW")

        # Expected: Calculate from geometry and stepover
        # Each offset loop moves inward by stepover distance on each side
        # Available clearance = (pocket_size - tool_diameter) / 2
        # Number of loops = available_clearance / stepover_distance
        stepover_distance = tool_diameter * (stepover_percent / 100.0)
        available_clearance = (pocket_size - tool_diameter) / 2.0
        expected_num_loops = int(available_clearance / stepover_distance)

        # Verify actual loop count matches expected loop count
        self.assertEqual(actual_num_loops, expected_num_loops)

    def test_pocket_pointy_triangle_offset(self):
        """test_pocket_pointy_triangle_offset() Verify pocket operation with pointy triangular geometry and offset clearing."""

        # Test geometry constants
        triangle_base = 15.0
        triangle_height = 100.0
        box_margin = 10.0
        outer_box_size = max(triangle_base, triangle_height) + box_margin
        outer_box_height = 20.0
        pocket_depth_amount = 1.0
        pocket_bottom_z = outer_box_height - pocket_depth_amount

        # Tool and operation constants
        tool_diameter = 5.0
        stepover_percent = 90

        # Create a box with a triangular pocket (extrusion with cutout)
        # Pocket is 1mm deep, from Z=19 to Z=20
        outer = Part.makeBox(outer_box_size, outer_box_size, outer_box_height)

        # Create triangle centered in the box
        center_x = outer_box_size / 2.0
        center_y = outer_box_size / 2.0

        # Create a pointy triangle (isosceles with apex pointing up)
        # Base centered at bottom, apex at top
        base_y = center_y - triangle_height / 2.0
        apex_y = center_y + triangle_height / 2.0

        v1 = FreeCAD.Vector(center_x - triangle_base / 2.0, base_y, pocket_bottom_z)
        v2 = FreeCAD.Vector(center_x + triangle_base / 2.0, base_y, pocket_bottom_z)
        v3 = FreeCAD.Vector(center_x, apex_y, pocket_bottom_z)

        # Create triangle wire and extrude
        triangle_wire = Part.makePolygon([v1, v2, v3, v1])
        triangle_face = Part.Face(triangle_wire)
        inner = triangle_face.extrude(FreeCAD.Vector(0, 0, pocket_depth_amount))

        pocket_solid = outer.cut(inner)

        part_obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "TrianglePart")
        part_obj.Shape = pocket_solid

        # Calculate max expected loops based on base width
        stepover_distance = tool_diameter * (stepover_percent / 100.0)
        available_clearance = (triangle_base - tool_diameter) / 2.0
        max_expected_loops = int(available_clearance / stepover_distance)

        # Create pocket operation without ForceMaxStepOver
        pocket = self.createPocketOperation(
            part_obj,
            pocket_bottom_z,
            "pocket_pointy_triangle_offset",
            tool_diameter,
            StepOver=stepover_percent,
            ClearingPattern="Offset",
            StartAt="Edge",
        )

        # Count offset loops from generated G-code
        actual_num_loops = countOffsetLoops(pocket.Path.Commands, pocket_bottom_z)

        # Check that offset loops are clockwise (climb cutting for this pocket)
        first_loop_direction = getLoopDirection(pocket.Path.Commands, pocket_bottom_z, 0)
        self.assertEqual(first_loop_direction, "CCW")

        # Without ForceMaxStepOver, pocket should generate more loops than base-calculated max
        # to ensure full area coverage
        self.assertGreater(actual_num_loops, max_expected_loops)

        # Create second pocket with ForceMaxStepOver=True
        # With the flag set, algorithm should use max stepover even if not all area is cleared
        # (This is the existing behavior and should pass)
        pocket_forced = self.createPocketOperation(
            part_obj,
            pocket_bottom_z,
            "pocket_pointy_triangle_forced",
            tool_diameter,
            StepOver=stepover_percent,
            ClearingPattern="Offset",
            StartAt="Edge",
            ForceMaxStepOver=True,
        )

        # Count loops for forced max stepover pocket
        actual_num_loops_forced = countOffsetLoops(pocket_forced.Path.Commands, pocket_bottom_z)

        # With ForceMaxStepOver=True, should be close to max expected (slightly less, because of
        # narrowing geometry)
        self.assertGreaterEqual(actual_num_loops_forced, max_expected_loops - 1)
        self.assertLessEqual(actual_num_loops_forced, max_expected_loops)

    def test_pocket_square_line(self):
        """test_pocket_square_line() Verify pocket operation with Line clearing pattern."""

        # Test geometry constants
        pocket_width = 50.0
        pocket_height = 30.0
        box_margin = 10.0
        outer_box_width = pocket_width + box_margin
        outer_box_height_xy = pocket_height + box_margin
        outer_box_height_z = 20.0
        pocket_depth_amount = 1.0
        pocket_bottom_z = outer_box_height_z - pocket_depth_amount

        # Tool and operation constants
        tool_diameter = 5.0
        stepover_percent = 50  # 50% stepover = 2.5mm spacing
        line_angle = 0  # Horizontal lines (parallel to X-axis)

        # Create a box with a rectangular pocket
        # Pocket is 1mm deep, from Z=19 to Z=20
        outer = Part.makeBox(outer_box_width, outer_box_height_xy, outer_box_height_z)
        pocket_offset_x = (outer_box_width - pocket_width) / 2.0
        pocket_offset_y = (outer_box_height_xy - pocket_height) / 2.0
        inner = Part.makeBox(
            pocket_width,
            pocket_height,
            pocket_depth_amount,
            FreeCAD.Vector(pocket_offset_x, pocket_offset_y, pocket_bottom_z),
        )
        pocket_solid = outer.cut(inner)

        part_obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "LinePocketPart")
        part_obj.Shape = pocket_solid

        # Create pocket operation with Line clearing pattern
        pocket = self.createPocketOperation(
            part_obj,
            pocket_bottom_z,
            "pocket_square_line",
            tool_diameter,
            StepOver=stepover_percent,
            ClearingPattern="Line",
            StartAt="Edge",
            Angle=line_angle,
        )

        # Count the number of distinct line passes
        # For Line pattern, we expect parallel lines spaced by stepover distance
        # Each line is a separate cutting pass
        stepover_distance = tool_diameter * (stepover_percent / 100.0)

        # Calculate expected number of lines based on pocket height and stepover
        # The lines run parallel to the X-axis (angle=0), so spacing is in Y direction
        # Account for tool radius on each side
        effective_height = pocket_height - tool_diameter
        expected_num_lines = int(effective_height / stepover_distance) + 1

        # Count actual cutting passes by looking for Y-coordinate changes in G1 moves
        # at cutting depth
        y_positions = set()
        for cmd in pocket.Path.Commands:
            params = cmd.Parameters
            if cmd.Name == "G1" and "Z" in params:
                z = params["Z"]
                # If we're at cutting depth and have Y coordinate
                if abs(z - pocket_bottom_z) < 0.01 and "Y" in params:
                    y_pos = round(params["Y"], 2)  # Round to avoid floating point issues
                    y_positions.add(y_pos)

        actual_num_lines = len(y_positions)

        # Verify the number of line passes is close to expected
        # Allow ±1 line tolerance due to boundary conditions
        self.assertGreaterEqual(
            actual_num_lines,
            expected_num_lines - 1,
            f"Line pocket should have at least {expected_num_lines - 1} lines, got {actual_num_lines}",
        )
        self.assertLessEqual(
            actual_num_lines,
            expected_num_lines + 1,
            f"Line pocket should have at most {expected_num_lines + 1} lines, got {actual_num_lines}",
        )

    def test_pocket_square_grid(self):
        """test_pocket_square_grid() Verify pocket operation with Grid clearing pattern."""

        # Test geometry constants
        pocket_width = 50.0
        pocket_height = 30.0
        box_margin = 10.0
        outer_box_width = pocket_width + box_margin
        outer_box_height_xy = pocket_height + box_margin
        outer_box_height_z = 20.0
        pocket_depth_amount = 1.0
        pocket_bottom_z = outer_box_height_z - pocket_depth_amount

        # Tool and operation constants
        tool_diameter = 5.0
        stepover_percent = 50  # 50% stepover = 2.5mm spacing
        grid_angle = 0  # Grid aligned with X/Y axes

        # Create a box with a rectangular pocket
        # Pocket is 1mm deep, from Z=19 to Z=20
        outer = Part.makeBox(outer_box_width, outer_box_height_xy, outer_box_height_z)
        pocket_offset_x = (outer_box_width - pocket_width) / 2.0
        pocket_offset_y = (outer_box_height_xy - pocket_height) / 2.0
        inner = Part.makeBox(
            pocket_width,
            pocket_height,
            pocket_depth_amount,
            FreeCAD.Vector(pocket_offset_x, pocket_offset_y, pocket_bottom_z),
        )
        pocket_solid = outer.cut(inner)

        part_obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "GridPocketPart")
        part_obj.Shape = pocket_solid

        # Create pocket operation with Grid clearing pattern
        pocket = self.createPocketOperation(
            part_obj,
            pocket_bottom_z,
            "pocket_square_grid",
            tool_diameter,
            StepOver=stepover_percent,
            ClearingPattern="Grid",
            StartAt="Edge",
            Angle=grid_angle,
        )

        # Count the number of distinct line passes in both X and Y directions
        # For Grid pattern, we expect parallel lines in both directions
        stepover_distance = tool_diameter * (stepover_percent / 100.0)

        # I don't fully understand but grid seems to go both directions, X+ and X-,
        # resulting in double density
        stepover_distance /= 2

        # Calculate expected number of lines in each direction
        # X-direction lines (parallel to X-axis, spaced in Y)
        effective_height = pocket_height - tool_diameter
        expected_num_x_lines = int(effective_height / stepover_distance) + 1

        # Y-direction lines (parallel to Y-axis, spaced in X)
        effective_width = pocket_width - tool_diameter
        expected_num_y_lines = int(effective_width / stepover_distance) + 1

        # Count actual cutting passes by looking for coordinate changes in G1 moves
        # at cutting depth
        x_positions = set()
        y_positions = set()
        for cmd in pocket.Path.Commands:
            params = cmd.Parameters
            if cmd.Name == "G1" and "Z" in params:
                z = params["Z"]
                # If we're at cutting depth
                if abs(z - pocket_bottom_z) < 0.01:
                    if "X" in params:
                        x_pos = round(params["X"], 2)  # Round to avoid floating point issues
                        x_positions.add(x_pos)
                    if "Y" in params:
                        y_pos = round(params["Y"], 2)
                        y_positions.add(y_pos)

        actual_num_x_lines = len(y_positions)  # Lines parallel to X have constant Y
        actual_num_y_lines = len(x_positions)  # Lines parallel to Y have constant X

        # Verify the number of line passes in X direction
        # Allow ±1 line tolerance due to boundary conditions
        self.assertGreaterEqual(
            actual_num_x_lines,
            expected_num_x_lines - 1,
            f"Grid pocket should have at least {expected_num_x_lines - 1} X-direction lines, got {actual_num_x_lines}",
        )
        self.assertLessEqual(
            actual_num_x_lines,
            expected_num_x_lines + 1,
            f"Grid pocket should have at most {expected_num_x_lines + 1} X-direction lines, got {actual_num_x_lines}",
        )

        # Verify the number of line passes in Y direction
        self.assertGreaterEqual(
            actual_num_y_lines,
            expected_num_y_lines - 1,
            f"Grid pocket should have at least {expected_num_y_lines - 1} Y-direction lines, got {actual_num_y_lines}",
        )
        self.assertLessEqual(
            actual_num_y_lines,
            expected_num_y_lines + 1,
            f"Grid pocket should have at most {expected_num_y_lines + 1} Y-direction lines, got {actual_num_y_lines}",
        )

    def test_pocket_square_zigzag(self):
        """test_pocket_square_zigzag() Verify pocket operation with ZigZag clearing pattern."""

        pocket_width = 50.0
        pocket_height = 33.0
        box_margin = 10.0
        outer_box_width = pocket_width + box_margin
        outer_box_height_xy = pocket_height + box_margin
        outer_box_height_z = 20.0
        pocket_depth_amount = 1.0
        pocket_bottom_z = outer_box_height_z - pocket_depth_amount

        tool_diameter = 5.0
        stepover_percent = 50

        outer = Part.makeBox(outer_box_width, outer_box_height_xy, outer_box_height_z)
        pocket_offset_x = (outer_box_width - pocket_width) / 2.0
        pocket_offset_y = (outer_box_height_xy - pocket_height) / 2.0
        inner = Part.makeBox(
            pocket_width,
            pocket_height,
            pocket_depth_amount,
            FreeCAD.Vector(pocket_offset_x, pocket_offset_y, pocket_bottom_z),
        )
        pocket_solid = outer.cut(inner)

        part_obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "ZigZagPocketPart")
        part_obj.Shape = pocket_solid

        pocket = self.createPocketOperation(
            part_obj,
            pocket_bottom_z,
            "pocket_square_zigzag",
            tool_diameter,
            StepOver=stepover_percent,
            ClearingPattern="ZigZagOffset",
            StartAt="Edge",
            Angle=0,
        )

        stepover_distance = tool_diameter * (stepover_percent / 100.0)
        effective_height = pocket_height - tool_diameter
        expected_num_horizontal_lines = int(math.ceil(effective_height / stepover_distance)) + 1
        expected_num_vertical_lines = expected_num_horizontal_lines - 1

        # Collect G1 moves at cutting depth
        cutting_moves = []
        for cmd in pocket.Path.Commands:
            params = cmd.Parameters
            if cmd.Name == "G1" and "Z" in params and abs(params["Z"] - pocket_bottom_z) < 0.01:
                cutting_moves.append(params)

        actual_num_horizontal_lines = 0
        actual_num_vertical_lines = 0
        actual_num_diagonal = 0
        last_direction = None
        pos = {"X": 0.0, "Y": 0.0}
        for curr in cutting_moves:
            x = curr.get("X", pos["X"])
            y = curr.get("Y", pos["Y"])
            dx = abs(x - pos["X"])
            dy = abs(y - pos["Y"])
            pos["X"] = x
            pos["Y"] = y
            if dy < 0.01 and dx > 0.01:
                direction = "horizontal"
            elif dx < 0.01 and dy > 0.01:
                direction = "vertical"
            else:
                direction = "diagonal"

            if last_direction is not None and direction != last_direction:
                if direction == "horizontal":
                    actual_num_horizontal_lines += 1
                if direction == "vertical":
                    actual_num_vertical_lines += 1
                if direction == "diagonal":
                    actual_num_diagonal += 1
            last_direction = direction

        self.assertEqual(actual_num_diagonal, 0)
        self.assertGreaterEqual(actual_num_horizontal_lines, expected_num_horizontal_lines - 1)
        self.assertLessEqual(actual_num_horizontal_lines, expected_num_horizontal_lines + 1)
        self.assertGreaterEqual(actual_num_vertical_lines, expected_num_vertical_lines - 1)
        self.assertLessEqual(actual_num_vertical_lines, expected_num_vertical_lines + 1)

    def test_pocket_rest_machining(self):
        """test_pocket_rest_machining() Verify pocket rest machining clears remaining material after large tool pass."""

        pocket_width = 50.0
        pocket_height = 33.0
        box_margin = 10.0
        outer_box_width = pocket_width + box_margin
        outer_box_height_xy = pocket_height + box_margin
        outer_box_height_z = 20.0
        pocket_depth_amount = 1.0
        pocket_bottom_z = outer_box_height_z - pocket_depth_amount
        large_tool_diameter = 5.0
        small_tool_diameter = 1.0

        outer = Part.makeBox(outer_box_width, outer_box_height_xy, outer_box_height_z)
        pocket_offset_x = (outer_box_width - pocket_width) / 2.0
        pocket_offset_y = (outer_box_height_xy - pocket_height) / 2.0
        inner = Part.makeBox(
            pocket_width,
            pocket_height,
            pocket_depth_amount,
            FreeCAD.Vector(pocket_offset_x, pocket_offset_y, pocket_bottom_z),
        )
        pocket_solid = outer.cut(inner)

        part_obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "RestMachiningPocketPart")
        part_obj.Shape = pocket_solid

        job = PathJob.Create("Job_pocket_rest", [part_obj])
        if FreeCAD.GuiUp:
            job.ViewObject.Proxy = PathJobGui.ViewProvider(job.ViewObject)

        # Pocket with large tool
        self.createPocketOperation(
            part_obj,
            pocket_bottom_z,
            "pocket_rest_large",
            tool_diameter=large_tool_diameter,
            job=job,
            StepOver=50,
            ClearingPattern="ZigZagOffset",
            StartAt="Edge",
            Angle=0,
        )

        # Pocket with small tool; rest machining
        pocket_small = self.createPocketOperation(
            part_obj,
            pocket_bottom_z,
            "pocket_rest_small",
            tool_diameter=small_tool_diameter,
            job=job,
            StepOver=50,
            StepDown=5,
            ClearingPattern="ZigZagOffset",
            StartAt="Edge",
            Angle=0,
            UseRestMachining=True,
        )

        margin = large_tool_diameter + small_tool_diameter

        pocket_left = pocket_offset_x
        pocket_right = pocket_offset_x + pocket_width
        pocket_front = pocket_offset_y
        pocket_back = pocket_offset_y + pocket_height

        corners = [
            (pocket_left, pocket_front),
            (pocket_right, pocket_front),
            (pocket_left, pocket_back),
            (pocket_right, pocket_back),
        ]
        corners_reached = [False] * 4

        pos = {"X": 0.0, "Y": 0.0}
        for cmd in pocket_small.Path.Commands:
            params = cmd.Parameters
            pos["X"] = params.get("X", pos["X"])
            pos["Y"] = params.get("Y", pos["Y"])

            if cmd.Name not in CAMConstants.GCODE_MOVE_MILL:
                continue
            if "Z" not in params or abs(params["Z"] - pocket_bottom_z) > 0.01:
                continue

            near_edge = (
                pos["X"] - pocket_left < margin
                or pocket_right - pos["X"] < margin
                or pos["Y"] - pocket_front < margin
                or pocket_back - pos["Y"] < margin
            )
            self.assertTrue(near_edge)

            for i, (cx, cy) in enumerate(corners):
                if math.hypot(pos["X"] - cx, pos["Y"] - cy) < margin:
                    corners_reached[i] = True

        for i, (cx, cy) in enumerate(corners):
            self.assertTrue(
                corners_reached[i],
                f"Rest machining never cut within {margin}mm of corner ({cx}, {cy})",
            )

    def test_regression_arka(self):
        """Regression test from #32769 arka_.FCStd"""

        # Create the star wire: regular 5-point star, top point on +Y, Inner points at r=25, top edges horizontal
        pocket_z = 2
        r_inner = 25.0
        r_outer = r_inner * math.cos(2 * math.pi / 10) / math.cos(2 * math.pi / 5)
        star_pts = []
        for k in range(5):
            a_outer = math.radians(90 + 360 / 5 * k)
            a_inner = math.radians(90 + 360 / 10 + 360 / 5 * k)
            p = lambda a, r: FreeCAD.Vector(r * math.cos(a), r * math.sin(a) - 8, pocket_z)
            star_pts.append(p(a_outer, r_outer))
            star_pts.append(p(a_inner, r_inner))
        star_lines = [
            Part.LineSegment(star_pts[i], star_pts[(i + 1) % len(star_pts)])
            for i in range(len(star_pts))
        ]
        star_wire = Part.Wire([line.toShape() for line in star_lines])

        # Extrude and fillet
        star_solid = Part.Face(star_wire).extrude(FreeCAD.Vector(0, 0, 50))

        def vertical_edges_at(shape, points):
            return [
                e
                for e in shape.Edges
                if PathGeom.isVertical(e)
                and any(PathGeom.pointsCoincide(v.Point, p) for v in e.Vertexes for p in points)
            ]

        outer_edges = vertical_edges_at(star_solid, star_pts[0::2])
        star_solid = star_solid.makeFillet(10.0, outer_edges)

        inner_edges = vertical_edges_at(star_solid, star_pts[1::2])
        star_solid = star_solid.makeFillet(3.0, inner_edges)

        # Final pocket geometry: subtract from a box
        box = Part.makeBox(100, 100, 10, FreeCAD.Vector(-50, -50, 0))
        pocket_solid = box.cut(star_solid).removeSplitter()
        part_obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "ArkaPocketPart")
        part_obj.Shape = pocket_solid

        # Create pocket
        pocket = self.createPocketOperation(
            part_obj,
            pocket_z,
            "regression_arka",
            10.0,
            StepOver=50,
            ClearingPattern="Offset",
            CutMode="Climb",
            StartAt="Center",
        )

        # Assert that the pocket computes without error
        self.assertTrue(pocket.isValid(), pocket.getStatusString())

    def test_regression_mill_face(self):
        """Regression test from #32769 cam_mill_face_issue.FCStd"""

        # Reproduce the wire bsplines as (poles, weights, knots, multiplicities)
        # fmt: off
        edge_data = [
            ([(-302.9021928196, -15.46394950774, 0), (-302.2877402151, -15.47619766776, 0), (-301.6740810196, -15.44266540774, 0)], [1.0, 0.99930593297, 1.0], [4.692458170302, 4.766977823158], [3, 3]),
            ([(-301.6740810196, -15.44266540774, 0), (-295.0790450672, -15.24183923915, 0), (-289.2470299196, -12.15599580774, 0)], [1.0, 0.974094332023, 1.0], [4.742830683991, 5.199061307779], [3, 3]),
            ([(-289.2470299196, -12.15599580774, 0), (-287.2357929271, -11.07364310224, 0), (-285.4478010196, -9.652500407747, 0)], [1.0, 0.996047612489, 1.0], [5.206090931582, 5.383967154155], [3, 3]),
            ([(-285.4478010196, -9.652500407747, 0), (-284.4457607777, -8.903767217826, 0), (-283.4908194196, -8.095823507802, 0)], [1.0, 0.999542599935, 1.0], [5.354101264565, 5.414594896552], [3, 3]),
            ([(-283.4908194196, -8.095823507747, 0), (-279.3749326438, -4.821116197088, 0), (-282.6493965685, -0.705035788754, 0), (-285.9238604932, 3.41104461958, 0), (-290.0401345196, 0.136824092253, 0)], [1.0, 0.707127687162, 1.0, 0.707127687162, 1.0], [5.384457268871, 6.955194463763, 8.525931658655], [3, 2, 3]),
            ([(-290.0401345196, 0.136824092253, 0), (-291.2585640642, -0.865526499565, 0), (-292.5175566196, -1.816428707747, 0)], [1.0, 0.999784437597, 1.0], [4.023984175791, 4.065512012003], [3, 3]),
            ([(-292.5175566196, -1.816428707747, 0), (-296.3696402801, -4.558027600674, 0), (-301.0794364196, -4.973639607747, 0)], [1.0, 0.965022892778, 1.0], [4.093841525222, 4.624372808848], [3, 3]),
            ([(-301.0794364196, -4.973639607747, 0), (-305.7495059172, -5.382250264054, 0), (-310.2509536196, -4.073250507747, 0)], [1.0, 0.982911940303, 1.0], [4.625115607744, 4.995379833974], [3, 3]),
            ([(-310.2509536196, -4.073250507747, 0), (-316.9437645337, -2.101960109193, 0), (-322.5435901196, 2.059969392253, 0)], [1.0, 0.984489529102, 1.0], [4.998827711282, 5.351539869904], [3, 3]),
            ([(-322.5435901196, 2.059969392253, 0), (-334.5226324809, 10.898647662948, 0), (-339.8415317196, 24.802927592253, 0)], [1.0, 0.959695674427, 1.0], [5.348065490663, 5.917823428443], [3, 3]),
            ([(-339.8415317196, 24.802927592253, 0), (-342.0700828137, 30.623404698798, 0), (-342.2745882196, 36.852575692253, 0)], [1.0, 0.986183813187, 1.0], [5.917523082451, 6.250366818204], [3, 3]),
            ([(-342.2745882196, 36.852575692253, 0), (-342.4247910046, 41.431721996335, 0), (-340.9055027196, 45.754094592253, 0)], [1.0, 0.982863060326, 1.0], [6.250395581723, 6.621190515585], [3, 3]),
            ([(-340.9055027196, 45.754094592253, 0), (-339.1400529815, 50.771553697458, 0), (-334.8542809196, 53.921770592253, 0)], [1.0, 0.955542223661, 1.0], [0.338332039509, 0.936938113959], [3, 3]),
            ([(-334.8542809196, 53.921770592253, 0), (-341.3906845196, 62.139156292253, 0)], [1.0, 1.0], [0, 10.499999988793], [2, 2]),
            ([(-341.3906845196, 62.139156292253, 0), (-341.4047357196, 62.140757292253, 0)], [1.0, 1.0], [0, 0.01414211520387], [2, 2]),
            ([(-341.4047357196, 62.140757292253, 0), (-348.1765155781, 57.015636285867, 0), (-350.8166136196, 48.943848992253, 0)], [1.0, 0.954322391322, 1.0], [2.218657596984, 2.825483095946], [3, 3]),
            ([(-350.8166136196, 48.943848992253, 0), (-351.0383303196, 48.299168292253, 0)], [1.0, 1.0], [0, 0.681741519941], [2, 2]),
            ([(-351.0383303196, 48.299168292253, 0), (-351.2839676196, 47.456886592253, 0)], [1.0, 1.0], [0, 0.877368876418], [2, 2]),
            ([(-351.2839676196, 47.456886592253, 0), (-351.4635568196, 46.841052692253, 0)], [1.0, 1.0], [0, 0.641485520605], [2, 2]),
            ([(-351.4635568196, 46.841052692253, 0), (-351.6984633196, 45.868914692253, 0)], [1.0, 1.0], [0, 1.000116670587], [2, 2]),
            ([(-351.6984633196, 45.868914692253, 0), (-351.8264934196, 45.339049192253, 0)], [1.0, 1.0], [0, 0.545113891399], [2, 2]),
            ([(-351.8264934196, 45.339049192253, 0), (-352.1082743196, 43.883767892253, 0)], [1.0, 1.0], [0, 1.482310405325], [2, 2]),
            ([(-352.1082743196, 43.883767892253, 0), (-353.383015604, 35.975103331964, 0), (-351.6503054196, 28.154000092253, 0)], [1.0, 0.982208575388, 1.0], [2.981784256296, 3.359614187819], [3, 3]),
            ([(-351.6503054196, 28.154000092253, 0), (-350.7534690699, 23.93549827792, 0), (-349.1597665196, 19.927982192253, 0)], [1.0, 0.996430892843, 1.0], [3.351069837621, 3.520096043117], [3, 3]),
            ([(-349.1597665196, 19.927982192253, 0), (-349.1597664196, 19.927982292253, 0)], [1.0, 1.0], [0, 1.4142135789e-07], [2, 2]),
            ([(-349.1597664196, 19.927982292253, 0), (-348.7223082196, 18.823696092253, 0)], [1.0, 1.0], [0, 1.187778467669], [2, 2]),
            ([(-348.7223082196, 18.823696092253, 0), (-348.31135125, 17.838531743235, 0), (-347.8615744196, 16.870474292253, 0)], [1.0, 0.99980255498, 1.0], [3.536791785816, 3.576536120524], [3, 3]),
            ([(-347.8615744196, 16.870474292253, 0), (-347.4692038822, 15.980792697359, 0), (-347.0177528196, 15.119584292253, 0)], [1.0, 0.999431185968, 1.0], [3.556956738313, 3.624417421105], [3, 3]),
            ([(-347.0177528196, 15.119584292253, 0), (-346.531636428, 14.15491275596, 0), (-346.0086195196, 13.209738892253, 0)], [1.0, 0.999813397739, 1.0], [3.6083706079, 3.64700821288], [3, 3]),
            ([(-346.0086195196, 13.209738892253, 0), (-345.544981532, 12.332784109564, 0), (-345.0238496196, 11.488727592253, 0)], [1.0, 0.999442450869, 1.0], [3.627928640916, 3.694717918424], [3, 3]),
            ([(-345.0238496196, 11.488727592253, 0), (-344.4664945205, 10.552920027383, 0), (-343.8741054196, 9.638887392253, 0)], [1.0, 0.999820706617, 1.0], [3.678761202648, 3.716634543283], [3, 3]),
            ([(-343.8741054196, 9.638887392253, 0), (-343.3436040766, 8.785720491566, 0), (-342.7576964196, 7.969609192253, 0)], [1.0, 0.999449607618, 1.0], [3.697889364514, 3.764248561511], [3, 3]),
            ([(-342.7576964196, 7.969609192253, 0), (-342.1338120731, 7.070780032651, 0), (-341.4767103196, 6.195941292253, 0)], [1.0, 0.999824675496, 1.0], [3.748353177139, 3.785804973038], [3, 3]),
            ([(-341.4767103196, 6.195941292253, 0), (-334.0587253017, -3.777455965043, 0), (-323.2267339196, -9.873611907747, 0)], [1.0, 0.978168425239, 1.0], [3.781099229963, 4.199777882551], [3, 3]),
            ([(-323.2267339196, -9.873611907747, 0), (-322.5255925196, -10.24330800774, 0)], [1.0, 1.0], [0, 0.79263766574], [2, 2]),
            ([(-322.5255925196, -10.24330800774, 0), (-321.5301401196, -10.76818800774, 0)], [1.0, 1.0], [0, 1.125355275042], [2, 2]),
            ([(-321.5301401196, -10.76818800774, 0), (-320.8147575196, -11.11131890774, 0)], [1.0, 1.0], [0, 0.793417342209], [2, 2]),
            ([(-320.8147575196, -11.11131890774, 0), (-319.8265394196, -11.58530380774, 0)], [1.0, 1.0], [0, 1.096009442749], [2, 2]),
            ([(-319.8265394196, -11.58530380774, 0), (-319.0981284196, -11.90071490774, 0)], [1.0, 1.0], [0, 0.793767438816], [2, 2]),
            ([(-319.0981284196, -11.90071490774, 0), (-318.1202649196, -12.32413160774, 0)], [1.0, 1.0], [0, 1.065597825857], [2, 2]),
            ([(-318.1202649196, -12.32413160774, 0), (-317.3797834196, -12.61083700774, 0)], [1.0, 1.0], [0, 0.794048385321], [2, 2]),
            ([(-317.3797834196, -12.61083700774, 0), (-316.4151829196, -12.98430850774, 0)], [1.0, 1.0], [0, 1.034376665394], [2, 2]),
            ([(-316.4151829196, -12.98430850774, 0), (-315.4900805347, -13.32187534953, 0), (-314.5498954196, -13.61482200774, 0)], [1.0, 0.99971400739, 1.0], [4.362505251072, 4.41033881611], [3, 3]),
            ([(-314.5498954196, -13.61482200774, 0), (-313.7949751526, -13.86994764791, 0), (-313.0232627196, -14.06857700774, 0)], [1.0, 0.999315939065, 1.0], [4.386488797142, 4.460469277769], [3, 3]),
            ([(-313.0232627196, -14.06857700774, 0), (-312.0973697281, -14.32361496159, 0), (-311.1597558196, -14.53146670774, 0)], [1.0, 0.999679572807, 1.0], [4.443603724061, 4.494235279973], [3, 3]),
            ([(-311.1597558196, -14.53146670774, 0), (-310.4248867392, -14.71390374819, 0), (-309.6783186196, -14.84020450774, 0)], [1.0, 0.999282827886, 1.0], [4.469050629925, 4.544800632057], [3, 3]),
            ([(-309.6783186196, -14.84020450774, 0), (-308.7585526652, -15.01203829204, 0), (-307.8308925196, -15.13427950774, 0)], [1.0, 0.999639890631, 1.0], [4.527694751, 4.581370144537], [3, 3]),
            ([(-307.8308925196, -15.13427950774, 0), (-307.1211570874, -15.24709219571, 0), (-306.4048132196, -15.30456000774, 0)], [1.0, 0.9992477759, 1.0], [4.554757345301, 4.632336642859], [3, 3]),
            ([(-306.4048132196, -15.30456000774, 0), (-304.6577478981, -15.477536924, 0), (-302.9021928196, -15.46394950774, 0)], [1.0, 0.99858447286, 1.0], [4.613700633722, 4.72012849544], [3, 3]),
        ]
        # fmt: on

        # Run once with the original BSpline edges, and once with each edge replaced by a
        # straight line between its endpoints
        for as_lines in (False, True):
            with self.subTest(as_lines=as_lines):
                # Rebuild the face from its edges
                edges = []
                for poles, weights, knots, mults in edge_data:
                    if as_lines:
                        edges.append(Part.makeLine(poles[0], poles[-1]))
                    else:
                        degree = sum(mults) - len(poles) - 1
                        curve = Part.BSplineCurve()
                        curve.buildFromPolesMultsKnots(
                            [FreeCAD.Vector(*p) for p in poles],
                            mults,
                            knots,
                            False,
                            degree,
                            weights,
                        )
                        edges.append(curve.toShape())
                part_obj = FreeCAD.ActiveDocument.addObject(
                    "Part::Feature", f"MillFacePart_{'lines' if as_lines else 'splines'}"
                )
                part_obj.Shape = Part.Face(Part.Wire(edges))

                # Create MillFace op
                job = PathJob.Create("Job_regression_mill_face", [part_obj])
                mill_face = PathMillFace.Create("regression_mill_face", parentJob=job)
                tc = PathToolController.Create(name="TC: 10mm Endmill")
                job.Proxy.addToolController(tc)
                tc.Tool.Diameter = 10.0
                mill_face.ToolController = tc
                mill_face.Base = [(part_obj, ["Face1"])]
                mill_face.BoundaryShape = "Face Region"
                mill_face.ClearingPattern = "ZigZag"
                mill_face.StepOver = 50
                mill_face.CutMode = "Climb"
                mill_face.StartAt = "Center"
                mill_face.ExtraOffset = 6.0
                mill_face.recompute()

                # Assert that it executes without error
                self.assertTrue(mill_face.isValid(), mill_face.getStatusString())


def _addViewProvider(pocketOp):
    if FreeCAD.GuiUp:
        PathOpGui = PathPocketGui.PathOpGui
        cmdRes = PathPocketGui.Command.res
        pocketOp.ViewObject.Proxy = PathOpGui.ViewProvider(pocketOp.ViewObject, cmdRes)
