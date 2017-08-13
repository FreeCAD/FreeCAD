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
import Part
from pivy import coin
import Draft

# xrange is not available in python3
if sys.version_info.major >= 3:
    xrange = range

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

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
        obj.addProperty("App::PropertyBool", "AddTipLength", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Calculate the tip length and subtract from final depth"))
        obj.addProperty("App::PropertyEnumeration", "ReturnLevel", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Controls how tool retracts Default=G98"))
        obj.ReturnLevel = ['G98', 'G99']  # this is the direction that the Contour runs

        # Heights & Depths
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Height to clear top of material"))
        obj.addProperty("App::PropertyDistance", "RetractHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height where feed starts and height during retract tool when path is finished"))

        # Tool Properties
        obj.addProperty("App::PropertyLink", "ToolController", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))

        if FreeCAD.GuiUp:
            _ViewProviderDrill(obj.ViewObject)

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

        if not obj.Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False
            return

        output = ""
        if obj.Comment != "":
            output += '(' + str(obj.Comment) + ')\n'

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
                self.radius = tool.Diameter / 2

        tiplength = 0.0
        if obj.AddTipLength:
            tiplength = PathUtils.drillTipLength(tool)

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
                                    holes.append({'x': x, 'y': y, 'featureName': baseobject.Name + '.' + 'Drill' + str(i), 'd': diameter})
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
                    self.setDepths(obj, baseobject, h)
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
            output += "G90 " + obj.ReturnLevel + "\n"
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
                    " Z" + fmt(obj.FinalDepth.Value + tiplength) + qword + pword + \
                    " R" + str(obj.RetractHeight.Value) + \
                    " F" + str(self.vertFeed) + "\n" \

            output += "G80\n"

        path = Path.Path(output)
        obj.Path = path

    def setDepths(self, obj, bobj, hole):
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
        # tooldiameter = obj.ToolController.Proxy.getTool(obj.ToolController).Diameter
        tooldiameter = None
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

    def deleteObjectsOnReject(self):
        return hasattr(self, 'deleteOnReject') and self.deleteOnReject

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel(vobj.Object, self.deleteObjectsOnReject())
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        self.deleteOnReject = False
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
        ztop = 10.0
        zbottom = 0.0

        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Drilling", "Create Drilling"))
        FreeCADGui.addModule("PathScripts.PathDrilling")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Drilling")')
        FreeCADGui.doCommand('PathScripts.PathDrilling.ObjectDrilling(obj)')
        FreeCADGui.doCommand('obj.Active = True')
        FreeCADGui.doCommand('obj.ViewObject.Proxy.deleteOnReject = True')

        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(ztop))
        FreeCADGui.doCommand('obj.RetractHeight= ' + str(ztop))
        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('obj.ToolController = PathScripts.PathUtils.findToolController(obj)')
        # FreeCADGui.doCommand('PathScripts.PathDrilling.ObjectDrilling.setDepths(obj.Proxy, obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class CustomPoint:
    def __init__(self, obj, formOrig, formPoint, whenDone):
        self.formOrig = formOrig
        self.formPoint = formPoint
        self.obj = obj

        self.setupUi()
        self.whenDone = whenDone

    def setupUi(self):
        self.formPoint.buttonBox.accepted.connect(self.pointAccept)
        self.formPoint.buttonBox.rejected.connect(self.pointReject)

        self.formPoint.ifValueX.editingFinished.connect(self.updatePoint)
        self.formPoint.ifValueY.editingFinished.connect(self.updatePoint)
        self.formPoint.ifValueZ.editingFinished.connect(self.updatePoint)

    def addEscapeShortcut(self):
        # The only way I could get to intercept the escape key, or really any key was
        # by creating an action with a shortcut .....
        self.escape = QtGui.QAction(self.formPoint)
        self.escape.setText('Done')
        self.escape.setShortcut(QtGui.QKeySequence.fromString('Esc'))
        QtCore.QObject.connect(self.escape, QtCore.SIGNAL('triggered()'), self.pointDone)
        self.formPoint.addAction(self.escape)

    def removeEscapeShortcut(self):
        if self.escape:
            self.formPoint.removeAction(self.escape)
            self.escape = None

    def getPoint(self, whenDone, start=None):

        def displayPoint(p):
            self.formPoint.ifValueX.setText(FreeCAD.Units.Quantity(p.x, FreeCAD.Units.Length).UserString)
            self.formPoint.ifValueY.setText(FreeCAD.Units.Quantity(p.y, FreeCAD.Units.Length).UserString)
            self.formPoint.ifValueZ.setText(FreeCAD.Units.Quantity(p.z, FreeCAD.Units.Length).UserString)
            self.formPoint.ifValueX.setFocus()
            self.formPoint.ifValueX.selectAll()

        def mouseMove(cb):
            event = cb.getEvent()
            pos = event.getPosition()
            cntrl = event.wasCtrlDown()
            shift = event.wasShiftDown()
            self.pt = FreeCADGui.Snapper.snap(pos, lastpoint=start, active=cntrl, constrain=shift)
            plane = FreeCAD.DraftWorkingPlane
            p = plane.getLocalCoords(self.pt)
            displayPoint(p)

        def click(cb):
            event = cb.getEvent()
            if event.getButton() == 1 and event.getState() == coin.SoMouseButtonEvent.DOWN:
                accept()

        def accept():
            if start:
                self.pointAccept()
            else:
                self.pointAcceptAndContinue()

        def cancel():
            self.pointCancel()

        self.pointWhenDone = whenDone
        self.formOrig.hide()
        self.formPoint.show()
        self.addEscapeShortcut()
        if start:
            displayPoint(start)
        else:
            displayPoint(FreeCAD.Vector(0, 0, 0))

        self.view = Draft.get3DView()
        self.pointCbClick = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), click)
        self.pointCbMove = self.view.addEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), mouseMove)

        # self.buttonBox.setEnabled(False)

    def addCustom(self):
        self.getPoint(whenDone=self.whenDone, start=FreeCAD.Vector(0, 0, 0))

    def pointFinish(self, ok, cleanup=True):
        obj = FreeCADGui.Snapper.lastSnappedObject

        if cleanup:
            self.removeGlobalCallbacks()
            FreeCADGui.Snapper.off()
            #self.buttonBox.setEnabled(True)
            self.removeEscapeShortcut()
            self.formPoint.hide()
            self.formOrig.show()
            self.formOrig.setFocus()

        if ok:
            self.pointWhenDone(self.pt, obj)
        else:
            self.pointWhenDone(None, None)

    def pointDone(self):
        self.pointFinish(False)

    def pointReject(self):
        self.pointFinish(False)

    def pointAccept(self):
        self.pointFinish(True)

    def pointAcceptAndContinue(self):
        self.pointFinish(True, False)

    def removeGlobalCallbacks(self):
        if hasattr(self, 'view') and self.view:
            if self.pointCbClick:
                self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), self.pointCbClick)
                self.pointCbClick = None
            if self.pointCbMove:
                self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), self.pointCbMove)
                self.pointCbMove = None
            self.view = None

    def updatePoint(self):
        x = FreeCAD.Units.Quantity(self.formPoint.ifValueX.text()).Value
        y = FreeCAD.Units.Quantity(self.formPoint.ifValueY.text()).Value
        z = FreeCAD.Units.Quantity(self.formPoint.ifValueZ.text()).Value
        self.pt = FreeCAD.Vector(x, y, z)


