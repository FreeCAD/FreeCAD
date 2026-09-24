# SPDX-License-Identifier: LGPL-2.1-or-later

"""Run with FreeCADCmd -t TestPropertyFlags."""

import ctypes
import os
import tempfile
import unittest

import FreeCAD as App


class TestPropertyFlags(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestPropertyFlags")
        self.obj = self.doc.addObject("App::VarSet", "VarSet")
        self.obj.addProperty("App::PropertyBool", "Test")

    def tearDown(self):
        if self.doc is not None:
            App.closeDocument(self.doc.Name)

    def test(self):
        print("TestPropertyFlags running");
        self.assertEqual(self.obj.getPropertyStatus('Test'), ['PropDynamic'])
        self.obj.Test=False

        self.assertEqual(self.obj.getPropertyStatus('Test'), ['Touched', 'PropDynamic'])
        self.obj.setPropertyStatus('Test', ['-Touched'])
        self.assertEqual(self.obj.getPropertyStatus('Test'), ['PropDynamic'])

        self.obj.setPropertyStatus('Test', ['User3'])
        self.assertEqual(self.obj.getPropertyStatus('Test'), ['PropDynamic', 'User3'])

        self.obj.setPropertyStatus('Test', ['-User3'])
        self.assertEqual(self.obj.getPropertyStatus('Test'), ['PropDynamic'])

        self.obj.setPropertyStatus('Test', [31])
        self.assertEqual(self.obj.getPropertyStatus('Test'), ['PropDynamic', 'User3'])

        self.obj.setPropertyStatus('Test', [-31])
        self.assertEqual(self.obj.getPropertyStatus('Test'), ['PropDynamic'])

        with self.assertRaises(ValueError):
            self.obj.setPropertyStatus('Test', ['User4'])
            raise RuntimeError("Expected FreeCAD failure")

        self.assertEqual(self.obj.getPropertyStatus('Test'), ['PropDynamic'])

