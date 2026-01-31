# SPDX-License-Identifier: LGPL-2.1-or-later

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
import Path.Base.Util as PathUtil
from Path.Dressup.Utils import toolController

import random

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

        # Random properties group
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

        obj.Active = True
        obj.Type = ["Linear1D", "Linear2D", "Polar"]
        obj.Copies = (0, 0, 99999, 1)
        obj.CopiesX = (0, 0, 99999, 1)
        obj.CopiesY = (0, 0, 99999, 1)
        obj.JitterSeed = (0, 0, 2147483647, 1)

        self.setEditorModes(obj)
        obj.Proxy = self

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def setEditorModes(self, obj):
        obj.setEditorMode("ToolController", 2)  # hidden
        obj.setEditorMode("CycleTime", 1)  # read-only

        angleMode = centreMode = copiesXMode = copiesYMode = swapDirectionMode = 2
        copiesMode = offsetMode = 2
        if obj.Type == "Linear1D":
            copiesMode = offsetMode = 0
        elif obj.Type == "Linear2D":
            copiesXMode = copiesYMode = offsetMode = swapDirectionMode = 0
        elif obj.Type == "Polar":
            angleMode = copiesMode = centreMode = 0

        obj.setEditorMode("Angle", angleMode)
        obj.setEditorMode("Centre", centreMode)
        obj.setEditorMode("Copies", copiesMode)
        obj.setEditorMode("CopiesX", copiesXMode)
        obj.setEditorMode("CopiesY", copiesYMode)
        obj.setEditorMode("Offset", offsetMode)
        obj.setEditorMode("SwapDirection", swapDirectionMode)
        obj.setEditorMode("JitterPercent", 0)
        obj.setEditorMode("JitterMagnitude", 0)
        obj.setEditorMode("JitterSeed", 0)

    def onChanged(self, obj, prop):
        if prop == "Type" and not obj.Document.Restoring:
            self.setEditorModes(obj)

        if prop == "Active" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def onDocumentRestored(self, obj):
        """onDocumentRestored(obj) ... Called automatically when document is restored."""
        if not obj.ViewObject.Proxy:
            Path.Op.Gui.Array.ViewProviderArray(obj.ViewObject)

        if not hasattr(obj, "Active"):
            obj.addProperty(
                "App::PropertyBool",
                "Active",
                "Path",
                QT_TRANSLATE_NOOP(
                    "PathOp", "Make False, to prevent operation from generating code"
                ),
            )
            obj.Active = True

        if not hasattr(obj, "JitterSeed"):
            obj.addProperty(
                "App::PropertyInteger",
                "JitterSeed",
                "Random",
                QT_TRANSLATE_NOOP("App::Property", "Seed value for jitter randomness"),
            )
            obj.JitterSeed = 0

        if not hasattr(obj, "CycleTime"):
            obj.addProperty(
                "App::PropertyString",
                "CycleTime",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Operations cycle time estimation"),
            )
            obj.CycleTime = self.getCycleTimeEstimate(obj)

        self.setEditorModes(obj)

    def execute(self, obj):
        # backwards compatibility for PathArrays created before support for multiple bases
        if isinstance(obj.Base, list):
            base = obj.Base
        else:
            base = [obj.Base]

        # Do not generate paths and clear current Path data
        # if operation not Active or no base operations or operations not compatible
        if not obj.Active or len(base) == 0 or not self.isBaseCompatible(obj):
            obj.Path = Path.Path()
            return

        obj.ToolController = toolController(base[0])

        # use seed if specified, otherwise default to object name for consistency during recomputes
        seed = obj.JitterSeed or obj.Name

        pa = PathArray(
            obj.Base,
            obj.Type,
            obj.Copies,
            obj.Offset,
            obj.CopiesX,
            obj.CopiesY,
            obj.Angle,
            obj.Centre,
            obj.SwapDirection,
            obj.JitterMagnitude,
            obj.JitterPercent,
            seed,
        )

        obj.Path = pa.getPath()
        obj.CycleTime = PathOp.getCycleTimeEstimate(obj)

    def isBaseCompatible(self, obj):
        if not obj.Base:
            return False
        tcs = []
        cms = []
        for sel in obj.Base:
            if not sel.isDerivedFrom("Path::Feature"):
                return False
            tcs.append(toolController(sel))
            cms.append(PathUtil.coolantModeForOp(sel))

        if tcs == {None} or len(set(tcs)) > 1:
            Path.Log.warning(
                translate(
                    "PathArray",
                    "Arrays of toolpaths having different tool controllers or tool controller not selected.",
                )
            )
            return False

        if set(cms) != {"None"}:
            Path.Log.warning(
                translate(
                    "PathArray",
                    "Arrays not compatible with coolant modes.",
                )
            )
            return False

        return True


