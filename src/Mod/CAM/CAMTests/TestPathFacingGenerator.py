# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2024 sliptonic <shopinthewoods@gmail.com>               *
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
import Path.Base.Generator.facing as facing
import Part
import math

from CAMTests.PathTestUtils import PathTestBase


class TestPathFacingGenerator(PathTestBase):
    """Test facing generator."""

    def setUp(self):
        """Set up test fixtures."""
        super().setUp()
        
        # Create test polygons
        self.square_wire = Part.makePolygon([
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Vector(10, 0, 0),
            FreeCAD.Vector(10, 10, 0),
            FreeCAD.Vector(0, 10, 0),
            FreeCAD.Vector(0, 0, 0)
        ])
        
        self.rectangle_wire = Part.makePolygon([
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Vector(20, 0, 0),
            FreeCAD.Vector(20, 10, 0),
            FreeCAD.Vector(0, 10, 0),
            FreeCAD.Vector(0, 0, 0)
        ])
        
        # Create a circular wire for testing curves
        self.circle_wire = Part.Wire([Part.Circle(FreeCAD.Vector(5, 5, 0), FreeCAD.Vector(0, 0, 1), 5).toShape()])
        
        # Create a wire with splines/curves
        points = [
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Vector(5, 0, 0),
            FreeCAD.Vector(10, 5, 0),
            FreeCAD.Vector(5, 10, 0),
            FreeCAD.Vector(0, 5, 0),
            FreeCAD.Vector(0, 0, 0)
        ]
        self.spline_wire = Part.Wire([Part.BSplineCurve(points, None, None, False, 3, None, False).toShape()])

    def test_basic_square_facing(self):
        """Test basic facing of a square polygon."""
        commands = facing.generate(
            wire=self.square_wire,
            tool_diameter=6.0,
            stepover_percent=75,
            start_depth=5.0,
            final_depth=0.0,
            start_point=FreeCAD.Vector(-5, 0, 0),  # Start from left side
            cutting_feedrate=1000
        )
        
        # Verify we get commands
        self.assertGreater(len(commands), 0)
        
        # First command should be rapid to approach position
        first_cmd = commands[0]
        self.assertEqual(first_cmd.Name, "G0")
        
        # Should start outside the polygon boundary
        self.assertLess(first_cmd.Parameters.get('X', 0), -6/2)
        
        # Last command should retract
        last_cmd = commands[-1]
        self.assertEqual(last_cmd.Name, "G0")
        self.assertEqual(last_cmd.Parameters.get('Z', 0), 5.0)

    def test_rectangular_facing(self):
        """Test facing of a rectangular polygon."""
        commands = facing.generate(
            wire=self.rectangle_wire,
            tool_diameter=4.0,
            stepover_percent=50,
            start_depth=3.0,
            final_depth=-2.0,
            start_point=FreeCAD.Vector(10, 15, 0),  # Start from top side
            cutting_feedrate=800
        )
        
        # Find first cutting move
        cutting_moves = [cmd for cmd in commands if cmd.Name == "G1"]
        self.assertGreater(len(cutting_moves), 0)
        
        first_cut = cutting_moves[0]
        # Should start outside in Y direction
        self.assertLess(first_cut.Parameters.get('Y', 0), -4/2)

    def test_auto_approach_direction(self):
        """Test automatic approach direction selection."""
        # Test with square (should choose X- for equal dimensions)
        commands_square = facing.generate(
            wire=self.square_wire,
            tool_diameter=6.0,
            stepover_percent=75,
            start_depth=5.0,
            final_depth=0.0
            # No start_point provided, should auto-select
        )
        
        # Should automatically choose X- approach for equal dimensions
        cutting_moves = [cmd for cmd in commands_square if cmd.Name == "G1"]
        self.assertGreater(len(cutting_moves), 0)
        
        first_cut = cutting_moves[0]
        # Should approach from X direction (equal dimensions)
        self.assertLess(first_cut.Parameters.get('X', 0), 0)

    def test_multiple_depth_passes(self):
        """Test facing with multiple depth passes."""
        commands = facing.generate(
            wire=self.square_wire,
            tool_diameter=6.0,
            stepover_percent=75,
            start_depth=5.0,
            final_depth=-5.0,
            max_depth_per_pass=2.0,
            start_point=FreeCAD.Vector(-5, 0, 0)
        )
        
        # Should have multiple Z levels
        z_values = set()
        for cmd in commands:
            if 'Z' in cmd.Parameters and cmd.Name in ["G1", "G2", "G3"]:
                z_values.add(cmd.Parameters['Z'])
        
        # Should have at least 5 passes (10mm / 2mm per pass)
        self.assertGreaterEqual(len(z_values), 5)

    def test_zigzag_pattern(self):
        """Test zigzag cutting pattern."""
        commands = facing.generate(
            wire=self.square_wire,
            tool_diameter=4.0,
            stepover_percent=50,
            start_depth=2.0,
            final_depth=0.0,
            pattern="zigzag",
            start_point=FreeCAD.Vector(-5, 0, 0)
        )
        
        # Find cutting moves and check for alternating directions
        cutting_moves = [cmd for cmd in commands if cmd.Name == "G1" and 'X' in cmd.Parameters]
        
        if len(cutting_moves) >= 2:
            # Check that consecutive moves alternate direction
            x_directions = []
            for i in range(1, len(cutting_moves)):
                prev_x = cutting_moves[i-1].Parameters.get('X', 0)
                curr_x = cutting_moves[i].Parameters.get('X', 0)
                if abs(curr_x - prev_x) > 0.1:  # Significant X movement
                    x_directions.append(curr_x > prev_x)
            
            # Should have alternating directions for zigzag
            if len(x_directions) >= 2:
                self.assertNotEqual(x_directions[0], x_directions[1])

    def test_climb_vs_conventional_milling(self):
        """Test climb vs conventional milling produces different toolpaths."""
        # Generate climb milling toolpath
        commands_climb = facing.generate(
            wire=self.square_wire,
            tool_diameter=6.0,
            stepover_percent=75,
            start_depth=2.0,
            final_depth=0.0,
            start_point=FreeCAD.Vector(-5, 0, 0),
            milling_direction="climb"
        )
        
        # Generate conventional milling toolpath
        commands_conventional = facing.generate(
            wire=self.square_wire,
            tool_diameter=6.0,
            stepover_percent=75,
            start_depth=2.0,
            final_depth=0.0,
            start_point=FreeCAD.Vector(-5, 0, 0),
            milling_direction="conventional"
        )
        
        # Both should generate valid paths
        self.assertGreater(len(commands_climb), 0)
        self.assertGreater(len(commands_conventional), 0)
        
        # Toolpaths should be different (different step order)
        # This is a basic check - more detailed verification could be added
        self.assertNotEqual(commands_climb, commands_conventional)

    def test12_stepover_validation(self):
        """Verify generator validates stepover parameters."""
        
        # Test negative stepover
        with self.assertRaises(ValueError) as context:
            facing.generate(
                wire=self.square_wire,
                tool_diameter=2.0,
                stepover_percent=-75.0,
                start_depth=1.0,
                final_depth=0.0
            )
        self.assertIn("positive", str(context.exception))
        
        # Test zero stepover
        with self.assertRaises(ValueError) as context:
            facing.generate(
                wire=self.square_wire,
                tool_diameter=2.0,
                stepover_percent=0.0,
                start_depth=1.0,
                final_depth=0.0
            )
        self.assertIn("positive", str(context.exception))

    def test13_depth_validation(self):
        """Verify generator validates depth parameters."""
        
        # Test same start and final depth
        with self.assertRaises(ValueError) as context:
            facing.generate(
                wire=self.square_wire,
                tool_diameter=2.0,
                stepover_percent=75.0,
                start_depth=1.0,
                final_depth=1.0  # Same as start
            )
        self.assertIn("different", str(context.exception))

    def test13a_start_point_validation(self):
        """Verify generator validates start point location."""
        
        # Test start point inside polygon (should fail)
        with self.assertRaises(ValueError) as context:
            facing.generate(
                wire=self.square_wire,
                tool_diameter=2.0,
                stepover_percent=75.0,
                start_depth=1.0,
                final_depth=0.0,
                start_point=FreeCAD.Vector(5, 5, 0)  # Inside the 10x10 square
            )
        self.assertIn("too close", str(context.exception))
        
        # Test start point outside polygon (should succeed)
        commands = facing.generate(
            wire=self.square_wire,
            tool_diameter=2.0,
            stepover_percent=75.0,
            start_depth=1.0,
            final_depth=0.0,
            start_point=FreeCAD.Vector(-5, 5, 0)  # Outside the square
        )
        self.assertGreater(len(commands), 0)

    def test14_improved_climb_conventional(self):
        """Verify improved climb vs conventional milling implementation."""
        
        tool_diameter = 3.0
        stepover_percent = 67.0
        start_depth = 1.0
        final_depth = 0.0
        start_point = FreeCAD.Vector(-5, 0, 0)  # Start from left side
        
        # Test climb milling
        path_climb = facing.generate(
            wire=self.square_wire,
            tool_diameter=tool_diameter,
            stepover_percent=stepover_percent,
            start_depth=start_depth,
            final_depth=final_depth,
            start_point=start_point,
            milling_direction="climb",
            pattern="unidirectional"  # Use unidirectional to avoid zigzag confusion
        )
        
        # Test conventional milling
        path_conventional = facing.generate(
            wire=self.square_wire,
            tool_diameter=tool_diameter,
            stepover_percent=stepover_percent,
            start_depth=start_depth,
            final_depth=final_depth,
            start_point=start_point,
            milling_direction="conventional",
            pattern="unidirectional"
        )
        
        # Extract cutting moves
        climb_cuts = [cmd for cmd in path_climb if cmd.Name == "G1"]
        conv_cuts = [cmd for cmd in path_conventional if cmd.Name == "G1"]
        
        self.assertGreater(len(climb_cuts), 0)
        self.assertGreater(len(conv_cuts), 0)
        
        # For X- approach with unidirectional pattern:
        # Climb: should step in Y+ direction, cut in X+ direction
        # Conventional: should step in Y- direction, cut in X- direction
        
        # Check step direction (Y coordinates should be different)
        climb_y_coords = [cmd.Parameters.get('Y', 0) for cmd in climb_cuts]
        conv_y_coords = [cmd.Parameters.get('Y', 0) for cmd in conv_cuts]
        
        # Remove duplicates and sort to see step progression
        climb_y_unique = sorted(set(climb_y_coords))
        conv_y_unique = sorted(set(conv_y_coords))
        
        if len(climb_y_unique) > 1 and len(conv_y_unique) > 1:
            # Climb should step in opposite direction from conventional
            climb_increasing = climb_y_unique[1] > climb_y_unique[0]
            conv_increasing = conv_y_unique[1] > conv_y_unique[0]
            self.assertNotEqual(climb_increasing, conv_increasing)

    def test15_curved_geometry_support(self):
        """Verify generator works with curved and spline geometries."""
        
        tool_diameter = 2.0
        stepover_percent = 75.0
        start_depth = 1.0
        final_depth = 0.0
        start_point = FreeCAD.Vector(-5, 0, 0)  # Start from left side
        
        # Test with circular wire
        path_circle = facing.generate(
            wire=self.circle_wire,
            tool_diameter=tool_diameter,
            stepover_percent=stepover_percent,
            start_depth=start_depth,
            final_depth=final_depth,
            start_point=start_point
        )
        
        # Test with spline wire
        path_spline = facing.generate(
            wire=self.spline_wire,
            tool_diameter=tool_diameter,
            stepover_percent=stepover_percent,
            start_depth=start_depth,
            final_depth=final_depth,
            start_point=start_point
        )
        
        # Both should generate valid paths
        self.assertGreater(len(path_circle), 0)
        self.assertGreater(len(path_spline), 0)
        
        # Should have cutting moves
        circle_cuts = [cmd for cmd in path_circle if cmd.Name == "G1"]
        spline_cuts = [cmd for cmd in path_spline if cmd.Name == "G1"]
        
        self.assertGreater(len(circle_cuts), 0)
        self.assertGreater(len(spline_cuts), 0)
        
        # Verify toolpath stays within reasonable bounds of bounding box
        circle_bb = self.circle_wire.BoundBox
        spline_bb = self.spline_wire.BoundBox
        
        for cmd in circle_cuts:
            x = cmd.Parameters.get('X', 0)
            y = cmd.Parameters.get('Y', 0)
            # Should be within extended bounding box
            self.assertGreaterEqual(x, circle_bb.XMin - tool_diameter)
            self.assertLessEqual(x, circle_bb.XMax + tool_diameter)
            self.assertGreaterEqual(y, circle_bb.YMin - tool_diameter)
            self.assertLessEqual(y, circle_bb.YMax + tool_diameter)
        
        for cmd in spline_cuts:
            x = cmd.Parameters.get('X', 0)
            y = cmd.Parameters.get('Y', 0)
            # Should be within extended bounding box
            self.assertGreaterEqual(x, spline_bb.XMin - tool_diameter)
            self.assertLessEqual(x, spline_bb.XMax + tool_diameter)
            self.assertGreaterEqual(y, spline_bb.YMin - tool_diameter)
            self.assertLessEqual(y, spline_bb.YMax + tool_diameter)
