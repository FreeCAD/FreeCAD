# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD
import FreeCADGui
from PySide import QtWidgets


class PartMirrorGuiTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartMirrorGuiTest")

    def tearDown(self):
        if FreeCADGui.Control.activeDialog():
            FreeCADGui.Control.closeDialog()
        FreeCADGui.Selection.clearSelection()
        FreeCAD.closeDocument(self.Doc.Name)

    def mirrorBoxWithLabel(self, label):
        if not FreeCAD.GuiUp:
            self.skipTest("This test requires a graphical user interface (GUI).")

        box = self.Doc.addObject("Part::Box", "Box")
        box.Label = label
        self.Doc.recompute()

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(self.Doc.Name, box.Name)
        FreeCADGui.runCommand("Part_Mirror")
        self.assertTrue(FreeCADGui.Control.activeDialog(), "Part Mirror task dialog did not open.")

        FreeCADGui.Control.activeTaskDialog().accept()
        QtWidgets.QApplication.processEvents()

        mirrors = [obj for obj in self.Doc.Objects if obj.isDerivedFrom("Part::Mirroring")]
        self.assertEqual(1, len(mirrors))
        return mirrors[0].Label

    def testMirrorLabelWithUnicodeIsNotDoubleEscaped(self):
        self.assertEqual("caf\u00e9 (Mirror #1)", self.mirrorBoxWithLabel("caf\u00e9"))

    def testMirrorLabelEscapesQuotesBeforePythonCommand(self):
        label = 'a");print("Erasing your hard drive, please stand by....")'
        self.assertEqual(f"{label} (Mirror #1)", self.mirrorBoxWithLabel(label))

    def testMirrorLabelWithNewlinesIsNotMangled(self):
        label = "a\nb\nc"
        self.assertEqual(f"{label} (Mirror #1)", self.mirrorBoxWithLabel(label))
