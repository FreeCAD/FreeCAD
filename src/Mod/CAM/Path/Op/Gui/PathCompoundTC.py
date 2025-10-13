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
import Path.Op.Base as PathOp
import PathScripts.PathUtils as PathUtils
from Path.Base.Util import toolControllerForOp
from Path.Base.Util import coolantModeForOp

from PySide.QtCore import QT_TRANSLATE_NOOP

from itertools import permutations
from numpy import random
import math
import copy
import time

__title__ = "CAM Path Compound with Tool Controller"
__author__ = ""
__url__ = "https://forum.freecad.org/viewtopic.php?t=96765"
__doc__ = ""


translate = FreeCAD.Qt.translate


class ObjectCompound:
    def __init__(self, obj):
        self.Type = "ObjectCompound"
        self.obj = obj
        obj.Proxy = self
        obj.addProperty(
            "App::PropertyBool",
            "Active",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "Make False, to prevent operation from generating code"
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "Comment",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "An optional comment for this Operation"),
        )
        obj.addProperty(
            "App::PropertyString",
            "UserLabel",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "User Assigned Label"),
        )
        obj.addProperty(
            "App::PropertyLink",
            "ToolController",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The tool controller that will be used to calculate the path"
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "CycleTime",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Operations Cycle Time Estimation"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "RemoveG0X0Y0",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Attempts to avoid unnecessary movements G0 X0 Y0"),
        )
        obj.addProperty(
            "App::PropertyLength",
            "RetractThreshold",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "If end points are separated within this threshold,\nthey are consider as connected",
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "LimitRetractHeight",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Limit retract height between operations"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "Sorting",
            "Pattern",
            QT_TRANSLATE_NOOP("App::Property", "Sorting mode"),
        )
        obj.Active = True
        obj.LimitRetractHeight = 100
        obj.setEditorMode("CycleTime", 1)  # read-only
        obj.setEditorMode("ToolController", 3)  # read-only and hidden
        obj.Sorting = ("2-opt", "Brute-force", "Manual")
        self.setToolController(obj)

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, obj, prop):
        if prop == "Active" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def onDocumentRestored(self, obj):
        self.Type = "ObjectCompound"

        return None

    def execute(self, obj):
        self.setToolController(obj)
        if not obj.Active or not obj.ToolController:
            # Do not generate and clear current Path
            # if PathCompound not Active or ToolController is different in Group
            obj.Path = Path.Path()
            obj.CycleTime = "0"
        else:
            obj.Path = Compound(obj).getPath()
            obj.CycleTime = PathOp.getCycleTimeEstimate(obj)

    def setToolController(self, obj):
        if isinstance(obj.Group, list) and obj.Group:
            tc0 = toolControllerForOp(obj.Group[0])
            coolant0 = coolantModeForOp(obj.Group[0])
            for op in obj.Group[1:]:
                tc = toolControllerForOp(op)
                coolant = coolantModeForOp(op)
                if tc != tc0 or tc is None:
                    Path.Log.error(
                        translate(
                            "CAM", "Tool Controllers of the combined objects is different or None"
                        )
                    )
                    return translate("CAM", "Tool Error")
                if coolant != coolant0:
                    Path.Log.error(
                        translate("CAM", "Coolant mode of the combined objects is different")
                    )
                    return translate("CAM", "Tool Error")

            obj.ToolController = tc0

        return None


