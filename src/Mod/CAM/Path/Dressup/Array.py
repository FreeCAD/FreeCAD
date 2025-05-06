# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
# *   Reimplemented as dressup in 2025  phaseloop <phaseloop@protonmail.com *
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
import Path
import PathScripts.PathUtils as PathUtils
from Path.Dressup.Base import DressupBase
import random
from PySide.QtCore import QT_TRANSLATE_NOOP

__doc__ = """CAM Array dressup"""

translate = FreeCAD.Qt.translate


class DressupArray(DressupBase):
    def __init__(self, obj, base, job):
        super().__init__(obj, base)

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
            QT_TRANSLATE_NOOP("App::Property", "The centre of rotation in Polar pattern"),
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

        self.obj = obj
        obj.Base = base

        obj.Active = True
        # assigning array tells the type of possible enum choices
        obj.Type = ["Linear1D", "Linear2D", "Polar"]
        # assign value
        obj.Type = "Linear1D"

        obj.Copies = 0
        obj.JitterPercent = 0

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

    def onChanged(self, obj, prop):
        if prop == "Type":
            self.setEditorModes(obj)

    def dresupOnDocumentRestored(self, obj):
        """onDocumentRestored(obj) ... Called automatically when document is restored."""
        self.obj = obj
        self.setEditorModes(obj)

    def onDelete(self, obj, args):
        if obj.Base:
            job = PathUtils.findParentJob(obj)
            if job:
                job.Proxy.addOperation(obj.Base, obj)
            if obj.Base.ViewObject:
                obj.Base.ViewObject.Visibility = True
            obj.Base = None
        return True

    def dressupExecute(self, obj):

        if not obj.Base or not obj.Base.isDerivedFrom("Path::Feature") or not obj.Base.Path:
            Path.Log.error(translate("PathArray", "Base is empty or an invalid object."))
            return None

        # Do not generate paths and clear current Path data if operation not active
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
        base,
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
        self.base = base
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

        if self.base is None:
            Path.Log.error(translate("PathArray", "No base objects for PathArray."))
            return None

        base = self.base

        # build copies
        # initially output contains original base path, copies are added on top of that
        output = PathUtils.getPathWithPlacement(base).toGCode()

        random.seed(self.seed)

        if self.arrayType == "Linear1D":
            for i in range(self.copies):
                pos = FreeCAD.Vector(
                    self.offsetVector.x * (i + 1),
                    self.offsetVector.y * (i + 1),
                    self.offsetVector.z * (i + 1),
                )
                pos = self._calculateJitter(pos)

                pl = FreeCAD.Placement()
                pl.move(pos)
                np = Path.Path(
                    [cm.transform(pl) for cm in PathUtils.getPathWithPlacement(base).Commands]
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

                        pl = FreeCAD.Placement()
                        # do not process the index 0,0. It will be processed by the base Paths themselves
                        if not (i == 0 and j == 0):
                            pl.move(pos)
                            np = Path.Path(
                                [
                                    cm.transform(pl)
                                    for cm in PathUtils.getPathWithPlacement(base).Commands
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

                        pl = FreeCAD.Placement()
                        # do not process the index 0,0. It will be processed by the base Paths themselves
                        if not (i == 0 and j == 0):
                            pl.move(pos)
                            np = Path.Path(
                                [
                                    cm.transform(pl)
                                    for cm in PathUtils.getPathWithPlacement(base).Commands
                                ]
                            )
                            output += np.toGCode()
            # Eif
        else:
            for i in range(self.copies):
                ang = 360
                if self.copies > 0:
                    ang = self.angle / self.copies * (1 + i)

                pl = FreeCAD.Placement()
                pl.rotate(self.centre, FreeCAD.Vector(0, 0, 1), ang)
                np = PathUtils.applyPlacementToPath(pl, PathUtils.getPathWithPlacement(base))
                output += np.toGCode()

        # return output
        return Path.Path(output)


def Create(base, name="DressupArray"):
    """Create(base, name='DressupPathBoundary') ... creates a dressup array."""

    if not base.isDerivedFrom("Path::Feature"):
        Path.Log.error(translate("CAM_DressupArray", "The selected object is not a path") + "\n")
        return None

    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    job = PathUtils.findParentJob(base)
    obj.Proxy = DressupArray(obj, base, job)
    job.Proxy.addOperation(obj, base, True)
    return obj
