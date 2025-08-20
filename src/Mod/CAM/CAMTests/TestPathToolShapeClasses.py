# -*- coding: utf-8 -*-
# Unit tests for the Path.Tool.Shape module and its utilities.

from pathlib import Path
from typing import Mapping, Tuple
import FreeCAD
from CAMTests.PathTestUtils import PathTestWithAssets
from Path.Tool.assets import DummyAssetSerializer
from Path.Tool.shape import (
    ToolBitShape,
    ToolBitShapeBallend,
    ToolBitShapeVBit,
    ToolBitShapeBullnose,
    ToolBitShapeSlittingSaw,
)


# Helper dummy class for testing abstract methods
class DummyShape(ToolBitShape):
    name = "dummy"

    def __init__(self, id, **kwargs):
        super().__init__(id=id, **kwargs)
        # Always define defaults in the subclass
        self._defaults = {
            "Param1": FreeCAD.Units.Quantity("10 mm"),
            "Param2": FreeCAD.Units.Quantity("5 deg"),
        }
        # Merge defaults into _params, allowing kwargs to override
        self._params = self._defaults | self._params

    @classmethod
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
        return {
            "Param1": (
                FreeCAD.Qt.translate("Param1", "Parameter 1"),
                "App::PropertyLength",
            ),
            "Param2": (
                FreeCAD.Qt.translate("Param2", "Parameter 2"),
                "App::PropertyAngle",
            ),
        }

    @property
    def label(self):
        return "Dummy Shape"


def unit(param):
    return param.getUserPreferred()[2]


