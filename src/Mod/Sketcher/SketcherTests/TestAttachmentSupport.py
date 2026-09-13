# test for FreeCAD issue https://github.com/FreeCAD/FreeCAD/issues/22341

import unittest
import FreeCAD as App
from PySide.QtGui import QApplication

# check if GUI is available
try:
    import FreeCADGui as Gui
    from PySide import QtCore, QtWidgets

    GUI_AVAILABLE = Gui.getMainWindow() is not None
except (ImportError, AttributeError):
    GUI_AVAILABLE = False


class CallableAcceptDialog:
    def __init__(self, test):
        self.test = test

    def __call__(self):
        dialog = QApplication.activeModalWidget()
        self.test.assertIsNotNone(dialog, "Dialog box could not be found")
        if dialog is not None:
            QtCore.QTimer.singleShot(0, dialog, QtCore.SLOT("accept()"))


class TestSketchAttachmentSupport(unittest.TestCase):
    """
    - Test that Sketch created on selected face of Link object will be supported on that Link object, instead pf original object.
    - Test that Sketch created on selected face of original object will be supported on original object.

    These tests require GUI and will be skipped with freecadcmd.
    """

    def _runCommand(self):
        QtCore.QTimer.singleShot(500, CallableAcceptDialog(self))

        # tested command -- start
        Gui.runCommand("Sketcher_NewSketch", 0)
        # tested command -- end
        Gui.ActiveDocument.resetEdit()

        self.sketchSupport = App.ActiveDocument.Sketch.AttachmentSupport

    def setUp(self):
        """
        Create a document.
        Add inside an object of type Part, including a Box object. Name it original object.
        Then create object of Link type pointed to Original Part object.
        Recompute deocument.
        """
        if not GUI_AVAILABLE:
            self.skipTest("GUI not available")

        self.doc = App.newDocument("TestSketchAttachmentSupport")

        self.originalPart = self.doc.addObject("App::Part", "OriginalPart")
        self.box = self.doc.addObject("Part::Box", "Box")
        self.originalPart.addObject(self.box)

        self.partWithSketch = self.doc.addObject("App::Part", "PartWithSketch")
        self.linkToPart = self.doc.addObject("App::Link", "LinkToPart")
        self.linkToPart.setLink(self.originalPart)
        self.partWithSketch.addObject(self.linkToPart)

        self.doc.recompute()
        Gui.Selection.clearSelection()

    def tearDown(self):
        if GUI_AVAILABLE and hasattr(self, "doc") and self.doc:
            App.closeDocument(self.doc.Name)

    @unittest.skipIf(not GUI_AVAILABLE, "GUI not available")
    def test_attachment_link_to_part(self):
        Gui.Selection.addSelection(self.doc.Name, self.linkToPart.Name, f"{self.box.Name}.Face6")

        self._runCommand()

        self.assertEqual(self.sketchSupport[0][0], self.linkToPart)  # compare support object
        self.assertEqual(self.sketchSupport[0][1][0], "Box.Face6")  # compare support face

    @unittest.skipIf(not GUI_AVAILABLE, "GUI not available")
    def test_attachment_original_part(self):
        Gui.Selection.addSelection(self.doc.Name, self.originalPart.Name, f"{self.box.Name}.Face6")

        self._runCommand()

        self.assertEqual(self.sketchSupport[0][0], self.box)  # compare support object
        self.assertEqual(self.sketchSupport[0][1][0], "Face6")  # compare support face
