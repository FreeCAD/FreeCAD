#***************************************************************************
#*   Copyright (c) 2020 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

"""View providers and UI elements for the Ifc classes."""

import FreeCAD
import ArchIFC

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from draftutils.translate import translate
else:
    def translate(ctxt,txt):
        return txt

class IfcContextView:
    """A default view provider for IfcContext objects."""

    def attach(self, vobj):
        self.Object = vobj.Object

    def setEdit(self, vobj, mode):
        if mode == 1 or mode == 2:
            return None

        FreeCADGui.Control.showDialog(IfcContextUI(vobj.Object))
        return True

    def unsetEdit(self, vobj, mode):
        if mode == 1 or mode == 2:
            return None

        FreeCADGui.Control.closeDialog()
        return True

    def setupContextMenu(self, vobj, menu):
        actionEdit = QtGui.QAction(translate("Arch", "Edit"),
                                   menu)
        QtCore.QObject.connect(actionEdit,
                               QtCore.SIGNAL("triggered()"),
                               self.edit)
        menu.addAction(actionEdit)

        # The default Part::FeaturePython context menu contains a `Set colors`
        # option. This option does not makes sense here. We therefore
        # override this menu and have to add our own `Transform` item.
        # To override the default menu this function must return `True`.
        actionTransform = QtGui.QAction(FreeCADGui.getIcon("Std_TransformManip.svg"),
                                        translate("Command", "Transform"), # Context `Command` instead of `Arch`.
                                        menu)
        QtCore.QObject.connect(actionTransform,
                               QtCore.SIGNAL("triggered()"),
                               self.transform)
        menu.addAction(actionTransform)

        return True

    def edit(self):
        FreeCADGui.ActiveDocument.setEdit(self.Object, 0)

    def transform(self):
        FreeCADGui.ActiveDocument.setEdit(self.Object, 1)

    def dumps(self):
        return None

    def loads(self,state):
        return None


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
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def reject(self):
        FreeCADGui.ActiveDocument.resetEdit()
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
            if lineEdit.objectName() in data:
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
