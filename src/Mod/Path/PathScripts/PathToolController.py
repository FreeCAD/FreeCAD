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
import PathScripts.PathUtil as PathUtil

from PySide import QtCore

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

if FreeCAD.GuiUp:
    # need ViewProvider class in this file to support loading of old files
    from PathScripts.PathToolControllerGui import ViewProvider

FreeCAD.Console.PrintLog("Loading PathToolController... done\n")
