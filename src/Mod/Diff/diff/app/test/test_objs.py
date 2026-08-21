# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 Pieter Hijma <info@pieterhijma.net>                 *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import unittest

import FreeCAD

from diff.app.compute_diff import compute_diff, DiffResultDocument, DiffResultPropertyContainer

class TestDiffDocument(unittest.TestCase):
    def test_empty_doc(self):
        """Test that two empty documents are considered the same.
        """
        doc1 = FreeCAD.newDocument("TestDoc1")
        doc2 = FreeCAD.newDocument("TestDoc2")

        diff_doc: DiffResultDocument = compute_diff(doc1, doc2)

        self.assertEqual(len(diff_doc.objs_only_in_left), 0)
        self.assertEqual(len(diff_doc.objs_only_in_right), 0)
        self.assertEqual(len(diff_doc._objs_in_both), 0)
        self.assertEqual(len(diff_doc.objs_different), 0)
        self.assertEqual(len(diff_doc.objs_same), 0)

        diff_props: DiffResultPropertyContainer = diff_doc.props

        self.assertEqual(len(diff_props.props_only_in_left), 0)
        self.assertEqual(len(diff_props.props_only_in_right), 0)
        self.assertTrue(len(diff_props.props_in_both) > 0)
        self.assertIn("Label", diff_props.props_different)
        self.assertIn("Uid", diff_props.props_different)
        self.assertIn("TransientDir", diff_props.props_different)

        FreeCAD.closeDocument(doc1.Name)
        FreeCAD.closeDocument(doc2.Name)

    def test_obj_only_in_left(self):
        """Test that an object that exists only in the left document is correctly identified.
        """
        doc1 = FreeCAD.newDocument("TestDoc1")
        doc2 = FreeCAD.newDocument("TestDoc2")
        obj = doc1.addObject("Part::Box", "Box")

        diff_doc: DiffResultDocument = compute_diff(doc1, doc2)

        self.assertEqual(len(diff_doc.objs_only_in_left), 1)
        self.assertIn(obj, diff_doc.objs_only_in_left)
        self.assertEqual(len(diff_doc.objs_only_in_right), 0)
        self.assertEqual(len(diff_doc._objs_in_both), 0)

        FreeCAD.closeDocument(doc1.Name)
        FreeCAD.closeDocument(doc2.Name)

    def test_obj_common(self):
        """Test that common objects are correctly identified.
        """
        def add_objects(doc):
            doc.addObject("Part::Box", "Box")
            doc.addObject("Part::Box", "Box")

        doc1 = FreeCAD.newDocument("TestDoc1")
        add_objects(doc1)

        doc2 = FreeCAD.newDocument("TestDoc2")
        add_objects(doc2)

        diff_doc: DiffResultDocument = compute_diff(doc1, doc2)

        self.assertEqual(len(diff_doc.objs_only_in_left), 0)
        self.assertEqual(len(diff_doc.objs_only_in_right), 0)
        self.assertEqual(len(diff_doc._objs_in_both), 2)
        self.assertIn("Box", diff_doc._objs_in_both)
        self.assertIn("Box001", diff_doc._objs_in_both)

        FreeCAD.closeDocument(doc1.Name)
        FreeCAD.closeDocument(doc2.Name)

    def test_different_type(self):
        """Test that different obj types are not the same.
        """
        doc1 = FreeCAD.newDocument("TestDoc1")
        box = doc1.addObject("Part::Box", "Box")

        doc2 = FreeCAD.newDocument("TestDoc2")
        cylinder = doc2.addObject("Part::Cylinder", "Box")

        diff_doc: DiffResultDocument = compute_diff(doc1, doc2)

        self.assertEqual(len(diff_doc.objs_only_in_left), 1)
        self.assertEqual(len(diff_doc.objs_only_in_right), 1)
        self.assertIn(box, diff_doc.objs_only_in_left)
        self.assertIn(cylinder, diff_doc.objs_only_in_right)
        self.assertEqual(len(diff_doc._objs_in_both), 0)

        FreeCAD.closeDocument(doc1.Name)
        FreeCAD.closeDocument(doc2.Name)

