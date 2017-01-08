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
import Draft
import Part

from PySide import QtCore, QtGui
from PathScripts import PathUtils

"""Path Engrave object and FreeCAD command"""

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
        obj.addProperty("App::PropertyLinkSubList", "Base", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The base geometry of this object"))
        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","An optional comment for this profile"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","User Assigned Label"))

        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property","The library or Algorithm used to generate the path"))
        obj.Algorithm = ['OCC Native']

        # Tool Properties
        obj.addProperty("App::PropertyEnumeration", "ToolController", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property","The tool controller to use"))
        obj.ToolController = ["None"]

        obj.addProperty("App::PropertyIntegerConstraint", "ToolNumber", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property","The tool number in use"))
        obj.ToolNumber = (0, 0, 1000, 1)
        obj.setEditorMode('ToolNumber', 1)  # make this read only
        obj.addProperty("App::PropertyString", "ToolDescription", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property","The description of the tool "))
        obj.setEditorMode('ToolDescription', 1)  # make this read onlyt

        # Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyInteger", "StartVertex", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The vertex index to start the path from"))

        if FreeCAD.GuiUp:
            _ViewProviderEngrave(obj.ViewObject)

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):
        if prop == "UserLabel":
            obj.Label = obj.UserLabel + " :" + obj.ToolDescription

    def execute(self, obj):
        output = ""
        if obj.Comment != "":
            output += '(' + str(obj.Comment)+')\n'

        # myJob = PathUtils.findParentJob(obj)
        # if myJob is not None:
        #     controllers = myJob.Proxy.getToolControllers(myJob)
        #     if len(controllers) >= 1:
        #         mlist = []
        #         for c in controllers:
        #             mlist.append(c.Name)
        #     else:
        #         mlist = ["None"]
        #     obj.ToolController = mlist

        toolLoad = PathUtils.getLastToolLoad(obj)
        if toolLoad is None or toolLoad.ToolNumber == 0:
            self.vertFeed = 100
            self.horizFeed = 100
            self.vertRapid = 100
            self.horizRapid = 100
            self.radius = 0.25
            obj.ToolNumber = 0
            obj.ToolDescription = "UNDEFINED"
        else:
            self.vertFeed = toolLoad.VertFeed.Value
            self.horizFeed = toolLoad.HorizFeed.Value
            self.vertRapid = toolLoad.VertRapid.Value
            self.horizRapid = toolLoad.HorizRapid.Value
            tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
            if tool.Diameter == 0:
                self.radius = 0.25
            else:
                self.radius = tool.Diameter/2
            obj.ToolNumber = toolLoad.ToolNumber
            obj.ToolDescription = toolLoad.Name

        if obj.UserLabel == "":
            obj.Label = obj.Name + " :" + obj.ToolDescription
        else:
            obj.Label = obj.UserLabel + " :" + obj.ToolDescription

        if obj.Base:
            output += "G0 Z" + PathUtils.fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"

            wires = []
            for o in obj.Base:
                # we only consider the outer wire if this is a Face
                for w in o[0].Shape.Wires:
                    tempedges = PathUtils.cleanedges(w.Edges, 0.5)
                    wires.append (Part.Wire(tempedges))

                if obj.Algorithm == "OCC Native":
                    output += self.buildpathocc(obj, wires)

            output += "G0 Z" + PathUtils.fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) +"\n"


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
        # path = Path.Path(output)
        # obj.Path = path

    def buildpathocc(self, obj, wires):
        import Part
        import DraftGeomUtils
        output = ""

        # absolute coords, millimeters, cancel offsets

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
                    output += "G0" + " X" + PathUtils.fmt(last.x) + " Y" + PathUtils.fmt(last.y) + " Z" + PathUtils.fmt(obj.SafeHeight.Value)  + "F " + PathUtils.fmt(self.horizRapid)  # Rapid sto starting position
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

    def addEngraveBase(self, obj, ss):
        baselist = obj.Base
        if len(baselist) == 0:  # When adding the first base object, guess at heights
            try:
                bb = ss.Shape.BoundBox  # parent boundbox
                obj.ClearanceHeight = bb.ZMax + 5.0
                obj.SafeHeight = bb.ZMax + 3.0
                obj.FinalDepth = bb.ZMax - 1
            except:
                obj.ClearanceHeight = 10.0
                obj.SafeHeight = 8.0
                obj.FinalDepth = 4.0

        item = (ss, "")
        if item in baselist:
            FreeCAD.Console.PrintWarning("Object already in the Engraving list" + "\n")

        else:
            baselist.append(item)
        obj.Base = baselist
        self.execute(obj)


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

    def onDelete(self):
        return None


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

        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.form.finalDepth.setText(FreeCAD.Units.Quantity(self.obj.FinalDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.safeHeight.setText(FreeCAD.Units.Quantity(self.obj.SafeHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.clearanceHeight.setText(FreeCAD.Units.Quantity(self.obj.ClearanceHeight.Value, FreeCAD.Units.Length).UserString)

        self.form.baseList.clear()
        for i in self.obj.Base:
            self.form.baseList.addItem(i[0].Name)

    def open(self):
        self.s = SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)

    def addBase(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()

        if not len(selection) >= 1:
            FreeCAD.Console.PrintError(translate("Path_Engrave", "Please select engraveable geometry\n"))
            return
        for s in selection:
            if not Draft.getType(s.Object) in ["ShapeString", "Part"]:
                FreeCAD.Console.PrintError(translate("Path_Engrave", "Please select valid geometry\n"))
                return
            self.obj.Proxy.addEngraveBase(self.obj, s.Object)

        self.setFields()

    def deleteBase(self):
        dlist = self.form.baseList.selectedItems()
        for d in dlist:
            newlist = []
            for i in self.obj.Base:
                if not i[0].Name == d.text():
                    newlist.append(i)
            self.obj.Base = newlist
        self.form.baseList.takeItem(self.form.baseList.row(d))

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        slist = self.form.baseList.selectedItems()
        for i in slist:
            o = FreeCAD.ActiveDocument.getObject(i.text())
            FreeCADGui.Selection.addSelection(o)
        FreeCADGui.updateGui()

    def reorderBase(self):
        newlist = []
        for i in range(self.form.baseList.count()):
            s = self.form.baseList.item(i).text()
            obj = FreeCAD.ActiveDocument.getObject(s)
            newlist.append(obj)
        self.obj.Base = newlist
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def setupUi(self):

        # Connect Signals and Slots
        self.form.finalDepth.editingFinished.connect(self.getFields)
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)

        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.reorderBase.clicked.connect(self.reorderBase)

        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)

        sel = FreeCADGui.Selection.getSelectionEx()
        if len(sel) != 0:
                self.addBase()

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
