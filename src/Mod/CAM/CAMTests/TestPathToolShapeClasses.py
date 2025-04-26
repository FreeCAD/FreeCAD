# -*- coding: utf-8 -*-
# Unit tests for the Path.Tool.Shape module and its utilities.

import CAMTests.PathTestUtils as PathTestUtils
from pathlib import Path
from typing import Mapping, Tuple
import FreeCAD
from Path.Tool.shape import ToolBitShape
from Path.Tool.shape.registry import SHAPE_REGISTRY


# Helper dummy class for testing abstract methods
class DummyShape(ToolBitShape):
    name = "dummy"

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        if not self._params:  # Only set defaults if not loaded from file
            self._defaults = {
                "Param1": FreeCAD.Units.Quantity("10", "mm"),
                "Param2": FreeCAD.Units.Quantity("5", "deg"),
            }

    @classmethod
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
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

    def test_base_validate_success(self):
        """Test ToolBitShape.validate success path."""
        # Use a real built-in shape file for validation
        filepath = SHAPE_REGISTRY.shape_dir / "ballend.fcstd"
        result = DummyShape.validate(filepath)
        self.assertIsNone(result) # validate returns None on success

    def test_base_validate_file_not_found(self):
        """Test ToolBitShape.validate file not found."""
        filepath = Path("/fake/nonexistent.fcstd")
        result = DummyShape.validate(filepath)
        self.assertIsNotNone(result)
        self.assertIn("File not found", result)


