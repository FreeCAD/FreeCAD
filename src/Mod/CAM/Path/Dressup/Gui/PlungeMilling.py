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

import FreeCAD
import FreeCADGui
import Path
import PathScripts.PathUtils as PathUtils
import Path.Base.Generator.plunge_milling as plunge_milling
import Path.Dressup.Utils as PathDressup
from PySide.QtCore import QT_TRANSLATE_NOOP

__doc__ = """Plunge Milling Dressup object"""


translate = FreeCAD.Qt.translate


class ObjectDressup:
    def __init__(self, obj, base):
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base path"),
        )
        obj.addProperty(
            "App::PropertyPercent",
            "StepOver",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "Percent of cutter diameter to step over on each pass"
            ),
        )
        obj.StepOver = 20
        obj.Proxy = self
        obj.Base = base

    def dumps(self):
        return

    def loads(self, state):
        return

    def onDocumentRestored(self, obj):
        pass

    def onChanged(self, obj, prop):
        if prop == "Path" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def execute(self, obj):
        if not obj.Base:
            obj.Path = Path.Path()
            Path.Log.warning(translate("PlungeMillingDressup", "No base operation"))
            return

        if not obj.Base.isDerivedFrom("Path::Feature"):
            obj.Path = Path.Path()
            Path.Log.warning(
                translate(
                    "PlungeMillingDressup", "Base object '%s' is not derived from Path::Feature"
                )
                % obj.Base.Label
            )
            return

        if not obj.Base.Path.Commands:
            obj.Path = Path.Path()
            Path.Log.warning(
                translate("PlungeMillingDressup", "Base operation '%s' with empty path")
                % obj.Base.Label
            )
            return

        if obj.StepOver < 1:
            obj.StepOver = 1

        baseOp = PathDressup.baseOp(obj)
        toolController = baseOp.ToolController
        step = toolController.Tool.Diameter.Value * obj.StepOver / 100

        obj.Path = plunge_milling.get_path(
            path=PathUtils.getPathWithPlacement(obj.Base),
            step=step,
            step_min=step / 2,
            retract_height=baseOp.SafeHeight.Value,
            vert_feed=toolController.VertFeed.Value,
        )


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
        """this makes sure that the base operation is added back to the project and visible"""
        Path.Log.debug("Deleting Dressup")
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

    def clearTaskPanel(self):
        pass

    def getIcon(self):
        if getattr(PathDressup.baseOp(self.obj), "Active", True):
            return ":/icons/CAM_Dressup.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


class CommandPathDressup:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupPlungeMilling", "Plunge Milling"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_DressupPlungeMilling",
                "Creates plunge milling for a selected path",
            ),
        }

    def IsActive(self):
        return bool(PathDressup.selection())

    def Activated(self):
        # check that the selection contains exactly what we want
        op = PathDressup.selection(verbose=True)
        if not op:
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create PlungeMilling Dressup")
        FreeCADGui.addModule("Path.Dressup.Gui.PlungeMilling")
        FreeCADGui.doCommand(f"base = FreeCAD.ActiveDocument.getObject('{op.Name}')")
        FreeCADGui.doCommand("Path.Dressup.Gui.PlungeMilling.Create(base)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


def Create(base, name="DressupPlungeMilling", mode=0):
    """
    Create(basePath, name='DressupPlungeMilling') ... create Plunge Milling dressup object for the given base path.

    import Path.Dressup.Gui.PlungeMilling as plunge
    plunge.Create(basePath)
    """
    if not base.isDerivedFrom("Path::Feature"):
        Path.Log.error(
            translate("CAM_DressupPlungeMilling", "The selected object is not a path") + "\n"
        )
        return None

    if base.isDerivedFrom("Path::FeatureCompoundPython"):
        Path.Log.error(translate("CAM_DressupPlungeMilling", "Select a profile object"))
        return None

    FreeCAD.ActiveDocument.openTransaction("Create a DressupPlungeMilling")
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    ObjectDressup(obj, base)
    job = PathUtils.findParentJob(base)
    job.Proxy.addOperation(obj, base)
    ViewProviderDressup(obj.ViewObject)
    FreeCAD.ActiveDocument.commitTransaction()
    obj.ViewObject.Document.setEdit(obj.ViewObject, mode)

    return obj


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupPlungeMilling", CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup... done\n")
