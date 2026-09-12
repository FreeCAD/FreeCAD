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
import Part
import Path
import Path.Dressup.Utils as PathDressup
from Path.Base import FeedRate
from Path.Base.Generator import linking
import Path.Base.MachineState as PathMachineState
from PathScripts import PathUtils
from PySide.QtCore import QT_TRANSLATE_NOOP

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


class ObjectDressup:
    def __init__(self, obj, base):
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base toolpath to modify"),
        )
        obj.addProperty(
            "App::PropertyLink",
            "Boundary",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Shape to limit toolpath"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "Side",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Keep path inside or outside the shape"),
        )
        obj.addProperty(
            "App::PropertyLength",
            "RetractThreshold",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Set distance which will attempts to avoid unnecessary retractions",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "RestMachiningPass",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Apply boundary to Rest Machining"
                "Does not affects to current path, but can be used by other operations"
                " with Rest Machining feature",
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "Offset",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Apply offset to boundary shape"),
        )

        self.obj = obj
        obj.Proxy = self
        obj.Base = base
        obj.Side = ("Inside", "Outside")

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, obj, prop):
        if prop == "Path" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def onDocumentRestored(self, obj):
        pass

    def execute(self, obj):
        if not obj.Base or not obj.Base.isDerivedFrom("Path::Feature") or not obj.Base.Path:
            obj.Path = Path.Path()
            return

        if not PathDressup.baseOp(obj.Base).Active:
            obj.Path = Path.Path("(inactive operation)")
            return

        if not getattr(obj, "Boundary", None):
            Path.Log.warning(translate("CAM_DressupPathBoundary2", "Boundary model missing"))
            obj.Path = Path.Path()
            return
        if not getattr(obj.Boundary, "Shape", None) or obj.Boundary.Shape.isNull():
            Path.Log.warning(translate("CAM_DressupPathBoundary2", "Boundary has no shape"))
            obj.Path = Path.Path()
            return
        if not obj.Boundary.Shape.Solids:
            Path.Log.warning(translate("CAM_DressupPathBoundary2", "Boundary shape is not a solid"))
            obj.Path = Path.Path()
            return

        if obj.Offset:
            offset = obj.Offset
            if obj.Side == "Inside":
                offset = -offset
            shapes = Path.Geom.uncompound(obj.Boundary.Shape)
            boundaryShapes = [sh.makeOffsetShape(offset, tolerance=0.1, join=2) for sh in shapes]
        else:
            boundaryShapes = [obj.Boundary.Shape]

        baseOp = PathDressup.baseOp(obj)
        job = PathUtils.findParentJob(obj)
        clearanceHeight = baseOp.ClearanceHeight.Value
        safeHeight = baseOp.SafeHeight.Value
        collision_clearance = 0.1
        if hasattr(baseOp, "CollisionClearance"):
            collision_clearance = baseOp.CollisionClearance.Value

        wires = Path.Geom.wiresForPath(PathUtils.getPathWithPlacement(obj.Base))
        boundaryWires = []
        for wire in wires:
            if obj.Side == "Inside":
                boundaryWires.extend(wire.common(boundaryShapes).Wires)
            else:
                boundaryWires.extend(wire.cut(boundaryShapes).Wires)

        if not boundaryWires:
            Path.Log.warning(translate("CAM_DressupPathBoundary2", "No path with such boundary"))

        linkingArgs = {
            "start_position": None,
            "target_position": None,
            "heights_clearance": (safeHeight, clearanceHeight),
            "solids": [base.Shape for base in job.Model.Group],
            "tool_shape": None,
            "tool_diameter": None,
            "collision_clearance": collision_clearance,
        }

        machinestate = PathMachineState.MachineState()
        commands = []
        first = True
        for wire in boundaryWires:
            # get clusters, because Edges order in Wire may changed while cutting
            for cluster in Part.getSortedClusters(wire.Edges):
                sortedEdges = Part.__sortEdges__(cluster)
                p = sortedEdges[0].valueAt(sortedEdges[0].FirstParameter)
                if first:
                    first = False
                    commands.append(Path.Command("G0", {"Z": clearanceHeight}))
                    commands.append(Path.Command("G0", {"X": p.x, "Y": p.y}))
                    commands.append(Path.Command("G0", {"Z": safeHeight}))
                    commands.append(Path.Command("G1", {"X": p.x, "Y": p.y, "Z": p.z}))
                elif (p - machinestate.getPosition()).Length <= obj.RetractThreshold:
                    commands.append(Path.Command("G1", {"X": p.x, "Y": p.y, "Z": p.z}))
                else:
                    linkingArgs["start_position"] = machinestate.getPosition()
                    linkingArgs["target_position"] = FreeCAD.Vector(p.x, p.y, p.z)
                    linkingMoves = linking.get_linking_moves(**linkingArgs)
                    for cmd in linkingMoves:
                        if cmd.z < safeHeight:
                            cmd.Name = "G1"
                    commands.extend(linkingMoves)

                for edge in sortedEdges:
                    cmds = Path.Geom.cmdsForEdge(edge)
                    commands.extend(cmds)
                    machinestate.addCommands(commands[-1])

        commands.append(Path.Command("G0", {"Z": clearanceHeight}))
        FeedRate.setFeedRate(commands, baseOp.ToolController)
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


def Create(baseObject, name="DressupPathBoundary", mode=0):
    """
    Create(baseObject, name='DressupPathBoundary', mode=0) … create dressup object for the given base path.

    import Path.Dressup.Gui.Boundary2 as boundary
    boundary.Create(basePath)
    """
    if not baseObject.isDerivedFrom("Path::Feature"):
        Path.Log.error(
            translate("CAM_DressupPathBoundary2", "The selected object is not a path") + "\n"
        )
        return None

    if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
        Path.Log.error(translate("CAM_DressupPathBoundary2", "Select a profile object"))
        return None

    FreeCAD.ActiveDocument.openTransaction("Create a DressupPathBoundary")
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    ObjectDressup(obj, baseObject)
    job = PathUtils.findParentJob(baseObject)
    job.Proxy.addOperation(obj, baseObject)
    ViewProviderDressup(obj.ViewObject)
    FreeCAD.ActiveDocument.commitTransaction()
    obj.ViewObject.Document.setEdit(obj.ViewObject, mode)

    return obj


class CommandPathDressup:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupPathBoundary2", "Boundary2"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_DressupPathBoundary2",
                "Creates a boundary dress-up from a selected toolpath",
            ),
        }

    def IsActive(self):
        op = PathDressup.selection()
        if not op:
            return False
        baseOp = PathDressup.baseOp(op)
        return hasattr(baseOp, "ClearanceHeight") and hasattr(baseOp, "SafeHeight")

    def Activated(self):
        # check that the selection contains exactly what we want
        op = PathDressup.selection(verbose=True)
        if not op:
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create Path Boundary Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.Boundary2")
        FreeCADGui.doCommand(f"base = FreeCAD.ActiveDocument.getObject('{op.Name}')")
        FreeCADGui.doCommand("Path.Dressup.Gui.Boundary2.Create(base)")
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()` called via TaskPanel.accept()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupPathBoundary2", CommandPathDressup())

Path.Log.notice("Loading PathDressupPathBoundaryGui... done\n")
