# -*- coding: utf-8 -*-
# Unit tests for the Path.Tool.Shape module and its utilities.

import CAMTests.PathTestUtils as PathTestUtils
from pathlib import Path
from unittest.mock import patch, MagicMock
from typing import Dict, Tuple
import FreeCAD
from Path.Tool.shape import (
    ToolBitShape,
    util,
    ToolBitShapeBallEnd,
    ToolBitShapeChamfer,
    ToolBitShapeDovetail,
    ToolBitShapeDrill,
    ToolBitShapeEndMill,
    ToolBitShapeProbe,
    ToolBitShapeReamer,
    ToolBitShapeSlittingSaw,
    ToolBitShapeTap,
    ToolBitShapeThreadMill,
    ToolBitShapeBullnose,
    ToolBitShapeVBit,
)
from Path.Tool.shape.util import get_shape_class_from_name


# Helper dummy class for testing abstract methods
class DummyShape(ToolBitShape):
    name = "dummy"

    def __init__(self, filepath, **kwargs):
        super().__init__(filepath, **kwargs)
        if not self._params:  # Only set defaults if not loaded from file
            self._defaults = {
                "Param1": FreeCAD.Units.Quantity("10", "mm"),
                "Param2": FreeCAD.Units.Quantity("5", "deg"),
            }

    @classmethod
    def shape_schema(cls) -> Dict[str, Tuple[str, str]]:
        return {
            "Param1": (
                FreeCAD.Qt.translate("Param1", "Parameter 1"),
                "App::PropertyLength",
            ),
            "": (
                FreeCAD.Qt.translate("Param2", "Parameter 2"),
                "App::PropertyAngle",
            ),
        }

    @property
    def label(self):
        return "Dummy Shape"


def unit(param):
    return param.getUserPreferred()[2]


