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
import Path.Dressup.Utils as PathDressup

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

        obj.MirrorAxis = ("X", "Y", "XY")
        obj.MirrorAxis = "Y"
        obj.Proxy = self
        self.setEditorModes(obj)

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDocumentRestored(self, obj):
        self.setEditorModes(obj)

    def onChanged(self, obj, prop):
        if prop == "MirrorAxis":
            self.setEditorModes(obj)

        elif prop == "Path" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def setEditorModes(self, obj):
        offsetXMode = 2 if obj.MirrorAxis == "X" else 0
        offsetYMode = 2 if obj.MirrorAxis == "Y" else 0
        obj.setEditorMode("MirrorOffsetX", offsetXMode)
        obj.setEditorMode("MirrorOffsetY", offsetYMode)

    def execute(self, obj):
        if not obj.Base:
            obj.Path = Path.Path()
            Path.Log.warning(translate("MirrorDressup", "No base operation"))
            return

        if not obj.Base.isDerivedFrom("Path::Feature"):
            obj.Path = Path.Path()
            Path.Log.warning(
                translate("MirrorDressup", "Base object '%s' is not derived from Path::Feature")
                % obj.Base.Label
            )
            return

        if not obj.Base.Path.Commands:
            obj.Path = Path.Path()
            Path.Log.warning(
                translate("MirrorDressup", "Base operation '%s' with empty path") % obj.Base.Label
            )
            return

        offsetX = obj.MirrorOffsetX.Value
        offsetY = obj.MirrorOffsetY.Value

        # Calculate offset for center of model
        if obj.CenterModel:
            # if possible get model from base operation
            baseOp = self.getBaseOperation(obj)
            if (
                isinstance(baseOp.Base, (list, tuple))
                and baseOp.Base
                and isinstance(baseOp.Base[0], (list, tuple))
                and baseOp.Base[0]
                and baseOp.Base[0][0].isDerivedFrom("Part::Feature")
            ):
                model = baseOp.Base[0][0]
            else:
                # otherwise get first model from Model group of the Job
                job = PathUtils.findParentJob(obj)
                model = job.Model.Group[0]

            offsetX += model.Shape.BoundBox.XMax + model.Placement.Base.x
            offsetY += model.Shape.BoundBox.YMax + model.Placement.Base.y

        mirrorAxis = obj.MirrorAxis
        commandlist = PathUtils.getPathWithPlacement(obj.Base).Commands
        for cmd in commandlist:
            if not re.search(r"^G0?[0123]$", cmd.Name, re.IGNORECASE):
                # command without move, change nothing
                continue
            else:
                if cmd.x is not None and mirrorAxis in ("Y", "XY"):
                    # process X move
                    cmd.x = -cmd.x + offsetX

                if cmd.y is not None and mirrorAxis in ("X", "XY"):
                    # process Y move
                    cmd.y = -cmd.y + offsetY

                if cmd.i is not None and mirrorAxis in ("Y", "XY"):
                    # process I (X offset) from Arc move
                    cmd.i = -cmd.i

                if cmd.j is not None and mirrorAxis in ("X", "XY"):
                    # process J (Y offset) from Arc move
                    cmd.j = -cmd.j

                if re.search(r"^G0?[23]$", cmd.Name, re.IGNORECASE):
                    # phange direction of Arc move
                    if mirrorAxis != "XY":
                        cmd.Name = "G2" if cmd.Name == "G3" else "G3"

        obj.Path = Path.Path(commandlist)

    def getBaseOperation(self, obj):
        if not obj.isDerivedFrom("Path::Feature"):
            return None
        elif "Dressup" in obj.Name and hasattr(obj, "Base"):
            return self.getBaseOperation(obj.Base)

        return obj


class ViewProviderDressup:
    def __init__(self, vobj):
        self.obj = vobj.Object

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object
        self.panel = None

    def unsetEdit(self, vobj, mode=0):
        return False

    def setEdit(self, vobj, mode=0):
        return True

    def claimChildren(self):
        if hasattr(self.obj.Base, "InList"):
            for i in self.obj.Base.InList:
                if hasattr(i, "Group"):
                    group = i.Group
                    for g in group:
                        if g.Name == self.obj.Base.Name:
                            group.remove(g)
                    i.Group = group
                    print(i.Group)
        return [self.obj.Base]

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDelete(self, arg1=None, arg2=None):
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(self.obj)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True

    def getIcon(self):
        if getattr(PathDressup.baseOp(self.obj), "Active", True):
            return ":/icons/CAM_Dressup.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


class CommandPathDressup:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupMirror", "Mirror"),
            "Accel": "",
            "ToolTip": QT_TRANSLATE_NOOP("CAM_DressupMirror", "Creates mirror of a selected path"),
        }

    def IsActive(self):
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            return False
        if not selection[0].isDerivedFrom("Path::Feature"):
            return False

        return True

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
        FreeCADGui.doCommand("baseOp = FreeCAD.ActiveDocument." + selection[0].Name)
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(baseOp)")
        FreeCADGui.doCommand("obj.Base = baseOp")
        FreeCADGui.doCommand("job.Proxy.addOperation(obj, baseOp)")
        FreeCADGui.doCommand(
            "obj.ViewObject.Proxy = Path.Dressup.Gui.Mirror.ViewProviderDressup(obj.ViewObject)"
        )
        FreeCADGui.doCommand("baseOp.Visibility = False")
        FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()`
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupMirror", CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup... done\n")