class PathArray:
    """class PathArray ...
    This class receives one or more base operations and repeats those operations
    at set intervals based upon array type requested and the related settings for that type."""

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
        jitterMagnitude,
        jitterPercent,
        seed,
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
        self.jitterMagnitude = jitterMagnitude
        self.jitterPercent = jitterPercent
        self.seed = seed

    def getPath(self):
        """getPath() ... Call this method on an instance of the class to generate and return
        path data for the requested path array."""

        commands = []
        random.seed(self.seed)

        if self.arrayType == "Polar":
            self.getPolarArray(commands)
        elif self.arrayType == "Linear2D":
            if self.swapDirection:
                self.getLinear2DXYArray(commands)
            else:
                self.getLinear2DYXArray(commands)
        else:
            self.getLinear1DArray(commands)

        return Path.Path(commands)

    def calculateJitter(self, pos):
        """Returns the position argument with a random vector shift applied."""
        if self.jitterPercent == 0:
            pass
        elif random.randint(0, 100) < self.jitterPercent:
            pos.x = pos.x + random.uniform(-self.jitterMagnitude.x, self.jitterMagnitude.x)
            pos.y = pos.y + random.uniform(-self.jitterMagnitude.y, self.jitterMagnitude.y)
            pos.z = pos.z + random.uniform(-self.jitterMagnitude.z, self.jitterMagnitude.z)
        return pos

    def getLinear1DArray(self, commands):
        """Array type Linear1D"""
        for i in range(self.copies):
            pos = FreeCAD.Vector(
                self.offsetVector.x * (i + 1),
                self.offsetVector.y * (i + 1),
                self.offsetVector.z * (i + 1),
            )
            pos = self.calculateJitter(pos)

            for b in self.base:
                pl = FreeCAD.Placement()
                pl.move(pos)
                path = PathUtils.getPathWithPlacement(b)
                path = PathUtils.applyPlacementToPath(pl, path)
                commands.extend(path.Commands)

    def getLinear2DXYArray(self, commands):
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
                pos = self.calculateJitter(pos)

                for b in self.base:
                    pl = FreeCAD.Placement()
                    # index 0,0 will be processed by the base Paths themselves
                    if i != 0 or j != 0:
                        pl.move(pos)
                        path = PathUtils.getPathWithPlacement(b)
                        path = PathUtils.applyPlacementToPath(pl, path)
                        commands.extend(path.Commands)

    def getLinear2DYXArray(self, commands):
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
                pos = self.calculateJitter(pos)

                for b in self.base:
                    pl = FreeCAD.Placement()
                    # index 0,0 will be processed by the base Paths themselves
                    if i != 0 or j != 0:
                        pl.move(pos)
                        path = PathUtils.getPathWithPlacement(b)
                        path = PathUtils.applyPlacementToPath(pl, path)
                        commands.extend(path.Commands)

    def getPolarArray(self, commands):
        """Array type Polar"""
        for i in range(self.copies):
            ang = 360
            if self.copies > 0:
                ang = self.polarAngle / self.copies * (1 + i)

            # prepare placement for polar pattern
            pl = FreeCAD.Placement()
            pl.rotate(self.polarCentre, FreeCAD.Vector(0, 0, 1), ang)

            for b in self.base:
                path = PathUtils.getPathWithPlacement(b)
                path = PathUtils.applyPlacementToPath(pl, path)
                commands.extend(path.Commands)


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
        return None

    def claimChildren(self):
        return []

    def onDelete(self, vobj, args):
        return True

    def getIcon(self):
        if self.obj.Active:
            return ":/icons/CAM_Array.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


class CommandPathArray:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Array",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Array", "Array"),
            "ToolTip": QT_TRANSLATE_NOOP("CAM_Array", "Creates an array from selected toolpaths"),
        }

    def IsActive(self):
        selection = FreeCADGui.Selection.getSelection()
        if not selection:
            return False
        tcs = []
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

        FreeCADGui.doCommand("Path.Op.Gui.Array.ViewProviderArray(obj.ViewObject)")
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(obj.Base[0])")
        FreeCADGui.doCommand("PathScripts.PathUtils.addToJob(obj, job.Name)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Array", CommandPathArray())
