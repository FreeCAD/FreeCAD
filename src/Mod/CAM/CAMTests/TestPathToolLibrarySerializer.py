# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest
import json
import FreeCAD
from CAMTests.PathTestUtils import PathTestWithAssets
from Path.Tool.library import Library
from Path.Tool.toolbit import ToolBitEndmill
from Path.Tool.shape import ToolBitShapeEndmill, ToolBitShapeBallend
from Path.Tool.library.serializers import CamoticsLibrarySerializer, LinuxCNCSerializer


class TestPathToolLibrarySerializerBase(PathTestWithAssets):
    """Base class for Library serializer tests."""

    def setUp(self):
        super().setUp()
        self.test_library_id = "test_library"
        self.test_library_label = "Test Library"
        self.test_library = Library(self.test_library_label, id=self.test_library_id)

        # Create some dummy tool bits
        shape1 = ToolBitShapeEndmill("endmill_1")
        shape1.set_parameter("Diameter", FreeCAD.Units.Quantity("6.0 mm"))
        shape1.set_parameter("Length", FreeCAD.Units.Quantity("20.0 mm"))
        tool1 = ToolBitEndmill(shape1, id="tool_1")
        tool1.label = "Endmill 6mm"

        shape2 = ToolBitShapeEndmill("endmill_2")
        shape2.set_parameter("Diameter", FreeCAD.Units.Quantity("3.0 mm"))
        shape2.set_parameter("Length", FreeCAD.Units.Quantity("15.0 mm"))
        tool2 = ToolBitEndmill(shape2, id="tool_2")
        tool2.label = "Endmill 3mm"

        shape3 = ToolBitShapeBallend("ballend_1")
        shape3.set_parameter("Diameter", FreeCAD.Units.Quantity("5.0 mm"))
        shape3.set_parameter("Length", FreeCAD.Units.Quantity("18.0 mm"))
        tool3 = ToolBitEndmill(shape3, id="tool_3")
        tool3.label = "Ballend 5mm"

        self.test_library.add_bit(tool1, 1)
        self.test_library.add_bit(tool2, 2)
        self.test_library.add_bit(tool3, 3)


class TestCamoticsLibrarySerializer(TestPathToolLibrarySerializerBase):
    """Tests for the CamoticsLibrarySerializer."""

    def test_camotics_serialize(self):
        serializer = CamoticsLibrarySerializer
        serialized_data = serializer.serialize(self.test_library)
        self.assertIsInstance(serialized_data, bytes)

        # Verify the content structure (basic check)
        data_dict = json.loads(serialized_data.decode("utf-8"))
        self.assertEqual(
            data_dict["1"],
            {
                "description": "Endmill 6mm",
                "diameter": 6.0,
                "length": 20.0,
                "shape": "Cylindrical",
                "units": "metric",
            },
        )
        self.assertEqual(
            data_dict["2"],
            {
                "description": "Endmill 3mm",
                "diameter": 3.0,
                "length": 15.0,
                "shape": "Cylindrical",
                "units": "metric",
            },
        )
        self.assertEqual(
            data_dict["3"],
            {
                "description": "Ballend 5mm",
                "diameter": 5.0,
                "length": 18.0,
                "shape": "Ballnose",
                "units": "metric",
            },
        )

    def test_camotics_deserialize(self):
        serializer = CamoticsLibrarySerializer
        # Create a dummy serialized data matching the expected format
        dummy_data = {
            "10": {
                "units": "metric",
                "shape": "Ballnose",
                "length": 25,
                "diameter": 8,
                "description": "Ballnose 8mm",
            },
            "20": {
                "units": "metric",
                "shape": "Cylindrical",
                "length": 30,
                "diameter": 10,
                "description": "Endmill 10mm",
            },
        }
        dummy_bytes = json.dumps(dummy_data, indent=2).encode("utf-8")

        # Deserialize the data
        deserialized_library = serializer.deserialize(dummy_bytes, "deserialized_lib", {})

        self.assertIsInstance(deserialized_library, Library)
        self.assertEqual(deserialized_library.get_id(), "deserialized_lib")
        self.assertEqual(len(deserialized_library._bit_nos), 2)

        tool_10 = deserialized_library._bit_nos.get(10)
        assert tool_10 is not None, "tool not in the library"
        self.assertEqual(tool_10.label, "Ballnose 8mm")
        self.assertEqual(tool_10._tool_bit_shape.name, "Ballend")
        self.assertEqual(
            tool_10._tool_bit_shape.get_parameter("Diameter"), FreeCAD.Units.Quantity("8 mm")
        )
        self.assertEqual(
            tool_10._tool_bit_shape.get_parameter("Length"), FreeCAD.Units.Quantity("25 mm")
        )

        tool_20 = deserialized_library._bit_nos.get(20)
        assert tool_20 is not None, "tool not in the library"
        self.assertEqual(tool_20.label, "Endmill 10mm")
        self.assertEqual(tool_20._tool_bit_shape.name, "Endmill")
        self.assertEqual(
            tool_20._tool_bit_shape.get_parameter("Diameter"), FreeCAD.Units.Quantity("10 mm")
        )
        self.assertEqual(
            tool_20._tool_bit_shape.get_parameter("Length"), FreeCAD.Units.Quantity("30 mm")
        )


