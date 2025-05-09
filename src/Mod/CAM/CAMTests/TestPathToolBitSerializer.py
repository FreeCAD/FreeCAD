import unittest
import json
import pathlib
import FreeCAD
from Path.Tool.shape.store import toolbitshape_store
from Path.Tool.toolbit import ToolBit, TOOLBIT_REGISTRY
from Path.Tool.toolbit.serializers import (
    ToolBitSerializer,
    CamoticsToolBitSerializer,
    FCTBSerializer,
    LinuxCNCToolBitSerializer,
)


class BaseToolBitSerializerTestCase(unittest.TestCase):
    """Base test case for ToolBit Serializers."""

    serializer_class = None
    test_tool_bit = None

    def setUp(self):
        """Create serializer instance and a tool bit for each test."""
        if self.serializer_class is None or not issubclass(
            self.serializer_class, ToolBitSerializer
        ):
            raise NotImplementedError(
                "Subclasses must define a valid serializer_class"
            )
        self.serializer = self.serializer_class()

        # Create a ToolBit instance that serializers can use
        tool_dir = pathlib.Path(__file__).parent.parent / "Tools"
        toolbitshape_store.set_dir(tool_dir / "Shape")
        TOOLBIT_REGISTRY.set_dir(tool_dir / "Bit")

        self.test_tool_bit = TOOLBIT_REGISTRY.get_bit_from_filename("5mm_Endmill.fctb")
        self.test_tool_bit.set_label("Test Tool")
        self.test_tool_bit.set_diameter(FreeCAD.Units.Quantity("4.12 mm"))
        self.test_tool_bit.set_length(FreeCAD.Units.Quantity("15.0 mm"))

    def test_serialize_toolbit(self):
        """Test serialization of a toolbit."""
        if self.test_tool_bit is None:
            raise NotImplementedError("Subclasses must define a test_tool_bit")
        serialized_data = self.serializer.serialize_toolbit(self.test_tool_bit)
        self.assertIsInstance(serialized_data, bytes)


class TestCamoticsToolBitSerializer(BaseToolBitSerializerTestCase):
    serializer_class = CamoticsToolBitSerializer

    def test_serialize_toolbit(self):
        super().test_serialize_toolbit()
        serialized_data = self.serializer.serialize_toolbit(self.test_tool_bit)
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

    def test_deserialize_toolbit(self):
        # Create a known serialized data string based on the Camotics format
        camotics_data = (
            b'{"units": "metric", "shape": "Cylindrical", "length": 15, '
            b'"diameter": 4.12, "description": "Test Tool"}'
        )
        deserialized_bit = self.serializer.deserialize_toolbit(camotics_data)

        self.assertIsInstance(deserialized_bit, ToolBit)
        self.assertEqual(deserialized_bit.get_label(), "Test Tool")
        self.assertEqual(str(deserialized_bit.get_diameter()), "4.12 mm")
        self.assertEqual(str(deserialized_bit.get_length()), "15.0 mm")
        self.assertEqual(deserialized_bit.get_shape_name(), "Endmill")


class TestFCTBSerializer(BaseToolBitSerializerTestCase):
    serializer_class = FCTBSerializer

    def test_serialize_toolbit(self):
        super().test_serialize_toolbit()
        serialized_data = self.serializer.serialize_toolbit(self.test_tool_bit)
        # FCTB specific assertions (JSON format)
        data = json.loads(serialized_data.decode("utf-8"))
        self.assertEqual(data.get("name"), "Test Tool")
        self.assertEqual(data.get("shape"), "endmill.fcstd")
        self.assertEqual(data.get("parameter", {}).get("Diameter"), "4.12 mm")
        self.assertEqual(data.get("parameter", {}).get("Length"), "15.0 mm", data)

    def test_deserialize_toolbit(self):
        # Create a known serialized data string based on the FCTB format
        fctb_data = (
            b'{"name": "Test Tool", "pocket": null, "shape": "endmill", '
            b'"parameter": {"Diameter": "4.12 mm", "Length": "15.0 mm"}, "attribute": {}}'
        )
        deserialized_bit = self.serializer.deserialize_toolbit(fctb_data)

        self.assertIsInstance(deserialized_bit, ToolBit)
        self.assertEqual(deserialized_bit.get_label(), "Test Tool")
        self.assertEqual(deserialized_bit.get_shape_name(), "Endmill")
        self.assertEqual(str(deserialized_bit.get_diameter()), "4.12 mm")
        self.assertEqual(str(deserialized_bit.get_length()), "15.0 mm")


class TestLinuxCNCToolBitSerializer(BaseToolBitSerializerTestCase):
    serializer_class = LinuxCNCToolBitSerializer

    def test_serialize_toolbit(self):
        super().test_serialize_toolbit()
        # LinuxCNC serializer uses an internal ID_POOL for the tool number
        # and expects the pocket number to be available via bit.get_pocket()
        # at some point (based on the TODO comment in the serializer).
        # The serialized output format is "T<ID_POOL> P<pocket> D<diameter> ;<label>"
        serialized_data = self.serializer.serialize_toolbit(self.test_tool_bit)
        expected_output_pattern = rb"T\d+ P\d* D4\.12 ;Test Tool"
        self.assertRegex(serialized_data, expected_output_pattern)

    def test_deserialize_toolbit(self):
        """Test deserialization of a toolbit (should raise NotImplementedError)."""
        with self.assertRaises(NotImplementedError):
            self.serializer.deserialize_toolbit(b"some data")
