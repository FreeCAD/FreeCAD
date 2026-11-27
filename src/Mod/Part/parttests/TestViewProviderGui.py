# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD


class PartGuiViewProviderTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartGuiTest")

    def testCanDropObject(self):
        # https://github.com/FreeCAD/FreeCAD/pull/6850
        box = self.Doc.addObject("Part::Box", "Box")
        with self.assertRaises(TypeError):
            box.ViewObject.canDragObject(0)
        with self.assertRaises(TypeError):
            box.ViewObject.canDropObject(0)
        box.ViewObject.canDropObject()
        with self.assertRaises(TypeError):
            box.ViewObject.dropObject(box, 0)

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument("PartGuiTest")
