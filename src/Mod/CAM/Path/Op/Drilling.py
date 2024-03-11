# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Schildkroet                                        *
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
import Part
import Path
import Path.Base.FeedRate as PathFeedRate
import Path.Base.Generator.drill as drill
import Path.Base.MachineState as PathMachineState
import Path.Op.Base as PathOp
import Path.Op.CircularHoleBase as PathCircularHoleBase
import PathScripts.PathUtils as PathUtils
from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "CAM Drilling Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "CAM Drilling operation."
__contributors__ = "IMBack!"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class ObjectDrilling(PathCircularHoleBase.ObjectOp):
    """Proxy object for Drilling operation."""

    @classmethod
    def propertyEnumerations(self, dataType="data"):
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
            "RetractMode": [
                (translate("CAM_Drilling", "G98"), "G98"),
                (translate("CAM_Drilling", "G99"), "G99"),
            ],  # How high to retract after a drilling move
            "ExtraOffset": [
                (translate("CAM_Drilling", "None"), "None"),
                (translate("CAM_Drilling", "Drill Tip"), "Drill Tip"),
                (translate("CAM_Drilling", "2x Drill Tip"), "2x Drill Tip"),
            ],  # extra drilling depth to clear drill taper
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
        """circularHoleFeatures(obj) ... drilling works on anything, turn on all Base geometries and Locations."""
        return (
            PathOp.FeatureBaseGeometry | PathOp.FeatureLocations | PathOp.FeatureCoolant
        )

    def onDocumentRestored(self, obj):
        if not hasattr(obj, "chipBreakEnabled"):
            obj.addProperty(
                "App::PropertyBool",
                "chipBreakEnabled",
                "Drill",
                QT_TRANSLATE_NOOP("App::Property", "Use chipbreaking"),
            )

    def initCircularHoleOperation(self, obj):
        """initCircularHoleOperation(obj) ... add drilling specific properties to obj."""
        obj.addProperty(
            "App::PropertyLength",
            "PeckDepth",
            "Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Incremental Drill depth before retracting to clear chips",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "PeckEnabled",
            "Drill",
            QT_TRANSLATE_NOOP("App::Property", "Enable pecking"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "chipBreakEnabled",
            "Drill",
            QT_TRANSLATE_NOOP("App::Property", "Use chipbreaking"),
        )
        obj.addProperty(
            "App::PropertyFloat",
            "DwellTime",
            "Drill",
            QT_TRANSLATE_NOOP("App::Property", "The time to dwell between peck cycles"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "DwellEnabled",
            "Drill",
            QT_TRANSLATE_NOOP("App::Property", "Enable dwell"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "AddTipLength",
            "Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Calculate the tip length and subtract from final depth",
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "RetractMode",
            "Drill",
            QT_TRANSLATE_NOOP(
                "App::Property", "Controls tool retract height between holes in same op, Default=G98: safety height"
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "RetractHeight",
            "Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The height where cutting feed rate starts and retract height for peck operation",
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "ExtraOffset",
            "Drill",
            QT_TRANSLATE_NOOP("App::Property", "How far the drilling depth is extended"),
        )
        obj.addProperty(
            "App::PropertyBool", "KeepToolDown", "Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Apply G99 retraction: only retract to RetractHeight between holes in this operation")
        )

        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

    def circularHoleExecute(self, obj, holes):
        """circularHoleExecute(obj, holes) ... generate drill operation for each hole in holes."""
        Path.Log.track()
        machine = PathMachineState.MachineState()

        self.commandlist.append(Path.Command("(Begin Drilling)"))

        # rapid to clearance height
        command = Path.Command("G0", {"Z": obj.ClearanceHeight.Value})
        machine.addCommand(command)
        self.commandlist.append(command)

        self.commandlist.append(Path.Command("G90"))  # Absolute distance mode

        # Calculate offsets to add to target edge
        endoffset = 0.0
        if obj.ExtraOffset == "Drill Tip":
            endoffset = PathUtils.drillTipLength(self.tool)
        elif obj.ExtraOffset == "2x Drill Tip":
            endoffset = PathUtils.drillTipLength(self.tool) * 2

        if not hasattr(obj, "KeepToolDown"):
            obj.addProperty(
                "App::PropertyBool", "KeepToolDown", "Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Apply G99 retraction: only retract to RetractHeight between holes in this operation")
            )

        if not hasattr(obj, "RetractMode"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "RetractMode",
                "Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Controls tool retract height between holes in same op, Default=G98: safety height"
                ),
            )
            # ensure new enums exist in old class
            for n in self.propertyEnumerations():
                setattr(obj, n[0], n[1])

        # http://linuxcnc.org/docs/html/gcode/g-code.html#gcode:g98-g99
        if obj.KeepToolDown: obj.RetractMode = "G99"
        else: obj.RetractMode = "G98"
        self.commandlist.append(Path.Command(obj.RetractMode))

        holes = PathUtils.sort_locations(holes, ["x", "y"])

        # This section is technical debt. The computation of the
        # target shapes should be factored out for re-use.
        # This will likely mean refactoring upstream CircularHoleBase to pass
        # spotshapes instead of holes.

        edgelist = []
        for hole in holes:
            v1 = FreeCAD.Vector(hole["x"], hole["y"], obj.StartDepth.Value)
            v2 = FreeCAD.Vector(hole["x"], hole["y"], obj.FinalDepth.Value - endoffset)
            edgelist.append(Part.makeLine(v1, v2))

        # iterate the edgelist and generate gcode
        for edge in edgelist:

            Path.Log.debug(edge)

            # move to hole location

            startPoint = edge.Vertexes[0].Point

            # G81,83 will do this move anyway but FreeCAD sets feedrate according to _isVertical,
            # so do this explicitly before drilling.
            command = Path.Command("G0", {"X": startPoint.x, "Y": startPoint.y})
            self.commandlist.append(command)
            machine.addCommand(command)

            # Technical Debt:  We are assuming the edges are aligned.
            # This assumption should be corrected and the necessary rotations
            # performed to align the edge with the Z axis for drilling

            # Perform drilling
            dwelltime = obj.DwellTime if obj.DwellEnabled else 0.0
            peckdepth = obj.PeckDepth.Value if obj.PeckEnabled else 0.0
            repeat = 1  # technical debt:  Add a repeat property for user control
            chipBreak = obj.chipBreakEnabled and obj.PeckEnabled

            try:
                drillcommands = drill.generate(
                    edge,
                    dwelltime,
                    peckdepth,
                    repeat,
                    obj.RetractHeight.Value,
                    chipBreak=chipBreak,
                )

            except ValueError as e:  # any targets that fail the generator are ignored
                Path.Log.info(e)
                continue

            for command in drillcommands:
                self.commandlist.append(command)
                machine.addCommand(command)

        # Cancel canned drilling cycle
        self.commandlist.append(Path.Command("G80"))
        command = Path.Command("G0", {"Z": obj.SafeHeight.Value})
        self.commandlist.append(command)
        machine.addCommand(command)

        # Apply feedrates to commands
        PathFeedRate.setFeedRate(self.commandlist, obj.ToolController)

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj, job) ... set default value for RetractHeight"""
        obj.ExtraOffset = "None"
        obj.KeepToolDown = False  # default to safest option: G98

        if hasattr(job.SetupSheet, "RetractHeight"):
            obj.RetractHeight = job.SetupSheet.RetractHeight
        elif self.applyExpression(
            obj, "RetractHeight", "StartDepth+SetupSheet.SafeHeightOffset"
        ):
            if not job:
                obj.RetractHeight = 10
            else:
                obj.RetractHeight.Value = obj.StartDepth.Value + 1.0

        if hasattr(job.SetupSheet, "PeckDepth"):
            obj.PeckDepth = job.SetupSheet.PeckDepth
        elif self.applyExpression(obj, "PeckDepth", "OpToolDiameter*0.75"):
            obj.PeckDepth = 1

        if hasattr(job.SetupSheet, "DwellTime"):
            obj.DwellTime = job.SetupSheet.DwellTime
        else:
            obj.DwellTime = 1


def SetupProperties():
    setup = []
    setup.append("PeckDepth")
    setup.append("PeckEnabled")
    setup.append("DwellTime")
    setup.append("DwellEnabled")
    setup.append("AddTipLength")
    setup.append("RetractMode")
    setup.append("ExtraOffset")
    setup.append("RetractHeight")
    setup.append("KeepToolDown")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Drilling operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)

    obj.Proxy = ObjectDrilling(obj, name, parentJob)
    if obj.Proxy:
        obj.Proxy.findAllHoles(obj)

    return obj
