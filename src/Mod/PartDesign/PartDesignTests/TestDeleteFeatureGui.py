# SPDX-License-Identifier: LGPL-2.1-or-later
"""Std_Delete on a face picked in a body that is not active: the feature or the body."""

import unittest

import FreeCAD as App
import FreeCADGui as Gui
import PartDesign
import PartDesignGui


class TestDeleteFeatureGui(unittest.TestCase):
    def setUp(self):
        previous = Gui.activeWorkbench()
        self.previous_workbench = previous.name() if previous else None
        Gui.activateWorkbench("PartDesignWorkbench")
        self.doc = App.newDocument("DeleteFeatureGui")
        App.setActiveDocument(self.doc.Name)
        self.body = self.doc.addObject("PartDesign::Body", "Body")
        self.box = self.body.newObject("PartDesign::AdditiveBox", "Box")
        self.tip = self.body.newObject("PartDesign::AdditiveBox", "Box001")
        self.doc.recompute()

    def tearDown(self):
        try:
            Gui.Selection.clearSelection()
            App.closeDocument(self.doc.Name)
        finally:
            if self.previous_workbench:
                Gui.activateWorkbench(self.previous_workbench)

    def deletePickedFace(self):
        # What a click on a face of the body in the 3D view selects.
        Gui.ActiveDocument.ActiveView.setActiveObject("pdbody", None)
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(self.doc.Name, self.body.Name, self.tip.Name + ".Face1")
        Gui.runCommand("Std_Delete", 0)
        Gui.updateGui()

    def testOnlyBodyKeepsItsOtherFeatures(self):
        # https://github.com/AstoCAD/FreeCAD/issues/161
        self.deletePickedFace()
        self.assertIsNotNone(self.doc.getObject("Body"))
        self.assertIsNotNone(self.doc.getObject("Box"))
        self.assertIsNone(self.doc.getObject("Box001"))
        self.assertEqual(self.body.Tip, self.box)

    def testBodyAmongOthersIsDeletedWhole(self):
        # https://github.com/FreeCAD/FreeCAD/issues/27740
        self.doc.addObject("PartDesign::Body", "Body001")
        self.deletePickedFace()
        self.assertIsNone(self.doc.getObject("Body"))
        self.assertIsNone(self.doc.getObject("Box"))
        self.assertIsNone(self.doc.getObject("Box001"))
        self.assertIsNotNone(self.doc.getObject("Body001"))
