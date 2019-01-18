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
from FreeCAD import Base, Vector
import Part
from FreeCAD import Units
from PySide import QtGui, QtCore
from . import PlotAux
import Instance
from shipUtils import Paths
import shipUtils.Units as USys
import shipUtils.Locale as Locale
from . import Tools


class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/shipHydrostatics/TaskPanel.ui"
        self.ship = None
        self.running = False

    def accept(self):
        if not self.ship:
            return False
        if self.running:
            return
        self.save()

        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.trim = self.widget(QtGui.QLineEdit, "Trim")
        form.minDraft = self.widget(QtGui.QLineEdit, "MinDraft")
        form.maxDraft = self.widget(QtGui.QLineEdit, "MaxDraft")
        form.nDraft = self.widget(QtGui.QSpinBox, "NDraft")

        trim = Units.parseQuantity(Locale.fromString(form.trim.text()))
        min_draft = Units.parseQuantity(Locale.fromString(form.minDraft.text()))
        max_draft = Units.parseQuantity(Locale.fromString(form.maxDraft.text()))
        n_draft = form.nDraft.value()

        draft = min_draft
        drafts = [draft]
        dDraft = (max_draft - min_draft) / (n_draft - 1)
        for i in range(1, n_draft):
            draft = draft + dDraft
            drafts.append(draft)

        # Get external faces
        self.loop = QtCore.QEventLoop()
        self.timer = QtCore.QTimer()
        self.timer.setSingleShot(True)
        QtCore.QObject.connect(self.timer,
                               QtCore.SIGNAL("timeout()"),
                               self.loop,
                               QtCore.SLOT("quit()"))
        self.running = True
        faces = self.externalFaces(self.ship.Shape)
        if not self.running:
            return False
        if len(faces) == 0:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Failure detecting external faces from the ship object",
                None)
            App.Console.PrintError(msg + '\n')
            return False
        faces = Part.makeShell(faces)

        # Get the hydrostatics
        msg = QtGui.QApplication.translate(
            "ship_console",
            "Computing hydrostatics",
            None)
        App.Console.PrintMessage(msg + '...\n')
        points = []
        for i in range(len(drafts)):
            App.Console.PrintMessage("\t{} / {}\n".format(i + 1, len(drafts)))
            draft = drafts[i]
            point = Tools.Point(self.ship,
                                faces,
                                draft,
                                trim)
            points.append(point)
            self.timer.start(0.0)
            self.loop.exec_()
            if(not self.running):
                break
        PlotAux.Plot(self.ship, trim, points)
        return True

    def reject(self):
        if not self.ship:
            return False
        if self.running:
            self.running = False
            return
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
        form.trim = self.widget(QtGui.QLineEdit, "Trim")
        form.minDraft = self.widget(QtGui.QLineEdit, "MinDraft")
        form.maxDraft = self.widget(QtGui.QLineEdit, "MaxDraft")
        form.nDraft = self.widget(QtGui.QSpinBox, "NDraft")
        self.form = form
        # Initial values
        if self.initValues():
            return True
        self.retranslateUi()
        # Connect Signals and Slots
        QtCore.QObject.connect(form.trim,
                               QtCore.SIGNAL("valueChanged(double)"),
                               self.onData)
        QtCore.QObject.connect(form.minDraft,
                               QtCore.SIGNAL("valueChanged(double)"),
                               self.onData)
        QtCore.QObject.connect(form.maxDraft,
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
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.trim = self.widget(QtGui.QLineEdit, "Trim")
        form.minDraft = self.widget(QtGui.QLineEdit, "MinDraft")
        form.maxDraft = self.widget(QtGui.QLineEdit, "MaxDraft")
        form.nDraft = self.widget(QtGui.QSpinBox, "NDraft")

        selObjs = Gui.Selection.getSelection()
        if not selObjs:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "A ship instance must be selected before using this tool (no"
                " objects selected)",
                None)
            App.Console.PrintError(msg + '\n')
            return True
        for i in range(len(selObjs)):
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

        props = self.ship.PropertiesList

        length_format = USys.getLengthFormat()
        angle_format = USys.getAngleFormat()

        try:
            props.index("HydrostaticsTrim")
            form.trim.setText(Locale.toString(angle_format.format(
                self.ship.HydrostaticsTrim.getValueAs(
                    USys.getLengthUnits()).Value)))
        except ValueError:
            form.trim.setText(Locale.toString(angle_format.format(0.0)))

        try:
            props.index("HydrostaticsMinDraft")
            form.minDraft.setText(Locale.toString(length_format.format(
                self.ship.HydrostaticsMinDraft.getValueAs(
                    USys.getLengthUnits()).Value)))
        except ValueError:
            form.minDraft.setText(Locale.toString(length_format.format(
                0.9 * self.ship.Draft.getValueAs(USys.getLengthUnits()).Value)))
        try:
            props.index("HydrostaticsMaxDraft")
            form.maxDraft.setText(Locale.toString(length_format.format(
                self.ship.HydrostaticsMaxDraft.getValueAs(
                    USys.getLengthUnits()).Value)))
        except ValueError:
            form.maxDraft.setText(Locale.toString(length_format.format(
                1.1 * self.ship.Draft.getValueAs(USys.getLengthUnits()).Value)))

        try:
            props.index("HydrostaticsNDraft")
            form.nDraft.setValue(self.ship.HydrostaticsNDraft)
        except ValueError:
            pass

        return False

    def retranslateUi(self):
        """ Set user interface locale strings.
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.setWindowTitle(QtGui.QApplication.translate(
            "ship_hydrostatic",
            "Plot hydrostatics",
            None))
        self.widget(QtGui.QLabel, "TrimLabel").setText(
            QtGui.QApplication.translate(
                "ship_hydrostatic",
                "Trim",
                None))
        self.widget(QtGui.QLabel, "MinDraftLabel").setText(
            QtGui.QApplication.translate(
                "ship_hydrostatic",
                "Minimum draft",
                None))
        self.widget(QtGui.QLabel, "MaxDraftLabel").setText(
            QtGui.QApplication.translate(
                "ship_hydrostatic",
                "Maximum draft",
                None))
        self.widget(QtGui.QLabel, "NDraftLabel").setText(
            QtGui.QApplication.translate(
                "ship_hydrostatic",
                "Number of points",
                None))

    def clampLength(self, widget, val_min, val_max, val):
        if val >= val_min and val <= val_max:
            return val
        input_format = USys.getLengthFormat()
        val = min(val_max, max(val_min, val))
        qty = Units.Quantity('{} m'.format(val))
        widget.setText(Locale.toString(input_format.format(
            qty.getValueAs(USys.getLengthUnits()).Value)))
        return val

    def clampAngle(self, widget, val_min, val_max, val):
        if val >= val_min and val <= val_max:
            return val
        input_format = USys.getAngleFormat()
        val = min(val_max, max(val_min, val))
        qty = Units.Quantity('{} deg'.format(val))
        widget.setText(Locale.toString(input_format.format(
            qty.getValueAs(USys.getLengthUnits()).Value)))
        return val

    def onData(self, value):
        """ Method called when input data is changed.
         @param value Changed value.
        """
        if not self.ship:
            return
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.trim = self.widget(QtGui.QLineEdit, "Trim")
        form.minDraft = self.widget(QtGui.QLineEdit, "MinDraft")
        form.maxDraft = self.widget(QtGui.QLineEdit, "MaxDraft")

        # Get the values (or fix them in bad setting case)
        try:
            trim = Units.Quantity(Locale.fromString(
                form.trim.text())).getValueAs('deg').Value
        except:
            trim = 0.0
            input_format = USys.getAngleFormat()
            qty = Units.Quantity('{} deg'.format(trim))
            form.trim.setText(Locale.toString(input_format.format(
                qty.getValueAs(USys.getLengthUnits()).Value)))
        try:
            min_draft = Units.Quantity(Locale.fromString(
                form.minDraft.text())).getValueAs('m').Value
        except:
            min_draft = 0.9 * self.ship.Draft.getValueAs('m').Value
            input_format = USys.getLengthFormat()
            qty = Units.Quantity('{} m'.format(min_draft))
            form.minDraft.setText(Locale.toString(input_format.format(
                qty.getValueAs(USys.getLengthUnits()).Value)))
        try:
            max_draft = Units.Quantity(Locale.fromString(
                form.minDraft.text())).getValueAs('m').Value
        except:
            max_draft = 0.9 * self.ship.Draft.getValueAs('m').Value
            input_format = USys.getLengthFormat()
            qty = Units.Quantity('{} m'.format(max_draft))
            form.maxDraft.setText(Locale.toString(input_format.format(
                qty.getValueAs(USys.getLengthUnits()).Value)))

        # Clamp the values to the bounds
        bbox = self.ship.Shape.BoundBox
        draft_min = bbox.ZMin / Units.Metre.Value
        draft_max = bbox.ZMax / Units.Metre.Value
        min_draft = self.clampLength(form.minDraft,
                                     draft_min,
                                     draft_max,
                                     min_draft)
        max_draft = self.clampLength(form.maxDraft,
                                     draft_min,
                                     draft_max,
                                     max_draft)
        trim_min = -180.0
        trim_max = 180.0
        trim = self.clampAngle(form.trim, trim_min, trim_max, trim)

        # Clamp draft values to assert that the minimum value is lower than
        # the maximum one
        min_draft = self.clampLength(form.minDraft,
                                     draft_min,
                                     max_draft,
                                     min_draft)
        max_draft = self.clampLength(form.maxDraft,
                                     min_draft,
                                     draft_max,
                                     max_draft)


    def save(self):
        """ Saves data into ship instance.
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.trim = self.widget(QtGui.QLineEdit, "Trim")
        form.minDraft = self.widget(QtGui.QLineEdit, "MinDraft")
        form.maxDraft = self.widget(QtGui.QLineEdit, "MaxDraft")
        form.nDraft = self.widget(QtGui.QSpinBox, "NDraft")

        trim = Units.Quantity(Locale.fromString(
            form.trim.text())).getValueAs('deg').Value
        min_draft = Units.Quantity(Locale.fromString(
            form.minDraft.text())).getValueAs('m').Value
        max_draft = Units.Quantity(Locale.fromString(
            form.maxDraft.text())).getValueAs('m').Value
        n_draft = form.nDraft.value()

        props = self.ship.PropertiesList
        try:
            props.index("HydrostaticsTrim")
        except ValueError:
            tooltip = str(QtGui.QApplication.translate(
                "ship_hydrostatic",
                "Hydrostatics tool trim selected",
                None))
            self.ship.addProperty("App::PropertyAngle",
                                  "HydrostaticsTrim",
                                  "Ship",
                                  tooltip)
        self.ship.HydrostaticsTrim = '{} deg'.format(trim)

        try:
            props.index("HydrostaticsMinDraft")
        except ValueError:
            tooltip = str(QtGui.QApplication.translate(
                "ship_hydrostatic",
                "Hydrostatics tool minimum draft selected [m]",
                None))
            self.ship.addProperty("App::PropertyLength",
                                  "HydrostaticsMinDraft",
                                  "Ship",
                                  tooltip)
        self.ship.HydrostaticsMinDraft = '{} m'.format(min_draft)

        try:
            props.index("HydrostaticsMaxDraft")
        except ValueError:
            tooltip = str(QtGui.QApplication.translate(
                "ship_hydrostatic",
                "Hydrostatics tool maximum draft selected [m]",
                None))
            self.ship.addProperty("App::PropertyLength",
                                  "HydrostaticsMaxDraft",
                                  "Ship",
                                  tooltip)
        self.ship.HydrostaticsMaxDraft = '{} m'.format(max_draft)

        try:
            props.index("HydrostaticsNDraft")
        except ValueError:
            tooltip = str(QtGui.QApplication.translate(
                "ship_hydrostatic",
                "Hydrostatics tool number of points selected",
                None))
            self.ship.addProperty("App::PropertyInteger",
                                  "HydrostaticsNDraft",
                                  "Ship",
                                  tooltip)
        self.ship.HydrostaticsNDraft = form.nDraft.value()

    def lineFaceSection(self, line, surface):
        """ Returns the point of section of a line with a face
        @param line Line object, that can be a curve.
        @param surface Surface object (must be a Part::Shape)
        @return Section points array, [] if line don't cut surface
        """
        result = []
        vertexes = line.Vertexes
        nVertex = len(vertexes)

        section = line.cut(surface)

        points = section.Vertexes
        return points

    def externalFaces(self, shape):
        """ Returns detected external faces.
        @param shape Shape where external faces wanted.
        @return List of external faces detected.
        """
        result = []
        faces = shape.Faces
        bbox = shape.BoundBox
        L = bbox.XMax - bbox.XMin
        B = bbox.YMax - bbox.YMin
        T = bbox.ZMax - bbox.ZMin
        dist = math.sqrt(L*L + B*B + T*T)
        msg = QtGui.QApplication.translate(
            "ship_console",
            "Computing external faces",
            None)
        App.Console.PrintMessage(msg + '...\n')
        # Valid/unvalid faces detection loop
        for i in range(len(faces)):
            App.Console.PrintMessage("\t{} / {}\n".format(i + 1, len(faces)))
            f = faces[i]
            # Create a line normal to surface at middle point
            u = 0.0
            v = 0.0
            try:
                surf = f.Surface
                u = 0.5*(surf.getUKnots()[0]+surf.getUKnots()[-1])
                v = 0.5*(surf.getVKnots()[0]+surf.getVKnots()[-1])
            except:
                cog = f.CenterOfMass
                [u, v] = f.Surface.parameter(cog)
            p0 = f.valueAt(u, v)
            try:
                n = f.normalAt(u, v).normalize()
            except:
                continue
            p1 = p0 + n.multiply(1.5 * dist)
            line = Part.makeLine(p0, p1)
            # Look for faces in front of this
            nPoints = 0
            for j in range(len(faces)):
                f2 = faces[j]
                section = self.lineFaceSection(line, f2)
                if len(section) <= 2:
                    continue
                # Add points discarding start and end
                nPoints = nPoints + len(section) - 2
            # In order to avoid special directions we can modify line
            # normal a little bit.
            angle = 5
            line.rotate(p0, Vector(1, 0, 0), angle)
            line.rotate(p0, Vector(0, 1, 0), angle)
            line.rotate(p0, Vector(0, 0, 1), angle)
            nPoints2 = 0
            for j in range(len(faces)):
                if i == j:
                    continue
                f2 = faces[j]
                section = self.lineFaceSection(line, f2)
                if len(section) <= 2:
                    continue
                # Add points discarding start and end
                nPoints2 = nPoints + len(section) - 2
            # If the number of intersection points is pair, is a
            # external face. So if we found an odd points intersection,
            # face must be discarded.
            if (nPoints % 2) or (nPoints2 % 2):
                continue
            result.append(f)
            self.timer.start(0.0)
            self.loop.exec_()
            if(not self.running):
                break
        return result


def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
