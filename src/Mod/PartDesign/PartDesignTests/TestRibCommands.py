# SPDX-License-Identifier: LGPL-2.1-or-later
"""Rib command and task-panel smoke test."""

import unittest

import FreeCADGui as Gui
from PySide import QtWidgets

from PartDesignTests import TestRib


class TestRibReferenceSelection(unittest.TestCase):
    def testCommandFromProfile(self):
        fixture = TestRib.TestRib()
        fixture.setUp()
        try:
            Gui.activateWorkbench("PartDesignWorkbench")
            fixture.makeRib("L", "Line")
            fixture.doc.removeObject(fixture.rib.Name)
            fixture.doc.recompute()
            Gui.activeDocument().activeView().setActiveObject("pdbody", fixture.body)
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(fixture.profile)
            Gui.runCommand("PartDesign_Rib")
            Gui.updateGui()
            rib = next(obj for obj in fixture.body.Group if obj.TypeId == "PartDesign::Rib")
            self.assertEqual(rib.Profile[0], fixture.profile)
            self.assertNotIn("Invalid", rib.State, rib.getStatusString())
            self.assertIsNotNone(Gui.Control.activeDialog())
            window = Gui.getMainWindow()
            self.assertIsNotNone(window.findChild(QtWidgets.QWidget, "ribParametersPanel"))
            self.assertIsNone(window.findChild(QtWidgets.QWidget, "ribAdvancedPanel"))
        finally:
            Gui.Selection.clearSelection()
            Gui.activeDocument().resetEdit()
            Gui.Control.closeDialog()
            fixture.tearDown()
