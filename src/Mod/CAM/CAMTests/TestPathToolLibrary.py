import unittest
from Path.Tool.library import Library
from Path.Tool.toolbit import ToolBitEndmill, ToolBitDrill, ToolBitVBit
from Path.Tool.shape import (
    ToolBitShapeEndmill,
    ToolBitShapeDrill,
    ToolBitShapeVBit,
)


class TestPathToolLibrary(unittest.TestCase):
    def test_init(self):
        library = Library("Test Library")
        self.assertEqual(library.label, "Test Library")
        self.assertIsNotNone(library.id)
        self.assertEqual(len(library._bits), 0)
        self.assertEqual(len(library._bit_nos), 0)

    def test_str(self):
        library = Library("My Library", "123-abc")
        self.assertEqual(str(library), '123-abc "My Library"')

    def test_eq(self):
        library1 = Library("Lib1", "same-id")
        library2 = Library("Lib2", "same-id")
        library3 = Library("Lib3", "different-id")
        self.assertEqual(library1, library2)
        self.assertNotEqual(library1, library3)

    def test_iter(self):
        library = Library("Test Library")
        # Create ToolBitShape instances with required parameters
        shape1 = ToolBitShapeEndmill(
            id="dummy_endmill",
            CuttingEdgeHeight=10.0,
            Diameter=10.0,
            Flutes=2,
            Length=50.0,
            ShankDiameter=10.0,
        )
        shape2 = ToolBitShapeDrill(
            id="dummy_drill",
            Diameter=5.0,
            Length=40.0,
            ShankDiameter=5.0,
            Flutes=2,
            TipAngle=118.0,
        )
        bit1 = ToolBitEndmill(shape1)
        bit2 = ToolBitDrill(shape2)
        library.add_bit(bit1)
        library.add_bit(bit2)
        _bits_list = list(library)
        self.assertEqual(len(_bits_list), 2)
        self.assertIn(bit1, _bits_list)
        self.assertIn(bit2, _bits_list)

    def test_get_next_bit_no(self):
        library = Library("Test Library")
        self.assertEqual(library.get_next_bit_no(), 1)
        # Using ToolBit instances in _bit_nos with ToolBitShape
        shape_a = ToolBitShapeEndmill(
            id="dummy_a",
            CuttingEdgeHeight=1.0,
            Diameter=1.0,
            Flutes=1,
            Length=10.0,
            ShankDiameter=1.0,
        )
        shape_b = ToolBitShapeDrill(
            id="dummy_b",
            Diameter=2.0,
            Length=20.0,
            ShankDiameter=2.0,
            Flutes=2,
            TipAngle=118.0,
        )
        shape_c = ToolBitShapeVBit(
            id="dummy_c",
            Diameter=3.0,
            Angle=90.0,
            Length=30.0,
            ShankDiameter=3.0,
            CuttingEdgeAngle=30.0,
            CuttingEdgeHeight=10.0,
            Flutes=2,
            TipDiameter=1.0,
        )
        library._bit_nos = {
            1: ToolBitEndmill(shape_a),
            5: ToolBitDrill(shape_b),
            2: ToolBitVBit(shape_c),
        }
        self.assertEqual(library.get_next_bit_no(), 6)
        library._bit_nos = {}
        self.assertEqual(library.get_next_bit_no(), 1)

    def test_get_bit_no_from_bit(self):
        library = Library("Test Library")
        shape1 = ToolBitShapeEndmill(
            id="dummy_endmill_1",
            CuttingEdgeHeight=10.0,
            Diameter=10.0,
            Flutes=2,
            Length=50.0,
            ShankDiameter=10.0,
        )
        shape2 = ToolBitShapeDrill(
            id="dummy_drill_1",
            Diameter=5.0,
            Length=40.0,
            ShankDiameter=5.0,
            Flutes=2,
            TipAngle=118.0,
        )
        bit1 = ToolBitEndmill(shape1)
        bit2 = ToolBitDrill(shape2)
        library.add_bit(bit1, 1)
        library.add_bit(bit2, 2)
        self.assertEqual(library.get_bit_no_from_bit(bit1), 1)
        self.assertEqual(library.get_bit_no_from_bit(bit2), 2)
        shape_cutter = ToolBitShapeVBit(
            id="dummy_cutter_1",
            Diameter=3.0,
            Angle=90.0,
            Length=30.0,
            ShankDiameter=3.0,
            CuttingEdgeAngle=30.0,
            CuttingEdgeHeight=10.0,
            Flutes=2,
            TipDiameter=1.0,
        )
        self.assertIsNone(library.get_bit_no_from_bit(ToolBitVBit(shape_cutter)))

    def test_assign_new_bit_no(self):
        library = Library("Test Library")
        shape1 = ToolBitShapeEndmill(
            id="dummy_endmill_2",
            CuttingEdgeHeight=10.0,
            Diameter=10.0,
            Flutes=2,
            Length=50.0,
            ShankDiameter=10.0,
        )
        shape2 = ToolBitShapeDrill(
            id="dummy_drill_2",
            Diameter=5.0,
            Length=40.0,
            ShankDiameter=5.0,
            Flutes=2,
            TipAngle=118.0,
        )
        bit1 = ToolBitEndmill(shape1)
        bit2 = ToolBitDrill(shape2)

        # Assign bit1 without specifying number (should get 1)
        library.add_bit(bit1)
        self.assertEqual(len(library._bits), 1)
        self.assertEqual(len(library._bit_nos), 1)
        self.assertIn(1, library._bit_nos)
        self.assertEqual(library._bit_nos[1], bit1)

        # Assign bit2 to number 1 (should reassign bit1)
        library.add_bit(bit2, 1)
        self.assertEqual(len(library._bits), 2)
        self.assertEqual(len(library._bit_nos), 2)
        self.assertIn(1, library._bit_nos)
        self.assertEqual(library._bit_nos[1], bit2)
        # Check if bit1 was reassigned to a new bit number (should be 2)
        self.assertIn(2, library._bit_nos)
        self.assertEqual(library._bit_nos[2], bit1)

        # Assign bit2 to number 10
        library.assign_new_bit_no(bit2, 10)
        self.assertEqual(len(library._bits), 2)
        self.assertEqual(len(library._bit_nos), 2)
        self.assertIn(10, library._bit_nos)
        self.assertEqual(library._bit_nos[10], bit2)
        self.assertNotIn(1, library._bit_nos)  # bit2 should no longer be at 1
        self.assertIn(2, library._bit_nos)
        self.assertEqual(library._bit_nos[2], bit1)

        # Assign bit1 to number 5
        library.assign_new_bit_no(bit1, 5)
        self.assertEqual(len(library._bits), 2)
        self.assertEqual(len(library._bit_nos), 2)
        self.assertIn(5, library._bit_nos)
        self.assertEqual(library._bit_nos[5], bit1)
        self.assertNotIn(2, library._bit_nos)  # bit1 should no longer be at 2
        self.assertIn(10, library._bit_nos)
        self.assertEqual(library._bit_nos[10], bit2)

    def test_add_bit(self):
        library = Library("Test Library")
        shape1 = ToolBitShapeEndmill(
            id="dummy_endmill_3",
            CuttingEdgeHeight=10.0,
            Diameter=10.0,
            Flutes=2,
            Length=50.0,
            ShankDiameter=10.0,
        )
        shape2 = ToolBitShapeDrill(
            id="dummy_drill_3",
            Diameter=5.0,
            Length=40.0,
            ShankDiameter=5.0,
            Flutes=2,
            TipAngle=118.0,
        )
        bit1 = ToolBitEndmill(shape1)
        bit2 = ToolBitDrill(shape2)

        library.add_bit(bit1)
        self.assertEqual(len(library._bits), 1)
        self.assertIn(bit1, library._bits)
        self.assertEqual(len(library._bit_nos), 1)
        self.assertIn(1, library._bit_nos)
        self.assertEqual(library._bit_nos[1], bit1)

        library.add_bit(bit2, 5)
        self.assertEqual(len(library._bits), 2)
        self.assertIn(bit1, library._bits)
        self.assertIn(bit2, library._bits)
        self.assertEqual(len(library._bit_nos), 2)
        self.assertIn(1, library._bit_nos)
        self.assertEqual(library._bit_nos[1], bit1)
        self.assertIn(5, library._bit_nos)
        self.assertEqual(library._bit_nos[5], bit2)

        # Add bit1 again (should not increase bit count in _bits list)
        library.add_bit(bit1, 10)
        self.assertEqual(len(library._bits), 2)
        self.assertIn(bit1, library._bits)
        self.assertIn(bit2, library._bits)
        self.assertEqual(len(library._bit_nos), 2)  # _bit_nos count remains 2
        self.assertIn(10, library._bit_nos)
        self.assertEqual(library._bit_nos[10], bit1)
        self.assertIn(5, library._bit_nos)
        self.assertEqual(library._bit_nos[5], bit2)
        self.assertNotIn(1, library._bit_nos)  # bit1 should no longer be at 1

    def test_get_bits(self):
        library = Library("Test Library")
        shape1 = ToolBitShapeEndmill(
            id="dummy_endmill_4",
            CuttingEdgeHeight=10.0,
            Diameter=10.0,
            Flutes=2,
            Length=50.0,
            ShankDiameter=10.0,
        )
        shape2 = ToolBitShapeDrill(
            id="dummy_drill_4",
            Diameter=5.0,
            Length=40.0,
            ShankDiameter=5.0,
            Flutes=2,
            TipAngle=118.0,
        )
        bit1 = ToolBitEndmill(shape1)
        bit2 = ToolBitDrill(shape2)
        self.assertEqual(library.get_bits(), [])
        library.add_bit(bit1)
        library.add_bit(bit2)
        _bits_list = library.get_bits()
        self.assertEqual(len(_bits_list), 2)
        self.assertIn(bit1, _bits_list)
        self.assertIn(bit2, _bits_list)

    def test_has_bit(self):
        library = Library("Test Library")
        shape1 = ToolBitShapeEndmill(
            id="dummy_endmill_5",
            CuttingEdgeHeight=10.0,
            Diameter=10.0,
            Flutes=2,
            Length=50.0,
            ShankDiameter=10.0,
        )
        shape2 = ToolBitShapeDrill(
            id="dummy_drill_5",
            Diameter=5.0,
            Length=40.0,
            ShankDiameter=5.0,
            Flutes=2,
            TipAngle=118.0,
        )
        bit1 = ToolBitEndmill(shape1)
        bit2 = ToolBitDrill(shape2)
        library.add_bit(bit1)
        self.assertTrue(library.has_bit(bit1))
        self.assertFalse(library.has_bit(bit2))
        # Create a new ToolBit with the same properties but different instance
        shape1_copy = ToolBitShapeEndmill(
            id="dummy_endmill_5_copy",  # Use a different ID for the copy
            CuttingEdgeHeight=10.0,
            Diameter=10.0,
            Flutes=2,
            Length=50.0,
            ShankDiameter=10.0,
        )
        bit1_copy = ToolBitEndmill(shape1_copy)
        self.assertFalse(library.has_bit(bit1_copy))

    def test_remove_bit(self):
        library = Library("Test Library")
        shape1 = ToolBitShapeEndmill(
            id="dummy_endmill_6",
            CuttingEdgeHeight=10.0,
            Diameter=10.0,
            Flutes=2,
            Length=50.0,
            ShankDiameter=10.0,
        )
        shape2 = ToolBitShapeDrill(
            id="dummy_drill_6",
            Diameter=5.0,
            Length=40.0,
            ShankDiameter=5.0,
            Flutes=2,
            TipAngle=118.0,
        )
        shape3 = ToolBitShapeVBit(
            id="dummy_cutter_6",
            Diameter=3.0,
            Angle=90.0,
            Length=30.0,
            ShankDiameter=3.0,
            CuttingEdgeAngle=30.0,
            CuttingEdgeHeight=10.0,
            Flutes=2,
            TipDiameter=1.0,
        )
        bit1 = ToolBitEndmill(shape1)
        bit2 = ToolBitDrill(shape2)
        bit3 = ToolBitVBit(shape3)

        library.add_bit(bit1, 1)
        library.add_bit(bit2, 2)
        library.add_bit(bit3, 3)
        self.assertEqual(len(library._bits), 3)
        self.assertEqual(len(library._bit_nos), 3)

        library.remove_bit(bit2)
        self.assertEqual(len(library._bits), 2)
        self.assertNotIn(bit2, library._bits)
        self.assertEqual(len(library._bit_nos), 2)
        self.assertNotIn(2, library._bit_nos)
        self.assertNotIn(bit2, library._bit_nos.values())

        library.remove_bit(bit1)
        self.assertEqual(len(library._bits), 1)
        self.assertNotIn(bit1, library._bits)
        self.assertEqual(len(library._bit_nos), 1)
        self.assertNotIn(1, library._bit_nos)
        self.assertNotIn(bit1, library._bit_nos.values())

        library.remove_bit(bit3)
        self.assertEqual(len(library._bits), 0)
        self.assertNotIn(bit3, library._bits)
        self.assertEqual(len(library._bit_nos), 0)
        self.assertNotIn(3, library._bit_nos)
        self.assertNotIn(bit3, library._bit_nos.values())

        # Removing a non-existent bit should not raise an error
        shape_nonexistent = ToolBitShapeEndmill(
            id="dummy_nonexistent_6",
            CuttingEdgeHeight=99.0,
            Diameter=99.0,
            Flutes=1,
            Length=99.0,
            ShankDiameter=99.0,
        )
        library.remove_bit(ToolBitEndmill(shape_nonexistent))
        self.assertEqual(len(library._bits), 0)
        self.assertEqual(len(library._bit_nos), 0)


if __name__ == "__main__":
    unittest.main()