class TestPathToolShapeClasses(PathTestUtils.PathTestBase):
    """Tests for the concrete ToolBitShape subclasses."""

    def _test_shape_common(self, alias):
        filename = f"{alias}.fcstd"
        shape = SHAPE_REGISTRY.get_shape_from_filename(filename, {})
        # Validation is handled internally by from_file called by the registry
        return shape.get_parameters()

    def test_concrete_classes_instantiation(self):
        """Test that all concrete classes can be instantiated."""
        # No patching of FreeCAD document operations here.
        # The test relies on the actual FreeCAD environment.

        # Iterate through registered shapes instead of subclasses
        for filename in SHAPE_REGISTRY.shape_dir.iterdir():
            if filename.suffix == ".fcstd":
                alias = filename.stem
                # Skip the DummyShape file if it exists in the shapes directory
                if alias == "dummy":
                    continue

                with self.subTest(shape=alias):
                    try:
                        instance = SHAPE_REGISTRY.get_shape_from_filename(filename.name, {})
                        self.assertIsInstance(instance, ToolBitShape)
                        # Check if default params were set by checking if the
                        # parameters dictionary is not empty.
                        self.assertTrue(instance.get_parameters())

                    except Exception as e:
                        self.fail(f"Failed to instantiate shape from {filename}: {e}")

    # The following tests for default parameters and labels
    # should also not use mocks for FreeCAD document operations or Units.
    # They should rely on the actual FreeCAD environment and the
    # load_file method of the base class.

    def test_toolbitshapeballend_defaults(self):
        """Test ToolBitShapeBallend default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        # The actual file content is not loaded in this test,
        # only the default parameters are checked.
        shape = self._test_shape_common("ballend")
        self.assertEqual(shape["Diameter"].Value, 5.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["Length"].Value, 50.0)
        self.assertEqual(unit(shape["Length"]), "mm")
        # Need an instance to get parameter labels, get it from the registry
        instance = SHAPE_REGISTRY.get_shape_from_filename("ballend.fcstd", {})
        self.assertEqual(instance.get_parameter_label("Diameter"), "Diameter")
        self.assertEqual(instance.get_parameter_label("Length"), "Overall tool length")

    def test_toolbitshapedrill_defaults(self):
        """Test ToolBitShapeDrill default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("drill")
        self.assertEqual(shape["Diameter"].Value, 3.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["TipAngle"].Value, 119.0)
        self.assertEqual(unit(shape["TipAngle"]), "°")
        # Need an instance to get parameter labels, get it from the registry
        instance = SHAPE_REGISTRY.get_shape_from_filename("drill.fcstd", {})
        self.assertEqual(instance.get_parameter_label("Diameter"), "Diameter")
        self.assertEqual(instance.get_parameter_label("TipAngle"), "Tip angle")

    def test_toolbitshapechamfer_defaults(self):
        """Test ToolBitShapeChamfer default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("chamfer")
        self.assertEqual(shape["Diameter"].Value, 12.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["CuttingEdgeAngle"].Value, 60.0)
        self.assertEqual(unit(shape["CuttingEdgeAngle"]), "°")
        # Need an instance to get parameter labels, get it from the registry
        instance = SHAPE_REGISTRY.get_shape_from_filename("chamfer.fcstd", {})
        self.assertEqual(instance.get_parameter_label("Diameter"), "Diameter")

    def test_toolbitshapedovetail_defaults(self):
        """Test ToolBitShapeDovetail default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("dovetail")
        self.assertEqual(shape["Diameter"].Value, 20.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["CuttingEdgeAngle"].Value, 60.0)
        self.assertEqual(unit(shape["CuttingEdgeAngle"]), "°")
        # Need an instance to get parameter labels, get it from the registry
        instance = SHAPE_REGISTRY.get_shape_from_filename("dovetail.fcstd", {})
        self.assertEqual(instance.get_parameter_label("CuttingEdgeAngle"), "Cutting angle")

    def test_toolbitshapeendmill_defaults(self):
        """Test ToolBitShapeEndmill default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("endmill")
        self.assertEqual(shape["Diameter"].Value, 5.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["CuttingEdgeHeight"].Value, 30.0)
        self.assertEqual(unit(shape["CuttingEdgeHeight"]), "mm")
        # Need an instance to get parameter labels, get it from the registry
        instance = SHAPE_REGISTRY.get_shape_from_filename("endmill.fcstd", {})
        self.assertEqual(instance.get_parameter_label("CuttingEdgeHeight"), "Cutting edge height")

    def test_toolbitshapeprobe_defaults(self):
        """Test ToolBitShapeProbe default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("probe")
        self.assertEqual(shape["Diameter"].Value, 6.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["ShaftDiameter"].Value, 4.0)
        self.assertEqual(unit(shape["ShaftDiameter"]), "mm")
        # Need an instance to get parameter labels, get it from the registry
        instance = SHAPE_REGISTRY.get_shape_from_filename("probe.fcstd", {})
        self.assertEqual(instance.get_parameter_label("Diameter"), "Ball diameter")
        self.assertEqual(instance.get_parameter_label("ShaftDiameter"), "Shaft diameter")

    def test_toolbitshapereamer_defaults(self):
        """Test ToolBitShapeReamer default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("reamer")
        self.assertEqual(shape["Diameter"].Value, 5.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["Length"].Value, 50.0)
        self.assertEqual(unit(shape["Length"]), "mm")
        # Need an instance to get parameter labels, get it from the registry
        instance = SHAPE_REGISTRY.get_shape_from_filename("reamer.fcstd", {})
        self.assertEqual(instance.get_parameter_label("Diameter"), "Diameter")

    def test_toolbitshapeslittingsaw_defaults(self):
        """Test ToolBitShapeSlittingSaw default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("slittingsaw")
        self.assertEqual(shape["Diameter"].Value, 100.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["BladeThickness"].Value, 3.0)
        self.assertEqual(unit(shape["BladeThickness"]), "mm")
        # Need an instance to get parameter labels, get it from the registry
        instance = SHAPE_REGISTRY.get_shape_from_filename("slittingsaw.fcstd", {})
        self.assertEqual(instance.get_parameter_label("BladeThickness"), "Blade thickness")

    def test_toolbitshapetap_defaults(self):
        """Test ToolBitShapeTap default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("tap")
        self.assertAlmostEqual(shape["Diameter"].Value, 8, 4)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["TipAngle"].Value, 90.0)
        self.assertEqual(unit(shape["TipAngle"]), "°")
        # Need an instance to get parameter labels, get it from the registry
        instance = SHAPE_REGISTRY.get_shape_from_filename("tap.fcstd", {})
        self.assertEqual(instance.get_parameter_label("TipAngle"), "Tip angle")

    def test_toolbitshapethreadmill_defaults(self):
        """Test ToolBitShapeThreadMill default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("threadmill")
        self.assertEqual(shape["Diameter"].Value, 5.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["cuttingAngle"].Value, 60.0)
        self.assertEqual(unit(shape["cuttingAngle"]), "°")
        # Need an instance to get parameter labels, get it from the registry
        instance = SHAPE_REGISTRY.get_shape_from_filename("threadmill.fcstd", {})
        self.assertEqual(instance.get_parameter_label("cuttingAngle"), "Cutting angle")

    def test_toolbitshapebullnose_defaults(self):
        """Test ToolBitShapeBullnose default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("bullnose")
        self.assertEqual(shape["Diameter"].Value, 5.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["FlatRadius"].Value, 1.5)
        self.assertEqual(unit(shape["FlatRadius"]), "mm")
        # Need an instance to get parameter labels, get it from the registry
        instance = SHAPE_REGISTRY.get_shape_from_filename("bullnose.fcstd", {})
        self.assertEqual(instance.get_parameter_label("FlatRadius"), "Torus radius")

    def test_toolbitshapevbit_defaults(self):
        """Test ToolBitShapeVBit default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("vbit")
        self.assertEqual(shape["Diameter"].Value, 10.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["CuttingEdgeAngle"].Value, 90.0)
        self.assertEqual(unit(shape["CuttingEdgeAngle"]), "°")
        self.assertEqual(shape["TipDiameter"].Value, 1.0)
        self.assertEqual(unit(shape["TipDiameter"]), "mm")
        # Need an instance to get parameter labels, get it from the registry
        instance = SHAPE_REGISTRY.get_shape_from_filename("vbit.fcstd", {})
        self.assertEqual(instance.get_parameter_label("CuttingEdgeAngle"), "Cutting edge angle")
