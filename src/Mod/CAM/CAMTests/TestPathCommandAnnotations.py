# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 FreeCAD Contributors                               *
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
import Path
from CAMTests.PathTestUtils import PathTestBase


class TestPathCommandAnnotations(PathTestBase):
    """Test Path.Command annotations functionality."""

    def test00(self):
        """Test basic annotations property access."""
        # Create empty command
        c = Path.Command()
        self.assertIsInstance(c, Path.Command)

        # Test empty annotations
        self.assertEqual(c.Annotations, {})

        # Set annotations via property
        c.Annotations = {"tool": "tap", "material": "steel"}
        self.assertEqual(c.Annotations, {"tool": "tap", "material": "steel"})

        # Test individual annotation access
        self.assertEqual(c.Annotations.get("tool"), "tap")
        self.assertEqual(c.Annotations.get("material"), "steel")
        self.assertIsNone(c.Annotations.get("nonexistent"))

    def test01(self):
        """Test annotations with command creation."""
        # Create command with parameters
        c = Path.Command("G84", {"X": 10, "Y": 20, "Z": -5})

        # Set annotations
        c.Annotations = {"operation": "tapping", "thread": "M6"}

        # Verify command still works normally
        self.assertEqual(c.Name, "G84")
        self.assertEqual(c.Parameters["X"], 10.0)
        self.assertEqual(c.Parameters["Y"], 20.0)
        self.assertEqual(c.Parameters["Z"], -5.0)

        # Verify annotations are preserved
        self.assertEqual(c.Annotations["operation"], "tapping")
        self.assertEqual(c.Annotations["thread"], "M6")

    def test02(self):
        """Test addAnnotations method with dictionary input."""
        c = Path.Command("G1", {"X": 5, "Y": 5})

        # Test method chaining with dictionary
        result = c.addAnnotations({"note": "test note", "tool": "end mill"})

        # Verify method returns the command object for chaining
        self.assertIs(result, c)

        # Verify annotations were set
        self.assertEqual(c.Annotations["note"], "test note")
        self.assertEqual(c.Annotations["tool"], "end mill")

    def test03(self):
        """Test addAnnotations method with string input."""
        c = Path.Command("G2", {"X": 15, "Y": 15})

        # Test method chaining with string
        result = c.addAnnotations("xyz:abc test:1234 operation:milling")

        # Verify method returns the command object for chaining
        self.assertIs(result, c)

        # Verify annotations were parsed and set correctly
        self.assertEqual(c.Annotations["xyz"], "abc")
        self.assertEqual(c.Annotations["test"], 1234)
        self.assertEqual(c.Annotations["operation"], "milling")

    def test04(self):
        """Test annotations update behavior."""
        c = Path.Command("G0", {"Z": 20})

        # Set initial annotations
        c.Annotations = {"initial": "value"}
        self.assertEqual(c.Annotations, {"initial": "value"})

        # Add more annotations - should merge/update
        c.addAnnotations({"additional": "value2", "initial": "updated"})

        expected = {"initial": "updated", "additional": "value2"}
        self.assertEqual(c.Annotations, expected)

    def test05(self):
        """Test method chaining in fluent interface."""
        # Test the fluent interface - create command and set annotations in one line
        c = Path.Command("G84", {"X": 10, "Y": 10, "Z": 0.0}).addAnnotations("thread:M8 depth:15mm")

        # Verify command parameters
        self.assertEqual(c.Name, "G84")
        self.assertEqual(c.Parameters["X"], 10.0)
        self.assertEqual(c.Parameters["Y"], 10.0)
        self.assertEqual(c.Parameters["Z"], 0.0)

        # Verify annotations
        self.assertEqual(c.Annotations["thread"], "M8")
        self.assertEqual(c.Annotations["depth"], "15mm")

    def test06(self):
        """Test annotations with special characters and edge cases."""
        c = Path.Command("G1")

        # Test annotations with special characters
        c.Annotations = {
            "unicode": "café",
            "numbers": "123.45",
            "empty": "",
            "spaces": "value with spaces",
        }

        self.assertEqual(c.Annotations["unicode"], "café")
        self.assertEqual(c.Annotations["numbers"], "123.45")
        self.assertEqual(c.Annotations["empty"], "")
        self.assertEqual(c.Annotations["spaces"], "value with spaces")

    def test07(self):
        """Test annotations persistence through operations."""
        c = Path.Command("G1", {"X": 10, "Y": 20})
        c.Annotations = {"persistent": "value"}

        # Test that annotations survive parameter changes
        c.Parameters = {"X": 30, "Y": 40}
        self.assertEqual(c.Annotations["persistent"], "value")

        # Test that annotations survive name changes
        c.Name = "G2"
        self.assertEqual(c.Annotations["persistent"], "value")

    def test08(self):
        """Test multiple annotation update methods."""
        c = Path.Command()

        # Method 1: Property assignment
        c.Annotations = {"method1": "property"}

        # Method 2: addAnnotations with dict
        c.addAnnotations({"method2": "dict"})

        # Method 3: addAnnotations with string
        c.addAnnotations("method3:string")

        # Verify all methods worked and annotations are merged
        expected = {"method1": "property", "method2": "dict", "method3": "string"}
        self.assertEqual(c.Annotations, expected)

    def test09(self):
        """Test string parsing edge cases."""
        c = Path.Command()

        # Test various string formats
        c.addAnnotations("simple:value")
        self.assertEqual(c.Annotations["simple"], "value")

        # Test multiple key:value pairs
        c.Annotations = {}  # Clear first
        c.addAnnotations("key1:val1 key2:val2 key3:val3")
        expected = {"key1": "val1", "key2": "val2", "key3": "val3"}
        self.assertEqual(c.Annotations, expected)

        # Test that malformed strings are ignored
        c.Annotations = {}  # Clear first
        c.addAnnotations("valid:value invalid_no_colon")
        self.assertEqual(c.Annotations, {"valid": "value"})

    def test10(self):
        """Test annotations in gcode context."""
        # Create a tapping command with annotations
        c = Path.Command(
            "G84", {"X": 25.0, "Y": 30.0, "Z": -10.0, "R": 2.0, "P": 0.5, "F": 100.0}
        ).addAnnotations("operation:tapping thread:M6x1.0 depth:10mm")

        # Verify gcode output is unaffected by annotations
        gcode = c.toGCode()
        self.assertIn("G84", gcode)
        self.assertIn("X25", gcode)
        self.assertIn("Y30", gcode)
        self.assertIn("Z-10", gcode)

        # Verify annotations are preserved
        self.assertEqual(c.Annotations["operation"], "tapping")
        self.assertEqual(c.Annotations["thread"], "M6x1.0")
        self.assertEqual(c.Annotations["depth"], "10mm")

        # Annotations should not appear in gcode output
        self.assertNotIn("operation", gcode)
        self.assertNotIn("tapping", gcode)
        self.assertNotIn("thread", gcode)

    def test11(self):
        """Test save/restore with mixed string and numeric annotations (in-memory)."""
        # Create command with mixed annotations
        original = Path.Command("G1", {"X": 10.0, "Y": 20.0, "F": 1000.0})
        original.Annotations = {
            "tool_name": "6mm_endmill",  # string
            "spindle_speed": 12000.0,  # float
            "feed_rate": 1500,  # int -> float
            "operation": "pocket",  # string
            "depth_of_cut": -2.5,  # negative float
        }

        # Use FreeCAD's in-memory serialization
        content = original.dumpContent()

        # Create new command and restore from memory
        restored = Path.Command()
        restored.restoreContent(content)

        # Verify all annotations were restored with correct types
        self.assertEqual(restored.Annotations["tool_name"], "6mm_endmill")
        self.assertEqual(restored.Annotations["spindle_speed"], 12000.0)
        self.assertEqual(restored.Annotations["feed_rate"], 1500.0)
        self.assertEqual(restored.Annotations["operation"], "pocket")
        self.assertEqual(restored.Annotations["depth_of_cut"], -2.5)

        # Verify types are preserved
        self.assertIsInstance(restored.Annotations["tool_name"], str)
        self.assertIsInstance(restored.Annotations["spindle_speed"], float)
        self.assertIsInstance(restored.Annotations["feed_rate"], float)
        self.assertIsInstance(restored.Annotations["operation"], str)
        self.assertIsInstance(restored.Annotations["depth_of_cut"], float)

        # Verify GCode parameters were also restored correctly
        self.assertEqual(restored.Name, "G1")
        # Note: Parameters are restored via GCode parsing

    def test12(self):
        """Test save/restore with empty and complex annotations (in-memory)."""
        # Test 1: Empty annotations (should work and use compact format)
        simple = Path.Command("G0", {"Z": 5.0})
        self.assertEqual(simple.Annotations, {})

        simple_content = simple.dumpContent()
        simple_restored = Path.Command()
        simple_restored.restoreContent(simple_content)

        self.assertEqual(simple_restored.Annotations, {})
        self.assertEqual(simple_restored.Name, "G0")

        # Test 2: Complex CAM annotations with edge cases
        complex_cmd = Path.Command("G84", {"X": 25.4, "Y": 12.7, "Z": -8.0})
        complex_cmd.Annotations = {
            # Mixed types with edge cases
            "tool_type": "tap",  # string
            "spindle_speed": 500.0,  # float
            "zero_value": 0.0,  # zero
            "negative": -123.456,  # negative
            "large_number": 999999.999,  # large number
            "operation_id": "OP_030",  # alphanumeric string
            "thread_spec": "M4x0.7",  # string with numbers
            "scientific": 1.23e-6,  # scientific notation
        }

        # Serialize and restore
        complex_content = complex_cmd.dumpContent()
        complex_restored = Path.Command()
        complex_restored.restoreContent(complex_content)

        # Verify all complex data restored correctly
        self.assertEqual(len(complex_restored.Annotations), 8)

        # Check specific values and types
        self.assertEqual(complex_restored.Annotations["tool_type"], "tap")
        self.assertIsInstance(complex_restored.Annotations["tool_type"], str)

        self.assertEqual(complex_restored.Annotations["spindle_speed"], 500.0)
        self.assertIsInstance(complex_restored.Annotations["spindle_speed"], float)

        self.assertEqual(complex_restored.Annotations["zero_value"], 0.0)
        self.assertEqual(complex_restored.Annotations["negative"], -123.456)
        self.assertEqual(complex_restored.Annotations["large_number"], 999999.999)

        # Verify strings with numbers stay as strings
        self.assertEqual(complex_restored.Annotations["operation_id"], "OP_030")
        self.assertEqual(complex_restored.Annotations["thread_spec"], "M4x0.7")
        self.assertIsInstance(complex_restored.Annotations["operation_id"], str)
        self.assertIsInstance(complex_restored.Annotations["thread_spec"], str)

        # Check scientific notation
        self.assertAlmostEqual(complex_restored.Annotations["scientific"], 1.23e-6, places=8)
        self.assertIsInstance(complex_restored.Annotations["scientific"], float)
