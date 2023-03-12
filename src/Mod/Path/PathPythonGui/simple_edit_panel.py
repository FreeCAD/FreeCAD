
import FreeCAD
import FreeCADGui
from PySide import QtGui

translate = FreeCAD.Qt.translate

PROP_TYPE_QTYES = ["App::PropertyDistance", "App::PropertyAngle"]
PROP_TYPE_NUMERIC = PROP_TYPE_QTYES + ["App::PropertyPercent", "App:PropertyFloat"]


class SimpleEditPanel:
    """A simple property edit panel for a FreeCAD object

    Attributes:
        _transaction_name: Name to use for the undo transaction used in this panel.
        _ui_file: Path to the Qt UI file to use when creating this panel. Must be
            overridden by the subclass.
    """

    _transaction_name = "Property Edit"
    _ui_file = ""

    def __init__(self, obj, view):
        self.obj = obj
        self.viewProvider = view
        FreeCAD.ActiveDocument.openTransaction(self._transaction_name)
        self.form = FreeCADGui.PySideUic.loadUi(self._ui_file)
        self._fc = {}
        self.setupUi()

    def getFields(self):
        for prop_name, (get_field, set_field) in self._fc.items():
            setattr(self.obj, prop_name, get_field())

    def setFields(self):
        for prop_name, (get_field, set_field) in self._fc.items():
            set_field(getattr(self.obj, prop_name))

    def connectWidget(self, prop_name, widget, custom_lbls=None):
        """Connect a widget to a proxy object property

        This registers a connection between a UI widget and an object property,
        performing a series of convenience functions in the process, namely:
          * It copies the tooltip from the object property, with proper handling
            of translation.
          * If it is an enum property, it loads the translated choices from the
            property into the widget. Optionally the caller can override some or
            all of these labels using the `custom_lbls` dictionary.
          * It registers type conversion functions so when the form is applied or
            accepted the data is converted and stored/retrieved without manual
            intervention.
        """
        if custom_lbls is None:
            custom_lbls = {}
        prop_type = self.obj.getTypeIdOfProperty(prop_name)
        widget_type = type(widget).__name__
        if prop_type == "App::PropertyEnumeration" and widget_type == "QComboBox":
            enum = self.obj.getEnumerationsOfProperty(prop_name)
            # Populate the combo box with the enumeration elements, use the form context for translation
            elements = [
                translate(self.form.objectName(), custom_lbls.get(itm, itm))
                for itm in enum
            ]
            widget.clear()
            widget.addItems(elements)

            def _getter():
                return enum[widget.currentIndex()]

            def _setter(val):
                widget.setCurrentIndex(enum.index(val))

            self._fc[prop_name] = _getter, _setter
        elif prop_type == "App::PropertyBool" and widget_type == "QCheckBox":
            self._fc[prop_name] = widget.isChecked, widget.setChecked
        elif prop_type in PROP_TYPE_NUMERIC and widget_type == "QDoubleSpinBox":
            self._fc[prop_name] = widget.value, widget.setValue
        elif prop_type in PROP_TYPE_QTYES and widget_type == "QLineEdit":
            self._fc[prop_name] = widget.text, lambda v: widget.setText(str(v))
        else:
            raise ValueError(
                f"Unsupported connection between '{prop_type}' property and '{widget_type}' widget"
            )
        # Set the tooltip to the one corresponding to the property.
        widget.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty(prop_name))
        )

    def getStandardButtons(self):
        return int(
            QtGui.QDialogButtonBox.Ok
            | QtGui.QDialogButtonBox.Apply
            | QtGui.QDialogButtonBox.Cancel
        )

    def clicked(self, button):
        # callback for standard buttons
        if button == QtGui.QDialogButtonBox.Apply:
            self.updateModel()
            FreeCAD.ActiveDocument.recompute()
        if button == QtGui.QDialogButtonBox.Cancel:
            self.abort()

    def abort(self):
        FreeCAD.ActiveDocument.abortTransaction()
        self.cleanup(True)

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def accept(self):
        self.getFields()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def cleanup(self, gui):
        self.viewProvider.clearTaskPanel()
        if gui:
            FreeCADGui.Control.closeDialog()
            FreeCAD.ActiveDocument.recompute()

    def updateModel(self):
        self.getFields()
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def open(self):
        pass
