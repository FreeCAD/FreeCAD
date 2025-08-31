import json
from typing import Type, cast
import FreeCAD
from CAMTests.PathTestUtils import PathTestWithAssets
from Path.Tool.toolbit import ToolBit, ToolBitEndmill
from Path.Tool.toolbit.serializers import (
    FCTBSerializer,
    CamoticsToolBitSerializer,
)
from Path.Tool.assets.asset import Asset
from Path.Tool.assets.serializer import AssetSerializer
from Path.Tool.assets.uri import AssetUri
from Path.Tool.shape import ToolBitShapeEndmill
from typing import Mapping


class _BaseToolBitSerializerTestCase(PathTestWithAssets):
    """Base test case for ToolBit Serializers."""

    __test__ = False

    serializer_class: Type[AssetSerializer]
    test_tool_bit: ToolBit

    def setUp(self):
        """Create a tool bit for each test."""
        super().setUp()
        if self.serializer_class is None or not issubclass(self.serializer_class, AssetSerializer):
            raise NotImplementedError("Subclasses must define a valid serializer_class")

        self.test_tool_bit = cast(ToolBitEndmill, self.assets.get("toolbit://5mm_Endmill"))
        self.test_tool_bit.label = "Test Tool"
        self.test_tool_bit.set_diameter(FreeCAD.Units.Quantity("4.12 mm"))
        self.test_tool_bit.set_length(FreeCAD.Units.Quantity("15.0 mm"))

    def test_serialize(self):
        """Test serialization of a toolbit."""
        if self.test_tool_bit is None:
            raise NotImplementedError("Subclasses must define a test_tool_bit")
        serialized_data = self.serializer_class.serialize(self.test_tool_bit)
        self.assertIsInstance(serialized_data, bytes)

    def test_extract_dependencies(self):
        """Test dependency extraction."""
        # This test assumes that the serializers don't have dependencies
        # and can be overridden in subclasses if needed.
        serialized_data = self.serializer_class.serialize(self.test_tool_bit)
        dependencies = self.serializer_class.extract_dependencies(serialized_data)
        self.assertIsInstance(dependencies, list)
        self.assertEqual(len(dependencies), 0)


class TestCamoticsToolBitSerializer(_BaseToolBitSerializerTestCase):
    serializer_class = CamoticsToolBitSerializer

    def test_serialize(self):
        super().test_serialize()
        serialized_data = self.serializer_class.serialize(self.test_tool_bit)
        # Camotics specific assertions
        expected_substrings = [
            b'"units": "metric"',
            b'"shape": "Cylindrical"',
            b'"length": 15',
            b'"diameter": 4.12',
            b'"description": "Test Tool"',
        ]
        for substring in expected_substrings:
            self.assertIn(substring, serialized_data)

    def test_deserialize(self):
        # Create a known serialized data string based on the Camotics format
        camotics_data = (
            b'{"units": "metric", "shape": "Cylindrical", "length": 15, '
            b'"diameter": 4.12, "description": "Test Tool"}'
        )
        deserialized_bit = cast(
            ToolBitEndmill,
            self.serializer_class.deserialize(camotics_data, id="test_id", dependencies=None),
        )

        self.assertIsInstance(deserialized_bit, ToolBit)
        self.assertEqual(deserialized_bit.label, "Test Tool")
        self.assertEqual(str(deserialized_bit.get_diameter()), "4.12 mm")
        self.assertEqual(str(deserialized_bit.get_length()), "15.0 mm")
        self.assertEqual(deserialized_bit.get_shape_name(), "Endmill")


class TestFCTBSerializer(_BaseToolBitSerializerTestCase):
    serializer_class = FCTBSerializer

    def test_serialize(self):
        super().test_serialize()
        serialized_data = self.serializer_class.serialize(self.test_tool_bit)
        # FCTB specific assertions (JSON format)
        data = json.loads(serialized_data.decode("utf-8"))
        self.assertEqual(data.get("name"), "Test Tool")
        self.assertEqual(data.get("shape"), "endmill.fcstd")
        self.assertEqual(data.get("parameter", {}).get("Diameter"), "4.12 mm")
        self.assertEqual(data.get("parameter", {}).get("Length"), "15.00 mm", data)

    def test_extract_dependencies(self):
        """Test dependency extraction for FCTB."""
        fctb_data = (
            b'{"name": "Test Tool", "pocket": null, "shape": "endmill", '
            b'"parameter": {"Diameter": "4.12 mm", "Length": "15.0 mm"}, "attribute": {}}'
        )
        dependencies = self.serializer_class.extract_dependencies(fctb_data)
        self.assertIsInstance(dependencies, list)
        self.assertEqual(len(dependencies), 1)
        self.assertEqual(dependencies[0], AssetUri.build("toolbitshape", "endmill"))

    def test_deserialize(self):
        # Create a known serialized data string based on the FCTB format
        fctb_data = (
            b'{"name": "Test Tool", "pocket": null, "shape": "endmill", '
            b'"parameter": {"Diameter": "4.12 mm", "Length": "15.0 mm"}, "attribute": {}}'
        )
        # Create a ToolBitShapeEndmill instance for 'endmill'
        shape = ToolBitShapeEndmill("endmill")

        # Create the dependencies dictionary with the shape instance
        dependencies: Mapping[AssetUri, Asset] = {AssetUri.build("toolbitshape", "endmill"): shape}

        # Provide dummy id and dependencies for deserialization test
        deserialized_bit = cast(
            ToolBitEndmill,
            self.serializer_class.deserialize(fctb_data, id="test_id", dependencies=dependencies),
        )

        self.assertIsInstance(deserialized_bit, ToolBit)
        self.assertEqual(deserialized_bit.label, "Test Tool")
        self.assertEqual(deserialized_bit.get_shape_name(), "Endmill")
        self.assertEqual(str(deserialized_bit.get_diameter()), "4.12 mm")
        self.assertEqual(str(deserialized_bit.get_length()), "15.0 mm")
