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
from FreeCAD import Units
import FreeCADGui
import PathUtils
import Path
import Part
import PathScripts
from PySide import QtCore, QtGui
import PathScripts.PathLog as PathLog

LOG_MODULE = 'PathLoadTool'
PathLog.setLevel(PathLog.Level.INFO, LOG_MODULE)
#PathLog.trackModule('PathLoadTool')

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class LoadTool():
    def __init__(self, obj, tool=1):
        PathLog.track('tool: {}'.format(tool))

        obj.addProperty("App::PropertyIntegerConstraint", "ToolNumber", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property", "The active tool"))
        obj.ToolNumber = (0, 0, 10000, 1)
        obj.addProperty("Path::PropertyTooltable", "Tooltable", "Base", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tooltable used for this CNC program"))

        obj.addProperty("App::PropertyFloat", "SpindleSpeed", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property", "The speed of the cutting spindle in RPM"))
        obj.addProperty("App::PropertyEnumeration", "SpindleDir", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property", "Direction of spindle rotation"))
        obj.SpindleDir = ['Forward', 'Reverse']
        obj.addProperty("App::PropertySpeed", "VertFeed", "Feed", QtCore.QT_TRANSLATE_NOOP("App::Property", "Feed rate for vertical moves in Z"))
        obj.addProperty("App::PropertySpeed", "HorizFeed", "Feed", QtCore.QT_TRANSLATE_NOOP("App::Property", "Feed rate for horizontal moves"))
        obj.addProperty("App::PropertySpeed", "VertRapid", "Rapid", QtCore.QT_TRANSLATE_NOOP("App::Property", "Rapid rate for vertical moves in Z"))
        obj.addProperty("App::PropertySpeed", "HorizRapid", "Rapid", QtCore.QT_TRANSLATE_NOOP("App::Property", "Rapid rate for horizontal moves"))
        obj.Proxy = self
        mode = 2
        obj.setEditorMode('Placement', mode)

    def execute(self, obj):
        PathLog.track()

        #toolnum = obj.Tooltable.Tools.keys()[0]
        commands = ""
        commands += "(" + obj.Label + ")"+'\n'
        commands += 'M6 T'+str(obj.ToolNumber)+'\n'

        if obj.SpindleDir == 'Forward':
            commands += 'M3 S' + str(obj.SpindleSpeed) + '\n'
        else:
            commands += 'M4 S' + str(obj.SpindleSpeed) + '\n'

        if commands == "":
            commands += "(No commands processed)"

        path = Path.Path(commands)
        obj.Path = path
        if obj.ViewObject:
            obj.ViewObject.Visibility = True

    def onChanged(self, obj, prop):
        PathLog.track('prop: {}  state: {}'.format(prop, obj.State))


        if 'Restore' not in obj.State:
            if prop == "ToolNumber":
                toolitem = obj.Tooltable.Tools.popitem()
                oldtoolnum = toolitem[0]
                tool = toolitem[1]
                obj.Tooltable.deleteTool(oldtoolnum)
                obj.Tooltable.setTool(obj.ToolNumber, tool)
            else:
                job = PathUtils.findParentJob(obj)
                if job is not None:
                    for g in job.Group:
                        if not(isinstance(g.Proxy, PathScripts.PathLoadTool.LoadTool)):
                            g.touch()

    def getTool(self, obj):
        '''returns the tool associated with this tool controller'''
        PathLog.track()
        toolitem = obj.Tooltable.Tools.popitem()
        return toolitem[1]


class _ViewProviderLoadTool:

    def __init__(self, vobj):
        vobj.Proxy = self
        mode = 2
        vobj.setEditorMode('LineWidth', mode)
        vobj.setEditorMode('MarkerColor', mode)
        vobj.setEditorMode('NormalColor', mode)
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
        return ":/icons/Path-LengthOffset.svg"

    def onChanged(self, vobj, prop):
        mode = 2
        vobj.setEditorMode('LineWidth', mode)
        vobj.setEditorMode('MarkerColor', mode)
        vobj.setEditorMode('NormalColor', mode)
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
        return {'Pixmap': 'Path-LengthOffset',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_LoadTool", "Add Tool Controller to the Job"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_LoadTool", "Add Tool Controller")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):
        PathLog.track()
        self.Create()

#         FreeCAD.ActiveDocument.openTransaction(translate("Path_LoadTool", "Create Tool Controller Object"))

#         obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "TC")
#         PathScripts.PathLoadTool.LoadTool(obj)
#         PathScripts.PathLoadTool._ViewProviderLoadTool(obj.ViewObject)

#         PathUtils.addToJob(obj)

#         FreeCAD.ActiveDocument.commitTransaction()
#         FreeCAD.ActiveDocument.recompute()

    @staticmethod
    def Create(jobname=None, assignViewProvider=True, tool=None, toolNumber=1):
        PathLog.track("tool: {} with toolNumber: {}".format(tool, toolNumber))

        import PathScripts
        import PathUtils

        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Default Tool")
        PathScripts.PathLoadTool.LoadTool(obj)
        if assignViewProvider:
            PathScripts.PathLoadTool._ViewProviderLoadTool(obj.ViewObject)

        if tool is None:
            tool = Path.Tool()
            tool.Diameter = 5.0
            tool.Name = "Default Tool"
            tool.CuttingEdgeHeight = 15.0
            tool.ToolType = "EndMill"
            tool.Material = "HighSpeedSteel"
        obj.Tooltable.setTool(toolNumber, tool)
        obj.ToolNumber = toolNumber
        PathUtils.addToJob(obj, jobname)


class TaskPanel:
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolControl.ui")
        #self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/ToolControl.ui")
        self.editform = FreeCADGui.PySideUic.loadUi(":/panels/ToolEdit.ui")

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
        if self.toolrep is not None:
            FreeCAD.ActiveDocument.removeObject(self.toolrep.Name)
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def getFields(self):
        if self.obj:
            if hasattr(self.obj, "Label"):
                self.obj.Label = self.form.tcoName.text()
            if hasattr(self.obj, "VertFeed"):
                self.obj.VertFeed = self.form.vertFeed.text()
            if hasattr(self.obj, "HorizFeed"):
                self.obj.HorizFeed = self.form.horizFeed.text()
            if hasattr(self.obj, "VertRapid"):
                self.obj.VertRapid = self.form.vertRapid.text()
            if hasattr(self.obj, "HorizRapid"):
                self.obj.HorizRapid = self.form.horizRapid.text()
            if hasattr(self.obj, "ToolNumber"):
                self.obj.ToolNumber = self.form.uiToolNum.value()
            if hasattr(self.obj, "SpindleSpeed"):
                self.obj.SpindleSpeed = self.form.spindleSpeed.value()
            if hasattr(self.obj, "SpindleDir"):
                self.obj.SpindleDir = str(self.form.cboSpindleDirection.currentText())

        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.form.tcoName.setText(self.obj.Label)
        self.form.uiToolNum.setValue(self.obj.ToolNumber)
        self.form.vertFeed.setText(str(self.obj.VertFeed.Value))
        self.form.horizFeed.setText(str(self.obj.HorizFeed.Value))
        self.form.vertRapid.setText(str(self.obj.VertRapid.Value))
        self.form.horizRapid.setText(str(self.obj.HorizRapid.Value))

        self.form.spindleSpeed.setValue(self.obj.SpindleSpeed)

        index = self.form.cboSpindleDirection.findText(self.obj.SpindleDir, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.cboSpindleDirection.setCurrentIndex(index)
        tooltable = self.obj.Tooltable

        try:
            toolnum = tooltable.Tools.keys()[0]
            tool = tooltable.getTool(toolnum)
            self.form.txtToolType.setText(tool.ToolType)
            self.form.txtToolMaterial.setText(tool.Material)
            diam = Units.Quantity(tool.Diameter, FreeCAD.Units.Length)
            self.form.txtToolDiameter.setText(diam.getUserPreferred()[0])
            self.form.txtToolName.setText(tool.Name)
        except:
            self.form.txtToolType.setText("UNDEFINED")
            self.form.txtToolMaterial.setText("UNDEFINED")
            self.form.txtToolDiameter.setText("UNDEFINED")

        radius = tool.Diameter / 2
        length = tool.CuttingEdgeHeight
        t = Part.makeCylinder(radius, length)
        self.toolrep.Shape = t

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
        self.form.tcoName.editingFinished.connect(self.getFields)
        self.form.cmdEditLocal.clicked.connect(self.editTool)

        t = Part.makeCylinder(1, 1)
        self.toolrep = FreeCAD.ActiveDocument.addObject("Part::Feature", "tool")
        self.toolrep.Shape = t

        self.setFields()

    def getType(self, tooltype):
        "gets a combobox index number for a given type or viceversa"
        toolslist = ["Drill", "CenterDrill", "CounterSink", "CounterBore",
                     "Reamer", "Tap", "EndMill", "SlotCutter", "BallEndMill",
                     "ChamferMill", "CornerRound", "Engraver"]
        if isinstance(tooltype, str):
            if tooltype in toolslist:
                return toolslist.index(tooltype)
            else:
                return 0
        else:
            return toolslist[tooltype]

    def getMaterial(self, material):
        "gets a combobox index number for a given material or viceversa"
        matslist = ["HighSpeedSteel", "HighCarbonToolSteel", "CastAlloy",
                    "Carbide", "Ceramics", "Diamond", "Sialon"]
        if isinstance(material, str):
            if material in matslist:
                return matslist.index(material)
            else:
                return 0
        else:
            return matslist[material]

    def editTool(self):
        toolnum = self.obj.Tooltable.Tools.keys()[0]
        tool = self.obj.Tooltable.getTool(toolnum)
        editform = FreeCADGui.PySideUic.loadUi(":/panels/ToolEdit.ui")

        editform.NameField.setText(tool.Name)
        editform.TypeField.setCurrentIndex(self.getType(tool.ToolType))
        editform.MaterialField.setCurrentIndex(self.getMaterial(tool.Material))
        editform.DiameterField.setText(FreeCAD.Units.Quantity(tool.Diameter, FreeCAD.Units.Length).UserString)
        editform.LengthOffsetField.setText(FreeCAD.Units.Quantity(tool.LengthOffset, FreeCAD.Units.Length).UserString)
        editform.FlatRadiusField.setText(FreeCAD.Units.Quantity(tool.FlatRadius, FreeCAD.Units.Length).UserString)
        editform.CornerRadiusField.setText(FreeCAD.Units.Quantity(tool.CornerRadius, FreeCAD.Units.Length).UserString)
        editform.CuttingEdgeAngleField.setText(FreeCAD.Units.Quantity(tool.CuttingEdgeAngle, FreeCAD.Units.Angle).UserString)
        editform.CuttingEdgeHeightField.setText(FreeCAD.Units.Quantity(tool.CuttingEdgeHeight, FreeCAD.Units.Length).UserString)

        r = editform.exec_()
        if r:
            if editform.NameField.text():
                tool.Name = str(editform.NameField.text()) #FIXME: not unicode safe!
            tool.ToolType = self.getType(editform.TypeField.currentIndex())
            tool.Material = self.getMaterial(editform.MaterialField.currentIndex())
            tool.Diameter = FreeCAD.Units.parseQuantity(editform.DiameterField.text())
            tool.LengthOffset = FreeCAD.Units.parseQuantity(editform.LengthOffsetField.text())
            tool.FlatRadius = FreeCAD.Units.parseQuantity(editform.FlatRadiusField.text())
            tool.CornerRadius = FreeCAD.Units.parseQuantity(editform.CornerRadiusField.text())
            tool.CuttingEdgeAngle = FreeCAD.Units.Quantity(editform.CuttingEdgeAngleField.text())
            tool.CuttingEdgeHeight = FreeCAD.Units.parseQuantity(editform.CuttingEdgeHeightField.text())
            self.obj.Tooltable.setTool(toolnum, tool)
            self.setFields()



class SelObserver:
    def __init__(self):
        pass

    def __del__(self):
        pass

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_LoadTool', CommandPathLoadTool())

FreeCAD.Console.PrintLog("Loading PathLoadTool... done\n")
