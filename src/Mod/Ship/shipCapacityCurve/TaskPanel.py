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
        # Plot data
        l, z, v = self.compute()
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

    def compute(self):
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.points = self.widget(QtGui.QSpinBox, "Points")

        bbox = self.tank.Shape.BoundBox
        dz = Units.Quantity(bbox.ZMax - bbox.ZMin, Units.Length)

        n = form.points.value()
        dlevel = 1.0 / (n - 1)
        l = [0.0]
        v = [0.0]
        z = [0.0]

        msg = QtGui.QApplication.translate(
            "ship_console",
            "Computing capacity curves",
            None,
            QtGui.QApplication.UnicodeUTF8)
        App.Console.PrintMessage(msg + '...\n')
        for i in range(1, n):
            App.Console.PrintMessage("\t{} / {}\n".format(i + 1, n))
            level = i * dlevel
            vol = self.tank.Proxy.getVolume(self.tank, level)
            l.append(level * 100.0)
            z.append(level * dz.getValueAs("m").Value)
            v.append(vol.getValueAs("m^3").Value)
        return (l, z, v)


def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
