# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic  <shopinthewoods@gmail.com>              *
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
from PathScripts import PathUtils

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui

__title__ = "Path Surface Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path surface object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectStrategy:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLinkSubList", "Base", "Path", "The base geometry of this toolpath")
        obj.addProperty("App::PropertyBool", "Active", "Path", "Make False, to prevent operation from generating code")
        obj.addProperty("App::PropertyString", "Comment", "Path", "An optional comment for this profile")
        obj.addProperty("App::PropertyString", "UserLabel", "Path", "User Assigned Label")

        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm", "The library to use to generate the path")
        obj.Algorithm = ['OCL Dropcutter', 'OCL Waterline']

        # Tool Properties
        obj.addProperty("App::PropertyIntegerConstraint", "ToolNumber", "Tool", "The tool number in use")
        obj.ToolNumber = (0, 0, 1000, 0)
        obj.setEditorMode('ToolNumber', 1)  # make this read only
        obj.addProperty("App::PropertyString", "ToolDescription", "Tool", "The description of the tool ")
        obj.setEditorMode('ToolDescription', 1)  # make this read onlyt

        # Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", "The height needed to clear clamps and obstructions")
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", "Rapid Safety Height between locations.")
        obj.addProperty("App::PropertyFloatConstraint", "StepDown", "Depth", "Incremental Step Down of Tool")
        obj.StepDown = (0.0, 0.01, 100.0, 0.5)
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", "Starting Depth of Tool- first cut depth in Z")
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", "Final Depth of Tool- lowest value in Z")
        obj.addProperty("App::PropertyDistance", "FinishDepth", "Depth", "Maximum material removed on final pass.")

        obj.Proxy = self

    def addbase(self, obj, ss, sub=""):
        baselist = obj.Base
        if len(baselist) == 0:  # When adding the first base object, guess at heights
            try:
                bb = ss.Shape.BoundBox  # parent boundbox
                subobj = ss.Shape.getElement(sub)
                fbb = subobj.BoundBox  # feature boundbox
                obj.StartDepth = bb.ZMax
                obj.ClearanceHeight = bb.ZMax + 5.0
                obj.SafeHeight = bb.ZMax + 3.0

                if fbb.ZMax < bb.ZMax:
                    obj.FinalDepth = fbb.ZMax
                else:
                    obj.FinalDepth = bb.ZMin
            except:
                obj.StartDepth = 5.0
                obj.ClearanceHeight = 10.0
                obj.SafeHeight = 8.0

        item = (ss, sub)
        if item in baselist:
            FreeCAD.Console.PrintWarning(
                "this object already in the list" + "\n")
        else:
            baselist.append(item)
        obj.Base = baselist
        self.execute(obj)

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):
        return None

    def execute(self, obj):
        output = ""

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


        output += "(" + obj.Label + ")"
        output += "(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"

        if obj.Active:
            path = Path.Path(output)
            obj.Path = path
            obj.ViewObject.Visibility = True

        else:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False

class ViewProviderStrategy:

    def __init__(self, obj):  # mandatory
        #        obj.addProperty("App::PropertyFloat","SomePropertyName","PropertyGroup","Description of this property")
        obj.Proxy = self

    def __getstate__(self):  # mandatory
        return None

    def __setstate__(self, state):  # mandatory
        return None

    def getIcon(self):  # optional
        return ":/icons/Path-Surfacing.svg"

    def onChanged(self, obj, prop):  # optional
        # this is executed when a property of the VIEW PROVIDER changes
        pass

    def updateData(self, obj, prop):  # optional
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel()
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        return True

    def unsetEdit(self, vobj, mode):  # optional
        # this is executed when the user cancels or terminates edit mode
        pass


