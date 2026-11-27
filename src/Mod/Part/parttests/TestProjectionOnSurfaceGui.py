# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD
import FreeCADGui
import Part
import Sketcher
from PySide import QtWidgets


class ProjectionOnSurfaceTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("ProjectionOnSurface")

    def testSketchInternalFaceAsSupportFace(self):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        sketch.MakeInternals = True
        sketch.addGeometry(
            [
                Part.LineSegment(FreeCAD.Vector(0, 0), FreeCAD.Vector(10, 0)),
                Part.LineSegment(FreeCAD.Vector(10, 0), FreeCAD.Vector(10, 10)),
                Part.LineSegment(FreeCAD.Vector(10, 10), FreeCAD.Vector(0, 10)),
                Part.LineSegment(FreeCAD.Vector(0, 10), FreeCAD.Vector(0, 0)),
            ],
            False,
        )
        self.Doc.recompute()

        FreeCADGui.activateWorkbench("PartWorkbench")
        FreeCADGui.updateGui()
        FreeCADGui.runCommand("Part_ProjectionOnSurface")
        FreeCADGui.updateGui()

        taskDialog = FreeCADGui.Control.activeTaskDialog()
        self.assertIsNotNone(taskDialog)
        supportButton = None
        for widget in taskDialog.getDialogContent():
            supportButton = widget.findChild(QtWidgets.QPushButton, "pushButtonAddProjFace")
            if supportButton:
                break
        self.assertIsNotNone(supportButton)
        supportButton.click()
        FreeCADGui.Selection.addSelection(sketch, "InternalFace1")

        projection = self.Doc.getObject("Projection")
        self.assertIsNotNone(projection)
        self.assertEqual(projection.SupportFace[0], sketch)
        self.assertEqual(projection.SupportFace[1], ["InternalFace1"])

    def tearDown(self):
        FreeCADGui.Selection.clearSelection()
        guiDocument = FreeCADGui.getDocument("ProjectionOnSurface")
        if FreeCADGui.Control.activeDialog(guiDocument):
            FreeCADGui.Control.closeDialog(guiDocument)
        FreeCAD.closeDocument("ProjectionOnSurface")