class TestPathToolShapeClasses(PathTestWithAssets):
    """Tests for the concrete ToolBitShape subclasses."""

    def _test_shape_common(self, alias):
        uri = ToolBitShape.resolve_name(alias)
        shape = self.assets.get(uri)
        return shape.get_parameters()

    def test_base_init_with_defaults(self):
        """Test base class initialization uses default parameters."""
        # Provide a dummy filepath and id for instantiation
        shape = DummyShape(id="dummy_shape_1", filepath=Path("/fake/dummy.fcstd"))
        self.assertEqual(str(shape.get_parameter("Param1")), "10.0 mm")
        self.assertEqual(str(shape.get_parameter("Param2")), "5.0 deg")

    def test_base_init_with_kwargs(self):
        """Test base class initialization overrides defaults with kwargs."""
        # Provide a dummy filepath for instantiation
        shape = DummyShape(
            id="dummy_shape_2",
            filepath=Path("/fake/dummy.fcstd"),
            Param1=FreeCAD.Units.Quantity("20 mm"),
            Param3="Ignored",
        )
        self.assertEqual(shape.get_parameter("Param1").Value, 20.0)
        self.assertEqual(shape.get_parameter("Param1").Value, 20.0)
        self.assertEqual(
            str(shape.get_parameter("Param1").Unit),
            "Unit: mm (1,0,0,0,0,0,0,0) [Length]",
        )
        self.assertEqual(shape.get_parameter("Param2").Value, 5.0)
        self.assertEqual(
            str(shape.get_parameter("Param2").Unit),
            "Unit: deg (0,0,0,0,0,0,0,1) [Angle]",
        )  # Should remain default

    def test_base_get_set_parameter(self):
        """Test getting and setting individual parameters."""
        # Provide a dummy filepath for instantiation
        shape = DummyShape(id="dummy_shape_3", filepath=Path("/fake/dummy.fcstd"))
        self.assertEqual(shape.get_parameter("Param1").Value, 10.0)
        self.assertEqual(shape.get_parameter("Param1").Unit, FreeCAD.Units.Unit("mm"))
        shape.set_parameter("Param1", FreeCAD.Units.Quantity("15 mm"))
        self.assertEqual(shape.get_parameter("Param1").Value, 15.0)
        self.assertEqual(shape.get_parameter("Param1").Unit, FreeCAD.Units.Unit("mm"))
        with self.assertRaisesRegex(KeyError, "Shape 'dummy' has no parameter 'InvalidParam'"):
            shape.get_parameter("InvalidParam")

    def test_base_get_parameters(self):
        """Test getting the full parameter dictionary."""
        # Provide a dummy filepath for instantiation
        shape = DummyShape(
            id="dummy_shape_4",
            filepath=Path("/fake/dummy.fcstd"),
            Param1=FreeCAD.Units.Quantity("12 mm"),
        )
        # Create mock quantity instances using the configured mock class
        expected_param1 = FreeCAD.Units.Quantity("12.0 mm")
        expected_param2 = FreeCAD.Units.Quantity("5.0 deg")

        expected = {"Param1": expected_param1, "Param2": expected_param2}
        params = shape.get_parameters()
        self.assertEqual(params["Param1"].Value, expected["Param1"].Value)
        self.assertEqual(str(params["Param1"].Unit), str(expected["Param1"].Unit))
        self.assertEqual(params["Param2"].Value, expected["Param2"].Value)
        self.assertEqual(str(params["Param2"].Unit), str(expected["Param2"].Unit))

    def test_base_name_property(self):
        """Test the name property returns the primary alias."""
        # Provide a dummy filepath for instantiation
        shape = DummyShape(id="dummy_shape_5", filepath=Path("/fake/dummy.fcstd"))
        self.assertEqual(shape.name, "dummy")

    def test_base_get_parameter_label(self):
        """Test retrieving parameter labels."""
        # Provide a dummy filepath for instantiation
        shape = DummyShape(id="dummy_shape_6", filepath=Path("/fake/dummy.fcstd"))
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
        shape = DummyShape(id="dummy_shape_7", filepath=Path("/fake/dummy.fcstd"))
        # Dynamically construct the expected string using the actual parameter string representations
        params_str = ", ".join(f"{name}={str(val)}" for name, val in shape.get_parameters().items())
        expected_str = f"dummy({params_str})"
        self.assertEqual(str(shape), expected_str)
        self.assertEqual(repr(shape), expected_str)

    def test_base_resolve_name(self):
        """Test resolving shape aliases to canonical names."""
        self.assertEqual(ToolBitShape.resolve_name("ballend").asset_id, "ballend")
        self.assertEqual(ToolBitShape.resolve_name("v-bit").asset_id, "v-bit")
        self.assertEqual(ToolBitShape.resolve_name("vbit").asset_id, "vbit")
        self.assertEqual(ToolBitShape.resolve_name("torus").asset_id, "torus")
        self.assertEqual(ToolBitShape.resolve_name("torus.fcstd").asset_id, "torus")
        self.assertEqual(ToolBitShape.resolve_name("SlittingSaw").asset_id, "SlittingSaw")
        # Test unknown name - should return the input name
        self.assertEqual(ToolBitShape.resolve_name("nonexistent").asset_id, "nonexistent")
        self.assertEqual(ToolBitShape.resolve_name("UnknownShape").asset_id, "UnknownShape")

    def test_concrete_classes_instantiation(self):
        """Test that all concrete classes can be instantiated."""
        # No patching of FreeCAD document operations here.
        # The test relies on the actual FreeCAD environment.

        shape_uris = self.assets.list_assets(asset_type="toolbitshape")
        for uri in shape_uris:
            # Skip the DummyShape asset if it exists
            if uri.asset_id == "dummy":
                continue

            with self.subTest(uri=uri):
                instance = self.assets.get(uri)
                self.assertIsInstance(instance, ToolBitShape)
                # Check if default params were set by checking if the
                # parameters dictionary is not empty.
                self.assertTrue(instance.get_parameters())

    def test_get_shape_class(self):
        """Test the get_shape_class function."""
        uri = ToolBitShape.resolve_name("ballend")
        self.assets.get(uri)  # Ensure it's loadable

        self.assertEqual(ToolBitShape.get_subclass_by_name("ballend"), ToolBitShapeBallend)
        self.assertEqual(ToolBitShape.get_subclass_by_name("v-bit"), ToolBitShapeVBit)
        self.assertEqual(ToolBitShape.get_subclass_by_name("VBit"), ToolBitShapeVBit)
        self.assertEqual(ToolBitShape.get_subclass_by_name("torus"), ToolBitShapeBullnose)
        self.assertEqual(ToolBitShape.get_subclass_by_name("slitting-saw"), ToolBitShapeSlittingSaw)
        self.assertIsNone(ToolBitShape.get_subclass_by_name("nonexistent"))

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
        # Need an instance to get parameter labels, get it from the asset manager
        uri = ToolBitShape.resolve_name("ballend")
        instance = self.assets.get(uri)
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
        # Need an instance to get parameter labels, get it from the asset manager
        uri = ToolBitShape.resolve_name("drill")
        instance = self.assets.get(uri)
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
        # Need an instance to get parameter labels, get it from the asset manager
        uri = ToolBitShape.resolve_name("chamfer")
        instance = self.assets.get(uri)
        self.assertEqual(instance.get_parameter_label("Diameter"), "Diameter")

    def test_toolbitshapedovetail_defaults(self):
        """Test ToolBitShapeDovetail default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("dovetail")
        self.assertEqual(shape["Diameter"].Value, 20.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["CuttingEdgeAngle"].Value, 60.0)
        self.assertEqual(unit(shape["CuttingEdgeAngle"]), "°")
        # Need an instance to get parameter labels, get it from the asset manager
        uri = ToolBitShape.resolve_name("dovetail")
        instance = self.assets.get(uri)
        self.assertEqual(instance.get_parameter_label("CuttingEdgeAngle"), "Cutting angle")

    def test_toolbitshapeendmill_defaults(self):
        """Test ToolBitShapeEndmill default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("endmill")
        self.assertEqual(shape["Diameter"].Value, 5.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["CuttingEdgeHeight"].Value, 30.0)
        self.assertEqual(unit(shape["CuttingEdgeHeight"]), "mm")
        # Need an instance to get parameter labels, get it from the asset manager
        uri = ToolBitShape.resolve_name("endmill")
        instance = self.assets.get(uri)
        self.assertEqual(instance.get_parameter_label("CuttingEdgeHeight"), "Cutting edge height")

    def test_toolbitshapeprobe_defaults(self):
        """Test ToolBitShapeProbe default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("probe")
        self.assertEqual(shape["Diameter"].Value, 6.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["ShaftDiameter"].Value, 4.0)
        self.assertEqual(unit(shape["ShaftDiameter"]), "mm")
        # Need an instance to get parameter labels, get it from the asset manager
        uri = ToolBitShape.resolve_name("probe")
        instance = self.assets.get(uri)
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
        # Need an instance to get parameter labels, get it from the asset manager
        uri = ToolBitShape.resolve_name("reamer")
        instance = self.assets.get(uri)
        self.assertEqual(instance.get_parameter_label("Diameter"), "Diameter")

    def test_toolbitshapeslittingsaw_defaults(self):
        """Test ToolBitShapeSlittingSaw default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("slittingsaw")
        self.assertEqual(shape["Diameter"].Value, 100.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["BladeThickness"].Value, 3.0)
        self.assertEqual(unit(shape["BladeThickness"]), "mm")
        # Need an instance to get parameter labels, get it from the asset manager
        uri = ToolBitShape.resolve_name("slittingsaw")
        instance = self.assets.get(uri)
        self.assertEqual(instance.get_parameter_label("BladeThickness"), "Blade thickness")

    def test_toolbitshapetap_defaults(self):
        """Test ToolBitShapeTap default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("tap")
        self.assertAlmostEqual(shape["Diameter"].Value, 8, 4)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["TipAngle"].Value, 90.0)
        self.assertEqual(unit(shape["TipAngle"]), "°")
        # Need an instance to get parameter labels, get it from the asset manager
        uri = ToolBitShape.resolve_name("tap")
        instance = self.assets.get(uri)
        self.assertEqual(instance.get_parameter_label("TipAngle"), "Tip angle")

    def test_toolbitshapethreadmill_defaults(self):
        """Test ToolBitShapeThreadMill default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("thread-mill")
        self.assertEqual(shape["Diameter"].Value, 5.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["cuttingAngle"].Value, 60.0)
        self.assertEqual(unit(shape["cuttingAngle"]), "°")
        # Need an instance to get parameter labels, get it from the asset manager
        uri = ToolBitShape.resolve_name("thread-mill")
        instance = self.assets.get(uri)
        self.assertEqual(instance.get_parameter_label("cuttingAngle"), "Cutting angle")

    def test_toolbitshapebullnose_defaults(self):
        """Test ToolBitShapeBullnose default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("bullnose")
        self.assertEqual(shape["Diameter"].Value, 5.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["FlatRadius"].Value, 1.5)
        self.assertEqual(unit(shape["FlatRadius"]), "mm")
        # Need an instance to get parameter labels, get it from the asset manager
        uri = ToolBitShape.resolve_name("bullnose")
        instance = self.assets.get(uri)
        self.assertEqual(instance.get_parameter_label("FlatRadius"), "Torus radius")

    def test_toolbitshapevbit_defaults(self):
        """Test ToolBitShapeVBit default parameters and labels."""
        # Provide a dummy filepath for instantiation.
        shape = self._test_shape_common("v-bit")
        self.assertEqual(shape["Diameter"].Value, 10.0)
        self.assertEqual(unit(shape["Diameter"]), "mm")
        self.assertEqual(shape["CuttingEdgeAngle"].Value, 90.0)
        self.assertEqual(unit(shape["CuttingEdgeAngle"]), "°")
        self.assertEqual(shape["TipDiameter"].Value, 1.0)
        self.assertEqual(unit(shape["TipDiameter"]), "mm")
        # Need an instance to get parameter labels, get it from the asset manager
        uri = ToolBitShape.resolve_name("v-bit")
        instance = self.assets.get(uri)
        self.assertEqual(instance.get_parameter_label("CuttingEdgeAngle"), "Cutting edge angle")

    def test_serialize_deserialize(self):
        """
        Tests serialization and deserialization of a ToolBitShape object
        using the Asset interface methods.
        """
        # Load a shape instance from a fixture file
        fixture_path = (
            Path(__file__).parent / "Tools" / "Shape" / "test-path-tool-bit-shape-00.fcstd"
        )
        original_shape = ToolBitShape.from_file(fixture_path)

        # Serialize the shape using the to_bytes method
        serialized_data = original_shape.to_bytes(DummyAssetSerializer)

        # Assert that the serialized data is bytes and not empty
        self.assertIsInstance(serialized_data, bytes)
        self.assertTrue(len(serialized_data) > 0)

        # Deserialize the data using the from_bytes classmethod
        # Provide an empty dependencies mapping for this test
        deserialized_shape = ToolBitShape.from_bytes(
            serialized_data, original_shape.get_id(), {}, DummyAssetSerializer
        )

        # Assert that the deserialized object is a ToolBitShape instance
        self.assertIsInstance(deserialized_shape, ToolBitShape)
        # Assert that the deserialized shape has the same parameters as the original
        self.assertEqual(original_shape.get_parameters(), deserialized_shape.get_parameters())
        self.assertEqual(original_shape.name, deserialized_shape.name)
