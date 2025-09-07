# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import FreeCADGui
import Path
import Path.Op.Base as PathOp
import PathScripts.PathUtils as PathUtils
from Path.Base.Util import coolantModeForOp
from Path.Base.Util import toolControllerForOp

import random
import datetime
import time

from PySide.QtCore import QT_TRANSLATE_NOOP

__doc__ = """CAM Array object and FreeCAD command"""

translate = FreeCAD.Qt.translate


class ObjectArray:
    def __init__(self, obj):
        # Path properties group
        obj.addProperty(
            "App::PropertyBool",
            "Active",
            "Path",
            QT_TRANSLATE_NOOP("PathOp", "Make False, to prevent operation from generating code"),
        )
        obj.addProperty(
            "App::PropertyLinkList",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The toolpaths to array"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "ClearanceHeight",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Height for rapid moves between repeats\nIgnore if clearance height of the first operation is upper",
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "CycleTime",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Operations cycle time estimation"),
        )
        obj.addProperty(
            "App::PropertyLink",
            "ToolController",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The tool controller that will be used to calculate the toolpath\nShould be identical for all base operations",
            ),
        )
        obj.addProperty(
            "App::PropertyLinkListHidden",
            "ArrayGroup",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "List array objects"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "Combine",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "Combine operations with same tool controller and coolant mode"
            ),
        )

        # Pattern properties group
        obj.addProperty(
            "App::PropertyEnumeration",
            "Type",
            "Pattern",
            QT_TRANSLATE_NOOP("App::Property", "Pattern method"),
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "Copies",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property", "The number of copies in Linear1D and Polar pattern"
            ),
        )
        obj.addProperty(
            "App::PropertyVectorDistance",
            "Offset",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The spacing between the array copies in linear pattern",
            ),
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "CopiesX",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property", "The number of copies in X-direction in linear pattern"
            ),
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "CopiesY",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property", "The number of copies in Y-direction in linear pattern"
            ),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "Angle",
            "Pattern",
            QT_TRANSLATE_NOOP("App::Property", "Total angle in polar pattern"),
        )
        obj.addProperty(
            "App::PropertyVector",
            "Centre",
            "Pattern",
            QT_TRANSLATE_NOOP("App::Property", "The centre of rotation in polar pattern"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "SwapDirection",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Make copies in X direction before Y in Linear 2D pattern",
            ),
        )
        obj.addProperty(
            "App::PropertyLinkSubListGlobal",
            "PointsObject",
            "Pattern",
            QT_TRANSLATE_NOOP("App::Property", "The source of points for array"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "PointsSortingMode",
            "Pattern",
            QT_TRANSLATE_NOOP("App::Property", "Sorting mode"),
        )

        # Random properties group
        obj.addProperty(
            "App::PropertyBool",
            "UseJitter",
            "Random",
            QT_TRANSLATE_NOOP("App::Property", "Use randomly offset"),
        )
        obj.addProperty(
            "App::PropertyVectorDistance",
            "JitterMagnitude",
            "Random",
            QT_TRANSLATE_NOOP("App::Property", "Maximum random offset of copies"),
        )
        obj.addProperty(
            "App::PropertyPercent",
            "JitterPercent",
            "Random",
            QT_TRANSLATE_NOOP("App::Property", "Percent of copies to randomly offset"),
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "JitterSeed",
            "Random",
            QT_TRANSLATE_NOOP("App::Property", "Seed value for jitter randomness"),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "JitterAngle",
            "Random",
            QT_TRANSLATE_NOOP("App::Property", "Max angle of rotation for jitter randomness"),
        )
        obj.addProperty(
            "App::PropertyVector",
            "JitterCentre",
            "Random",
            QT_TRANSLATE_NOOP("App::Property", "The centre of rotation for jitter randomness"),
        )

        obj.Active = True
        obj.Type = ("Linear1D", "Linear2D", "Polar", "Points")
        obj.PointsSortingMode = ("Automatic", "Manual")
        obj.Copies = (0, 0, 99999, 1)
        obj.CopiesX = (0, 0, 99999, 1)
        obj.CopiesY = (0, 0, 99999, 1)
        obj.JitterSeed = (0, 0, 2147483647, 1)
        obj.JitterPercent = 100
        obj.JitterMagnitude = FreeCAD.Vector(10, 10, 0)
        obj.JitterAngle = 10

        self.group = []
        self.setEditorModes(obj)
        obj.Proxy = self

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def setEditorModes(self, obj):
        obj.setEditorMode("ToolController", 2)  # hidden
        obj.setEditorMode("ArrayGroup", 3)  # hidden and read-only
        obj.setEditorMode("CycleTime", 1)  # read-only

        if obj.Type == "Linear1D":
            angleMode = copiesXYMode = pointsMode = 2
            copiesMode = offsetMode = 0
        elif obj.Type == "Linear2D":
            angleMode = copiesMode = pointsMode = 2
            copiesXYMode = offsetMode = 0
        elif obj.Type == "Polar":
            copiesXYMode = offsetMode = pointsMode = 2
            angleMode = copiesMode = 0
        elif obj.Type == "Points":
            angleMode = copiesMode = copiesXYMode = offsetMode = 2
            pointsMode = 0
        else:
            angleMode = copiesMode = pointsMode = copiesXYMode = offsetMode = 0

        obj.setEditorMode("Angle", angleMode)
        obj.setEditorMode("Centre", angleMode)
        obj.setEditorMode("Copies", copiesMode)
        obj.setEditorMode("CopiesX", copiesXYMode)
        obj.setEditorMode("CopiesY", copiesXYMode)
        obj.setEditorMode("Offset", offsetMode)
        obj.setEditorMode("SwapDirection", copiesXYMode)

        obj.setEditorMode("PointsObject", pointsMode)
        obj.setEditorMode("PointsSortingMode", pointsMode)

        jitterMode = 0 if obj.UseJitter else 2
        obj.setEditorMode("JitterPercent", jitterMode)
        obj.setEditorMode("JitterMagnitude", jitterMode)
        obj.setEditorMode("JitterSeed", jitterMode)
        obj.setEditorMode("JitterAngle", jitterMode)
        obj.setEditorMode("JitterCentre", jitterMode)

    def onChanged(self, obj, prop):
        # print("prop", prop)
        if prop in ("Type", "UseJitter") and not obj.Document.Restoring:
            self.setEditorModes(obj)

        if prop == "Path" and hasattr(self, "group"):
            if obj.Combine:
                self.processGroupCompound(obj)
            else:
                self.processGroup(obj)

            self.processCycleTime(obj)

    def onDocumentRestored(self, obj):
        """onDocumentRestored(obj) ... Called automatically when document is restored"""
        if not hasattr(obj, "UseJitter"):
            obj.addProperty(
                "App::PropertyBool",
                "UseJitter",
                "Random",
                QT_TRANSLATE_NOOP("App::Property", "Use randomly offset"),
            )
            obj.setGroupOfProperty("JitterMagnitude", "Random")
            obj.setGroupOfProperty("JitterPercent", "Random")
            obj.setGroupOfProperty("JitterSeed", "Random")

            obj.setGroupOfProperty("SwapDirection", "Pattern")
            obj.setGroupOfProperty("CopiesX", "Pattern")
            obj.setGroupOfProperty("CopiesY", "Pattern")
            obj.setGroupOfProperty("Copies", "Pattern")
            obj.setGroupOfProperty("Offset", "Pattern")
            obj.setGroupOfProperty("Angle", "Pattern")
            obj.setGroupOfProperty("Type", "Pattern")

        if not hasattr(obj, "CycleTime"):
            obj.addProperty(
                "App::PropertyString",
                "CycleTime",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Operations cycle time estimation"),
            )

        if not hasattr(obj, "ClearanceHeight"):
            obj.addProperty(
                "App::PropertyDistance",
                "ClearanceHeight",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Height for rapid moves between repeats\nIgnore if clearance height of the first operation is upper",
                ),
            )

        if not hasattr(obj, "PointsObject"):
            obj.addProperty(
                "App::PropertyLinkSubListGlobal",
                "PointsObject",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "The source of points for array"),
            )
            type = obj.Type
            obj.Type = ("Linear1D", "Linear2D", "Polar", "Points")
            obj.Type = type

        if not hasattr(obj, "PointsSortingMode"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "PointsSortingMode",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "Sorting mode"),
            )
            obj.PointsSortingMode = ("Automatic", "Manual")

        if not hasattr(obj, "JitterAngle"):
            obj.addProperty(
                "App::PropertyAngle",
                "JitterAngle",
                "Random",
                QT_TRANSLATE_NOOP("App::Property", "Max angle of rotation for jitter randomness"),
            )
        if not hasattr(obj, "JitterCentre"):
            obj.addProperty(
                "App::PropertyVector",
                "JitterCentre",
                "Random",
                QT_TRANSLATE_NOOP("App::Property", "The centre of rotation for jitter randomness"),
            )

        if not hasattr(obj, "ArrayGroup"):
            obj.addProperty(
                "App::PropertyLinkListHidden",
                "ArrayGroup",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "List array objects"),
            )

        self.setEditorModes(obj)

    def execute(self, obj):
        # print("execute")
        """Do not generate paths and clear current Path data,
        if operation not Active or no base operations or operations not compatible"""
        if not obj.Active or not len(obj.Base) or not self.isBaseCompatible(obj):
            return

        # this tool controller is workaround and will not be used in post-processing
        obj.ToolController = toolControllerForOp(obj.Base[0])

        # Prepare random function
        if (
            obj.UseJitter
            and obj.JitterPercent
            and (obj.JitterMagnitude != FreeCAD.Vector() or obj.JitterAngle)
        ):
            random.seed(obj.JitterSeed)
            jitterPercent = obj.JitterPercent
        else:
            jitterPercent = 0

        args = (
            obj.Base,
            obj.Type,
            obj.Copies,
            obj.Offset,
            obj.CopiesX,
            obj.CopiesY,
            obj.Angle,
            obj.Centre,
            obj.SwapDirection,
            jitterPercent,
            obj.JitterMagnitude,
            obj.JitterAngle,
            obj.JitterCentre,
            obj.ClearanceHeight.Value,
            obj.PointsObject,
            obj.PointsSortingMode,
        )

        pa = PathArray(*args)
        pathOp = pa.getPath()
        if self.isBaseCompatibleStrict(obj):
            self.group = []
            cmds = [cmd for op in pathOp for cmd in op["path"].Commands]
            obj.Path = Path.Path(cmds)
            self.cleanGroup(obj, 0)
            combineMode = 2
        else:
            combineMode = 0
            self.group = pathOp
            obj.Path = Path.Path()  # trigger to recompute elements in Group

        obj.setEditorMode("Combine", combineMode)

    # Check compatibility of base operations
    # for array with multi tool controllers and coolant
    def isBaseCompatible(self, obj):
        if not obj.Base:
            return False

        for base in obj.Base:
            if not base.isDerivedFrom("Path::Feature"):
                return False

            if not toolControllerForOp(base):
                Path.Log.warning(
                    translate("PathArray", "Tool controller not selected for operation %s")
                    % base.Label
                )
                return False

        return True

    # Check compatibility of base operations for old type of array,
    # identical tool controller and no coolant
    def isBaseCompatibleStrict(self, obj):
        tc0 = toolControllerForOp(obj.Base[0])
        for base in obj.Base:
            tc = toolControllerForOp(base)
            if tc != tc0:
                return False

            if coolantModeForOp(base) != "None":
                return False

        return True

    def cleanGroup(self, obj, amount):
        while len(obj.ArrayGroup) > amount:
            # remove extra Path object from obj.ArrayGroup list
            op = obj.ArrayGroup[-1]
            obj.ArrayGroup = obj.ArrayGroup[:-1]
            op.Document.removeObject(op.Name)

    # Create separated Path elements for each repeat and each object
    def processGroup(self, obj):
        doc = FreeCAD.ActiveDocument
        for i, op in enumerate(self.group):
            if i >= len(obj.ArrayGroup):
                # add new Path object if amount is not enough
                newPathObj = doc.addObject("Path::FeaturePython", "Array")
                ObjectArrayChild(newPathObj)
                newPathObj.ViewObject.Proxy = ViewProviderArrayChild(newPathObj.ViewObject)
                obj.ArrayGroup = obj.ArrayGroup + [newPathObj]

            baseOp = doc.getObject(op["opName"])
            obj.ArrayGroup[i].Label = f"Array_{baseOp.Label}"
            obj.ArrayGroup[i].Base = [baseOp.Name]
            obj.ArrayGroup[i].ToolController = toolControllerForOp(baseOp)
            obj.ArrayGroup[i].CoolantMode = coolantModeForOp(baseOp)
            obj.ArrayGroup[i].PathTemp = op["path"]
            if obj.ArrayGroup[i].Active:
                obj.ArrayGroup[i].Path = op["path"]
            else:
                obj.ArrayGroup[i].Path = Path.Path()

        self.cleanGroup(obj, len(self.group))

    # Combine elements if Combine is True
    def processGroupCompound(self, obj):
        doc = FreeCAD.ActiveDocument
        # get ordered list with unique combinations
        combs = []
        for op in self.group:
            baseOp = doc.getObject(op["opName"])
            toolController = toolControllerForOp(baseOp)
            coolantMode = coolantModeForOp(baseOp)
            candidate = {"tc": toolController, "cm": coolantMode}
            if candidate not in combs:
                combs.append(candidate)

        for i, comb in enumerate(combs):
            if i >= len(obj.ArrayGroup):
                # add new Path object if amount is not enough
                newPathObj = doc.addObject("Path::FeaturePython", "Array")
                ObjectArrayChild(newPathObj)
                newPathObj.ViewObject.Proxy = ViewProviderArrayChild(newPathObj.ViewObject)
                newPathObj.ToolController = comb["tc"]
                newPathObj.CoolantMode = comb["cm"]
                obj.ArrayGroup = obj.ArrayGroup + [newPathObj]
            else:
                obj.ArrayGroup[i].Base = []
                obj.ArrayGroup[i].PathTemp = Path.Path()

            obj.ArrayGroup[i].Label = f"Array_{comb['cm']}_{comb['tc'].Label}"

            for op in self.group:
                baseOp = doc.getObject(op["opName"])
                toolController = toolControllerForOp(baseOp)
                coolantMode = coolantModeForOp(baseOp)
                if toolController == comb["tc"] and coolantMode == comb["cm"]:
                    obj.ArrayGroup[i].Base += [op["opName"]]
                    obj.ArrayGroup[i].PathTemp.addCommands(op["path"].Commands)

                    if obj.ArrayGroup[i].Active:
                        obj.ArrayGroup[i].Path = obj.ArrayGroup[i].PathTemp
                    else:
                        obj.ArrayGroup[i].Path = Path.Path()

        self.cleanGroup(obj, len(combs))

    # Get total cycle time
    def processCycleTime(self, obj):
        totalTime = 0
        for op in obj.ArrayGroup:
            (h, m, s) = op.CycleTime.split(":")
            opTime = datetime.timedelta(hours=int(h), minutes=int(m), seconds=int(s))
            totalTime += opTime.seconds

        obj.CycleTime = time.strftime("%H:%M:%S", time.gmtime(totalTime))


