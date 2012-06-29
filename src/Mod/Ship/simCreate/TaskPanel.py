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
import SimInstance
from shipUtils import Paths, Translator

class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/simCreate/TaskPanel.ui"

    def accept(self):
        form = self.form
        obj = App.ActiveDocument.addObject("Part::FeaturePython","ShipSimulation")
        sim = SimInstance.ShipSimulation(obj, 
              [form.length.value(), form.beam.value(), form.n.value()])
        SimInstance.ViewProviderShipSimulation(obj.ViewObject)
        return True

    def reject(self):
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
        form.n = form.findChild(QtGui.QSpinBox, "N")
        self.form = form
        # Initial values
        if self.initValues():
            return True
        self.retranslateUi()
        # Connect Signals and Slots
        QtCore.QObject.connect(form.length, QtCore.SIGNAL("valueChanged(double)"), self.onFS)
        QtCore.QObject.connect(form.beam, QtCore.SIGNAL("valueChanged(double)"), self.onFS)
        QtCore.QObject.connect(form.n, QtCore.SIGNAL("valueChanged(int)"), self.onFS)

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
        msg = Translator.translate("Ready to work\n")
        App.Console.PrintMessage(msg)
        return False

    def retranslateUi(self):
        """ Set user interface locale strings. 
        """
        self.form.setWindowTitle(Translator.translate("Create a new ship simulation"))
        self.form.findChild(QtGui.QGroupBox, "FSDataBox").setTitle(Translator.translate("Free surface"))
        self.form.findChild(QtGui.QLabel, "LengthLabel").setText(Translator.translate("Length"))
        self.form.findChild(QtGui.QLabel, "BeamLabel").setText(Translator.translate("Beam"))
        self.form.findChild(QtGui.QLabel, "NLabel").setText(Translator.translate("Number of points"))

    def onFS(self, value):
        """ Method called when ship data is changed.
         Annotations must be showed.
         @param value Changed value.
        """
        pass

def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