class Compound:
    def __init__(self, obj):
        self.obj = obj

    # Here we can change gcode by template
    def preprocessPath(self, pathObj):
        isFullyDefined = False  # Position of the tool is unknown in the beginning
        startX = None
        startY = None
        startZ = None

        positionPrev = FreeCAD.Vector()  # Any move prevision position
        positionNext = FreeCAD.Vector()  # Any move next position
        G123Prev = FreeCAD.Vector()  #     Mill processing prevision position
        G123Next = FreeCAD.Vector()  #     Mill processing next position
        tempG0 = []  #                     Temporary list for num G0 commands
        uselessG0 = []  #                  List num commands G0 which need to remove
        newCommands = []  #                New movements commands if exclude retract
        g0x0y0 = []  #                     List commands G0 X0 Y0
        lastF = None  #                    Last feed rate value
        g0LimitZ = []  #                   List commands G0 which need replace Z
        firstG123 = -1  #                  Number first G1|G2|G3 command
        lastG123 = -1  #                   Number last G1|G2|G3 command

        for counter, cmd in enumerate(pathObj.Commands):

            # Mark movements G0 X0 Y0
            if self.obj.RemoveG0X0Y0:
                if cmd.Name in Path.Geom.CmdMoveRapid and cmd.x == 0 and cmd.y == 0:
                    g0x0y0.append(counter)

            # Storage for G0 which greater than limit Z
            if cmd.Name in Path.Geom.CmdMoveMill:
                if firstG123 == -1:
                    firstG123 = counter
                lastG123 = counter
            if (
                cmd.Name in Path.Geom.CmdMoveRapid
                and cmd.z
                and cmd.z > self.obj.LimitRetractHeight.Value
            ):
                g0LimitZ.append(counter)

            # Mark retract with movements less than Threshold
            if self.obj.RetractThreshold:

                if isFullyDefined and cmd.Name in Path.Geom.CmdMoveRapid:
                    # Temporary storage for G0, which will be cleaned when meeting with G1|G2|G3
                    tempG0.append(counter)

                if isFullyDefined and cmd.Name in Path.Geom.CmdMoveAll:
                    # Get position from any move command
                    positionNext.x = cmd.x if cmd.x else positionPrev.x
                    positionNext.y = cmd.y if cmd.y else positionPrev.y
                    positionNext.z = cmd.z if cmd.z else positionPrev.z

                if isFullyDefined and cmd.Name in Path.Geom.CmdMoveMill:
                    lastF = cmd.f if cmd.f else lastF
                    # Get position from mill cmd
                    G123Next.x = cmd.x if cmd.x else positionPrev.x
                    G123Next.y = cmd.y if cmd.y else positionPrev.y
                    G123Next.z = cmd.z if cmd.z else positionPrev.z
                    # Distance between mill commands (do not take into account Z)
                    p1 = copy.copy(G123Prev)
                    p2 = copy.copy(positionNext)
                    p1.z = 0
                    p2.z = 0
                    if tempG0 and p1.distanceToPoint(p2) <= self.obj.RetractThreshold:
                        uselessG0.extend(tempG0)
                        newCmd = Path.Command(f"G1 X{p2.x} Y{p2.y}")
                        if lastF:
                            newCmd.F = lastF
                        newCommands.append([counter, newCmd])
                    tempG0.clear()

                if not isFullyDefined and cmd.Name in Path.Geom.CmdMoveAll:
                    # After start program, X, Y and Z is not defined
                    # Define position from the few first commands
                    startX = cmd.x if cmd.x else startX
                    startY = cmd.y if cmd.y else startY
                    startZ = cmd.z if cmd.z else startZ
                    if startX and startY and startZ:
                        isFullyDefined = True
                        positionNext.x = startX
                        positionNext.y = startY
                        positionNext.z = startZ
                        positionPrev = copy.copy(positionNext)
                        G123Next = copy.copy(positionNext)
                        G123Prev = copy.copy(positionNext)

                G123Prev = copy.copy(G123Next)
                positionPrev = copy.copy(positionNext)

        # Remove movements G0 X0 Y0
        print("g0x0y0", g0x0y0)
        for i in g0x0y0:
            cmd = pathObj.Commands[i]
            # Leave only Z movement
            newCmd = Path.Command(f"{cmd.Name} Z{cmd.Z}")
            pathObj.deleteCommand(i)
            pathObj.insertCommand(newCmd, i)

        # Replace Z in movements G0
        print("g0LimitZ", g0LimitZ)
        for i in g0LimitZ:
            if (i > firstG123) and (i < lastG123):
                cmd = pathObj.Commands[i]
                cmd.z = self.obj.LimitRetractHeight.Value
                pathObj.deleteCommand(i)
                pathObj.insertCommand(cmd, i)

        # Remove useless movements G0
        print("uselessG0", uselessG0)
        for i in uselessG0:
            cmd = pathObj.Commands[i]
            newCmd = Path.Command(f"({cmd.toGCode()})")
            pathObj.deleteCommand(i)
            pathObj.insertCommand(newCmd, i)

        # Insert commands G1 between retracts (needed in some cases)
        for i, newCmd in reversed(newCommands):
            pathObj.insertCommand(newCmd, i)

    # Sort path to get minimal travel length
    def sortPathBruteForce(self, pathObj):
        startTime = time.monotonic()
        elements = []
        startPoint = FreeCAD.Vector()
        endPoint = FreeCAD.Vector()
        travelOrig = 0
        el = {"startIndex": None, "endIndex": None, "startPoint": None, "endPoint": None}
        for i, cmd in enumerate(pathObj.Commands):
            if cmd.Name in Path.Geom.CmdMoveAll:
                # Get position from any movement
                endPoint.x = cmd.x if cmd.x else startPoint.x
                endPoint.y = cmd.y if cmd.y else startPoint.y
                endPoint.z = cmd.z if cmd.z else startPoint.z

            if el["startIndex"] is None and cmd.Name in Path.Geom.CmdMoveRapid:
                el["startIndex"] = i

            if el["startPoint"] is None and cmd.Name in Path.Geom.CmdMoveMill:
                el["startPoint"] = copy.copy(startPoint)

            if (
                el["startIndex"] is not None
                and el["startPoint"] is not None
                and cmd.Name in Path.Geom.CmdMoveRapid
            ):
                el["endIndex"] = i
                el["endPoint"] = copy.copy(endPoint)

            if el["startIndex"] is not None and el["endIndex"] is not None:
                elements.append(el)
                if len(elements) > 1:
                    travelOrig += elements[-2]["endPoint"].distanceToPoint(
                        elements[-1]["startPoint"]
                    )
                el = {"startIndex": None, "endIndex": None, "startPoint": None, "endPoint": None}

            startPoint = copy.copy(endPoint)

        elLen = len(elements)
        combAmount = math.factorial(elLen)
        maxCombAmount = 500_000
        travelMin = None

        if combAmount > maxCombAmount:
            # to much combinations
            # random way seaching optimal order
            for counter in range(maxCombAmount):
                variant = random.permutation(elements)
                travel = 0
                for i in range(elLen - 1):
                    travel += variant[i]["endPoint"].distanceToPoint(variant[i + 1]["startPoint"])
                    if travelMin is not None and travel > travelMin:
                        # this variant is long
                        break
                else:
                    travelMin = travel
                    optimalOrder = variant

        else:
            # brute-force seaching optimal order
            perm = permutations(elements)
            for counter, variant in enumerate(perm):
                travel = 0
                for i in range(elLen - 1):
                    travel += variant[i]["endPoint"].distanceToPoint(variant[i + 1]["startPoint"])
                    if travelMin is not None and travel > travelMin:
                        # this variant is long
                        break
                else:
                    travelMin = travel
                    optimalOrder = variant

        print()
        print("Optimizing path travel")
        print(f"  Time spent = {round(time.monotonic() - startTime, 2)} sec")
        print(f"  Path elements amount = {len(elements)}")
        print(f"  Combinations amount = {combAmount:_}")
        print(f"  Combinations processed = {counter + 1:_}")
        print(f"  Length original rapid travel = {round(travelOrig, 1)} mm")
        print(f"  Length minimal rapid travel = {round(travelMin, 1)} mm")
        sortedPath = Path.Path()
        for el in optimalOrder:
            sortedPath.addCommands(pathObj.Commands[el["startIndex"] : el["endIndex"] + 1])

        return sortedPath

    def getPath(self):
        # Call this method on an instance of the class
        # to generate and return Path of the combined operations
        resultPathObj = Path.Path()
        for operation in self.obj.Group:
            resultPathObj.addCommands(PathUtils.getPathWithPlacement(operation).Commands)

        if (
            self.obj.RetractThreshold
            or self.obj.RemoveG0X0Y0
            or self.obj.LimitRetractHeight.Value < 100
        ):
            self.preprocessPath(resultPathObj)

        if self.obj.Sorting == "Brute-force":
            resultPathObj = self.sortPathBruteForce(resultPathObj)

        # return combined and pre-processed Path object
        return resultPathObj