class ObjectArrayChild:
    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyBool",
            "Active",
            "Path",
            "Make False, to prevent operation from generating code",
        )
        obj.addProperty(
            "App::PropertyLink",
            "ToolController",
            "Path",
            "The tool controller that will be used to calculate the toolpath",
        )
        obj.addProperty(
            "App::PropertyString",
            "CoolantMode",
            "Path",
            "Coolant mode for this operation",
        )
        obj.addProperty(
            "Path::PropertyPath",
            "PathTemp",
            "Path",
            "Temporary storage of the tool path if element not active",
        )
        obj.addProperty(
            "App::PropertyString",
            "CycleTime",
            "Path",
            "Operations cycle time estimation",
        )
        obj.addProperty(
            "App::PropertyStringList",
            "Base",
            "Path",
            "Name of the base operations",
        )
        obj.Active = True
        obj.Proxy = self
        obj.setEditorMode("ToolController", 1)  # read-only
        obj.setEditorMode("CoolantMode", 1)  # read-only
        obj.setEditorMode("CycleTime", 1)  # read-only
        obj.setEditorMode("Base", 1)  # read-only

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, obj, prop):
        # print("ObjectArrayChild", prop, obj.Name)
        if prop == "Path":
            obj.CycleTime = PathOp.getCycleTimeEstimate(obj)

    def onDocumentRestored(self, obj):
        return

    def execute(self, obj):
        # print("ObjectArrayChild execute", obj.Name)
        if obj.Active:
            obj.Path = obj.PathTemp
        else:
            obj.Path = Path.Path()


