# SPDX-License-Identifier: LGPL-2.1-or-later
"""Pasted sketch geometry must be selected so it can be moved as a set (#32362)."""

import FreeCAD as App
import Part

from SketcherTests.GuiTestCase import FreeCADGui as Gui, SketcherGuiTestCase


class TestPasteSelectionGui(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()
        Gui.activateWorkbench("SketcherWorkbench")
        self.doc = App.newDocument("PasteSelectionTest")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.sketch.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0)))
        self.sketch.addGeometry(Part.Point(App.Vector(5, 5, 0)))
        self.sketch.addGeometry(Part.Circle(App.Vector(20, 0, 0), App.Vector(0, 0, 1), 3))
        self.doc.recompute()
        self.assertTrue(Gui.getDocument(self.doc.Name).setEdit(self.sketch.Name))
        self.flush_gui(100)

    def vertexName(self, geoId):
        for vertex in range(32):
            if tuple(self.sketch.getGeoVertexIndex(vertex)) == (geoId, 1):
                return "Vertex{}".format(vertex + 1)
        self.fail("No vertex for point {}".format(geoId))

    def testPastedGeometryIsSelected(self):
        Gui.Selection.clearSelection()
        for name in ("Edge1", self.vertexName(1), "Edge3"):
            Gui.Selection.addSelection(self.doc.Name, self.sketch.Name, name)
        Gui.runCommand("Sketcher_CopyClipboard", 0)
        Gui.runCommand("Sketcher_Paste", 0)
        self.flush_gui(50)

        self.assertEqual(self.sketch.GeometryCount, 6)
        expected = []
        for geoId in range(3, 6):
            if isinstance(self.sketch.Geometry[geoId], Part.Point):
                expected.append(self.vertexName(geoId))
            else:
                expected.append("Edge{}".format(geoId + 1))
        selection = Gui.Selection.getSelectionEx(self.doc.Name)
        selected = selection[0].SubElementNames if selection else []
        self.assertEqual(sorted(selected), sorted(expected))
