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

import FreeCAD
import FreeCADGui
import Path
#import Draft
import Part
import ArchPanel

import PathScripts.PathLog as PathLog
from PySide import QtCore, QtGui
from PathScripts import PathUtils

"""Path Engrave object and FreeCAD command"""

LOG_MODULE = 'PathEngrave'
PathLog.setLevel(PathLog.Level.INFO, LOG_MODULE)
# PathLog.trackModule('PathEngrave')

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectPathEngrave:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLinkSubList", "Base", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The base geometry of this object"))
        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "An optional comment for this profile"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "User Assigned Label"))

        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "The library or Algorithm used to generate the path"))
        obj.Algorithm = ['OCC Native']

        # Tool Properties
        obj.addProperty("App::PropertyLink", "ToolController", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))

        # Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyInteger", "StartVertex", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The vertex index to start the path from"))

        if FreeCAD.GuiUp:
            _ViewProviderEngrave(obj.ViewObject)

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):
        pass

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
            tool = toolLoad.Proxy.getTool(toolLoad)  # PathUtils.getTool(obj, toolLoad.ToolNumber)
            if not tool or tool.Diameter == 0:
                FreeCAD.Console.PrintError("No Tool found or diameter is zero. We need a tool to build a Path.")
                return
            else:
                self.radius = tool.Diameter/2

        wires = []

        parentJob = PathUtils.findParentJob(obj)
        if parentJob is None:
            return
        baseobject = parentJob.Base
        if baseobject is None:
            return
        try:
            if baseobject.isDerivedFrom('Sketcher::SketchObject') or \
                    baseobject.isDerivedFrom('Part::Part2DObject'):

                output += "G0 Z" + PathUtils.fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"

                # we only consider the outer wire if this is a Face
                for w in baseobject.Shape.Wires:
                    tempedges = PathUtils.cleanedges(w.Edges, 0.5)
                    wires.append(Part.Wire(tempedges))

                if obj.Algorithm == "OCC Native":
                    output += self.buildpathocc(obj, wires)

            elif isinstance(baseobject.Proxy, ArchPanel.PanelSheet):  # process the sheet

                shapes = baseobject.Proxy.getTags(baseobject, transform=True)
                for shape in shapes:
                    output += "G0 Z" + PathUtils.fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"
                    for w in shape.Wires:
                        tempedges = PathUtils.cleanedges(w.Edges, 0.5)
                        wires.append(Part.Wire(tempedges))
                    if obj.Algorithm == "OCC Native":
                        output += self.buildpathocc(obj, wires)
            else:
                raise ValueError('Unknown baseobject type for engraving')

            output += "G0 Z" + PathUtils.fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"

        except:
            FreeCAD.Console.PrintError("The Job Base Object has no engraveable element.  Engraving operation will produce no output.")

        # print output
        if output == "":
            output += "(No commands processed)"

        if obj.Active:
            path = Path.Path(output)
            obj.Path = path
            obj.ViewObject.Visibility = True

        else:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False

    def buildpathocc(self, obj, wires):
        PathLog.track()

        # import Part
        import DraftGeomUtils
        output = ""

        for wire in wires:
            offset = wire

            # reorder the wire
            offset = DraftGeomUtils.rebaseWire(offset, obj.StartVertex)

            # we create the path from the offset shape
            last = None
            for edge in offset.Edges:
                if not last:
                    # we set the first move to our first point
                    last = edge.Vertexes[0].Point
                    output += "G0" + " X" + PathUtils.fmt(last.x) + " Y" + PathUtils.fmt(last.y) + " Z" + PathUtils.fmt(obj.SafeHeight.Value) + "F " + PathUtils.fmt(self.horizRapid)  # Rapid sto starting position
                    output += "G1" + " X" + PathUtils.fmt(last.x) + " Y" + PathUtils.fmt(last.y) + " Z" + PathUtils.fmt(obj.FinalDepth.Value) + "F " + PathUtils.fmt(self.vertFeed) + "\n"  # Vertical feed to depth
                if isinstance(edge.Curve, Part.Circle):
                    point = edge.Vertexes[-1].Point
                    if point == last:  # edges can come flipped
                        point = edge.Vertexes[0].Point
                    center = edge.Curve.Center
                    relcenter = center.sub(last)
                    v1 = last.sub(center)
                    v2 = point.sub(center)
                    if v1.cross(v2).z < 0:
                        output += "G2"
                    else:
                        output += "G3"
                    output += " X" + PathUtils.fmt(point.x) + " Y" + PathUtils.fmt(point.y) + " Z" + PathUtils.fmt(obj.FinalDepth.Value)
                    output += " I" + PathUtils.fmt(relcenter.x) + " J" + PathUtils.fmt(relcenter.y) + " K" + PathUtils.fmt(relcenter.z)
                    output += " F " + PathUtils.fmt(self.horizFeed)
                    output += "\n"
                    last = point
                else:
                    point = edge.Vertexes[-1].Point
                    if point == last:  # edges can come flipped
                        point = edge.Vertexes[0].Point
                    output += "G1 X" + PathUtils.fmt(point.x) + " Y" + PathUtils.fmt(point.y) + " Z" + PathUtils.fmt(obj.FinalDepth.Value)
                    output += " F " + PathUtils.fmt(self.horizFeed)
                    output += "\n"
                    last = point
            output += "G0 Z " + PathUtils.fmt(obj.SafeHeight.Value)
        return output