class PathArray:
    """class PathArray ...
    This class receives one or more base operations and repeats those operations
    at set intervals based upon array type requested and the related settings for that type"""

    def __init__(
        self,
        base,
        arrayType,
        copies,
        offsetVector,
        copiesX,
        copiesY,
        angle,
        centre,
        swapDirection,
        jitterPercent,
        jitterMagnitude,
        jitterAngle,
        jitterCentre,
        clearanceHeight,
        pointsBase,
        pointsSortingMode,
    ):
        self.base = base
        self.arrayType = arrayType  # ['Linear1D', 'Linear2D', 'Polar']
        self.copies = copies
        self.offsetVector = offsetVector
        self.copiesX = copiesX
        self.copiesY = copiesY
        self.polarAngle = angle
        self.polarCentre = centre
        self.swapDirection = swapDirection
        self.jitterPercent = jitterPercent
        self.jitterMagnitude = jitterMagnitude
        self.jitterAngle = jitterAngle
        self.jitterCentre = jitterCentre
        self.clearanceHeight = clearanceHeight
        self.pointsBase = pointsBase
        self.pointsSortingMode = pointsSortingMode

    def calculateJitter(self, pos):
        """calculateJitter(pos) ...
        Returns the position argument with
        a random vector shift applied and random angle"""
        alpha = 0
        if self.jitterPercent == 0:
            return pos, alpha

        if random.randint(0, 100) < self.jitterPercent:
            pos.x = pos.x + random.uniform(-self.jitterMagnitude.x, self.jitterMagnitude.x)
            pos.y = pos.y + random.uniform(-self.jitterMagnitude.y, self.jitterMagnitude.y)
            pos.z = pos.z + random.uniform(-self.jitterMagnitude.z, self.jitterMagnitude.z)

            alpha = random.uniform(-self.jitterAngle, self.jitterAngle)

        return pos, alpha

    def addMiddleMoves(self, commands):
        """Returns list of commands for rapid moves between repeats with specific height"""
        x = y = z = index = None

        for i, cmd in enumerate(commands):
            x = cmd.x if x is None and cmd.x is not None else x
            y = cmd.y if y is None and cmd.y is not None else y
            z = cmd.z if z is None and cmd.z is not None else z
            if index is None and (x is not None or y is not None or z is not None):
                # first move command in operation
                index = i
            if x is not None and y is not None and z is not None:
                break
        else:
            return commands

        if self.clearanceHeight > z:
            z = self.clearanceHeight
            extraCommands = []
            extraCommands.append(Path.Command("G0", {"Z": z}))
            extraCommands.append(Path.Command("G0", {"X": x, "Y": y}))
            for cmd in reversed(extraCommands):
                commands.insert(index, cmd)

        return commands

    def getStartPoint(self, commands):
        """Last tool position in last base operation"""
        x = y = z = None

        for cmd in reversed(commands):
            x = cmd.x if x is None and cmd.x is not None else x
            y = cmd.y if y is None and cmd.y is not None else y
            z = cmd.z if z is None and cmd.z is not None else z

            if x is not None and y is not None and z is not None:
                return FreeCAD.Vector(x, y, z)

        return FreeCAD.Vector()

    def getLinear1DArray(self):
        """Array type Linear1D"""
        pathGroup = []
        for i in range(self.copies):
            pos = FreeCAD.Vector(
                self.offsetVector.x * (i + 1),
                self.offsetVector.y * (i + 1),
                self.offsetVector.z * (i + 1),
            )
            pos, alpha = self.calculateJitter(pos)

            for bIndex, b in enumerate(self.base):
                pl = FreeCAD.Placement()
                pl.move(pos)
                pl.rotate(self.jitterCentre, FreeCAD.Vector(0, 0, 1), alpha)
                path = PathUtils.getPathWithPlacement(b)
                path = PathUtils.applyPlacementToPath(pl, path)
                cmds = path.Commands

                if bIndex == 0:
                    # add moves at extra height for moves between repeats
                    cmds = self.addMiddleMoves(cmds)

                pathGroup.append(
                    {
                        "path": Path.Path(cmds),
                        "opName": b.Name,
                    }
                )

        return pathGroup

    def getLinear2DXYArray(self):
        """Array type Linear2D with initial X direction"""
        pathGroup = []
        for i in range(self.copiesY + 1):
            for j in range(self.copiesX + 1):
                if (i % 2) == 0:
                    pos = FreeCAD.Vector(
                        self.offsetVector.x * j,
                        self.offsetVector.y * i,
                        self.offsetVector.z * i,
                    )
                else:
                    pos = FreeCAD.Vector(
                        self.offsetVector.x * (self.copiesX - j),
                        self.offsetVector.y * i,
                        self.offsetVector.z * i,
                    )
                pos, alpha = self.calculateJitter(pos)

                for bIndex, b in enumerate(self.base):
                    pl = FreeCAD.Placement()
                    # index 0,0 will be processed by the base Paths themselves
                    if i != 0 or j != 0:
                        pl.move(pos)
                        pl.rotate(self.jitterCentre, FreeCAD.Vector(0, 0, 1), alpha)
                        path = PathUtils.getPathWithPlacement(b)
                        path = PathUtils.applyPlacementToPath(pl, path)
                        cmds = path.Commands

                        if bIndex == 0:
                            # add moves at extra height for moves between repeats
                            cmds = self.addMiddleMoves(cmds)

                        pathGroup.append(
                            {
                                "path": Path.Path(cmds),
                                "opName": b.Name,
                            }
                        )

        return pathGroup

    def getLinear2DYXArray(self):
        """Array type Linear2D with initial Y direction"""
        pathGroup = []
        for i in range(self.copiesX + 1):
            for j in range(self.copiesY + 1):
                if (i % 2) == 0:
                    pos = FreeCAD.Vector(
                        self.offsetVector.x * i,
                        self.offsetVector.y * j,
                        self.offsetVector.z * i,
                    )
                else:
                    pos = FreeCAD.Vector(
                        self.offsetVector.x * i,
                        self.offsetVector.y * (self.copiesY - j),
                        self.offsetVector.z * i,
                    )
                pos, alpha = self.calculateJitter(pos)

                for bIndex, b in enumerate(self.base):
                    pl = FreeCAD.Placement()
                    # do not process the index 0,0. It will be processed by the base Paths themselves
                    if i != 0 or j != 0:
                        pl.move(pos)
                        pl.rotate(self.jitterCentre, FreeCAD.Vector(0, 0, 1), alpha)
                        path = PathUtils.getPathWithPlacement(b)
                        path = PathUtils.applyPlacementToPath(pl, path)
                        cmds = path.Commands

                        if bIndex == 0:
                            # add moves at extra height for moves between repeats
                            cmds = self.addMiddleMoves(cmds)

                        pathGroup.append(
                            {
                                "path": Path.Path(cmds),
                                "opName": b.Name,
                            }
                        )

        return pathGroup

    def getPolarArray(self):
        """Array type Polar"""
        pathGroup = []
        for i in range(self.copies):
            ang = 360
            if self.copies > 0:
                ang = self.polarAngle / self.copies * (1 + i)

            # prepare placement for polar pattern
            pl = FreeCAD.Placement()
            pl.rotate(self.polarCentre, FreeCAD.Vector(0, 0, 1), ang)

            # add jitter to placement
            pos, alpha = self.calculateJitter(FreeCAD.Vector())
            pl.move(pos)
            pl.rotate(self.jitterCentre, FreeCAD.Vector(0, 0, 1), alpha)

            for bIndex, b in enumerate(self.base):
                path = PathUtils.getPathWithPlacement(b)
                path = PathUtils.applyPlacementToPath(pl, path)
                cmds = path.Commands

                if bIndex == 0:
                    # add moves at extra height for moves between repeats
                    cmds = self.addMiddleMoves(cmds)

                pathGroup.append(
                    {
                        "path": Path.Path(cmds),
                        "opName": b.Name,
                    }
                )

        return pathGroup

    def getPointsArray(self):
        """Array type Points"""
        pathGroup = []

        # get points from base object
        points = []
        for base in self.pointsBase:
            (baseObj, subNames) = base
            if not subNames or subNames == ("",):
                # add all vertexes from object
                subNames = [f"Vertex{i[0]+1}" for i in enumerate(baseObj.Shape.Vertexes)]

            points.extend(
                [
                    baseObj.Shape.getElement(sub).Point
                    for sub in subNames
                    if sub.startswith("Vertex")
                ]
            )

        # get sorted positions for array
        if self.pointsSortingMode == "Automatic":
            startPoint = self.getStartPoint(self.base[-1].Path.Commands)
            positions = [{"x": pos.x, "y": pos.y, "z": pos.z} for pos in points]
            positions = PathUtils.sort_locations_2opt(
                positions, ["x", "y"], startPoint=startPoint, endPoint=None
            )
            points = [FreeCAD.Vector(pos["x"], pos["y"], pos["z"]) for pos in positions]

        for pos in points:
            # apply jitter
            pos, alpha = self.calculateJitter(pos)

            for bIndex, b in enumerate(self.base):
                pl = FreeCAD.Placement()
                pl.move(pos)
                pl.rotate(self.jitterCentre, FreeCAD.Vector(0, 0, 1), alpha)
                path = PathUtils.getPathWithPlacement(b)
                path = PathUtils.applyPlacementToPath(pl, path)
                cmds = path.Commands

                if bIndex == 0:
                    # add moves at extra height for moves between repeats
                    cmds = self.addMiddleMoves(cmds)

                pathGroup.append(
                    {
                        "path": Path.Path(cmds),
                        "opName": b.Name,
                    }
                )

        return pathGroup

    def getPath(self):
        """getPath() ... Call this method on an instance of the class to generate and return
        path data for the requested path array."""

        if self.arrayType == "Polar":
            pathGroup = self.getPolarArray()

        elif self.arrayType == "Linear2D":
            if self.swapDirection:
                pathGroup = self.getLinear2DXYArray()
            else:
                pathGroup = self.getLinear2DYXArray()

        elif self.arrayType == "Points":
            pathGroup = self.getPointsArray()

        else:
            pathGroup = self.getLinear1DArray()

        return pathGroup


