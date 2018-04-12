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
from FreeCAD import Units
from PySide import QtGui, QtCore
from . import PlotAux
from . import Tools
from shipUtils import Paths
import shipUtils.Units as USys
import shipUtils.Locale as Locale


class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/shipGZ/TaskPanel.ui"

    def accept(self):
        if self.lc is None:
            return False
        self.save()

        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.angle = self.widget(QtGui.QLineEdit, "Angle")
        form.n_points = self.widget(QtGui.QSpinBox, "NumPoints")
        form.var_trim = self.widget(QtGui.QCheckBox, "VariableTrim")

        roll = Units.Quantity(Locale.fromString(form.angle.text()))
        n_points = form.n_points.value()
        var_trim = form.var_trim.isChecked()

        rolls = []
        for i in range(n_points):
            rolls.append(roll * i / float(n_points - 1))

        points = Tools.gz(self.lc, rolls, var_trim)
        gzs = []
        drafts = []
        trims = []
        for p in points:
            gzs.append(p[0].getValueAs('m').Value)
            drafts.append(p[1].getValueAs('m').Value)
            trims.append(p[2].getValueAs('deg').Value)

        PlotAux.Plot(rolls, gzs, drafts, trims)

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

        form.angle = self.widget(QtGui.QLineEdit, "Angle")
        form.n_points = self.widget(QtGui.QSpinBox, "NumPoints")
        form.var_trim = self.widget(QtGui.QCheckBox, "VariableTrim")
        self.form = form
        if self.initValues():
            return True
        self.retranslateUi()

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
        # Look for selected loading conditions (Spreadsheets)
        self.lc = None
        selObjs = Gui.Selection.getSelection()
        if not selObjs:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "A loading condition instance must be selected before using"
                " this tool (no objects selected)",
                None)
            App.Console.PrintError(msg + '\n')
            return True
        for i in range(len(selObjs)):
            obj = selObjs[i]
            try:
                if obj.TypeId != 'Spreadsheet::Sheet':
                    continue
            except ValueError:
                continue
            # Check if it is a Loading condition:
            # B1 cell must be a ship
            # B2 cell must be the loading condition itself
            doc = App.ActiveDocument
            try:
                if obj not in doc.getObjectsByLabel(obj.get('B2')):
                    continue
                ships = doc.getObjectsByLabel(obj.get('B1'))
                if len(ships) != 1:
                    if len(ships) == 0:
                        msg = QtGui.QApplication.translate(
                            "ship_console",
                            "Wrong Ship label! (no instances labeled as"
                            "'{}' found)",
                            None)
                        App.Console.PrintError(msg + '\n'.format(
                            obj.get('B1')))
                    else:
                        msg = QtGui.QApplication.translate(
                            "ship_console",
                            "Ambiguous Ship label! ({} instances labeled as"
                            "'{}' found)",
                            None)
                        App.Console.PrintError(msg + '\n'.format(
                            len(ships),
                            obj.get('B1')))
                    continue
                ship = ships[0]
                if ship is None or not ship.PropertiesList.index("IsShip"):
                    continue
            except ValueError:
                continue
            # Let's see if several loading conditions have been selected (and
            # prompt a warning)
            if self.lc:
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "More than one loading condition have been selected (the"
                    " extra loading conditions will be ignored)",
                    None)
                App.Console.PrintWarning(msg + '\n')
                break
            self.lc = obj
            self.ship = ship
        if not self.lc:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "A loading condition instance must be selected before using"
                " this tool (no valid loading condition found at the selected"
                " objects)",
                None)
            App.Console.PrintError(msg + '\n')
            return True

        # We have a valid loading condition, let's set the initial field values
        angle_format = USys.getAngleFormat()
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.angle = self.widget(QtGui.QLineEdit, "Angle")
        form.n_points = self.widget(QtGui.QSpinBox, "NumPoints")
        form.var_trim = self.widget(QtGui.QCheckBox, "VariableTrim")
        form.angle.setText(Locale.toString(angle_format.format(90.0)))
        # Try to use saved values
        props = self.ship.PropertiesList
        try:
            props.index("GZAngle")
            form.angle.setText(Locale.toString(angle_format.format(
                self.ship.GZAngle.getValueAs(
                    USys.getAngleUnits()).Value)))
        except:
            pass
        try:
            props.index("GZNumPoints")
            form.n_points.setValue(self.ship.GZNumPoints)
        except ValueError:
            pass
        try:
            props.index("GZVariableTrim")
            if self.ship.GZVariableTrim:
                form.var_trim.setCheckState(QtCore.Qt.Checked)
            else:
                form.var_trim.setCheckState(QtCore.Qt.Unchecked)
        except ValueError:
            pass


        return False

    def retranslateUi(self):
        """ Set user interface locale strings. """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.setWindowTitle(QtGui.QApplication.translate(
            "ship_gz",
            "Plot the GZ curve",
            None))
        self.widget(QtGui.QLabel, "AngleLabel").setText(
            QtGui.QApplication.translate(
                "ship_gz",
                "Maximum angle",
                None))
        self.widget(QtGui.QLabel, "NumPointsLabel").setText(
            QtGui.QApplication.translate(
                "ship_gz",
                "Number of points",
                None))
        self.widget(QtGui.QCheckBox, "VariableTrim").setText(
            QtGui.QApplication.translate(
                "ship_gz",
                "Variable trim",
                None))
        self.widget(QtGui.QCheckBox, "VariableTrim").setToolTip(
            QtGui.QApplication.translate(
                "ship_gz",
                "The ship will be rotated to the equilibrium trim angle for" + \
                " each roll angle. It will significantly increase the" + \
                " required computing time",
                None))

    def save(self):
        """ Saves the data into ship instance. """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.angle = self.widget(QtGui.QLineEdit, "Angle")
        form.n_points = self.widget(QtGui.QSpinBox, "NumPoints")
        form.var_trim = self.widget(QtGui.QCheckBox, "VariableTrim")

        angle = Units.Quantity(Locale.fromString(
            form.angle.text())).getValueAs('deg').Value
        n_points = form.n_points.value()
        var_trim = form.var_trim.isChecked()

        props = self.ship.PropertiesList
        try:
            props.index("GZAngle")
        except ValueError:
            try:
                tooltip = str(QtGui.QApplication.translate(
                    "ship_gz",
                    "GZ curve tool angle selected [deg]",
                    None))
            except:
                tooltip = "GZ curve tool angle selected [deg]"
            self.ship.addProperty("App::PropertyAngle",
                                  "GZAngle",
                                  "Ship",
                                  tooltip)
        self.ship.GZAngle = '{} deg'.format(angle)
        try:
            props.index("GZNumPoints")
        except ValueError:
            try:
                tooltip = str(QtGui.QApplication.translate(
                    "ship_gz",
                    "GZ curve tool number of points selected",
                    None))
            except:
                tooltip = "GZ curve tool number of points selected"
            self.ship.addProperty("App::PropertyInteger",
                                  "GZNumPoints",
                                  "Ship",
                                  tooltip)
        self.ship.GZNumPoints = n_points
        try:
            props.index("GZVariableTrim")
        except ValueError:
            try:
                tooltip = str(QtGui.QApplication.translate(
                    "ship_gz",
                    "GZ curve tool variable trim angle selection",
                    None))
            except:
                tooltip = "GZ curve tool variable trim angle selection"
            self.ship.addProperty("App::PropertyBool",
                                  "GZVariableTrim",
                                  "Ship",
                                  tooltip)
        self.ship.GZVariableTrim = var_trim

def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
