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
        self.assertEqual(len(library.tools), 0)
        self.assertEqual(len(library.tool_nos), 0)

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
        tool1 = ToolBitEndmill(shape1)
        tool2 = ToolBitDrill(shape2)
        library.add_tool(tool1)
        library.add_tool(tool2)
        tools_list = list(library)
        self.assertEqual(len(tools_list), 2)
        self.assertIn(tool1, tools_list)
        self.assertIn(tool2, tools_list)

    def test_get_next_tool_no(self):
        library = Library("Test Library")
        self.assertEqual(library.get_next_tool_no(), 1)
        # Using ToolBit instances in tool_nos with ToolBitShape
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
        library.tool_nos = {
            1: ToolBitEndmill(shape_a),
            5: ToolBitDrill(shape_b),
            2: ToolBitVBit(shape_c),
        }
        self.assertEqual(library.get_next_tool_no(), 6)
        library.tool_nos = {}
        self.assertEqual(library.get_next_tool_no(), 1)

    def test_get_tool_no_from_tool(self):
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
        tool1 = ToolBitEndmill(shape1)
        tool2 = ToolBitDrill(shape2)
        library.add_tool(tool1, 1)
        library.add_tool(tool2, 2)
        self.assertEqual(library.get_tool_no_from_tool(tool1), 1)
        self.assertEqual(library.get_tool_no_from_tool(tool2), 2)
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
        self.assertIsNone(
            library.get_tool_no_from_tool(ToolBitVBit(shape_cutter))
        )

    def test_assign_new_tool_no(self):
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
        tool1 = ToolBitEndmill(shape1)
        tool2 = ToolBitDrill(shape2)

        # Assign tool1 without specifying number (should get 1)
        library.add_tool(tool1)
        self.assertEqual(len(library.tools), 1)
        self.assertEqual(len(library.tool_nos), 1)
        self.assertIn(1, library.tool_nos)
        self.assertEqual(library.tool_nos[1], tool1)

        # Assign tool2 to number 1 (should reassign tool1)
        library.add_tool(tool2, 1)
        self.assertEqual(len(library.tools), 2)
        self.assertEqual(len(library.tool_nos), 2)
        self.assertIn(1, library.tool_nos)
        self.assertEqual(library.tool_nos[1], tool2)
        # Check if tool1 was reassigned to a new tool number (should be 2)
        self.assertIn(2, library.tool_nos)
        self.assertEqual(library.tool_nos[2], tool1)

        # Assign tool2 to number 10
        library.assign_new_tool_no(tool2, 10)
        self.assertEqual(len(library.tools), 2)
        self.assertEqual(len(library.tool_nos), 2)
        self.assertIn(10, library.tool_nos)
        self.assertEqual(library.tool_nos[10], tool2)
        self.assertNotIn(1, library.tool_nos)  # tool2 should no longer be at 1
        self.assertIn(2, library.tool_nos)
        self.assertEqual(library.tool_nos[2], tool1)

        # Assign tool1 to number 5
        library.assign_new_tool_no(tool1, 5)
        self.assertEqual(len(library.tools), 2)
        self.assertEqual(len(library.tool_nos), 2)
        self.assertIn(5, library.tool_nos)
        self.assertEqual(library.tool_nos[5], tool1)
        self.assertNotIn(2, library.tool_nos)  # tool1 should no longer be at 2
        self.assertIn(10, library.tool_nos)
        self.assertEqual(library.tool_nos[10], tool2)

    def test_add_tool(self):
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
        tool1 = ToolBitEndmill(shape1)
        tool2 = ToolBitDrill(shape2)

        library.add_tool(tool1)
        self.assertEqual(len(library.tools), 1)
        self.assertIn(tool1, library.tools)
        self.assertEqual(len(library.tool_nos), 1)
        self.assertIn(1, library.tool_nos)
        self.assertEqual(library.tool_nos[1], tool1)

        library.add_tool(tool2, 5)
        self.assertEqual(len(library.tools), 2)
        self.assertIn(tool1, library.tools)
        self.assertIn(tool2, library.tools)
        self.assertEqual(len(library.tool_nos), 2)
        self.assertIn(1, library.tool_nos)
        self.assertEqual(library.tool_nos[1], tool1)
        self.assertIn(5, library.tool_nos)
        self.assertEqual(library.tool_nos[5], tool2)

        # Add tool1 again (should not increase tool count in tools list)
        library.add_tool(tool1, 10)
        self.assertEqual(len(library.tools), 2)
        self.assertIn(tool1, library.tools)
        self.assertIn(tool2, library.tools)
        self.assertEqual(len(library.tool_nos), 2)  # tool_nos count remains 2
        self.assertIn(10, library.tool_nos)
        self.assertEqual(library.tool_nos[10], tool1)
        self.assertIn(5, library.tool_nos)
        self.assertEqual(library.tool_nos[5], tool2)
        self.assertNotIn(1, library.tool_nos)  # tool1 should no longer be at 1

    def test_get_tools(self):
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
        tool1 = ToolBitEndmill(shape1)
        tool2 = ToolBitDrill(shape2)
        self.assertEqual(library.get_tools(), [])
        library.add_tool(tool1)
        library.add_tool(tool2)
        tools_list = library.get_tools()
        self.assertEqual(len(tools_list), 2)
        self.assertIn(tool1, tools_list)
        self.assertIn(tool2, tools_list)

    def test_has_tool(self):
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
        tool1 = ToolBitEndmill(shape1)
        tool2 = ToolBitDrill(shape2)
        library.add_tool(tool1)
        self.assertTrue(library.has_tool(tool1))
        self.assertFalse(library.has_tool(tool2))
        # Create a new ToolBit with the same properties but different instance
        shape1_copy = ToolBitShapeEndmill(
            id="dummy_endmill_5_copy", # Use a different ID for the copy
            CuttingEdgeHeight=10.0,
            Diameter=10.0,
            Flutes=2,
            Length=50.0,
            ShankDiameter=10.0,
        )
        tool1_copy = ToolBitEndmill(shape1_copy)
        self.assertFalse(library.has_tool(tool1_copy))

    def test_remove_tool(self):
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
        tool1 = ToolBitEndmill(shape1)
        tool2 = ToolBitDrill(shape2)
        tool3 = ToolBitVBit(shape3)

        library.add_tool(tool1, 1)
        library.add_tool(tool2, 2)
        library.add_tool(tool3, 3)
        self.assertEqual(len(library.tools), 3)
        self.assertEqual(len(library.tool_nos), 3)

        library.remove_tool(tool2)
        self.assertEqual(len(library.tools), 2)
        self.assertNotIn(tool2, library.tools)
        self.assertEqual(len(library.tool_nos), 2)
        self.assertNotIn(2, library.tool_nos)
        self.assertNotIn(tool2, library.tool_nos.values())

        library.remove_tool(tool1)
        self.assertEqual(len(library.tools), 1)
        self.assertNotIn(tool1, library.tools)
        self.assertEqual(len(library.tool_nos), 1)
        self.assertNotIn(1, library.tool_nos)
        self.assertNotIn(tool1, library.tool_nos.values())

        library.remove_tool(tool3)
        self.assertEqual(len(library.tools), 0)
        self.assertNotIn(tool3, library.tools)
        self.assertEqual(len(library.tool_nos), 0)
        self.assertNotIn(3, library.tool_nos)
        self.assertNotIn(tool3, library.tool_nos.values())

        # Removing a non-existent tool should not raise an error
        shape_nonexistent = ToolBitShapeEndmill(
            id="dummy_nonexistent_6",
            CuttingEdgeHeight=99.0,
            Diameter=99.0,
            Flutes=1,
            Length=99.0,
            ShankDiameter=99.0,
        )
        library.remove_tool(ToolBitEndmill(shape_nonexistent))
        self.assertEqual(len(library.tools), 0)
        self.assertEqual(len(library.tool_nos), 0)


if __name__ == "__main__":
    unittest.main()