class ViewProviderArray:
    def __init__(self, vobj):
        self.Object = vobj.Object
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, vobj, prop):
        # print("ViewProviderArray", prop)
        if prop == "Visibility":
            for op in self.Object.ArrayGroup:
                op.Visibility = vobj.Visibility

    def claimChildren(self):
        return [base for base in self.Object.ArrayGroup]

    def onDelete(self, vobj, args):
        for op in self.Object.ArrayGroup:
            op.Document.removeObject(op.Name)
        self.Object.Document.removeObject(self.Object.Name)


class ViewProviderArrayChild:

    def __init__(self, vobj):
        self.Object = vobj.Object
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, vobj, prop):
        # print("ViewProviderArrayChild", prop)
        return


class CommandPathArray:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Array",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Array", "ArrayMultiToolController"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_Array",
                "Creates an array with multi tool controllers and coolant\nExperimental",
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

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()

        for sel in selection:
            if not (sel.isDerivedFrom("Path::Feature")):
                FreeCAD.Console.PrintError(
                    translate("CAM_Array", "Arrays can be created only from toolpath operations.")
                    + "\n"
                )
                return

        # if everything is ok, execute and register the transaction in the
        # undo/redo stack
        FreeCAD.ActiveDocument.openTransaction("Create Array")
        FreeCADGui.addModule("Path.Op.Gui.Array")
        FreeCADGui.addModule("PathScripts.PathUtils")

        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Array")'
        )

        FreeCADGui.doCommand("Path.Op.Gui.Array.ObjectArray(obj)")

        baseString = "[%s]" % ",".join(
            ["FreeCAD.ActiveDocument.%s" % sel.Name for sel in selection]
        )
        FreeCADGui.doCommand("obj.Base = %s" % baseString)

        FreeCADGui.doCommand("obj.ViewObject.Proxy = 0")
        FreeCADGui.doCommand(
            "obj.ViewObject.Proxy = Path.Op.Gui.Array.ViewProviderArray(obj.ViewObject)"
        )
        FreeCADGui.doCommand("PathScripts.PathUtils.addToJob(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Array", CommandPathArray())