class _ViewProviderEngrave:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel()
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        return True

    def getIcon(self):
        return ":/icons/Path-Engrave.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    # def onDelete(self):
    #     return None


class CommandPathEngrave:

    def GetResources(self):
        return {'Pixmap': 'Path-Engrave',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Engrave", "ShapeString Engrave"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Engrave", "Creates an Engraving Path around a Draft ShapeString")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):

        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction("Create Engrave Path")
        FreeCADGui.addModule("PathScripts.PathFaceProfile")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PathEngrave")')
        FreeCADGui.doCommand('PathScripts.PathEngrave.ObjectPathEngrave(obj)')

        FreeCADGui.doCommand('obj.ClearanceHeight = 10')
        FreeCADGui.doCommand('obj.FinalDepth= -0.1')
        FreeCADGui.doCommand('obj.SafeHeight= 5.0')
        FreeCADGui.doCommand('obj.Active = True')

        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('obj.ToolController = PathScripts.PathUtils.findToolController(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class TaskPanel:
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/EngraveEdit.ui")

    def __del__(self):
        FreeCADGui.Selection.removeObserver(self.s)

    def accept(self):
        self.getFields()

        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def getFields(self):
        if self.obj:
            if hasattr(self.obj, "FinalDepth"):
                self.obj.FinalDepth = FreeCAD.Units.Quantity(self.form.finalDepth.text()).Value
            if hasattr(self.obj, "SafeHeight"):
                self.obj.SafeHeight = FreeCAD.Units.Quantity(self.form.safeHeight.text()).Value
            if hasattr(self.obj, "ClearanceHeight"):
                self.obj.ClearanceHeight = FreeCAD.Units.Quantity(self.form.clearanceHeight.text()).Value
            if hasattr(self.obj, "ToolController"):
                tc = PathUtils.findToolController(self.obj, self.form.uiToolController.currentText())
                self.obj.ToolController = tc

        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.form.finalDepth.setText(FreeCAD.Units.Quantity(self.obj.FinalDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.safeHeight.setText(FreeCAD.Units.Quantity(self.obj.SafeHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.clearanceHeight.setText(FreeCAD.Units.Quantity(self.obj.ClearanceHeight.Value, FreeCAD.Units.Length).UserString)

        controllers = PathUtils.getToolControllers(self.obj)
        labels = [c.Label for c in controllers]
        self.form.uiToolController.blockSignals(True)
        self.form.uiToolController.addItems(labels)
        self.form.uiToolController.blockSignals(False)

        if self.obj.ToolController is None:
            self.obj.ToolController = PathUtils.findToolController(self.obj)

        if self.obj.ToolController is not None:
            index = self.form.uiToolController.findText(
                self.obj.ToolController.Label, QtCore.Qt.MatchFixedString)
            if index >= 0:
                self.form.uiToolController.blockSignals(True)
                self.form.uiToolController.setCurrentIndex(index)
                self.form.uiToolController.blockSignals(False)
        else:
            self.obj.ToolController = PathUtils.findToolController(self.obj)

    def open(self):
        self.s = SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def setupUi(self):

        # Connect Signals and Slots
        self.form.finalDepth.editingFinished.connect(self.getFields)
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)

        self.form.uiToolController.currentIndexChanged.connect(self.getFields)

        self.setFields()


class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST
        PST.engraveselect()

    def __del__(self):
        import PathScripts.PathSelection as PST
        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
        FreeCADGui.updateGui()

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Path_Engrave', CommandPathEngrave())
