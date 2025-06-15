import unittest
import FreeCAD
import sys
from PySide2 import QtWidgets, QtCore

try:
    from PySide2.QtTest import QTest

    GUI_AVAILABLE = True
except ImportError:
    GUI_AVAILABLE = False

# Import the dialog only if running in GUI mode
if GUI_AVAILABLE:
    from freecad.gui.Dialogs import DlgAddPropertyToObjects


@unittest.skipUnless(GUI_AVAILABLE, "GUI tests require PySide2 and graphical environment")
class TestPropertyAdditionGUI(unittest.TestCase):
    """GUI-dependent tests"""

    @classmethod
    def setUpClass(cls):
        cls.doc = FreeCAD.newDocument("TestPropertyAdditionGUI")
        cls.obj1 = cls.doc.addObject("App::FeaturePython", "TestObjectGUI1")
        cls.obj2 = cls.doc.addObject("App::FeaturePython", "TestObjectGUI2")
        cls.testObjects = [cls.obj1, cls.obj2]

        cls.obj1.addProperty("App::PropertyString", "ExistingPropGUI1", "TestGroupGUI1")
        cls.app = QtWidgets.QApplication.instance() or QtWidgets.QApplication(sys.argv)

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        if hasattr(cls, "dialog"):
            cls.dialog.close()

    def setUp(self):
        self.dialog = DlgAddPropertyToObjects(None, self.testObjects)
        self.dialog.show()
        QTest.qWaitForWindowExposed(self.dialog)

    def tearDown(self):
        self.dialog.close()

    def test_dialog_initialization(self):
        """Test dialog initializes correctly"""
        self.assertTrue(self.dialog.isVisible())
        self.assertEqual(self.dialog.windowTitle(), "Add Property to Selected Objects")

        # Verify type combo box is populated
        self.assertGreater(
            self.dialog.ui.comboBoxType.count(), 10
        )  # Should have many property types

        # Verify group combo box has expected items
        groups = [
            self.dialog.comboBoxGroup.itemText(i) for i in range(self.dialog.comboBoxGroup.count())
        ]
        self.assertIn("TestGroupGUI1", groups)
        self.assertIn("Base", groups)

    def test_property_addition_gui(self):
        """Test property addition through GUI"""
        prop_name = "TestPropertyGUI"
        prop_type = "App::PropertyString"

        # Set values in dialog
        self.dialog.ui.lineEditName.setText(prop_name)
        self.dialog.ui.comboBoxType.setCurrentText(prop_type)

        # Accept dialog
        self.dialog.accept()

        # Verify property was added
        self.assertTrue(hasattr(self.obj1, prop_name))
        self.assertTrue(hasattr(self.obj2, prop_name))

        # Verify property type
        self.assertEqual(self.obj1.getTypeIdOfProperty(prop_name), prop_type)

    def test_name_validation_gui(self):
        """Test name validation in GUI"""
        test_cases = [
            ("ValidName", True),
            ("invalid name", False),
            ("1Invalid", False),
            ("", False),
        ]

        for name, should_enable in test_cases:
            with self.subTest(name=name):
                self.dialog.ui.lineEditName.setText(name)
                QTest.qWait(100)  # Process events
                ok_button = self.dialog.ui.buttonBox.button(QtWidgets.QDialogButtonBox.Ok)
                self.assertEqual(ok_button.isEnabled(), should_enable)

    def test_undo_redo(self):
        """Test undo/redo operations are per property addition with proper dialog state"""
        # Store initial undo count
        initial_undo_count = self.doc.undoCount()

        # First addition - with "Add another" checked
        self.dialog.ui.checkBoxAdd.setChecked(True)  # Keep dialog open
        self.dialog.ui.lineEditName.setText("FirstProperty")
        self.dialog.ui.comboBoxType.setCurrentText("App::PropertyString")
        self.dialog.accept()

        # Verify dialog remains open
        self.assertTrue(self.dialog.isVisible())
        self.assertEqual(self.dialog.ui.lineEditName.text(), "")  # Fields cleared

        # Second addition - with "Add another" unchecked
        self.dialog.ui.checkBoxAdd.setChecked(False)  # Close after this one
        self.dialog.ui.lineEditName.setText("SecondProperty")
        self.dialog.ui.comboBoxType.setCurrentText("App::PropertyFloat")
        self.dialog.accept()

        # Verify dialog closed
        self.assertFalse(self.dialog.isVisible())

        # Verify two undo steps were created
        self.assertEqual(self.doc.undoCount(), initial_undo_count + 2)

        # Test undo for second property
        self.doc.undo()
        self.assertTrue(hasattr(self.obj1, "FirstProperty"))
        self.assertFalse(hasattr(self.obj1, "SecondProperty"))

        # Test undo for first property
        self.doc.undo()
        self.assertFalse(hasattr(self.obj1, "FirstProperty"))
        self.assertFalse(hasattr(self.obj1, "SecondProperty"))

        # Test redo for first property
        self.doc.redo()
        self.assertTrue(hasattr(self.obj1, "FirstProperty"))
        self.assertFalse(hasattr(self.obj1, "SecondProperty"))

        # Test redo for second property
        self.doc.redo()
        self.assertTrue(hasattr(self.obj1, "FirstProperty"))
        self.assertTrue(hasattr(self.obj1, "SecondProperty"))

    def test_partial_property_addition(self):
        """Test behavior when property addition fails on some objects"""
        # Make obj2 fail property addition
        with unittest.mock.patch.object(
            self.obj2, "addDynamicProperty", side_effect=RuntimeError("Test error")
        ):
            self.dialog.ui.lineEditName.setText("PartialProperty")
            self.dialog.ui.comboBoxType.setCurrentText("App::PropertyBool")

            # Catch the expected question dialog
            with unittest.mock.patch.object(
                QtWidgets.QMessageBox, "question", return_value=QtWidgets.QMessageBox.Yes
            ):
                self.dialog.accept()

            # Verify partial success
            self.assertTrue(hasattr(self.obj1, "PartialProperty"))
            self.assertFalse(hasattr(self.obj2, "PartialProperty"))

    def test_group_initialization(self):
        """Test group combo box is properly initialized"""
        # Verify Base group exists
        self.assertIn(
            "Base",
            [
                self.dialog.comboBoxGroup.itemText(i)
                for i in range(self.dialog.comboBoxGroup.count())
            ],
        )

        # Test with objects that have different groups
        self.obj1.addProperty("App::PropertyString", "GroupTestProp", "CustomGroup")
        self.dialog = DlgAddPropertyToObjects(None, self.testObjects)
        groups = [
            self.dialog.comboBoxGroup.itemText(i) for i in range(self.dialog.comboBoxGroup.count())
        ]
        self.assertIn("CustomGroup", groups)
        self.assertEqual(groups[-1], "Base")  # Base should be last

    def test_editor_creation(self):
        """Test appropriate editors are created"""
        test_cases = [
            ("App::PropertyBool", QtWidgets.QCheckBox),
            ("App::PropertyInteger", QtWidgets.QSpinBox),
            ("App::PropertyFloat", QtWidgets.QDoubleSpinBox),
            ("App::PropertyString", QtWidgets.QLineEdit),
        ]

        for prop_type, editor_type in test_cases:
            with self.subTest(prop_type=prop_type):
                self.dialog.ui.comboBoxType.setCurrentText(prop_type)
                QTest.qWait(100)  # Allow editor creation
                if self.dialog.editor:
                    self.assertIsInstance(self.dialog.editor, editor_type)


def run_tests():
    """Run all GUI tests"""
    if not GUI_AVAILABLE:
        print("GUI tests skipped - graphical environment required")
        return unittest.TestResult()

    suite = unittest.TestLoader().loadTestsFromTestCase(TestPropertyAdditionGUI)
    runner = unittest.TextTestRunner(verbosity=2)
    return runner.run(suite)


if __name__ == "__main__":
    result = run_tests()
    if not result.wasSuccessful():
        sys.exit(1)
