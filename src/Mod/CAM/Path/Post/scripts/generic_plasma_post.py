# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic
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

"""
Generic Postprocessor for plasma, laser, and waterjet cutters that require a pierce delay
"""

from typing import Any, Dict
import copy

from Path.Post.Processor import PostProcessor

import Constants
import Path
import FreeCAD

translate = FreeCAD.Qt.translate

DEBUG = False
if DEBUG:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

Path.Log.debug("generic_plasma_post.py module loaded")

# Define some types that are used throughout this file.
Values = Dict[str, Any]

POST_TYPE = "machine"


# Round value to within precision (for the purposes of float comparisons)
def CompValue(val):
    # 5 significant digits should be precise enough (for plasma)
    return round(val, 5)


class GenericPlasma(PostProcessor):
    """
    The GenericPlasma post processor class.
    """

    @classmethod
    def get_common_property_schema(cls):
        Path.Log.debug("GenericPlasma.get_common_property_schema() called")
        common_props = copy.deepcopy(super().get_common_property_schema())

        # Override defaults for GenericPlasma
        for prop in common_props:
            if prop["name"] == "file_extension":
                prop["default"] = "nc"
            elif prop["name"] == "supports_tool_radius_compensation":
                prop["default"] = True
            elif prop["name"] == "preamble":
                prop["default"] = "G17 G54 G40 G49 G80 G90"
            elif prop["name"] == "postamble":
                prop["default"] = "M05\nG17 G54 G90 G80 G40\nM2"
            elif prop["name"] == "safetyblock":
                prop["default"] = "G40 G49 G80"

        return common_props

    @classmethod
    def get_property_schema(cls):
        """Return schema for plasma-specific configurable properties."""
        return [
            {
                "name": "pierce_delay",
                "type": "integer",
                "label": translate("CAM", "Pierce Delay"),
                "default": 1000,
                "min": 0,
                "max": 10000,
                "help": translate(
                    "CAM",
                    "Pierce delay in milliseconds to wait after torch ignites (M3) before starting movement",
                ),
            },
            {
                "name": "cooling_delay",
                "type": "integer",
                "label": translate("CAM", "Cooling Delay"),
                "default": 500,
                "min": 0,
                "max": 10000,
                "help": translate(
                    "CAM",
                    "Cooling delay in milliseconds to wait after torch extinguishes (M5) before movement",
                ),
            },
            {
                "name": "marking_delay",
                "type": "integer",
                "label": translate("CAM", "Marking Delay"),
                "default": 100,
                "min": 0,
                "max": 10000,
                "help": translate(
                    "CAM",
                    "Marking delay in milliseconds to wait after torch ignites (M3) when making a mark",
                ),
            },
            {
                "name": "torch_zaxis_control",
                "type": "bool",
                "label": translate("CAM", "Torch Z-Axis Control"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Torch ignites (M3) on Z- movement and extinguishes (M5) on Z+ movement. "
                    "When disabled, any M3/M5 commands are output as-is.",
                ),
            },
            {
                "name": "force_rapid_feeds",
                "type": "bool",
                "label": translate("CAM", "Force Rapid Feeds"),
                "default": False,
                "runtime": True,
                "help": translate(
                    "CAM",
                    "Force rapid-feed speeds for all feed specified commands. "
                    "Useful for dry runs to verify paths without cutting.",
                ),
            },
            {
                "name": "mark_entry_only",
                "type": "bool",
                "label": translate("CAM", "Mark Entry Points Only"),
                "default": False,
                "runtime": True,
                "help": translate(
                    "CAM",
                    "Mark first entry points only (for drilling prep). "
                    "Skips cutting moves and only marks where the torch would pierce.",
                ),
            },
        ]

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Generic Plasma post processor"),
        tooltipargs=[],
        units="Metric",
    ) -> None:
        super().__init__(
            job=job,
            tooltip=tooltip,
            tooltipargs=tooltipargs,
            units=units,
        )
        Path.Log.debug("Generic Plasma post processor initialized.")

        # Torch commands
        self.TorchIgniteCommand = Path.Command("M3")
        self.TorchExtinguishCommand = Path.Command("M5")

        # State tracking for plasma-specific features
        self._torch_active = False
        self._last_z = None  # Track last Z position for direction detection

    def _reset_plasma_state(self, item):
        """Reset plasma-specific state tracking for each operation."""
        reset_commands = []
        clearance_height = self._get_operation_height(item, "ClearanceHeight", 0)

        if self._torch_active is not False:
            Path.Log.debug("Resetting torch to inactive")
            self._torch_active = False
            reset_commands.append(self.TorchExtinguishCommand)

        if (
            self._last_z is not None
            and CompValue(clearance_height) != 0
            and CompValue(self._last_z) != CompValue(clearance_height)
        ):
            Path.Log.debug("Resetting torch to clearence height")
            self._last_z = clearance_height
            move_cmd = Path.Command("G0", {"Z": clearance_height})
            reset_commands.append(move_cmd)

        return reset_commands

    def _get_property_value(self, name, default):
        """Get a property value from machine configuration with fallback to default."""
        if self._machine and hasattr(self._machine, "postprocessor_properties"):
            return self._machine.postprocessor_properties.get(name, default)
        return default

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        #
        super().init_values(values)
        #
        # Set any values here that need to override the default values set
        # in the parent routine.
        #
        values["ENABLE_COOLANT"] = True
        #
        # The order of parameters.
        #
        # linuxcnc doesn't want K properties on XY plane; Arcs need work.
        #
        values["PARAMETER_ORDER"] = [
            "X",
            "Y",
            "Z",
            "A",
            "B",
            "C",
            "I",
            "J",
            "F",
            "S",
            "T",
            "Q",
            "R",
            "L",
            "H",
            "D",
            "P",
        ]

        values["MACHINE_NAME"] = "GenericPlasma"
        values["POSTPROCESSOR_FILE_NAME"] = __name__
        #
        # Load preamble from machine configuration if available
        #
        if self._machine and hasattr(self._machine, "postprocessor_properties"):
            props = self._machine.postprocessor_properties
            values["PREAMBLE"] = props.get("preamble", "")
        else:
            values["PREAMBLE"] = ""

    def _inject_pierce_delay(self, postables):
        """Inject pierce delay after torch ignition command."""
        pierce_delay_ms = int(self._get_property_value("pierce_delay", 1000))
        if pierce_delay_ms <= 0:
            return

        # Marking doesn't pierce through stock (i.e. no delay needed)
        if self._get_property_value("mark_entry_only", False):
            return

        # Convert milliseconds to seconds for G4 command
        pierce_delay_sec = pierce_delay_ms / 1000.0

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    # Reset state for each operation
                    new_commands = self._reset_plasma_state(item)

                    for cmd in item.Path.Commands:
                        new_commands.append(cmd)
                        # After torch on commands, inject G4 pause
                        if cmd.Name == self.TorchIgniteCommand.Name:
                            # Create G4 dwell command with P parameter (seconds)
                            pause_cmd = Path.Command("G4", {"P": pierce_delay_sec})
                            new_commands.append(pause_cmd)
                    # Replace Path with modified command list
                    item.Path = Path.Path(new_commands)

    def _inject_cooling_delay(self, postables):
        """Inject cooling delay after torch extinguish command."""
        cooling_delay_ms = int(self._get_property_value("cooling_delay", 500))
        if cooling_delay_ms <= 0:
            return

        # Convert milliseconds to seconds for G4 command
        cooling_delay_sec = cooling_delay_ms / 1000.0

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    # Reset state for each operation
                    new_commands = self._reset_plasma_state(item)

                    for cmd in item.Path.Commands:
                        new_commands.append(cmd)
                        # After torch off command, inject G4 pause
                        if cmd.Name == self.TorchExtinguishCommand.Name:
                            # Create G4 dwell command with P parameter (seconds)
                            pause_cmd = Path.Command("G4", {"P": cooling_delay_sec})
                            new_commands.append(pause_cmd)
                    # Replace Path with modified command list
                    item.Path = Path.Path(new_commands)

    def _inject_torch_control(self, postables):
        """Handle torch ignition/extinguishment based on Z-axis movement."""
        if not self._get_property_value("torch_zaxis_control", True):
            return

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    # Get operation heights from the path object
                    pierce_height = self._get_operation_height(item, "StartDepth", 0)
                    cut_height = self._get_operation_height(item, "FinalDepth", 0)

                    # Reset state for each operation
                    new_commands = self._reset_plasma_state(item)

                    for cmd in item.Path.Commands:
                        # Only track Z movements for this injection
                        if "Z" not in cmd.Parameters:
                            new_commands.append(cmd)
                            continue

                        # Handle torch control based on Z movement
                        # Torch ignites AT pierce_height but is TRIGGERED by a move to cut_height
                        if not self._torch_active and CompValue(cmd.Parameters["Z"]) <= CompValue(
                            cut_height
                        ):
                            if self._get_property_value("mark_entry_only", False):
                                new_commands.append(cmd)
                                new_commands.append(self.TorchIgniteCommand)
                                self._torch_active = True
                                continue
                            else:
                                # Move to pierce height first if not already there
                                if self._last_z is None or CompValue(self._last_z) > CompValue(
                                    pierce_height
                                ):
                                    move_cmd = Path.Command("G0", {"Z": pierce_height})
                                    new_commands.append(move_cmd)

                                # Insert torch ignition command before Z- move
                                new_commands.append(self.TorchIgniteCommand)
                                self._torch_active = True
                        elif self._torch_active and CompValue(cmd.Parameters["Z"]) > CompValue(
                            cut_height
                        ):
                            # Insert torch extinguish command before Z+ move
                            new_commands.append(self.TorchExtinguishCommand)
                            self._torch_active = False

                        # Update last Z position
                        self._last_z = cmd.Parameters["Z"]

                        new_commands.append(cmd)
                    # Replace Path with modified command list
                    item.Path = Path.Path(new_commands)

    def _get_operation_height(self, item, height_type, default):
        """Get operation height (StartDepth/FinalDepth) from path object."""
        try:
            # Try to get the height from the path object's properties
            if hasattr(item, height_type):
                value = getattr(item, height_type)
                if value is not None:
                    return float(value)
            elif hasattr(item, "Base") and hasattr(item.Base, height_type):
                value = getattr(item.Base, height_type)
                if value is not None:
                    return float(value)
            else:
                # Try to get from proxy object
                proxy = getattr(item, "Proxy", None)
                if proxy and hasattr(proxy, height_type):
                    value = getattr(proxy, height_type)
                    if value is not None:
                        return float(value)
        except (AttributeError, TypeError, ValueError) as e:
            Path.Log.debug(f"GenericPlasma: Could not get {height_type}: {e}")
        return default

    def _inject_mark_entry_only(self, postables):
        """Mark first entry points only (Z- to cut height, torch on, short delay, torch off, Z+ to clearance)."""
        if not self._get_property_value("mark_entry_only", False):
            return

        marking_delay_ms = int(self._get_property_value("marking_delay", 100))

        # Convert milliseconds to seconds for G4 command
        marking_delay_sec = marking_delay_ms / 1000.0

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    # Get operation heights from the path object
                    cut_height = self._get_operation_height(item, "FinalDepth", 0)

                    # Reset state for each operation
                    new_commands = self._reset_plasma_state(item)
                    marked = False  # True once the first entry has been marked
                    in_cut = False  # True while descending/at cut height

                    for cmd in item.Path.Commands:
                        # Check if this is a Z move to cut height (first entry)
                        if (
                            not marked
                            and not in_cut
                            and "Z" in cmd.Parameters
                            and self._last_z is not None
                            and CompValue(cmd.Parameters["Z"]) < CompValue(self._last_z)
                            and CompValue(cmd.Parameters["Z"]) <= CompValue(cut_height)
                        ):

                            # Mark the entry point
                            # 1. Keep move decending to cut height (torch on)
                            new_commands.append(cmd)

                            # 2. Very short delay (torch mark)
                            if marking_delay_sec:
                                new_commands.append(Path.Command("G4", {"P": marking_delay_sec}))

                            marked = True
                            in_cut = True

                        # Skip remaining movement commands [while at cut height] until Z+ (retraction)
                        elif (
                            cmd.Name in Constants.GCODE_MOVE_LINE + Constants.GCODE_MOVE_ARC
                            and "Z" in cmd.Parameters
                        ):
                            # Only keep movements that are ascending (retraction)
                            if self._last_z is not None and CompValue(
                                cmd.Parameters["Z"]
                            ) > CompValue(self._last_z):
                                # 3. Keep movement ascending from cut height (torch off)
                                new_commands.append(cmd)
                                in_cut = False
                            elif not marked:
                                # Before first mark, allow initial positioning moves
                                new_commands.append(cmd)
                        elif cmd.Name in [
                            self.TorchIgniteCommand.Name,
                            self.TorchExtinguishCommand.Name,
                        ]:
                            # Skip torch commands in mark entry mode
                            continue
                        else:
                            # Keep non-movement commands
                            new_commands.append(cmd)

                        # Update last Z position
                        if "Z" in cmd.Parameters:
                            self._last_z = cmd.Parameters["Z"]

                    # Replace Path with modified command list
                    item.Path = Path.Path(new_commands)

    def _force_rapid_feeds(self, postables):
        """Replace all feed rates with rapid speeds for dry runs."""
        if not self._get_property_value("force_rapid_feeds", False):
            return

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    new_commands = []
                    for cmd in item.Path.Commands:
                        new_cmd = cmd
                        # Remove F parameter from all movement commands
                        if (
                            cmd.Name in Constants.GCODE_MOVE_LINE + Constants.GCODE_MOVE_ARC
                            and "F" in cmd.Parameters
                        ):
                            # Create new command without F parameter
                            new_params = dict(cmd.Parameters)
                            del new_params["F"]
                            new_cmd = Path.Command(cmd.Name, new_params)
                        new_commands.append(new_cmd)
                    # Replace Path with modified command list
                    item.Path = Path.Path(new_commands)

    def _expand_postprocessor_commands(self, postables):
        """Apply plasma-specific transformations to postables.

        This hook is called by the parent's export2() between Stage 1 (ordering)
        and Stage 2 (command expansion), ensuring transformations are applied to
        the actual postables that get converted to G-code.
        """
        Path.Log.debug("GenericPlasma: Applying plasma-specific transformations")
        self._inject_mark_entry_only(postables)
        self._inject_torch_control(postables)
        self._inject_pierce_delay(postables)
        self._inject_cooling_delay(postables)
        self._force_rapid_feeds(postables)

    def get_sanity_checks(self, job):
        """Plasma cutter specific sanity checks."""
        Path.Log.track("GenericPlasma.get_sanity_checks() called")
        squawks = []

        # Test squawk.  Remove this.  It will always add a warning.
        Path.Log.track("Adding test squawk from GenericPlasma")
        squawks.append(self._create_squawk("WARNING", "This is a test warning message"))

        # Check pierce delay vs material thickness
        pierce_delay = self.values.get("pierce_delay", 1000)
        if hasattr(job, "Stock") and hasattr(job.Stock, "Thickness"):
            thickness_mm = job.Stock.Thickness
            # Recommended pierce delay: ~70ms per mm of thickness, minimum 500ms
            recommended_delay = max(500, int(thickness_mm * 70))

            if pierce_delay < recommended_delay:
                if thickness_mm > 6.35:  # > 1/4 inch is more critical
                    squawks.append(
                        self._create_squawk(
                            "WARNING",
                            f"Pierce delay ({pierce_delay}ms) may be insufficient for {thickness_mm:.1f}mm material. Recommended: {recommended_delay}ms",
                        )
                    )
                else:
                    squawks.append(
                        self._create_squawk(
                            "NOTE",
                            f"Pierce delay ({pierce_delay}ms) may be short for {thickness_mm:.1f}mm material. Consider: {recommended_delay}ms",
                        )
                    )

        # Check cooling delay for thick materials
        cooling_delay = self.values.get("cooling_delay", 500)
        if hasattr(job, "Stock") and hasattr(job.Stock, "Thickness"):
            thickness_mm = job.Stock.Thickness
            if thickness_mm > 10.0 and cooling_delay < 1000:  # Thick materials need more cooling
                squawks.append(
                    self._create_squawk(
                        "NOTE",
                        f"Consider increasing cooling delay for {thickness_mm:.1f}mm material to prevent torch overheating",
                    )
                )

        # Check for rapid moves with torch enabled
        if self.values.get("torch_zaxis_control", True):
            for op in getattr(job.Operations, "Group", []):
                if hasattr(op, "HoriFeed") and op.HoriFeed > 3000:  # High feed rates
                    squawks.append(
                        self._create_squawk(
                            "CAUTION",
                            f"Operation '{op.Label}' has high feed rate ({op.HoriFeed}) with torch Z-control - verify safe operation",
                        )
                    )

        # Check marking delay vs pierce delay
        marking_delay = self.values.get("marking_delay", 100)
        if marking_delay >= pierce_delay:
            squawks.append(
                self._create_squawk(
                    "NOTE",
                    f"Marking delay ({marking_delay}ms) >= pierce delay ({pierce_delay}ms) - marking may not be distinct from cutting",
                )
            )

        Path.Log.track(f"GenericPlasma.get_sanity_checks() returning {len(squawks)} squawks")
        return squawks

    @property
    def tooltip(self):
        tooltip: str = """
        This is a postprocessor file for the CAM workbench.
        It is used to take a pseudo-gcode fragment from a CAM object
        and output 'real' GCode suitable for a plasma cutter.

        Features:
        - Torch Z-axis control (M3/M5 based on Z movement)
        - Pierce delay after torch ignition
        - Cooling delay after torch extinguishment
        - Mark entry points only mode
        - Force rapid feeds for dry runs
        - Material-aware sanity checks
        """
        return tooltip


# Class aliases for PostProcessorFactory
# The factory looks for a class with title-cased postname (e.g., "Generic_Plasma")
Generic_Plasma = GenericPlasma
Genericplasma = GenericPlasma  # Fallback for different title() behavior