class TestLinuxCNCLibrarySerializer(TestPathToolLibrarySerializerBase):
    """Tests for the LinuxCNCLibrarySerializer."""

    def test_linuxcnc_serialize(self):
        # TODO: this test uses the user-preferences for the tester's installed version of FreeCAD
        # i.e., it depends on what the developer happened to set last.
        # it probably shouldn't: set the pref, and or test several unit-systems
        serializer = LinuxCNCSerializer
        serialized_data = serializer.serialize(self.test_library)
        self.assertIsInstance(serialized_data, bytes)

        decimals = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt(
            "Decimals", 2
        )

        def format_D(v):
            # Convert and format to userPreferred
            # It is important to get the units for userPreferred from the actual `v`,
            # because the units are dependant on the order-of-magnitude of v
            # for some user-preference unit-systems.
            # E.g. U.S. Customary will give `thous` for 1mm, `"` (inch) for 6mm, etc.
            as_quantity = FreeCAD.Units.Quantity(v)
            units = as_quantity.getUserPreferred()[2]
            in_user_units = as_quantity.getValueAs(units).Value
            return f"{in_user_units:.{decimals}f}"

        # Verify the content format (basic check)
        lines = serialized_data.decode("ascii", "ignore").strip().split("\n")
        self.assertEqual(len(lines), 3)

        # D values from setUp()
        # TODO: this will fail when Tool/library/serializers/linuxcnc.py serialize() uses MBPP, as noted in its TODO
        # and the test will have to use MBPP in place of the userPreferred stuff above
        self.assertEqual(
            lines[0], f"T1 P0 X0 Y0 Z0 A0 B0 C0 U0 V0 W0 D{format_D('6mm')} I0 J0 Q0 ;Endmill 6mm"
        )
        self.assertEqual(
            lines[1], f"T2 P0 X0 Y0 Z0 A0 B0 C0 U0 V0 W0 D{format_D('3mm')} I0 J0 Q0 ;Endmill 3mm"
        )
        self.assertEqual(
            lines[2], f"T3 P0 X0 Y0 Z0 A0 B0 C0 U0 V0 W0 D{format_D('5mm')} I0 J0 Q0 ;Ballend 5mm"
        )

    def test_linuxcnc_deserialize_not_implemented(self):
        serializer = LinuxCNCSerializer
        dummy_data = b"T1 P0 X0 Y0 Z0 A0 B0 C0 U0 V0 W0 D6.00 I0 J0 Q0 ;Endmill 6mm\n"
        with self.assertRaises(NotImplementedError):
            serializer.deserialize(dummy_data, "dummy_id", {})


if __name__ == "__main__":
    unittest.main()
