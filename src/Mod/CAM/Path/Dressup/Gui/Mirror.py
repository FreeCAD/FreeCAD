# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import Constants
import FreeCAD
import FreeCADGui
import Path
import PathScripts.PathUtils as PathUtils
import Path.Dressup.Utils as PathDressup

from PySide.QtCore import QT_TRANSLATE_NOOP

__doc__ = """Mirror Dressup object. This dressup create mirrored path."""


translate = FreeCAD.Qt.translate


class ObjectDressup:
    def __init__(self, obj, base):
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
            "ReferenceOffset",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Center point of selected shape defines offset"),
        )
        obj.MirrorAxis = ("X", "Y", "XY", "None")
        self.obj = obj
        obj.Proxy = self
        obj.Base = base

        self.setEditorModes(obj)

    def dumps(self):
        return

    def loads(self, state):
        return

    def onDocumentRestored(self, obj):
        self.setEditorModes(obj)

    def onChanged(self, obj, prop):
        if prop == "CenterModel":
            self.setEditorModes(obj)
        if prop == "ReferenceOffset":
            self.setMirrorAxis(obj)
        elif prop == "Path" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def setEditorModes(self, obj):
        referenceMode = 2 if obj.CenterModel else 0
        obj.setEditorMode("ReferenceOffset", referenceMode)

    def setMirrorAxis(self, obj):
        if obj.ReferenceOffset:
            model, subNames = obj.ReferenceOffset
            if subNames:
                sub = model.Shape.getElement(subNames[0])
                bb = sub.BoundBox
            else:
                bb = model.Shape.BoundBox
            if Path.Geom.isRoughly(bb.XLength, 0) and not Path.Geom.isRoughly(bb.YLength, 0):
                obj.MirrorAxis = "Y"
            elif Path.Geom.isRoughly(bb.YLength, 0) and not Path.Geom.isRoughly(bb.XLength, 0):
                obj.MirrorAxis = "X"

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

        bb = None
        if obj.CenterModel:  # calculate offset for center of model
            baseOp = PathDressup.baseOp(obj)
            if (
                getattr(baseOp, "Base", None)
                and isinstance(baseOp.Base, (list, tuple))
                and isinstance(baseOp.Base[0], (list, tuple))
                and baseOp.Base[0][0].isDerivedFrom("Part::Feature")
            ):  # get model from base operation
                model = baseOp.Base[0][0]
            else:  # otherwise get first model from Model group of the Job
                job = PathUtils.findParentJob(obj)
                model = job.Model.Group[0]
            bb = model.Shape.BoundBox
        elif obj.ReferenceOffset:  # calculate offset for reference shape
            model, subNames = obj.ReferenceOffset
            if hasattr(model, "Shape"):
                if subNames:
                    sub = model.Shape.getElement(subNames[0])
                    bb = sub.BoundBox
                else:
                    bb = model.Shape.BoundBox

        offsetX = obj.Offset.x
        offsetY = obj.Offset.y
        offsetZ = obj.Offset.z
        if bb:
            if obj.MirrorAxis == "Y":
                offsetX += 2 * bb.Center.x
            elif obj.MirrorAxis == "X":
                offsetY += 2 * bb.Center.y
            else:
                offsetX += 2 * bb.Center.x
                offsetY += 2 * bb.Center.y

        commands = PathUtils.getPathWithPlacement(obj.Base).Commands
        for cmd in commands:
            if cmd.Name not in Constants.GCODE_MOVE_ALL:
                # command without move, change nothing
                continue
            else:
                if cmd.x is not None:
                    # process X move
                    if obj.MirrorAxis in ("Y", "XY"):
                        cmd.x = -cmd.x
                    cmd.x += offsetX

                if cmd.y is not None:
                    # process Y move
                    if obj.MirrorAxis in ("X", "XY"):
                        cmd.y = -cmd.y
                    cmd.y += offsetY

                if cmd.z is not None:
                    # process Z move
                    cmd.z += offsetZ

                if cmd.i is not None and obj.MirrorAxis in ("Y", "XY"):
                    # process I (X offset) from Arc move
                    cmd.i = -cmd.i

                if cmd.j is not None and obj.MirrorAxis in ("X", "XY"):
                    # process J (Y offset) from Arc move
                    cmd.j = -cmd.j

                if obj.MirrorAxis != "XY" and cmd.Name in Constants.GCODE_MOVE_ARC:
                    # change direction of Arc move
                    if cmd.Name in Constants.GCODE_MOVE_CCW:
                        cmd.Name = "G2"
                    else:
                        cmd.Name = "G3"

        if obj.KeepBasePath:
            obj.Path = PathUtils.getPathWithPlacement(obj.Base)
            obj.Path.addCommands(commands)
        else:
            obj.Path = Path.Path(commands)


class ViewProviderDressup:
    def __init__(self, vobj):
        self.obj = vobj.Object
        vobj.Proxy = self

    def attach(self, vobj):
        self.obj = vobj.Object
        self.panel = None

        if self.obj and self.obj.Base:
            for i in self.obj.Base.InList:
                if hasattr(i, "Group") and self.obj.Base.Name in [o.Name for o in i.Group]:
                    i.Group = [o for o in i.Group if o.Name != self.obj.Base.Name]
            if self.obj.Base.ViewObject:
                self.obj.Base.ViewObject.Visibility = False

    def claimChildren(self):
        return [self.obj.Base]

    def setEdit(self, vobj, mode=0):
        if mode == 1:
            FreeCADGui.runCommand("Std_TransformManip")
        return True

    def unsetEdit(self, vobj, mode=0):
        pass

    def onDelete(self, arg1=None, arg2=None):
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(self.obj)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, vobj, prop):
        return None

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
        FreeCADGui.doCommand(f"base = FreeCAD.ActiveDocument.getObject('{op.Name}')")
        FreeCADGui.doCommand("Path.Dressup.Gui.Mirror.Create(base)")
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()`
        FreeCAD.ActiveDocument.recompute()


def Create(baseObject, name="DressupMirror", mode=0):
    """
    Create(baseObject, name='DressupMirror', mode=0) … create mirror dressup object for the given base path.

    import Path.Dressup.Gui.Mirror as mirror
    mirror.Create(basePath)
    """
    if not baseObject.isDerivedFrom("Path::Feature"):
        Path.Log.error(translate("CAM_DressupMirror", "The selected object is not a path") + "\n")
        return None

    if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
        Path.Log.error(translate("CAM_DressupMirror", "Select a profile object"))
        return None

    FreeCAD.ActiveDocument.openTransaction("Create a DressupMirror")
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    ObjectDressup(obj, baseObject)
    job = PathUtils.findParentJob(baseObject)
    job.Proxy.addOperation(obj, baseObject)
    ViewProviderDressup(obj.ViewObject)
    FreeCAD.ActiveDocument.commitTransaction()
    obj.ViewObject.Document.setEdit(obj.ViewObject, mode)

    return obj


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupMirror", CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup... done\n")
