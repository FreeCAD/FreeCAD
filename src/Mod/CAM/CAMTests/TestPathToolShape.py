# -*- coding: utf-8 -*-
# Unit tests for the Path.Tool.Shape module and its utilities.

import CAMTests.PathTestUtils as PathTestUtils
import unittest
import os
import sys
from pathlib import Path
from unittest.mock import patch, MagicMock, mock_open, call

# Mock FreeCAD modules before they are imported by the module under test
# This is crucial for running tests outside the FreeCAD environment
mock_freecad = MagicMock(Name="FreeCAD_Mock")
mock_units = MagicMock(Name="Units_Mock")

# Mock Console methods
mock_freecad.Console = MagicMock()
mock_freecad.Console.PrintWarning = MagicMock()
mock_freecad.Console.PrintError = MagicMock()


# Simple mock class for FreeCAD.Units.Quantity
class MockQuantity:
    def __init__(self, value, unit=None):
        self.value = value
        self.unit = unit
        self.val_str = f"{value} {unit}" if unit else str(value)

    def __str__(self):
        return self.val_str

    def __repr__(self):
        return f"<MockQuantity '{self.val_str}'>"

    def __eq__(self, other):
        if isinstance(other, str):
            return self.val_str == other
        # Allow comparison with other MockQuantity instances
        if isinstance(other, MockQuantity):
            return self.value == other.value and self.unit == other.unit
        return NotImplemented


# Apply the mock class to Units.Quantity
mock_units.Quantity = MockQuantity

# Mock Document/Object structure minimally
mock_doc = MagicMock(Name="Document_Mock")
mock_obj = MagicMock(Name="Object_Mock")
mock_obj.Label = "MockObjectLabel"
mock_obj.Name = "MockObjectName"
mock_doc.Objects = [mock_obj]

# Mock FreeCAD functions used by util
mock_freecad.openDocument = MagicMock(return_value=mock_doc, name="openDocument_Mock")
mock_freecad.openDocument.configure_mock(**{'Hidden': True})
mock_freecad.closeDocument = MagicMock()

from Path.Tool.Shape import (
    ToolBitShape, util, ToolBitShapeBallEnd, ToolBitShapeChamfer,
    ToolBitShapeDovetail, ToolBitShapeDrill, ToolBitShapeEndMill,
    ToolBitShapeProbe, ToolBitShapeReamer, ToolBitShapeSlittingSaw,
    ToolBitShapeTap, ToolBitShapeThreadMill, ToolBitShapeTorus,
    ToolBitShapeVBit, TOOL_BIT_SHAPE_CLASSES, get_shape_class
)

# Helper dummy class for testing abstract methods


class DummyShape(ToolBitShape):
    _LABELS = {"Param1": "Parameter 1", "Param2": "Parameter 2"}

    def set_default_parameters(self):
        self._params = {
            "Param1": (mock_units.Quantity("10", "mm"),
                       'App::PropertyLength'),
            "Param2": (mock_units.Quantity("5", "deg"),
                       'App::PropertyAngle'),
        }


