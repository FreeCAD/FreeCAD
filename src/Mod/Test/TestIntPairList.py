# SPDX-License-Identifier: LGPL-2.1-or-later

"""Run with FreeCADCmd -t TestIntPairList."""

import ctypes
import os
import tempfile
import unittest

import FreeCAD as App


class TestIntPairList(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestIntPairList")
        self.obj = self.doc.addObject("App::FeaturePython", "Pairs")
        self.obj.addProperty("App::PropertyIntPairList", "Values")

    def tearDown(self):
        if self.doc is not None:
            App.closeDocument(self.doc.Name)

    def test_round_trip(self):
        self.assertEqual(self.obj.Values, [])
        self.obj.Values = [(0, 2), [-3, 4], (0, 2)]
        self.assertEqual(self.obj.Values, [(0, 2), (-3, 4), (0, 2)])
        self.obj.Values = ((5, 6),)
        self.assertEqual(self.obj.Values, [(5, 6)])
        self.obj.Values = []
        self.assertEqual(self.obj.Values, [])

    def test_integer_limits(self):
        bits = ctypes.sizeof(ctypes.c_long) * 8
        lower = -(1 << (bits - 1))
        upper = (1 << (bits - 1)) - 1
        self.obj.Values = [(lower, upper)]
        self.assertEqual(self.obj.Values, [(lower, upper)])
        for value in (lower - 1, upper + 1):
            for pair in ((value, 0), (0, value)):
                with self.subTest(pair=pair):
                    with self.assertRaises(OverflowError):
                        self.obj.Values = [pair]
                    self.assertEqual(self.obj.Values, [(lower, upper)])

    def test_invalid_entries_do_not_change_the_list(self):
        self.obj.Values = [(1, 2)]
        for entry in ((), (1,), (1, 2, 3), (1.0, 2), (1, 2.5), ("1", 2), "12", None):
            with self.subTest(entry=entry):
                with self.assertRaises(TypeError):
                    self.obj.Values = [(3, 4), entry]
                self.assertEqual(self.obj.Values, [(1, 2)])

    def test_indexed_assignment(self):
        self.obj.Values = [(1, 2), (3, 4)]
        self.obj.Values = {1: (5, 6)}
        self.assertEqual(self.obj.Values, [(1, 2), (5, 6)])

    def test_changes_touch_the_object(self):
        self.doc.recompute()
        self.assertNotIn("Touched", self.obj.State)
        self.obj.Values = [(1, 2)]
        self.assertIn("Touched", self.obj.State)

    def test_copy_is_independent(self):
        self.obj.Values = [(1, 2)]
        copied = self.doc.copyObject(self.obj)
        self.assertEqual(copied.Values, [(1, 2)])
        copied.Values = [(3, 4)]
        self.assertEqual(self.obj.Values, [(1, 2)])

    def test_undo_redo(self):
        self.obj.Values = [(1, 2)]
        self.doc.UndoMode = 1
        self.doc.openTransaction("Change integer pairs")
        self.obj.Values = [(3, 4), (5, 6)]
        self.doc.commitTransaction()
        self.doc.undo()
        self.assertEqual(self.obj.Values, [(1, 2)])
        self.doc.redo()
        self.assertEqual(self.obj.Values, [(3, 4), (5, 6)])

    def test_save_reload(self):
        self.obj.Values = [(-1, 2), (3, -4)]
        self.obj.addProperty("App::PropertyIntPairList", "Empty")
        with tempfile.TemporaryDirectory() as directory:
            filename = os.path.join(directory, "Pairs.FCStd")
            self.doc.saveAs(filename)
            App.closeDocument(self.doc.Name)
            self.doc = None
            self.doc = App.openDocument(filename)
            try:
                obj = self.doc.getObject("Pairs")
                self.assertEqual(obj.getTypeIdOfProperty("Values"), "App::PropertyIntPairList")
                self.assertEqual(obj.Values, [(-1, 2), (3, -4)])
                self.assertEqual(obj.Empty, [])
            finally:
                App.closeDocument(self.doc.Name)
                self.doc = None