class TestDiffPropertyContainer(unittest.TestCase):
    def test_prop_only_in_left(self):
        """Test that a property that exists only in the left document is correctly identified.
        """
        doc1 = FreeCAD.newDocument("TestDoc1")
        doc2 = FreeCAD.newDocument("TestDoc2")
        prop_name = "MyNumber"
        doc1.addProperty("App::PropertyInteger", prop_name)

        diff_doc: DiffResultPropertyContainer = compute_diff(doc1, doc2).props

        self.assertEqual(len(diff_doc.props_only_in_left), 1)
        self.assertIn(prop_name, diff_doc.props_only_in_left)

        self.assertEqual(len(diff_doc.props_only_in_right), 0)

        FreeCAD.closeDocument(doc1.Name)
        FreeCAD.closeDocument(doc2.Name)

    def test_prop_common(self):
        """Test that common properties are correctly identified.
        """
        prop_name = "MyNumber"

        def add_prop(doc):
            doc.addProperty("App::PropertyInteger", prop_name)

        doc1 = FreeCAD.newDocument("TestDoc1")
        add_prop(doc1)

        doc2 = FreeCAD.newDocument("TestDoc2")
        add_prop(doc2)

        diff_doc: DiffResultPropertyContainer = compute_diff(doc1, doc2).props

        self.assertEqual(len(diff_doc.props_only_in_left), 0)
        self.assertEqual(len(diff_doc.props_only_in_right), 0)
        self.assertIn(prop_name, diff_doc.props_in_both)

        FreeCAD.closeDocument(doc1.Name)
        FreeCAD.closeDocument(doc2.Name)

    def test_different_type(self):
        """Test that different property types are not the same.
        """
        prop_name = "MyNumber"
        doc1 = FreeCAD.newDocument("TestDoc1")
        doc1.addProperty("App::PropertyInteger", prop_name)

        doc2 = FreeCAD.newDocument("TestDoc2")
        doc2.addProperty("App::PropertyFloat", prop_name)

        diff_doc: DiffResultPropertyContainer = compute_diff(doc1, doc2).props

        self.assertEqual(len(diff_doc.props_only_in_left), 1)
        self.assertEqual(len(diff_doc.props_only_in_right), 1)
        self.assertIn(prop_name, diff_doc.props_only_in_left)
        self.assertIn(prop_name, diff_doc.props_only_in_right)

        FreeCAD.closeDocument(doc1.Name)
        FreeCAD.closeDocument(doc2.Name)

class TestDiffProperty(unittest.TestCase):
    def test_changed_property(self):
        """Test that changed properties are correctly identified.
        """
        doc1 = FreeCAD.newDocument("TestDoc1")
        doc1.addObject("Part::Box", "Box")

        doc2 = FreeCAD.newDocument("TestDoc2")
        objDoc2 = doc2.addObject("Part::Box", "Box")

        objDoc2.Length = 20

        diff_doc: DiffResultDocument = compute_diff(doc1, doc2)

        self.assertEqual(len(diff_doc.objs_only_in_left), 0)
        self.assertEqual(len(diff_doc.objs_only_in_right), 0)
        self.assertEqual(len(diff_doc._objs_in_both), 1)
        self.assertEqual(len(diff_doc.objs_different), 1)
        self.assertIn("Box", diff_doc.objs_different)

        diff_props = diff_doc.objs_different["Box"]

        self.assertEqual(len(diff_props.props_only_in_left), 0)
        self.assertEqual(len(diff_props.props_only_in_right), 0)
        self.assertEqual(len(diff_props.props_different), 3)
        self.assertIn("Length", diff_props.props_different)
        self.assertIn("Shape", diff_props.props_different)

        self.assertEqual(diff_props.props_different["Length"][0], 10)
        self.assertEqual(diff_props.props_different["Length"][1], 20)

        FreeCAD.closeDocument(doc1.Name)
        FreeCAD.closeDocument(doc2.Name)
