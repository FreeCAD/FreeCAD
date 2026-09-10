# SPDX-License-Identifier: LGPL-2.1-or-later
"""GUI smoke checks for the Rib template."""

import unittest

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtWidgets

from PartDesignTests.TestRib import TestRib


class TestRibReferenceSelection(unittest.TestCase):
    def setUp(self):
        Gui.activateWorkbench("PartDesignWorkbench")
        self.fixture = TestRib()
        self.fixture.setUp()
        self.doc = self.fixture.doc
        self.rib = self.fixture.rib
        self.doc.recompute()
        Gui.activeDocument().activeView().setActiveObject("pdbody", self.fixture.body)

    def tearDown(self):
        Gui.Selection.clearSelection()
        Gui.activeDocument().resetEdit()
        Gui.Control.closeDialog()
        self.fixture.tearDown()

    def testTemplatePanel(self):
        self.assertEqual(self.rib.ViewObject.TypeId, "PartDesignGui::ViewProviderRib")
        Gui.activeDocument().setEdit(self.rib.Name)
        panel = Gui.getMainWindow().findChild(QtWidgets.QWidget, "ribParametersPanel")
        self.assertIsNotNone(panel)
        self.assertEqual(
            panel.findChild(QtWidgets.QLineEdit, "profileName").text(),
            self.fixture.profile.Label,
        )
        self.assertIsNone(Gui.getMainWindow().findChild(QtWidgets.QWidget, "ribAdvancedParameters"))
        self.assertIsNone(panel.findChild(QtWidgets.QComboBox))
        self.assertIsNone(panel.findChild(QtWidgets.QDoubleSpinBox))

    def testCommandFromProfile(self):
        self.doc.removeObject(self.rib.Name)
        self.fixture.body.Tip = self.fixture.base
        self.doc.recompute()
        Gui.Selection.addSelection(self.fixture.profile, "Edge1")
        Gui.runCommand("PartDesign_Rib", 0)
        created = self.fixture.body.Tip
        self.assertEqual(created.TypeId, "PartDesign::Rib")
        self.assertEqual(created.Profile[0], self.fixture.profile)
        self.assertIn("not implemented", created.getStatusString())
        self.assertIsNotNone(
            Gui.getMainWindow().findChild(QtWidgets.QWidget, "ribParametersPanel")
        )
