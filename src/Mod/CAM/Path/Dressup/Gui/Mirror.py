# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import FreeCADGui
import Path
import PathScripts.PathUtils as PathUtils

from PySide.QtCore import QT_TRANSLATE_NOOP

import re

__doc__ = """Mirror Dressup object. This dressup create mirrored path."""


translate = FreeCAD.Qt.translate


class ObjectDressup:
    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base path to modify"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "MirrorAxis",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The input mirror axis"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "MirrorOffsetX",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The offset for mirror axis"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "MirrorOffsetY",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The offset for mirror axis"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "CenterModel",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The offset for mirror at the center of base model"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "RemoveOriginalPath",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The offset for mirror at the center of base model"),
        )
        obj.MirrorAxis = ["X", "Y", "XY"]
        obj.MirrorAxis = "Y"
        obj.MirrorOffsetX = 0
        obj.MirrorOffsetY = 0
        obj.CenterModel = False
        obj.RemoveOriginalPath = True
        obj.Proxy = self

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDocumentRestored(self, obj):
        """onDocumentRestored(obj) ... Called automatically when document is restored."""
        return None

    def execute(self, obj):
        if not obj.Base:
            obj.Path = Path.Path()
            return

        if not obj.Base.isDerivedFrom("Path::Feature"):
            obj.Path = Path.Path()
            return

        if not obj.Base.Path:
            obj.Path = Path.Path()
            return

        if not obj.Base.Path.Commands:
            obj.Path = Path.Path()
            return

        offsetX = obj.MirrorOffsetX.Value
        offsetY = obj.MirrorOffsetY.Value

        # Calculate offset for center of model
        if obj.CenterModel:
            # If possible get model from Base object
            model = self.getBaseModel(obj)
            if not model:
                # Otherwise get first model from Model group of the Job
                job = PathUtils.findParentJob(obj)
                model = job.Model.Group[0]
            bounbox = model.Shape.BoundBox
            offsetX += bounbox.XMax + model.Placement.Base.x
            offsetY += bounbox.YMax + model.Placement.Base.y

        mirrorAxis = obj.MirrorAxis
        pathlist = PathUtils.getPathWithPlacement(obj.Base).Commands
        if obj.RemoveOriginalPath:
            newcommandlist = []
        else:
            newcommandlist = PathUtils.getPathWithPlacement(obj.Base).Commands

        for cmd in pathlist:
            if not re.search(r"^G0?[0123]$", cmd.Name, re.IGNORECASE):
                # Add command without move
                newcommandlist.append(cmd)
            else:
                if cmd.x is not None and mirrorAxis in ["Y", "XY"]:
                    # Process X move
                    cmd.x = -cmd.x + offsetX

                if cmd.y is not None and mirrorAxis in ["X", "XY"]:
                    # Process Y move
                    cmd.y = -cmd.y + offsetY

                if cmd.i is not None and mirrorAxis in ["Y", "XY"]:
                    # Process I (X offset) from Arc move
                    cmd.i = -cmd.i

                if cmd.j is not None and mirrorAxis in ["X", "XY"]:
                    # Process J (Y offset) from Arc move
                    cmd.j = -cmd.j

                if re.search(r"^G0?[23]$", cmd.Name, re.IGNORECASE):
                    # Change direction of Arc move
                    if mirrorAxis != "XY":
                        cmd.Name = "G2" if cmd.Name == "G3" else "G3"

                newcommandlist.append(cmd)

        obj.Path = Path.Path(newcommandlist)

    def getBaseModel(self, obj):
        def recursiveSearch(obj):
            if not hasattr(obj, "Base") or not obj.Base:
                return None
            if isinstance(obj.Base, list):
                return obj.Base[0]
            else:
                return recursiveSearch(obj.Base)

        result = recursiveSearch(obj)

        if isinstance(result, tuple) and len(result) > 0:
            if result[0].isDerivedFrom("Part::Feature"):
                return result[0]
        return None

    def onChanged(self, obj, prop):
        if hasattr(obj, "MirrorOffsetX") and hasattr(obj, "MirrorOffsetX"):
            obj.setEditorMode("MirrorOffsetX", 0)  # show
            obj.setEditorMode("MirrorOffsetY", 0)  # show
            if obj.MirrorAxis == "Y":
                obj.setEditorMode("MirrorOffsetY", 2)  # hide
            elif obj.MirrorAxis == "X":
                obj.setEditorMode("MirrorOffsetX", 2)  # hide
            return None


class ViewProviderDressup:
    def __init__(self, vobj):
        self.obj = vobj.Object

    def attach(self, vobj):
        self.obj = vobj.Object
        if self.obj and self.obj.Base:
            for i in self.obj.Base.InList:
                if hasattr(i, "Group"):
                    group = i.Group
                    for g in group:
                        if g.Name == self.obj.Base.Name:
                            group.remove(g)
                    i.Group = group
        return

    def unsetEdit(self, vobj, mode=0):
        return False

    def setEdit(self, vobj, mode=0):
        return True

    def claimChildren(self):
        return [self.obj.Base]

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDelete(self, arg1=None, arg2=None):
        """this makes sure that the base operation is added back to the project and visible"""
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(arg1.Object)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True


class CommandPathDressup:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupMirror", "Mirror"),
            "Accel": "",
            "ToolTip": QT_TRANSLATE_NOOP("CAM_DressupMirror", "Mirror."),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(
                translate("CAM_Dressup", "Please select one toolpath object\n")
            )
            return
        if not selection[0].isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(
                translate("CAM_Dressup", "The selected object is not a toolpath\n")
            )
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.Mirror")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "MirrorDressup")'
        )
        FreeCADGui.doCommand("Path.Dressup.Gui.Mirror.ObjectDressup(obj)")
        FreeCADGui.doCommand("base = FreeCAD.ActiveDocument." + selection[0].Name)
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(base)")
        FreeCADGui.doCommand("obj.Base = base")

        FreeCADGui.doCommand("job.Proxy.addOperation(obj, base)")
        FreeCADGui.doCommand(
            "obj.ViewObject.Proxy = Path.Dressup.Gui.Mirror.ViewProviderDressup(obj.ViewObject)"
        )
        FreeCADGui.doCommand("Gui.ActiveDocument.getObject(base.Name).Visibility = False")

        FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()`

        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupMirror", CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup... done\n")
