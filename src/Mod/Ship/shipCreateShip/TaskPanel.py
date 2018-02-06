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
import Preview
import Tools
import Instance
from shipUtils import Paths
import shipUtils.Units as USys
import shipUtils.Locale as Locale

class TaskPanel:
    def __init__(self):
        """Constructor"""
        self.ui = Paths.modulePath() + "/shipCreateShip/TaskPanel.ui"
        self.preview = Preview.Preview()

    def accept(self):
        """Create the ship instance"""
        self.preview.clean()
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.length = self.widget(QtGui.QLineEdit, "Length")
        form.breadth = self.widget(QtGui.QLineEdit, "Breadth")
        form.draft = self.widget(QtGui.QLineEdit, "Draft")

        Tools.createShip(self.solids,
                         Locale.fromString(form.length.text()),
                         Locale.fromString(form.breadth.text()),
                         Locale.fromString(form.draft.text()))
        return True

    def reject(self):
        """Cancel the job"""
        self.preview.clean()
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
        form.length = self.widget(QtGui.QLineEdit, "Length")
        form.breadth = self.widget(QtGui.QLineEdit, "Breadth")
        form.draft = self.widget(QtGui.QLineEdit, "Draft")
        form.mainLogo = self.widget(QtGui.QLabel, "MainLogo")
        form.mainLogo.setPixmap(QtGui.QPixmap(":/icons/Ship_Logo.svg"))
        self.form = form
        if self.initValues():
            return True
        self.retranslateUi()
        self.preview.update(self.L, self.B, self.T)
        QtCore.QObject.connect(
            form.length,
            QtCore.SIGNAL("valueChanged(double)"),
            self.onData)
        QtCore.QObject.connect(
            form.breadth,
            QtCore.SIGNAL("valueChanged(double)"),
            self.onData)
        QtCore.QObject.connect(
            form.draft,
            QtCore.SIGNAL("valueChanged(double)"),
            self.onData)

    def getMainWindow(self):
        toplevel = QtGui.qApp.topLevelWidgets()
        for i in toplevel:
            if i.metaObject().className() == "Gui::MainWindow":
                return i
        raise RuntimeError("No main window found")

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
        self.solids = None
        selObjs = Gui.Selection.getSelection()
        if not selObjs:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Ship objects can only be created on top of hull geometry"
                " (no objects selected)",
                None)
            App.Console.PrintError(msg + '\n')
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Please create or load a ship hull geometry before using"
                " this tool",
                None)
            App.Console.PrintError(msg + '\n')
            return True
        self.solids = []
        for i in range(0, len(selObjs)):
            solids = self.getSolids(selObjs[i])
            for j in range(0, len(solids)):
                self.solids.append(solids[j])
        if not self.solids:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Ship objects can only be created on top of hull geometry"
                " (no solid found at selected objects)",
                None)
            App.Console.PrintError(msg + '\n')
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Please create or load a ship hull geometry before using"
                " this tool",
                None)
            App.Console.PrintError(msg + '\n')
            return True
        # Get the ship bounds. The ship instance can not have dimensions
        # out of these values.
        self.bounds = [0.0, 0.0, 0.0]
        bbox = self.solids[0].BoundBox
        minX = bbox.XMin
        maxX = bbox.XMax
        minY = bbox.YMin
        maxY = bbox.YMax
        minZ = bbox.ZMin
        maxZ = bbox.ZMax
        for i in range(1, len(self.solids)):
            bbox = self.solids[i].BoundBox
            if minX > bbox.XMin:
                minX = bbox.XMin
            if maxX < bbox.XMax:
                maxX = bbox.XMax
            if minY > bbox.YMin:
                minY = bbox.YMin
            if maxY < bbox.YMax:
                maxY = bbox.YMax
            if minZ > bbox.ZMin:
                minZ = bbox.ZMin
            if maxZ < bbox.ZMax:
                maxZ = bbox.ZMax
        self.bounds[0] = maxX - minX
        self.bounds[1] = max(maxY - minY, abs(maxY), abs(minY))
        self.bounds[2] = maxZ - minZ

        input_format = USys.getLengthFormat()

        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.length = self.widget(QtGui.QLineEdit, "Length")
        form.breadth = self.widget(QtGui.QLineEdit, "Breadth")
        form.draft = self.widget(QtGui.QLineEdit, "Draft")

        qty = Units.Quantity(self.bounds[0], Units.Length)
        form.length.setText(Locale.toString(input_format.format(
            qty.getValueAs(USys.getLengthUnits()).Value)))
        self.L = self.bounds[0] / Units.Metre.Value
        qty = Units.Quantity(self.bounds[1], Units.Length)
        form.breadth.setText(Locale.toString(input_format.format(
            qty.getValueAs(USys.getLengthUnits()).Value)))
        self.B = self.bounds[1] / Units.Metre.Value
        qty = Units.Quantity(self.bounds[2], Units.Length)
        form.draft.setText(Locale.toString(input_format.format(
            0.5 * qty.getValueAs(USys.getLengthUnits()).Value)))
        self.T = 0.5 * self.bounds[2] / Units.Metre.Value
        return False

    def retranslateUi(self):
        """Set the user interface locale strings."""
        self.form.setWindowTitle(QtGui.QApplication.translate(
            "ship_create",
            "Create a new ship",
            None))
        self.widget(QtGui.QLabel, "LengthLabel").setText(
            QtGui.QApplication.translate(
                "ship_create",
                "Length",
                None))
        self.widget(QtGui.QLabel, "BreadthLabel").setText(
            QtGui.QApplication.translate(
                "ship_create",
                "Breadth",
                None))
        self.widget(QtGui.QLabel, "DraftLabel").setText(
            QtGui.QApplication.translate(
                "ship_create",
                "Draft",
                None))

    def clampVal(self, widget, val_min, val_max, val):
        if val >= val_min and val <= val_max:
            return val
        input_format = USys.getLengthFormat()
        val = min(val_max, max(val_min, val))
        qty = Units.Quantity('{} m'.format(val))
        widget.setText(Locale.toString(input_format.format(
            qty.getValueAs(USys.getLengthUnits()).Value)))
        return val

    def onData(self, value):
        """Updates the 3D preview on data changes.

        Keyword arguments:
        value -- Edited value. This parameter is required in order to use this
        method as a callback function, but it is not useful.
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.length = self.widget(QtGui.QLineEdit, "Length")
        form.breadth = self.widget(QtGui.QLineEdit, "Breadth")
        form.draft = self.widget(QtGui.QLineEdit, "Draft")

        qty = Units.Quantity(Locale.fromString(form.length.text()))
        val_min = 0.001
        val_max = self.bounds[0] / Units.Metre.Value
        val = qty.getValueAs('m').Value
        self.L = self.clampVal(form.length, val_min, val_max, val)
        qty = Units.Quantity(Locale.fromString(form.breadth.text()))
        val_min = 0.001
        val_max = self.bounds[1] / Units.Metre.Value
        val = qty.getValueAs('m').Value
        self.B = self.clampVal(form.breadth, val_min, val_max, val)
        qty = Units.Quantity(Locale.fromString(form.draft.text()))
        val_min = 0.001
        val_max = self.bounds[2] / Units.Metre.Value
        val = qty.getValueAs('m').Value
        self.T = self.clampVal(form.draft, val_min, val_max, val)
        self.preview.update(self.L, self.B, self.T)

    def getSolids(self, obj):
        """Returns the solid entities from an object
        Keyword arguments:
        obj -- FreeCAD object to extract the solids from.

        Returns:
        The solid entities, None if no solid have been found.
        """
        if not obj:
            return None
        if obj.isDerivedFrom('Part::Feature'):
            # get shape
            shape = obj.Shape
            if not shape:
                return None
            obj = shape
        if not obj.isDerivedFrom('Part::TopoShape'):
            return None
        # get face
        solids = obj.Solids
        if not solids:
            return None
        return solids


def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
