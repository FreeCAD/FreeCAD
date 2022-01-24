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
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathToolBit as PathToolBit


if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

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
    def __init__(self, obj, legacyTool=False, createTool=True):
        PathLog.track("tool: {}".format(legacyTool))

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
            self.ensureUseLegacyTool(obj, legacyTool)

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
            ],  # this is the direction that the profile runs
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        PathLog.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        PathLog.debug(data)

        return data

    def onDocumentRestored(self, obj):
        obj.setEditorMode("Placement", 2)

    def onDelete(self, obj, arg2=None):
        # pylint: disable=unused-argument
        if not self.usesLegacyTool(obj):
            if hasattr(obj.Tool, "InList") and len(obj.Tool.InList) == 1:
                if hasattr(obj.Tool.Proxy, "onDelete"):
                    obj.Tool.Proxy.onDelete(obj.Tool)

    def setFromTemplate(self, obj, template):
        """
        setFromTemplate(obj, xmlItem) ... extract properties from xmlItem
        and assign to receiver.
        """
        PathLog.track(obj.Name, template)
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
                    toolVersion = template.get(ToolControllerTemplate.Tool).get(
                        ToolControllerTemplate.Version
                    )
                    if toolVersion == 1:
                        self.ensureUseLegacyTool(obj, True)
                        obj.Tool.setFromTemplate(
                            template.get(ToolControllerTemplate.Tool)
                        )
                    else:
                        self.ensureUseLegacyTool(obj, False)
                        obj.Tool = PathToolBit.Factory.CreateFromAttrs(
                            template.get(ToolControllerTemplate.Tool)
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
                PathLog.error(
                    "Unsupported PathToolController template version {}".format(
                        template.get(ToolControllerTemplate.Version)
                    )
                )
        else:
            PathLog.error(
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
        if self.usesLegacyTool(obj):
            attrs[ToolControllerTemplate.Tool] = obj.Tool.templateAttrs()
        else:
            attrs[ToolControllerTemplate.Tool] = obj.Tool.Proxy.templateAttrs(obj.Tool)
        expressions = []
        for expr in obj.ExpressionEngine:
            PathLog.debug("%s: %s" % (expr[0], expr[1]))
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
        PathLog.track()

        commands = ""
        commands += "(" + obj.Label + ")" + "\n"
        commands += "M6 T" + str(obj.ToolNumber) + "\n"

        # If a toolbit is used, check to see if spindlepower is allowed.
        # This is to prevent accidentally spinning the spindle with an
        # unpowered tool like probe or dragknife

        allowSpindlePower = True
        if not isinstance(obj.Tool, Path.Tool) and hasattr(obj.Tool, "SpindlePower"):
            allowSpindlePower = obj.Tool.SpindlePower

        if allowSpindlePower:
            PathLog.debug("selected tool preventing spindle power")
            if obj.SpindleDir == "Forward":
                commands += "M3 S" + str(obj.SpindleSpeed) + "\n"
            else:
                commands += "M4 S" + str(obj.SpindleSpeed) + "\n"

        if commands == "":
            commands += "(No commands processed)"

        path = Path.Path(commands)
        obj.Path = path
        if obj.ViewObject:
            obj.ViewObject.Visibility = True

    def getTool(self, obj):
        """returns the tool associated with this tool controller"""
        PathLog.track()
        return obj.Tool

    def usesLegacyTool(self, obj):
        """returns True if the tool being controlled is a legacy tool"""
        return isinstance(obj.Tool, Path.Tool)

    def ensureUseLegacyTool(self, obj, legacy):
        if not hasattr(obj, "Tool") or (legacy != self.usesLegacyTool(obj)):
            if legacy and hasattr(obj, "Tool") and len(obj.Tool.InList) == 1:
                if hasattr(obj.Tool.Proxy, "onDelete"):
                    obj.Tool.Proxy.onDelete(obj.Tool)
                obj.Document.removeObject(obj.Tool.Name)

            if hasattr(obj, "Tool"):
                obj.removeProperty("Tool")

            if legacy:
                obj.addProperty(
                    "Path::PropertyTool",
                    "Tool",
                    "Base",
                    QT_TRANSLATE_NOOP(
                        "App::Property", "The tool used by this controller"
                    ),
                )
            else:
                obj.addProperty(
                    "App::PropertyLink",
                    "Tool",
                    "Base",
                    QT_TRANSLATE_NOOP(
                        "App::Property", "The tool used by this controller"
                    ),
                )


def Create(
    name="TC: Default Tool",
    tool=None,
    toolNumber=1,
    assignViewProvider=True,
    assignTool=True,
):
    legacyTool = (
        PathPreferences.toolsUseLegacyTools()
        if tool is None
        else isinstance(tool, Path.Tool)
    )

    PathLog.track(tool, toolNumber, legacyTool)

    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Label = name
    obj.Proxy = ToolController(obj, legacyTool, assignTool)

    if FreeCAD.GuiUp and assignViewProvider:
        ViewProvider(obj.ViewObject)

    if assignTool:
        if not tool:
            if legacyTool:
                tool = Path.Tool()
                tool.Diameter = 5.0
                tool.Name = "Default Tool"
                tool.CuttingEdgeHeight = 15.0
                tool.ToolType = "EndMill"
                tool.Material = "HighSpeedSteel"
            else:
                tool = PathToolBit.Factory.Create()
                if tool.ViewObject:
                    tool.ViewObject.Visibility = False
        obj.Tool = tool
    obj.ToolNumber = toolNumber
    return obj


def FromTemplate(template, assignViewProvider=True):
    # pylint: disable=unused-argument
    PathLog.track()

    name = template.get(ToolControllerTemplate.Name, ToolControllerTemplate.Label)
    obj = Create(name, assignViewProvider=True, assignTool=False)
    obj.Proxy.setFromTemplate(obj, template)

    return obj


if FreeCAD.GuiUp:
    # need ViewProvider class in this file to support loading of old files
    from PathScripts.PathToolControllerGui import ViewProvider

FreeCAD.Console.PrintLog("Loading PathToolController... done\n")
