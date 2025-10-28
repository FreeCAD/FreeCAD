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

import DraftVecUtils
import FreeCAD
import FreeCADGui
import Path
import Path.Op.Base as PathOp
import PathScripts.PathUtils as PathUtils
from Path.Base.Util import coolantModeForOp
from Path.Base.Util import toolControllerForOp

import random
import datetime
import math
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
            "ExtraClearanceHeight",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Increase height for rapid moves between repeats",
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
                "App::Property",
                "If operations with identical tool controller and without coolant:"
                "\ncombine by copies"
                "\n\nIf operations with different tool controllers or with coolant:"
                "\ncombine operations with same tool controller and coolant mode, "
                "\nbut only if operations placed one by one in tree",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "Split",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Split path by elements even all base operations without coolant and with identical tool controller",
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
            "PointsSource",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The sources of points for array"
                "\n\nUse selected vertexes."
                "\nIf edges selected, use first vertex of each edge."
                "\n\nUse all vertexes, if not sub elements selected"
                "\nand shape contain only vertexes."
                "\nUse first vertex of each edge, if not sub elements selected"
                "\nand shape contain edges.",
            ),
        )
        obj.addProperty(
            "App::PropertyLinkSubGlobal",
            "PointsOrigin",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The origin for points"
                "\n\nUse selected vertex."
                "\nIf edge selected, use first vertex of edge"
                "\nand determine angle of rotation."
                "\n\nUse first vertex, if not sub elements selected"
                "\nand shape contain only vertexes."
                "\nUse first vertex of edge, if not sub elements selected"
                "\nand shape contain edges. Also determine angle of rotation.",
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "PointsSorting",
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
            "App::PropertyBool",
            "AddBasePath",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Add base path to array operation"),
        )

        obj.Active = True
        obj.Type = ("Linear1D", "Linear2D", "Polar", "Points")
        obj.PointsSorting = ("Automatic", "Manual")
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
        obj.setEditorMode("AddBasePath", 2)  # hidden

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

        obj.setEditorMode("PointsOrigin", pointsMode)
        obj.setEditorMode("PointsSource", pointsMode)
        obj.setEditorMode("PointsSorting", pointsMode)

        jitterMode = 0 if obj.UseJitter else 2
        obj.setEditorMode("JitterPercent", jitterMode)
        obj.setEditorMode("JitterMagnitude", jitterMode)
        obj.setEditorMode("JitterSeed", jitterMode)
        obj.setEditorMode("JitterAngle", jitterMode)

        splitMode = 0 if self.isBaseCompatibleStrict(obj) else 2
        obj.setEditorMode("Split", splitMode)

        combineMode = 0 if obj.ArrayGroup else 2
        obj.setEditorMode("Combine", combineMode)

    def onChanged(self, obj, prop):
        if prop in ("Path", "Split", "Type", "UseJitter") and not obj.Document.Restoring:
            self.setEditorModes(obj)

        if prop == "Active" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

        if prop == "Active":
            for op in obj.ArrayGroup:
                op.Active = obj.Active

    def onDocumentRestored(self, obj):
        """onDocumentRestored(obj) ... Called automatically when document is restored"""
        if not obj.ViewObject.Proxy:
            obj.ViewObject.Proxy = Path.Op.Gui.Array.ViewProviderArray(obj.ViewObject)

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

        if not hasattr(obj, "ExtraClearanceHeight"):
            obj.addProperty(
                "App::PropertyDistance",
                "ExtraClearanceHeight",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Increase height for rapid moves between repeats",
                ),
            )

        if not hasattr(obj, "PointsSource"):
            obj.addProperty(
                "App::PropertyLinkSubListGlobal",
                "PointsSource",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "The sources of points for array"),
            )
            type = obj.Type
            obj.Type = ("Linear1D", "Linear2D", "Polar", "Points")
            obj.Type = type

        if not hasattr(obj, "PointsOrigin"):
            obj.addProperty(
                "App::PropertyLinkSubGlobal",
                "PointsOrigin",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "The origin for points"),
            )

        if not hasattr(obj, "PointsSorting"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "PointsSorting",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "Sorting mode"),
            )
            obj.PointsSorting = ("Automatic", "Manual")

        if not hasattr(obj, "JitterAngle"):
            obj.addProperty(
                "App::PropertyAngle",
                "JitterAngle",
                "Random",
                QT_TRANSLATE_NOOP("App::Property", "Max angle of rotation for jitter randomness"),
            )

        if not hasattr(obj, "ArrayGroup"):
            obj.addProperty(
                "App::PropertyLinkListHidden",
                "ArrayGroup",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "List array objects"),
            )
        if not hasattr(obj, "Combine"):
            obj.addProperty(
                "App::PropertyBool",
                "Combine",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If operations with identical tool controller and without coolant:"
                    "\ncombine by copies"
                    "\n\nIf operations with different tool controllers or with coolant:"
                    "\ncombine operations with same tool controller and coolant mode, "
                    "\nbut only if operations placed one by one in tree",
                ),
            )
        if not hasattr(obj, "Split"):
            obj.addProperty(
                "App::PropertyBool",
                "Split",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Split path by elements even all base operations without coolant and with identical tool controller",
                ),
            )
        if not hasattr(obj, "AddBasePath"):
            obj.addProperty(
                "App::PropertyBool",
                "AddBasePath",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Add base path to array operation"),
            )

        self.setEditorModes(obj)

    def execute(self, obj):
        """Do not generate paths and clear current Path data,
        if operation not Active or no base operations or operations not compatible"""
        if not obj.Active or not self.isBaseCompatible(obj):
            obj.Path = Path.Path()
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
            self.getPathCenter(obj),
            obj.ExtraClearanceHeight.Value,
            obj.PointsSource,
            obj.PointsOrigin,
            obj.PointsSorting,
            obj.AddBasePath,
        )

        pathArray = PathArray(*args).getPath()
        if self.isBaseCompatibleStrict(obj) and not obj.Split:
            # old type of array
            self.cleanArrayGroup(obj, 0)
            self.group = []
            cmds = [cmd for op in pathArray for cmd in op["path"].Commands]
            obj.Path = Path.Path(cmds)
            obj.CycleTime = PathOp.getCycleTimeEstimate(obj)
        else:
            # new type of array with separated copies
            self.group = pathArray
            if obj.Combine:
                self.processGroupCompound(obj)
            else:
                self.processGroup(obj)
            self.processGroupCycleTime(obj)
            obj.Path = Path.Path()  # trigger to recompute elements in Group

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
        if obj.Base:
            tc0 = toolControllerForOp(obj.Base[0])

        for base in obj.Base[1:]:
            tc = toolControllerForOp(base)
            if tc != tc0:
                return False

            if coolantModeForOp(base) != "None":
                return False

        return True

    # Get center point of all base operations
    def getPathCenter(self, obj):
        path = Path.Path()
        for op in obj.Base:
            path.addCommands(op.Path.Commands)
        return path.BoundBox.Center

    def cleanArrayGroup(self, obj, amount):
        while len(obj.ArrayGroup) > amount:
            # remove extra Path object from obj.ArrayGroup list
            op = obj.ArrayGroup[-1]
            obj.ArrayGroup = obj.ArrayGroup[:-1]
            op.Document.removeObject(op.Name)

    def prepareArrayGroup(self, obj):
        for op in obj.ArrayGroup:
            op.Base = []
            op.PathTemp = Path.Path()

    # create new Path Array object
    def addNewArrayElement(self, obj):
        doc = FreeCAD.ActiveDocument
        newPathObj = doc.addObject("Path::FeaturePython", "Array")
        ObjectArrayChild(newPathObj)
        newPathObj.ViewObject.Proxy = ViewProviderArrayChild(newPathObj.ViewObject)
        obj.ArrayGroup = obj.ArrayGroup + [newPathObj]

        return newPathObj

    # Create separated Path elements for each repeat and each object
    def processGroup(self, obj):
        if not self.group:
            # remove all child elements
            self.cleanArrayGroup(obj, 0)
            return

        doc = FreeCAD.ActiveDocument
        counterCopies = 0
        for i, op in enumerate(self.group):
            if i >= len(obj.ArrayGroup):
                self.addNewArrayElement(obj)

            if op["opName"] == obj.Base[0].Name:
                counterCopies += 1
                cp = f"_{counterCopies}"

            baseOp = doc.getObject(op["opName"])
            obj.ArrayGroup[i].Label = f"Array{cp}_{baseOp.Label}_{i:03d}"
            obj.ArrayGroup[i].Base = [baseOp.Name]
            obj.ArrayGroup[i].ToolController = toolControllerForOp(baseOp)
            obj.ArrayGroup[i].CoolantMode = coolantModeForOp(baseOp)
            obj.ArrayGroup[i].PathTemp = op["path"]

            if obj.ArrayGroup[i].Active:
                obj.ArrayGroup[i].Path = op["path"]
            else:
                obj.ArrayGroup[i].Path = Path.Path()

        self.cleanArrayGroup(obj, len(self.group))

    # Combine elements if Combine is True
    def processGroupCompound(self, obj):
        if not self.group:
            # remove all child elements
            self.cleanArrayGroup(obj, 0)
            return

        self.prepareArrayGroup(obj)
        doc = FreeCAD.ActiveDocument
        cp = ""
        counterCopies = 0
        i = 0
        lastToolController = None
        lastCoolant = None
        for op in self.group:
            baseOp = doc.getObject(op["opName"])
            toolController = toolControllerForOp(baseOp)
            coolantMode = coolantModeForOp(baseOp)

            if counterCopies:
                if (
                    op["opName"] == obj.Base[0].Name
                    or lastToolController != toolController
                    or lastCoolant != coolantMode
                ):
                    i += 1

            if i > len(obj.ArrayGroup) - 1:
                self.addNewArrayElement(obj)

            cmName = f"_{coolantMode}" if coolantMode != "None" else ""
            if op["opName"] == obj.Base[0].Name:
                counterCopies += 1
                cp = f"_{counterCopies}"

            obj.ArrayGroup[i].Label = f"Array{cp}{cmName}_{toolController.Label}"
            obj.ArrayGroup[i].ToolController = toolController
            obj.ArrayGroup[i].CoolantMode = coolantMode

            obj.ArrayGroup[i].Base += [op["opName"]]
            obj.ArrayGroup[i].PathTemp.addCommands(op["path"].Commands)

            if obj.ArrayGroup[i].Active:
                obj.ArrayGroup[i].Path = obj.ArrayGroup[i].PathTemp
            else:
                obj.ArrayGroup[i].Path = Path.Path()

            lastToolController = toolController
            lastCoolant = coolantMode

        self.cleanArrayGroup(obj, i + 1)

    # Get total cycle time ArrayMultiToolController
    def processGroupCycleTime(self, obj):
        totalTime = 0
        for op in obj.ArrayGroup:
            temp = op.CycleTime.split(":")
            if len(temp) == 3:
                (h, m, s) = temp
                opTime = datetime.timedelta(hours=int(h), minutes=int(m), seconds=int(s))
                totalTime += opTime.seconds
            else:
                obj.CycleTime = translate("CAM", "Cycle time error (%s)" % op.Label)
                break
        else:
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
        if prop == "Path":
            obj.CycleTime = PathOp.getCycleTimeEstimate(obj)

        if prop == "Active" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def onDocumentRestored(self, obj):
        return

    def execute(self, obj):
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
        extraClearanceHeight,
        pointsSource,
        pointsOrigin,
        pointsSorting,
        addBasePath,
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
        self.extraClearanceHeight = extraClearanceHeight
        self.pointsSource = pointsSource
        self.pointsOrigin = pointsOrigin
        self.pointsSorting = pointsSorting
        self.addBasePath = addBasePath

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

    def addMiddleMoves(self, cmds):
        """Returns list of commands for rapid moves between repeats with specific height"""
        x = y = z = index = None

        for i, cmd in enumerate(cmds):
            x = cmd.x if x is None and cmd.x is not None else x
            y = cmd.y if y is None and cmd.y is not None else y
            z = cmd.z if z is None and cmd.z is not None else z
            if index is None and (x is not None or y is not None or z is not None):
                # first move command in operation
                index = i
            if x is not None and y is not None and z is not None:
                break
        else:
            return cmds

        z += self.extraClearanceHeight
        extraCommands = []
        extraCommands.append(Path.Command("G0", {"Z": z}))
        extraCommands.append(Path.Command("G0", {"X": x, "Y": y}))
        for cmd in reversed(extraCommands):
            cmds.insert(index, cmd)

        return cmds

    def getStartPoint(self, cmds):
        """First tool position in first base operation"""
        x = y = z = None

        for cmd in cmds:
            x = cmd.x if x is None and cmd.x is not None else x
            y = cmd.y if y is None and cmd.y is not None else y
            z = cmd.z if z is None and cmd.z is not None else z

            if x is not None and y is not None and z is not None:
                return FreeCAD.Vector(x, y, z)

        return FreeCAD.Vector()

    def getEndPoint(self, cmds):
        """Last tool position in last base operation"""
        x = y = z = None

        for cmd in reversed(cmds):
            x = cmd.x if x is None and cmd.x is not None else x
            y = cmd.y if y is None and cmd.y is not None else y
            z = cmd.z if z is None and cmd.z is not None else z

            if x is not None and y is not None and z is not None:
                return FreeCAD.Vector(x, y, z)

        return FreeCAD.Vector()

    def getLinear1DArray(self, pathGroup):
        """Array type Linear1D"""
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

    def getLinear2DXYArray(self, pathGroup):
        """Array type Linear2D with initial X direction"""
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

    def getLinear2DYXArray(self, pathGroup):
        """Array type Linear2D with initial Y direction"""
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

    def getPolarArray(self, pathGroup):
        """Array type Polar"""
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

    def getPointsArray(self, pathGroup):
        """Array type Points"""

        # get points from base objects
        points = []
        for base in self.pointsSource:
            (baseObj, subNames) = base
            if not subNames or subNames == ("",):
                # no sub elements selection
                if not baseObj.Shape.Edges:
                    # shape contain only vertexes
                    points.extend([{"point": v.Point, "angle": 0} for v in baseObj.Shape.Vertexes])
                else:
                    # shape contain edges
                    point = baseObj.Shape.Vertexes[0].Point
                    angle = self.getEdgeAngle(baseObj.Shape.Edges[0])
                    points.append({"point": point, "angle": angle})

            else:
                # with sub elements selection
                for sub in subNames:
                    if sub.startswith("Vertex"):
                        points.append({"point": baseObj.Shape.getElement(sub).Point, "angle": 0})
                    elif sub.startswith("Edge"):
                        edge = baseObj.Shape.getElement(sub)
                        angle = self.getEdgeAngle(edge)
                        points.append({"point": edge.Vertexes[0].Point, "angle": angle})

        # Apply origin offset to each point
        originPoint = FreeCAD.Vector()
        originAngle = 0
        if self.pointsOrigin:
            (originObj, originSubNames) = self.pointsOrigin

            if not originSubNames or subNames == ("",):
                # no sub element selection
                if originObj.Shape.Edges:
                    # shape contain edges
                    originPoint = originObj.Shape.Edges[0].Vertexes[0].Point
                    originAngle = self.getEdgeAngle(originObj.Shape.Edges[0])
                else:
                    # shape contain only vertexes
                    originPoint = originObj.Shape.Vertexes[0].Point

            else:
                # with sub element selection
                originPoint = originObj.Shape.getElement(originSubNames[0]).Vertexes[0].Point
                if originSubNames[0].startswith("Edge"):
                    originEdge = originObj.Shape.getElement(originSubNames[0])
                    originAngle = self.getEdgeAngle(originEdge)

            for pos in points:
                pos["point"] -= originPoint
                pos["angle"] -= originAngle

        # get sorted positions for array
        if self.pointsSorting == "Automatic":
            basePathStartPoint = self.getStartPoint(self.base[0].Path.Commands)
            basePathEndPoint = self.getEndPoint(self.base[-1].Path.Commands)
            isOpen = not Path.Geom.pointsCoincide(basePathStartPoint, basePathEndPoint)
            dirStart = basePathStartPoint - originPoint
            dirEnd = basePathEndPoint - originPoint
            route = []
            for i, pos in enumerate(points):
                origin = originPoint + pos["point"]
                dirStartOffset = DraftVecUtils.rotate(
                    dirStart, math.radians(pos["angle"]), FreeCAD.Vector(0, 0, 1)
                )
                dirEndOffset = DraftVecUtils.rotate(
                    dirEnd, math.radians(pos["angle"]), FreeCAD.Vector(0, 0, 1)
                )
                route.append(
                    {
                        "index": i,
                        "isOpen": isOpen,
                        "startX": origin.x + dirStartOffset.x,
                        "startY": origin.y + dirStartOffset.y,
                        "endX": origin.x + dirEndOffset.x,
                        "endY": origin.y + dirEndOffset.y,
                        "flipped": False,
                        "point": pos["point"],
                        "a": pos["angle"],
                    }
                )

            route = PathUtils.tsp_solver_tunnels(route, routeStartPoint=basePathEndPoint)
            points = [{"point": pos["point"], "angle": pos["a"]} for pos in route]

        for pos in points:
            # apply jitter
            point, alpha = self.calculateJitter(pos["point"])

            for bIndex, b in enumerate(self.base):
                pl = FreeCAD.Placement()
                pl.move(point)
                pl.rotate(self.jitterCentre, FreeCAD.Vector(0, 0, 1), alpha)
                pl.rotate(originPoint, FreeCAD.Vector(0, 0, 1), pos["angle"])
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

    def getEdgeAngle(self, edge):
        direction = edge.Vertexes[-1].Point - edge.Vertexes[0].Point
        if Path.Geom.pointsCoincide(direction, FreeCAD.Vector()):
            return 0
        angle = math.degrees(direction.getAngle(FreeCAD.Vector(0, 1, 0)))
        if direction.x > 0:
            angle = -angle

        return angle

    def getPath(self):
        """getPath() ... Call this method on an instance of the class to generate and return
        path data for the requested path array."""

        if self.addBasePath:
            for base in self.base:
                basePath = PathUtils.getPathWithPlacement(base)
                pathGroup = [{"path": basePath, "opName": base.Name}]
        else:
            pathGroup = []

        if self.arrayType == "Polar":
            self.getPolarArray(pathGroup)

        elif self.arrayType == "Linear2D":
            if self.swapDirection:
                self.getLinear2DXYArray(pathGroup)
            else:
                self.getLinear2DYXArray(pathGroup)

        elif self.arrayType == "Points":
            self.getPointsArray(pathGroup)

        else:
            self.getLinear1DArray(pathGroup)

        return pathGroup


class ViewProviderArray:
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

    def onChanged(self, vobj, prop):
        if prop == "Visibility":
            for op in self.obj.ArrayGroup:
                op.Visibility = vobj.Visibility

    def claimChildren(self):
        return [base for base in self.obj.ArrayGroup]

    def onDelete(self, vobj, args):
        for op in self.obj.ArrayGroup:
            op.Document.removeObject(op.Name)
        self.obj.Document.removeObject(self.obj.Name)

    def getIcon(self):
        if self.obj.Active:
            return ":/icons/CAM_ArrayMTC.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


class ViewProviderArrayChild:

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

    def onChanged(self, vobj, prop):
        return

    def getIcon(self):
        if self.obj.Active:
            return ":/icons/CAM_Array.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


class CommandPathArray:
    def GetResources(self):
        return {
            "Pixmap": "CAM_ArrayMTC",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Array", "Array"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_Array",
                "Creates an array with multiple tool controllers and coolant modes",
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
