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

import math
import FreeCAD as App
import FreeCADGui as Gui
import Units
from PySide import QtGui, QtCore
import PlotAux
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
        form.var_draft = self.widget(QtGui.QCheckBox, "VariableDraft")
        form.var_trim = self.widget(QtGui.QCheckBox, "VariableTrim")
        self.form = form
        if self.initValues():
            return True
        self.retranslateUi()

    def getMainWindow(self):
        toplevel = QtGui.qApp.topLevelWidgets()
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
                None,
                QtGui.QApplication.UnicodeUTF8)
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
                            None,
                            QtGui.QApplication.UnicodeUTF8)
                        App.Console.PrintError(msg + '\n'.format(
                            obj.get('B1')))
                    else:
                        msg = QtGui.QApplication.translate(
                            "ship_console",
                            "Ambiguous Ship label! ({} instances labeled as"
                            "'{}' found)",
                            None,
                            QtGui.QApplication.UnicodeUTF8)
                        App.Console.PrintError(msg + '\n'.format(
                            len(ships),
                            obj.get('B1')))
                    continue
                ship = ships[0]
                if ship is None or not ship.PropertiesList.index("IsShip"):
                    continue
            except ValueError:
                continue
            # Extract the weights and the tanks
            weights = []
            index = 6
            while True:
                try:
                    ws = doc.getObjectsByLabel(obj.get('A{}'.format(index)))
                except ValueError:
                    break
                index += 1
                if len(ws) != 1:
                    if len(ws) == 0:
                        msg = QtGui.QApplication.translate(
                            "ship_console",
                            "Wrong Weight label! (no instances labeled as"
                            "'{}' found)",
                            None,
                            QtGui.QApplication.UnicodeUTF8)
                        App.Console.PrintError(msg + '\n'.format(
                            obj.get('A{}'.format(index - 1))))
                    else:
                        msg = QtGui.QApplication.translate(
                            "ship_console",
                            "Ambiguous Weight label! ({} instances labeled as"
                            "'{}' found)",
                            None,
                            QtGui.QApplication.UnicodeUTF8)
                        App.Console.PrintError(msg + '\n'.format(
                            len(ws),
                            obj.get('A{}'.format(index - 1))))
                    continue
                w = ws[0]
                try:
                    if w is None or not w.PropertiesList.index("IsWeight"):
                        msg = QtGui.QApplication.translate(
                            "ship_console",
                            "Invalid Weight! (the object labeled as"
                            "'{}' is not a weight)",
                            None,
                            QtGui.QApplication.UnicodeUTF8)
                        App.Console.PrintError(msg + '\n'.format(
                            len(ws),
                            obj.get('A{}'.format(index - 1))))
                        continue
                except ValueError:
                    continue
                weights.append(w)
            tanks = []
            index = 6
            while True:
                try:
                    ts = doc.getObjectsByLabel(obj.get('C{}'.format(index)))
                    dens = float(obj.get('D{}'.format(index)))
                    level = float(obj.get('E{}'.format(index)))
                except ValueError:
                    break
                index += 1
                if len(ts) != 1:
                    if len(ts) == 0:
                        msg = QtGui.QApplication.translate(
                            "ship_console",
                            "Wrong Tank label! (no instances labeled as"
                            "'{}' found)",
                            None,
                            QtGui.QApplication.UnicodeUTF8)
                        App.Console.PrintError(msg + '\n'.format(
                            obj.get('C{}'.format(index - 1))))
                    else:
                        msg = QtGui.QApplication.translate(
                            "ship_console",
                            "Ambiguous Tank label! ({} instances labeled as"
                            "'{}' found)",
                            None,
                            QtGui.QApplication.UnicodeUTF8)
                        App.Console.PrintError(msg + '\n'.format(
                            len(ts),
                            obj.get('C{}'.format(index - 1))))
                    continue
                t = ts[0]
                try:
                    if t is None or not t.PropertiesList.index("IsTank"):
                        msg = QtGui.QApplication.translate(
                            "ship_console",
                            "Invalid Tank! (the object labeled as"
                            "'{}' is not a tank)",
                            None,
                            QtGui.QApplication.UnicodeUTF8)
                        App.Console.PrintError(msg + '\n'.format(
                            len(ws),
                            obj.get('C{}'.format(index - 1))))
                        continue
                except ValueError:
                    continue
                tanks.append((t, dens, level))
            # Let's see if several loading conditions have been selected (and
            # prompt a warning)
            if self.lc:
                msg = QtGui.QApplication.translate(
                    "ship_console",
                    "More than one loading condition have been selected (the"
                    " extra loading conditions will be ignored)",
                    None,
                    QtGui.QApplication.UnicodeUTF8)
                App.Console.PrintWarning(msg + '\n')
                break
            self.lc = obj
            self.ship = ship
            self.weights = weights
            self.tanks = tanks
        if not self.lc:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "A loading condition instance must be selected before using"
                " this tool (no valid loading condition found at the selected"
                " objects)",
                None,
                QtGui.QApplication.UnicodeUTF8)
            App.Console.PrintError(msg + '\n')
            return True

        # We have a valid loading condition, let's set the initial field values
        angle_format = USys.getAngleFormat()
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.angle = self.widget(QtGui.QLineEdit, "Angle")
        form.n_points = self.widget(QtGui.QSpinBox, "NumPoints")
        form.var_draft = self.widget(QtGui.QCheckBox, "VariableDraft")
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
            props.index("GZVariableDraft")
            if self.ship.GZVariableDraft:
                form.var_draft.setCheckState(QtCore.Qt.Checked)
            else:
                form.var_draft.setCheckState(QtCore.Qt.Unchecked)
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
            None,
            QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QLabel, "AngleLabel").setText(
            QtGui.QApplication.translate(
                "ship_gz",
                "Maximum angle",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QLabel, "NumPointsLabel").setText(
            QtGui.QApplication.translate(
                "ship_gz",
                "Number of points",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QCheckBox, "VariableDraft").setText(
            QtGui.QApplication.translate(
                "ship_gz",
                "Variable draft",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QCheckBox, "VariableDraft").setToolTip(
            QtGui.QApplication.translate(
                "ship_gz",
                "The ship will be moved to the equilibrium draft for each" + \
                " roll angle. It will significantly increase the required" + \
                " computing time",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QCheckBox, "VariableTrim").setText(
            QtGui.QApplication.translate(
                "ship_gz",
                "Variable trim",
                None,
                QtGui.QApplication.UnicodeUTF8))
        self.widget(QtGui.QCheckBox, "VariableTrim").setToolTip(
            QtGui.QApplication.translate(
                "ship_gz",
                "The ship will be rotated to the equilibrium trim angle for" + \
                " each roll angle. It will significantly increase the" + \
                " required computing time",
                None,
                QtGui.QApplication.UnicodeUTF8))

    def save(self):
        """ Saves the data into ship instance. """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.angle = self.widget(QtGui.QLineEdit, "Angle")
        form.n_points = self.widget(QtGui.QSpinBox, "NumPoints")
        form.var_draft = self.widget(QtGui.QCheckBox, "VariableDraft")
        form.var_trim = self.widget(QtGui.QCheckBox, "VariableTrim")

        angle = Units.Quantity(Locale.fromString(
            form.angle.text())).getValueAs('deg').Value
        n_points = form.n_points.value()
        var_draft = form.var_draft.isChecked()
        var_trim = form.var_trim.isChecked()

        props = self.ship.PropertiesList
        try:
            props.index("GZAngle")
        except ValueError:
            try:
                tooltip = str(QtGui.QApplication.translate(
                    "ship_gz",
                    "GZ curve tool angle selected [deg]",
                    None,
                    QtGui.QApplication.UnicodeUTF8))
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
                    "ship_areas",
                    "GZ curve tool number of points selected",
                    None,
                    QtGui.QApplication.UnicodeUTF8))
            except:
                tooltip = "GZ curve tool number of points selected"
            self.ship.addProperty("App::PropertyInteger",
                                  "GZNumPoints",
                                  "Ship",
                                  tooltip)
        self.ship.GZNumPoints = n_points
        try:
            props.index("GZVariableDraft")
        except ValueError:
            try:
                tooltip = str(QtGui.QApplication.translate(
                    "ship_areas",
                    "GZ curve tool variable draft selection",
                    None,
                    QtGui.QApplication.UnicodeUTF8))
            except:
                tooltip = "GZ curve tool variable draft selection"
            self.ship.addProperty("App::PropertyBool",
                                  "GZVariableDraft",
                                  "Ship",
                                  tooltip)
        self.ship.GZVariableDraft = var_draft
        try:
            props.index("GZVariableTrim")
        except ValueError:
            try:
                tooltip = str(QtGui.QApplication.translate(
                    "ship_areas",
                    "GZ curve tool variable trim angle selection",
                    None,
                    QtGui.QApplication.UnicodeUTF8))
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