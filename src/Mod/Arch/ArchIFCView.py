import FreeCAD, ArchIFC

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui

class IfcContextView:
    def setEdit(self, viewObject, mode):
        # What does mode do?
        FreeCADGui.Control.showDialog(IfcContextUI(viewObject.Object))
        return True

class IfcContextUI:
    def __init__(self, object):
        self.object = object
        self.lineEditObjects = []
        self.createBaseLayout()
        self.createMapConversionFormLayout()
        self.prefillMapConversionForm()
        self.form = self.baseWidget

    def accept(self):
        data = {}
        for lineEdit in self.lineEditObjects:
            data[lineEdit.objectName()] = lineEdit.text()
        ArchIFC.IfcRoot.setObjIfcComplexAttributeValue(self, self.object, "RepresentationContexts", data)
        return True

    def createBaseLayout(self):
        self.baseWidget = QtGui.QWidget()
        self.baseLayout = QtGui.QVBoxLayout(self.baseWidget)

    def createMapConversionFormLayout(self):
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
        data = ArchIFC.IfcRoot.getObjIfcComplexAttribute(self, self.object, "RepresentationContexts")
        for lineEdit in self.lineEditObjects:
            if lineEdit.objectName() in data.keys():
                lineEdit.setText(data[lineEdit.objectName()])

    def createFormEntry(self, name, label):
        layout = QtGui.QHBoxLayout(self.baseWidget)
        layout.addWidget(self.createLabel(label))
        layout.addWidget(self.createLineEdit(name))
        return layout

    def createLabel(self, value):
        label = QtGui.QLabel(self.baseWidget)
        label.setText(QtGui.QApplication.translate("Arch", value, None))
        return label

    def createLineEdit(self, name):
        lineEdit = QtGui.QLineEdit(self.baseWidget)
        lineEdit.setObjectName(name)
        self.lineEditObjects.append(lineEdit)
        return lineEdit
