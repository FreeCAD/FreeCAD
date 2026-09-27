# SPDX-License-Identifier: LGPL-2.1-or-later

"""Test that dragging in the model tree
does not trigger CarbonCopy multiple times.
"""

import FreeCAD
import Part
from PySide import QtCore, QtWidgets
from FreeCAD import Base

from SketcherTests.GuiTestCase import FreeCADGui, SketcherGuiTestCase

try:
    import SketcherGui
except ImportError:
    SketcherGui = None


class TestCarbonCopyModelTreeDragging(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("TestSketcherGuiCarbonCopyDragging")
        self.tree = SketcherGui.findModelTreeWidget()
        self.assertIsNotNone(self.tree, "Model TreeWidget not found")

    def findModelTreeItem(self, label):
        items = self.tree.findItems(label, QtCore.Qt.MatchExactly | QtCore.Qt.MatchRecursive, 0)
        self.assertEqual(
            len(items), 1, f"Expected to find one item with label '{label}', found {len(items)}"
        )
        return items[0]

    def sendMouseDragEvents(self, widget, pos):
        dragDistance = max(QtWidgets.QApplication.startDragDistance(), 1)
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseButtonPress,
            pos,
            QtCore.Qt.LeftButton,
            QtCore.Qt.LeftButton,
        )
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseMove,
            pos + QtCore.QPoint(0, dragDistance),
            QtCore.Qt.NoButton,
            QtCore.Qt.LeftButton,
        )
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseMove,
            pos,
            QtCore.Qt.NoButton,
            QtCore.Qt.LeftButton,
        )
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseMove,
            pos + QtCore.QPoint(0, -dragDistance),
            QtCore.Qt.NoButton,
            QtCore.Qt.LeftButton,
        )
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseMove,
            pos,
            QtCore.Qt.NoButton,
            QtCore.Qt.LeftButton,
        )
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseButtonRelease,
            pos,
            QtCore.Qt.LeftButton,
            QtCore.Qt.NoButton,
        )

    def test_CarbonCopyDragging(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        sketch1 = self.doc.addObject("Sketcher::SketchObject", "Sketch1")
        sketch2 = self.doc.addObject("Sketcher::SketchObject", "Sketch2")
        body.addObject(sketch1)
        body.addObject(sketch2)
        sketch1.AttachmentSupport = [(body.getObject("Origin"), "XY_Plane.")]
        sketch2.AttachmentSupport = [(body.getObject("Origin"), "XY_Plane.")]
        sketch1.MapMode = "FlatFace"
        sketch2.MapMode = "FlatFace"

        sketch1.addGeometry(Part.Circle(Base.Vector(-10, 10, 0), Base.Vector(0, 0, 1), 5), False)
        self.doc.recompute()
        self.flush_gui(50)

        FreeCADGui.ActiveDocument.setEdit(sketch2.Name)
        self.flush_gui(50)
        FreeCADGui.runCommand("Sketcher_CarbonCopy", 0)
        self.flush_gui(50)

        item1 = self.findModelTreeItem(sketch1.Label)
        self.tree.scrollToItem(item1)
        self.flush_gui(50)
        rect = self.tree.visualItemRect(item1)
        pos = rect.center()
        self.sendMouseDragEvents(self.tree.viewport(), pos)
        FreeCADGui.ActiveDocument.resetEdit()

        self.assertEqual(sketch2.GeometryCount, 1)
