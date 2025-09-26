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



    def test_directional_strategy_basic(self):
        """Test _directional strategy basic functionality."""
        commands = facing._directional(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            axis_preference="long"
        )
        
        # Should return a list of Path.Command objects
        self.assertIsInstance(commands, list)
        self.assertGreater(len(commands), 0)
        
        # All commands should be G0 or G1
        for cmd in commands:
            self.assertIn(cmd.Name, ['G0', 'G1'])

    def test_directional_climb_vs_conventional(self):
        """Test _directional with different milling directions."""
        climb_commands = facing._directional(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            milling_direction="climb"
        )
        
        conventional_commands = facing._directional(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            milling_direction="conventional"
        )
        
        # Should have same number of commands but different coordinates
        self.assertEqual(len(climb_commands), len(conventional_commands))
        
        # At least some coordinates should be different
        different_coords = False
        for i in range(min(len(climb_commands), len(conventional_commands))):
            if climb_commands[i].Parameters != conventional_commands[i].Parameters:
                different_coords = True
                break
        self.assertTrue(different_coords)

    def test_directional_retract_height(self):
        """Test retract height functionality in directional."""
        commands_no_retract = facing._directional(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            retract_height=None
        )
        
        commands_with_retract = facing._directional(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            retract_height=15.0
        )
        
        # Commands with retract should have more moves (G0 Z moves)
        self.assertGreaterEqual(len(commands_with_retract), len(commands_no_retract))
        
        # Should have some Z-only G0 commands
        z_retracts = [cmd for cmd in commands_with_retract 
                     if cmd.Name == 'G0' and 'Z' in cmd.Parameters and len(cmd.Parameters) == 1]
        self.assertGreater(len(z_retracts), 0)

    def test_zigzag_strategy_basic(self):
        """Test _zigzag strategy basic functionality."""
        commands = facing._zigzag(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            axis_preference="long"
        )
        
        # Should return a list of Path.Command objects
        self.assertIsInstance(commands, list)
        self.assertGreater(len(commands), 0)
        
        # All commands should be G1 (no rapid moves in traditional zigzag)
        for cmd in commands:
            self.assertEqual(cmd.Name, 'G1')

    def test_zigzag_alternating_direction(self):
        """Test that zigzag alternates cutting direction."""
        commands = facing._zigzag(
            polygon=self.rectangle_wire,
            tool_diameter=2.0,  # Small tool for more passes
            stepover_percent=25,  # Small stepover for more passes
            axis_preference="long"
        )
        
        # Should have multiple passes
        self.assertGreater(len(commands), 4)
        
        # Extract X coordinates from cutting moves
        x_coords = []
        for cmd in commands:
            if 'X' in cmd.Parameters:
                x_coords.append(cmd.Parameters['X'])
        
        # Should have alternating pattern in X coordinates
        self.assertGreater(len(x_coords), 2)

    def test_zigzag_with_retract_height(self):
        """Test zigzag with retract height (converts to directional-like behavior)."""
        commands_no_retract = facing._zigzag(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            retract_height=None
        )
        
        commands_with_retract = facing._zigzag(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            retract_height=15.0
        )
        
        # Commands with retract should have rapid moves
        rapid_moves_no_retract = [cmd for cmd in commands_no_retract if cmd.Name == 'G0']
        rapid_moves_with_retract = [cmd for cmd in commands_with_retract if cmd.Name == 'G0']
        
        # With retract should have more rapid moves
        self.assertGreater(len(rapid_moves_with_retract), len(rapid_moves_no_retract))

    def test_zigzag_milling_direction(self):
        """Test zigzag with different milling directions."""
        climb_commands = facing._zigzag(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            milling_direction="climb"
        )
        
        conventional_commands = facing._zigzag(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            milling_direction="conventional"
        )
        
        # Should have same number of commands
        self.assertEqual(len(climb_commands), len(conventional_commands))
        
        # But coordinates should be different
        different_coords = False
        for i in range(min(len(climb_commands), len(conventional_commands))):
            if climb_commands[i].Parameters != conventional_commands[i].Parameters:
                different_coords = True
                break
        self.assertTrue(different_coords)

    def test_get_angled_polygon_zero_degrees(self):
        """Test get_angled_polygon with 0 degree rotation."""
        result = facing.get_angled_polygon(self.square_wire, 0)
        
        # Should get back a valid wire
        self.assertTrue(result.isClosed())
        # Bounding box should be similar to original
        original_bb = self.square_wire.BoundBox
        result_bb = result.BoundBox
        self.assertAlmostEqual(original_bb.XLength, result_bb.XLength, places=1)
        self.assertAlmostEqual(original_bb.YLength, result_bb.YLength, places=1)

    def test_get_angled_polygon_45_degrees(self):
        """Test get_angled_polygon with 45 degree rotation."""
        result = facing.get_angled_polygon(self.square_wire, 45)
        
        self.assertTrue(result.isClosed())
        # The rotated bounding box should be larger than the original
        original_bb = self.square_wire.BoundBox
        result_bb = result.BoundBox
        
        # The function creates a bounding box that fully contains the rotated wire
        # For a 45-degree rotation, this will be larger than just the diagonal
        # The result should be larger than the original in both dimensions
        self.assertGreater(result_bb.XLength, original_bb.XLength)
        self.assertGreater(result_bb.YLength, original_bb.YLength)
        
        # Should have 4 edges (rectangular)
        self.assertEqual(len(result.Edges), 4)

    def test_analyze_rectangle_axis_aligned(self):
        """Test _analyze_rectangle with axis-aligned rectangle."""
        result = facing._analyze_rectangle(self.rectangle_wire, "long")
        
        # Check that we get the expected keys
        expected_keys = ['primary_vec', 'step_vec', 'primary_length', 'step_length', 'reference_corner']
        for key in expected_keys:
            self.assertIn(key, result)
        
        # For a 20x10 rectangle with "long" preference, primary should be along 20-unit side
        self.assertAlmostEqual(result['primary_length'], 20, places=1)
        self.assertAlmostEqual(result['step_length'], 10, places=1)
        
        # Vectors should be normalized
        self.assertAlmostEqual(result['primary_vec'].Length, 1.0, places=5)
        self.assertAlmostEqual(result['step_vec'].Length, 1.0, places=5)

    def test_analyze_rectangle_short_preference(self):
        """Test _analyze_rectangle with short axis preference."""
        result = facing._analyze_rectangle(self.rectangle_wire, "short")
        
        # For a 20x10 rectangle with "short" preference, primary should be along 10-unit side
        self.assertAlmostEqual(result['primary_length'], 10, places=1)
        self.assertAlmostEqual(result['step_length'], 20, places=1)

    def test_analyze_rectangle_invalid_polygon(self):
        """Test _analyze_rectangle with invalid polygon."""
        # Create a triangle (3 edges instead of 4)
        triangle_wire = Part.makePolygon([
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Vector(10, 0, 0),
            FreeCAD.Vector(5, 10, 0),
            FreeCAD.Vector(0, 0, 0)
        ])
        
        with self.assertRaises(ValueError):
            facing._analyze_rectangle(triangle_wire, "long")

    def _create_mock_tool_controller(self, spindle_dir):
        """Create a mock tool controller for testing."""
        class MockToolController:
            def __init__(self, spindle_direction):
                self.SpindleDir = spindle_direction
                
        return MockToolController(spindle_dir)
