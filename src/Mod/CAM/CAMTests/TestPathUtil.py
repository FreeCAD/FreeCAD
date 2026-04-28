# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
import Path.Base.Util as PathUtil
import Path.Op.Base as PathOpBase
import TestSketcherApp

from CAMTests.PathTestUtils import PathTestBase


class TestPathUtil(PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathUtils")

    def tearDown(self):
        FreeCAD.closeDocument("TestPathUtils")

    def test00(self):
        """Check that isValidBaseObject detects solids."""
        box = self.doc.addObject("Part::Box", "Box")
        cylinder = self.doc.addObject("Part::Cylinder", "Cylinder")
        self.doc.recompute()
        self.assertTrue(PathUtil.isValidBaseObject(box))
        self.assertTrue(PathUtil.isValidBaseObject(cylinder))

    def test01(self):
        """Check that isValidBaseObject detects PDs."""
        body = self.doc.addObject("PartDesign::Body", "Body")
        box = self.doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        self.doc.recompute()
        self.assertTrue(PathUtil.isValidBaseObject(body))

    def test02(self):
        """Check that isValidBaseObject detects compounds."""
        box = self.doc.addObject("Part::Box", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 1
        box.Placement = FreeCAD.Placement(
            FreeCAD.Vector(-5, -5, 0), FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
        )
        cyl = self.doc.addObject("Part::Cylinder", "Cylinder")
        cyl.Radius = 1
        cyl.Height = 10
        box.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0, 0, -5), FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
        )
        cut = self.doc.addObject("Part::Cut", "Cut")
        cut.Base = box
        cut.Tool = cyl
        self.doc.recompute()
        self.assertTrue(PathUtil.isValidBaseObject(cut))

    def test03(self):
        """Check that isValidBaseObject ignores sketches."""
        body = self.doc.addObject("PartDesign::Body", "Body")
        sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        body.addObject(sketch)
        TestSketcherApp.CreateSlotPlateSet(sketch)
        self.doc.recompute()
        pad = self.doc.addObject("PartDesign::Pad", "Pad")
        body.addObject(pad)
        pad.Profile = sketch
        self.doc.recompute()

        # the body is a solid
        self.assertTrue(PathUtil.isValidBaseObject(body))

        # the pad inside the body cannot be used due to the linking constraints
        self.assertFalse(PathUtil.isValidBaseObject(pad))

        # the sketch is no good neither
        self.assertFalse(PathUtil.isValidBaseObject(sketch))

    def test04(self):
        """Check that Part is handled correctly."""
        part = self.doc.addObject("App::Part", "Part")

        # an empty part is not a valid base object
        self.assertFalse(PathUtil.isValidBaseObject(part))

        # a non-empty part where none of the objects have a shape, is no good either
        fp = self.doc.addObject("App::FeaturePython", "Feature")
        part.addObject(fp)
        self.assertFalse(PathUtil.isValidBaseObject(part))

        # create a valid base object
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()
        self.assertTrue(PathUtil.isValidBaseObject(box))

        # a part with at least one valid object is valid
        part.addObject(box)
        self.assertTrue(PathUtil.isValidBaseObject(part))

        # however, the object itself is no longer valid
        self.assertFalse(PathUtil.isValidBaseObject(box))


