#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
#*   Jose Luis Cercós Pita <jlcercos@gmail.com>                            *  
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
from FreeCAD import Vector
# Qt library
from PyQt4 import QtGui,QtCore
# Module
from surfUtils import Paths, Geometry, Math, Translator
import Preview
import PointTracker

class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/surfSlice/TaskPanel.ui"
        self.preview = Preview.Preview()
        self.tracker = None
        self.dir = Vector(0.0,0.0,1.0)

    def accept(self):
        if not self.objs:
            return True
        for i in range(0,len(self.objs)):
            self.objs[i].Label = 'SliceCurve'
        self.close()
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
        form.direction = form.findChild(QtGui.QComboBox, "direction")
        form.r = form.findChild(QtGui.QSlider, "r")
        form.rText = form.findChild(QtGui.QDoubleSpinBox, "rText")
        self.form = form
        # Initial values
        if self.initValues():
            return True
        self.retranslateUi()
        self.form.rText.setMinimum(self.bound[4])
        self.form.rText.setMaximum(self.bound[5])
        self.setR(self.r)
        # Connect Signals and Slots
        QtCore.QObject.connect(form.direction, QtCore.SIGNAL("activated(QString)"), self.selectDirection)
        QtCore.QObject.connect(form.r, QtCore.SIGNAL("valueChanged(int)"), self.onRSlider)
        QtCore.QObject.connect(form.rText, QtCore.SIGNAL("valueChanged(double)"), self.onRText)
        return False

    def getMainWindow(self):
        "returns the main window"
        # using QtGui.qApp.activeWindow() isn't very reliable because if another
        # widget than the mainwindow is active (i.e. a dialog) the wrong widget is
        # returned
        toplevel = QtGui.qApp.topLevelWidgets()
        for i in toplevel:
            if i.metaObject().className() == "Gui::MainWindow":
                return i
        raise Exception("No main window found")
        
    def selectDirection(self):
        if(self.form.direction.currentIndex() == 0):    # XY
            self.dir = Vector(0.0,0.0,1.0)
            self.r = max(self.r, self.bound[4])
            self.r = min(self.r, self.bound[5])
            self.form.rText.setMinimum(self.bound[4])
            self.form.rText.setMaximum(self.bound[5])
        if(self.form.direction.currentIndex() == 1):    # XZ
            self.dir = Vector(0.0,1.0,0.0)
            self.r = max(self.r, self.bound[2])
            self.r = min(self.r, self.bound[3])
            self.form.rText.setMinimum(self.bound[2])
            self.form.rText.setMaximum(self.bound[3])
        if(self.form.direction.currentIndex() == 2):    # YZ
            self.dir = Vector(1.0,0.0,0.0)
            self.r = max(self.r, self.bound[0])
            self.r = min(self.r, self.bound[1])
            self.form.rText.setMinimum(self.bound[0])
            self.form.rText.setMaximum(self.bound[1])
        self.objs = self.preview.update(self.face, self.dir, self.r)
        if not self.objs:
            msg = Translator.translate("Can't calculate section")
            App.Console.PrintWarning(msg)

    def onRSlider(self, value):
        # Get r at [0,1] interval
        r = self.form.r.value()
        r = r/(float(self.form.r.maximum()))
        # Expand to real interval
        dr = self.form.rText.maximum()-self.form.rText.minimum()
        r = r*dr + self.form.rText.minimum()
        # Set value
        self.setR(r)

    def onRText(self, value):
        r = self.form.rText.value()
        self.setR(r)

    def setR(self, r):
        self.form.rText.setValue(r)
        self.r = max(self.form.rText.minimum(), min(self.form.rText.maximum(), r))
        dr = self.form.rText.maximum()-self.form.rText.minimum()
        r = (r - self.form.rText.minimum())/dr*self.form.r.maximum()
        self.form.r.setValue(r)
        if not self.face:
            return
        self.objs = self.preview.update(self.face, self.dir, self.r)
        if not self.objs:
            msg = Translator.translate("Can't calculate section")
            App.Console.PrintWarning(msg)

    def setR3(self, r):
        if(self.form.direction.currentIndex() == 0):    # XY
            self.setR(r.z)
        if(self.form.direction.currentIndex() == 1):    # XZ
            self.setR(r.y)
        if(self.form.direction.currentIndex() == 2):    # YZ
            self.setR(r.x)

    def initValues(self):
        self.objs = None
        self.dir = Vector(0.0,0.0,1.0)
        self.r = 0.0
        self.face = None
        self.selObj  = Geometry.getSelectedObj()
        if not self.selObj:
            msg = Translator.translate("At least 1 surface must be selected (Any selected object)")
            App.Console.PrintError(msg)
            return True
        self.face = Geometry.getFaces()
        if not self.face:
            msg = Translator.translate("At least 1 surface must be selected (Any face object found into selected objects)")
            App.Console.PrintError(msg)
            return True
        bound = self.face[0].BoundBox
        self.bound = [bound.XMin, bound.XMax, bound.YMin, bound.YMax, bound.ZMin, bound.ZMax]
        for i in range(1,len(self.face)):
            face = self.face[i]
            bound = face.BoundBox
            self.bound[0] = min(self.bound[0],bound.XMin)
            self.bound[1] = min(self.bound[1],bound.XMax)
            self.bound[2] = min(self.bound[2],bound.YMin)
            self.bound[3] = min(self.bound[3],bound.YMax)
            self.bound[4] = min(self.bound[4],bound.ZMin)
            self.bound[5] = min(self.bound[5],bound.ZMax)
        self.r = max(self.r, self.bound[4])        
        self.r = min(self.r, self.bound[5])
        msg = Translator.translate("Ready to work")
        App.Console.PrintMessage(msg)
        return False

    def retranslateUi(self):
        """ Set user interface locale strings. 
        """
        self.form.setWindowTitle(Translator.translate("Get surface slice"))

    def close(self):
        """ Destroy all dependant objects
        @param self Main object.
        """
        if self.tracker:
            self.tracker.close()

def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    tracker = PointTracker.PointTracker(Gui.ActiveDocument.ActiveView, panel)
    return panel
