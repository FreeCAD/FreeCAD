# -*- coding: utf-8 -*-
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
            QT_TRANSLATE_NOOP(
                "App::Property", "Number of points to probe in X direction"
            ),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "PointCountY",
            "Probe",
            QT_TRANSLATE_NOOP(
                "App::Property", "Number of points to probe in Y direction"
            ),
        )
        obj.addProperty(
            "App::PropertyFile",
            "OutputFileName",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The output location for the probe data to be written"
            ),
        )

    def nextpoint(self, startpoint=0.0, endpoint=0.0, count=3):
        curstep = 0
        dist = (endpoint - startpoint) / (count - 1)
        while curstep <= count - 1:
            yield startpoint + (curstep * dist)
            curstep += 1

    def opExecute(self, obj):
        """opExecute(obj) ... generate probe locations."""
        Path.Log.track()
        self.commandlist.append(Path.Command("(Begin Probing)"))

        stock = PathUtils.findParentJob(obj).Stock
        bb = stock.Shape.BoundBox

        openstring = "(PROBEOPEN {})".format(obj.OutputFileName)
        self.commandlist.append(Path.Command(openstring))
        self.commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        for y in self.nextpoint(bb.YMin, bb.YMax, obj.PointCountY):
            for x in self.nextpoint(bb.XMin, bb.XMax, obj.PointCountX):
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

        self.commandlist.append(Path.Command("(PROBECLOSE)"))

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj, job) ... set default value for RetractHeight"""


def SetupProperties():
    setup = ["Xoffset", "Yoffset", "PointCountX", "PointCountY", "OutputFileName"]
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Probing operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectProbing(obj, name, parentJob)
    return obj
