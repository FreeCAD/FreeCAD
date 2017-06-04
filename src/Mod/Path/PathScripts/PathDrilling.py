# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

from __future__ import print_function
import sys
import FreeCAD
import Path
import PathScripts.PathLog as PathLog
from PySide import QtCore, QtGui
from PathScripts import PathUtils
from PathScripts.PathUtils import fmt, waiting_effects
import ArchPanel


# xrange is not available in python3
if sys.version_info.major >= 3:
    xrange = range

LOG_MODULE = 'PathDrilling'
PathLog.setLevel(PathLog.Level.INFO, LOG_MODULE)
PathLog.trackModule('PathDrilling')

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Drilling object and FreeCAD command"""

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectDrilling:

    def __init__(self, obj):
        # Properties of the holes
        obj.addProperty("App::PropertyStringList", "Names", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Names of the holes"))
        obj.addProperty("App::PropertyVectorList", "Positions", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Locations of insterted holes"))
        obj.addProperty("App::PropertyIntegerList", "Enabled", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable/disable status of the holes"))
        obj.addProperty("App::PropertyFloatList", "Diameters", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Diameters of the holes"))

        # General Properties
        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "An optional comment for this profile"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "User Assigned Label"))

        # Drilling Properties
        obj.addProperty("App::PropertyLength", "PeckDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Incremental Drill depth before retracting to clear chips"))
        obj.addProperty("App::PropertyBool", "PeckEnabled", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable pecking"))
        obj.addProperty("App::PropertyLength", "StartDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyFloat", "DwellTime", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The time to dwell between peck cycles"))
        obj.addProperty("App::PropertyBool", "DwellEnabled", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable dwell"))

        # Heights & Depths
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Height to clear top of material"))
        obj.addProperty("App::PropertyDistance", "RetractHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height where feed starts and height during retract tool when path is finished"))

        # Tool Properties
        obj.addProperty("App::PropertyLink", "ToolController", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))

        obj.Proxy = self
        self.vertFeed = 0.0
        self.horizFeed = 0.0
        self.vertRapid = 0.0
        self.horizRapid = 0.0

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):
        pass

    @waiting_effects
    def execute(self, obj):
        PathLog.track()
        output = ""
        if obj.Comment != "":
            output += '(' + str(obj.Comment)+')\n'

        toolLoad = obj.ToolController
        if toolLoad is None or toolLoad.ToolNumber == 0:
            FreeCAD.Console.PrintError("No Tool Controller is selected. We need a tool to build a Path.")
            return
        else:
            self.vertFeed = toolLoad.VertFeed.Value
            self.horizFeed = toolLoad.HorizFeed.Value
            self.vertRapid = toolLoad.VertRapid.Value
            self.horizRapid = toolLoad.HorizRapid.Value
            tool = toolLoad.Proxy.getTool(toolLoad)
            if not tool or tool.Diameter == 0:
                FreeCAD.Console.PrintError("No Tool found or diameter is zero. We need a tool to build a Path.")
                return
            else:
                self.radius = tool.Diameter/2

        if len(obj.Names) == 0:
            parentJob = PathUtils.findParentJob(obj)
            if parentJob is None:
                return
            baseobject = parentJob.Base
            if baseobject is None:
                return

            # Arch PanelSheet
            if hasattr(baseobject, "Proxy"):
                holes = []
                if isinstance(baseobject.Proxy, ArchPanel.PanelSheet):
                    baseobject.Proxy.execute(baseobject)
                    i = 0
                    holeshapes = baseobject.Proxy.getHoles(baseobject, transform=True)
                    tooldiameter = obj.ToolController.Proxy.getTool(obj.ToolController).Diameter
                    for holeshape in holeshapes:
                        PathLog.debug('Entering new HoleShape')
                        for wire in holeshape.Wires:
                            PathLog.debug('Entering new Wire')
                            for edge in wire.Edges:
                                if PathUtils.isDrillable(baseobject, edge, tooldiameter):
                                    PathLog.debug('Found drillable hole edges: {}'.format(edge))
                                    x = edge.Curve.Center.x
                                    y = edge.Curve.Center.y
                                    diameter = edge.BoundBox.XLength
                                    holes.append({'x': x, 'y': y, 'featureName': baseobject.Name+'.'+'Drill'+str(i), 'd': diameter})
                                    i = i + 1
            else:
                holes = self.findHoles(obj, baseobject.Shape)
                for i in range(len(holes)):
                    holes[i]['featureName'] = baseobject.Name + '.' + holes[i]['featureName']
            names = []
            positions = []
            enabled = []
            diameters = []
            for h in holes:
                if len(names) == 0:
                    self.findHeights(obj, baseobject, h)
                names.append(h['featureName'])
                positions.append(FreeCAD.Vector(h['x'], h['y'], 0))
                enabled.append(1)
                diameters.append(h['d'])
            obj.Names = names
            obj.Positions = positions
            obj.Enabled = enabled
            obj.Diameters = diameters

        locations = []
        output = "(Begin Drilling)\n"

        for i in range(len(obj.Names)):
            if obj.Enabled[i] > 0:
                locations.append({'x': obj.Positions[i].x, 'y': obj.Positions[i].y})
        if len(locations) > 0:
            locations = PathUtils.sort_jobs(locations, ['x', 'y'])
            output += "G90 G98\n"
            # rapid to clearance height
            output += "G0 Z" + str(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"
            # rapid to first hole location, with spindle still retracted:

            p0 = locations[0]
            output += "G0 X" + fmt(p0['x']) + " Y" + fmt(p0['y']) + "F " + PathUtils.fmt(self.horizRapid) + "\n"
            # move tool to clearance plane
            output += "G0 Z" + fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"
            pword = ""
            qword = ""
            if obj.PeckDepth.Value > 0 and obj.PeckEnabled:
                cmd = "G83"
                qword = " Q" + fmt(obj.PeckDepth.Value)
            elif obj.DwellTime > 0 and obj.DwellEnabled:
                cmd = "G82"
                pword = " P" + fmt(obj.DwellTime)
            else:
                cmd = "G81"
            for p in locations:
                output += cmd + \
                    " X" + fmt(p['x']) + \
                    " Y" + fmt(p['y']) + \
                    " Z" + fmt(obj.FinalDepth.Value) + qword + pword + \
                    " R" + str(obj.RetractHeight.Value) + \
                    " F" + str(self.vertFeed) + "\n" \

            output += "G80\n"

        if obj.Active:
            path = Path.Path(output)
            obj.Path = path
            obj.ViewObject.Visibility = True

        else:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False

    def findHeights(self, obj, bobj, hole):
        try:
            bb = bobj.Shape.BoundBox
            subobj = hole['feature']
            fbb = subobj.BoundBox
            obj.StartDepth = bb.ZMax
            obj.ClearanceHeight = bb.ZMax + 5.0
            obj.SafeHeight = bb.ZMax + 3.0
            obj.RetractHeight = bb.ZMax + 1.0

            if fbb.ZMax < bb.ZMax:
                obj.FinalDepth = fbb.ZMax
            else:
                obj.FinalDepth = bb.ZMin
        except:
            obj.StartDepth = 5.0
            obj.ClearanceHeight = 10.0
            obj.SafeHeight = 8.0
            obj.RetractHeight = 6.0

    def findHoles(self, obj, shape):
        import DraftGeomUtils as dgu
        PathLog.track('obj: {} shape: {}'.format(obj, shape))
        holelist = []
        tooldiameter = obj.ToolController.Proxy.getTool(obj.ToolController).Diameter
        PathLog.debug('search for holes larger than tooldiameter: {}: '.format(tooldiameter))
        if dgu.isPlanar(shape):
            PathLog.debug("shape is planar")
            for i in range(len(shape.Edges)):
                candidateEdgeName = "Edge" + str(i + 1)
                e = shape.getElement(candidateEdgeName)
                if PathUtils.isDrillable(shape, e, tooldiameter):
                    PathLog.debug('edge candidate: {} (hash {})is drillable '.format(e, e.hashCode()))
                    x = e.Curve.Center.x
                    y = e.Curve.Center.y
                    diameter = e.BoundBox.XLength
                    holelist.append({'featureName': candidateEdgeName, 'feature': e, 'x': x, 'y': y, 'd': diameter, 'enabled': True})
        else:
            PathLog.debug("shape is not planar")
            for i in range(len(shape.Faces)):
                candidateFaceName = "Face" + str(i + 1)
                f = shape.getElement(candidateFaceName)
                if PathUtils.isDrillable(shape, f, tooldiameter):
                    PathLog.debug('face candidate: {} is drillable '.format(f))
                    x = f.Surface.Center.x
                    y = f.Surface.Center.y
                    diameter = f.BoundBox.XLength
                    holelist.append({'featureName': candidateFaceName, 'feature': f, 'x': x, 'y': y, 'd': diameter, 'enabled': True})

        PathLog.debug("holes found: {}".format(holelist))
        return holelist


class _ViewProviderDrill:
    def __init__(self, obj):
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def getIcon(self):
        return ":/icons/Path-Drilling.svg"

    def onChanged(self, obj, prop):
        # this is executed when a property of the VIEW PROVIDER changes
        pass

    def updateData(self, obj, prop):
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel()
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        return True

    def unsetEdit(self, vobj, mode):
        # this is executed when the user cancels or terminates edit mode
        pass


class CommandPathDrilling:

    def GetResources(self):
        return {'Pixmap': 'Path-Drilling',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Drilling", "Drilling"),
                'Accel': "P, D",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Drilling", "Creates a Path Drilling object")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):

        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Drilling", "Create Drilling"))
        FreeCADGui.addModule("PathScripts.PathDrilling")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Drilling")')
        FreeCADGui.doCommand('PathScripts.PathDrilling.ObjectDrilling(obj)')
        FreeCADGui.doCommand('obj.Active = True')
        FreeCADGui.doCommand('PathScripts.PathDrilling._ViewProviderDrill(obj.ViewObject)')

        ztop = 10.0
        zbottom = 0.0
        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(ztop))
        FreeCADGui.doCommand('obj.RetractHeight= ' + str(ztop))
        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('obj.ToolController = PathScripts.PathUtils.findToolController(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class TaskPanel:
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/DrillingEdit.ui")

    def accept(self):
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        # FreeCADGui.Selection.removeObserver(self.s)

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        # FreeCADGui.Selection.removeObserver(self.s)

    def getFields(self):
        PathLog.track()
        if self.obj:
            try:
                if hasattr(self.obj, "StartDepth"):
                    self.obj.StartDepth = FreeCAD.Units.Quantity(self.form.startDepth.text()).Value
                if hasattr(self.obj, "FinalDepth"):
                    self.obj.FinalDepth = FreeCAD.Units.Quantity(self.form.finalDepth.text()).Value
                if hasattr(self.obj, "PeckDepth"):
                    if FreeCAD.Units.Quantity(self.form.peckDepth.text()).Value >= 0:
                        self.obj.PeckDepth = FreeCAD.Units.Quantity(self.form.peckDepth.text()).Value
                    else:
                        self.form.peckDepth.setText("0.00")
                if hasattr(self.obj, "SafeHeight"):
                    self.obj.SafeHeight = FreeCAD.Units.Quantity(self.form.safeHeight.text()).Value
                if hasattr(self.obj, "ClearanceHeight"):
                    self.obj.ClearanceHeight = FreeCAD.Units.Quantity(self.form.clearanceHeight.text()).Value
                if hasattr(self.obj, "RetractHeight"):
                    self.obj.RetractHeight = FreeCAD.Units.Quantity(self.form.retractHeight.text()).Value
                if hasattr(self.obj, "DwellTime"):
                    if FreeCAD.Units.Quantity(self.form.dwellTime.text()).Value >= 0:
                        self.obj.DwellTime = FreeCAD.Units.Quantity(self.form.dwellTime.text()).Value
                    else:
                        self.form.dwellTime.setText("0.00")

                if hasattr(self.obj, "DwellEnabled"):
                    self.obj.DwellEnabled = self.form.dwellEnabled.isChecked()
                if hasattr(self.obj, "PeckEnabled"):
                    self.obj.PeckEnabled = self.form.peckEnabled.isChecked()

                if hasattr(self.obj, "ToolController"):
                    PathLog.debug("name: {}".format(self.form.uiToolController.currentText()))
                    tc = PathUtils.findToolController(self.obj, self.form.uiToolController.currentText())
                    self.obj.ToolController = tc
            except ValueError:
                self.setFields()
        self.obj.Proxy.execute(self.obj)

    def updateFeatureList(self):

        self.form.baseList.itemChanged.disconnect(self.checkedChanged)  # disconnect this slot while creating objects
        self.form.baseList.clear()
        self.form.baseList.setColumnCount(2)
        self.form.baseList.setRowCount(0)
        self.form.baseList.setHorizontalHeaderLabels(["Feature", "Diameter"])
        self.form.baseList.horizontalHeader().setStretchLastSection(True)
        self.form.baseList.resizeColumnToContents(0)
        self.form.baseList.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        for i in range(len(self.obj.Names)):
            self.form.baseList.insertRow(self.form.baseList.rowCount())
            item = QtGui.QTableWidgetItem(self.obj.Names[i])
            item.setFlags(item.flags() | QtCore.Qt.ItemIsUserCheckable)

            if self.obj.Enabled[i] > 0:
                item.setCheckState(QtCore.Qt.Checked)
            else:
                item.setCheckState(QtCore.Qt.Unchecked)
            self.form.baseList.setItem(self.form.baseList.rowCount()-1, 0, item)
            item = QtGui.QTableWidgetItem("{:.3f}".format(self.obj.Diameters[i]))

            self.form.baseList.setItem(self.form.baseList.rowCount()-1, 1, item)
        self.form.baseList.resizeColumnToContents(0)
        self.form.baseList.itemChanged.connect(self.checkedChanged)

        self.form.baseList.setSortingEnabled(True)

    def setFields(self):
        PathLog.track()
        self.form.startDepth.setText(FreeCAD.Units.Quantity(self.obj.StartDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.finalDepth.setText(FreeCAD.Units.Quantity(self.obj.FinalDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.peckDepth.setText(FreeCAD.Units.Quantity(self.obj.PeckDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.safeHeight.setText(FreeCAD.Units.Quantity(self.obj.SafeHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.clearanceHeight.setText(FreeCAD.Units.Quantity(self.obj.ClearanceHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.retractHeight.setText(FreeCAD.Units.Quantity(self.obj.RetractHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.dwellTime.setText(str(self.obj.DwellTime))

        if self.obj.DwellEnabled:
            self.form.dwellEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.dwellEnabled.setCheckState(QtCore.Qt.Unchecked)

        if self.obj.PeckEnabled:
            self.form.peckEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.peckEnabled.setCheckState(QtCore.Qt.Unchecked)

        self.updateFeatureList()

        controllers = PathUtils.getToolControllers(self.obj)
        labels = [c.Label for c in controllers]
        self.form.uiToolController.blockSignals(True)
        self.form.uiToolController.addItems(labels)
        self.form.uiToolController.blockSignals(False)

        if self.obj.ToolController is not None:
            index = self.form.uiToolController.findText(
                self.obj.ToolController.Label, QtCore.Qt.MatchFixedString)
            PathLog.debug("searching for TC label {}. Found Index: {}".format(self.obj.ToolController.Label, index))
            if index >= 0:
                self.form.uiToolController.blockSignals(True)
                self.form.uiToolController.setCurrentIndex(index)
                self.form.uiToolController.blockSignals(False)

    def open(self):
        """ """
        # self.s = SelObserver()
        # FreeCADGui.Selection.addObserver(self.s)

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        slist = self.form.baseList.selectedItems()
        # parentJob = PathUtils.findParentJob(self.obj)
        for i in slist:
            if i.column() == 0:
                objstring = i.text().partition(".")
                obj = FreeCAD.ActiveDocument.getObject(objstring[0])
                if obj is not None:
                    if objstring[2] != "":
                        FreeCADGui.Selection.addSelection(obj, objstring[2])
                    else:
                        FreeCADGui.Selection.addSelection(obj)

        FreeCADGui.updateGui()

    def checkedChanged(self):
        enabledlist = self.obj.Enabled

        for i in xrange(0, self.form.baseList.rowCount()):
            try:
                ind = self.obj.Names.index(self.form.baseList.item(i, 0).text())
                if self.form.baseList.item(i, 0).checkState() == QtCore.Qt.Checked:
                    enabledlist[ind] = 1
                else:
                    enabledlist[ind] = 0
            except:
                PathLog.track("Not found:"+self.form.baseList.item(i, 0).text() + " in " + str(self.obj.Names))

        self.obj.Enabled = enabledlist
        FreeCAD.ActiveDocument.recompute()

    def enableAll(self):
        for i in xrange(0, self.form.baseList.rowCount()):
            self.form.baseList.item(i, 0).setCheckState(QtCore.Qt.Checked)

    def enableSelected(self):
        slist = self.form.baseList.selectedItems()
        for i in slist:
            r = i.row()
            self.form.baseList.item(r, 0).setCheckState(QtCore.Qt.Checked)

    def disableAll(self):
        for i in xrange(0, self.form.baseList.rowCount()):
            self.form.baseList.item(i, 0).setCheckState(QtCore.Qt.Unchecked)

    def disableSelected(self):
        slist = self.form.baseList.selectedItems()
        for i in slist:
            r = i.row()
            self.form.baseList.item(r, 0).setCheckState(QtCore.Qt.Unchecked)

    def findAll(self):
        """ Reset the list of features by running the findHoles again """
        self.obj.Names = []
        self.obj.Diameters = []
        self.obj.Enabled = []
        self.obj.Positions = []

        self.obj.Proxy.execute(self.obj)

        self.updateFeatureList()
        FreeCAD.ActiveDocument.recompute()

    def addSelected(self):
        for sel in FreeCAD.Gui.Selection.getSelectionEx():

            names = self.obj.Names
            positions = self.obj.Positions
            enabled = self.obj.Enabled
            diameters = self.obj.Diameters

            objectname = sel.ObjectName
            sobj = sel.Object
            for i, sub in enumerate(sel.SubObjects):
                if hasattr(sub, 'ShapeType'):
                    if sub.ShapeType == 'Vertex':
                        PathLog.debug("Selection is a vertex, lets drill that")
                        names.append(objectname+'.'+sel.SubElementNames[i])
                        positions.append(FreeCAD.Vector(sub.X, sub.Y, 0))
                        enabled.append(1)
                        diameters.append(0)

                    elif sub.ShapeType == 'Edge':
                        if PathUtils.isDrillable(sobj, sub):
                            PathLog.debug("Selection is a drillable edge, lets drill that")
                            names.append(objectname+'.'+sel.SubElementNames[i])
                            positions.append(FreeCAD.Vector(sub.Curve.Center.x, sub.Curve.Center.y, 0))
                            enabled.append(1)
                            diameters.append(sub.BoundBox.XLength)
                    elif sub.ShapeType == 'Face':
                        if PathUtils.isDrillable(sobj.Shape, sub):
                            PathLog.debug("Selection is a drillable face, lets drill that")
                            names.append(objectname+'.'+sel.SubElementNames[i])
                            positions.append(FreeCAD.Vector(sub.Surface.Center.x, sub.Surface.Center.y, 0))
                            enabled.append(1)
                            diameters.append(sub.BoundBox.XLength)

            self.obj.Names = names
            self.obj.Positions = positions
            self.obj.Enabled = enabled
            self.obj.Diameters = diameters

            self.updateFeatureList()

            FreeCAD.ActiveDocument.recompute()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def setupUi(self):
        PathLog.track()

        # Connect Signals and Slots
        self.form.startDepth.editingFinished.connect(self.getFields)
        self.form.finalDepth.editingFinished.connect(self.getFields)
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.retractHeight.editingFinished.connect(self.getFields)
        self.form.peckDepth.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)
        self.form.dwellTime.editingFinished.connect(self.getFields)
        self.form.dwellEnabled.stateChanged.connect(self.getFields)
        self.form.peckEnabled.stateChanged.connect(self.getFields)

        # buttons
        self.form.uiEnableSelected.clicked.connect(self.enableSelected)
        self.form.uiDisableSelected.clicked.connect(self.disableSelected)
        self.form.uiFindAllHoles.clicked.connect(self.findAll)
        self.form.uiAddSelected.clicked.connect(self.addSelected)

        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.form.baseList.itemChanged.connect(self.checkedChanged)

        self.form.uiToolController.currentIndexChanged.connect(self.getFields)

        self.setFields()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Drilling', CommandPathDrilling())

FreeCAD.Console.PrintLog("Loading PathDrilling... done\n")
