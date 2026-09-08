# SPDX-License-Identifier: LGPL-2.1-or-later

import os

import FreeCAD as App
import Part
import Sketcher
from PySide import QtCore, QtGui
from SketcherTests.GuiTestCase import FreeCADGui as Gui, SketcherGuiTestCase


class TestSketchGroupsGui(SketcherGuiTestCase):
    def testNestedTextGroup(self):
        font = os.path.join(App.getResourceDir(), "examples", "osifont-lgpl3fe.ttf")
        if not os.path.isfile(font):
            self.skipTest("Bundled osifont not found")
        Gui.activateWorkbench("SketcherWorkbench")
        self.doc = App.newDocument("NestedTextGroupTest")
        s = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        for x, text in [(0, "A"), (30, "B")]:
            handle = s.addGeometry(
                Part.LineSegment(App.Vector(x, 0, 0), App.Vector(x, 10, 0)), True
            )
            c = s.addConstraint(Sketcher.Constraint("Text", [handle, 0], text, font, True))
            s.setTextAndFont(c, text, font, True, False)
        self.doc.recompute()
        Gui.activeDocument().setEdit(s.Name)

        def select(ids):
            Gui.Selection.clearSelection()
            for geo in ids:
                Gui.Selection.addSelection(self.doc.Name, s.Name, "Edge{}".format(geo + 1))

        before = s.GeometryCount
        select(range(before))
        Gui.runCommand("Sketcher_ConstrainGroup", 0)
        self.assertEqual(s.solve(), 0)
        self.assertEqual((s.GeometryCount, s.ConstraintCount), (before + 1, 3))

        circle = s.addGeometry(Part.Circle(App.Vector(60, 5, 0), App.Vector(0, 0, 1), 2))
        select([0, circle])  # Selecting a text member must select its outer group.
        Gui.runCommand("Sketcher_ConstrainGroup", 0)
        root = s.GeometryCount - 1
        s.addConstraint(Sketcher.Constraint("Distance", root, 20.0))
        self.assertEqual(s.solve(), 0)
        self.assertAlmostEqual(s.Geometry[circle].Radius, 4)

        # Regeneration changes geometry indices and the order of the text constraints.
        s.setTextAndFont(0, "ABC", font, True, False)
        self.assertEqual(s.solve(), 0)
        self.assertEqual(sum(c.Type == "Text" for c in s.Constraints), 2)
        groups = [c for c in s.Constraints if c.Type == "Group"]
        self.assertEqual(len(groups), 2)
        root = groups[-1].First
        counts = (s.GeometryCount, s.ConstraintCount)

        clipboard = QtGui.QApplication.clipboard()
        original = clipboard.mimeData()
        saved = {fmt: original.data(fmt) for fmt in original.formats()} if original else {}
        try:
            select([root])
            Gui.runCommand("Sketcher_CopyClipboard", 0)
            Gui.runCommand("Sketcher_Paste", 0)
            self.assertEqual(s.solve(), 0)
            self.assertEqual((s.GeometryCount, s.ConstraintCount), tuple(2 * n for n in counts))
            self.doc.undo()
            self.assertEqual((s.GeometryCount, s.ConstraintCount), counts)
        finally:
            restored = QtCore.QMimeData()
            for fmt, data in saved.items():
                restored.setData(fmt, data)
            clipboard.setMimeData(restored)

        select([root])
        Gui.runCommand("Std_Delete", 0)
        self.assertEqual((s.GeometryCount, s.ConstraintCount), (0, 0))
        self.doc.undo()
        self.assertEqual(s.solve(), 0)
        self.assertEqual((s.GeometryCount, s.ConstraintCount), counts)
