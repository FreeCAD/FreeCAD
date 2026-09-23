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
import Path.Op.Pocket as PathPocket
import Path.Op.MillFace as PathMillFace
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

    def testPocketRegression01(self):
        v = FreeCAD.Vector
        box_edges = [
            Part.makeLine(v(0, 0, 0), v(0, 100, 0)),
            Part.makeLine(v(0, 100, 0), v(100, 100, 0)),
            Part.makeLine(v(100, 0, 0), v(100, 100, 0)),
            Part.makeLine(v(0, 0, 0), v(100, 0, 0)),
        ]
        box_wire = Part.Wire(box_edges)
        box_face = Part.Face(box_wire)
        box = box_face.extrude(v(0, 0, 5))

        cut_edges = [
            Part.makeLine(v(35.9789, 64.2984, 2), v(40.4894, 78.1803, 2)),
            Part.Arc(
                v(35.9789, 64.2984, 2), v(34.8891, 62.7984, 2), v(33.1257, 62.2254, 2)
            ).toShape(),
            Part.Arc(v(40.4894, 78.1803, 2), v(50, 85.0902, 2), v(59.5106, 78.1803, 2)).toShape(),
            Part.makeLine(v(18.5294, 62.2254, 2), v(33.1257, 62.2254, 2)),
            Part.makeLine(v(59.5106, 78.1803, 2), v(64.0211, 64.2984, 2)),
            Part.Arc(
                v(12.6515, 44.1353, 2), v(9.01881, 55.3156, 2), v(18.5294, 62.2254, 2)
            ).toShape(),
            Part.Arc(
                v(64.0211, 64.2984, 2), v(65.1109, 62.7984, 2), v(66.8743, 62.2254, 2)
            ).toShape(),
            Part.makeLine(v(24.4602, 35.5557, 2), v(12.6515, 44.1353, 2)),
            Part.makeLine(v(66.8743, 62.2254, 2), v(81.4706, 62.2254, 2)),
            Part.Arc(v(24.4602, 35.5557, 2), v(25.55, 34.0557, 2), v(25.55, 32.2016, 2)).toShape(),
            Part.Arc(
                v(81.4706, 62.2254, 2), v(90.9812, 55.3156, 2), v(87.3485, 44.1353, 2)
            ).toShape(),
            Part.makeLine(v(21.0395, 18.3197, 2), v(25.55, 32.2016, 2)),
            Part.makeLine(v(87.3485, 44.1353, 2), v(75.5398, 35.5557, 2)),
            Part.Arc(
                v(36.4279, 7.13932, 2), v(24.6722, 7.13932, 2), v(21.0395, 18.3197, 2)
            ).toShape(),
            Part.Arc(v(75.5398, 35.5557, 2), v(74.45, 34.0557, 2), v(74.45, 32.2016, 2)).toShape(),
            Part.makeLine(v(48.2366, 15.7188, 2), v(36.4279, 7.13932, 2)),
            Part.makeLine(v(74.45, 32.2016, 2), v(78.9605, 18.3197, 2)),
            Part.Arc(v(48.2366, 15.7188, 2), v(50, 16.2918, 2), v(51.7634, 15.7188, 2)).toShape(),
            Part.Arc(
                v(78.9605, 18.3197, 2), v(75.3278, 7.13932, 2), v(63.5721, 7.13932, 2)
            ).toShape(),
            Part.makeLine(v(63.5721, 7.13932, 2), v(51.7634, 15.7188, 2)),
        ]
        cutout_wire = Part.Wire(cut_edges)
        cutout_face = Part.Face(cutout_wire)
        cutout = cutout_face.extrude(v(0, 0, box.BoundBox.ZMax + 1))

        solid = box.cut(cutout)
        pocket_bottom_z = cutout.BoundBox.ZMin

        part_obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "Regression01Part")
        part_obj.Shape = solid

        pocket = self.createPocketOperation(
            part_obj,
            pocket_bottom_z,
            "regression01",
            tool_diameter=3.0,
            ClearingPattern="Offset",
            StartAt="Edge",
        )

        # assert that there is a meaningful amount of output commands
        self.assertGreater(len(pocket.Path.Commands), 50)

    def testPocketRegression02(self):
        v = FreeCAD.Vector

        wires = []
        wires.append(
            Part.Wire(
                [
                    Part.makeLine(v(0, 0, 0), v(97.5364, 0, 0)),
                    Part.makeLine(v(0, 68.4919, 0), v(0, 0, 0)),
                    Part.makeLine(v(97.5364, 68.4919, 0), v(0, 68.4919, 0)),
                    Part.makeLine(v(97.5364, 0, 0), v(97.5364, 68.4919, 0)),
                ]
            )
        )
        box_face = Part.Face(wires)
        box = box_face.extrude(v(0, 0, 5))

        wires = []
        wires.append(
            Part.Wire(
                [
                    Part.Arc(
                        v(32.1612, 54.1129, 3), v(9.80652, 31.7582, 3), v(32.1612, 9.40351, 3)
                    ).toShape(),
                    Part.makeLine(v(32.1612, 54.1129, 3), v(66.2554, 54.1129, 3)),
                    Part.makeLine(v(32.1612, 9.40351, 3), v(66.2554, 9.40351, 3)),
                    Part.Arc(
                        v(66.2554, 9.40351, 3), v(88.6101, 31.7582, 3), v(66.2554, 54.1129, 3)
                    ).toShape(),
                ]
            )
        )
        wires.append(
            Part.Wire(
                [
                    Part.makeLine(v(31.7954, 44.6145, 3), v(31.7954, 18.902, 3)),
                    Part.makeLine(v(31.7954, 18.902, 3), v(33.7954, 18.902, 3)),
                    Part.makeLine(v(33.7954, 18.902, 3), v(33.7954, 44.6145, 3)),
                    Part.makeLine(v(33.7954, 44.6145, 3), v(31.7954, 44.6145, 3)),
                ]
            ).reversed()
        )
        wires.append(
            Part.Wire(
                [
                    Part.makeLine(v(48.2083, 44.6145, 3), v(48.2083, 18.902, 3)),
                    Part.makeLine(v(48.2083, 18.902, 3), v(50.2083, 18.902, 3)),
                    Part.makeLine(v(50.2083, 18.902, 3), v(50.2083, 44.6145, 3)),
                    Part.makeLine(v(50.2083, 44.6145, 3), v(48.2083, 44.6145, 3)),
                ]
            ).reversed()
        )
        wires.append(
            Part.Wire(
                [
                    Part.makeLine(v(64.6213, 44.6145, 3), v(64.6213, 18.902, 3)),
                    Part.makeLine(v(64.6213, 18.902, 3), v(66.6213, 18.902, 3)),
                    Part.makeLine(v(66.6213, 18.902, 3), v(66.6213, 44.6145, 3)),
                    Part.makeLine(v(66.6213, 44.6145, 3), v(64.6213, 44.6145, 3)),
                ]
            ).reversed()
        )
        pocket_face = Part.Face(wires)
        cutout = pocket_face.extrude(v(0, 0, box.BoundBox.ZMax + 1))

        solid = box.cut(cutout)
        pocket_bottom_z = cutout.BoundBox.ZMin

        part_obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "Regression02Part")
        part_obj.Shape = solid

        pocket = self.createPocketOperation(
            part_obj,
            pocket_bottom_z,
            "regression02",
            tool_diameter=3.0,
            ClearingPattern="Offset",
            StartAt="Edge",
        )

        self.assertGreater(len(pocket.Path.Commands), 50)

    def testMillFaceRegression03(self):
        v = FreeCAD.Vector

        def _bs(poles, mults, knots, periodic, degree, u1, u2):
            c = Part.BSplineCurve()
            c.buildFromPolesMultsKnots(poles, mults, knots, periodic, degree)
            return c.toShape(u1, u2)

        wires = []
        wires.append(
            Part.Wire(
                [
                    _bs(
                        [
                            v(-302.902, -15.4639, -30),
                            v(-302.288, -15.4762, -30),
                            v(-301.674, -15.4427, -30),
                        ],
                        [3, 3],
                        [4.69246, 4.76698],
                        False,
                        2,
                        4.69246,
                        4.76698,
                    ),
                    _bs(
                        [
                            v(-301.674, -15.4427, -30),
                            v(-295.079, -15.2418, -30),
                            v(-289.247, -12.156, -30),
                        ],
                        [3, 3],
                        [4.74283, 5.19906],
                        False,
                        2,
                        4.74283,
                        5.19906,
                    ),
                    _bs(
                        [
                            v(-289.247, -12.156, -30),
                            v(-287.236, -11.0736, -30),
                            v(-285.448, -9.6525, -30),
                        ],
                        [3, 3],
                        [5.20609, 5.38397],
                        False,
                        2,
                        5.20609,
                        5.38397,
                    ),
                    _bs(
                        [
                            v(-285.448, -9.6525, -30),
                            v(-284.446, -8.90377, -30),
                            v(-283.491, -8.09582, -30),
                        ],
                        [3, 3],
                        [5.3541, 5.41459],
                        False,
                        2,
                        5.3541,
                        5.41459,
                    ),
                    _bs(
                        [
                            v(-283.491, -8.09582, -30),
                            v(-279.375, -4.82112, -30),
                            v(-282.649, -0.705036, -30),
                            v(-285.924, 3.41104, -30),
                            v(-290.04, 0.136824, -30),
                        ],
                        [3, 2, 3],
                        [5.38446, 6.95519, 8.52593],
                        False,
                        2,
                        5.38446,
                        8.52593,
                    ),
                    _bs(
                        [
                            v(-290.04, 0.136824, -30),
                            v(-291.259, -0.865526, -30),
                            v(-292.518, -1.81643, -30),
                        ],
                        [3, 3],
                        [4.02398, 4.06551],
                        False,
                        2,
                        4.02398,
                        4.06551,
                    ),
                    _bs(
                        [
                            v(-292.518, -1.81643, -30),
                            v(-296.37, -4.55803, -30),
                            v(-301.079, -4.97364, -30),
                        ],
                        [3, 3],
                        [4.09384, 4.62437],
                        False,
                        2,
                        4.09384,
                        4.62437,
                    ),
                    _bs(
                        [
                            v(-301.079, -4.97364, -30),
                            v(-305.75, -5.38225, -30),
                            v(-310.251, -4.07325, -30),
                        ],
                        [3, 3],
                        [4.62512, 4.99538],
                        False,
                        2,
                        4.62512,
                        4.99538,
                    ),
                    _bs(
                        [
                            v(-310.251, -4.07325, -30),
                            v(-316.944, -2.10196, -30),
                            v(-322.544, 2.05997, -30),
                        ],
                        [3, 3],
                        [4.99883, 5.35154],
                        False,
                        2,
                        4.99883,
                        5.35154,
                    ),
                    _bs(
                        [
                            v(-322.544, 2.05997, -30),
                            v(-334.523, 10.8986, -30),
                            v(-339.842, 24.8029, -30),
                        ],
                        [3, 3],
                        [5.34807, 5.91782],
                        False,
                        2,
                        5.34807,
                        5.91782,
                    ),
                    _bs(
                        [
                            v(-339.842, 24.8029, -30),
                            v(-342.07, 30.6234, -30),
                            v(-342.275, 36.8526, -30),
                        ],
                        [3, 3],
                        [5.91752, 6.25037],
                        False,
                        2,
                        5.91752,
                        6.25037,
                    ),
                    _bs(
                        [
                            v(-342.275, 36.8526, -30),
                            v(-342.425, 41.4317, -30),
                            v(-340.906, 45.7541, -30),
                        ],
                        [3, 3],
                        [6.2504, 6.62119],
                        False,
                        2,
                        6.2504,
                        6.62119,
                    ),
                    _bs(
                        [
                            v(-340.906, 45.7541, -30),
                            v(-339.14, 50.7716, -30),
                            v(-334.854, 53.9218, -30),
                        ],
                        [3, 3],
                        [0.338332, 0.936938],
                        False,
                        2,
                        0.338332,
                        0.936938,
                    ),
                    _bs(
                        [v(-334.854, 53.9218, -30), v(-341.391, 62.1392, -30)],
                        [2, 2],
                        [0, 10.5],
                        False,
                        1,
                        0,
                        10.5,
                    ),
                    _bs(
                        [v(-341.391, 62.1392, -30), v(-341.405, 62.1408, -30)],
                        [2, 2],
                        [0, 0.0141421],
                        False,
                        1,
                        0,
                        0.0141421,
                    ),
                    _bs(
                        [
                            v(-341.405, 62.1408, -30),
                            v(-348.177, 57.0156, -30),
                            v(-350.817, 48.9438, -30),
                        ],
                        [3, 3],
                        [2.21866, 2.82548],
                        False,
                        2,
                        2.21866,
                        2.82548,
                    ),
                    _bs(
                        [v(-350.817, 48.9438, -30), v(-351.038, 48.2992, -30)],
                        [2, 2],
                        [0, 0.681742],
                        False,
                        1,
                        0,
                        0.681742,
                    ),
                    _bs(
                        [v(-351.038, 48.2992, -30), v(-351.284, 47.4569, -30)],
                        [2, 2],
                        [0, 0.877369],
                        False,
                        1,
                        0,
                        0.877369,
                    ),
                    _bs(
                        [v(-351.284, 47.4569, -30), v(-351.464, 46.8411, -30)],
                        [2, 2],
                        [0, 0.641486],
                        False,
                        1,
                        0,
                        0.641486,
                    ),
                    _bs(
                        [v(-351.464, 46.8411, -30), v(-351.698, 45.8689, -30)],
                        [2, 2],
                        [0, 1.00012],
                        False,
                        1,
                        0,
                        1.00012,
                    ),
                    _bs(
                        [v(-351.698, 45.8689, -30), v(-351.826, 45.339, -30)],
                        [2, 2],
                        [0, 0.545114],
                        False,
                        1,
                        0,
                        0.545114,
                    ),
                    _bs(
                        [v(-351.826, 45.339, -30), v(-352.108, 43.8838, -30)],
                        [2, 2],
                        [0, 1.48231],
                        False,
                        1,
                        0,
                        1.48231,
                    ),
                    _bs(
                        [
                            v(-352.108, 43.8838, -30),
                            v(-353.383, 35.9751, -30),
                            v(-351.65, 28.154, -30),
                        ],
                        [3, 3],
                        [2.98178, 3.35961],
                        False,
                        2,
                        2.98178,
                        3.35961,
                    ),
                    _bs(
                        [
                            v(-351.65, 28.154, -30),
                            v(-350.753, 23.9355, -30),
                            v(-349.16, 19.928, -30),
                        ],
                        [3, 3],
                        [3.35107, 3.5201],
                        False,
                        2,
                        3.35107,
                        3.5201,
                    ),
                    _bs(
                        [v(-349.16, 19.928, -30), v(-349.16, 19.928, -30)],
                        [2, 2],
                        [0, 1.41421e-07],
                        False,
                        1,
                        0,
                        1.41421e-07,
                    ),
                    _bs(
                        [v(-349.16, 19.928, -30), v(-348.722, 18.8237, -30)],
                        [2, 2],
                        [0, 1.18778],
                        False,
                        1,
                        0,
                        1.18778,
                    ),
                    _bs(
                        [
                            v(-348.722, 18.8237, -30),
                            v(-348.311, 17.8385, -30),
                            v(-347.862, 16.8705, -30),
                        ],
                        [3, 3],
                        [3.53679, 3.57654],
                        False,
                        2,
                        3.53679,
                        3.57654,
                    ),
                    _bs(
                        [
                            v(-347.862, 16.8705, -30),
                            v(-347.469, 15.9808, -30),
                            v(-347.018, 15.1196, -30),
                        ],
                        [3, 3],
                        [3.55696, 3.62442],
                        False,
                        2,
                        3.55696,
                        3.62442,
                    ),
                    _bs(
                        [
                            v(-347.018, 15.1196, -30),
                            v(-346.532, 14.1549, -30),
                            v(-346.009, 13.2097, -30),
                        ],
                        [3, 3],
                        [3.60837, 3.64701],
                        False,
                        2,
                        3.60837,
                        3.64701,
                    ),
                    _bs(
                        [
                            v(-346.009, 13.2097, -30),
                            v(-345.545, 12.3328, -30),
                            v(-345.024, 11.4887, -30),
                        ],
                        [3, 3],
                        [3.62793, 3.69472],
                        False,
                        2,
                        3.62793,
                        3.69472,
                    ),
                    _bs(
                        [
                            v(-345.024, 11.4887, -30),
                            v(-344.466, 10.5529, -30),
                            v(-343.874, 9.63889, -30),
                        ],
                        [3, 3],
                        [3.67876, 3.71663],
                        False,
                        2,
                        3.67876,
                        3.71663,
                    ),
                    _bs(
                        [
                            v(-343.874, 9.63889, -30),
                            v(-343.344, 8.78572, -30),
                            v(-342.758, 7.96961, -30),
                        ],
                        [3, 3],
                        [3.69789, 3.76425],
                        False,
                        2,
                        3.69789,
                        3.76425,
                    ),
                    _bs(
                        [
                            v(-342.758, 7.96961, -30),
                            v(-342.134, 7.07078, -30),
                            v(-341.477, 6.19594, -30),
                        ],
                        [3, 3],
                        [3.74835, 3.7858],
                        False,
                        2,
                        3.74835,
                        3.7858,
                    ),
                    _bs(
                        [
                            v(-341.477, 6.19594, -30),
                            v(-334.059, -3.77746, -30),
                            v(-323.227, -9.87361, -30),
                        ],
                        [3, 3],
                        [3.7811, 4.19978],
                        False,
                        2,
                        3.7811,
                        4.19978,
                    ),
                    _bs(
                        [v(-323.227, -9.87361, -30), v(-322.526, -10.2433, -30)],
                        [2, 2],
                        [0, 0.792638],
                        False,
                        1,
                        0,
                        0.792638,
                    ),
                    _bs(
                        [v(-322.526, -10.2433, -30), v(-321.53, -10.7682, -30)],
                        [2, 2],
                        [0, 1.12536],
                        False,
                        1,
                        0,
                        1.12536,
                    ),
                    _bs(
                        [v(-321.53, -10.7682, -30), v(-320.815, -11.1113, -30)],
                        [2, 2],
                        [0, 0.793417],
                        False,
                        1,
                        0,
                        0.793417,
                    ),
                    _bs(
                        [v(-320.815, -11.1113, -30), v(-319.827, -11.5853, -30)],
                        [2, 2],
                        [0, 1.09601],
                        False,
                        1,
                        0,
                        1.09601,
                    ),
                    _bs(
                        [v(-319.827, -11.5853, -30), v(-319.098, -11.9007, -30)],
                        [2, 2],
                        [0, 0.793767],
                        False,
                        1,
                        0,
                        0.793767,
                    ),
                    _bs(
                        [v(-319.098, -11.9007, -30), v(-318.12, -12.3241, -30)],
                        [2, 2],
                        [0, 1.0656],
                        False,
                        1,
                        0,
                        1.0656,
                    ),
                    _bs(
                        [v(-318.12, -12.3241, -30), v(-317.38, -12.6108, -30)],
                        [2, 2],
                        [0, 0.794048],
                        False,
                        1,
                        0,
                        0.794048,
                    ),
                    _bs(
                        [v(-317.38, -12.6108, -30), v(-316.415, -12.9843, -30)],
                        [2, 2],
                        [0, 1.03438],
                        False,
                        1,
                        0,
                        1.03438,
                    ),
                    _bs(
                        [
                            v(-316.415, -12.9843, -30),
                            v(-315.49, -13.3219, -30),
                            v(-314.55, -13.6148, -30),
                        ],
                        [3, 3],
                        [4.36251, 4.41034],
                        False,
                        2,
                        4.36251,
                        4.41034,
                    ),
                    _bs(
                        [
                            v(-314.55, -13.6148, -30),
                            v(-313.795, -13.8699, -30),
                            v(-313.023, -14.0686, -30),
                        ],
                        [3, 3],
                        [4.38649, 4.46047],
                        False,
                        2,
                        4.38649,
                        4.46047,
                    ),
                    _bs(
                        [
                            v(-313.023, -14.0686, -30),
                            v(-312.097, -14.3236, -30),
                            v(-311.16, -14.5315, -30),
                        ],
                        [3, 3],
                        [4.4436, 4.49424],
                        False,
                        2,
                        4.4436,
                        4.49424,
                    ),
                    _bs(
                        [
                            v(-311.16, -14.5315, -30),
                            v(-310.425, -14.7139, -30),
                            v(-309.678, -14.8402, -30),
                        ],
                        [3, 3],
                        [4.46905, 4.5448],
                        False,
                        2,
                        4.46905,
                        4.5448,
                    ),
                    _bs(
                        [
                            v(-309.678, -14.8402, -30),
                            v(-308.759, -15.012, -30),
                            v(-307.831, -15.1343, -30),
                        ],
                        [3, 3],
                        [4.52769, 4.58137],
                        False,
                        2,
                        4.52769,
                        4.58137,
                    ),
                    _bs(
                        [
                            v(-307.831, -15.1343, -30),
                            v(-307.121, -15.2471, -30),
                            v(-306.405, -15.3046, -30),
                        ],
                        [3, 3],
                        [4.55476, 4.63234],
                        False,
                        2,
                        4.55476,
                        4.63234,
                    ),
                    _bs(
                        [
                            v(-306.405, -15.3046, -30),
                            v(-304.658, -15.4775, -30),
                            v(-302.902, -15.4639, -30),
                        ],
                        [3, 3],
                        [4.6137, 4.72013],
                        False,
                        2,
                        4.6137,
                        4.72013,
                    ),
                ]
            )
        )
        pocket_face = Part.Face(wires)

        solid = pocket_face.extrude(v(0, 0, -5))

        part_obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "Regression03Part")
        part_obj.Shape = solid

        job = PathJob.Create("Job_regression03", [part_obj])
        if FreeCAD.GuiUp:
            job.ViewObject.Proxy = PathJobGui.ViewProvider(job.ViewObject)

        op = PathMillFace.Create("regression03", parentJob=job)

        tc = PathToolController.Create(name="TC: 3mm Endmill")
        job.Proxy.addToolController(tc)
        tc.Tool.Diameter = 10
        op.ToolController = tc
        op.BoundaryShape = "Face Region"
        op.ExtraOffset = 6

        top_z = solid.BoundBox.ZMax
        top_faces = [
            "Face{}".format(i + 1)
            for i, f in enumerate(solid.Faces)
            if abs(f.BoundBox.ZMax - top_z) < 0.1 and abs(f.BoundBox.ZMin - top_z) < 0.1
        ]
        op.Base = [(part_obj, top_faces)]

        self.doc.recompute()

        self.assertGreater(len(op.Path.Commands), 10)

    def testPocketPancake04(self):
        v = FreeCAD.Vector
        box_edges = [
            Part.makeLine(v(-100, -100, 0), v(-100, 100, 0)),
            Part.makeLine(v(-100, 100, 0), v(100, 100, 0)),
            Part.makeLine(v(100, -100, 0), v(100, 100, 0)),
            Part.makeLine(v(-100, -100, 0), v(100, -100, 0)),
        ]
        box_wire = Part.Wire(box_edges)
        box_face = Part.Face(box_wire)
        box = box_face.extrude(v(0, 0, 5))

        cut_edges = [
            Part.Arc(
                v(-26.124492, -42.7890488, 2),
                v(-46.425036, -42.6018578, 2),
                v(-66.643016, -42.3401452, 2),
            ).toShape(),
            Part.makeLine(v(-66.643016, -42.3401452, 2), v(-75.438384, -48.2753298, 2)),
            Part.makeLine(v(-75.438384, -48.2753298, 2), v(-66.643016, -42.3401454, 2)),
            Part.Arc(
                v(-66.643016, -42.3401454, 2),
                v(-82.043978, -42.0799932, 2),
                v(-97.327170, -41.7577888, 2),
            ).toShape(),
            Part.makeLine(v(-97.327170, -41.7577888, 2), v(-26.124492, -41.7577888, 2)),
            Part.makeLine(v(-26.124492, -41.7577888, 2), v(-26.124492, -42.7890488, 2)),
        ]
        cutout_wire = Part.Wire(cut_edges)
        cutout_face = Part.Face(cutout_wire)
        cutout = cutout_face.extrude(v(0, 0, box.BoundBox.ZMax + 1))

        solid = box.cut(cutout)
        pocket_bottom_z = cutout.BoundBox.ZMin

        part_obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "Regression04Part")
        part_obj.Shape = solid

        pocket = self.createPocketOperation(
            part_obj,
            pocket_bottom_z,
            "regression04",
            tool_diameter=0.5,
            ClearingPattern="Offset",
            StartAt="Edge",
            StepDown=5,
        )

        # assert that there is a meaningful amount of output commands
        self.assertGreater(len(pocket.Path.Commands), 10)


def _addViewProvider(pocketOp):
    if FreeCAD.GuiUp:
        PathOpGui = PathPocketGui.PathOpGui
        cmdRes = PathPocketGui.Command.res
        pocketOp.ViewObject.Proxy = PathOpGui.ViewProvider(pocketOp.ViewObject, cmdRes)
