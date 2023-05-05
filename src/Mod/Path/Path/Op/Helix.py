# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 Lorenz Hüdepohl <dev@stellardeath.org>             *
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

import Path.Base.Generator.helix as helix
from PathScripts.PathUtils import fmt
from PathScripts.PathUtils import sort_locations
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Part
import Path
import Path.Base.FeedRate as PathFeedRate
import Path.Op.Base as PathOp
import Path.Op.CircularHoleBase as PathCircularHoleBase


__title__ = "Path Helix Drill Operation"
__author__ = "Lorenz Hüdepohl"
__url__ = "https://www.freecad.org"
__doc__ = "Class and implementation of Helix Drill operation"
__contributors__ = "russ4262 (Russell Johnson)"
__created__ = "2016"
__scriptVersion__ = "1b testing"
__lastModified__ = "2019-07-12 09:50 CST"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class ObjectHelix(PathCircularHoleBase.ObjectOp):
    """Proxy class for Helix operations."""

    @classmethod
    def helixOpPropertyEnumerations(self, dataType="data"):
        """helixOpPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        # Enumeration lists for App::PropertyEnumeration properties
        enums = {
            "Direction": [
                (translate("Path_Helix", "CW"), "CW"),
                (translate("Path_Helix", "CCW"), "CCW"),
            ],  # this is the direction that the profile runs
            "StartSide": [
                (translate("PathProfile", "Outside"), "Outside"),
                (translate("PathProfile", "Inside"), "Inside"),
            ],  # side of profile that cutter is on in relation to direction of profile
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def circularHoleFeatures(self, obj):
        """circularHoleFeatures(obj) ... enable features supported by Helix."""
        return (
            PathOp.FeatureStepDown | PathOp.FeatureBaseEdges | PathOp.FeatureBaseFaces
        )

    def initCircularHoleOperation(self, obj):
        """initCircularHoleOperation(obj) ... create helix specific properties."""
        obj.addProperty(
            "App::PropertyEnumeration",
            "Direction",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The direction of the circular cuts, ClockWise (CW), or CounterClockWise (CCW)",
            ),
        )

        obj.addProperty(
            "App::PropertyEnumeration",
            "StartSide",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property", "Start cutting from the inside or outside"
            ),
        )

        obj.addProperty(
            "App::PropertyPercent",
            "StepOver",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property", "Percent of cutter diameter to step over on each pass"
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "StartRadius",
            "Helix Drill",
            QT_TRANSLATE_NOOP("App::Property", "Starting Radius"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "OffsetExtra",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extra value to stay away from final profile- good for roughing toolpath",
            ),
        )

        ENUMS = self.helixOpPropertyEnumerations()
        for n in ENUMS:
            setattr(obj, n[0], n[1])
        obj.StepOver = 50

    def opOnDocumentRestored(self, obj):
        if not hasattr(obj, "StartRadius"):
            obj.addProperty(
                "App::PropertyLength",
                "StartRadius",
                "Helix Drill",
                QT_TRANSLATE_NOOP("App::Property", "Starting Radius"),
            )

        if not hasattr(obj, "OffsetExtra"):
            obj.addProperty(
                "App::PropertyDistance",
                "OffsetExtra",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Extra value to stay away from final profile- good for roughing toolpath",
                ),
            )

    def circularHoleExecute(self, obj, holes):
        """circularHoleExecute(obj, holes) ... generate helix commands for each hole in holes"""
        Path.Log.track()
        self.commandlist.append(Path.Command("(helix cut operation)"))

        self.commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        holes = sort_locations(holes, ["x", "y"])

        tool = obj.ToolController.Tool
        tooldiamter = (
            tool.Diameter.Value if hasattr(tool.Diameter, "Value") else tool.Diameter
        )

        args = {
            "edge": None,
            "hole_radius": None,
            "step_down": obj.StepDown.Value,
            "step_over": obj.StepOver / 100,
            "tool_diameter": tooldiamter,
            "inner_radius": obj.StartRadius.Value + obj.OffsetExtra.Value,
            "direction": obj.Direction,
            "startAt": obj.StartSide,
        }

        for hole in holes:
            args["hole_radius"] = (hole["r"] / 2) - (obj.OffsetExtra.Value)
            startPoint = FreeCAD.Vector(hole["x"], hole["y"], obj.StartDepth.Value)
            endPoint = FreeCAD.Vector(hole["x"], hole["y"], obj.FinalDepth.Value)
            args["edge"] = Part.makeLine(startPoint, endPoint)

            # move to starting position
            self.commandlist.append(
                Path.Command("G0", {"Z": obj.ClearanceHeight.Value})
            )
            self.commandlist.append(
                Path.Command(
                    "G0",
                    {
                        "X": startPoint.x,
                        "Y": startPoint.y,
                        "Z": obj.ClearanceHeight.Value,
                    },
                )
            )
            self.commandlist.append(
                Path.Command(
                    "G0", {"X": startPoint.x, "Y": startPoint.y, "Z": startPoint.z}
                )
            )

            results = helix.generate(**args)

            for command in results:
                self.commandlist.append(command)

        PathFeedRate.setFeedRate(self.commandlist, obj.ToolController)


def SetupProperties():
    setup = []
    setup.append("Direction")
    setup.append("StartSide")
    setup.append("StepOver")
    setup.append("StartRadius")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Helix operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectHelix(obj, name, parentJob)
    if obj.Proxy:
        obj.Proxy.findAllHoles(obj)
    return obj
