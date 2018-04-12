#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2016                                              *
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *
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

import FreeCAD as App
import FreeCADGui as Gui
from FreeCAD import Units
from PySide import QtGui, QtCore
from . import Tools
import WeightInstance as Instance
from shipUtils import Paths
import shipUtils.Units as USys
import shipUtils.Locale as Locale

class TaskPanel:
    def __init__(self):
        """Constructor"""
        self.ui = Paths.modulePath() + "/shipCreateWeight/TaskPanel.ui"

    def accept(self):
        """Create the ship instance"""
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.ship = self.widget(QtGui.QComboBox, "Ship")
        form.weight = self.widget(QtGui.QLineEdit, "Weight")

        ship = self.ships[form.ship.currentIndex()]
        density = Units.parseQuantity(Locale.fromString(form.weight.text()))

        Tools.createWeight(self.shapes, ship, density)
        return True

    def reject(self):
        """Cancel the job"""
        return True

    def clicked(self, index):
        pass

    def open(self):
        pass

    def needsFullSpace(self):
        return True

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return True

    def isAllowedAlterDocument(self):
        return False

    def helpRequested(self):
        pass

    def setupUi(self):
        """Create and configurate the user interface"""
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.ship = self.widget(QtGui.QComboBox, "Ship")
        form.weight = self.widget(QtGui.QLineEdit, "Weight")
        self.form = form
        if self.initValues():
            return True
        self.retranslateUi()

    def getMainWindow(self):
        toplevel = QtGui.QApplication.topLevelWidgets()
        for i in toplevel:
            if i.metaObject().className() == "Gui::MainWindow":
                return i
        raise Exception("No main window found")

    def widget(self, class_id, name):
        """Return the selected widget.

        Keyword arguments:
        class_id -- Class identifier
        name -- Name of the widget
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        return form.findChild(class_id, name)

    def initValues(self):
        """Setup the initial values"""
        # Ensure that there are at least one valid object to generate the
        # weight
        selObjs = Gui.Selection.getSelection()
        self.shapes = []
        if not selObjs:
            msg = QtGui.QApplication.translate(
                "ship_weight",
                "Weight objects can only be created on top of its geometry"
                " (no objects selected)",
                None)
            App.Console.PrintError(msg + '\n')
            return True
        for obj in selObjs:
            try:
                self.shapes.append(obj.Shape)
            except:
                continue
        if not len(self.shapes):
            msg = QtGui.QApplication.translate(
                "ship_weight",
                "No geometrical shapes found in the selected objects",
                None)
            App.Console.PrintError(msg + '\n')
            return True

        # Get the element type
        # 0 = unknown, 1 = vertex, 2 = line, 3 = face, 4 = solids
        self.elem_type = 0
        for shape in self.shapes:
            # Doing it in this way we are protected under strange entities,
            # and we are prepared to add higher level type of entities in the
            # future, just in case...
            try:
                if len(shape.Solids):
                    self.elem_type = max(4, self.elem_type)
            except:
                pass
            try:
                if len(shape.Faces):
                    self.elem_type = max(3, self.elem_type)
            except:
                pass
            try:
                if len(shape.Edges):
                    self.elem_type = max(2, self.elem_type)
            except:
                pass
            try:
                if len(shape.Vertexes):
                    self.elem_type = max(1, self.elem_type)
            except:
                pass
        # Could it happens???
        if self.elem_type == 0:
            msg = QtGui.QApplication.translate(
                "ship_weight",
                "Unknown object shapes selected",
                None)
            App.Console.PrintError(msg + '\n')
            return True

        # Ensure as well that exist at least one valid ship to create the
        # entity inside it
        self.ships = []
        for obj in App.ActiveDocument.Objects:
            try:
                if obj.IsShip:
                    self.ships.append(obj)
            except:
                continue
        if not len(self.ships):
            msg = QtGui.QApplication.translate(
                "ship_weight",
                "There are not ship objects to create weights into them",
                None)
            App.Console.PrintError(msg + '\n')
            return True

        # Fill the ships combo box
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.ship = self.widget(QtGui.QComboBox, "Ship")
        form.weight = self.widget(QtGui.QLineEdit, "Weight")
        icon = QtGui.QIcon(QtGui.QPixmap(":/icons/Ship_Instance.svg"))
        form.ship.clear()
        for ship in self.ships:
            form.ship.addItem(icon, ship.Label)
        form.ship.setCurrentIndex(0)

        # Initialize the 0 mass/density string field
        m_unit = USys.getMassUnits()
        l_unit = USys.getLengthUnits()
        if self.elem_type == 1:
            w_unit = m_unit
        elif self.elem_type == 2:
            w_unit = m_unit + '/' + l_unit
        elif self.elem_type == 3:
            w_unit = m_unit + '/' + l_unit + '^2'
        elif self.elem_type == 4:
            w_unit = m_unit + '/' + l_unit + '^3'
        form.weight.setText('0 ' + w_unit)
        return False

    def retranslateUi(self):
        """Set the user interface locale strings."""
        self.form.setWindowTitle(QtGui.QApplication.translate(
            "ship_weight",
            "Create a new weight",
            None))
        self.widget(QtGui.QLabel, "ShipLabel").setText(
            QtGui.QApplication.translate(
                "ship_weight",
                "Ship",
                None))
        if self.elem_type == 1:
            self.widget(QtGui.QLabel, "WeightLabel").setText(
                QtGui.QApplication.translate(
                    "ship_weight",
                    "Mass",
                    None))
        elif self.elem_type == 2:
            self.widget(QtGui.QLabel, "WeightLabel").setText(
                QtGui.QApplication.translate(
                    "ship_weight",
                    "Linear density",
                    None))
        elif self.elem_type == 3:
            self.widget(QtGui.QLabel, "WeightLabel").setText(
                QtGui.QApplication.translate(
                    "ship_weight",
                    "Area density",
                    None))
        elif self.elem_type == 4:
            self.widget(QtGui.QLabel, "WeightLabel").setText(
                QtGui.QApplication.translate(
                    "ship_weight",
                    "Density",
                    None))


def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