@patch.dict(sys.modules, {'FreeCAD': mock_freecad, 'FreeCAD.Units': mock_units})
class TestPathToolShapeBase(PathTestUtils.PathTestBase):
    """Tests for the ToolBitShape abstract base class."""

    def setUp(self):
        """Reset mocks before each test."""
        mock_freecad.Console.PrintWarning.reset_mock()
        mock_freecad.Console.PrintError.reset_mock()
        mock_freecad.openDocument.reset_mock()
        mock_freecad.closeDocument.reset_mock()
        # Reset mock object properties if needed for specific tests
        mock_obj.reset_mock()
        mock_obj.Label = "MockObjectLabel"
        mock_obj.Name = "MockObjectName"
        # Clear any dynamically added attributes from previous tests
        for attr in list(mock_obj.__dict__.keys()):
            if not attr.startswith('_') and attr not in [
                    'Label', 'Name', 'reset_mock']:
                try:
                    delattr(mock_obj, attr)
                except AttributeError:
                    pass  # Ignore errors if attribute is already gone

    def test_base_init_with_defaults(self):
        """Test base class initialization uses default parameters."""
        shape = DummyShape()
        params = shape.get_parameters()
        self.assertEqual(params["Param1"], "10 mm")
        self.assertEqual(params["Param2"], "5 deg")

    def test_base_init_with_kwargs(self):
        """Test base class initialization overrides defaults with kwargs."""
        shape = DummyShape(Param1="20 mm", Param3="Ignored")
        params = shape.get_parameters()
        self.assertEqual(params["Param1"], "20 mm")
        self.assertEqual(params["Param2"], "5 deg")  # Should remain default
        # Check warning for ignored param
        mock_freecad.Console.PrintWarning.assert_called_once()
        call_args, _ = mock_freecad.Console.PrintWarning.call_args
        self.assertIn("Ignoring unknown parameter 'Param3'", call_args[0])
        self.assertIn("DummyShape", call_args[0])

    def test_base_get_set_parameter(self):
        """Test getting and setting individual parameters."""
        shape = DummyShape()
        self.assertEqual(shape.get_parameter("Param1"), "10 mm")
        shape.set_parameter("Param1", "15 mm")
        self.assertEqual(shape.get_parameter("Param1"), "15 mm")
        with self.assertRaisesRegex(KeyError, "ToolBitShape 'DummyShape' has no parameter 'InvalidParam'"):
            shape.get_parameter("InvalidParam")
        with self.assertRaisesRegex(KeyError, "ToolBitShape 'DummyShape' has no parameter 'InvalidParam'"):
            shape.set_parameter("InvalidParam", "1")

    def test_base_get_parameters(self):
        """Test getting the full parameter dictionary."""
        shape = DummyShape(Param1="12 mm")
        expected = {"Param1": "12 mm", "Param2": "5 deg"}
        self.assertEqual(shape.get_parameters(), expected)
        # Ensure it's a copy
        params = shape.get_parameters()
        params["Param1"] = "99 mm"
        self.assertEqual(shape.get_parameter("Param1"), "12 mm")

    def test_base_name_property(self):
        """Test the name property returns the class name."""
        shape = DummyShape()
        self.assertEqual(shape.name, "DummyShape")

    def test_base_get_parameter_label(self):
        """Test retrieving parameter labels."""
        shape = DummyShape()
        self.assertEqual(shape.get_parameter_label("Param1"), "Parameter 1")
        self.assertEqual(shape.get_parameter_label("Param2"), "Parameter 2")
        # Test fallback for unknown parameter
        self.assertEqual(
            shape.get_parameter_label("UnknownParam"),
            "UnknownParam")

    def test_base_get_expected_parameter_names(self):
        """Test retrieving the list of expected parameter names."""
        expected = ["Param1", "Param2"]
        self.assertCountEqual(
            DummyShape.get_expected_parameter_names(), expected)

    def test_base_str_repr(self):
        """Test string representation."""
        shape = DummyShape()
        expected_str = "DummyShape(Param1=10 mm, Param2=5 deg)"
        self.assertEqual(str(shape), expected_str)
        self.assertEqual(repr(shape), expected_str)

    @patch('os.path.exists', return_value=True)
    @patch.object(util, 'find_shape_object')
    @patch.object(ToolBitShape, 'get_expected_parameter_names')
    def test_base_validate_success(
            self,
            mock_get_expected,
            mock_find_obj,
            mock_exists):
        """Test ToolBitShape.validate success path."""
        mock_get_expected.return_value = ["Param1", "Param2"]
        mock_find_obj.return_value = mock_obj
        # Ensure mock object has the expected parameters
        setattr(mock_obj, "Param1", "10 mm")
        setattr(mock_obj, "Param2", "5 deg")
        filepath = Path("/fake/valid.fcstd")

        result = DummyShape.validate(filepath)

        self.assertTrue(result)
        mock_exists.assert_called_once_with(filepath)
        mock_freecad.openDocument.assert_called_once_with(
            filepath, Hidden=True)
        mock_find_obj.assert_called_once_with(mock_doc)
        mock_freecad.closeDocument.assert_called_once_with(mock_doc.Name)
        mock_freecad.Console.PrintError.assert_not_called()
        mock_freecad.Console.PrintWarning.assert_not_called()

    @patch('os.path.exists', return_value=False)
    def test_base_validate_file_not_found(self, mock_exists):
        """Test ToolBitShape.validate file not found."""
        filepath = Path("/fake/nonexistent.fcstd")
        result = DummyShape.validate(filepath)
        self.assertFalse(result)
        mock_exists.assert_called_once_with(filepath)
        mock_freecad.Console.PrintError.assert_called_once()
        self.assertIn(
            "File not found",
            mock_freecad.Console.PrintError.call_args[0][0])
        mock_freecad.openDocument.assert_not_called()

    @patch('os.path.exists', return_value=True)
    def test_base_validate_open_fails(self, mock_exists):
        """Test ToolBitShape.validate handles document open failure."""
        mock_freecad.openDocument.return_value = None  # Simulate failure
        filepath = Path("/fake/fail_open.fcstd")
        result = DummyShape.validate(filepath)
        self.assertFalse(result)
        mock_exists.assert_called_once_with(filepath)
        mock_freecad.openDocument.assert_called_once_with(
            filepath, Hidden=True)
        mock_freecad.Console.PrintError.assert_called_once()
        self.assertIn("Failed to open document",
                      mock_freecad.Console.PrintError.call_args[0][0])
        mock_freecad.closeDocument.assert_not_called()  # Should fail before close

    @patch('os.path.exists', return_value=True)
    @patch.object(util, 'find_shape_object', return_value=None)
    def test_base_validate_no_object(self, mock_find_obj, mock_exists):
        """Test ToolBitShape.validate handles no suitable object found."""
        mock_freecad.openDocument.return_value = mock_doc  # Ensure open succeeds
        filepath = Path("/fake/no_object.fcstd")
        result = DummyShape.validate(filepath)
        self.assertFalse(result)
        mock_exists.assert_called_once_with(filepath)
        mock_freecad.openDocument.assert_called_once_with(
            filepath, Hidden=True)
        mock_find_obj.assert_called_once_with(mock_doc)
        mock_freecad.Console.PrintError.assert_called_once()
        self.assertIn("No suitable shape object found",
                      mock_freecad.Console.PrintError.call_args[0][0])
        mock_freecad.closeDocument.assert_called_once_with(
            mock_doc.Name)  # Closed in finally

    @patch('os.path.exists', return_value=True)
    @patch.object(util, 'find_shape_object')
    @patch.object(ToolBitShape, 'get_expected_parameter_names')
    def test_base_validate_missing_params(
            self,
            mock_get_expected,
            mock_find_obj,
            mock_exists):
        """Test ToolBitShape.validate handles missing parameters on object."""
        mock_get_expected.return_value = ["Param1", "Param2", "MissingParam"]
        mock_find_obj.return_value = mock_obj
        # Object only has Param1 and Param2
        setattr(mock_obj, "Param1", "10 mm")
        setattr(mock_obj, "Param2", "5 deg")
        # delattr(mock_obj, "MissingParam") # Ensure it's not there
        filepath = Path("/fake/missing_params.fcstd")

        result = DummyShape.validate(filepath)

        # Currently validation fails if params missing
        self.assertFalse(result)
        mock_exists.assert_called_once_with(filepath)
        mock_freecad.openDocument.assert_called_once_with(
            filepath, Hidden=True)
        mock_find_obj.assert_called_once_with(mock_doc)
        mock_freecad.Console.PrintWarning.assert_called_once()
        self.assertIn("is missing parameters",
                      mock_freecad.Console.PrintWarning.call_args[0][0])
        self.assertIn(
            "MissingParam",
            mock_freecad.Console.PrintWarning.call_args[0][0])
        mock_freecad.closeDocument.assert_called_once_with(mock_doc.Name)

    @patch.object(util, 'load_doc_and_get_properties')
    @patch.object(ToolBitShape, 'get_expected_parameter_names')
    def test_base_from_file_success(self, mock_get_expected, mock_load_props):
        """Test ToolBitShape.from_file success path."""
        mock_get_expected.return_value = ["Param1", "Param2"]
        # Simulate load_doc_and_get_properties returning the doc and params
        loaded_params = {"Param1": "Loaded 15 mm", "Param2": "Loaded 3 deg"}
        mock_load_props.return_value = (mock_doc, loaded_params)
        filepath = Path("/fake/load_success.fcstd")

        instance = DummyShape.from_file(filepath)

        mock_get_expected.assert_called_once()
        mock_load_props.assert_called_once_with(filepath, ["Param1", "Param2"])
        self.assertIsInstance(instance, DummyShape)
        self.assertEqual(instance.get_parameter("Param1"), "Loaded 15 mm")
        self.assertEqual(instance.get_parameter("Param2"), "Loaded 3 deg")
        # Ensure doc is closed (happens in the finally block of from_file)
        mock_freecad.closeDocument.assert_called_once_with(mock_doc.Name)

    @patch.object(util, 'load_doc_and_get_properties')
    def test_base_from_file_load_fails(self, mock_load_props):
        """Test ToolBitShape.from_file handles exceptions from loading."""
        # Simulate load_doc_and_get_properties raising an error
        filepath = Path("/fake/load_fail.fcstd")
        mock_load_props.side_effect = ValueError("Failed to load")

        with self.assertRaisesRegex(ValueError, "Failed to load"):
            DummyShape.from_file(filepath)

        mock_load_props.assert_called_once_with(
            filepath, DummyShape.get_expected_parameter_names())
        # Ensure doc is closed even if load_doc_and_get_properties fails partially
        # (depends on *where* it fails, but the finally block should try)
        # If load_doc_and_get_properties returns doc=None due to early failure,
        # closeDocument won't be called. If it returns a doc before failing, it should.
        # Let's assume it might return a doc before failing in some cases.
        # If mock_load_props raises *before* returning, close won't be called.
        # If mock_load_props returns (doc, params) and *then* an error occurs
        # *within* from_file (unlikely here), close *would* be called.
        # Given the structure, if load_doc fails, it likely raises before
        # returning doc.
        mock_freecad.closeDocument.assert_not_called()  # Adjusted expectation


