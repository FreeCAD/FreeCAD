# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
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
''' Tool Controller defines tool, spindle speed and feed rates for Path Operations '''

import FreeCAD
import FreeCADGui
import Path
# import PathGui
import PathScripts
import PathUtils
import Part
# from PathScripts import PathProject
from PySide import QtCore, QtGui

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class LoadTool():
    def __init__(self, obj):
        obj.addProperty("App::PropertyIntegerConstraint", "ToolNumber", "Tool", "The active tool")
        obj.ToolNumber = (0, 0, 10000, 1)
        obj.addProperty("App::PropertyFloat", "SpindleSpeed", "Tool", "The speed of the cutting spindle in RPM")
        obj.addProperty("App::PropertyEnumeration", "SpindleDir", "Tool", "Direction of spindle rotation")
        obj.SpindleDir = ['Forward', 'Reverse']
        obj.addProperty("App::PropertySpeed", "VertFeed", "Feed", "Feed rate for vertical moves in Z")
        obj.addProperty("App::PropertySpeed", "HorizFeed", "Feed", "Feed rate for horizontal moves")

        obj.Proxy = self
        mode = 2
        obj.setEditorMode('Placement', mode)

    def execute(self, obj):

        tool = PathUtils.getTool(obj, obj.ToolNumber)
        if tool is not None:
            obj.Label = obj.Name + ": " + tool.Name
        else:
            obj.Label = obj.Name + ": UNDEFINED TOOL"

        commands = ""
        commands += "(" + obj.Label + ")"+'\n'
        commands += 'M6T'+str(obj.ToolNumber)+'\n'

        if obj.SpindleDir == 'Forward':
            commands += 'M3S' + str(obj.SpindleSpeed) + '\n'

        else:
            commands += 'M4S' + str(obj.SpindleSpeed) + '\n'

        obj.Path = Path.Path(commands)

    def onChanged(self, obj, prop):
        mode = 2
        obj.setEditorMode('Placement', mode)
        # if prop == "ToolNumber":
        proj = PathUtils.findProj()
        for g in proj.Group:
            if not(isinstance(g.Proxy, PathScripts.PathLoadTool.LoadTool)):
                g.touch()


class _ViewProviderLoadTool:

    def __init__(self, vobj):
        vobj.Proxy = self
        mode = 2
        vobj.setEditorMode('LineWidth', mode)
        vobj.setEditorMode('MarkerColor', mode)
        vobj.setEditorMode('NormalColor', mode)
        vobj.setEditorMode('ShowFirstRapid', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('Selectable', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('Transparency', mode)
        vobj.setEditorMode('Visibility', mode)

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def getIcon(self):
        return ":/icons/Path-LoadTool.svg"

    def onChanged(self, vobj, prop):
        mode = 2
        vobj.setEditorMode('LineWidth', mode)
        vobj.setEditorMode('MarkerColor', mode)
        vobj.setEditorMode('NormalColor', mode)
        vobj.setEditorMode('ShowFirstRapid', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('Selectable', mode)

    def updateData(self, vobj, prop):
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self, vobj, mode):
        # this is executed when the object is double-clicked in the tree
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel()
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()

        FreeCAD.ActiveDocument.recompute()

        return True

    def unsetEdit(self, vobj, mode):
        # this is executed when the user cancels or terminates edit mode
        return False


class CommandPathLoadTool:
    def GetResources(self):
        return {'Pixmap': 'Path-LoadTool',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_LoadTool", "Add Tool Controller to the Project"),
                'Accel': "P, T",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_LoadTool", "Add Tool Controller")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_LoadTool", "Create Tool Controller Object"))
        snippet = '''
import Path, PathScripts
from PathScripts import PathUtils, PathLoadTool

obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","TC")
PathScripts.PathLoadTool.LoadTool(obj)
PathScripts.PathLoadTool._ViewProviderLoadTool(obj.ViewObject)

PathUtils.addToProject(obj)
'''
        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    @staticmethod
    def Create():
        # FreeCADGui.addModule("PathScripts.PathLoadTool")
        # import Path
        import PathScripts
        import PathUtils

        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "TC")
        PathScripts.PathLoadTool.LoadTool(obj)
        PathScripts.PathLoadTool._ViewProviderLoadTool(obj.ViewObject)

        PathUtils.addToProject(obj)


class TaskPanel:
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolControl.ui")
        #self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/ToolControl.ui")
        self.updating = False
        self.toolrep = None


    def accept(self):
        self.getFields()

        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        if self.toolrep is not None:
            FreeCAD.ActiveDocument.removeObject(self.toolrep.Name)
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)


    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def getFields(self):
        if self.obj:

            if hasattr(self.obj, "VertFeed"):
                self.obj.Label = self.form.tcoName.text()
            if hasattr(self.obj, "VertFeed"):
                self.obj.VertFeed = self.form.vertFeed.text()
            if hasattr(self.obj, "HorizFeed"):
                self.obj.HorizFeed = self.form.horizFeed.text()
            if hasattr(self.obj, "SpindleSpeed"):
                self.obj.SpindleSpeed = self.form.spindleSpeed.value()
            if hasattr(self.obj, "SpindleDir"):
                self.obj.SpindleDir = str(self.form.cboSpindleDirection.currentText())
            #if hasattr(self.obj, "ToolNumber"):
             #   self.obj.ToolNumber = self.form.ToolNumber.value()
        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.form.vertFeed.setText(str(self.obj.VertFeed.Value))
        self.form.horizFeed.setText(str(self.obj.HorizFeed.Value))
        self.form.spindleSpeed.setValue(self.obj.SpindleSpeed)
        self.form.tcoName.setText(str(self.obj.Label))

        index = self.form.cboSpindleDirection.findText(self.obj.SpindleDir, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.cboSpindleDirection.setCurrentIndex(index)
        # Populate the tool list
        mach = PathUtils.findMachine()
        try:
            tool = mach.Tooltable.Tools[self.obj.ToolNumber]
            self.form.txtToolName.setText(tool.Name)
            self.form.txtToolType.setText(tool.ToolType)
            self.form.txtToolMaterial.setText(tool.Material)
            self.form.txtToolDiameter.setText(str(tool.Diameter))
        except:
            self.form.txtToolName.setText("UNDEFINED")
            self.form.txtToolType.setText("UNDEFINED")
            self.form.txtToolMaterial.setText("UNDEFINED")
            self.form.txtToolDiameter.setText("UNDEFINED")


    def open(self):
        self.s = SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)


    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def edit(self, item, column):
        if not self.updating:
            self.resetObject()

    def resetObject(self, remove=None):
        "transfers the values from the widget to the object"
        FreeCAD.ActiveDocument.recompute()

    def setupUi(self):
        # setup the form fields
        self.setFields()

        # build the tool representation
        if self.form.txtToolDiameter.text() != "UNDEFINED":
            radius = float(self.form.txtToolDiameter.text()) / 2
            length = radius * 8
            t = Part.makeCylinder(radius, length)
            self.toolrep = FreeCAD.ActiveDocument.addObject("Part::Feature", "tool")
            self.toolrep.Shape = t

class SelObserver:
    def __init__(self):
        pass

    def __del__(self):
        pass


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_LoadTool', CommandPathLoadTool())

FreeCAD.Console.PrintLog("Loading PathLoadTool... done\n")
