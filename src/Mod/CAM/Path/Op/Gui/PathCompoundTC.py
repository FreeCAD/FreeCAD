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
import Path.Base.Util as PathUtil
import Path.Op.Base as PathOp
import PathScripts.PathUtils as PathUtils
from Path.Dressup.Utils import toolController

from PySide.QtCore import QT_TRANSLATE_NOOP

from itertools import permutations
from numpy import random
import math
import re
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
            "App::PropertyBool",
            "MinTravel",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Sort path to get minimal travel length"),
        )
        obj.Active = True
        obj.RemoveG0X0Y0 = False
        obj.RetractThreshold = 0
        obj.LimitRetractHeight = 100
        obj.ToolController = None
        obj.setEditorMode("CycleTime", 1)  # read-only
        obj.setEditorMode("ToolController", 3)  # read-only and hidden
        self.setToolController(obj)

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDelete(self, obj, args):
        print("onDelete from class ObjectCompound")
        # Does not work. Do not understand why ...
        # Someone who read this, please explain the reason )))
        # For workaround added onDelete() to class ViewProviderCompound
        return True

    def onChanged(self, obj, prop):
        return None

    def onDocumentRestored(self, obj):
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
        obj.ToolController = None
        if isinstance(obj.Group, list) and len(obj.Group) > 0:
            tcs = [toolController(op) for op in obj.Group]
            if len(set(tcs)) == 1 and tcs[0]:
                obj.ToolController = tcs[0]
        if not obj.ToolController:
            Path.Log.error(
                translate("CAM", "Tool Controllers of the combined objects is different or None.")
            )
            return translate("CAM", "Tool Error")
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

        for counter, command in enumerate(pathObj.Commands):

            # Mark movements G0 X0 Y0
            if self.obj.RemoveG0X0Y0:
                if re.search(r"^G0?[0]$", command.Name, re.IGNORECASE) and (
                    command.x,
                    command.y,
                ) == (0, 0):
                    g0x0y0.append(counter)

            # Storage for G0 which greater than limit Z
            if re.search(r"^G0?[123]$", command.Name, re.IGNORECASE):
                if firstG123 == -1:
                    firstG123 = counter
                lastG123 = counter
            if (
                re.search(r"^G0?[0]$", command.Name, re.IGNORECASE)
                and command.z
                and command.z > self.obj.LimitRetractHeight.Value
            ):
                g0LimitZ.append(counter)

            # Mark retract with movements less than Threshold
            if self.obj.RetractThreshold:

                if isFullyDefined and re.search(r"^G0?[0]$", command.Name, re.IGNORECASE):
                    # Temporary storage for G0, which will be cleaned when meeting with G1|G2|G3
                    tempG0.append(counter)

                if isFullyDefined and re.search(r"^G0?[0123]$", command.Name, re.IGNORECASE):
                    # Get position from any movement command
                    positionNext.x = command.x if command.x else positionPrev.x
                    positionNext.y = command.y if command.y else positionPrev.y
                    positionNext.z = command.z if command.z else positionPrev.z

                if isFullyDefined and re.search(r"^G0?[123]$", command.Name, re.IGNORECASE):
                    lastF = command.f if command.f else lastF
                    # Get position from mill command
                    G123Next.x = command.x if command.x else positionPrev.x
                    G123Next.y = command.y if command.y else positionPrev.y
                    G123Next.z = command.z if command.z else positionPrev.z
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

                if not isFullyDefined and re.search(r"^G0?[0123]$", command.Name, re.IGNORECASE):
                    # After start program, X, Y and Z is not defined
                    # Define position from the few first commands
                    startX = command.x if command.x else startX
                    startY = command.y if command.y else startY
                    startZ = command.z if command.z else startZ
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
        for i, newCmd in newCommands:
            pathObj.insertCommand(newCmd, i)

    # Sort path to get minimal travel length
    def sortPath(self, pathObj):
        startTime = time.monotonic()
        elements = []
        startPoint = FreeCAD.Vector()
        endPoint = FreeCAD.Vector()
        travelOrig = 0
        el = {"startIndex": None, "endIndex": None, "startPoint": None, "endPoint": None}
        for i, command in enumerate(pathObj.Commands):
            if re.search(r"^G0?[0123]$", command.Name, re.IGNORECASE):
                # Get position from any movement command
                endPoint.x = command.x if command.x else startPoint.x
                endPoint.y = command.y if command.y else startPoint.y
                endPoint.z = command.z if command.z else startPoint.z

            if el["startIndex"] is None and re.search(r"^G0?[0]$", command.Name, re.IGNORECASE):
                el["startIndex"] = i

            if el["startPoint"] is None and re.search(r"^G0?[123]$", command.Name, re.IGNORECASE):
                el["startPoint"] = copy.copy(startPoint)

            if (
                el["startIndex"] is not None
                and el["startPoint"] is not None
                and re.search(r"^G0?[0]$", command.Name, re.IGNORECASE)
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

        if self.obj.MinTravel:
            resultPathObj = self.sortPath(resultPathObj)

        # return combined and pre-processed Path object
        return resultPathObj


class ViewProviderCompound:
    def __init__(self, vobj):
        self.attach(vobj)
        vobj.Proxy = self

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object

    def claimChildren(self):
        if hasattr(self.obj, "Group"):
            return self.obj.Group
        else:
            return []

    def getIcon(self):
        return ":/icons/CAM_CompoundTC.svg"

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
        tcs = []
        selection = FreeCADGui.Selection.getSelection()
        for sel in selection:
            if not sel.isDerivedFrom("Path::Feature"):
                return False
            tc = toolController(sel)
            if tc:
                # Active only for operations with identical tool controller
                tcs.append(tc)
                if len(set(tcs)) != 1:
                    return False
            else:
                return False
            if PathUtil.coolantModeForOp(sel) != "None":
                # Active only for operations without cooling
                return False
        return True

    def Activated(self):
        doc = FreeCAD.ActiveDocument

        groupObjs = []
        selection = FreeCADGui.Selection.getSelection()
        for sel in selection:
            if hasattr(sel, "Path"):
                sel.ViewObject.Visibility = False
                groupObjs.append(sel)

        jobObj = PathUtils.findParentJob(groupObjs[0])
        compoundObj = doc.addObject("Path::FeatureCompoundPython", "PathCompound")
        compoundObj.ViewObject.Proxy = 0
        ViewProviderCompound(compoundObj.ViewObject)
        PathUtils.addToJob(compoundObj, jobObj.Name)
        compoundObj.Group = groupObjs
        ObjectCompound(compoundObj)

        # Remove Path objects from 'Operations' group to exclude gcode duplication
        jobObj.Operations.removeObjects(groupObjs)

        doc.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_PathCompoundTC", commandPathCompoundTC())
