# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2014 Yorik van Havre <yorik@uncreated.net>
# SPDX-FileCopyrightText: 2020 Schildkroet
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import Part
import Path
import Path.Base.FeedRate as PathFeedRate
from Path.Base.Generator import drill
from Path.Base.Generator import tapping
from Path.Base.Generator import linking
import Path.Base.MachineState as PathMachineState
import Path.Op.Base as PathOp
import Path.Op.CircularHoleBase as PathCircularHoleBase
from Path.Op.Util import drillTipLength
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
                (translate("CAM_Drilling", "Tool Tip"), "Tool Tip"),
                (translate("CAM_Drilling", "2x Tool Tip"), "2x Tool Tip"),
            ],  # extra depth to clear the drill/tap tip's cone. Was "Drill Tip"/"2x
            # Drill Tip" -- renamed since this applies to Tapping too, not just
            # Drilling. See opOnDocumentRestored for the value migration.
        }

        if dataType == "raw":
            return enums

        data = []
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def circularHoleFeatures(self, obj):
        """circularHoleFeatures(obj) ... drilling works on anything, turn on all Base geometries and Locations."""
        return PathOp.FeatureBaseGeometry | PathOp.FeatureLocations | PathOp.FeatureCoolant

    def opOnDocumentRestored(self, obj):
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

        # Migration: ExtraOffset enum values renamed from "Drill Tip"/"2x Drill Tip"
        # to "Tool Tip"/"2x Tool Tip" -- applies to Tapping too, not just Drilling.
        # Capture the stored value before refreshing the valid-options list, then
        # remap it explicitly, so an old doc's selection survives the list change.
        if hasattr(obj, "ExtraOffset"):
            old_offset = str(obj.ExtraOffset)
            for n in self.propertyEnumerations():
                if n[0] == "ExtraOffset":
                    setattr(obj, n[0], n[1])
            if old_offset == "Drill Tip":
                obj.ExtraOffset = "Tool Tip"
            elif old_offset == "2x Drill Tip":
                obj.ExtraOffset = "2x Tool Tip"
            else:
                obj.ExtraOffset = old_offset

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

        # Migration: the peck-retract property (G83/G73/G85 "R" value) was briefly
        # restored under its pre-regression name "RetractHeight" before being renamed
        # to "PeckRetract" -- "RetractHeight" is too easily confused with SafeHeight,
        # which is also loosely "the retract height" but a different, general plane.
        # Handles both: (a) old docs that already have RetractHeight (rename it), and
        # (b) docs saved while the property was missing entirely (add it fresh).
        if hasattr(obj, "RetractHeight"):
            obj.renameProperty("RetractHeight", "PeckRetract")
        elif not hasattr(obj, "PeckRetract"):
            obj.addProperty(
                "App::PropertyDistance",
                "PeckRetract",
                "Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The height (R) the tool retracts to between pecks and when "
                    "the canned drilling cycle finishes",
                ),
            )
            obj.PeckRetract = obj.SafeHeight.Value

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
                    "Apply G99 retraction: between holes, retract only to PeckRetract (or "
                    "SafeHeight if not pecking) instead of the full retract G98 does",
                ),
            )

        self.updateStrategyVisibility(obj)

    def opOnChanged(self, obj, prop):
        """opOnChanged(obj, prop) ... react to Strategy changes to update property visibility."""
        if prop == "Strategy":
            self.updateStrategyVisibility(obj)

    def updateStrategyVisibility(self, obj):
        """Hide the peck-only properties in the Property Editor while Strategy != Drilling.
        Tapping never pecks in this codebase, so PeckEnabled/PeckDepth/ChipBreakEnabled/
        FeedRetractEnabled/PeckRetract are stored but inert -- matches how the task panel
        already hides the same fields (Gui/Drilling.py's updateStrategyVisibility), and
        how CircularHoleBase does the same for SortingMode's StartPoint/EndPoint/UseEndPoint.
        """
        if not hasattr(obj, "Strategy"):
            return
        mode = 0 if obj.Strategy == "Drilling" else 2  # 0=visible, 2=hidden
        for prop in (
            "PeckEnabled",
            "PeckDepth",
            "ChipBreakEnabled",
            "FeedRetractEnabled",
            "PeckRetract",
        ):
            if hasattr(obj, prop):
                obj.setEditorMode(prop, mode)

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
            "App::PropertyDistance",
            "PeckRetract",
            "Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The height (R) the tool retracts to between pecks and when "
                "the canned drilling cycle finishes",
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "ExtraOffset",
            "Drill",
            QT_TRANSLATE_NOOP("App::Property", "How far the cutting depth is extended"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "KeepToolDown",
            "Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Apply G99 retraction: between holes, retract only to PeckRetract (or "
                "SafeHeight if not pecking) instead of the full retract G98 does",
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
        if obj.ExtraOffset == "Tool Tip":
            endoffset = drillTipLength(self.tool)
        elif obj.ExtraOffset == "2x Tool Tip":
            endoffset = drillTipLength(self.tool) * 2

        # compute the drilling targets
        edgelist = []
        for hole in holes:
            v1 = FreeCAD.Vector(hole["x"], hole["y"], obj.StartDepth.Value)
            v2 = FreeCAD.Vector(hole["x"], hole["y"], obj.FinalDepth.Value - endoffset)
            edgelist.append(Part.makeLine(v1, v2))

        # Prepare linking parameters
        # Use self.model which is transformed when 3+2 workplane is active
        solids = [base.Shape for base in self.model]
        linkingArgs = {
            "start_position": None,
            "target_position": None,
            "heights_clearance": (safe_height, obj.ClearanceHeight.Value),
            "solids": None,
            "tool_shape": None,
            "tool_diameter": None,
            "collision_clearance": obj.CollisionClearance.Value,
        }
        if obj.CollisionAvoidanceStrategy == "Clearance Height":
            linkingArgs["heights_clearance"] = obj.ClearanceHeight.Value
        elif obj.CollisionAvoidanceStrategy == "Retract Height":
            pass
        elif obj.CollisionAvoidanceStrategy == "Line of Sight":
            linkingArgs["solids"] = solids
        elif obj.CollisionAvoidanceStrategy == "Tool Diameter":
            linkingArgs["solids"] = solids
            linkingArgs["tool_diameter"] = obj.ToolController.Tool.Diameter.Value
        elif obj.CollisionAvoidanceStrategy == "Tool Shape":
            linkingArgs["solids"] = solids
            linkingArgs["tool_shape"] = obj.ToolController.Tool.BitBody.Shape

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
                # For G99 mode, tool is at PeckRetract (R-plane) after previous hole,
                # which may be below SafeHeight (e.g. a low peck retract inside a deep
                # hole). The collision check below assumes travel happens at SafeHeight,
                # so if the tool isn't there yet, climb to it first -- otherwise a "no
                # collision" result here would be silently wrong: with no commands
                # emitted, the modal G99 cycle would actually traverse to the next hole
                # at the low R height instead, potentially dragging the tool through
                # stock between holes.
                current_pos = machinestate.getPosition()
                if current_pos.z < safe_height and not Path.Geom.isRoughly(
                    current_pos.z, safe_height
                ):
                    command = Path.Command("G0", {"Z": safe_height})
                    self.commandlist.append(command)
                    machinestate.addCommand(command)
                    current_pos = machinestate.getPosition()

                # Check if direct move at retract plane would collide with model
                target_at_safe_height = FreeCAD.Vector(startPoint.x, startPoint.y, safe_height)
                linkingArgs["start_position"] = current_pos
                linkingArgs["target_position"] = target_at_safe_height
                linking_moves = linking.get_linking_moves(**linkingArgs)

                # linking_moves should be skipped, if first move not vertical
                if not Path.Geom.isRoughly(linking_moves[0].z, startPoint.z):
                    # Cannot traverse at retract plane - need to break cycle group
                    # Retract to safe height, traverse, then plunge to safe height for new cycle
                    self.commandlist.extend(linking_moves)
                    machinestate.addCommands(linking_moves)
                # else: no collision - G99 cycle continues, tool stays at retract plane

            # Perform drilling
            dwelltime = obj.DwellTime if obj.DwellEnabled else 0.0
            peckdepth = obj.PeckDepth.Value if obj.PeckEnabled else 0.0
            repeat = 1  # technical debt:  Add a repeat property for user control
            chipBreak = obj.ChipBreakEnabled and obj.PeckEnabled

            # PeckRetract (R) only applies to peck cycles; a non-peck cycle just
            # retracts to SafeHeight like it did before PeckRetract existed.
            peck_retract = obj.PeckRetract.Value if obj.PeckEnabled else safe_height

            # Save Z position before canned cycle for G98 retract
            z_before_cycle = machinestate.Z

            try:
                drillcommands = drill.generate(
                    edge,
                    dwelltime,
                    peckdepth,
                    repeat,
                    peck_retract,
                    chipBreak=chipBreak,
                    feedRetract=obj.FeedRetractEnabled,
                )

            except ValueError as e:  # any targets that fail the generator are ignored
                Path.Log.info(e)
                continue

            # Set RetractMode annotation for each command
            for command in drillcommands:
                command.addAnnotations({"RetractMode": mode, "operation": "drilling"})
                self.commandlist.append(command)
                machinestate.addCommand(command)

            # Update Z position based on RetractMode
            # G98: retract to initial Z (Z before cycle started)
            # G99: retract to R parameter (peck_retract)
            if mode == "G98":
                machinestate.Z = z_before_cycle
            else:  # G99
                machinestate.Z = peck_retract

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
        if obj.ExtraOffset == "Tool Tip":
            endoffset = drillTipLength(self.tool)
        elif obj.ExtraOffset == "2x Tool Tip":
            endoffset = drillTipLength(self.tool) * 2

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
                command.addAnnotations({"RetractMode": mode, "operation": "tapping"})
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

        # Default retract (R) to the op's SafeHeight. These are not the same thing:
        # SafeHeight is the general rapid-clearance plane, while PeckRetract is the
        # canned-cycle R value and may need to be set lower (e.g. inside a deep hole)
        # to avoid repeated full retracts with a long, thin drill. Bound as an
        # expression (like the other Op* defaults below) so it tracks SafeHeight
        # until the user overrides it with an explicit value.
        if not self.applyExpression(obj, "PeckRetract", "SafeHeight"):
            obj.PeckRetract = obj.SafeHeight.Value

        if hasattr(job.SetupSheet, "PeckDepth"):
            obj.PeckDepth = job.SetupSheet.PeckDepth
        elif self.applyExpression(obj, "PeckDepth", "OpToolDiameter*0.75"):
            obj.PeckDepth = 1

        if hasattr(job.SetupSheet, "DwellTime"):
            obj.DwellTime = job.SetupSheet.DwellTime
        else:
            obj.DwellTime = 1


def SetupProperties():
    setup = PathOp.SetupPropertiesLinking()
    setup.append("Strategy")
    setup.append("PeckDepth")
    setup.append("PeckEnabled")
    setup.append("DwellTime")
    setup.append("DwellEnabled")
    setup.append("AddTipLength")
    setup.append("PeckRetract")
    setup.append("ExtraOffset")
    setup.append("KeepToolDown")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Drilling operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectDrilling(obj, name, parentJob)
    return obj
