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
import Part
import Path
import PathScripts
import PathScripts.PathLog as PathLog
import PathScripts.PathToolEdit as PathToolEdit
import PathScripts.PathUtil as PathUtil

from FreeCAD import Units
from PySide import QtCore

if FreeCAD.GuiUp:
    import FreeCADGui
    import PathScripts.PathGui as PathGui
    from PySide import QtGui

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ToolControllerTemplate:
    '''Attribute and sub element strings for template export/import.'''
    Expressions  = 'xengine'
    ExprExpr     = 'expr'
    ExprProp     = 'prop'
    HorizFeed    = 'hfeed'
    HorizRapid   = 'hrapid'
    Label        = 'label'
    Name         = 'name'
    SpindleDir   = 'dir'
    SpindleSpeed = 'speed'
    ToolNumber   = 'nr'
    Tool         = 'tool'
    Version      = 'version'
    VertFeed     = 'vfeed'
    VertRapid    = 'vrapid'

class ToolController:
    def __init__(self, obj, tool=1):
        PathLog.track('tool: {}'.format(tool))

        obj.addProperty("App::PropertyIntegerConstraint", "ToolNumber", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property", "The active tool"))
        obj.ToolNumber = (0, 0, 10000, 1)
        obj.addProperty("Path::PropertyTool", "Tool", "Base", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool used by this controller"))

        obj.addProperty("App::PropertyFloat", "SpindleSpeed", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property", "The speed of the cutting spindle in RPM"))
        obj.addProperty("App::PropertyEnumeration", "SpindleDir", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property", "Direction of spindle rotation"))
        obj.SpindleDir = ['Forward', 'Reverse']
        obj.addProperty("App::PropertySpeed", "VertFeed", "Feed", QtCore.QT_TRANSLATE_NOOP("App::Property", "Feed rate for vertical moves in Z"))
        obj.addProperty("App::PropertySpeed", "HorizFeed", "Feed", QtCore.QT_TRANSLATE_NOOP("App::Property", "Feed rate for horizontal moves"))
        obj.addProperty("App::PropertySpeed", "VertRapid", "Rapid", QtCore.QT_TRANSLATE_NOOP("App::Property", "Rapid rate for vertical moves in Z"))
        obj.addProperty("App::PropertySpeed", "HorizRapid", "Rapid", QtCore.QT_TRANSLATE_NOOP("App::Property", "Rapid rate for horizontal moves"))
        obj.Proxy = self
        obj.setEditorMode('Placement', 2)

    def onDocumentRestored(self, obj):
        obj.setEditorMode('Placement', 2)

    def setFromTemplate(self, obj, template):
        '''setFromTemplate(obj, xmlItem) ... extract properties from xmlItem and assign to receiver.'''
        PathLog.track(obj.Name, template)
        if template.get(ToolControllerTemplate.Version) and 1 == int(template.get(ToolControllerTemplate.Version)):
            if template.get(ToolControllerTemplate.Label):
                obj.Label = template.get(ToolControllerTemplate.Label)
            if template.get(ToolControllerTemplate.VertFeed):
                obj.VertFeed = template.get(ToolControllerTemplate.VertFeed)
            if template.get(ToolControllerTemplate.HorizFeed):
                obj.HorizFeed = template.get(ToolControllerTemplate.HorizFeed)
            if template.get(ToolControllerTemplate.VertRapid):
                obj.VertRapid = template.get(ToolControllerTemplate.VertRapid)
            if template.get(ToolControllerTemplate.HorizRapid):
                obj.HorizRapid = template.get(ToolControllerTemplate.HorizRapid)
            if template.get(ToolControllerTemplate.SpindleSpeed):
                obj.SpindleSpeed = float(template.get(ToolControllerTemplate.SpindleSpeed))
            if template.get(ToolControllerTemplate.SpindleDir):
                obj.SpindleDir = template.get(ToolControllerTemplate.SpindleDir)
            if template.get(ToolControllerTemplate.ToolNumber):
                obj.ToolNumber = int(template.get(ToolControllerTemplate.ToolNumber))
            if template.get(ToolControllerTemplate.Tool):
                obj.Tool.setFromTemplate(template.get(ToolControllerTemplate.Tool))
            if template.get(ToolControllerTemplate.Expressions):
                for exprDef in template.get(ToolControllerTemplate.Expressions):
                    if exprDef[ToolControllerTemplate.ExprExpr]:
                        obj.setExpression(exprDef[ToolControllerTemplate.ExprProp], exprDef[ToolControllerTemplate.ExprExpr])
        else:
            PathLog.error(translate('PathToolController', "Unsupported PathToolController template version %s") % template.get(ToolControllerTemplate.Version))

    def templateAttrs(self, obj):
        '''templateAttrs(obj) ... answer a dictionary with all properties that should be stored for a template.'''
        attrs = {}
        attrs[ToolControllerTemplate.Version]      = 1
        attrs[ToolControllerTemplate.Name]         = obj.Name
        attrs[ToolControllerTemplate.Label]        = obj.Label
        attrs[ToolControllerTemplate.ToolNumber]   = obj.ToolNumber
        attrs[ToolControllerTemplate.VertFeed]     = ("%s" % (obj.VertFeed))
        attrs[ToolControllerTemplate.HorizFeed]    = ("%s" % (obj.HorizFeed))
        attrs[ToolControllerTemplate.VertRapid]    = ("%s" % (obj.VertRapid))
        attrs[ToolControllerTemplate.HorizRapid]   = ("%s" % (obj.HorizRapid))
        attrs[ToolControllerTemplate.SpindleSpeed] = obj.SpindleSpeed
        attrs[ToolControllerTemplate.SpindleDir]   = obj.SpindleDir
        attrs[ToolControllerTemplate.Tool]         = obj.Tool.templateAttrs()
        expressions = []
        for expr in obj.ExpressionEngine:
            PathLog.debug('%s: %s' % (expr[0], expr[1]))
            expressions.append({ToolControllerTemplate.ExprProp: expr[0], ToolControllerTemplate.ExprExpr: expr[1]})
        if expressions:
            attrs[ToolControllerTemplate.Expressions] = expressions
        return attrs

    def execute(self, obj):
        PathLog.track()

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

    def getTool(self, obj):
        '''returns the tool associated with this tool controller'''
        PathLog.track()
        return obj.Tool


class ViewProvider:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
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
        self.vobj = vobj

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def getIcon(self):
        return ":/icons/Path-ToolController.svg"

    def onChanged(self, vobj, prop):
        mode = 2
        vobj.setEditorMode('LineWidth', mode)
        vobj.setEditorMode('MarkerColor', mode)
        vobj.setEditorMode('NormalColor', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('Selectable', mode)

    def onDelete(self, vobj, args=None):
        PathUtil.clearExpressionEngine(vobj.Object)
        return True

    def updateData(self, vobj, prop):
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self, vobj=None, mode=0):
        if 0 == mode:
            if vobj is None:
                vobj = self.vobj
            FreeCADGui.Control.closeDialog()
            taskd = TaskPanel(vobj.Object)
            FreeCADGui.Control.showDialog(taskd)
            taskd.setupUi()

            FreeCAD.ActiveDocument.recompute()

            return True
        return False

    def unsetEdit(self, vobj, mode):
        # this is executed when the user cancels or terminates edit mode
        return False

    def setupContextMenu(self, vobj, menu):
        PathLog.track()
        for action in menu.actions():
            menu.removeAction(action)
        action = QtGui.QAction(translate('Path', 'Edit'), menu)
        action.triggered.connect(self.setEdit)
        menu.addAction(action)

def Create(name = 'Default Tool', tool=None, toolNumber=1, assignViewProvider=True):
    PathLog.track(tool, toolNumber)

    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Label = name
    ToolController(obj)
    if FreeCAD.GuiUp and assignViewProvider:
        ViewProvider(obj.ViewObject)

    if tool is None:
        tool = Path.Tool()
        tool.Diameter = 5.0
        tool.Name = "Default Tool"
        tool.CuttingEdgeHeight = 15.0
        tool.ToolType = "EndMill"
        tool.Material = "HighSpeedSteel"
    obj.Tool = tool
    obj.ToolNumber = toolNumber
    return obj

def FromTemplate(template, assignViewProvider=True):
    PathLog.track()

    name = template.get(ToolControllerTemplate.Name, ToolControllerTemplate.Label)
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    tc  = ToolController(obj)
    if FreeCAD.GuiUp and assignViewProvider:
        ViewProvider(obj.ViewObject)

    tc.setFromTemplate(obj, template)

    return obj


class CommandPathToolController:
    def GetResources(self):
        return {'Pixmap': 'Path-LengthOffset',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_ToolController", "Add Tool Controller to the Job"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_ToolController", "Add Tool Controller")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):
        PathLog.track()
        Create()

class ToolControllerEditor:

    def __init__(self, obj, asDialog):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/DlgToolControllerEdit.ui")
        if not asDialog:
            self.form.buttonBox.hide()
        self.obj = obj

        self.vertFeed = PathGui.QuantitySpinBox(self.form.vertFeed, obj, 'VertFeed')
        self.horizFeed = PathGui.QuantitySpinBox(self.form.horizFeed, obj, 'HorizFeed')
        self.vertRapid = PathGui.QuantitySpinBox(self.form.vertRapid, obj, 'VertRapid')
        self.horizRapid = PathGui.QuantitySpinBox(self.form.horizRapid, obj, 'HorizRapid')

        self.editor = PathToolEdit.ToolEditor(obj.Tool, self.form.toolEditor)

    def updateUi(self):
        tc = self.obj
        self.form.tcName.setText(tc.Label)
        self.form.tcNumber.setValue(tc.ToolNumber)
        self.horizFeed.updateSpinBox()
        self.horizRapid.updateSpinBox()
        self.vertFeed.updateSpinBox()
        self.vertRapid.updateSpinBox()
        self.form.spindleSpeed.setValue(tc.SpindleSpeed)
        index = self.form.spindleDirection.findText(tc.SpindleDir, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.spindleDirection.setCurrentIndex(index)

        self.editor.updateUI()

    def updateToolController(self):
        tc = self.obj
        try:
            tc.Label = self.form.tcName.text()
            tc.ToolNumber = self.form.tcNumber.value()
            self.horizFeed.updateProperty()
            self.vertFeed.updateProperty()
            self.horizRapid.updateProperty()
            self.vertRapid.updateProperty()
            tc.SpindleSpeed = self.form.spindleSpeed.value()
            tc.SpindleDir = self.form.spindleDirection.currentText()

            self.editor.updateTool()
            tc.Tool = self.editor.tool

        except Exception as e:
            PathLog.error(translate("PathToolController", "Error updating TC: %s") % e)


    def refresh(self):
        self.form.blockSignals(True)
        self.updateToolController()
        self.updateUi()
        self.form.blockSignals(False)

    def setupUi(self):
        self.editor.setupUI()

        self.form.tcName.editingFinished.connect(self.refresh)
        self.form.horizFeed.editingFinished.connect(self.refresh)
        self.form.vertFeed.editingFinished.connect(self.refresh)
        self.form.horizRapid.editingFinished.connect(self.refresh)
        self.form.vertRapid.editingFinished.connect(self.refresh)


class TaskPanel:

    def __init__(self, obj):
        self.editor = ToolControllerEditor(obj, False)
        self.form = self.editor.form
        self.updating = False
        self.toolrep = None
        self.obj = obj

    def accept(self):
        self.getFields()

        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        if self.toolrep is not None:
            FreeCAD.ActiveDocument.removeObject(self.toolrep.Name)
        FreeCAD.ActiveDocument.recompute()

    def reject(self):
        FreeCADGui.Control.closeDialog()
        if self.toolrep is not None:
            FreeCAD.ActiveDocument.removeObject(self.toolrep.Name)
        FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        self.editor.updateToolController()
        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.editor.updateUi()

        tool = self.obj.Tool
        radius = tool.Diameter / 2
        length = tool.CuttingEdgeHeight
        t = Part.makeCylinder(radius, length)
        self.toolrep.Shape = t

    def edit(self, item, column):
        if not self.updating:
            self.resetObject()

    def resetObject(self, remove=None):
        "transfers the values from the widget to the object"
        FreeCAD.ActiveDocument.recompute()

    def setupUi(self):
        t = Part.makeCylinder(1, 1)
        self.toolrep = FreeCAD.ActiveDocument.addObject("Part::Feature", "tool")
        self.toolrep.Shape = t

        self.setFields()
        self.editor.setupUi()


class DlgToolControllerEdit:
    def __init__(self, obj):
        self.editor = ToolControllerEditor(obj, True)
        self.editor.updateUi()
        self.editor.setupUi()
        self.obj = obj

    def exec_(self):
        restoreTC   = self.obj.Proxy.templateAttrs(self.obj)

        rc = False
        if not self.editor.form.exec_():
            PathLog.info("revert")
            self.obj.Proxy.setFromTemplate(self.obj, restoreTC)
            rc = True
        return rc

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_ToolController', CommandPathToolController())

FreeCAD.Console.PrintLog("Loading PathToolController... done\n")
