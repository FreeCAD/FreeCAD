# test for FreeCAD issue https://github.com/FreeCAD/FreeCAD/issues/22341

import unittest
import FreeCAD as App

# check if GUI is available
try:
    import FreeCADGui as Gui
    from PySide import QtCore, QtWidgets

    GUI_AVAILABLE = Gui.getMainWindow() is not None
except (ImportError, AttributeError):
    GUI_AVAILABLE = False


class TestSketchAttachmentSupport(unittest.TestCase):
    """
    - Test that Sketch created on selected face of Link object will be supported on that Link object, instead pf original object.
    - Test that Sketch created on selected face of original object will be supported on original object.

    These tests require GUI and will be skipped with freecadcmd.
    """

    def _accept_sketch_attachment_dialog(self):
        """
        Attempts to find and accept the 'Sketch Attachment' dialog.

        WARNING: This implementation relies on internal GUI structure (widget hierarchy and window titles).
        It is fragile and may break if the FreeCAD GUI for sketch attachment changes.
        """
        for top_widget in QtWidgets.QApplication.topLevelWidgets():
            for widget in self._iterate_widgets(top_widget):
                if widget and isinstance(widget, QtWidgets.QInputDialog):
                    # Check if this is likely the attachment dialog.
                    if widget.windowTitle() == "Sketch Attachment":
                        buttons = widget.findChildren(QtWidgets.QPushButton)
                        for button in buttons:
                            if button.text() in ("OK", "&OK"):
                                button.click()
                                self._dialog_accepted = True
                                return
                        # If no OK button found, try to accept the dialog directly
                        widget.accept()
                        self._dialog_accepted = True
                        return

    def _iterate_widgets(self, widget):
        yield widget
        for child in widget.findChildren(QtWidgets.QWidget):
            yield from self._iterate_widgets(child)

    def _start_dialog_watcher(self):
        if self._dialog_timer is None:
            self._dialog_timer = QtCore.QTimer()
            self._dialog_timer.timeout.connect(self._accept_sketch_attachment_dialog)
            self._dialog_timer.start(1)  # Check every 1ms

    def _stop_dialog_watcher(self):
        if self._dialog_timer is not None:
            self._dialog_timer.stop()
            self._dialog_timer = None

    def _runCommand_with_watcher(self):
        """
        Start a timer to watch for and accept the Sketch Attachment dialog.

        Note: This is a workaround for GUI testing without a dedicated GUI automation framework (e.g., pytest-qt).
        It relies on polling top-level widgets.
        """
        self._start_dialog_watcher()
        QtWidgets.QApplication.processEvents()

        # tested command -- start
        Gui.runCommand("Sketcher_NewSketch", 0)
        # tested command -- end
        Gui.ActiveDocument.resetEdit()

        self._stop_dialog_watcher()

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

        self._dialog_timer = None
        self._dialog_accepted = False

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
        """clean up the test document"""
        # Stop dialog watcher if still running
        self._stop_dialog_watcher()

        if GUI_AVAILABLE and hasattr(self, "doc") and self.doc:
            App.closeDocument(self.doc.Name)

    @unittest.skipIf(not GUI_AVAILABLE, "GUI not available")
    def test_attachment_link_to_part(self):
        Gui.Selection.addSelection(self.doc.Name, self.linkToPart.Name, f"{self.box.Name}.Face6")

        self._runCommand_with_watcher()

        self.assertTrue(self._dialog_accepted)
        self.assertEqual(self.sketchSupport[0][0], self.linkToPart)  # compare support object
        self.assertEqual(self.sketchSupport[0][1][0], "Box.Face6")  # compare support face

    @unittest.skipIf(not GUI_AVAILABLE, "GUI not available")
    def test_attachment_original_part(self):
        Gui.Selection.addSelection(self.doc.Name, self.originalPart.Name, f"{self.box.Name}.Face6")

        self._runCommand_with_watcher()

        self.assertTrue(self._dialog_accepted)
        self.assertEqual(self.sketchSupport[0][0], self.box)  # compare support object
        self.assertEqual(self.sketchSupport[0][1][0], "Face6")  # compare support face
