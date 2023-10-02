# -*- coding: utf-8 -*-
# ***************************************************************************
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

"""Tool Controller defines tool, spindle speed and feed rates for Path Operations"""

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import Path.Tool.Bit as PathToolBit
import Path.Base.Generator.toolchange as toolchange


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class ToolControllerTemplate:
    """Attribute and sub element strings for template export/import."""

    Expressions = "xengine"
    ExprExpr = "expr"
    ExprProp = "prop"
    HorizFeed = "hfeed"
    HorizRapid = "hrapid"
    Label = "label"
    Name = "name"
    SpindleDir = "dir"
    SpindleSpeed = "speed"
    ToolNumber = "nr"
    Tool = "tool"
    Version = "version"
    VertFeed = "vfeed"
    VertRapid = "vrapid"


class ToolController:
    def __init__(self, obj, createTool=True):
        Path.Log.track("tool: ")

        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "ToolNumber",
            "Tool",
            QT_TRANSLATE_NOOP("App::Property", "The active tool"),
        )
        obj.ToolNumber = (0, 0, 10000, 1)
        obj.addProperty(
            "App::PropertyFloat",
            "SpindleSpeed",
            "Tool",
            QT_TRANSLATE_NOOP(
                "App::Property", "The speed of the cutting spindle in RPM"
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "SpindleDir",
            "Tool",
            QT_TRANSLATE_NOOP("App::Property", "Direction of spindle rotation"),
        )
        obj.addProperty(
            "App::PropertySpeed",
            "VertFeed",
            "Feed",
            QT_TRANSLATE_NOOP("App::Property", "Feed rate for vertical moves in Z"),
        )
        obj.addProperty(
            "App::PropertySpeed",
            "HorizFeed",
            "Feed",
            QT_TRANSLATE_NOOP("App::Property", "Feed rate for horizontal moves"),
        )
        obj.addProperty(
            "App::PropertySpeed",
            "VertRapid",
            "Rapid",
            QT_TRANSLATE_NOOP("App::Property", "Rapid rate for vertical moves in Z"),
        )
        obj.addProperty(
            "App::PropertySpeed",
            "HorizRapid",
            "Rapid",
            QT_TRANSLATE_NOOP("App::Property", "Rapid rate for horizontal moves"),
        )
        obj.setEditorMode("Placement", 2)

        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

        if createTool:
            self.ensureToolBit(obj)

    @classmethod
    def propertyEnumerations(self, dataType="data"):
        """helixOpPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        # Enumeration lists for App::PropertyEnumeration properties
        enums = {
            "SpindleDir": [
                (translate("Path_ToolController", "Forward"), "Forward"),
                (translate("Path_ToolController", "Reverse"), "Reverse"),
                (translate("Path_ToolController", "None"), "None"),
            ],  # this is the direction that the profile runs
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def onDocumentRestored(self, obj):
        obj.setEditorMode("Placement", 2)

    def onDelete(self, obj, arg2=None):
        if hasattr(obj.Tool, "InList") and len(obj.Tool.InList) == 1:
            if hasattr(obj.Tool.Proxy, "onDelete"):
                obj.Tool.Proxy.onDelete(obj.Tool)

    def setFromTemplate(self, obj, template):
        """
        setFromTemplate(obj, xmlItem) ... extract properties from xmlItem
        and assign to receiver.
        """
        Path.Log.track(obj.Name, template)
        version = 0
        if template.get(ToolControllerTemplate.Version):
            version = int(template.get(ToolControllerTemplate.Version))
            if version == 1 or version == 2:
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
                    obj.SpindleSpeed = float(
                        template.get(ToolControllerTemplate.SpindleSpeed)
                    )
                if template.get(ToolControllerTemplate.SpindleDir):
                    obj.SpindleDir = template.get(ToolControllerTemplate.SpindleDir)
                if template.get(ToolControllerTemplate.ToolNumber):
                    obj.ToolNumber = int(
                        template.get(ToolControllerTemplate.ToolNumber)
                    )
                if template.get(ToolControllerTemplate.Tool):
                    self.ensureToolBit(obj)
                    toolVersion = template.get(ToolControllerTemplate.Tool).get(
                        ToolControllerTemplate.Version
                    )
                    if toolVersion == 2:
                        obj.Tool = PathToolBit.Factory.CreateFromAttrs(
                            template.get(ToolControllerTemplate.Tool)
                        )
                    else:
                        obj.Tool = None
                        if toolVersion == 1:
                            Path.Log.error(
                                f"{obj.Name} - legacy Tools no longer supported - ignoring"
                            )
                        else:
                            Path.Log.error(
                                f"{obj.Name} - unknown Tool version {toolVersion} - ignoring"
                            )
                    if (
                        obj.Tool
                        and obj.Tool.ViewObject
                        and obj.Tool.ViewObject.Visibility
                    ):
                        obj.Tool.ViewObject.Visibility = False
                if template.get(ToolControllerTemplate.Expressions):
                    for exprDef in template.get(ToolControllerTemplate.Expressions):
                        if exprDef[ToolControllerTemplate.ExprExpr]:
                            obj.setExpression(
                                exprDef[ToolControllerTemplate.ExprProp],
                                exprDef[ToolControllerTemplate.ExprExpr],
                            )
            else:
                Path.Log.error(
                    "Unsupported PathToolController template version {}".format(
                        template.get(ToolControllerTemplate.Version)
                    )
                )
        else:
            Path.Log.error(
                "PathToolController template has no version - corrupted template file?"
            )

    def templateAttrs(self, obj):
        """templateAttrs(obj) ... answer a dictionary with all properties that should be stored for a template."""
        attrs = {}
        attrs[ToolControllerTemplate.Version] = 1
        attrs[ToolControllerTemplate.Name] = obj.Name
        attrs[ToolControllerTemplate.Label] = obj.Label
        attrs[ToolControllerTemplate.ToolNumber] = obj.ToolNumber
        attrs[ToolControllerTemplate.VertFeed] = "%s" % (obj.VertFeed)
        attrs[ToolControllerTemplate.HorizFeed] = "%s" % (obj.HorizFeed)
        attrs[ToolControllerTemplate.VertRapid] = "%s" % (obj.VertRapid)
        attrs[ToolControllerTemplate.HorizRapid] = "%s" % (obj.HorizRapid)
        attrs[ToolControllerTemplate.SpindleSpeed] = obj.SpindleSpeed
        attrs[ToolControllerTemplate.SpindleDir] = obj.SpindleDir
        attrs[ToolControllerTemplate.Tool] = obj.Tool.Proxy.templateAttrs(obj.Tool)
        expressions = []
        for expr in obj.ExpressionEngine:
            Path.Log.debug("%s: %s" % (expr[0], expr[1]))
            expressions.append(
                {
                    ToolControllerTemplate.ExprProp: expr[0],
                    ToolControllerTemplate.ExprExpr: expr[1],
                }
            )
        if expressions:
            attrs[ToolControllerTemplate.Expressions] = expressions
        return attrs

    def execute(self, obj):
        Path.Log.track(obj.Name)

        args = {
            "toolnumber": obj.ToolNumber,
            "toollabel": obj.Label,
            "spindlespeed": obj.SpindleSpeed,
            "spindledirection": toolchange.SpindleDirection.OFF,
        }

        if hasattr(obj.Tool, "SpindlePower"):
            if not obj.Tool.SpindlePower:
                args["spindledirection"] = toolchange.SpindleDirection.OFF
            else:
                if obj.SpindleDir == "Forward":
                    args["spindledirection"] = toolchange.SpindleDirection.CW
                else:
                    args["spindledirection"] = toolchange.SpindleDirection.CCW

        elif obj.SpindleDir == "None":
            args["spindledirection"] = toolchange.SpindleDirection.OFF
        else:
            if obj.SpindleDir == "Forward":
                args["spindledirection"] = toolchange.SpindleDirection.CW
            else:
                args["spindledirection"] = toolchange.SpindleDirection.CCW

        commands = toolchange.generate(**args)

        path = Path.Path(commands)
        obj.Path = path
        if obj.ViewObject:
            obj.ViewObject.Visibility = True

    def getTool(self, obj):
        """returns the tool associated with this tool controller"""
        Path.Log.track()
        return obj.Tool

    def ensureToolBit(self, obj):
        if not hasattr(obj, "Tool"):
            obj.addProperty(
                "App::PropertyLink",
                "Tool",
                "Base",
                QT_TRANSLATE_NOOP("App::Property", "The tool used by this controller"),
            )


def Create(
    name="TC: Default Tool",
    tool=None,
    toolNumber=1,
    assignViewProvider=True,
    assignTool=True,
):

    Path.Log.track(name, tool, toolNumber, assignViewProvider, assignTool)

    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Label = name
    obj.Proxy = ToolController(obj, assignTool)

    if FreeCAD.GuiUp and assignViewProvider:
        from Path.Tool.Gui.Controller import ViewProvider

        ViewProvider(obj.ViewObject)

    if assignTool:
        if not tool:
            tool = PathToolBit.Factory.Create()
            if tool.ViewObject:
                tool.ViewObject.Visibility = False
        obj.Tool = tool

        if hasattr(obj.Tool, "SpindleDirection"):
            obj.SpindleDir = obj.Tool.SpindleDirection

    obj.ToolNumber = toolNumber
    return obj


def FromTemplate(template, assignViewProvider=True):
    Path.Log.track()

    name = template.get(ToolControllerTemplate.Name, ToolControllerTemplate.Label)
    obj = Create(name, assignViewProvider=True, assignTool=False)
    obj.Proxy.setFromTemplate(obj, template)
    if obj.Tool:
        return obj
    FreeCAD.ActiveDocument.removeObject(obj.Name)
    return None


FreeCAD.Console.PrintLog("Loading Path.Tool.Gui.Controller... done\n")
