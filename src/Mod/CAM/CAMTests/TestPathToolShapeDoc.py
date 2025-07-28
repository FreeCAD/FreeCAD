# -*- coding: utf-8 -*-
# Unit tests for the Path.Tool.Shape module and its document utilities.
import unittest
from unittest.mock import MagicMock
from Path.Tool.shape import doc


mock_obj = MagicMock(Name="Object_Mock")
mock_obj.Label = "MockObjectLabel"
mock_obj.Name = "MockObjectName"

mock_doc = MagicMock(Name="Document_Mock")
mock_doc.Objects = [mock_obj]


class TestPathToolShapeDoc(unittest.TestCase):
    def setUp(self):
        """Reset mocks before each test."""
        # Resetting the top-level mock recursively resets its children
        # (newDocument, getDocument, openDocument, closeDocument, Console, etc.)
        # and their call counts, return_values, side_effects.
        mock_doc.reset_mock()
        mock_obj.reset_mock()

    def test_doc_find_shape_object_body_priority(self):
        """Test find_shape_object prioritizes PartDesign::Body."""
        body_obj = MagicMock(Name="Body_Mock")
        body_obj.isDerivedFrom = lambda typeName: typeName == "PartDesign::Body"
        part_obj = MagicMock(Name="Part_Mock")
        part_obj.isDerivedFrom = lambda typeName: typeName == "Part::Feature"
        mock_doc.Objects = [part_obj, body_obj]
        found = doc.find_shape_object(mock_doc)
        self.assertEqual(found, body_obj)

    def test_doc_find_shape_object_part_fallback(self):
        """Test find_shape_object falls back to Part::Feature."""
        part_obj = MagicMock(Name="Part_Mock")
        part_obj.isDerivedFrom = lambda typeName: typeName == "Part::Feature"
        other_obj = MagicMock(Name="Other_Mock")
        other_obj.isDerivedFrom = lambda typeName: False
        mock_doc.Objects = [other_obj, part_obj]
        found = doc.find_shape_object(mock_doc)
        self.assertEqual(found, part_obj)

    def test_doc_find_shape_object_first_obj_fallback(self):
        """Test find_shape_object falls back to the first object."""
        other_obj1 = MagicMock(Name="Other1_Mock")
        other_obj1.isDerivedFrom = lambda typeName: False
        other_obj2 = MagicMock(Name="Other2_Mock")
        other_obj2.isDerivedFrom = lambda typeName: False
        mock_doc.Objects = [other_obj1, other_obj2]
        found = doc.find_shape_object(mock_doc)
        self.assertEqual(found, other_obj1)

    def test_doc_find_shape_object_no_objects(self):
        """Test find_shape_object returns None if document has no objects."""
        mock_doc.Objects = []
        found = doc.find_shape_object(mock_doc)
        self.assertIsNone(found)


# Test execution
if __name__ == "__main__":
    unittest.main()