@patch.dict(sys.modules, {'FreeCAD': mock_freecad, 'FreeCAD.Units': mock_units})
class TestPathToolShapeClasses(PathTestUtils.PathTestBase):
    """Tests for the concrete ToolBitShape subclasses."""

    def test_concrete_classes_instantiation(self):
        """Test that all concrete classes can be instantiated."""
        for name, cls in TOOL_BIT_SHAPE_CLASSES.items():
            with self.subTest(shape=name):
                try:
                    instance = cls()
                    self.assertIsInstance(
                        instance, ToolBitShape)
                    # Check if default params were set
                    self.assertTrue(instance.get_parameters())
                except Exception as e:
                    self.fail(f"Failed to instantiate {cls.__name__}: {e}")

    def test_toolbitshapeballend_defaults(self):
        """Test ToolBitShapeBallEnd default parameters and labels."""
        with patch('Path.Tool.Shape.base.FreeCAD', new=mock_freecad):
            shape = ToolBitShapeBallEnd()
            shape = ToolBitShapeBallEnd()
            params = shape.get_parameters()
            self.assertEqual(str(params["Diameter"]), "6.0 mm")
            self.assertEqual(str(params["Length"]), "50.0 mm")
            self.assertEqual(shape.get_parameter_label("Diameter"), "Diameter")
            self.assertEqual(
                shape.get_parameter_label("Length"),
                "Overall Tool Length")

    # Add similar tests for other concrete shapes (Chamfer, Drill, etc.)
    # focusing on a couple of key default parameters and labels per shape.
    # Example:
    def test_toolbitshapedrill_defaults(self):
        """Test ToolBitShapeDrill default parameters and labels."""
        with patch('Path.Tool.Shape.base.FreeCAD', new=mock_freecad):
            shape = ToolBitShapeDrill()
            params = shape.get_parameters()
            self.assertEqual(str(params["Diameter"]), "5.0 mm")
            self.assertEqual(str(params["TipAngle"]), "118.0 deg")
            self.assertEqual(shape.get_parameter_label("Diameter"), "Diameter")
            self.assertEqual(shape.get_parameter_label("TipAngle"), "Tip angle")

    def test_toolbitshapechamfer_defaults(self):
        """Test ToolBitShapeChamfer default parameters and labels."""
        with patch('Path.Tool.Shape.base.FreeCAD', new=mock_freecad):
            shape = ToolBitShapeChamfer()
            params = shape.get_parameters()
            self.assertEqual(str(params["Diameter"]), "6.0 mm")
            self.assertEqual(str(params["Radius"]), "1.0 mm")
            self.assertEqual(shape.get_parameter_label("Radius"), "Radius")

    def test_toolbitshapedovetail_defaults(self):
        """Test ToolBitShapeDovetail default parameters and labels."""
        with patch('Path.Tool.Shape.base.FreeCAD', new=mock_freecad):
            shape = ToolBitShapeDovetail()
            params = shape.get_parameters()
            self.assertEqual(str(params["Diameter"]), "10.0 mm")
            self.assertEqual(str(params["CuttingAngle"]), "45.0 deg")
            self.assertEqual(
                shape.get_parameter_label("CuttingAngle"),
                "Cutting angle")

    def test_toolbitshapeendmill_defaults(self):
        """Test ToolBitShapeEndMill default parameters and labels."""
        with patch('Path.Tool.Shape.base.FreeCAD', new=mock_freecad):
            shape = ToolBitShapeEndMill()
            params = shape.get_parameters()
            self.assertEqual(str(params["Diameter"]), "10.0 mm")
            self.assertEqual(str(params["CuttingEdgeHeight"]), "20.0 mm")
            self.assertEqual(
                shape.get_parameter_label("CuttingEdgeHeight"),
                "Cutting edge height")

    def test_toolbitshapeprobe_defaults(self):
        """Test ToolBitShapeProbe default parameters and labels."""
        with patch('Path.Tool.Shape.base.FreeCAD', new=mock_freecad):
            shape = ToolBitShapeProbe()
            params = shape.get_parameters()
            self.assertEqual(str(params["Diameter"]), "3.0 mm")
            self.assertEqual(str(params["ShankDiameter"]), "6.0 mm")
            self.assertEqual(
                shape.get_parameter_label("Diameter"),
                "Ball diameter")
            self.assertEqual(
                shape.get_parameter_label("ShankDiameter"),
                "Shaft diameter")

    def test_toolbitshapereamer_defaults(self):
        """Test ToolBitShapeReamer default parameters and labels."""
        with patch('Path.Tool.Shape.base.FreeCAD', new=mock_freecad):
            shape = ToolBitShapeReamer()
            params = shape.get_parameters()
            self.assertEqual(str(params["Diameter"]), "8.0 mm")
            self.assertEqual(str(params["Length"]), "80.0 mm")
            self.assertEqual(shape.get_parameter_label("Diameter"), "Diameter")

    def test_toolbitshapeslittingsaw_defaults(self):
        """Test ToolBitShapeSlittingSaw default parameters and labels."""
        with patch('Path.Tool.Shape.base.FreeCAD', new=mock_freecad):
            shape = ToolBitShapeSlittingSaw()
            params = shape.get_parameters()
            self.assertEqual(str(params["Diameter"]), "50.0 mm")
            self.assertEqual(str(params["BladeThickness"]), "1.0 mm")
            self.assertEqual(shape.get_parameter_label(
                "BladeThickness"), "Blade thickness")

    def test_toolbitshapetap_defaults(self):
        """Test ToolBitShapeTap default parameters and labels."""
        with patch('Path.Tool.Shape.base.FreeCAD', new=mock_freecad):
            shape = ToolBitShapeTap()
            params = shape.get_parameters()
            self.assertEqual(str(params["Diameter"]), "6.0 mm")
            self.assertEqual(str(params["TipAngle"]), "90.0 deg")
            self.assertEqual(shape.get_parameter_label("TipAngle"), "Tip Angle")

    def test_toolbitshapethreadmill_defaults(self):
        """Test ToolBitShapeThreadMill default parameters and labels."""
        with patch('Path.Tool.Shape.base.FreeCAD', new=mock_freecad):
            shape = ToolBitShapeThreadMill()
            params = shape.get_parameters()
            self.assertEqual(str(params["Diameter"]), "8.0 mm")
            self.assertEqual(str(params["CuttingAngle"]), "60.0 deg")
            self.assertEqual(
                shape.get_parameter_label("CuttingAngle"),
                "Cutting angle")

    def test_toolbitshapetorus_defaults(self):
        """Test ToolBitShapeTorus default parameters and labels."""
        with patch('Path.Tool.Shape.base.FreeCAD', new=mock_freecad):
            shape = ToolBitShapeTorus()
            params = shape.get_parameters()
            self.assertEqual(str(params["Diameter"]), "10.0 mm")
            self.assertEqual(str(params["TorusRadius"]), "1.0 mm")
            self.assertEqual(
                shape.get_parameter_label("TorusRadius"),
                "Torus radius")

    def test_toolbitshapevbit_defaults(self):
        """Test ToolBitShapeVBit default parameters and labels."""
        with patch('Path.Tool.Shape.base.FreeCAD', new=mock_freecad):
            shape = ToolBitShapeVBit()
            params = shape.get_parameters()
            self.assertEqual(str(params["Diameter"]), "12.0 mm")
            self.assertEqual(str(params["CuttingEdgeAngle"]), "60.0 deg")
            self.assertEqual(str(params["TipDiameter"]), "0.2 mm")
            self.assertEqual(shape.get_parameter_label(
                "CuttingEdgeAngle"), "Cutting edge angle")


