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
            "InvertPathDirection",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Invert Path direction of Base operation to get correct direction after mirror",
            ),
        )
        obj.MirrorAxis = ["X", "Y", "XY"]
        obj.MirrorAxis = "Y"
        obj.MirrorOffsetX = 0
        obj.MirrorOffsetY = 0
        obj.CenterModel = False
        obj.InvertPathDirection = True
        obj.Proxy = self

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDocumentRestored(self, obj):
        """onDocumentRestored(obj) ... Called automatically when document is restored."""
        return None

    def execute(self, obj):
        baseOp = self.getBaseOperation(obj)
        if not baseOp:
            obj.Path = Path.Path()
            return

        if not baseOp.isDerivedFrom("Path::Feature"):
            obj.Path = Path.Path()
            return

        if not baseOp.Path:
            obj.Path = Path.Path()
            return

        if not baseOp.Path.Commands:
            obj.Path = Path.Path()
            return

        # Change Path direction
        if hasattr(baseOp, "Direction"):
            baseOp.clearExpression("Direction")
            origObj = self.getOriginalOperation(baseOp)
            # Change direction only if needed
            if obj.InvertPathDirection and baseOp.Direction == origObj.Direction:
                baseOp.Direction = "CCW" if origObj.Direction == "CW" else "CW"
            elif not obj.InvertPathDirection and baseOp.Direction != origObj.Direction:
                baseOp.Direction = origObj.Direction

        offsetX = obj.MirrorOffsetX.Value
        offsetY = obj.MirrorOffsetY.Value

        # Calculate offset for center of model
        if obj.CenterModel:
            model = None
            # If possible get model from Base operation in low level
            if isinstance(baseOp.Base, list) and baseOp.Base != []:
                if baseOp.Base[0][0].isDerivedFrom("Part::Feature"):
                    model = baseOp.Base[0][0]
            if not model:
                # Otherwise get first model from Model group of the Job
                job = PathUtils.findParentJob(obj)
                model = job.Model.Group[0]
            offsetX += model.Shape.BoundBox.XMax + model.Placement.Base.x
            offsetY += model.Shape.BoundBox.YMax + model.Placement.Base.y

        mirrorAxis = obj.MirrorAxis
        pathlist = PathUtils.getPathWithPlacement(obj.Base).Commands
        newcommandlist = []
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

    def recursiveSearch(self, obj):
        if not hasattr(obj, "Base"):
            return None
        elif not obj.Base:
            return obj
        elif isinstance(obj.Base, list):
            if obj.Base[0][0].isDerivedFrom("Part::Feature"):
                return obj
        elif obj.Base.isDerivedFrom("Path::Feature"):
            return self.recursiveSearch(obj.Base)
        return None

    def getBaseOperation(self, obj):
        # Get operation in low level, e.g. from bottom of Dressups
        return self.recursiveSearch(obj)

    def getOriginalOperation(self, obj):
        # Get original object from expression of copy object
        for expr in obj.ExpressionEngine:
            if expr[0] == "Label2":
                name = expr[1].split(".")[0]
                origObj = FreeCAD.ActiveDocument.getObject(name)
                return origObj

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


def createBaseCopy(obj):
    # Get recursive list of operations
    def getRecursiveOperationsList(obj):
        if not hasattr(obj, "Base"):
            return None
        if obj.isDerivedFrom("Path::Feature"):
            tree.append(obj)
            return getRecursiveOperationsList(obj.Base)
        else:
            return None

    # Set expressions for properties to link to original object
    def setProperties(newObj, origObj):
        for property in newObj.PropertiesList:
            if property == "Label":
                newObj.Label = f"{origObj.Name}_expCopy"
            # Do not change properties in list below
            elif property not in [
                "Active",
                "AreaParams",
                "Base",
                "CycleTime",
                "_ElementMapVersion",
                "ExpressionEngine",
                "PathParams",
                "removalshape",
            ]:
                expression = f"{origObj.Name}.{property}"
                newObj.setExpression(property, expression)

    tree = []
    getRecursiveOperationsList(obj)
    if len(tree) > 1:
        print(f"    Recursive list of operations: {[op.Name for op in tree]}")
    job = PathUtils.findParentJob(obj)

    temp = None
    # Create copy of objects in list
    for i, obj in enumerate(reversed(tree)):
        newObj = FreeCAD.ActiveDocument.copyObject(obj, False)
        setProperties(newObj, obj)
        # Set Base for objects, but do not touch first in the list
        if i > 0:
            newObj.Base = temp
        # Add only last object copy to Job
        if i == len(tree) - 1:
            PathUtils.addToJob(newObj, job.Name)
            break
        temp = newObj

    return newObj


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
        FreeCADGui.doCommand("orig = FreeCAD.ActiveDocument." + selection[0].Name)
        FreeCADGui.doCommand("base = Path.Dressup.Gui.Mirror.createBaseCopy(orig)")
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(base)")
        FreeCADGui.doCommand("obj.Base = base")
        FreeCADGui.doCommand("job.Proxy.addOperation(obj, base)")
        FreeCADGui.doCommand(
            "obj.ViewObject.Proxy = Path.Dressup.Gui.Mirror.ViewProviderDressup(obj.ViewObject)"
        )
        FreeCADGui.doCommand("base.Visibility = False")
        FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()`
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupMirror", CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup... done\n")