class CommandPathStrategy:

    def GetResources(self):
        return {'Pixmap': 'Path-3DSurface',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Strategy", "Strategy"),
                'Accel': "P, D",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Strategy", "Creates a Path Strategy object")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):

        ztop = 10
        zbottom = 0

        FreeCAD.ActiveDocument.openTransaction(translate("Path_Strategy", "Create Strategy"))
        FreeCADGui.addModule("PathScripts.PathStrategy")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Strategy")')
        FreeCADGui.doCommand('PathScripts.PathStrategy.ObjectStrategy(obj)')
        FreeCADGui.doCommand('obj.Active = True')
        FreeCADGui.doCommand('PathScripts.PathStrategy.ViewProviderStrategy(obj.ViewObject)')
        FreeCADGui.doCommand('from PathScripts import PathUtils')
        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(ztop + 2))
        FreeCADGui.doCommand('obj.StartDepth = ' + str(ztop))
        FreeCADGui.doCommand('obj.SafeHeight = ' + str(ztop + 2))
        FreeCADGui.doCommand('obj.StepDown = ' + str((ztop - zbottom) / 8))
        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCAD.ActiveDocument.commitTransaction()

        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class TaskPanel:

    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/StrategyEdit.ui")
        #self.form = FreeCADGui.PySideUic.loadUi(":/panels/SurfaceEdit.ui")

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
            if hasattr(self.obj, "FinishDepth"):
                self.obj.FinishDepth = self.form.finishDepth.text()
            if hasattr(self.obj, "StepDown"):
                self.obj.StepDown = self.form.stepDown.value()
            if hasattr(self.obj, "SafeHeight"):
                self.obj.SafeHeight = self.form.safeHeight.text()
            if hasattr(self.obj, "ClearanceHeight"):
                self.obj.ClearanceHeight = self.form.clearanceHeight.text()

        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.form.startDepth.setText(str(self.obj.StartDepth.Value))
        self.form.finalDepth.setText(str(self.obj.FinalDepth.Value))
        self.form.finishDepth.setText(str(self.obj.FinishDepth.Value))
        self.form.stepDown.setValue(self.obj.StepDown)

        self.form.safeHeight.setText(str(self.obj.SafeHeight.Value))
        self.form.clearanceHeight.setText(str(self.obj.ClearanceHeight.Value))

        for i in self.obj.Base:
            self.form.baseList.addItem(i[0].Name)

    def open(self):
        self.s = SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)

    def addBase(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate(
                "PathSurface", "Please select a single solid object from the project tree\n"))
            return

        if not len(selection[0].SubObjects) == 0:
            FreeCAD.Console.PrintError(translate(
                "PathSurface", "Please select a single solid object from the project tree\n"))
            return

        sel = selection[0].Object
        # get type of object
        # if sel.TypeId.startswith('Mesh'):
        #     # it is a mesh already
        #     print 'was already mesh'

        # elif sel.TypeId.startswith('Part') and \
        #         (sel.Shape.BoundBox.XLength > 0) and \
        #         (sel.Shape.BoundBox.YLength > 0) and \
        #         (sel.Shape.BoundBox.ZLength > 0):
        #     print 'this is a solid Part object'

        # else:
        #     FreeCAD.Console.PrintError(
        #         translate("PathSurface", "Cannot work with this object\n"))
        #     return

        self.obj.Proxy.addbase(self.obj, sel)

        self.setFields()  # defaults may have changed.  Reload.
        self.form.baseList.clear()
        for i in self.obj.Base:
            self.form.baseList.addItem(i[0].Name)

    def deleteBase(self):
        dlist = self.form.baseList.selectedItems()
        for d in dlist:
            newlist = []
            for i in self.obj.Base:
                if not i[0].Name == d.text():
                    newlist.append(i)
            self.obj.Base = newlist
        self.form.baseList.takeItem(self.form.baseList.row(d))
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

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

        #Base Geometry
        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.reorderBase.clicked.connect(self.reorderBase)
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)

        # Depths
        self.form.startDepth.editingFinished.connect(self.getFields)
        self.form.finalDepth.editingFinished.connect(self.getFields)
        self.form.finishDepth.editingFinished.connect(self.getFields)
        self.form.stepDown.editingFinished.connect(self.getFields)

        # Heights
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)

        sel = FreeCADGui.Selection.getSelectionEx()
        self.setFields()

        if len(sel) != 0:
            self.addBase()


class SelObserver:

    def __init__(self):
        import PathScripts.PathSelection as PST
        PST.surfaceselect()

    def __del__(self):
        import PathScripts.PathSelection as PST
        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):               # Selection object
        FreeCADGui.doCommand(
            'Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
        FreeCADGui.updateGui()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Strategy', CommandPathStrategy())

FreeCAD.Console.PrintLog("Loading PathStrategy... done\n")
