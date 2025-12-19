# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 - 2025 Larry Woestman <LarryWoestman2@gmail.com>   *
# *   Copyright (c) 2024 Ondsel <development@ondsel.com>                    *
# *   Copyright (c) 2024 Carl Slater <CandLWorkshopLLC@gmail.com>           *
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


from typing import Any, Dict

from Path.Post.Processor import PostProcessor

import Path
import FreeCAD

translate = FreeCAD.Qt.translate

DEBUG = False
if DEBUG:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

# Define some types that are used throughout this file.
Values = Dict[str, Any]


class Linuxcnc(PostProcessor):
    """
    The LinuxCNC post processor class.
    LinuxCNC supports various trajectory control methods (path blending) as
    described at https://linuxcnc.org/docs/2.4/html/common_User_Concepts.html#r1_1_2

    This post processor implements the following trajectory control methods:
    - Exact Path (G61)
    - Exact Stop (G61.1)
    - Blend (G64)
    """

    @classmethod
    def get_common_property_schema(cls):
        """Override common properties with LinuxCNC-specific defaults."""
        common_props = super().get_common_property_schema()

        # Override defaults for LinuxCNC
        for prop in common_props:
            if prop["name"] == "file_extension":
                prop["default"] = "ngc"
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
        """Return schema for LinuxCNC-specific configurable properties."""
        return [
            {
                "name": "blend_mode",
                "type": "choice",
                "label": translate("CAM", "Path Blending Mode"),
                "default": "BLEND",
                "choices": ["EXACT_PATH", "EXACT_STOP", "BLEND"],
                "help": translate(
                    "CAM",
                    "Path blending mode: EXACT_PATH (G61) stops at each point, "
                    "EXACT_STOP (G61.1) stops at path ends, BLEND (G64) allows smooth motion",
                ),
            },
            {
                "name": "blend_tolerance",
                "type": "float",
                "label": translate("CAM", "Blend Tolerance"),
                "default": 0.0,
                "min": 0.0,
                "max": 10.0,
                "decimals": 4,
                "help": translate(
                    "CAM",
                    "Tolerance for BLEND mode (P value): 0 = no tolerance (G64), "
                    ">0 = tolerance (G64 P-), in current units",
                ),
            },
        ]

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "LinuxCNC post processor"),
        tooltipargs=[],
        units="Metric",
    ) -> None:
        super().__init__(
            job=job,
            tooltip=tooltip,
            tooltipargs=tooltipargs,
            units=units,
        )
        Path.Log.debug("LinuxCNC post processor initialized.")

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

        values["MACHINE_NAME"] = "LinuxCNC"
        values["POSTPROCESSOR_FILE_NAME"] = __name__
        #
        # Load preamble from machine configuration if available
        #
        if self._machine and hasattr(self._machine, "postprocessor_properties"):
            props = self._machine.postprocessor_properties
            values["PREAMBLE"] = props.get("preamble", "")
        else:
            values["PREAMBLE"] = ""

        # Path blending mode configuration (LinuxCNC-specific)
        # Load from machine configuration if available, otherwise use defaults
        #
        if self._machine and hasattr(self._machine, "postprocessor_properties"):
            props = self._machine.postprocessor_properties
            values["BLEND_MODE"] = props.get("blend_mode", "BLEND")
            values["BLEND_TOLERANCE"] = props.get("blend_tolerance", 0.0)
        else:
            # Fallback to defaults if no machine configuration
            values["BLEND_MODE"] = "BLEND"
            values["BLEND_TOLERANCE"] = 0.0

        # Add blend command to PREAMBLE
        blend_cmd = self._get_blend_command()
        if values["PREAMBLE"]:
            values["PREAMBLE"] += f"\n{blend_cmd}"
        else:
            values["PREAMBLE"] = blend_cmd

    def export2(self):
        """Override export2 to inject blend command before parent processing.

        This ensures the blend command is added to the preamble before
        the parent's export2() reads it from postprocessor_properties.
        """
        # Inject blend command into preamble before parent export2 processes it
        if self._machine and hasattr(self._machine, "postprocessor_properties"):
            blend_cmd = self._get_blend_command()
            props = self._machine.postprocessor_properties
            current_preamble = props.get("preamble", "")
            if current_preamble:
                props["preamble"] = f"{current_preamble}\n{blend_cmd}"
            else:
                props["preamble"] = blend_cmd

        # Call parent export2 which will now include the blend command in preamble
        return super().export2()

    def _get_blend_command(self) -> str:
        """Generate the path blending G-code command based on current settings.

        Reads from postprocessor_properties if available, otherwise falls back to values dict.
        """
        # Try to read from postprocessor_properties first (for export2)
        if self._machine and hasattr(self._machine, "postprocessor_properties"):
            props = self._machine.postprocessor_properties
            mode = props.get("blend_mode", "BLEND")
            tolerance = props.get("blend_tolerance", 0.0)
        else:
            # Fallback to values dict (for legacy export)
            mode = self.values.get("BLEND_MODE", "BLEND")
            tolerance = self.values.get("BLEND_TOLERANCE", 0.0)

        if mode == "EXACT_PATH":
            return "G61"
        elif mode == "EXACT_STOP":
            return "G61.1"
        else:  # BLEND
            if tolerance > 0:
                return f"G64 P{tolerance:.4f}"
            else:
                return "G64"

    # Property tooltip is inherited from base class

    def _convert_drill_cycle(self, command):
        """
        Convert drill cycle commands to G-code.

        For G84/G74 tapping cycles, check for 'rigid' annotation and convert
        to G33.1 rigid tapping if present. Otherwise use standard conversion.
        """
        from Path.Post.UtilsParse import format_command_line

        # Check if this is a tapping cycle with rigid annotation
        if command.Name in ["G84", "G74"]:
            annotations = command.Annotations
            is_rigid = annotations.get("rigid", "False") == "True"

            if is_rigid:
                # Rigid tapping - convert to G33.1
                params = command.Parameters.copy()

                # Extract pitch from F parameter
                if "F" not in params:
                    Path.Log.warning(f"Rigid tapping {command.Name} missing F (pitch) parameter")
                    return super()._convert_drill_cycle(command)

                pitch = params["F"]

                # Get unit conversion function
                def get_value(val):
                    if self._machine and hasattr(self._machine, "output"):
                        from Machine.models.machine import OutputUnits

                        if self._machine.output.units == OutputUnits.IMPERIAL:
                            return val / 25.4
                    return val

                pitch = get_value(pitch)

                # Build output commands
                output = []
                block_delete = "/" if annotations.get("blockdelete") else ""

                # Initial G33.1 command (in)
                cmd_line = ["G33.1"]
                cmd_line.append(f"K{pitch:.4f}")

                if "Z" in params:
                    z_val = get_value(params["Z"])
                    cmd_line.append(f"Z{z_val:.4f}")

                if "X" in params:
                    x_val = get_value(params["X"])
                    cmd_line.append(f"X{x_val:.4f}")

                if "Y" in params:
                    y_val = get_value(params["Y"])
                    cmd_line.append(f"Y{y_val:.4f}")

                output.append(f"{block_delete}{' '.join(cmd_line)}")

                # Handle dwell if P parameter present
                if "P" in params:
                    output.append(f"{block_delete}M5")
                    output.append(f"{block_delete}G04 P{params['P']:.2f}")

                # Reverse out
                if command.Name == "G84":
                    # Right-hand tap: reverse spindle (M4), retract, restore (M3)
                    output.append(f"{block_delete}M4")

                    # Retract to R height
                    retract_line = ["G33.1", f"K{pitch:.4f}"]
                    if "R" in params:
                        r_val = get_value(params["R"])
                        retract_line.append(f"Z{r_val:.4f}")
                    output.append(f"{block_delete}{' '.join(retract_line)}")

                    output.append(f"{block_delete}M3")

                elif command.Name == "G74":
                    # Left-hand tap: forward spindle (M3), retract, restore (M4)
                    output.append(f"{block_delete}M3")

                    # Retract to R height
                    retract_line = ["G33.1", f"K{pitch:.4f}"]
                    if "R" in params:
                        r_val = get_value(params["R"])
                        retract_line.append(f"Z{r_val:.4f}")
                    output.append(f"{block_delete}{' '.join(retract_line)}")

                    output.append(f"{block_delete}M4")

                return "\n".join(output)

        # Not rigid tapping or not a tapping cycle - use parent implementation
        return super()._convert_drill_cycle(command)

    def _convert_modal_command(self, command):
        """
        Convert modal commands to G-code.

        Suppress G80, G98, G99 if they're part of a rigid tapping operation.
        """
        # Check if this is G80/G98/G99 with tapping annotation
        if command.Name in ["G80", "G98", "G99"]:
            annotations = command.Annotations
            # Check if this is part of a tapping operation with rigid annotation
            if annotations.get("operation") == "tapping":
                is_rigid = annotations.get("rigid", "False") == "True"
                if is_rigid:
                    # Suppress these commands for rigid tapping
                    return None

        # Use parent implementation for other modal commands
        return super()._convert_modal_command(command)

    @property
    def tooltip(self):
        tooltip: str = """
        This is a postprocessor file for the CAM workbench.
        It is used to take a pseudo-gcode fragment from a CAM object
        and output 'real' GCode suitable for a linuxcnc 3 axis mill.

        Supports rigid tapping via G33.1 when the 'rigid' annotation is present
        on G84/G74 tapping cycles.
        """
        return tooltip
