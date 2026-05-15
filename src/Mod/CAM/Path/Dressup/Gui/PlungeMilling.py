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
    def __init__(self, obj):
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
        obj.StepOver = 50
        obj.Proxy = self

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
                    commands.append(cmd)
                    machine.addCommand(cmd)
                    continue

                if isinstance(edge.Curve, Part.Circle):
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
                        points = [e.discretize(Number=3)[1]]

                    for p in points:
                        if last and Path.Geom.pointsCoincide(p, last):
                            continue
                        commands.append(Path.Command("G0", {"X": p.x, "Y": p.y}))
                        commands.append(Path.Command("G1", {"Z": p.z, "F": vertFeed}))
                        commands.append(Path.Command("G0", {"Z": retractHeight}))
                        last = p

            machine.addCommand(cmd)
            if cmd.Name in Constants.GCODE_MOVE_RAPID:
                retractHeight = machine.getPosition().z

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

    def setEdit(self, vobj, mode=0):
        if mode == 1:
            FreeCADGui.runCommand("Std_TransformManip")
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
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupPlungeMilling", "Plunge Milling"),
            "Accel": "",
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_DressupPlungeMilling",
                "Creates plunge milling of a selected path",
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
        FreeCAD.ActiveDocument.openTransaction("Create Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.PlungeMilling")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PlungeMillingDressup")'
        )
        FreeCADGui.doCommand("Path.Dressup.Gui.PlungeMilling.ObjectDressup(obj)")
        FreeCADGui.doCommand("baseOp = FreeCAD.ActiveDocument." + op.Name)
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(baseOp)")
        FreeCADGui.doCommand("obj.Base = baseOp")
        FreeCADGui.doCommand("job.Proxy.addOperation(obj, baseOp)")
        FreeCADGui.doCommand("Path.Dressup.Gui.PlungeMilling.ViewProviderDressup(obj.ViewObject)")
        FreeCADGui.doCommand("baseOp.Visibility = False")
        FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()`
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupPlungeMilling", CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup... done\n")
