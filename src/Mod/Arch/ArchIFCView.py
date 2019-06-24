import FreeCAD

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
        self.createBaseLayout()
        self.createMapConversionFormLayout()
        self.form = self.baseWidget

    def createBaseLayout(self):
        self.baseWidget = QtGui.QWidget()
        self.baseLayout = QtGui.QVBoxLayout(self.baseWidget)

    def createMapConversionFormLayout(self):
        self.baseLayout.addWidget(self.createLabel("Target Coordinate Reference System"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "Name"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "Description"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "Geodetic datum"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "Vertical datum"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "Map projection"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "Map zone"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "Map unit"))

        self.baseLayout.addWidget(self.createLabel("Map Conversion"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "Eastings"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "Northings"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "Orthogonal height"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "X axis abscissa"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "X axis ordinate"))
        self.baseLayout.addLayout(self.createFormEntry("foo", "Scale"))

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
        return lineEdit
