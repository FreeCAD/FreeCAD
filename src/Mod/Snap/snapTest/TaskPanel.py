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
# Pivy
import pivy
from pivy import coin
from pivy.coin import *
# Module
from snapUtils import Paths, Translator

class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/snapTest/TaskPanel.ui"
        self.point = None

    def accept(self):
        self.close()
        return True

    def reject(self):
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
        form.label = form.findChild(QtGui.QLabel, "pointLabel")
        self.form = form
        self.retranslateUi()
        # Register coin callbacks
        self.view = Gui.ActiveDocument.ActiveView
        self.callbackMove = self.view.addEventCallbackPivy(SoLocation2Event.getClassTypeId(),self.mouseMove)
        self.callback = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.mouseButton)
    
    def mouseMove(self, event_cb):
        event = event_cb.getEvent()
        # Get screen point
        screen = event.getPosition()
        # Get snapped point
        point = Gui.newSnapper.snap(screen, self.point)

    def mouseButton(self, event_cb):
        event = event_cb.getEvent()
        if event.getState() != coin.SoMouseButtonEvent.DOWN:
            return
        # Get screen point
        screen = event.getPosition()
        # Get snapped object if exist, else the screen point will used
        self.point = Gui.newSnapper.snap(screen)
        self.form.label.setText('Point: %g, %g, %g' % (self.point.x, self.point.y, self.point.z))

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

    def retranslateUi(self):
        """ Set user interface locale strings. 
        """
        self.form.setWindowTitle(Translator.translate("Select points"))

    def close(self):
        """ Destroy all dependant objects
        @param self Main object.
        """
        # Switch off snapping
        Gui.Snapper.off()
        # Remove callback (Program crash otherwise)
        self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.callback)
        self.view.removeEventCallbackPivy(SoLocation2Event.getClassTypeId(),self.callbackMove)

def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
