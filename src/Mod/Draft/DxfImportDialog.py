import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui

class DxfImportDialog:
    """
    A controller class that creates, manages, and shows the DXF import dialog.
    """
    def __init__(self, entity_counts, parent=None):
        # Step 1: Load the UI from the resource file. This returns a new QDialog instance.
        self.dialog = FreeCADGui.PySideUic.loadUi(":/ui/preferences-dxf-import.ui")

        # Now, all widgets like "label_Summary" are attributes of self.dialog

        self.entity_counts = entity_counts
        self.total_entities = sum(entity_counts.values())

        self.setup_ui()
        self.connect_signals()
        self.load_settings_and_set_initial_state()

    def setup_ui(self):
        """Perform initial UI setup."""
        self.dialog.label_Summary.setText(f"File contains approximately {self.total_entities} geometric entities.")
        self.dialog.label_Warning.hide()

    def connect_signals(self):
        """Connect signals from the dialog's widgets to our methods."""
        buttonBox = self.dialog.findChild(QtGui.QDialogButtonBox, "buttonBox")
        if buttonBox:
            # Connect to our custom slots INSTEAD of the dialog's built-in ones
            buttonBox.accepted.connect(self.on_accept)
            buttonBox.rejected.connect(self.on_reject)
            FreeCAD.Console.PrintLog("DxfImportDialog: OK and Cancel buttons connected.\n")
        else:
            FreeCAD.Console.PrintWarning("DxfImportDialog: Could not find buttonBox!\n")

        self.dialog.radio_ImportAs_Draft.toggled.connect(self.update_warning_label)
        self.dialog.radio_ImportAs_Primitives.toggled.connect(self.update_warning_label)
        self.dialog.radio_ImportAs_Shapes.toggled.connect(self.update_warning_label)
        self.dialog.radio_ImportAs_Fused.toggled.connect(self.update_warning_label)

    def on_accept(self):
        """Custom slot to debug the OK button click."""
        FreeCAD.Console.PrintLog("DxfImportDialog: 'OK' button clicked. Calling self.dialog.accept().\n")
        # Manually call the original slot
        self.dialog.accept()
        FreeCAD.Console.PrintLog("DxfImportDialog: self.dialog.accept() has been called.\n")

    def on_reject(self):
        """Custom slot to debug the Cancel button click."""
        FreeCAD.Console.PrintLog("DxfImportDialog: 'Cancel' button clicked. Calling self.dialog.reject().\n")
        # Manually call the original slot
        self.dialog.reject()
        FreeCAD.Console.PrintLog("DxfImportDialog: self.dialog.reject() has been called.\n")

    def load_settings_and_set_initial_state(self):
        """Load saved preferences and set the initial state of the dialog."""
        hGrp = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

        mode = hGrp.GetInt("DxfImportMode", 2)

        if mode == 0:
            self.dialog.radio_ImportAs_Draft.setChecked(True)
        elif mode == 1:
            self.dialog.radio_ImportAs_Primitives.setChecked(True)
        elif mode == 3:
            self.dialog.radio_ImportAs_Fused.setChecked(True)
        else:
            self.dialog.radio_ImportAs_Shapes.setChecked(True)

        is_legacy = hGrp.GetBool("dxfUseLegacyImporter", False)
        if is_legacy:
            self.dialog.radio_ImportAs_Primitives.setEnabled(False)
            self.dialog.radio_ImportAs_Draft.setEnabled(True)
            self.dialog.radio_ImportAs_Shapes.setEnabled(True)
            self.dialog.radio_ImportAs_Fused.setEnabled(True)
        else:
            self.dialog.radio_ImportAs_Draft.setEnabled(True)
            self.dialog.radio_ImportAs_Primitives.setEnabled(True)
            self.dialog.radio_ImportAs_Shapes.setEnabled(True)
            self.dialog.radio_ImportAs_Fused.setEnabled(True)

        self.update_warning_label()

    def update_warning_label(self):
        """Updates the warning label based on selection and entity count."""
        self.dialog.label_Warning.hide()
        current_mode = self.get_selected_mode()

        if self.total_entities > 5000 and (current_mode == 0 or current_mode == 1):
            self.dialog.label_Warning.setText("Warning: Importing over 5000 entities as editable objects can be very slow.")
            self.dialog.label_Warning.show()
        elif self.total_entities > 20000 and current_mode == 2:
            self.dialog.label_Warning.setText("Warning: Importing over 20,000 entities as individual shapes may be slow.")
            self.dialog.label_Warning.show()

    def exec_(self):
        FreeCAD.Console.PrintLog("DxfImportDialog: Calling self.dialog.exec_()...\n")
        result = self.dialog.exec_()
        FreeCAD.Console.PrintLog("DxfImportDialog: self.dialog.exec_() returned with result: {}\n".format(result))
        # QDialog.Accepted is usually 1, Rejected is 0.
        FreeCAD.Console.PrintLog("(Note: QDialog.Accepted = {}, QDialog.Rejected = {})\n".format(QtGui.QDialog.Accepted, QtGui.QDialog.Rejected))
        return result

    def get_selected_mode(self):
        """Return the integer value of the selected import mode."""
        if self.dialog.radio_ImportAs_Draft.isChecked(): return 0
        if self.dialog.radio_ImportAs_Primitives.isChecked(): return 1
        if self.dialog.radio_ImportAs_Fused.isChecked(): return 3
        if self.dialog.radio_ImportAs_Shapes.isChecked(): return 2
        return 2

    def get_show_dialog_again(self):
        """Return True if the dialog should be shown next time."""
        return not self.dialog.checkBox_ShowDialogAgain.isChecked()
