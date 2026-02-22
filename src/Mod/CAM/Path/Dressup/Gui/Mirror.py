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

__doc__ = """Mirror Dressup object. This dressup create mirrored path."""


translate = FreeCAD.Qt.translate


class ObjectDressup:
    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base path for mirroring"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "MirrorAxis",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The mirroring axis"),
        )
        obj.addProperty(
            "App::PropertyVectorDistance",
            "Offset",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Offset for the mirroring axis "),
        )
        obj.addProperty(
            "App::PropertyBool",
            "CenterModel",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Mirroring at the center of base model"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "KeepBasePath",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Add path from base operation"),
        )
        obj.addProperty(
            "App::PropertyLinkSubGlobal",
            "Reference",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Define the reference edge or plane for mirroring"),
        )
        obj.MirrorAxis = ("X", "Y", "XY", "Reference", "None")
        obj.Proxy = self
        self.setEditorModes(obj)

    def dumps(self):
        return

    def loads(self, state):
        return

    def onDocumentRestored(self, obj):
        self.setEditorModes(obj)

    def onChanged(self, obj, prop):
        if prop == "MirrorAxis":
            self.setEditorModes(obj)
        elif prop == "Path" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def setEditorModes(self, obj):
        centerModelMode = 2 if obj.MirrorAxis in ("None", "Reference") else 0
        keepBasePathMode = 2 if obj.MirrorAxis == "None" else 0
        referenceMode = 0 if obj.MirrorAxis == "Reference" else 2

        obj.setEditorMode("CenterModel", centerModelMode)
        obj.setEditorMode("KeepBasePath", keepBasePathMode)
        obj.setEditorMode("Reference", referenceMode)

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

        if obj.MirrorAxis == "None":
            obj.Path = obj.Base.Path
            return

        offsetX = obj.Offset.x
        offsetY = obj.Offset.y
        offsetZ = obj.Offset.z

        if obj.MirrorAxis == "Reference":
            if not obj.Reference or not obj.Reference[1]:
                obj.Path = obj.Base.Path
                return
            (model, subName) = obj.Reference
            sub = model.Shape.getElement(subName[0])
            bb = sub.BoundBox
            if Path.Geom.isRoughly(bb.XLength, 0):
                mirrorAxis = "Y"
                offsetX += 2 * bb.XMin
            elif Path.Geom.isRoughly(bb.YLength, 0):
                mirrorAxis = "X"
                offsetY += 2 * bb.YMin
        else:
            mirrorAxis = obj.MirrorAxis

        # Calculate offset for center of model
        if obj.CenterModel and obj.MirrorAxis != "Reference":
            # if possible get model from base operation
            baseOp = PathDressup.baseOp(obj)
            if (
                hasattr(baseOp, "Base")
                and isinstance(baseOp.Base, (list, tuple))
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

        commands = PathUtils.getPathWithPlacement(obj.Base).Commands
        for cmd in commands:
            if cmd.Name not in Path.Geom.CmdMoveAll:
                # command without move, change nothing
                continue
            else:
                if cmd.x is not None:
                    # process X move
                    if mirrorAxis in ("Y", "XY"):
                        cmd.x = -cmd.x
                    cmd.x += offsetX

                if cmd.y is not None:
                    # process Y move
                    if mirrorAxis in ("X", "XY"):
                        cmd.y = -cmd.y
                    cmd.y += offsetY

                if cmd.z is not None:
                    # process Y move
                    cmd.z += offsetZ

                if cmd.i is not None and mirrorAxis in ("Y", "XY"):
                    # process I (X offset) from Arc move
                    cmd.i = -cmd.i

                if cmd.j is not None and mirrorAxis in ("X", "XY"):
                    # process J (Y offset) from Arc move
                    cmd.j = -cmd.j

                if cmd.Name in Path.Geom.CmdMoveArc and mirrorAxis != "XY":
                    # change direction of Arc move
                    cmd.Name = "G2" if cmd.Name in Path.Geom.CmdMoveCCW else "G3"

        if obj.KeepBasePath:
            obj.Path = PathUtils.getPathWithPlacement(obj.Base)
            obj.Path.addCommands(commands)
        else:
            obj.Path = Path.Path(commands)


class ViewProviderDressup:
    def __init__(self, vobj):
        self.attach(vobj)
        vobj.Proxy = self

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object

    def dumps(self):
        return

    def loads(self, state):
        return

    def onChanged(self, vobj, prop):
        return

    def claimChildren(self):
        if hasattr(self.obj.Base, "InList"):
            for i in self.obj.Base.InList:
                if hasattr(i, "Group"):
                    group = i.Group
                    for g in group:
                        if g.Name == self.obj.Base.Name:
                            group.remove(g)
                    i.Group = group
        return [self.obj.Base]

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
        return bool(PathDressup.selection())

    def Activated(self):
        # check that the selection contains exactly what we want
        op = PathDressup.selection(verbose=True)
        if not op:
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.Mirror")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "MirrorDressup")'
        )
        FreeCADGui.doCommand("Path.Dressup.Gui.Mirror.ObjectDressup(obj)")
        FreeCADGui.doCommand("baseOp = FreeCAD.ActiveDocument." + op.Name)
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(baseOp)")
        FreeCADGui.doCommand("obj.Base = baseOp")
        FreeCADGui.doCommand("job.Proxy.addOperation(obj, baseOp)")
        FreeCADGui.doCommand("Path.Dressup.Gui.Mirror.ViewProviderDressup(obj.ViewObject)")
        FreeCADGui.doCommand("baseOp.Visibility = False")
        FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()`
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupMirror", CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup... done\n")
