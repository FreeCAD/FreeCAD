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

# FreeCAD modules
import FreeCAD as App
import FreeCADGui as Gui
# Qt library
from PyQt4 import QtGui,QtCore
# Module
import Preview
import Instance
from shipUtils import Paths, Translator
from surfUtils import Geometry

class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/shipCreateShip/TaskPanel.ui"
        self.preview = Preview.Preview()

    def accept(self):
        self.preview.clean()
        # Create new ship instance
        obj = App.ActiveDocument.addObject("Part::FeaturePython","Ship")
        ship = Instance.Ship(obj, self.faces)
        Instance.ViewProviderShip(obj.ViewObject)
        # Set main dimensions
        obj.Length = self.form.length.value()
        obj.Beam   = self.form.beam.value()
        obj.Draft  = self.form.draft.value()
        # Discretize it
        ship.discretize(self.form.nSections.value(), self.form.nPoints.value())
        return True

    def reject(self):
        self.preview.clean()
        self.close()
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
        form.length = form.findChild(QtGui.QDoubleSpinBox, "Length")
        form.beam = form.findChild(QtGui.QDoubleSpinBox, "Beam")
        form.draft = form.findChild(QtGui.QDoubleSpinBox, "Draft")
        form.nSections = form.findChild(QtGui.QSpinBox, "NSections")
        form.nPoints = form.findChild(QtGui.QSpinBox, "NPoints")
        form.mainLogo = form.findChild(QtGui.QLabel, "MainLogo")
        iconPath = Paths.iconsPath() + "/Ico.xpm"
        form.mainLogo.setPixmap(QtGui.QPixmap(iconPath))
        self.form = form
        # Initial values
        if self.initValues():
            return True
        self.retranslateUi()
        self.preview.update(self.L, self.B, self.T)
        # Connect Signals and Slots
        QtCore.QObject.connect(form.length, QtCore.SIGNAL("valueChanged(double)"), self.onData)
        QtCore.QObject.connect(form.beam, QtCore.SIGNAL("valueChanged(double)"), self.onData)
        QtCore.QObject.connect(form.draft, QtCore.SIGNAL("valueChanged(double)"), self.onData)
        QtCore.QObject.connect(form.nSections, QtCore.SIGNAL("valueChanged(int)"), self.onDiscretization)
        QtCore.QObject.connect(form.nPoints, QtCore.SIGNAL("valueChanged(int)"), self.onDiscretization)

    def getMainWindow(self):
        "returns the main window"
        # using QtGui.qApp.activeWindow() isn't very reliable because if another
        # widget than the mainwindow is active (e.g. a dialog) the wrong widget is
        # returned
        toplevel = QtGui.qApp.topLevelWidgets()
        for i in toplevel:
            if i.metaObject().className() == "Gui::MainWindow":
                return i
        raise Exception("No main window found")

    def initValues(self):
        """ Set initial values for fields
        """
        # Get objects
        self.faces = None
        selObjs  = Geometry.getSelectedObjs()
        if not selObjs:
            msg = Translator.translate("Ship objects can only be created on top of hull geometry (no object selected).\n")
            App.Console.PrintError(msg)
            msg = Translator.translate("Please create or download a ship hull geometry before using this tool\n")
            App.Console.PrintError(msg)
            return True
        self.faces = []
        for i in range(0, len(selObjs)):
            faces = Geometry.getFaces(selObjs[i])
            for j in range(0, len(faces)):
                self.faces.append(faces[j])
        if not self.faces:
            msg = Translator.translate("Ship objects can only be created on top of hull geometry (no face object selected).\n")
            App.Console.PrintError(msg)
            msg = Translator.translate("Please create or download a ship hull geometry before using this tool\n")
            App.Console.PrintError(msg)
            return True
        # Get bounds
        bounds = [0.0, 0.0, 0.0]
        bbox = self.faces[0].BoundBox
        minX = bbox.XMin
        maxX = bbox.XMax
        minY = bbox.YMin
        maxY = bbox.YMax
        minZ = bbox.ZMin
        maxZ = bbox.ZMax
        for i in range(1,len(self.faces)):
            bbox = self.faces[i].BoundBox
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
        # Set UI fields
        self.form.length.setMaximum(bounds[0])
        self.form.length.setMinimum(0.001)
        self.form.length.setValue(bounds[0])
        self.L = bounds[0]
        self.form.beam.setMaximum(2.0*bounds[1])
        self.form.beam.setMinimum(0.001)
        self.form.beam.setValue(2.0*bounds[1])
        self.B = 2.0*bounds[1]
        self.form.draft.setMaximum(bounds[2])
        self.form.draft.setMinimum(0.001)
        self.form.draft.setValue(0.5*bounds[2])
        self.T = 0.5*bounds[2]
        msg = Translator.translate("Ready to work\n")
        App.Console.PrintMessage(msg)
        return False

    def retranslateUi(self):
        """ Set user interface locale strings. 
        """
        self.form.setWindowTitle(Translator.translate("Create a new ship"))
        self.form.findChild(QtGui.QLabel, "LengthLabel").setText(Translator.translate("Length"))
        self.form.findChild(QtGui.QLabel, "BeamLabel").setText(Translator.translate("Beam"))
        self.form.findChild(QtGui.QLabel, "DraftLabel").setText(Translator.translate("Draft"))
        self.form.findChild(QtGui.QLabel, "NSectionsLabel").setText(Translator.translate("Number of sections"))
        self.form.findChild(QtGui.QLabel, "NPointsLabel").setText(Translator.translate("Points per section"))

    def onData(self, value):
        """ Method called when ship data is changed.
         Annotations must be showed.
         @param value Changed value.
        """
        self.L = self.form.length.value()
        self.B = self.form.beam.value()
        self.T = self.form.draft.value()
        self.preview.update(self.L, self.B, self.T)

    def onDiscretization(self, value):
        """ Method called when discretization data is changed.
         Annotations must be showed.
         @param value Changed value.
        """
        pass

    def close(self):
        """ Destroy all dependant objects
        """

def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
