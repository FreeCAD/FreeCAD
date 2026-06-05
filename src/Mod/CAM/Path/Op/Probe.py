# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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
import Path
import Path.Op.Base as PathOp
import PathScripts.PathUtils as PathUtils
from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "CAM Probing Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "CAM Probing operation."

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectProbing(PathOp.ObjectOp):
    """Proxy object for Probing operation."""

    def opFeatures(self, obj):
        """opFeatures(obj) ... Probing works on the stock object."""
        return PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureTool

    def initOperation(self, obj):
        obj.addProperty(
            "App::PropertyLength",
            "Xoffset",
            "Probe",
            QT_TRANSLATE_NOOP("App::Property", "X offset between tool and probe"),
        )
        obj.addProperty(
            "App::PropertyLength",
            "Yoffset",
            "Probe",
            QT_TRANSLATE_NOOP("App::Property", "Y offset between tool and probe"),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "PointCountX",
            "Probe",
            QT_TRANSLATE_NOOP("App::Property", "Number of points to probe in X-direction"),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "PointCountY",
            "Probe",
            QT_TRANSLATE_NOOP("App::Property", "Number of points to probe in Y-direction"),
        )
        obj.addProperty(
            "App::PropertyFile",
            "OutputFileName",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The output location for the probe data to be written"
            ),
        )
        obj.addProperty(
            "App::PropertyLink",
            "BaseShape",
            "Probe",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Limit probe area by shape. Point should be inside shape at final depth",
            ),
        )

    def opOnDocumentRestored(self, obj):
        if not hasattr(obj, "BaseShape"):
            obj.addProperty(
                "App::PropertyLink",
                "BaseShape",
                "Probe",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Limit probe area by shape. Point should be inside shape at final depth",
                ),
            )

    def nextpoint(self, startpoint=0.0, endpoint=0.0, count=3):
        curstep = 0
        dist = (endpoint - startpoint) / (count - 1)
        while curstep <= count - 1:
            yield startpoint + (curstep * dist)
            curstep += 1

    def isValidPoint(self, base, x, y, z):
        if not base:
            return True
        point = FreeCAD.Vector(x, y, z)
        if base.isDerivedFrom("Part::Feature"):
            return base.Shape.isInside(point, 0.1, True)

        return False

    def opExecute(self, obj):
        """opExecute(obj) ... generate probe locations."""
        Path.Log.track()
        if not self.isToolSupported(obj, self.tool):
            Path.Log.warning("No suitable probe tool found")
            return

        # annotation so the PP has canonical signal for open/close, and gets the filename
        self.commandlist.append(
            Path.Command(
                f"(Begin Probing {obj.OutputFileName})", {}, {"probe_open": obj.OutputFileName}
            )
        )

        stock = PathUtils.findParentJob(obj).Stock
        if obj.BaseShape and obj.BaseShape.isDerivedFrom("Part::Feature"):
            bb = obj.BaseShape.Shape.BoundBox
        else:
            bb = stock.Shape.BoundBox

        self.commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        for y in self.nextpoint(bb.YMin, bb.YMax, obj.PointCountY):
            for x in self.nextpoint(bb.XMin, bb.XMax, obj.PointCountX):
                if not self.isValidPoint(obj.BaseShape, x, y, obj.FinalDepth.Value):
                    continue
                self.commandlist.append(
                    Path.Command(
                        "G0",
                        {
                            "X": x + obj.Xoffset.Value,
                            "Y": y + obj.Yoffset.Value,
                            "Z": obj.SafeHeight.Value,
                        },
                    )
                )
                self.commandlist.append(
                    Path.Command(
                        "G38.2",
                        {
                            "Z": obj.FinalDepth.Value,
                            "F": obj.ToolController.VertFeed.Value,
                        },
                    )
                )
                self.commandlist.append(Path.Command("G0", {"Z": obj.SafeHeight.Value}))

        # annotation so the PP has canonical signal for open/close
        self.commandlist.append(Path.Command("(PROBECLOSE)", {}, {"probe_close": ""}))

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj, job) ... set default value for RetractHeight"""

    def isToolSupported(self, obj, tool):
        """Probe operation requires a probe tool"""
        support = PathUtils.getToolShapeName(tool) == "probe"
        Path.Log.track(tool.Label, support)
        return support


def SetupProperties():
    setup = ["Xoffset", "Yoffset", "PointCountX", "PointCountY", "OutputFileName"]
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Probing operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectProbing(obj, name, parentJob)
    return obj
