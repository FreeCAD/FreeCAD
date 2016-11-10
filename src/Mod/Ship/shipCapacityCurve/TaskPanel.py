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

import math
import FreeCAD as App
import FreeCADGui as Gui
import Units
from PySide import QtGui, QtCore
import Tools
import PlotAux
import TankInstance as Instance
from shipUtils import Paths
import shipUtils.Units as USys


class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/shipCapacityCurve/TaskPanel.ui"
        self.tank = None

    def accept(self):
        if self.tank is None:
            return False

        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.points = self.widget(QtGui.QSpinBox, "Points")
        n = form.points.value()

        points = Tools.tankCapacityCurve(self.tank, n)
        l = []
        z = []
        v = []
        for p in points:
            l.append(p[0] * 100)
            z.append(p[1].getValueAs("m").Value)
            v.append(p[2].getValueAs("m^3").Value)

        PlotAux.Plot(l, z, v, self.tank)
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
        form.points = self.widget(QtGui.QSpinBox, "Points")
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
        """ Set initial values for fields
        """
        selObjs = Gui.Selection.getSelection()
        if not selObjs:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "A tank instance must be selected before using this tool (no"
                " objects selected)",
                None,
                QtGui.QApplication.UnicodeUTF8)
            App.Console.PrintError(msg + '\n')
            return True
        for i in range(0, len(selObjs)):
            obj = selObjs[i]
            props = obj.PropertiesList
            try:
                props.index("IsTank")
            except ValueError:
                continue
            if obj.IsTank:
                if self.tank:
                    msg = QtGui.QApplication.translate(
                        "ship_console",
                        "More than one tank have been selected (the extra"
                        " tanks will be ignored)",
                        None,
                        QtGui.QApplication.UnicodeUTF8)
                    App.Console.PrintWarning(msg + '\n')
                    break
                self.tank = obj
        if not self.tank:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "A tank instance must be selected before using this tool (no"
                " valid tank found at the selected objects)",
                None,
                QtGui.QApplication.UnicodeUTF8)
            App.Console.PrintError(msg + '\n')
            return True
        return False

    def retranslateUi(self):
        """ Set user interface locale strings. """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.setWindowTitle(QtGui.QApplication.translate(
            "ship_capacity",
            "Plot the tank capacity curve",
            None,
            QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QLabel, "PointsLabel").setText(
            QtGui.QApplication.translate(
                "ship_capacity",
                "Number of points",
                None,
                QtGui.QApplication.UnicodeUTF8))

def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