@patch.dict(sys.modules, {'FreeCAD': mock_freecad, 'FreeCAD.Units': mock_units})
class TestPathToolShapeInitHelpers(PathTestUtils.PathTestBase):
    """Tests for helpers in Path/Tool/Shape/__init__.py"""

    def test_tool_bit_shape_classes_dict(self):
        """Verify the TOOL_BIT_SHAPE_CLASSES dictionary."""
        self.assertIn("ballend", TOOL_BIT_SHAPE_CLASSES)
        self.assertEqual(
            TOOL_BIT_SHAPE_CLASSES["ballend"],
            ToolBitShapeBallEnd)
        self.assertIn("vbit", TOOL_BIT_SHAPE_CLASSES)
        self.assertEqual(
            TOOL_BIT_SHAPE_CLASSES["vbit"],
            ToolBitShapeVBit)
        # Check number of classes (ensure all were imported and added)
        # Count subclasses of ToolBitShape defined in the Shape package
        expected_count = len([
            cls for cls in ToolBitShape.__subclasses__()
            if cls.__module__.startswith("Path.Tool.Shape")
        ])
        self.assertEqual(
            len(set(TOOL_BIT_SHAPE_CLASSES.values())), expected_count)

    def test_get_shape_class(self):
        """Test the get_shape_class function."""
        self.assertEqual(
            get_shape_class("ballend"),
            ToolBitShapeBallEnd)
        self.assertEqual(
            get_shape_class("BallEnd"),
            ToolBitShapeBallEnd)
        self.assertEqual(
            get_shape_class("BALLEND"),
            ToolBitShapeBallEnd)
        self.assertEqual(
            get_shape_class("vbit"),
            ToolBitShapeVBit)
        self.assertEqual(
            get_shape_class("V-Bit"),
            ToolBitShapeVBit)
        self.assertIsNone(get_shape_class("nonexistent"))


