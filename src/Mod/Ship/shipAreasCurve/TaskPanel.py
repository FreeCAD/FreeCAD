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
from . import Preview
from . import PlotAux
import Instance
from shipUtils import Paths
import shipUtils.Units as USys
import shipUtils.Locale as Locale
from shipHydrostatics import Tools as Hydrostatics


class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/shipAreasCurve/TaskPanel.ui"
        self.preview = Preview.Preview()
        self.ship = None

    def accept(self):
        if not self.ship:
            return False
        self.save()
        # Plot data
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.draft = self.widget(QtGui.QLineEdit, "Draft")
        form.trim = self.widget(QtGui.QLineEdit, "Trim")
        form.num = self.widget(QtGui.QSpinBox, "Num")
        draft = Units.parseQuantity(Locale.fromString(form.draft.text()))
        trim = Units.parseQuantity(Locale.fromString(form.trim.text()))
        num = form.num.value()

        disp, B, _ = Hydrostatics.displacement(self.ship,
                                               draft,
                                               Units.parseQuantity("0 deg"),
                                               trim)
        xcb = Units.Quantity(B.x, Units.Length)
        data = Hydrostatics.areas(self.ship,
                                  num,
                                  draft=draft,
                                  trim=trim)
        x = []
        y = []
        for i in range(0, len(data)):
            x.append(data[i][0].getValueAs("m").Value)
            y.append(data[i][1].getValueAs("m^2").Value)
        PlotAux.Plot(x, y, disp, xcb, self.ship)
        self.preview.clean()
        return True

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

        form.draft = self.widget(QtGui.QLineEdit, "Draft")
        form.trim = self.widget(QtGui.QLineEdit, "Trim")
        form.num = self.widget(QtGui.QSpinBox, "Num")
        form.output = self.widget(QtGui.QTextEdit, "OutputData")
        form.doc = QtGui.QTextDocument(form.output)
        self.form = form
        if self.initValues():
            return True
        self.retranslateUi()
        QtCore.QObject.connect(form.draft,
                               QtCore.SIGNAL("valueChanged(double)"),
                               self.onData)
        QtCore.QObject.connect(form.trim,
                               QtCore.SIGNAL("valueChanged(double)"),
                               self.onData)

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
        selObjs = Gui.Selection.getSelection()
        if not selObjs:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "A ship instance must be selected before using this tool (no"
                " objects selected)",
                None)
            App.Console.PrintError(msg + '\n')
            return True
        for i in range(0, len(selObjs)):
            obj = selObjs[i]
            props = obj.PropertiesList
            try:
                props.index("IsShip")
            except ValueError:
                continue
            if obj.IsShip:
                if self.ship:
                    msg = QtGui.QApplication.translate(
                        "ship_console",
                        "More than one ship have been selected (the extra"
                        " ships will be ignored)",
                        None)
                    App.Console.PrintWarning(msg + '\n')
                    break
                self.ship = obj
        if not self.ship:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "A ship instance must be selected before using this tool (no"
                " valid ship found at the selected objects)",
                None)
            App.Console.PrintError(msg + '\n')
            return True

        length_format = USys.getLengthFormat()
        angle_format = USys.getAngleFormat()

        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.draft = self.widget(QtGui.QLineEdit, "Draft")
        form.trim = self.widget(QtGui.QLineEdit, "Trim")
        form.num = self.widget(QtGui.QSpinBox, "Num")
        form.draft.setText(Locale.toString(length_format.format(
            self.ship.Draft.getValueAs(USys.getLengthUnits()).Value)))
        form.trim.setText(Locale.toString(angle_format.format(0.0)))
        # Try to use saved values
        props = self.ship.PropertiesList
        try:
            props.index("AreaCurveDraft")
            form.draft.setText(Locale.toString(length_format.format(
                self.ship.AreaCurveDraft.getValueAs(
                    USys.getLengthUnits()).Value)))
        except:
            pass
        try:
            props.index("AreaCurveTrim")
            form.trim.setText(Locale.toString(angle_format.format(
                self.ship.AreaCurveTrim.getValueAs(
                    USys.getAngleUnits()).Value)))
        except ValueError:
            pass
        try:
            props.index("AreaCurveNum")
            form.num.setValue(self.ship.AreaCurveNum)
        except ValueError:
            pass
        # Update GUI
        draft = Units.Quantity(form.draft.text()).getValueAs('m').Value
        trim = Units.Quantity(form.trim.text()).getValueAs('deg').Value
        self.preview.update(draft, trim, self.ship)
        self.onUpdate()
        return False

    def retranslateUi(self):
        """ Set user interface locale strings. """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.setWindowTitle(QtGui.QApplication.translate(
            "ship_areas",
            "Plot the transversal areas curve",
            None))
        self.widget(QtGui.QLabel, "DraftLabel").setText(
            QtGui.QApplication.translate(
                "ship_areas",
                "Draft",
                None))
        self.widget(QtGui.QLabel, "TrimLabel").setText(
            QtGui.QApplication.translate(
                "ship_areas",
                "Trim angle",
                None))
        self.widget(QtGui.QLabel, "NumLabel").setText(
            QtGui.QApplication.translate(
                "ship_areas",
                "Number of points",
                None))

    def clampValue(self, widget, val_min, val_max, val):
        if val_min <= val <= val_max:
            return val
        val = min(val_max, max(val_min, val))
        widget.setText(val.UserString)
        return val

    def onData(self, value):
        """ Method called when the tool input data is touched.
        @param value Changed value.
        """
        if not self.ship:
            return

        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.draft = self.widget(QtGui.QLineEdit, "Draft")
        form.trim = self.widget(QtGui.QLineEdit, "Trim")

        # Get the values (or fix them in bad setting case)
        try:
            draft = Units.parseQuantity(Locale.fromString(form.draft.text()))
        except:
            draft = self.ship.Draft
            form.draft.setText(draft.UserString)
        try:
            trim = Units.parseQuantity(Locale.fromString(form.trim.text()))
        except:
            trim = Units.parseQuantity("0 deg")
            form.trim.setText(trim.UserString)

        bbox = self.ship.Shape.BoundBox
        draft_min = Units.Quantity(bbox.ZMin, Units.Length)
        draft_max = Units.Quantity(bbox.ZMax, Units.Length)
        draft = self.clampValue(form.draft, draft_min, draft_max, draft)

        trim_min = Units.parseQuantity("-180 deg")
        trim_max = Units.parseQuantity("180 deg")
        trim = self.clampValue(form.trim, trim_min, trim_max, trim)

        self.onUpdate()
        self.preview.update(draft, trim, self.ship)

    def onUpdate(self):
        """ Method called when the data update is requested. """
        if not self.ship:
            return
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.draft = self.widget(QtGui.QLineEdit, "Draft")
        form.trim = self.widget(QtGui.QLineEdit, "Trim")
        form.output = self.widget(QtGui.QTextEdit, "OutputData")

        draft = Units.parseQuantity(Locale.fromString(form.draft.text()))
        trim = Units.parseQuantity(Locale.fromString(form.trim.text()))

        # Calculate the drafts at each perpendicular
        angle = trim.getValueAs("rad").Value
        L = self.ship.Length.getValueAs('m').Value
        B = self.ship.Breadth.getValueAs('m').Value
        draftAP = draft + 0.5 * self.ship.Length * math.tan(angle)
        if draftAP < 0.0:
            draftAP = 0.0
        draftFP = draft - 0.5 * self.ship.Length * math.tan(angle)
        if draftFP < 0.0:
            draftFP = 0.0
        # Calculate the involved hydrostatics
        disp, B, _ = Hydrostatics.displacement(self.ship,
                                               draft,
                                               Units.parseQuantity("0 deg"),
                                               trim)
        xcb = Units.Quantity(B.x, Units.Length)
        # Setup the html string
        string = u'L = {0}<BR>'.format(self.ship.Length.UserString)
        string += u'B = {0}<BR>'.format(self.ship.Breadth.UserString)
        string += u'T = {0}<HR>'.format(draft.UserString)
        string += u'Trim = {0}<BR>'.format(trim.UserString)
        string += u'T<sub>AP</sub> = {0}<BR>'.format(draftAP.UserString)
        string += u'T<sub>FP</sub> = {0}<HR>'.format(draftFP.UserString)
        dispText = QtGui.QApplication.translate(
            "ship_areas",
            'Displacement',
            None)
        string += dispText + u' = {0}<BR>'.format(disp.UserString)
        string += u'XCB = {0}'.format(xcb.UserString)
        form.output.setHtml(string)

    def save(self):
        """ Saves the data into ship instance. """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.draft = self.widget(QtGui.QLineEdit, "Draft")
        form.trim = self.widget(QtGui.QLineEdit, "Trim")
        form.num = self.widget(QtGui.QSpinBox, "Num")

        draft = Units.parseQuantity(Locale.fromString(form.draft.text()))
        trim = Units.parseQuantity(Locale.fromString(form.trim.text()))
        num = form.num.value()

        props = self.ship.PropertiesList
        try:
            props.index("AreaCurveDraft")
        except ValueError:
            try:
                tooltip = str(QtGui.QApplication.translate(
                    "ship_areas",
                    "Areas curve tool draft selected [m]",
                    None))
            except:
                tooltip = "Areas curve tool draft selected [m]"
            self.ship.addProperty("App::PropertyLength",
                                  "AreaCurveDraft",
                                  "Ship",
                                  tooltip)
        self.ship.AreaCurveDraft = draft
        try:
            props.index("AreaCurveTrim")
        except ValueError:
            try:
                tooltip = str(QtGui.QApplication.translate(
                    "ship_areas",
                    "Areas curve tool trim selected [deg]",
                    None))
            except:
                tooltip = "Areas curve tool trim selected [deg]"
            self.ship.addProperty("App::PropertyAngle",
                                  "AreaCurveTrim",
                                  "Ship",
                                  tooltip)
        self.ship.AreaCurveTrim = trim
        try:
            props.index("AreaCurveNum")
        except ValueError:
            try:
                tooltip = str(QtGui.QApplication.translate(
                    "ship_areas",
                    "Areas curve tool number of points",
                    None))
            except:
                tooltip = "Areas curve tool number of points"
            self.ship.addProperty("App::PropertyInteger",
                                  "AreaCurveNum",
                                  "Ship",
                                  tooltip)
        self.ship.AreaCurveNum = num


def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