class ViewProviderCompound:
    def __init__(self, vobj):
        self.attach(vobj)
        vobj.Proxy = self

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def claimChildren(self):
        if hasattr(self.obj, "Group"):
            return self.obj.Group
        else:
            return []

    def getIcon(self):
        if self.obj.Active:
            return ":/icons/CAM_CompoundTC.svg"
        else:
            return ":/icons/CAM_InactiveOp.svg"

    def onDelete(self, vobj, args=None):
        jobObj = PathUtils.findParentJob(self.obj)
        for operation in self.obj.Group:
            PathUtils.addToJob(operation, jobObj.Name)
            operation.ViewObject.Visibility = True
        return True


class commandPathCompoundTC:
    def GetResources(self):
        return {
            "Pixmap": "CAM_CompoundTC",
            "MenuText": QT_TRANSLATE_NOOP("CAM_PathCompoundTC", "Path Compound TC"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_PathCompoundTC",
                "Creates compound of Paths\nwith identical tool controller and without coolant",
            ),
        }

    def IsActive(self):
        selection = FreeCADGui.Selection.getSelection()
        if not selection:
            return False

        for sel in selection:
            if not sel.isDerivedFrom("Path::Feature"):
                return False

            if not toolControllerForOp(sel):
                # Active only for operations with tool controller
                return False

        return True

    def Activated(self):
        doc = FreeCAD.ActiveDocument
        doc.openTransaction("Create PathCompound")
        selection = FreeCADGui.Selection.getSelection()
        operations = [sel for sel in selection if hasattr(sel, "Path")]
        job = PathUtils.findParentJob(operations[0])
        obj = doc.addObject("Path::FeatureCompoundPython", "PathCompound")
        ViewProviderCompound(obj.ViewObject)
        PathUtils.addToJob(obj, job.Name)
        obj.Group = operations
        ObjectCompound(obj)
        for op in operations:
            op.ViewObject.Visibility = False
        job.Operations.removeObjects(operations)
        FreeCAD.ActiveDocument.commitTransaction()
        doc.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_PathCompoundTC", commandPathCompoundTC())