@patch.dict(sys.modules, {'FreeCAD': mock_freecad, 'FreeCAD.Units': mock_units})
class TestPathToolShapeUtil(PathTestUtils.PathTestBase):
    """Tests for the utility functions in Path/Tool/Shape/util.py"""

    def setUp(self):
        """Reset mocks before each test."""
        mock_freecad.Console.PrintWarning.reset_mock()
        mock_freecad.Console.PrintError.reset_mock()
        mock_freecad.openDocument.reset_mock()
        mock_freecad.closeDocument.reset_mock()
        mock_doc.reset_mock()
        mock_obj.reset_mock()
        mock_doc.Objects = [mock_obj]  # Reset objects list
        mock_obj.Label = "MockObjectLabel"
        mock_obj.Name = "MockObjectName"
        # Clear any dynamically added attributes from previous tests
        for attr in list(mock_obj.__dict__.keys()):
            if not attr.startswith('_') and attr not in [
                    'Label', 'Name', 'reset_mock']:
                try:
                    delattr(mock_obj, attr)
                except AttributeError:
                    pass  # Ignore errors if attribute is already gone

    def test_util_find_shape_object_body_priority(self):
        """Test find_shape_object prioritizes PartDesign::Body."""
        body_obj = MagicMock(Name="Body_Mock")
        body_obj.isDerivedFrom = lambda typeName: typeName == "PartDesign::Body"
        part_obj = MagicMock(Name="Part_Mock")
        part_obj.isDerivedFrom = lambda typeName: typeName == "Part::Feature"
        mock_doc.Objects = [part_obj, body_obj]
        found = util.find_shape_object(mock_doc)
        self.assertEqual(found, body_obj)

    def test_util_find_shape_object_part_fallback(self):
        """Test find_shape_object falls back to Part::Feature."""
        part_obj = MagicMock(Name="Part_Mock")
        part_obj.isDerivedFrom = lambda typeName: typeName == "Part::Feature"
        other_obj = MagicMock(Name="Other_Mock")
        other_obj.isDerivedFrom = lambda typeName: False
        mock_doc.Objects = [other_obj, part_obj]
        found = util.find_shape_object(mock_doc)
        self.assertEqual(found, part_obj)

    def test_util_find_shape_object_first_obj_fallback(self):
        """Test find_shape_object falls back to the first object."""
        other_obj1 = MagicMock(Name="Other1_Mock")
        other_obj1.isDerivedFrom = lambda typeName: False
        other_obj2 = MagicMock(Name="Other2_Mock")
        other_obj2.isDerivedFrom = lambda typeName: False
        mock_doc.Objects = [other_obj1, other_obj2]
        found = util.find_shape_object(mock_doc)
        self.assertEqual(found, other_obj1)

    def test_util_find_shape_object_no_objects(self):
        """Test find_shape_object returns None if document has no objects."""
        mock_doc.Objects = []
        found = util.find_shape_object(mock_doc)
        self.assertIsNone(found)

    def test_util_get_object_properties_found(self):
        """Test get_object_properties extracts existing properties."""
        setattr(mock_obj, "Diameter", "10 mm")
        setattr(mock_obj, "Length", "50 mm")
        params = util.get_object_properties(mock_obj, ["Diameter", "Length"])
        self.assertEqual(params, {"Diameter": ("10 mm", None),
                                  "Length": ("50 mm", None)})
        mock_freecad.Console.PrintWarning.assert_not_called()

    @patch('Path.Tool.Shape.util.FreeCAD', new=mock_freecad)
    def test_util_get_object_properties_missing(self):
        """Test get_object_properties handles missing properties with warning."""
        # Re-import util within the patch context to use the mocked FreeCAD
        import Path.Tool.Shape.util as util_patched

        setattr(mock_obj, "Diameter", "10 mm")
        # Explicitly delete Height to ensure hasattr returns False for MagicMock
        if hasattr(mock_obj, 'Height'):
            delattr(mock_obj, 'Height')
        params = util_patched.get_object_properties(mock_obj, ["Diameter", "Height"])
        self.assertEqual(params, {"Diameter": ("10 mm", None),
                                  "Height": (None, None)})  # Height is missing
        expected_calls = [
            call("Could not get type for property 'Diameter' on object "
                 "'MockObjectLabel'"),
            call("Parameter 'Height' not found on object 'MockObjectLabel' "
                 "(MockObjectName). Default value will be used by the shape "
                 "class.\n")
        ]
        mock_freecad.Console.PrintWarning.assert_has_calls(expected_calls,
                                                          any_order=True)

    @patch('os.path.exists', return_value=True)
    @patch('FreeCAD.openDocument', return_value=mock_doc)
    @patch.object(util,
                  'find_shape_object',
                  return_value=mock_obj)
    @patch.object(util, 'get_object_properties')
    @patch('Path.Tool.Shape.util.FreeCAD', new=mock_freecad)
    def test_util_load_doc_success(
            self,
            mock_get_props,
            mock_find_obj,
            mock_open_doc,
            mock_exists):
        """Test load_doc_and_get_properties success path."""
        mock_get_props.return_value = {"Diameter": "5 mm"}
        expected_params = ["Diameter"]
        filepath = Path("/fake/path/tool.fcstd")

        doc, params = util.load_doc_and_get_properties(
            filepath, expected_params)

        mock_exists.assert_called_once_with(filepath)
        mock_freecad.openDocument.assert_called_once_with(
            str(filepath), Hidden=True)
        mock_find_obj.assert_called_once_with(mock_doc)
        mock_get_props.assert_called_once_with(mock_obj, expected_params)
        self.assertEqual(doc, mock_doc)  # Returns the mock doc
        self.assertEqual(params, {"Diameter": "5 mm"})
        # IMPORTANT: The function itself doesn't close the doc, the caller does
        # (in base.py)
        mock_freecad.closeDocument.assert_not_called()

    @patch('os.path.exists', return_value=False)
    def test_util_load_doc_file_not_found(self, mock_exists):
        """Test load_doc_and_get_properties raises FileNotFoundError."""
        filepath = Path("/fake/nonexistent.fcstd")
        with self.assertRaisesRegex(FileNotFoundError, f"File not found: {filepath}"):
            util.load_doc_and_get_properties(filepath, ["Diameter"])
        mock_exists.assert_called_once_with(filepath)
        mock_freecad.openDocument.assert_not_called()

    @patch('os.path.exists', return_value=True)
    @patch('Path.Tool.Shape.util.FreeCAD', new=mock_freecad)
    def test_util_load_doc_open_fails(self, mock_exists):
        """Test load_doc_and_get_properties raises ValueError if open fails."""
        mock_freecad.openDocument.return_value = None  # Simulate open failure
        filepath = Path("/fake/fail_open.fcstd")
        with self.assertRaisesRegex(ValueError, f"Failed to open document: {filepath}"):
            util.load_doc_and_get_properties(filepath, ["Diameter"])
        mock_exists.assert_called_once_with(filepath)
        mock_freecad.openDocument.assert_called_once_with(
            str(filepath), Hidden=True)
        mock_freecad.closeDocument.assert_not_called()  # Should fail before close

    @patch('os.path.exists', return_value=True)
    @patch.object(util, 'find_shape_object', return_value=None)
    @patch('Path.Tool.Shape.util.FreeCAD', new=mock_freecad)
    def test_util_load_doc_no_object_found(self, mock_find_obj, mock_exists):
        """Test load_doc_and_get_properties raises ValueError if no object found."""
        filepath = Path("/fake/no_object.fcstd")
        mock_freecad.openDocument.return_value = mock_doc  # Ensure open succeeds
        with self.assertRaisesRegex(ValueError, f"Could not find suitable shape object in {filepath}"):
            util.load_doc_and_get_properties(filepath, ["Diameter"])
        mock_exists.assert_called_once_with(filepath)
        mock_freecad.openDocument.assert_called_once_with(
            str(filepath), Hidden=True)
        mock_find_obj.assert_called_once_with(mock_doc)
        # Document should be closed before raising in this case
        mock_freecad.closeDocument.assert_called_once_with(mock_doc.Name)


# Test execution
if __name__ == '__main__':
    unittest.main()