class TestPathToolShapeBase(PathTestUtils.PathTestBase):
    def setUp(self):
        """Reset mocks before each test."""
        # No mocks for FreeCAD or FreeCAD.Units here
        pass

    """Tests for the ToolBitShape abstract base class."""

    def test_base_init_with_defaults(self):
        """Test base class initialization uses default parameters."""
        # Provide a dummy filepath for instantiation
        shape = DummyShape(filepath=Path("/fake/dummy.fcstd"))
        params = self._test_shape_common("ballend")
        self.assertEqual(str(params["Param1"]), "10 mm")
        self.assertEqual(str(params["Param2"]), "5 deg")

    def test_base_init_with_kwargs(self):
        """Test base class initialization overrides defaults with kwargs."""
        # Provide a dummy filepath for instantiation
        shape = DummyShape(
            filepath=Path("/fake/dummy.fcstd"),
            Param1="20 mm",
            Param3="Ignored",
        )
        params = self._test_shape_common("ballend")
        self.assertEqual(str(params["Param1"]), "20 mm")
        self.assertEqual(str(params["Param2"]), "5 deg")  # Should remain default
        # Check warning for ignored param
        # Assuming FreeCAD.Console is available in the test environment
        FreeCAD.Console.PrintWarning.assert_called_once()
        call_args, _ = FreeCAD.Console.PrintWarning.call_args
        self.assertIn("Ignoring unknown parameter 'Param3'", call_args[0])
        self.assertIn("DummyShape", call_args[0])

    def test_base_get_set_parameter(self):
        """Test getting and setting individual parameters."""
        # Provide a dummy filepath for instantiation
        shape = DummyShape(filepath=Path("/fake/dummy.fcstd"))
        self.assertEqual(str(shape.get_parameter("Param1")), "10 mm")
        shape.set_parameter("Param1", "15 mm")
        self.assertEqual(str(shape.get_parameter("Param1")), "15 mm")
        with self.assertRaisesRegex(KeyError, "Shape 'dummy' has no parameter 'InvalidParam'"):
            shape.get_parameter("InvalidParam")
        with self.assertRaisesRegex(KeyError, "Shape 'dummy' has no parameter 'InvalidParam'"):
            shape.set_parameter("InvalidParam", "1")

    def test_base_get_parameters(self):
        """Test getting the full parameter dictionary."""
        # Provide a dummy filepath for instantiation
        shape = DummyShape(filepath=Path("/fake/dummy.fcstd"), Param1="12 mm")
        # Create mock quantity instances using the configured mock class
        expected_param1 = FreeCAD.Units.Quantity("12", "mm")
        expected_param2 = FreeCAD.Units.Quantity("5", "deg")

        expected = {"Param1": expected_param1, "Param2": expected_param2}
        self.assertEqual(self._test_shape_common("ballend"), expected)

        # Ensure it's a copy
        params = self._test_shape_common("ballend")
        params["Param1"] = "99 mm"
        self.assertEqual(str(shape.get_parameter("Param1")), "12 mm")

    def test_base_name_property(self):
        """Test the name property returns the primary alias."""
        # Provide a dummy filepath for instantiation
        shape = DummyShape(filepath=Path("/fake/dummy.fcstd"))
        self.assertEqual(shape.name, "dummy")

    def test_base_get_parameter_label(self):
        """Test retrieving parameter labels."""
        # Provide a dummy filepath for instantiation
        shape = DummyShape(filepath=Path("/fake/dummy.fcstd"))
        self.assertEqual(shape.get_parameter_label("Param1"), "Parameter 1")
        self.assertEqual(shape.get_parameter_label("Param2"), "Parameter 2")
        # Test fallback for unknown parameter
        self.assertEqual(shape.get_parameter_label("UnknownParam"), "UnknownParam")

    def test_base_get_expected_shape_parameters(self):
        """Test retrieving the list of expected parameter names."""
        expected = ["Param1", "Param2"]
        self.assertCountEqual(DummyShape.get_expected_shape_parameters(), expected)

    def test_base_str_repr(self):
        """Test string representation."""
        # Provide a dummy filepath for instantiation
        shape = DummyShape(filepath=Path("/fake/dummy.fcstd"))
        expected_str = "dummy(Param1=10 mm, Param2=5 deg)"
        self.assertEqual(str(shape), expected_str)
        self.assertEqual(repr(shape), expected_str)

    @patch("os.path.exists", return_value=True)
    @patch.object(util, "find_shape_object")
    @patch.object(ToolBitShape, "get_expected_shape_parameters")
    def test_base_validate_success(self, mock_get_expected, mock_find_obj, mock_exists):
        """Test ToolBitShape.validate success path."""
        mock_get_expected.return_value = ["Param1", "Param2"]
        # Create a mock document and object for find_shape_object to return
        mock_doc = MagicMock(Name="MockDocument")
        mock_obj = MagicMock(Name="MockObject")
        setattr(mock_obj, "Param1", FreeCAD.Units.Quantity("10", "mm"))
        setattr(mock_obj, "Param2", FreeCAD.Units.Quantity("5", "deg"))
        mock_find_obj.return_value = mock_obj

        # Patch FreeCAD document operations for this specific test
        with patch("FreeCAD.openDocument") as mock_open_doc, patch(
            "FreeCAD.closeDocument"
        ) as mock_close_doc:

            mock_open_doc.return_value = mock_doc
            mock_close_doc.return_value = None  # closeDocument doesn't return anything

            filepath = Path("/fake/valid.fcstd")
            result = DummyShape.validate(filepath)

            self.assertTrue(result)
            mock_exists.assert_called_once_with(filepath)
            mock_open_doc.assert_called_once_with(filepath, Hidden=True)
            mock_find_obj.assert_called_once_with(mock_doc)
            mock_close_doc.assert_called_once_with(mock_doc.Name)
            # Assuming FreeCAD.Console is available in the test environment
            FreeCAD.Console.PrintError.assert_not_called()
            FreeCAD.Console.PrintWarning.assert_not_called()

    @patch("os.path.exists", return_value=False)
    def test_base_validate_file_not_found(self, mock_exists):
        """Test ToolBitShape.validate file not found."""
        filepath = Path("/fake/nonexistent.fcstd")
        result = DummyShape.validate(filepath)
        self.assertFalse(result)
        mock_exists.assert_called_once_with(filepath)
        # Assuming FreeCAD.Console is available in the test environment
        FreeCAD.Console.PrintError.assert_called_once()
        self.assertIn("File not found", FreeCAD.Console.PrintError.call_args[0][0])
        # No FreeCAD document operations should be called
        # with patch('FreeCAD.openDocument') as mock_open_doc:
        #     mock_open_doc.assert_not_called()

    @patch("os.path.exists", return_value=True)
    def test_base_validate_open_fails(self, mock_exists):
        """Test ToolBitShape.validate handles document open failure."""
        # Patch FreeCAD document operations for this specific test
        with patch("FreeCAD.openDocument") as mock_open_doc, patch(
            "FreeCAD.closeDocument"
        ) as mock_close_doc:

            mock_open_doc.return_value = None  # Simulate failure
            filepath = Path("/fake/fail_open.fcstd")
            result = DummyShape.validate(filepath)
            self.assertFalse(result)
            mock_exists.assert_called_once_with(filepath)
            mock_open_doc.assert_called_once_with(filepath, Hidden=True)
            # Assuming FreeCAD.Console is available in the test environment
            FreeCAD.Console.PrintError.assert_called_once()
            self.assertIn(
                "Failed to open document",
                FreeCAD.Console.PrintError.call_args[0][0],
            )
            mock_close_doc.assert_not_called()  # Should fail before close

    @patch("os.path.exists", return_value=True)
    @patch.object(util, "find_shape_object", return_value=None)
    def test_base_validate_no_object(self, mock_find_obj, mock_exists):
        """Test ToolBitShape.validate handles no suitable object found."""
        # Create a mock document for openDocument to return
        mock_doc = MagicMock(Name="MockDocument")

        # Patch FreeCAD document operations for this specific test
        with patch("FreeCAD.openDocument") as mock_open_doc, patch(
            "FreeCAD.closeDocument"
        ) as mock_close_doc:

            mock_open_doc.return_value = mock_doc  # Ensure open succeeds
            mock_close_doc.return_value = None  # closeDocument doesn't return anything

            filepath = Path("/fake/no_object.fcstd")
            result = DummyShape.validate(filepath)
            self.assertFalse(result)
            mock_exists.assert_called_once_with(filepath)
            mock_open_doc.assert_called_once_with(filepath, Hidden=True)
            mock_find_obj.assert_called_once_with(mock_doc)
            # Assuming FreeCAD.Console is available in the test environment
            FreeCAD.Console.PrintError.assert_called_once()
            self.assertIn(
                "No suitable shape object found",
                FreeCAD.Console.PrintError.call_args[0][0],
            )
            mock_close_doc.assert_called_once_with(mock_doc.Name)  # Closed in finally

    @patch("os.path.exists", return_value=True)
    @patch.object(util, "find_shape_object")
    @patch.object(ToolBitShape, "get_expected_shape_parameters")
    def test_base_validate_missing_params(self, mock_get_expected, mock_find_obj, mock_exists):
        """Test ToolBitShape.validate handles missing parameters on object."""
        mock_get_expected.return_value = ["Param1", "Param2", "MissingParam"]
        # Create a mock document and object for find_shape_object to return
        mock_doc = MagicMock(Name="MockDocument")
        mock_obj = MagicMock(Name="MockObject")
        # Object only has Param1 and Param2
        setattr(mock_obj, "Param1", FreeCAD.Units.Quantity("10", "mm"))
        setattr(mock_obj, "Param2", FreeCAD.Units.Quantity("5", "deg"))
        mock_find_obj.return_value = mock_obj

        # Patch FreeCAD document operations for this specific test
        with patch("FreeCAD.openDocument") as mock_open_doc, patch(
            "FreeCAD.closeDocument"
        ) as mock_close_doc:

            mock_open_doc.return_value = mock_doc
            mock_close_doc.return_value = None  # closeDocument doesn't return anything

            filepath = Path("/fake/missing_params.fcstd")
            result = DummyShape.validate(filepath)

            # Currently validation fails if params missing
            self.assertFalse(result)
            mock_exists.assert_called_once_with(filepath)
            mock_open_doc.assert_called_once_with(filepath, Hidden=True)
            mock_find_obj.assert_called_once_with(mock_doc)
            # Assuming FreeCAD.Console is available in the test environment
            FreeCAD.Console.PrintWarning.assert_called_once()
            self.assertIn(
                "is missing parameters",
                FreeCAD.Console.PrintWarning.call_args[0][0],
            )
            self.assertIn("MissingParam", FreeCAD.Console.PrintWarning.call_args[0][0])
            mock_close_doc.assert_called_once_with(mock_doc.Name)


