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
from FreeCAD import Base, Vector
import FreeCADGui as Gui
from FreeCAD import Units
import Part
from PySide import QtGui, QtCore
from . import Preview
import Instance
from shipUtils import Paths
import shipUtils.Locale as Locale


class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/shipOutlineDraw/TaskPanel.ui"
        self.ship = None
        self.skip = False
        self.LSections = []
        self.BSections = []
        self.TSections = []
        self.obj = None
        self.preview = Preview.Preview()

    def accept(self):
        self.saveSections()
        # Add ship edges to the object
        edges = self.getEdges([self.ship.Shape])
        border = edges[0]
        for i in range(len(edges)):
            border = border.oldFuse(edges[i])
            border = border.oldFuse(edges[i].mirror(Vector(0.0, 0.0, 0.0),
                                                    Vector(0.0, 1.0, 0.0)))
        obj = border.oldFuse(self.obj.Shape)

        # Send the generated object to the scene
        Part.show(obj)
        objs = App.ActiveDocument.Objects
        self.obj = objs[len(objs) - 1]

        self.preview.clean()
        self.obj.Label = 'OutlineDraw'
        return True

    def getEdges(self, objs):
        """Return object edges (as a list)
        """
        edges = []
        if not objs:
            return None
        for i in range(len(objs)):
            obj = objs[i]
            if obj.isDerivedFrom('Part::Feature'):
                # get shape
                shape = obj.Shape
                if not shape:
                    return None
                obj = shape
            if not obj.isDerivedFrom('Part::TopoShape'):
                return None
            objEdges = obj.Edges
            if not objEdges:
                continue
            for j in range(0, len(objEdges)):
                edges.append(objEdges[j])
        return edges

    def reject(self):
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
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.sections = self.widget(QtGui.QTableWidget, "Sections")
        try:
            form.sections.setInputMethodHints(
                QtCore.Qt.ImhFormattedNumbersOnly)
            hasImhFormattedNumbersOnly = True
        except:
            hasImhFormattedNumbersOnly = False
        form.sectionType = self.widget(QtGui.QComboBox, "SectionType")
        form.deleteButton = self.widget(QtGui.QPushButton, "DeleteButton")
        form.nSections = self.widget(QtGui.QSpinBox, "NSections")
        form.createButton = self.widget(QtGui.QPushButton, "CreateButton")
        self.form = form
        # Initial values
        if self.initValues():
            return True
        self.retranslateUi()
        #don't do that here.
        #self.obj = self.preview.update(self.ship.Length.getValueAs('m').Value,
       #                                self.ship.Breadth.getValueAs('m').Value,
        #                               self.ship.Draft.getValueAs('m').Value,
        #                               self.LSections,
        #                               self.BSections,
        #                               self.TSections,
        #                               self.ship.Shape)
        # Connect Signals and Slots
        QtCore.QObject.connect(
            form.sectionType,
            QtCore.SIGNAL("activated(QString)"),
            self.onSectionType)
        QtCore.QObject.connect(
            form.sections,
            QtCore.SIGNAL("cellChanged(int,int)"),
            self.onTableItem)
        QtCore.QObject.connect(
            form.deleteButton,
            QtCore.SIGNAL("pressed()"),
            self.onDeleteButton)
        QtCore.QObject.connect(
            form.createButton,
            QtCore.SIGNAL("pressed()"),
            self.onCreateButton)
            
    def createPreview(self):
        self.obj = self.preview.update(self.ship.Length.getValueAs('m').Value,
                                      self.ship.Breadth.getValueAs('m').Value,
                                       self.ship.Draft.getValueAs('m').Value,
                                       self.LSections,
                                       self.BSections,
                                       self.TSections,
                                       self.ship.Shape)
    def getMainWindow(self):
        toplevel = QtGui.QApplication.topLevelWidgets()
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
        """ Set initial values for fields
        """
        # Get selected objects
        selObjs = Gui.Selection.getSelection()
        if not selObjs:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "A ship instance must be selected before use this tool (no"
                " objects selected)",
                None)
            App.Console.PrintError(msg + '\n')
            return True
        for i in range(0, len(selObjs)):
            obj = selObjs[i]
            # Test if is a ship instance
            props = obj.PropertiesList
            try:
                props.index("IsShip")
            except ValueError:
                continue
            if obj.IsShip:
                # Test if another ship already selected
                if self.ship:
                    msg = QtGui.QApplication.translate(
                        "ship_console",
                        "More than one ship has been selected (just the first"
                        " one will be used)",
                        None)
                    App.Console.PrintWarning(msg + '\n')
                    break
                self.ship = obj
        # Test if any valid ship was selected
        if not self.ship:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "A ship instance must be selected before use this tool (no"
                "valid ships found in the selected objects)",
                None)
            App.Console.PrintError(msg + '\n')
            return True
        # Load sections (if exist)
        self.loadSections()
        return False

    def retranslateUi(self):
        """Set the user interface locale strings."""
        self.form.setWindowTitle(QtGui.QApplication.translate(
            "ship_outline",
            "Outline draw",
            None))
        self.widget(QtGui.QGroupBox, "AutoCreateBox").setTitle(
            QtGui.QApplication.translate(
                "ship_outline",
                "Auto create",
                None))
        self.widget(QtGui.QPushButton, "DeleteButton").setText(
            QtGui.QApplication.translate(
                "ship_outline",
                "Delete all sections",
                None))
        self.widget(QtGui.QPushButton, "CreateButton").setText(
            QtGui.QApplication.translate(
                "ship_outline",
                "Create sections",
                None))
        self.widget(QtGui.QComboBox, "SectionType").setItemText(
            0,
            QtGui.QApplication.translate(
                "ship_outline",
                "Transversal",
                None))
        self.widget(QtGui.QComboBox, "SectionType").setItemText(
            1,
            QtGui.QApplication.translate(
                "ship_outline",
                "Longitudinal",
                None))
        self.widget(QtGui.QComboBox, "SectionType").setItemText(
            2,
            QtGui.QApplication.translate(
                "ship_outline",
                "Water lines",
                None))

    def onSectionType(self):
        """ Function called when the section type is changed.
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.sectionType = self.widget(QtGui.QComboBox, "SectionType")

        ID = form.sectionType.currentIndex()
        self.setSectionType(ID)

    def setSectionType(self, ID):
        """ Set the table depending on the selected section type.
        @param ID Id of the section type to set:
          - 0 = Transversal sections
          - 1 = Longitudinal sections
          - 2 = Water lines
        """
        SectionList = []
        if ID == 0:
            SectionList = self.LSections[:]
        elif ID == 1:
            SectionList = self.BSections[:]
        elif ID == 2:
            SectionList = self.TSections[:]
        nRow = len(SectionList)

        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.sections = self.widget(QtGui.QTableWidget, "Sections")

        form.sections.clearContents()
        form.sections.setRowCount(nRow + 1)
        self.skip = True
        for i in range(0, nRow):
            string = '{} m'.format(SectionList[i])
            item = QtGui.QTableWidgetItem(string)
            form.sections.setItem(i, 0, item)
        self.skip = False
        self.obj = self.preview.update(self.ship.Length.getValueAs('m').Value,
                                       self.ship.Breadth.getValueAs('m').Value,
                                       self.ship.Draft.getValueAs('m').Value,
                                       self.LSections,
                                       self.BSections,
                                       self.TSections,
                                       self.ship.Shape)


    def onTableItem(self, row, column):
        """ Function called when an item of the table is touched.
        @param row Changed item row
        @param column Changed item column
        """
        if self.skip:
            return

        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.sections = self.widget(QtGui.QTableWidget, "Sections")
        form.sectionType = self.widget(QtGui.QComboBox, "SectionType")

        # Add an empty item at the end of the list
        nRow = form.sections.rowCount()
        item = form.sections.item(nRow - 1, 0)
        if item:
            if(item.text() != ''):
                form.sections.setRowCount(nRow + 1)

        ID = form.sectionType.currentIndex()
        if ID == 0:
            SectionList = self.LSections
        elif ID == 1:
            SectionList = self.BSections
        elif ID == 2:
            SectionList = self.TSections

        item = form.sections.item(row, column)
        # Look for deleted row (empty string)
        if not item.text():
            del SectionList[row]
            form.sections.removeRow(row)
            self.obj = self.preview.update(self.ship.Length.getValueAs('m').Value,
                                           self.ship.Breadth.getValueAs('m').Value,
                                           self.ship.Draft.getValueAs('m').Value,
                                           self.LSections,
                                           self.BSections,
                                           self.TSections,
                                           self.ship.Shape)
            return

        # Get the new section value
        try:
            qty = Units.Quantity(item.text())
            number = qty.getValueAs('m').Value
        except:
            number = 0.0

        string = '{} m'.format(number)
        item.setText(Locale.toString(string))
        # Regenerate the list
        del SectionList[:]
        for i in range(0, nRow):
            item = form.sections.item(i, 0)
            try:
                qty = Units.Quantity(item.text())
                number = qty.getValueAs('m').Value
            except:
                number = 0.0
            SectionList.append(number)

        self.obj = self.preview.update(self.ship.Length.getValueAs('m').Value,
                                       self.ship.Breadth.getValueAs('m').Value,
                                       self.ship.Draft.getValueAs('m').Value,
                                       self.LSections,
                                       self.BSections,
                                       self.TSections,
                                       self.ship.Shape)

    def onDeleteButton(self):
        """ Function called when the delete button is pressed.
        All the sections of the active type must be erased therefore.
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.sections = self.widget(QtGui.QTableWidget, "Sections")
        form.sectionType = self.widget(QtGui.QComboBox, "SectionType")

        form.sections.clearContents()
        form.sections.setRowCount(1)
        ID = form.sectionType.currentIndex()
        if ID == 0:
            del self.LSections[:]
        elif ID == 1:
            del self.BSections[:]
        elif ID == 2:
            del self.TSections[:]
        self.setSectionType(ID)

    def onCreateButton(self):
        """ Function called when automatic creating button is pressed.
        Several sections must be added to the active sections list
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.sectionType = self.widget(QtGui.QComboBox, "SectionType")
        form.nSections = self.widget(QtGui.QSpinBox, "NSections")

        # Recolect data
        nSections = form.nSections.value()
        SectionList = []
        L = 0.0
        ID = form.sectionType.currentIndex()
        if ID == 0:
            L = self.ship.Length.getValueAs('m').Value
            d = L / (nSections - 1)
            start = - L / 2.0
        elif ID == 1:
            L = -0.5 * self.ship.Breadth.getValueAs('m').Value
            d = L / (nSections + 1)
            start = d
        elif ID == 2:
            L = self.ship.Draft.getValueAs('m').Value
            d = L / (nSections)
            start = d
        # Compute the sections positions
        for i in range(0, nSections):
            sec = i * d + start
            SectionList.append(sec)
        # Paste it into the corresponding section list
        if ID == 0:
            self.LSections = SectionList[:]
        elif ID == 1:
            self.BSections = SectionList[:]
        elif ID == 2:
            self.TSections = SectionList[:]
        self.setSectionType(ID)
        self.createPreview()

    def loadSections(self):
        """ Loads from the ship object all the previously selected sections.
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.sectionType = self.widget(QtGui.QComboBox, "SectionType")

        # Load sections
        props = self.ship.PropertiesList
        flag = True
        try:
            props.index("LSections")
        except ValueError:
            flag = False
        if flag:
            self.LSections = self.ship.LSections[:]
            self.BSections = self.ship.BSections[:]
            self.TSections = self.ship.TSections[:]
        # Set UI
        self.setSectionType(form.sectionType.currentIndex())

    def saveSections(self):
        """ Save the selected sections into ship object.
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")

        props = self.ship.PropertiesList
        try:
            props.index("LSections")
        except ValueError:
            tooltip = str(QtGui.QApplication.translate(
                "ship_outline",
                "Transversal section positions [m]",
                None))
            self.ship.addProperty("App::PropertyFloatList",
                                  "LSections",
                                  "Ship",
                                  tooltip).LSections = []
            tooltip = str(QtGui.QApplication.translate(
                "ship_outline",
                "Longitudinal section positions [m]",
                None))
            self.ship.addProperty("App::PropertyFloatList",
                                  "BSections",
                                  "Ship",
                                  tooltip).BSections = []
            tooltip = str(QtGui.QApplication.translate(
                "ship_outline",
                "Water line positions [m]",
                None))
            self.ship.addProperty("App::PropertyFloatList",
                                  "TSections",
                                  "Ship",
                                  tooltip).TSections = []
        # Save the sections
        self.ship.LSections = self.LSections[:]
        self.ship.BSections = self.BSections[:]
        self.ship.TSections = self.TSections[:]


def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
