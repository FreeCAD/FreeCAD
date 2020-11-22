"""View providers and UI elements for the Ifc classes."""

import FreeCAD, ArchIFC

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui

class IfcContextView:
    """A default view provider for IfcContext objects."""

    def setEdit(self, viewObject, mode):
        """Method called when the document requests the object to enter edit mode.

        Opens the IfcContextUi as the task panel.

        Edit mode is entered when a user double clicks on an object in the tree
        view, or when they use the menu option [Edit -> Toggle Edit Mode].

        Parameters
        ----------
        mode: int or str
            The edit mode the document has requested. Set to 0 when requested via
            a double click or [Edit -> Toggle Edit Mode].

        Returns
        -------
        bool
            If edit mode was entered.
        """

        # What does mode do?
        FreeCADGui.Control.showDialog(IfcContextUI(viewObject.Object))
        return True

class IfcContextUI:
    """A default task panel for editing context objects."""

    def __init__(self, object):
        self.object = object
        self.lineEditObjects = []
        self.createBaseLayout()
        self.createMapConversionFormLayout()
        self.prefillMapConversionForm()
        self.form = self.baseWidget

    def accept(self):
        """This method runs as a callback when the user selects the ok button.

        It writes the data entered into the forms to the object's IfcData
        property.
        """
        data = {}
        for lineEdit in self.lineEditObjects:
            data[lineEdit.objectName()] = lineEdit.text()
        ArchIFC.IfcRoot.setObjIfcComplexAttributeValue(self, self.object, "RepresentationContexts", data)
        return True

    def createBaseLayout(self):
        """Defines the basic layout of the task panel."""

        self.baseWidget = QtGui.QWidget()
        self.baseLayout = QtGui.QVBoxLayout(self.baseWidget)

    def createMapConversionFormLayout(self):
        """Creates form entries for the data being edited.

        Creates form entries for each of the data points being edited within
        the IFC complex attribute, RepresentationContexts.
        """

        self.baseLayout.addWidget(self.createLabel("Target Coordinate Reference System"))
        self.baseLayout.addLayout(self.createFormEntry("name", "Name"))
        self.baseLayout.addLayout(self.createFormEntry("description", "Description"))
        self.baseLayout.addLayout(self.createFormEntry("geodetic_datum", "Geodetic datum"))
        self.baseLayout.addLayout(self.createFormEntry("vertical_datum", "Vertical datum"))
        self.baseLayout.addLayout(self.createFormEntry("map_projection", "Map projection"))
        self.baseLayout.addLayout(self.createFormEntry("map_zone", "Map zone"))
        self.baseLayout.addLayout(self.createFormEntry("map_unit", "Map unit"))

        self.baseLayout.addWidget(self.createLabel("Map Conversion"))
        self.baseLayout.addLayout(self.createFormEntry("eastings", "Eastings"))
        self.baseLayout.addLayout(self.createFormEntry("northings", "Northings"))
        self.baseLayout.addLayout(self.createFormEntry("orthogonal_height", "Orthogonal height"))
        self.baseLayout.addLayout(self.createFormEntry("true_north", "True north (anti-clockwise from +Y)"))
        self.baseLayout.addLayout(self.createFormEntry("scale", "Scale"))

    def prefillMapConversionForm(self):
        """Prefills each of the form entries with the existing value.

        Gets the existing value from the object's IfcData, specifically the complex
        attribute, RepresentationContexts.
        """
        data = ArchIFC.IfcRoot.getObjIfcComplexAttribute(self, self.object, "RepresentationContexts")
        for lineEdit in self.lineEditObjects:
            if lineEdit.objectName() in data.keys():
                lineEdit.setText(data[lineEdit.objectName()])

    def createFormEntry(self, name, label):
        """Creates a form entry.

        The name corresponds to the data point being edited in the
        RepresentationContexts complex attribute. The label is a human readable
        version of the name.

        Parameters
        ----------
        name: str
            The name of the datapoint within the RepresentationContexts
            attribute being edited.
        label: str
            A human readable version of the name.

        Returns
        -------
        <PySide2.QtWidgets.QWidget>
            Widget containing the label and form.
        """

        layout = QtGui.QHBoxLayout(self.baseWidget)
        layout.addWidget(self.createLabel(label))
        layout.addWidget(self.createLineEdit(name))
        return layout

    def createLabel(self, value):
        """Creates a translated label.

        Parameters
        ----------
        value: str
            The human readable label text.

        Returns
        -------
        <PySide2.QtWidgets.QWidget>
            The label Qt widget.
        """

        label = QtGui.QLabel(self.baseWidget)
        label.setText(QtGui.QApplication.translate("Arch", value, None))
        return label

    def createLineEdit(self, name):
        """Creates a form with the name specified.

        Parameters
        ----------
        name: str
            The name of the datapoint within the RepresentationContexts
            attribute being edited.

        Returns
        -------
        <PySide2.QtWidgets.QWidget>
            The form Qt widget.
        """

        lineEdit = QtGui.QLineEdit(self.baseWidget)
        lineEdit.setObjectName(name)
        self.lineEditObjects.append(lineEdit)
        return lineEdit