class TaskPanel:
    def __init__(self, obj, deleteOnReject):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Drilling", "Drilling Operation"))
        self.form = QtGui.QWidget()
        self.formDrill = FreeCADGui.PySideUic.loadUi(":/panels/DrillingEdit.ui")
        self.formPoint = FreeCADGui.PySideUic.loadUi(":/panels/PointEdit.ui")
        self.layout = QtGui.QVBoxLayout(self.form)
        self.formDrill.setWindowTitle(self.formDrill.windowTitle())
        self.formDrill.setSizePolicy(self.formDrill.sizePolicy())
        self.formDrill.setParent(self.form)
        self.formPoint.setParent(self.form)
        self.layout.addWidget(self.formDrill)
        self.layout.addWidget(self.formPoint)
        self.formPoint.hide()

        self.deleteOnReject = deleteOnReject
        self.obj = obj
        self.isDirty = True
        self.np = CustomPoint(self.obj, self.formDrill, self.formPoint, self.addCustomPoint)

    def accept(self):
        FreeCADGui.Control.closeDialog()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.commitTransaction()
        if self.isDirty:
            FreeCAD.ActiveDocument.recompute()

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.abortTransaction()
        if self.deleteOnReject:
            FreeCAD.ActiveDocument.openTransaction(translate("Path_Drilling", "Uncreate Drilling Operation"))
            FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    def clicked(self, button):
        if button == QtGui.QDialogButtonBox.Apply:
            self.getFields()
            FreeCAD.ActiveDocument.recompute()
            self.isDirty = False

    def getFields(self):
        PathLog.track()
        if self.obj:
            try:
                if hasattr(self.obj, "StartDepth"):
                    self.obj.StartDepth = FreeCAD.Units.Quantity(self.formDrill.startDepth.text()).Value
                if hasattr(self.obj, "FinalDepth"):
                    self.obj.FinalDepth = FreeCAD.Units.Quantity(self.formDrill.finalDepth.text()).Value
                if hasattr(self.obj, "PeckDepth"):
                    if FreeCAD.Units.Quantity(self.formDrill.peckDepth.text()).Value >= 0:
                        self.obj.PeckDepth = FreeCAD.Units.Quantity(self.formDrill.peckDepth.text()).Value
                    else:
                        self.formDrill.peckDepth.setText("0.00")
                if hasattr(self.obj, "SafeHeight"):
                    self.obj.SafeHeight = FreeCAD.Units.Quantity(self.formDrill.safeHeight.text()).Value
                if hasattr(self.obj, "ClearanceHeight"):
                    self.obj.ClearanceHeight = FreeCAD.Units.Quantity(self.formDrill.clearanceHeight.text()).Value
                if hasattr(self.obj, "RetractHeight"):
                    self.obj.RetractHeight = FreeCAD.Units.Quantity(self.formDrill.retractHeight.text()).Value
                if hasattr(self.obj, "DwellTime"):
                    if FreeCAD.Units.Quantity(self.formDrill.dwellTime.text()).Value >= 0:
                        self.obj.DwellTime = FreeCAD.Units.Quantity(self.formDrill.dwellTime.text()).Value
                    else:
                        self.formDrill.dwellTime.setText("0.00")
                if hasattr(self.obj, "DwellEnabled"):
                    self.obj.DwellEnabled = self.formDrill.dwellEnabled.isChecked()
                if hasattr(self.obj, "PeckEnabled"):
                    self.obj.PeckEnabled = self.formDrill.peckEnabled.isChecked()
                if hasattr(self.obj, "ToolController"):
                    PathLog.debug("name: {}".format(self.formDrill.uiToolController.currentText()))
                    tc = PathUtils.findToolController(self.obj, self.formDrill.uiToolController.currentText())
                    self.obj.ToolController = tc
                if hasattr(self.obj, "AddTipLength"):
                    self.obj.AddTipLength = self.formDrill.chkTipDepth.isChecked()
            except ValueError:
                self.setFields()
        self.isDirty = True

    def updateFeatureList(self):
        self.formDrill.baseList.itemChanged.disconnect(self.checkedChanged)  # disconnect this slot while creating objects
        self.formDrill.baseList.clear()
        self.formDrill.baseList.setColumnCount(2)
        self.formDrill.baseList.setRowCount(0)
        self.formDrill.baseList.setHorizontalHeaderLabels(["Feature", "Diameter"])
        self.formDrill.baseList.horizontalHeader().setStretchLastSection(True)
        self.formDrill.baseList.resizeColumnToContents(0)
        self.formDrill.baseList.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        for i in range(len(self.obj.Names)):
            self.formDrill.baseList.insertRow(self.formDrill.baseList.rowCount())
            item = QtGui.QTableWidgetItem(self.obj.Names[i])
            item.setFlags(item.flags() | QtCore.Qt.ItemIsUserCheckable)

            if self.obj.Enabled[i] > 0:
                item.setCheckState(QtCore.Qt.Checked)
            else:
                item.setCheckState(QtCore.Qt.Unchecked)
            self.formDrill.baseList.setItem(self.formDrill.baseList.rowCount() - 1, 0, item)
            item = QtGui.QTableWidgetItem("{:.3f}".format(self.obj.Diameters[i]))

            self.formDrill.baseList.setItem(self.formDrill.baseList.rowCount() - 1, 1, item)
        self.formDrill.baseList.resizeColumnToContents(0)
        self.formDrill.baseList.itemChanged.connect(self.checkedChanged)

        self.formDrill.baseList.setSortingEnabled(True)

    def setFields(self):
        PathLog.track()
        self.formDrill.startDepth.setText(FreeCAD.Units.Quantity(self.obj.StartDepth.Value, FreeCAD.Units.Length).UserString)
        self.formDrill.finalDepth.setText(FreeCAD.Units.Quantity(self.obj.FinalDepth.Value, FreeCAD.Units.Length).UserString)
        self.formDrill.peckDepth.setText(FreeCAD.Units.Quantity(self.obj.PeckDepth.Value, FreeCAD.Units.Length).UserString)
        self.formDrill.safeHeight.setText(FreeCAD.Units.Quantity(self.obj.SafeHeight.Value, FreeCAD.Units.Length).UserString)
        self.formDrill.clearanceHeight.setText(FreeCAD.Units.Quantity(self.obj.ClearanceHeight.Value, FreeCAD.Units.Length).UserString)
        self.formDrill.retractHeight.setText(FreeCAD.Units.Quantity(self.obj.RetractHeight.Value, FreeCAD.Units.Length).UserString)
        self.formDrill.dwellTime.setText(str(self.obj.DwellTime))

        if self.obj.DwellEnabled:
            self.formDrill.dwellEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.formDrill.dwellEnabled.setCheckState(QtCore.Qt.Unchecked)

        if self.obj.PeckEnabled:
            self.formDrill.peckEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.formDrill.peckEnabled.setCheckState(QtCore.Qt.Unchecked)

        if self.obj.AddTipLength:
            self.formDrill.chkTipDepth.setCheckState(QtCore.Qt.Checked)
        else:
            self.formDrill.chkTipDepth.setCheckState(QtCore.Qt.Unchecked)

        self.updateFeatureList()

        controllers = PathUtils.getToolControllers(self.obj)
        labels = [c.Label for c in controllers]
        self.formDrill.uiToolController.blockSignals(True)
        self.formDrill.uiToolController.addItems(labels)
        self.formDrill.uiToolController.blockSignals(False)

        if self.obj.ToolController is not None:
            index = self.formDrill.uiToolController.findText(
                self.obj.ToolController.Label, QtCore.Qt.MatchFixedString)
            PathLog.debug("searching for TC label {}. Found Index: {}".format(self.obj.ToolController.Label, index))
            if index >= 0:
                self.formDrill.uiToolController.blockSignals(True)
                self.formDrill.uiToolController.setCurrentIndex(index)
                self.formDrill.uiToolController.blockSignals(False)

    def open(self):
        pass
        # self.s = SelObserver()
        # FreeCADGui.Selection.addObserver(self.s)

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        slist = self.formDrill.baseList.selectedItems()
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

        for i in xrange(0, self.formDrill.baseList.rowCount()):
            try:
                ind = self.obj.Names.index(self.formDrill.baseList.item(i, 0).text())
                if self.formDrill.baseList.item(i, 0).checkState() == QtCore.Qt.Checked:
                    enabledlist[ind] = 1
                else:
                    enabledlist[ind] = 0
            except:
                PathLog.track("Not found:" + self.formDrill.baseList.item(i, 0).text() + " in " + str(self.obj.Names))

        self.obj.Enabled = enabledlist
        FreeCAD.ActiveDocument.recompute()

    def enableAll(self):
        for i in xrange(0, self.formDrill.baseList.rowCount()):
            self.formDrill.baseList.item(i, 0).setCheckState(QtCore.Qt.Checked)

    def enableSelected(self):
        slist = self.formDrill.baseList.selectedItems()
        for i in slist:
            r = i.row()
            self.formDrill.baseList.item(r, 0).setCheckState(QtCore.Qt.Checked)

    def disableAll(self):
        for i in xrange(0, self.formDrill.baseList.rowCount()):
            self.formDrill.baseList.item(i, 0).setCheckState(QtCore.Qt.Unchecked)

    def disableSelected(self):
        slist = self.formDrill.baseList.selectedItems()
        for i in slist:
            r = i.row()
            self.formDrill.baseList.item(r, 0).setCheckState(QtCore.Qt.Unchecked)

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
                        names.append(objectname + '.' + sel.SubElementNames[i])
                        positions.append(FreeCAD.Vector(sub.X, sub.Y, 0))
                        enabled.append(1)
                        diameters.append(0)

                    elif sub.ShapeType == 'Edge':
                        if PathUtils.isDrillable(sobj, sub):
                            PathLog.debug("Selection is a drillable edge, lets drill that")
                            names.append(objectname + '.' + sel.SubElementNames[i])
                            positions.append(FreeCAD.Vector(sub.Curve.Center.x, sub.Curve.Center.y, 0))
                            enabled.append(1)
                            diameters.append(sub.BoundBox.XLength)
                        # check for arcs - isDrillable ignores them.
                        elif type(sub.Curve) == Part.Circle:
                            PathLog.debug("Selection is an arc or circle edge, lets drill the center")
                            names.append(objectname + '.' + sel.SubElementNames[i])
                            positions.append(FreeCAD.Vector(sub.Curve.Center.x, sub.Curve.Center.y, 0))
                            enabled.append(1)
                            diameters.append(sub.BoundBox.XLength)

                    elif sub.ShapeType == 'Face':
                        if PathUtils.isDrillable(sobj.Shape, sub):
                            PathLog.debug("Selection is a drillable face, lets drill that")
                            names.append(objectname + '.' + sel.SubElementNames[i])
                            positions.append(FreeCAD.Vector(sub.Surface.Center.x, sub.Surface.Center.y, 0))
                            enabled.append(1)
                            diameters.append(sub.BoundBox.XLength)

            self.obj.Names = names
            self.obj.Positions = positions
            self.obj.Enabled = enabled
            self.obj.Diameters = diameters

            self.updateFeatureList()

            FreeCAD.ActiveDocument.recompute()

    def addCustom(self):
        self.np.addCustom()

    def addCustomPoint(self, point, o):
        if point is not None:
            names = self.obj.Names
            positions = self.obj.Positions
            enabled = self.obj.Enabled
            diameters = self.obj.Diameters
            names.append('CustomX{:.3f}Y{:.3f}'.format(point.x, point.y))
            positions.append(FreeCAD.Vector(point.x, point.y, 0))
            enabled.append(1)
            diameters.append(0)
            self.obj.Names = names
            self.obj.Positions = positions
            self.obj.Enabled = enabled
            self.obj.Diameters = diameters

            self.updateFeatureList()
            FreeCAD.ActiveDocument.recompute()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Cancel)

    def setupUi(self):
        PathLog.track()

        # Connect Signals and Slots
        self.formDrill.startDepth.editingFinished.connect(self.getFields)
        self.formDrill.finalDepth.editingFinished.connect(self.getFields)
        self.formDrill.safeHeight.editingFinished.connect(self.getFields)
        self.formDrill.retractHeight.editingFinished.connect(self.getFields)
        self.formDrill.peckDepth.editingFinished.connect(self.getFields)
        self.formDrill.clearanceHeight.editingFinished.connect(self.getFields)
        self.formDrill.dwellTime.editingFinished.connect(self.getFields)
        self.formDrill.dwellEnabled.stateChanged.connect(self.getFields)
        self.formDrill.peckEnabled.stateChanged.connect(self.getFields)
        self.formDrill.chkTipDepth.stateChanged.connect(self.getFields)

        # buttons
        self.formDrill.uiEnableSelected.clicked.connect(self.enableSelected)
        self.formDrill.uiDisableSelected.clicked.connect(self.disableSelected)
        self.formDrill.uiFindAllHoles.clicked.connect(self.findAll)
        self.formDrill.uiAddSelected.clicked.connect(self.addSelected)
        self.formDrill.uiAddCustom.clicked.connect(self.addCustom)

        self.formDrill.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.formDrill.baseList.itemChanged.connect(self.checkedChanged)

        self.formDrill.uiToolController.currentIndexChanged.connect(self.getFields)

        self.setFields()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Drilling', CommandPathDrilling())

FreeCAD.Console.PrintLog("Loading PathDrilling... done\n")
