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

""" Used to create CNC machine fixture offsets such as G54,G55, etc..."""

import FreeCAD
import FreeCADGui
import Path
import PathScripts.PathUtils as PathUtils
from PySide.QtCore import QT_TRANSLATE_NOOP


class Fixture:
    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyEnumeration",
            "Fixture",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Fixture Offset Number"),
        )
        obj.Fixture = [
            "G54",
            "G55",
            "G56",
            "G57",
            "G58",
            "G59",
            "G59.1",
            "G59.2",
            "G59.3",
            "G59.4",
            "G59.5",
            "G59.6",
            "G59.7",
            "G59.8",
            "G59.9",
        ]
        obj.addProperty(
            "App::PropertyBool",
            "Active",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "Make False, to prevent operation from generating code"
            ),
        )

        obj.Proxy = self

    def execute(self, obj):
        fixlist = [
            "G54",
            "G55",
            "G56",
            "G57",
            "G58",
            "G59",
            "G59.1",
            "G59.2",
            "G59.3",
            "G59.4",
            "G59.5",
            "G59.6",
            "G59.7",
            "G59.8",
            "G59.9",
        ]
        fixture = fixlist.index(obj.Fixture)
        obj.Path = Path.Path(str(obj.Fixture))
        obj.Label = "Fixture" + str(fixture)
        if obj.Active:
            job = PathUtils.findParentJob(obj)
            c1 = Path.Command(str(obj.Fixture))
            c2 = Path.Command("G0" + str(job.Stock.Shape.BoundBox.ZMax))
            obj.Path = Path.Path([c1, c2])
            obj.ViewObject.Visibility = True
        else:
            obj.Path = Path.Path("(inactive operation)")
            obj.ViewObject.Visibility = False


class _ViewProviderFixture:
    def __init__(self, vobj):  # mandatory
        #        obj.addProperty("App::PropertyFloat","SomePropertyName","PropertyGroup","Description of this property")
        vobj.Proxy = self
        mode = 2
        vobj.setEditorMode("LineWidth", mode)
        vobj.setEditorMode("MarkerColor", mode)
        vobj.setEditorMode("NormalColor", mode)
        vobj.setEditorMode("DisplayMode", mode)
        vobj.setEditorMode("BoundingBox", mode)
        vobj.setEditorMode("Selectable", mode)
        vobj.setEditorMode("ShapeColor", mode)
        vobj.setEditorMode("Transparency", mode)
        vobj.setEditorMode("Visibility", mode)

    def dumps(self):  # mandatory
        return None

    def loads(self, state):  # mandatory
        return None

    def getIcon(self):  # optional
        return ":/icons/CAM_Datums.svg"

    def onChanged(self, vobj, prop):  # optional
        mode = 2
        vobj.setEditorMode("LineWidth", mode)
        vobj.setEditorMode("MarkerColor", mode)
        vobj.setEditorMode("NormalColor", mode)
        vobj.setEditorMode("DisplayMode", mode)
        vobj.setEditorMode("BoundingBox", mode)
        vobj.setEditorMode("Selectable", mode)
        vobj.setEditorMode("ShapeColor", mode)
        vobj.setEditorMode("Transparency", mode)
        vobj.setEditorMode("Visibility", mode)

    def updateData(self, vobj, prop):  # optional
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self, vobj, mode):  # optional
        # this is executed when the object is double-clicked in the tree
        pass

    def unsetEdit(self, vobj, mode):  # optional
        # this is executed when the user cancels or terminates edit mode
        pass


class CommandPathFixture:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Datums",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Fixture", "Fixture"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_Fixture", "Creates a Fixture Offset"
            ),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create a Fixture Offset")
        FreeCADGui.addModule("Path.Main.Gui.Fixture")
        snippet = """
import Path
import PathScripts
from PathScripts import PathUtils
prjexists = False
obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Fixture")
Path.Main.Gui.Fixture.Fixture(obj)
obj.Active = True
Path.Main.Gui.Fixture._ViewProviderFixture(obj.ViewObject)

PathUtils.addToJob(obj)

"""
        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Fixture", CommandPathFixture())


FreeCAD.Console.PrintLog("Loading PathFixture... done\n")
