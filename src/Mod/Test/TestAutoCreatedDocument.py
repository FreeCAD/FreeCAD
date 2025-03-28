import unittest
import FreeCAD as App
import FreeCADGui as Gui
from unittest.mock import patch


class TestAutoCreatedDocument(unittest.TestCase):

    def setUp(self):
        """Set up a new document before each test."""
        self.doc = App.newDocument("TestDoc")

    def tearDown(self):
        """Clean up by closing all documents after each test."""
        for doc_name in App.listDocuments().keys():
            App.closeDocument(doc_name)

    def test_set_get_auto_created(self):
        """Test setting and getting the autoCreated flag."""
        self.doc.setAutoCreated(True)
        self.assertTrue(self.doc.isAutoCreated(), "autoCreated flag should be True")

        self.doc.setAutoCreated(False)
        self.assertFalse(self.doc.isAutoCreated(), "autoCreated flag should be False")

    def test_auto_created_on_startup(self):
        """Test if a document is marked as autoCreated when created on startup."""
        param_grp = App.ParamGet("User parameter:BaseApp/Preferences/Document")
        param_grp.SetBool("CreateNewDoc", True)
        with patch("FreeCADGui.runCommand") as mock_run_cmd:
            mock_run_cmd.return_value = None
            Gui.runCommand("Std_New")
            new_doc = App.activeDocument()
            new_doc.setAutoCreated(True)
            self.assertIsNotNone(new_doc, "No document was created")
            self.assertTrue(new_doc.isAutoCreated(), "New document should be marked as autoCreated")

    def test_auto_created_document_closes_on_opening_existing_document(self):
        """Test if an empty autoCreated document is closed when opening an existing document."""
        self.doc.setAutoCreated(True)
        self.assertEqual(len(self.doc.Objects), 0, "Document should have no objects")
        gui_doc = Gui.getDocument(self.doc)
        self.assertFalse(gui_doc.Modified, "Document should not be modified")
        saved_doc = App.newDocument("SavedDoc")
        file_path = "/tmp/SavedDoc.FCStd"
        saved_doc.saveAs(file_path)
        App.closeDocument("SavedDoc")
        App.setActiveDocument("TestDoc")
        App.openDocument(file_path)
        if self.doc.isAutoCreated() and len(self.doc.Objects) == 0:
            App.closeDocument("TestDoc")
        self.assertNotIn(
            "TestDoc",
            App.listDocuments(),
            "Auto-created empty document should be closed when opening another document",
        )

    def test_manual_document_does_not_close_on_opening_existing_document(self):
        """Test that a manually created document does not close when opening an existing document."""
        self.assertFalse(self.doc.isAutoCreated(), "autoCreated flag should be False")
        self.assertEqual(len(self.doc.Objects), 0, "Document should have no objects")
        saved_doc = App.newDocument("SavedDoc")
        file_path = "/tmp/SavedDoc.FCStd"
        saved_doc.saveAs(file_path)
        App.closeDocument("SavedDoc")
        App.setActiveDocument("TestDoc")
        App.openDocument(file_path)
        if self.doc.isAutoCreated() and len(self.doc.Objects) == 0:
            App.closeDocument("TestDoc")
        self.assertIn(
            "TestDoc",
            App.listDocuments(),
            "Manually created document should not close when opening an existing document",
        )
        self.assertIn(
            "SavedDoc",
            App.listDocuments(),
            "Opened document should be present after opening an existing document",
        )


if __name__ == "__main__":
    unittest.main()
