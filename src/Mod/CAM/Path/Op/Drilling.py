# SPDX-License-Identifier: LGPL-2.1-or-later

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
import Path.Base.Generator.tapping as tapping
import Path.Base.Generator.linking as linking
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
            "Strategy": [
                (translate("CAM_Drilling", "Drilling"), "Drilling"),
                (translate("CAM_Drilling", "Tapping"), "Tapping"),
            ],  # hole-making strategy
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
        return PathOp.FeatureBaseGeometry | PathOp.FeatureLocations | PathOp.FeatureCoolant

    def onDocumentRestored(self, obj):
        # Add Strategy property if missing (old drilling operations)
        if not hasattr(obj, "Strategy"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "Strategy",
                "Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Hole-making strategy (Drilling, Tapping, etc.)",
                ),
            )
            # Set enumerations
            for n in self.propertyEnumerations():
                if n[0] == "Strategy":
                    setattr(obj, n[0], n[1])
            # Default to Drilling for old operations
            obj.Strategy = "Drilling"

        if hasattr(obj, "chipBreakEnabled"):
            obj.renameProperty("chipBreakEnabled", "ChipBreakEnabled")
        elif not hasattr(obj, "ChipBreakEnabled"):
            obj.addProperty(
                "App::PropertyBool",
                "ChipBreakEnabled",
                "Drill",
                QT_TRANSLATE_NOOP("App::Property", "Use chipbreaking"),
            )

        if hasattr(obj, "feedRetractEnabled"):
            obj.renameProperty("feedRetractEnabled", "FeedRetractEnabled")
        elif not hasattr(obj, "FeedRetractEnabled"):
            obj.addProperty(
                "App::PropertyBool",
                "FeedRetractEnabled",
                "Drill",
                QT_TRANSLATE_NOOP("App::Property", "Use G85 boring cycle with feed out"),
            )

        if hasattr(obj, "RetractMode"):
            obj.removeProperty("RetractMode")

        # Migration: Remove RetractHeight property and adjust StartDepth if needed
        # This handles old Tapping operations that used RetractHeight
        if hasattr(obj, "RetractHeight"):
            # If RetractHeight was higher than StartDepth, migrate to StartDepth
            if obj.RetractHeight.Value > obj.StartDepth.Value:
                Path.Log.warning(
                    f"Migrating RetractHeight ({obj.RetractHeight.Value}) to StartDepth. "
                    f"Old StartDepth was {obj.StartDepth.Value}"
                )
                obj.StartDepth = obj.RetractHeight.Value
            obj.removeProperty("RetractHeight")

        # Migration: Old Tapping ReturnLevel to KeepToolDown
        # This handles old Tapping operations that used ReturnLevel enum
        if hasattr(obj, "ReturnLevel"):
            if obj.ReturnLevel == "G99":
                obj.KeepToolDown = True
            else:
                obj.KeepToolDown = False
            obj.removeProperty("ReturnLevel")

        if not hasattr(obj, "KeepToolDown"):
            obj.addProperty(
                "App::PropertyBool",
                "KeepToolDown",
                "Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Apply G99 retraction: only retract to StartDepth between holes in this operation",
                ),
            )

    def initCircularHoleOperation(self, obj):
        """initCircularHoleOperation(obj) ... add drilling specific properties to obj."""
        obj.addProperty(
            "App::PropertyEnumeration",
            "Strategy",
            "Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Hole-making strategy (Drilling, Tapping, etc.)",
            ),
        )
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
            "ChipBreakEnabled",
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
            "ExtraOffset",
            "Drill",
            QT_TRANSLATE_NOOP("App::Property", "How far the drilling depth is extended"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "KeepToolDown",
            "Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Apply G99 retraction: only retract to StartDepth between holes in this operation",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "FeedRetractEnabled",
            "Drill",
            QT_TRANSLATE_NOOP("App::Property", "Use G85 boring cycle with feed out"),
        )

        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

    def circularHoleExecute(self, obj, holes):
        """circularHoleExecute(obj, holes) ... generate operation for each hole based on strategy."""
        Path.Log.track()

        strategy = obj.Strategy if hasattr(obj, "Strategy") else "Drilling"

        if strategy == "Drilling":
            self._executeDrilling(obj, holes)
        elif strategy == "Tapping":
            self._executeTapping(obj, holes)
        else:
            Path.Log.error(f"Unknown strategy: {strategy}")

    def _executeDrilling(self, obj, holes):
        """_executeDrilling(obj, holes) ... generate drilling operation for each hole in holes."""
        Path.Log.track()
        machinestate = PathMachineState.MachineState()
        # We should be at clearance height.

        mode = "G99" if obj.KeepToolDown else "G98"

        # Validate that SafeHeight doesn't exceed ClearanceHeight
        safe_height = obj.SafeHeight.Value
        if safe_height > obj.ClearanceHeight.Value:
            Path.Log.warning(
                f"SafeHeight ({safe_height}) is above ClearanceHeight ({obj.ClearanceHeight.Value}). "
                f"Using ClearanceHeight instead."
            )
            safe_height = obj.ClearanceHeight.Value

        # Calculate offsets to add to target edge
        endoffset = 0.0
        if obj.ExtraOffset == "Drill Tip":
            endoffset = PathUtils.drillTipLength(self.tool)
        elif obj.ExtraOffset == "2x Drill Tip":
            endoffset = PathUtils.drillTipLength(self.tool) * 2

        # compute the drilling targets
        edgelist = []
        for hole in holes:
            v1 = FreeCAD.Vector(hole["x"], hole["y"], obj.StartDepth.Value)
            v2 = FreeCAD.Vector(hole["x"], hole["y"], obj.FinalDepth.Value - endoffset)
            edgelist.append(Part.makeLine(v1, v2))

        # build list of solids for collision detection.
        # Include base objects from job
        solids = []
        for base in self.job.Model.Group:
            solids.append(base.Shape)

        # http://linuxcnc.org/docs/html/gcode/g-code.html#gcode:g98-g99

        # This section is technical debt. The computation of the
        # target shapes should be factored out for reuse.
        # This will likely mean refactoring upstream CircularHoleBase to pass
        # spotshapes instead of holes.

        # Start computing the Path
        self.commandlist.append(Path.Command("(Begin Drilling)"))

        # Make sure tool is at a clearance height
        command = Path.Command("G0", {"Z": obj.ClearanceHeight.Value})
        machinestate.addCommand(command)

        # machine.addCommand(command)
        self.commandlist.append(command)

        # iterate the edgelist and generate gcode
        firstMove = True
        for edge in edgelist:
            Path.Log.debug(edge)

            # Get the target start point
            startPoint = edge.Vertexes[0].Point

            # Get linking moves from current to start of target
            if firstMove:  # Build manually
                command = Path.Command("G0", {"X": startPoint.x, "Y": startPoint.y})
                self.commandlist.append(command)
                machinestate.addCommand(command)
                command = Path.Command("G0", {"Z": safe_height})
                self.commandlist.append(command)
                machinestate.addCommand(command)
                firstMove = False

            else:  # Check if we need linking moves
                # For G99 mode, tool is at StartDepth (R-plane) after previous hole
                # Check if direct move at retract plane would collide with model
                current_pos = machinestate.getPosition()
                target_at_retract_plane = FreeCAD.Vector(startPoint.x, startPoint.y, current_pos.z)

                # Check collision at the retract plane (current Z height)
                collision_detected = linking.check_collision(
                    start_position=current_pos,
                    target_position=target_at_retract_plane,
                    solids=solids,
                )

                if collision_detected:
                    # Cannot traverse at retract plane - need to break cycle group
                    # Retract to safe height, traverse, then plunge to safe height for new cycle
                    target_at_safe_height = FreeCAD.Vector(startPoint.x, startPoint.y, safe_height)
                    linking_moves = linking.get_linking_moves(
                        start_position=current_pos,
                        target_position=target_at_safe_height,
                        local_clearance=safe_height,
                        global_clearance=obj.ClearanceHeight.Value,
                        tool_shape=self.tool.Shape,
                        solids=solids,
                    )
                    self.commandlist.extend(linking_moves)
                    for move in linking_moves:
                        machinestate.addCommand(move)
                # else: no collision - G99 cycle continues, tool stays at retract plane

            # Perform drilling
            dwelltime = obj.DwellTime if obj.DwellEnabled else 0.0
            peckdepth = obj.PeckDepth.Value if obj.PeckEnabled else 0.0
            repeat = 1  # technical debt:  Add a repeat property for user control
            chipBreak = obj.ChipBreakEnabled and obj.PeckEnabled

            # Save Z position before canned cycle for G98 retract
            z_before_cycle = machinestate.Z

            try:
                drillcommands = drill.generate(
                    edge,
                    dwelltime,
                    peckdepth,
                    repeat,
                    obj.StartDepth.Value,
                    chipBreak=chipBreak,
                    feedRetract=obj.FeedRetractEnabled,
                )

            except ValueError as e:  # any targets that fail the generator are ignored
                Path.Log.info(e)
                continue

            # Set RetractMode annotation for each command
            for command in drillcommands:
                annotations = command.Annotations
                annotations["RetractMode"] = mode
                annotations["operation"] = "drilling"
                command.Annotations = annotations
                self.commandlist.append(command)
                machinestate.addCommand(command)

            # Update Z position based on RetractMode
            # G98: retract to initial Z (Z before cycle started)
            # G99: retract to R parameter (StartDepth)
            if mode == "G98":
                machinestate.Z = z_before_cycle
            else:  # G99
                machinestate.Z = obj.StartDepth.Value

        # Apply feedrates to commands
        PathFeedRate.setFeedRate(self.commandlist, obj.ToolController)

    def _executeTapping(self, obj, holes):
        """_executeTapping(obj, holes) ... generate tapping operation for each hole in holes."""
        Path.Log.track()
        machinestate = PathMachineState.MachineState()

        if not hasattr(obj.ToolController.Tool, "Pitch"):
            Path.Log.error(
                translate(
                    "CAM_Drilling",
                    "Tapping strategy requires a Tap tool with Pitch",
                )
            )
            return

        self.commandlist.append(Path.Command("(Begin Tapping)"))

        # Determine retract mode
        mode = "G99" if obj.KeepToolDown else "G98"

        # Validate that SafeHeight doesn't exceed ClearanceHeight
        safe_height = obj.SafeHeight.Value
        if safe_height > obj.ClearanceHeight.Value:
            Path.Log.warning(
                f"SafeHeight ({safe_height}) is above ClearanceHeight ({obj.ClearanceHeight.Value}). "
                f"Using ClearanceHeight instead."
            )
            safe_height = obj.ClearanceHeight.Value

        # Calculate offsets to add to target edge
        endoffset = 0.0
        if obj.ExtraOffset == "Drill Tip":
            endoffset = PathUtils.drillTipLength(self.tool)
        elif obj.ExtraOffset == "2x Drill Tip":
            endoffset = PathUtils.drillTipLength(self.tool) * 2

        # compute the tapping targets
        edgelist = []
        for hole in holes:
            v1 = FreeCAD.Vector(hole["x"], hole["y"], obj.StartDepth.Value)
            v2 = FreeCAD.Vector(hole["x"], hole["y"], obj.FinalDepth.Value - endoffset)
            edgelist.append(Part.makeLine(v1, v2))

        # Start computing the Path
        # Make sure tool is at clearance height
        command = Path.Command("G0", {"Z": obj.ClearanceHeight.Value})
        machinestate.addCommand(command)
        self.commandlist.append(command)

        # iterate the edgelist and generate gcode
        firstMove = True
        for edge in edgelist:
            Path.Log.debug(edge)

            # Get the target start point
            startPoint = edge.Vertexes[0].Point

            # Get linking moves from current to start of target
            if firstMove:  # Build manually
                command = Path.Command("G0", {"X": startPoint.x, "Y": startPoint.y})
                self.commandlist.append(command)
                machinestate.addCommand(command)
                command = Path.Command("G0", {"Z": safe_height})
                self.commandlist.append(command)
                machinestate.addCommand(command)
                firstMove = False
            # For subsequent holes, the canned cycle handles positioning

            # Perform tapping
            dwelltime = obj.DwellTime if obj.DwellEnabled else 0.0
            repeat = 1  # technical debt:  Add a repeat property for user control

            # Get attribute from obj.tool, assign default and set to bool for passing to generate
            isRightHand = (
                getattr(obj.ToolController.Tool, "SpindleDirection", "Forward") == "Forward"
            )

            # Get pitch in mm as a float (no unit string)
            pitch = getattr(obj.ToolController.Tool, "Pitch", None)
            if pitch is None or pitch == 0:
                Path.Log.error(
                    translate(
                        "CAM_Drilling",
                        "Tapping strategy requires a Tap tool with non-zero Pitch",
                    )
                )
                continue

            spindle_speed = getattr(obj.ToolController, "SpindleSpeed", None)
            if spindle_speed is None or spindle_speed == 0:
                Path.Log.error(
                    translate(
                        "CAM_Drilling",
                        "Tapping strategy requires a ToolController with non-zero SpindleSpeed",
                    )
                )
                continue

            # Save Z position before canned cycle for G98 retract
            z_before_cycle = machinestate.Z

            try:
                tappingcommands = tapping.generate(
                    edge,
                    dwelltime,
                    repeat,
                    obj.StartDepth.Value,
                    isRightHand,
                    pitch,
                    spindle_speed,
                )

            except ValueError as e:  # any targets that fail the generator are ignored
                Path.Log.info(e)
                continue

            # Set RetractMode annotation for each command
            for command in tappingcommands:
                annotations = command.Annotations
                annotations["RetractMode"] = mode
                annotations["operation"] = "tapping"
                command.Annotations = annotations
                self.commandlist.append(command)
                machinestate.addCommand(command)

            # Update Z position based on RetractMode
            # G98: retract to initial Z (Z before cycle started)
            # G99: retract to R parameter (StartDepth)
            if mode == "G98":
                machinestate.Z = z_before_cycle
            else:  # G99
                machinestate.Z = obj.StartDepth.Value

        # Apply feed rates to commands
        PathFeedRate.setFeedRate(self.commandlist, obj.ToolController)

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj, job) ... set default values for drilling operation"""
        obj.Strategy = "Drilling"
        obj.ExtraOffset = "None"
        obj.KeepToolDown = False  # default to safest option: G98

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
    setup.append("Strategy")
    setup.append("PeckDepth")
    setup.append("PeckEnabled")
    setup.append("DwellTime")
    setup.append("DwellEnabled")
    setup.append("AddTipLength")
    setup.append("ExtraOffset")
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
