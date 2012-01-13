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
# Qt library
from PyQt4 import QtGui,QtCore
# Module
from surfUtils import Paths, Geometry, Math, Translator
import Preview
import PointTracker

class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/surfISOCurve/TaskPanel.ui"
        self.preview = Preview.Preview()
        self.tracker = None

    def accept(self):
        if not self.obj:
            return True
        self.obj.Label = 'ISOCurve'
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
        form.uv = form.findChild(QtGui.QSlider, "uv")
        form.uvText = form.findChild(QtGui.QDoubleSpinBox, "uvText")
        self.form = form
        # Initial values
        if self.initValues():
            return True
        self.obj = self.preview.update(self.surf, self.dirId, self.uv)
        self.retranslateUi()
        # Connect Signals and Slots
        QtCore.QObject.connect(form.direction, QtCore.SIGNAL("activated(QString)"), self.selectDirection)
        QtCore.QObject.connect(form.uv, QtCore.SIGNAL("valueChanged(int)"), self.onUVSlider)
        QtCore.QObject.connect(form.uvText, QtCore.SIGNAL("valueChanged(double)"), self.onUVText)

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
        
    def selectDirection(self):
        self.dirId = self.form.direction.currentIndex()
        self.obj = self.preview.update(self.surf, self.dirId, self.uv)
        if not self.obj:
            msg = Translator.translate("Can't get the curve from selected object")
            App.Console.PrintWarning(msg)

    def onUVSlider(self, value):
        uv = self.form.uv.value()
        uv = uv/(float(self.form.uv.maximum()))
        self.setUV(uv)

    def onUVText(self, value):
        uv = self.form.uvText.value()
        self.setUV(uv)

    def setUV(self, uv):
        self.form.uv.setValue(uv*self.form.uv.maximum())
        self.form.uvText.setValue(uv)
        self.uv = uv
        if not self.surf:
            return
        self.obj = self.preview.update(self.surf, self.dirId, self.uv)
        if not self.obj:
            msg = Translator.translate("Can't get the curve from selected object")
            App.Console.PrintWarning(msg)

    def initValues(self):
        self.obj = None
        self.dirId = 0
        self.uv = 0
        self.surf = None
        self.selObj  = Geometry.getSelectedObj()
        if not self.selObj:
            msg = Translator.translate("1 surface must be selected (Any object has been selected)")
            App.Console.PrintError(msg)
            return True
        self.surf = Geometry.getSelectedSurface()
        if not self.surf:
            msg = Translator.translate("1 surface must be selected (Any face object found into selected objects)")
            App.Console.PrintError(msg)
            return True
        msg = Translator.translate("Ready to work")
        App.Console.PrintMessage(msg)
        return False

    def retranslateUi(self):
        """ Set user interface locale strings. 
        """
        self.form.setWindowTitle(Translator.translate("Get surface ISO curve"))
        self.form.direction.setItemText(0, Translator.translate("U direction"))
        self.form.direction.setItemText(1, Translator.translate("V direction"))

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