class TestCompass(PathTestBase):
    """Test the Compass helper class for climb/conventional milling."""

    def setUp(self):
        # Spindle directions for testing
        self.spindle_forward = "Forward"
        self.spindle_reverse = "Reverse"
        self.spindle_none = "None"
        super().setUp()

    def test_compass_initialization(self):
        """Test Compass class initialization."""
        compass = PathOpBase.Compass(self.spindle_forward)

        # Check default values
        self.assertEqual(compass.spindle_dir, self.spindle_forward)
        self.assertEqual(compass.cut_side, "Outside")
        self.assertEqual(compass.cut_mode, "Climb")

    def test_spindle_direction_property(self):
        """Test spindle direction property handling."""
        # Test forward spindle
        compass_forward = PathOpBase.Compass(self.spindle_forward)
        self.assertEqual(compass_forward.spindle_dir, "Forward")

        # Test reverse spindle
        compass_reverse = PathOpBase.Compass(self.spindle_reverse)
        self.assertEqual(compass_reverse.spindle_dir, "Reverse")

        # Test none/unknown spindle
        compass_none = PathOpBase.Compass(self.spindle_none)
        self.assertEqual(compass_none.spindle_dir, "None")

        # Test setting spindle direction
        compass = PathOpBase.Compass("Forward")
        compass.spindle_dir = "Reverse"
        self.assertEqual(compass.spindle_dir, "Reverse")

        # Test invalid spindle direction defaults to None
        compass.spindle_dir = "Invalid"
        self.assertEqual(compass.spindle_dir, "None")

    def test_cut_side_property(self):
        """Test cut side property and setter."""
        compass = PathOpBase.Compass(self.spindle_forward)

        # Test default
        self.assertEqual(compass.cut_side, "Outside")

        # Test setting inside
        compass.cut_side = "inside"
        self.assertEqual(compass.cut_side, "Inside")

        # Test setting outside
        compass.cut_side = "outside"
        self.assertEqual(compass.cut_side, "Outside")

    def test_cut_mode_property(self):
        """Test cut mode property and setter."""
        compass = PathOpBase.Compass(self.spindle_forward)

        # Test default
        self.assertEqual(compass.cut_mode, "Climb")

        # Test setting conventional
        compass.cut_mode = "conventional"
        self.assertEqual(compass.cut_mode, "Conventional")

        # Test setting climb
        compass.cut_mode = "climb"
        self.assertEqual(compass.cut_mode, "Climb")

    def test_path_direction_calculation(self):
        """Test path direction calculation logic."""
        # Test Forward spindle, Outside cut, Climb mode -> CW path
        compass = PathOpBase.Compass(self.spindle_forward)
        compass.cut_side = "Outside"
        compass.cut_mode = "Climb"
        self.assertEqual(compass.path_dir, "CW")

        # Test Forward spindle, Inside cut, Climb mode -> CCW path
        compass.cut_side = "Inside"
        compass.cut_mode = "Climb"
        self.assertEqual(compass.path_dir, "CCW")

        # Test Reverse spindle, Outside cut, Climb mode -> CCW path
        compass_reverse = PathOpBase.Compass(self.spindle_reverse)
        compass_reverse.cut_side = "Outside"
        compass_reverse.cut_mode = "Climb"
        self.assertEqual(compass_reverse.path_dir, "CCW")

        # Test Reverse spindle, Inside cut, Climb mode -> CW path
        compass_reverse.cut_side = "Inside"
        compass_reverse.cut_mode = "Climb"
        self.assertEqual(compass_reverse.path_dir, "CW")

    def test_conventional_milling_path_direction(self):
        """Test path direction for conventional milling."""
        # Test Forward spindle, Outside cut, Conventional mode -> CCW path
        compass = PathOpBase.Compass(self.spindle_forward)
        compass.cut_side = "Outside"
        compass.cut_mode = "Conventional"
        self.assertEqual(compass.path_dir, "CCW")

        # Test Forward spindle, Inside cut, Conventional mode -> CW path
        compass.cut_side = "Inside"
        compass.cut_mode = "Conventional"
        self.assertEqual(compass.path_dir, "CW")

    def test_unknown_spindle_direction(self):
        """Test behavior with unknown spindle direction."""
        compass = PathOpBase.Compass(self.spindle_none)
        self.assertEqual(compass.path_dir, "UNKNOWN")

    def test_expected_cut_mode_lookup(self):
        """Test the internal _expected_cut_mode lookup table."""
        compass = PathOpBase.Compass(self.spindle_forward)

        # Test lookup table entries for climb milling
        self.assertEqual(compass._expected_cut_mode("Inside", "CW", "CCW"), "Climb")
        self.assertEqual(compass._expected_cut_mode("Inside", "CCW", "CW"), "Climb")
        self.assertEqual(compass._expected_cut_mode("Outside", "CW", "CW"), "Climb")
        self.assertEqual(compass._expected_cut_mode("Outside", "CCW", "CCW"), "Climb")

        # Test default to conventional for other combinations
        self.assertEqual(compass._expected_cut_mode("Outside", "CW", "CCW"), "Conventional")
        self.assertEqual(compass._expected_cut_mode("Inside", "CW", "CW"), "Conventional")

    def test_rotation_from_spindle(self):
        """Test spindle direction to rotation mapping."""
        compass = PathOpBase.Compass(self.spindle_forward)

        self.assertEqual(compass._rotation_from_spindle("Forward"), "CW")
        self.assertEqual(compass._rotation_from_spindle("Reverse"), "CCW")

    def test_report_functionality(self):
        """Test the report method returns correct data."""
        compass = PathOpBase.Compass(self.spindle_forward)
        compass.cut_side = "Inside"
        compass.cut_mode = "Conventional"

        report = compass.report()

        expected = {
            "spindle_dir": "Forward",
            "cut_side": "Inside",
            "cut_mode": "Conventional",
            "operation_type": "Perimeter",
            "path_dir": "CW",
        }

        self.assertEqual(report, expected)

    def test_area_operation_initialization(self):
        """Test Compass class initialization for area operations."""
        compass = PathOpBase.Compass(self.spindle_forward, "Area")

        # Check values for area operations
        self.assertEqual(compass.spindle_dir, "Forward")
        self.assertEqual(compass.operation_type, "Area")
        self.assertEqual(compass.cut_mode, "Climb")
        self.assertEqual(compass.path_dir, "N/A")  # Not applicable for area operations

    def test_operation_type_property(self):
        """Test operation type property and setter."""
        compass = PathOpBase.Compass(self.spindle_forward)

        # Test default (perimeter for backward compatibility)
        self.assertEqual(compass.operation_type, "Perimeter")

        # Test setting area
        compass.operation_type = "area"
        self.assertEqual(compass.operation_type, "Area")
        self.assertEqual(compass.path_dir, "N/A")

        # Test setting back to perimeter
        compass.operation_type = "perimeter"
        self.assertEqual(compass.operation_type, "Perimeter")
        self.assertNotEqual(compass.path_dir, "N/A")

    def test_step_direction_for_area_operations(self):
        """Test step direction calculation for area operations."""
        compass = PathOpBase.Compass(self.spindle_forward, "Area")
        compass.cut_mode = "Climb"

        # Test X- approach with climb milling
        step_dir = compass.get_step_direction("X-")
        self.assertTrue(isinstance(step_dir, bool))

        # Test conventional milling gives different result
        compass.cut_mode = "Conventional"
        step_dir_conv = compass.get_step_direction("X-")
        self.assertNotEqual(step_dir, step_dir_conv)

        # Test with reverse spindle
        compass_reverse = PathOpBase.Compass(self.spindle_reverse, "Area")
        step_dir_reverse = compass_reverse.get_step_direction("X-")
        self.assertTrue(isinstance(step_dir_reverse, bool))

    def test_cutting_direction_for_area_operations(self):
        """Test cutting direction calculation for area operations."""
        compass = PathOpBase.Compass(self.spindle_forward, "Area")
        compass.cut_mode = "Climb"

        # Test zigzag pattern alternates direction
        cut_dir_0 = compass.get_cutting_direction("X-", 0, "zigzag")
        cut_dir_1 = compass.get_cutting_direction("X-", 1, "zigzag")
        self.assertNotEqual(cut_dir_0, cut_dir_1)

        # Test unidirectional pattern keeps same direction
        cut_dir_uni_0 = compass.get_cutting_direction("X-", 0, "unidirectional")
        cut_dir_uni_1 = compass.get_cutting_direction("X-", 1, "unidirectional")
        self.assertEqual(cut_dir_uni_0, cut_dir_uni_1)

    def test_area_operation_error_handling(self):
        """Test error handling for area operation methods on perimeter operations."""
        compass = PathOpBase.Compass(self.spindle_forward, "Perimeter")

        # Should raise error when calling area methods on perimeter operation
        with self.assertRaises(ValueError):
            compass.get_step_direction("X-")

        with self.assertRaises(ValueError):
            compass.get_cutting_direction("X-")

    def test_backward_compatibility(self):
        """Test that existing code still works (backward compatibility)."""
        # Old style initialization (no operation_type parameter)
        compass = PathOpBase.Compass(self.spindle_forward)

        # Should default to perimeter operation
        self.assertEqual(compass.operation_type, "Perimeter")

        # All existing functionality should work
        self.assertEqual(compass.spindle_dir, "Forward")
        self.assertEqual(compass.cut_side, "Outside")
        self.assertEqual(compass.cut_mode, "Climb")
        self.assertEqual(compass.path_dir, "CW")

        # Report should include operation_type
        report = compass.report()
        self.assertIn("operation_type", report)
        self.assertEqual(report["operation_type"], "Perimeter")
