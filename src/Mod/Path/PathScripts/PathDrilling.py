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
import Path
import Part
from PySide import QtCore, QtGui
from PathScripts import PathUtils
from PathScripts.PathUtils import fmt

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Drilling object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectDrilling:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLinkSubList", "Base","Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The base geometry of this toolpath"))
        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","An optional comment for this profile"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","User Assigned Label"))

        obj.addProperty("App::PropertyLength", "PeckDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Incremental Drill depth before retracting to clear chips"))
        obj.addProperty("App::PropertyLength", "StartDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Height to clear top of materil"))
        obj.addProperty("App::PropertyDistance", "RetractHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","The height where feed starts and height during retract tool when path is finished"))
        obj.addProperty("App::PropertyFloat", "DwellTime", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","The time to dwell between peck cycles"))
        # Tool Properties
        obj.addProperty("App::PropertyIntegerConstraint", "ToolNumber", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property","The tool number in use"))
        obj.ToolNumber = (0, 0, 1000, 1)
        obj.setEditorMode('ToolNumber', 1)  # make this read only
        obj.addProperty("App::PropertyString", "ToolDescription", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property","The description of the tool "))
        obj.setEditorMode('ToolDescription', 1) # make this read onlyt


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

        locations = []
        output = "(Begin Drilling)\n"
        if obj.Base:
            for loc in obj.Base:
                #print loc
                for sub in loc[1]:
                    #locations.append(self._findDrillingVector(loc))

                    if "Face" in sub or "Edge" in sub:
                        s = getattr(loc[0].Shape, sub)
                    else:
                        s = loc[0].Shape

                    if s.ShapeType in ['Wire', 'Edge']:
                        X = s.Edges[0].Curve.Center.x
                        Y = s.Edges[0].Curve.Center.y
                        Z = s.Edges[0].Curve.Center.z
                    elif s.ShapeType in ['Vertex']:
                        X = s.Point.x
                        Y = s.Point.y
                        Z = s.Point.z
                    elif s.ShapeType in ['Face']:
                        #if abs(s.normalAt(0, 0).z) == 1:  # horizontal face
                        X = s.CenterOfMass.x
                        Y = s.CenterOfMass.y
                        Z = s.CenterOfMass.z
                    locations.append(FreeCAD.Vector(X, Y, Z))


            output += "G90 G98\n"
            # rapid to clearance height
            output += "G0 Z" + str(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"
            # rapid to first hole location, with spindle still retracted:
            p0 = locations[0]
            output += "G0 X" + fmt(p0.x) + " Y" + fmt(p0.y)  + "F " + PathUtils.fmt(self.horizRapid) + "\n"
            # move tool to clearance plane
            output += "G0 Z" + fmt(obj.ClearanceHeight.Value)  + "F " + PathUtils.fmt(self.vertRapid) + "\n"
            pword = ""
            qword = ""
            if obj.PeckDepth.Value > 0:
                cmd = "G83"
                qword = " Q" + fmt(obj.PeckDepth.Value)
            elif obj.DwellTime > 0:
                cmd = "G82"
                pword = " P" + fmt(obj.DwellTime)
            else:
                cmd = "G81"
            for p in locations:
                output += cmd + \
                    " X" + fmt(p.x) + \
                    " Y" + fmt(p.y) + \
                    " Z" + fmt(obj.FinalDepth.Value) + qword + pword + \
                    " R" + str(obj.RetractHeight.Value) + \
                    " F" + str(self.vertFeed) + "\n" \

            output += "G80\n"

#         path = Path.Path(output)
#         obj.Path = path

        if obj.Active:
            path = Path.Path(output)
            obj.Path = path
            obj.ViewObject.Visibility = True

        else:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False



    def checkdrillable(self, obj, sub):
        drillable = False
        if obj.ShapeType == 'Vertex':
                drillable = True
        elif obj.ShapeType in['Solid', 'Compound']:
            if sub[0:4] == 'Face':
                subobj = obj.getElement(sub)
                if isinstance(subobj.Edges[0].Curve, Part.Circle):
                    drillable = True
                if str(subobj.Surface) == "<Cylinder object>":
                    drillable = True
                if len(subobj.Edges) == 3:
                    cedge = []
                    ledge = []
                    for e in subobj.Edges:
                        if isinstance (e.Curve, Part.Circle):
                            cedge.append(e)
                        elif isinstance (e.Curve, Part.Line):
                            ledge.append(e)
                    if len(cedge) == 2 and len(ledge) == 1:
                        drillable = True
                    else:
                #if len(subobj.Edges[0].Vertexes) > 1:
                        drillable = False
            if sub[0:4] == 'Edge':
                o = obj.getElement(sub)
                if isinstance(o.Curve, Part.Circle) and len(o.Vertexes) == 1:
                    drillable = True

        return drillable

    def addDrillableLocation(self, obj, ss, sub=""):
        baselist = obj.Base
        item = (ss, sub)
        if len(baselist) == 0:  # When adding the first base object, guess at heights
            try:
                bb = ss.Shape.BoundBox  # parent boundbox
                subobj = ss.Shape.getElement(sub)
                fbb = subobj.BoundBox  # feature boundbox
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

        if not self.checkdrillable(ss.Shape,sub):
            FreeCAD.Console.PrintError("Selected element is not a drillable location" + "\n")
            return

        if sub[0:4] == 'Face':
            # Check for other drillable faces and give user the option
            drillableFaces = []

            for i in range(len(ss.Shape.Faces)):
                if self.checkdrillable(ss.Shape, "Face" + str(i+1)):
                    drillableFaces.append("Face" + str(i+1))
            if len(drillableFaces) > 1:
                reply = QtGui.QMessageBox.question(None,"","Multiple drillable faces found.  Drill them all?",
                        QtGui.QMessageBox.Yes | QtGui.QMessageBox.No, QtGui.QMessageBox.No)
                if reply == QtGui.QMessageBox.Yes:
                    for i in drillableFaces:
                        if i in baselist:
                            FreeCAD.Console.PrintWarning("Drillable location already in the list" + "\n")
                            continue
                        else:
                            newitem = (ss, i)
                            baselist.append(newitem)
                else:
                    if item in baselist:
                        FreeCAD.Console.PrintWarning("Drillable location already in the list" + "\n")
                    else:
                        baselist.append(item)
            print baselist
            obj.Base = baselist
            self.execute(obj)

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

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class TaskPanel:
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/DrillingEdit.ui")

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
            if hasattr(self.obj, "StartDepth"):
                self.obj.StartDepth = self.form.startDepth.text()
            if hasattr(self.obj, "FinalDepth"):
                self.obj.FinalDepth = self.form.finalDepth.text()
            if hasattr(self.obj, "PeckDepth"):
                self.obj.PeckDepth = self.form.peckDepth.text()
            if hasattr(self.obj, "SafeHeight"):
                self.obj.SafeHeight = self.form.safeHeight.text()
            if hasattr(self.obj, "ClearanceHeight"):
                self.obj.ClearanceHeight = self.form.clearanceHeight.text()
            if hasattr(self.obj, "RetractHeight"):
                self.obj.RetractHeight = self.form.retractHeight.text()

        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.form.startDepth.setText(str(self.obj.StartDepth.Value))
        self.form.finalDepth.setText(str(self.obj.FinalDepth.Value))
        self.form.peckDepth.setText(str(self.obj.PeckDepth.Value))
        self.form.safeHeight.setText(str(self.obj.SafeHeight.Value))
        self.form.clearanceHeight.setText(str(self.obj.ClearanceHeight.Value))
        self.form.retractHeight.setText(str(self.obj.RetractHeight.Value))

        self.form.baseList.clear()
        for i in self.obj.Base:
            for sub in i[1]:
                self.form.baseList.addItem(i[0].Name + "." + sub)


    def open(self):
        self.s = SelObserver()
        FreeCADGui.Selection.addObserver(self.s)

    def addBase(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()

        if not len(selection) >= 1:
            FreeCAD.Console.PrintError(translate("PathProject", "Please select at least one Drillable Location\n"))
            return
        for s in selection:
            if s.HasSubObjects:
                for i in s.SubElementNames:
                    self.obj.Proxy.addDrillableLocation(self.obj, s.Object, i)
            else:
                self.obj.Proxy.addDrillableLocation(self.obj, s.Object)

        self.setFields()  # defaults may have changed.  Reload.
        self.form.baseList.clear()

        for i in self.obj.Base:
            for sub in i[1]:
                self.form.baseList.addItem(i[0].Name + "." + sub)

    def deleteBase(self):
        dlist = self.form.baseList.selectedItems()
        for d in dlist:
            newlist = []
            for i in self.obj.Base:
                if not i[0].Name == d.text().partition(".")[0]:
                    newlist.append(i)
            self.obj.Base = newlist
        self.form.baseList.takeItem(self.form.baseList.row(d))
        # self.obj.Proxy.execute(self.obj)
        # FreeCAD.ActiveDocument.recompute()

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        slist = self.form.baseList.selectedItems()
        for i in slist:
            objstring = i.text().partition(".")
            obj = FreeCAD.ActiveDocument.getObject(objstring[0])
            # sub = o.Shape.getElement(objstring[2])
            if objstring[2] != "":
                FreeCADGui.Selection.addSelection(obj, objstring[2])
            else:
                FreeCADGui.Selection.addSelection(obj)

        FreeCADGui.updateGui()

    def reorderBase(self):
        newlist = []
        for i in range(self.form.baseList.count()):
            s = self.form.baseList.item(i).text()
            objstring = s.partition(".")

            obj = FreeCAD.ActiveDocument.getObject(objstring[0])
            item = (obj, str(objstring[2]))
            newlist.append(item)
        self.obj.Base = newlist

        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def setupUi(self):

        # Connect Signals and Slots
        self.form.startDepth.editingFinished.connect(self.getFields)
        self.form.finalDepth.editingFinished.connect(self.getFields)
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)

        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.reorderBase.clicked.connect(self.reorderBase)

        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)

        sel = FreeCADGui.Selection.getSelectionEx()
        if len(sel) != 0 and sel[0].HasSubObjects:
                self.addBase()

        self.setFields()

class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST
        PST.drillselect()

    def __del__(self):
        import PathScripts.PathSelection as PST
        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
        FreeCADGui.updateGui()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Drilling', CommandPathDrilling())

FreeCAD.Console.PrintLog("Loading PathDrilling... done\n")
