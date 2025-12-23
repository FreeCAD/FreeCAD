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
from PySide import QtCore

import random

from PySide.QtCore import QT_TRANSLATE_NOOP

__doc__ = """CAM Array object and FreeCAD command"""

translate = FreeCAD.Qt.translate


class ObjectArray:
    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyLinkList",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The toolpaths to array"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "Type",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Pattern method"),
        )
        obj.addProperty(
            "App::PropertyVectorDistance",
            "Offset",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The spacing between the array copies in linear pattern",
            ),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "CopiesX",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The number of copies in X-direction in linear pattern"
            ),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "CopiesY",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The number of copies in Y-direction in linear pattern"
            ),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "Angle",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Total angle in polar pattern"),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "Copies",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The number of copies in linear 1D and polar pattern"
            ),
        )
        obj.addProperty(
            "App::PropertyVector",
            "Centre",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The centre of rotation in polar pattern"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "SwapDirection",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Make copies in X direction before Y in Linear 2D pattern",
            ),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "JitterPercent",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Percent of copies to randomly offset"),
        )
        obj.addProperty(
            "App::PropertyVectorDistance",
            "JitterMagnitude",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Maximum random offset of copies"),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "JitterSeed",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Seed value for jitter randomness"),
        )
        obj.addProperty(
            "App::PropertyLink",
            "ToolController",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The tool controller that will be used to calculate the toolpath",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "Active",
            "Path",
            QT_TRANSLATE_NOOP("PathOp", "Make False, to prevent operation from generating code"),
        )
        obj.addProperty(
            "App::PropertyString",
            "CycleTime",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Operations cycle time estimation"),
        )

        obj.setEditorMode("CycleTime", 1)  # read-only
        obj.Active = True
        obj.Type = ["Linear1D", "Linear2D", "Polar"]

        self.setEditorModes(obj)
        obj.Proxy = self

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def setEditorModes(self, obj):
        if obj.Type == "Linear1D":
            angleMode = centreMode = copiesXMode = copiesYMode = swapDirectionMode = 2
            copiesMode = offsetMode = 0
        elif obj.Type == "Linear2D":
            angleMode = copiesMode = centreMode = 2
            copiesXMode = copiesYMode = offsetMode = swapDirectionMode = 0
        elif obj.Type == "Polar":
            angleMode = copiesMode = centreMode = 0
            copiesXMode = copiesYMode = offsetMode = swapDirectionMode = 2

        obj.setEditorMode("Angle", angleMode)
        obj.setEditorMode("Copies", copiesMode)
        obj.setEditorMode("Centre", centreMode)
        obj.setEditorMode("CopiesX", copiesXMode)
        obj.setEditorMode("CopiesY", copiesYMode)
        obj.setEditorMode("Offset", offsetMode)
        obj.setEditorMode("SwapDirection", swapDirectionMode)
        obj.setEditorMode("JitterPercent", 0)
        obj.setEditorMode("JitterMagnitude", 0)
        obj.setEditorMode("JitterSeed", 0)
        obj.setEditorMode("ToolController", 2)

    def onChanged(self, obj, prop):
        if prop == "Type":
            self.setEditorModes(obj)

        if prop == "Active" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def onDocumentRestored(self, obj):
        """onDocumentRestored(obj) ... Called automatically when document is restored."""

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
                "Path",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Seed value for jitter randomness"),
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
        baseList,
        arrayType,
        copies,
        offsetVector,
        copiesX,
        copiesY,
        angle,
        centre,
        swapDirection,
        jitterMagnitude=FreeCAD.Vector(0, 0, 0),
        jitterPercent=0,
        seed="FreeCAD",
    ):
        self.baseList = list()
        self.arrayType = arrayType  # ['Linear1D', 'Linear2D', 'Polar']
        self.copies = copies
        self.offsetVector = offsetVector
        self.copiesX = copiesX
        self.copiesY = copiesY
        self.angle = angle
        self.centre = centre
        self.swapDirection = swapDirection
        self.jitterMagnitude = jitterMagnitude
        self.jitterPercent = jitterPercent
        self.seed = seed

        if baseList:
            if isinstance(baseList, list):
                self.baseList = baseList
            else:
                self.baseList = [baseList]

    # Private method
    def _calculateJitter(self, pos):
        """_calculateJitter(pos) ...
        Returns the position argument with a random vector shift applied."""
        if self.jitterPercent == 0:
            pass
        elif random.randint(0, 100) < self.jitterPercent:
            pos.x = pos.x + random.uniform(-self.jitterMagnitude.x, self.jitterMagnitude.x)
            pos.y = pos.y + random.uniform(-self.jitterMagnitude.y, self.jitterMagnitude.y)
            pos.z = pos.z + random.uniform(-self.jitterMagnitude.z, self.jitterMagnitude.z)
        return pos

    # Public method
    def getPath(self):
        """getPath() ... Call this method on an instance of the class to generate and return
        path data for the requested path array."""

        base = self.baseList
        for b in base:
            if not b.isDerivedFrom("Path::Feature"):
                return
            if not b.Path:
                return

            b_tool_controller = toolController(b)
            if not b_tool_controller:
                return

        # build copies
        output = ""
        random.seed(self.seed)

        if self.arrayType == "Linear1D":
            for i in range(self.copies):
                pos = FreeCAD.Vector(
                    self.offsetVector.x * (i + 1),
                    self.offsetVector.y * (i + 1),
                    self.offsetVector.z * (i + 1),
                )
                pos = self._calculateJitter(pos)

                for b in base:
                    pl = FreeCAD.Placement()
                    pl.move(pos)
                    np = Path.Path(
                        [cm.transform(pl) for cm in PathUtils.getPathWithPlacement(b).Commands]
                    )
                    output += np.toGCode()

        elif self.arrayType == "Linear2D":
            if self.swapDirection:
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
                        pos = self._calculateJitter(pos)

                        for b in base:
                            pl = FreeCAD.Placement()
                            # do not process the index 0,0. It will be processed by the base Paths themselves
                            if not (i == 0 and j == 0):
                                pl.move(pos)
                                np = Path.Path(
                                    [
                                        cm.transform(pl)
                                        for cm in PathUtils.getPathWithPlacement(b).Commands
                                    ]
                                )
                                output += np.toGCode()
            else:
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
                        pos = self._calculateJitter(pos)

                        for b in base:
                            pl = FreeCAD.Placement()
                            # do not process the index 0,0. It will be processed by the base Paths themselves
                            if not (i == 0 and j == 0):
                                pl.move(pos)
                                np = Path.Path(
                                    [
                                        cm.transform(pl)
                                        for cm in PathUtils.getPathWithPlacement(b).Commands
                                    ]
                                )
                                output += np.toGCode()
            # Eif
        else:
            for i in range(self.copies):
                for b in base:
                    ang = 360
                    if self.copies > 0:
                        ang = self.angle / self.copies * (1 + i)

                    pl = FreeCAD.Placement()
                    pl.rotate(self.centre, FreeCAD.Vector(0, 0, 1), ang)
                    np = PathUtils.applyPlacementToPath(pl, PathUtils.getPathWithPlacement(b))
                    output += np.toGCode()

        # return output
        return Path.Path(output)


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
        return None

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

        FreeCADGui.doCommand(
            "obj.ViewObject.Proxy = Path.Op.Gui.Array.ViewProviderArray(obj.ViewObject)"
        )
        FreeCADGui.doCommand("PathScripts.PathUtils.addToJob(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Array", CommandPathArray())
