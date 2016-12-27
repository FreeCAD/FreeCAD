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
import Units
from PySide import QtGui, QtCore
import Tools
import TankInstance as Instance
from shipUtils import Paths
import shipUtils.Units as USys

class TaskPanel:
    def __init__(self):
        """Constructor"""
        self.ui = Paths.modulePath() + "/shipCreateTank/TaskPanel.ui"

    def accept(self):
        """Create the ship instance"""
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.ship = self.widget(QtGui.QComboBox, "Ship")

        ship = self.ships[form.ship.currentIndex()]
        Tools.createTank(self.solids, ship)

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
        self.form = form
        if self.initValues():
            return True
        self.retranslateUi()

    def getMainWindow(self):
        toplevel = QtGui.qApp.topLevelWidgets()
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
        # tank
        selObjs = Gui.Selection.getSelection()
        self.solids = []
        if not selObjs:
            msg = QtGui.QApplication.translate(
                "ship_tank",
                "Tanks objects can only be created on top of its geometry"
                " (no objects selected)",
                None)
            App.Console.PrintError(msg + '\n')
            return True
        for obj in selObjs:
            try:
                self.solids.extend(obj.Shape.Solids)
            except:
                continue
        if not len(self.solids):
            msg = QtGui.QApplication.translate(
                "ship_tank",
                "No solids found in the selected objects",
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
                "ship_tank",
                "There are not ship objects to create weights into them",
                None)
            App.Console.PrintError(msg + '\n')
            return True

        # Fill the ships combo box
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.ship = self.widget(QtGui.QComboBox, "Ship")
        icon = QtGui.QIcon(QtGui.QPixmap(":/icons/Ship_Instance.svg"))
        form.ship.clear()
        for ship in self.ships:
            form.ship.addItem(icon, ship.Label)
        form.ship.setCurrentIndex(0)

        return False

    def retranslateUi(self):
        """Set the user interface locale strings."""
        self.form.setWindowTitle(QtGui.QApplication.translate(
            "ship_tank",
            "Create a new tank",
            None))
        self.widget(QtGui.QLabel, "ShipLabel").setText(
            QtGui.QApplication.translate(
                "ship_tank",
                "Ship",
                None))


def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
