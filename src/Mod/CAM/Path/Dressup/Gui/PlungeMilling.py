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
import Part
import Path
import PathScripts.PathUtils as PathUtils
import Path.Dressup.Utils as PathDressup
from Path.Base.MachineState import MachineState

from PySide.QtCore import QT_TRANSLATE_NOOP

import math

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
        retractHeight = baseOp.SafeHeight.Value
        toolController = baseOp.ToolController
        step = toolController.Tool.Diameter.Value * obj.StepOver / 100
        vertFeed = toolController.VertFeed.Value

        last = None
        machine = MachineState()
        commands = []
        for i, cmd in enumerate(PathUtils.getPathWithPlacement(obj.Base).Commands):
            print(i, cmd)
            if cmd.Name not in Constants.GCODE_MOVE_MILL:
                commands.append(cmd)

            else:
                position = machine.getPosition()
                edge = Path.Geom.edgeForCmd(cmd, position)
                if not edge:
                    continue

                if Path.Geom.isVertical(edge):
                    # commands.append(cmd)
                    machine.addCommand(cmd)
                    continue

                if False and isinstance(edge.Curve, Part.Circle):
                    # split arc by two parts to get extra point in the middle of arc
                    firstPar = edge.FirstParameter
                    midPar = firstPar + (edge.LastParameter - edge.FirstParameter) / 2
                    lastPar = edge.LastParameter
                    e0 = Part.Arc(edge.Curve, firstPar, midPar).toShape()
                    e1 = Part.Arc(edge.Curve, midPar, lastPar).toShape()
                    edges = [e0, e1]
                else:
                    edges = [edge]

                for e in edges:
                    number = math.ceil(round(e.Length / step, 6)) + 1
                    if number >= 2:
                        points = e.discretize(Number=number)
                    else:
                        points = [e.Vertexes[0].Point, e.Vertexes[-1].Point]

                    for p in points:
                        if last and Path.Geom.pointsCoincide(p, last, step / 2):
                            # skip too close point
                            continue
                        commands.append(Path.Command("G0", {"X": p.x, "Y": p.y}))
                        commands.append(Path.Command("G1", {"Z": p.z, "F": vertFeed}))
                        commands.append(Path.Command("G0", {"Z": retractHeight}))
                        last = p

            machine.addCommand(cmd)

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
        FreeCADGui.doCommand(
            "Path.Dressup.Gui.PlungeMilling.Create(App.ActiveDocument.%s)" % op.Name
        )
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


def Create(baseObject, name="DressupPlungeMilling", mode=0):
    """
    Create(basePath, name='DressupPlungeMilling') … create Plunge Milling dressup object for the given base path.

    import Path.Dressup.Gui.PlungeMilling as plunge
    plunge.Create(basePath)  # to show Task panel
    plunge.Create(basePath, 2)  # to skip Task panel
    """
    if not baseObject.isDerivedFrom("Path::Feature"):
        Path.Log.error(
            translate("CAM_DressupPlungeMilling", "The selected object is not a path") + "\n"
        )
        return None

    if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
        Path.Log.error(translate("CAM_DressupPlungeMilling", "Select a profile object"))
        return None

    FreeCAD.ActiveDocument.openTransaction("Create a DressupPlungeMilling")
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
    FreeCADGui.addCommand("CAM_DressupPlungeMilling", CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup... done\n")
