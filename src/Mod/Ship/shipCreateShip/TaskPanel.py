#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *
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
import Instance
from shipUtils import Paths


class TaskPanel:
    def __init__(self):
        """Constructor"""
        self.ui = Paths.modulePath() + "/shipCreateShip/TaskPanel.ui"
        self.preview = Preview.Preview()

    def accept(self):
        """Create the ship instance"""
        self.preview.clean()
        obj = App.ActiveDocument.addObject("Part::FeaturePython", "Ship")
        ship = Instance.Ship(obj, self.solids)
        Instance.ViewProviderShip(obj.ViewObject)
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.length = self.widget(QtGui.QDoubleSpinBox, "Length")
        form.breadth = self.widget(QtGui.QDoubleSpinBox, "Breadth")
        form.draft = self.widget(QtGui.QDoubleSpinBox, "Draft")
        obj.Length = form.length.value()
        obj.Breadth = form.breadth.value()
        obj.Draft = form.draft.value()
        App.ActiveDocument.recompute()
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
        form.length = self.widget(QtGui.QDoubleSpinBox, "Length")
        form.breadth = self.widget(QtGui.QDoubleSpinBox, "Breadth")
        form.draft = self.widget(QtGui.QDoubleSpinBox, "Draft")
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
        self.solids = None
        selObjs = Gui.Selection.getSelection()
        if not selObjs:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Ship objects can only be created on top of hull geometry"
                " (no objects selected)",
                None,
                QtGui.QApplication.UnicodeUTF8)
            App.Console.PrintError(msg + '\n')
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Please create or load a ship hull geometry before using"
                " this tool",
                None,
                QtGui.QApplication.UnicodeUTF8)
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
                None,
                QtGui.QApplication.UnicodeUTF8)
            App.Console.PrintError(msg + '\n')
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Please create or load a ship hull geometry before using"
                " this tool",
                None,
                QtGui.QApplication.UnicodeUTF8)
            App.Console.PrintError(msg + '\n')
            return True
        # Get the ship bounds. The ship instance can not have dimensions
        # out of these values.
        bounds = [0.0, 0.0, 0.0]
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
        bounds[0] = maxX - minX
        bounds[1] = max(maxY - minY, abs(maxY), abs(minY))
        bounds[2] = maxZ - minZ

        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.length = self.widget(QtGui.QDoubleSpinBox, "Length")
        form.breadth = self.widget(QtGui.QDoubleSpinBox, "Breadth")
        form.draft = self.widget(QtGui.QDoubleSpinBox, "Draft")

        form.length.setMaximum(bounds[0] / Units.Metre.Value)
        form.length.setMinimum(0.001)
        form.length.setValue(bounds[0] / Units.Metre.Value)
        self.L = bounds[0] / Units.Metre.Value
        form.breadth.setMaximum(bounds[1] / Units.Metre.Value)
        form.breadth.setMinimum(0.001)
        form.breadth.setValue(bounds[1] / Units.Metre.Value)
        self.B = bounds[1] / Units.Metre.Value
        form.draft.setMaximum(bounds[2] / Units.Metre.Value)
        form.draft.setMinimum(0.001)
        form.draft.setValue(0.5 * bounds[2] / Units.Metre.Value)
        self.T = 0.5 * bounds[2] / Units.Metre.Value
        return False

    def retranslateUi(self):
        """Set the user interface locale strings."""
        self.form.setWindowTitle(QtGui.QApplication.translate(
            "ship_create",
            "Create a new ship",
            None,
            QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QLabel, "LengthLabel").setText(
            QtGui.QApplication.translate(
                "ship_create",
                "Length",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QLabel, "BreadthLabel").setText(
            QtGui.QApplication.translate(
                "ship_create",
                "Breadth",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QLabel, "DraftLabel").setText(
            QtGui.QApplication.translate(
                "ship_create",
                "Draft",
                None,
                QtGui.QApplication.UnicodeUTF8))

    def onData(self, value):
        """Updates the 3D preview on data changes.

        Keyword arguments:
        value -- Edited value. This parameter is required in order to use this
        method as a callback function, but it is unuseful.
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.length = self.widget(QtGui.QDoubleSpinBox, "Length")
        form.breadth = self.widget(QtGui.QDoubleSpinBox, "Breadth")
        form.draft = self.widget(QtGui.QDoubleSpinBox, "Draft")

        self.L = form.length.value()
        self.B = form.breadth.value()
        self.T = form.draft.value()
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
