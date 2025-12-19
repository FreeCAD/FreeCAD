# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""
OpenSBP Post Processor for ShopBot Controllers

This is a new-style postprocessor that uses the hook methods pattern to override
specific command handling for the OpenSBP dialect used by ShopBot controllers.

OpenSBP uses commands like:
  - MX, MY, MZ - Move (feed) single axis
  - M2, M3 - Move (feed) multiple axes
  - JX, JY, JZ - Jog (rapid) single axis
  - J2, J3 - Jog (rapid) multiple axes
  - CG - Circular interpolation (arcs)
  - TR - Set spindle RPM
  - MS, JS - Set move/jog speeds

This postprocessor demonstrates how to override only the necessary hook methods
without reimplementing the entire convert_command_to_gcode function.
"""

from typing import Any, Dict

from Path.Post.Processor import PostProcessor

import Path
import FreeCAD

translate = FreeCAD.Qt.translate

DEBUG = False


# Set logging level based on DEBUG flag
def _setup_logging():
    if DEBUG:
        Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
        Path.Log.trackModule(Path.Log.thisModule())
    else:
        Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


_setup_logging()

# Define types
Values = Dict[str, Any]


class OpenSBPPost(PostProcessor):
    """
    OpenSBP postprocessor for ShopBot controllers.

    This class demonstrates the new hook methods pattern by overriding only
    the specific command conversion methods needed for OpenSBP dialect.

    OpenSBP uses native commands prefixed with '>' for non-G-code operations.
    """

    @classmethod
    def get_common_property_schema(cls):
        """Override common properties with OpenSBP-specific defaults."""
        common_props = super().get_common_property_schema()

        # Override defaults for OpenSBP
        for prop in common_props:
            if prop["name"] == "file_extension":
                prop["default"] = "sbp"
            elif prop["name"] == "preamble":
                prop["default"] = (
                    "'OpenSBP output from FreeCAD\n"
                    "'NOTE: In OpenSBP, M3 is a 3-axis MOVE command, NOT spindle control\n"
                    "'Spindle control is via TR (speed) and C6/C7 (on/off) commands"
                )
            elif prop["name"] == "postamble":
                prop["default"] = ">C7\n'End of program"

        return common_props

    @classmethod
    def get_property_schema(cls):
        """Return schema for OpenSBP-specific configurable properties."""
        return [
            {
                "name": "automatic_tool_changer",
                "type": "bool",
                "label": translate("CAM", "Automatic Tool Changer"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Enable if machine has automatic tool changer. "
                    "If disabled, tool changes will pause for manual intervention.",
                ),
            },
            {
                "name": "automatic_spindle",
                "type": "bool",
                "label": translate("CAM", "Automatic Spindle Control"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Enable if machine has automatic spindle speed control. "
                    "If disabled, spindle commands will prompt for manual adjustment.",
                ),
            },
        ]

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "OpenSBP post processor for ShopBot controllers"),
        tooltipargs=[],
        units="Metric",
    ) -> None:
        super().__init__(
            job=job,
            tooltip=tooltip,
            tooltipargs=tooltipargs,
            units=units,
        )
        Path.Log.debug("OpenSBP post processor initialized.")

        # Track current speeds for OpenSBP (separate XY and Z speeds)
        self._current_move_speed_xy = None
        self._current_move_speed_z = None
        self._current_jog_speed_xy = None
        self._current_jog_speed_z = None

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        super().init_values(values)

        # OpenSBP-specific settings
        values["MACHINE_NAME"] = "ShopBot"
        values["POSTPROCESSOR_FILE_NAME"] = __name__

        # Load configuration from machine properties if available
        if self._machine and hasattr(self._machine, "postprocessor_properties"):
            props = self._machine.postprocessor_properties
            values["AUTOMATIC_TOOL_CHANGER"] = props.get("automatic_tool_changer", False)
            values["AUTOMATIC_SPINDLE"] = props.get("automatic_spindle", False)
        else:
            values["AUTOMATIC_TOOL_CHANGER"] = False
            values["AUTOMATIC_SPINDLE"] = False

    def _convert_comment(self, command):
        """
        Convert comments to OpenSBP format (single quote prefix).
        """
        # Extract comment text
        comment_text = (
            command.Name[1:-1]
            if command.Name.startswith("(") and command.Name.endswith(")")
            else command.Name[1:]
        )

        # OpenSBP uses single quote for comments
        return f"'{comment_text}"

    def _convert_rapid_move(self, command):
        """
        Convert rapid moves (G0) to OpenSBP jog commands (JX, JY, JZ, J2, J3).
        """
        return self._convert_move_command(command, is_rapid=True)

    def _convert_linear_move(self, command):
        """
        Convert linear moves (G1) to OpenSBP move commands (MX, MY, MZ, M2, M3).
        """
        return self._convert_move_command(command, is_rapid=False)

    def _convert_move_command(self, command, is_rapid):
        """
        Convert move commands to OpenSBP format.

        OpenSBP uses different commands based on:
        - Move type: M (feed) or J (jog/rapid)
        - Axes involved: X, Y, Z, 2 (XY), 3 (XYZ)

        Native OpenSBP commands are prefixed with '>'
        """
        params = command.Parameters
        output = []

        # Determine which axes are moving
        has_x = "X" in params
        has_y = "Y" in params
        has_z = "Z" in params

        # Get unit conversion function
        def get_value(val):
            """Convert value based on machine units."""
            if self._machine and hasattr(self._machine, "output"):
                from Machine.models.machine import OutputUnits

                if self._machine.output.units == OutputUnits.IMPERIAL:
                    return val / 25.4
            return val

        # Handle speed settings (MS/JS commands)
        if "F" in params:
            speed = params["F"] * 60.0  # Convert mm/sec to mm/min
            speed = get_value(speed)

            prefix = "JS" if is_rapid else "MS"

            # OpenSBP has separate speeds for XY and Z
            xy_speed = ""
            z_speed = ""

            if has_z:
                speed_attr = "_current_jog_speed_z" if is_rapid else "_current_move_speed_z"
                if getattr(self, speed_attr) != speed:
                    setattr(self, speed_attr, speed)
                    z_speed = f"{speed:.4f}"

            if has_x or has_y:
                speed_attr = "_current_jog_speed_xy" if is_rapid else "_current_move_speed_xy"
                if getattr(self, speed_attr) != speed:
                    setattr(self, speed_attr, speed)
                    xy_speed = f"{speed:.4f}"

            # Only output speed command if it changed
            if xy_speed or z_speed:
                output.append(f">{prefix},{xy_speed},{z_speed}")

        # Generate move command based on axes
        prefix = "J" if is_rapid else "M"

        if has_x and has_y and has_z:
            # XYZ move - use M3/J3
            x_val = get_value(params["X"])
            y_val = get_value(params["Y"])
            z_val = get_value(params["Z"])
            output.append(f">{prefix}3,{x_val:.4f},{y_val:.4f},{z_val:.4f}")

        elif has_x and has_y:
            # XY move - use M2/J2
            x_val = get_value(params["X"])
            y_val = get_value(params["Y"])
            output.append(f">{prefix}2,{x_val:.4f},{y_val:.4f}")

        elif has_x and has_z:
            # XZ move - use M3/J3 with empty Y
            x_val = get_value(params["X"])
            z_val = get_value(params["Z"])
            output.append(f">{prefix}3,{x_val:.4f},,{z_val:.4f}")

        elif has_y and has_z:
            # YZ move - use M3/J3 with empty X
            y_val = get_value(params["Y"])
            z_val = get_value(params["Z"])
            output.append(f">{prefix}3,,{y_val:.4f},{z_val:.4f}")

        elif has_x:
            # X only - use MX/JX
            x_val = get_value(params["X"])
            output.append(f">{prefix}X,{x_val:.4f}")

        elif has_y:
            # Y only - use MY/JY
            y_val = get_value(params["Y"])
            output.append(f">{prefix}Y,{y_val:.4f}")

        elif has_z:
            # Z only - use MZ/JZ
            z_val = get_value(params["Z"])
            output.append(f">{prefix}Z,{z_val:.4f}")

        return "\n".join(output) if output else None

    def _convert_arc_move(self, command):
        """
        Convert arc moves (G2/G3) to OpenSBP CG command.

        OpenSBP CG format: >CG,,X,Y,I,J,T,direction[,plunge]
        where:
        - direction is 1 for CW (G2) or -1 for CCW (G3)
        - plunge is optional Z movement (relative, sign inverted)

        Note: ShopBot only supports arcs in XY plane with I,J offsets.
        If Z is present, it's converted to a helical arc with plunge parameter.
        """
        params = command.Parameters

        # Get unit conversion function
        def get_value(val):
            if self._machine and hasattr(self._machine, "output"):
                from Machine.models.machine import OutputUnits

                if self._machine.output.units == OutputUnits.IMPERIAL:
                    return val / 25.4
            return val

        # Determine direction
        direction = "1" if command.Name in ["G2", "G02"] else "-1"

        # Extract arc parameters
        x_val = get_value(params.get("X", 0))
        y_val = get_value(params.get("Y", 0))
        i_val = get_value(params.get("I", 0))
        j_val = get_value(params.get("J", 0))

        # Check for helical arc (Z parameter present)
        output = []
        if "Z" in params:
            # Helical arc - need to calculate plunge
            # Get current Z from modal state (default to 0 if not set)
            current_z = self._modal_state.get("Z", 0.0)
            if current_z is None:
                current_z = 0.0
            target_z = params["Z"]
            plunge = get_value(current_z - target_z)  # Relative, inverted sign

            # Set move speed if feed rate is specified
            if "F" in params:
                speed = params["F"] * 60.0  # Convert mm/sec to mm/min
                speed = get_value(speed)
                # Only output if speed changed
                if self._current_move_speed_xy != speed or self._current_move_speed_z != speed:
                    output.append(f">MS,{speed:.4f},{speed:.4f}")
                    self._current_move_speed_xy = speed
                    self._current_move_speed_z = speed

            # Use L (linear) instead of T (tool comp) for helical arcs
            output.append(
                f">CG,,{x_val:.4f},{y_val:.4f},{i_val:.4f},{j_val:.4f},L,{direction},{plunge:.4f}"
            )
        else:
            # Simple arc in XY plane (no tool compensation - use L instead of T)
            output.append(f">CG,,{x_val:.4f},{y_val:.4f},{i_val:.4f},{j_val:.4f},L,{direction}")

        return "\n".join(output) if output else None

    def _convert_tool_change(self, command):
        """
        Convert tool change (M6) to OpenSBP tool commands.

        Supports both automatic and manual tool changers based on configuration.
        """
        params = command.Parameters
        tool_num = int(params.get("T", 0))

        output = []

        # Check if automatic tool changer is enabled
        has_atc = self.values.get("AUTOMATIC_TOOL_CHANGER", False)

        if has_atc:
            # Automatic tool changer
            output.append(f">&ToolName={tool_num}")
            output.append(f">&Tool={tool_num}")
        else:
            # Manual tool change - pause and prompt
            output.append(f"'Manual tool change to T{tool_num}")
            output.append(f">&ToolName={tool_num}")
            output.append(f">&Tool={tool_num}")
            output.append(">PAUSE")

        return "\n".join(output)

    def _convert_spindle_command(self, command):
        """
        Convert spindle commands (M3/M4/M5) to OpenSBP TR command.

        Supports both automatic and manual spindle control based on configuration.
        """
        params = command.Parameters
        has_auto_spindle = self.values.get("AUTOMATIC_SPINDLE", False)

        if command.Name in ["M5", "M05"]:
            # Spindle off
            if has_auto_spindle:
                return ">TR,0\n>C7"
            else:
                return "'Turn spindle OFF manually\n>PAUSE"

        # Spindle on (M3/M4)
        rpm = int(params.get("S", 0))

        output = []

        if has_auto_spindle:
            # Automatic spindle control
            output.append(f">TR,{rpm}")
            output.append(">C6")  # Start spindle
            output.append(">PAUSE 2")  # Wait for spindle to reach speed
        else:
            # Manual spindle control - prompt user
            output.append(f"'Set spindle to {rpm} RPM and start manually")
            output.append(">PAUSE")

        return "\n".join(output)

    def _convert_dwell(self, command):
        """
        Convert dwell (G4) to OpenSBP PAUSE command.
        """
        params = command.Parameters
        seconds = params.get("P", 0)

        return f">PAUSE {seconds:.2f}"

    def _convert_fixture(self, command):
        """
        Suppress fixture commands (G54-G59) as OpenSBP doesn't use them.
        """
        # OpenSBP doesn't have work coordinate systems
        return None

    def _convert_modal_command(self, command):
        """
        Suppress most modal commands that don't apply to OpenSBP.
        """
        # OpenSBP doesn't use most G-code modal commands
        # Suppress G21/G20 (units), G43, G80, G90, G91, etc.
        return None

    @property
    def tooltip(self):
        tooltip: str = """
        This is a postprocessor file for the CAM workbench.
        It is used to take a pseudo-gcode fragment from a CAM object
        and output OpenSBP code suitable for ShopBot CNC controllers.

        OpenSBP uses native commands like MX, MY, M2, M3 for moves,
        CG for arcs, TR for spindle speed, etc.
        """
        return tooltip


# Class alias for PostProcessorFactory
# The factory looks for a class with title-cased postname (e.g., "Opensbp")
Opensbp = OpenSBPPost


# Factory function for creating the postprocessor
def create(job, **kwargs):
    """
    Factory function to create an OpenSBP postprocessor instance.
    """
    return OpenSBPPost(job, **kwargs)
