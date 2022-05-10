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
import PathScripts
from PathScripts import PathLog
from PathScripts.PathDressup import toolController
from PySide import QtCore
import math
import random
from PySide.QtCore import QT_TRANSLATE_NOOP

__doc__ = """Path Array object and FreeCAD command"""

translate = FreeCAD.Qt.translate


class ObjectArray:
    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyLinkList",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The path(s) to array"),
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
                "The spacing between the array copies in Linear pattern",
            ),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "CopiesX",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The number of copies in X direction in Linear pattern"
            ),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "CopiesY",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The number of copies in Y direction in Linear pattern"
            ),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "Angle",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Total angle in Polar pattern"),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "Copies",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The number of copies in Linear 1D and Polar pattern"
            ),
        )
        obj.addProperty(
            "App::PropertyVector",
            "Centre",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The centre of rotation in Polar pattern"
            ),
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
                "The tool controller that will be used to calculate the path",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "Active",
            "Path",
            QT_TRANSLATE_NOOP(
                "PathOp", "Make False, to prevent operation from generating code"
            ),
        )

        obj.Active = True
        obj.Type = ["Linear1D", "Linear2D", "Polar"]

        self.setEditorModes(obj)
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
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

        if not hasattr(obj, "JitterSeed"):
            obj.addProperty(
                "App::PropertyInteger",
                "JitterSeed",
                "Path",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Seed value for jitter randomness"
                ),
            )
            obj.JitterSeed = 0

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

        self.setEditorModes(obj)

    def rotatePath(self, path, angle, centre):
        """
        Rotates Path around given centre vector
        Only X and Y is considered
        """
        CmdMoveRapid = ["G0", "G00"]
        CmdMoveStraight = ["G1", "G01"]
        CmdMoveCW = ["G2", "G02"]
        CmdMoveCCW = ["G3", "G03"]
        CmdDrill = ["G81", "G82", "G83"]
        CmdMoveArc = CmdMoveCW + CmdMoveCCW
        CmdMove = CmdMoveStraight + CmdMoveArc

        commands = []
        ang = angle / 180 * math.pi
        currX = 0
        currY = 0
        for cmd in path.Commands:
            if (
                (cmd.Name in CmdMoveRapid)
                or (cmd.Name in CmdMove)
                or (cmd.Name in CmdDrill)
            ):
                params = cmd.Parameters
                x = params.get("X")
                if x is None:
                    x = currX
                currX = x
                y = params.get("Y")
                if y is None:
                    y = currY
                currY = y

                # "move" the centre to origin
                x = x - centre.x
                y = y - centre.y

                # rotation around origin:
                nx = x * math.cos(ang) - y * math.sin(ang)
                ny = y * math.cos(ang) + x * math.sin(ang)

                # "move" the centre back and update
                params.update({"X": nx + centre.x, "Y": ny + centre.y})

                # Arcs need to have the I and J params rotated as well
                if cmd.Name in CmdMoveArc:
                    i = params.get("I")
                    if i is None:
                        i = 0
                    j = params.get("J")
                    if j is None:
                        j = 0

                    ni = i * math.cos(ang) - j * math.sin(ang)
                    nj = j * math.cos(ang) + i * math.sin(ang)
                    params.update({"I": ni, "J": nj})

                cmd.Parameters = params
            commands.append(cmd)
        newPath = Path.Path(commands)

        return newPath

    def execute(self, obj):
        # backwards compatibility for PathArrays created before support for multiple bases
        if isinstance(obj.Base, list):
            base = obj.Base
        else:
            base = [obj.Base]

        if len(base) == 0:
            return

        obj.ToolController = toolController(base[0])

        # Do not generate paths and clear current Path data if operation not
        if not obj.Active:
            if obj.Path:
                obj.Path = Path.Path()
            return

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
            pos.x = pos.x + random.uniform(
                -self.jitterMagnitude.x, self.jitterMagnitude.x
            )
            pos.y = pos.y + random.uniform(
                -self.jitterMagnitude.y, self.jitterMagnitude.y
            )
            pos.z = pos.z + random.uniform(
                -self.jitterMagnitude.z, self.jitterMagnitude.z
            )
        return pos

    # Public method
    def getPath(self):
        """getPath() ... Call this method on an instance of the class to generate and return
        path data for the requested path array."""

        if len(self.baseList) == 0:
            PathLog.error(translate("PathArray", "No base objects for PathArray."))
            return None

        base = self.baseList
        for b in base:
            if not b.isDerivedFrom("Path::Feature"):
                return
            if not b.Path:
                return

            b_tool_controller = toolController(b)
            if not b_tool_controller:
                return

            if b_tool_controller != toolController(base[0]):
                # this may be important if Job output is split by tool controller
                PathLog.warning(
                    translate(
                        "PathArray",
                        "Arrays of paths having different tool controllers are handled according to the tool controller of the first path.",
                    )
                )

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
                    np = Path.Path([cm.transform(pl) for cm in b.Path.Commands])
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
                                    [cm.transform(pl) for cm in b.Path.Commands]
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
                                    [cm.transform(pl) for cm in b.Path.Commands]
                                )
                                output += np.toGCode()
            # Eif
        else:
            for i in range(self.copies):
                for b in base:
                    ang = 360
                    if self.copies > 0:
                        ang = self.angle / self.copies * (1 + i)
                    np = self.rotatePath(b.Path.Commands, ang, self.centre)
                    output += np.toGCode()

        # return output
        return Path.Path(output)


class ViewProviderArray:
    def __init__(self, vobj):
        self.Object = vobj.Object
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def claimChildren(self):
        if hasattr(self, "Object"):
            if hasattr(self.Object, "Base"):
                if self.Object.Base:
                    return self.Object.Base
        return []


class CommandPathArray:
    def GetResources(self):
        return {
            "Pixmap": "Path_Array",
            "MenuText": QT_TRANSLATE_NOOP("Path_Array", "Array"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Path_Array", "Creates an array from selected path(s)"
            ),
        }

    def IsActive(self):
        selections = [
            sel.isDerivedFrom("Path::Feature")
            for sel in FreeCADGui.Selection.getSelection()
        ]
        return selections and all(selections)

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()

        for sel in selection:
            if not (sel.isDerivedFrom("Path::Feature")):
                FreeCAD.Console.PrintError(
                    translate(
                        "Path_Array", "Arrays can be created only from Path operations."
                    )
                    + "\n"
                )
                return

        # if everything is ok, execute and register the transaction in the
        # undo/redo stack
        FreeCAD.ActiveDocument.openTransaction("Create Array")
        FreeCADGui.addModule("PathScripts.PathArray")
        FreeCADGui.addModule("PathScripts.PathUtils")

        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Array")'
        )

        FreeCADGui.doCommand("PathScripts.PathArray.ObjectArray(obj)")

        baseString = "[%s]" % ",".join(
            ["FreeCAD.ActiveDocument.%s" % sel.Name for sel in selection]
        )
        FreeCADGui.doCommand("obj.Base = %s" % baseString)

        FreeCADGui.doCommand("obj.ViewObject.Proxy = 0")
        FreeCADGui.doCommand("PathScripts.PathUtils.addToJob(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("Path_Array", CommandPathArray())