class TestPathToolShapeClasses(PathTestUtils.PathTestBase):
    """Tests for the concrete ToolBitShape subclasses."""

    def _test_shape_common(self, alias):
        cls = get_shape_class_from_name(alias)
        filepath = util.get_builtin_shape_file_from_name(alias)
        self.assertIsNone(cls.validate(filepath))
        shape = cls(filepath=filepath)
        return shape.get_parameters()

    def test_concrete_classes_instantiation(self):
        """Test that all concrete classes can be instantiated."""
        # No patching of FreeCAD document operations here.
        # The test relies on the actual FreeCAD environment.

        for cls in ToolBitShape.__subclasses__():
            # Skip the DummyShape class as it's for base class testing
            if cls.__name__ == "DummyShape":
                continue

            name = cls.name
            with self.subTest(shape=name):
                try:
                    # Use real file paths for instantiation
                    filepath = util.get_builtin_shape_file_from_name(name)
                    instance = cls(filepath=filepath)
                    self.assertIsInstance(instance, ToolBitShape)
                    # Check if default params were set by checking if the
                    # parameters dictionary is not empty.
                    self.assertTrue(instance.get_parameters())

                except Exception as e:
                    self.fail(f"Failed to instantiate {cls.__name__}: {e}")

    # The following tests for default parameters and labels
    # should also not use mocks for FreeCAD document operations or Units.
    # They should rely on the actual FreeCAD environment and the
    # load_file method of the base class.

    def test_toolbitshapeballend_defaults(self):
        """Test ToolBitShapeBallEnd default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        # The actual file content is not loaded in this test,
        # only the default parameters are checked.
        filepath = util.get_builtin_shape_file_from_name("ballend")
        shape = ToolBitShapeBallEnd(filepath=filepath)
        params = self._test_shape_common("ballend")
        self.assertEqual(params["Diameter"].Value, 5.0)
        self.assertEqual(unit(params["Diameter"]), "mm")  # Assuming Unit is an object with Name
        self.assertEqual(params["Length"].Value, 50.0)
        self.assertEqual(unit(params["Length"]), "mm")  # Assuming Unit is an object with Name
        self.assertEqual(shape.get_parameter_label("Diameter"), "Diameter")
        self.assertEqual(shape.get_parameter_label("Length"), "Overall tool length")

    def test_toolbitshapedrill_defaults(self):
        """Test ToolBitShapeDrill default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        filepath = util.get_builtin_shape_file_from_name("drill")
        shape = ToolBitShapeDrill(filepath=filepath)
        params = self._test_shape_common("drill")
        self.assertEqual(params["Diameter"].Value, 3.0)
        self.assertEqual(unit(params["Diameter"]), "mm")
        self.assertEqual(params["TipAngle"].Value, 119.0)
        self.assertEqual(unit(params["TipAngle"]), "°")
        self.assertEqual(shape.get_parameter_label("Diameter"), "Diameter")
        self.assertEqual(shape.get_parameter_label("TipAngle"), "Tip angle")

    def test_toolbitshapechamfer_defaults(self):
        """Test ToolBitShapeChamfer default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        filepath = util.get_builtin_shape_file_from_name("chamfer")
        self.assertIsNone(ToolBitShapeChamfer.validate(filepath))
        shape = ToolBitShapeChamfer(filepath=filepath)
        params = self._test_shape_common("chamfer")
        self.assertEqual(params["Diameter"].Value, 12.0)
        self.assertEqual(unit(params["Diameter"]), "mm")
        self.assertEqual(params["CuttingEdgeAngle"].Value, 60.0)
        self.assertEqual(unit(params["CuttingEdgeAngle"]), "°")
        self.assertEqual(shape.get_parameter_label("Diameter"), "Diameter")

    def test_toolbitshapedovetail_defaults(self):
        """Test ToolBitShapeDovetail default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        filepath = util.get_builtin_shape_file_from_name("dovetail")
        shape = ToolBitShapeDovetail(filepath=filepath)
        params = self._test_shape_common("dovetail")
        self.assertEqual(params["Diameter"].Value, 20.0)
        self.assertEqual(unit(params["Diameter"]), "mm")
        self.assertEqual(params["CuttingEdgeAngle"].Value, 60.0)
        self.assertEqual(unit(params["CuttingEdgeAngle"]), "°")
        self.assertEqual(shape.get_parameter_label("CuttingEdgeAngle"), "Cutting angle")

    def test_toolbitshapeendmill_defaults(self):
        """Test ToolBitShapeEndMill default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        filepath = util.get_builtin_shape_file_from_name("endmill")
        shape = ToolBitShapeEndMill(filepath=filepath)
        params = self._test_shape_common("endmill")
        self.assertEqual(params["Diameter"].Value, 5.0)
        self.assertEqual(unit(params["Diameter"]), "mm")
        self.assertEqual(params["CuttingEdgeHeight"].Value, 30.0)
        self.assertEqual(unit(params["CuttingEdgeHeight"]), "mm")
        self.assertEqual(shape.get_parameter_label("CuttingEdgeHeight"), "Cutting edge height")

    def test_toolbitshapeprobe_defaults(self):
        """Test ToolBitShapeProbe default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        filepath = util.get_builtin_shape_file_from_name("probe")
        shape = ToolBitShapeProbe(filepath=filepath)
        params = self._test_shape_common("probe")
        self.assertEqual(params["Diameter"].Value, 6.0)
        self.assertEqual(unit(params["Diameter"]), "mm")
        self.assertEqual(params["ShaftDiameter"].Value, 4.0)
        self.assertEqual(unit(params["ShaftDiameter"]), "mm")
        self.assertEqual(shape.get_parameter_label("Diameter"), "Ball diameter")
        self.assertEqual(shape.get_parameter_label("ShaftDiameter"), "Shaft diameter")

    def test_toolbitshapereamer_defaults(self):
        """Test ToolBitShapeReamer default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        filepath = util.get_builtin_shape_file_from_name("reamer")
        shape = ToolBitShapeReamer(filepath=filepath)
        params = self._test_shape_common("reamer")
        self.assertEqual(params["Diameter"].Value, 5.0)
        self.assertEqual(unit(params["Diameter"]), "mm")
        self.assertEqual(params["Length"].Value, 50.0)
        self.assertEqual(unit(params["Length"]), "mm")
        self.assertEqual(shape.get_parameter_label("Diameter"), "Diameter")

    def test_toolbitshapeslittingsaw_defaults(self):
        """Test ToolBitShapeSlittingSaw default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        filepath = util.get_builtin_shape_file_from_name("slittingsaw")
        shape = ToolBitShapeSlittingSaw(filepath=filepath)
        params = self._test_shape_common("slittingsaw")
        self.assertEqual(params["Diameter"].Value, 100.0)
        self.assertEqual(unit(params["Diameter"]), "mm")
        self.assertEqual(params["BladeThickness"].Value, 3.0)
        self.assertEqual(unit(params["BladeThickness"]), "mm")
        self.assertEqual(shape.get_parameter_label("BladeThickness"), "Blade thickness")

    def test_toolbitshapetap_defaults(self):
        """Test ToolBitShapeTap default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        filepath = util.get_builtin_shape_file_from_name("tap")
        shape = ToolBitShapeTap(filepath=filepath)
        params = self._test_shape_common("tap")
        self.assertAlmostEqual(params["Diameter"].Value, 8, 4)
        self.assertEqual(unit(params["Diameter"]), "mm")
        self.assertEqual(params["TipAngle"].Value, 90.0)
        self.assertEqual(unit(params["TipAngle"]), "°")
        self.assertEqual(shape.get_parameter_label("TipAngle"), "Tip angle")

    def test_toolbitshapethreadmill_defaults(self):
        """Test ToolBitShapeThreadMill default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        filepath = util.get_builtin_shape_file_from_name("thread-mill")
        shape = ToolBitShapeThreadMill(filepath=filepath)
        params = self._test_shape_common("thread-mill")
        self.assertEqual(params["Diameter"].Value, 5.0)
        self.assertEqual(unit(params["Diameter"]), "mm")
        self.assertEqual(params["cuttingAngle"].Value, 60.0)
        self.assertEqual(unit(params["cuttingAngle"]), "°")
        self.assertEqual(shape.get_parameter_label("cuttingAngle"), "Cutting angle")

    def test_toolbitshapebullnose_defaults(self):
        """Test ToolBitShapeBullnose default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        filepath = util.get_builtin_shape_file_from_name("bullnose")
        shape = ToolBitShapeBullnose(filepath=filepath)
        params = self._test_shape_common("bullnose")
        self.assertEqual(params["Diameter"].Value, 5.0)
        self.assertEqual(unit(params["Diameter"]), "mm")
        self.assertEqual(params["FlatRadius"].Value, 1.5)
        self.assertEqual(unit(params["FlatRadius"]), "mm")
        self.assertEqual(shape.get_parameter_label("FlatRadius"), "Torus radius")

    def test_toolbitshapevbit_defaults(self):
        """Test ToolBitShapeVBit default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = ToolBitShapeVBit(filepath=util.get_builtin_shape_file_from_name("v-bit"))
        params = self._test_shape_common("v-bit")
        self.assertEqual(params["Diameter"].Value, 10.0)
        self.assertEqual(unit(params["Diameter"]), "mm")
        self.assertEqual(params["CuttingEdgeAngle"].Value, 90.0)
        self.assertEqual(unit(params["CuttingEdgeAngle"]), "°")
        self.assertEqual(params["TipDiameter"].Value, 1.0)
        self.assertEqual(unit(params["TipDiameter"]), "mm")
        self.assertEqual(shape.get_parameter_label("CuttingEdgeAngle"), "Cutting edge angle")
