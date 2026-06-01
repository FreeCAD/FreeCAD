# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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
Standalone drill cycle expander for FreeCAD Path.Command objects.

This module provides a clean API for expanding canned drill cycles without
coupling to the postprocessing infrastructure.
"""

from typing import List, Optional

import Path
from Path.Base.MachineState import MachineState

EXPANDABLE_DRILL_CYCLES = {"G81", "G82", "G83", "G73"}


debug = True
if debug:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class DrillCycleExpander:
    """Expands canned drill cycles (Path.Command) into basic G-code movements."""

    EXPANDABLE_CYCLES = EXPANDABLE_DRILL_CYCLES

    def __init__(
        self,
        machine_state: MachineState,  # we mutate
    ):
        """
        Initialize the expander.

        Per ADR-002, Path Command coordinates are always absolute.

        Args:
            MachineState, including ReturnMode (G98/G99), and required initial axis XYZ, and F and G0F
        """
        self.machine_state = machine_state

    def expand_command(self, command: Path.Command) -> List[Path.Command]:
        """
        Expand a single drill cycle command into basic movements.

        Args:
            command: Path.Command object (e.g., Path.Command("G81", {"X": 10.0, "Y": 10.0, "Z": -5.0, "R": 2.0, "F": 100.0}))

        Returns:
            List of expanded Path.Command objects
        """
        cmd_name = command.Name.upper()
        params = command.Parameters

        # Handle modal commands - filter them out after processing
        if cmd_name in ["G98", "G99"]:
            self.machine_state.addCommand(command)
            return []  # Filter out after processing
        elif cmd_name == "G80":
            # Cancel drill cycle - filter out since cycles are already expanded
            return []

        # Handle drill cycles
        if cmd_name in ("G81", "G82", "G73", "G83"):
            result = self._expand_drill_cycle(command)
            Path.Log.debug(f"Expanded drill cycle: {command} -> {result}")
            return result

        else:
            # Update position for non-drill commands
            self.machine_state.addCommand(command)

            # Pass through other commands unchanged
            Path.Log.debug(f"Passing through command: {command}")
            return [command]

    def _expand_drill_cycle(self, command: Path.Command) -> List[Path.Command]:
        """Expand a drill cycle into basic movements.
         As per ADR-002, we are in Path.Command world, so no G91, and not modal
        Does not support repeat-on-move-till-G80 (modal repeat drill)
        Does not support L
        Needs a Z-start position
        Q is peck amount, a distance not position
        R is a position
            R must always be above surface: initial move is a G0
        Z is a position
        P is dwell
        chip-break is a position (R), or chipbreaking_amount|5% for G73
        peck-return-to-bottom-clearance (fast-move) is hard-coded delta.
        """
        cmd_name = command.Name.upper()
        params = command.Parameters

        # Extract parameters
        drill_x = params.get("X", self.machine_state.X)
        drill_y = params.get("Y", self.machine_state.Y)
        drill_z = params["Z"]
        retract_z = params["R"]
        feedrate = params.get("F")

        # Store initial Z for G98 mode
        initial_z = self.machine_state.Z

        # Determine final retract height
        if self.machine_state.ReturnMode == "Z":
            final_retract = max(initial_z, retract_z)
        else:  # G99
            final_retract = retract_z

        # Error check
        if retract_z < drill_z:
            # Return empty list or could raise exception
            return []

        # Preliminary moves should match the linuxcnc documentation
        # https://linuxcnc.org/docs/html/gcode/g-code.html#gcode:preliminary-motion

        expanded = []

        # Preliminary motion: If Z < R, move Z to R once (LinuxCNC spec)
        if self.machine_state.Z < retract_z:
            cmd = Path.Command(
                "G0",
                {
                    "X": self.machine_state.X,
                    "Y": self.machine_state.Y,
                    "Z": retract_z,
                    "F": self.machine_state.G0F,
                },
            )
            expanded.append(cmd)
            self.machine_state.addCommand(cmd)

        # Move to XY position at current Z height (which should be R)
        if drill_x != self.machine_state.X or drill_y != self.machine_state.Y:
            cmd = Path.Command(
                "G0",
                {
                    "X": drill_x,
                    "Y": drill_y,
                    "Z": self.machine_state.Z,
                    "F": self.machine_state.G0F,
                },
            )
            expanded.append(cmd)
            self.machine_state.addCommand(cmd)

        # Ensure Z is at R position (should already be there from preliminary motion)
        if self.machine_state.Z != retract_z:
            cmd = Path.Command(
                "G0",
                {
                    "X": self.machine_state.X,
                    "Y": self.machine_state.Y,
                    "Z": retract_z,
                    "F": self.machine_state.G0F,
                },
            )
            expanded.append(cmd)
            self.machine_state.addCommand(cmd)

        # Perform the drilling operation
        if cmd_name in ("G81", "G82"):
            expanded.extend(
                self._expand_g81_g82(cmd_name, params, drill_z, final_retract, feedrate)
            )
        elif cmd_name in ("G73", "G83"):
            expanded.extend(
                self._expand_g73_g83(cmd_name, params, drill_z, retract_z, final_retract, feedrate)
            )

        return expanded

    def _expand_g81_g82(
        self,
        cmd_name: str,
        params: dict,
        drill_z: float,
        final_retract: float,
        feedrate: Optional[float],
    ) -> List[Path.Command]:
        """Expand G81 (simple drill) or G82 (drill with dwell)."""
        expanded = []

        # Feed to depth
        move_params = {
            "X": self.machine_state.X,
            "Y": self.machine_state.Y,
            "Z": drill_z,
            "F": self.machine_state.F,
        }
        if feedrate:
            move_params["F"] = feedrate
        cmd = Path.Command("G1", move_params)
        expanded.append(cmd)
        self.machine_state.addCommand(cmd)

        # Dwell for G82
        if cmd_name == "G82" and "P" in params:
            expanded.append(Path.Command("G4", {"P": params["P"]}))

        # Retract
        cmd = Path.Command(
            "G0",
            {
                "X": self.machine_state.X,
                "Y": self.machine_state.Y,
                "Z": final_retract,
                "F": self.machine_state.G0F,
            },
        )
        expanded.append(cmd)
        self.machine_state.addCommand(cmd)

        return expanded

    def _expand_g73_g83(
        self,
        cmd_name: str,
        params: dict,
        drill_z: float,
        retract_z: float,
        final_retract: float,
        feedrate: Optional[float],
    ) -> List[Path.Command]:
        """Expand G73 (chip breaking) or G83 (peck drilling)."""
        expanded = []

        peck_depth = params.get("Q", abs(drill_z - retract_z))
        current_depth = retract_z
        clearance_amount = peck_depth * 0.05  # Small clearance above previous cut

        while current_depth > drill_z:
            # Calculate next peck depth
            next_depth = max(current_depth - peck_depth, drill_z)

            # If not first peck, rapid to clearance above previous depth
            if current_depth != retract_z and cmd_name == "G83":
                approach_height = current_depth + clearance_amount
                cmd = Path.Command(
                    "G0",
                    {
                        "X": self.machine_state.X,
                        "Y": self.machine_state.Y,
                        "Z": approach_height,
                        "F": self.machine_state.G0F,
                    },
                )
                expanded.append(cmd)
                self.machine_state.addCommand(cmd)

            # Feed to next depth
            move_params = {
                "X": self.machine_state.X,
                "Y": self.machine_state.Y,
                "Z": next_depth,
                "F": self.machine_state.F,
            }
            if feedrate:
                move_params["F"] = feedrate
            cmd = Path.Command("G1", move_params)
            expanded.append(cmd)
            self.machine_state.addCommand(cmd)

            # Retract based on cycle type
            if cmd_name == "G73":
                if next_depth == drill_z:
                    # Final peck - retract to R
                    cmd = Path.Command(
                        "G0",
                        {
                            "X": self.machine_state.X,
                            "Y": self.machine_state.Y,
                            "Z": retract_z,
                            "F": self.machine_state.G0F,
                        },
                    )
                    expanded.append(cmd)
                    self.machine_state.addCommand(cmd)
                else:
                    # Chip breaking - small retract
                    approach_height = next_depth + clearance_amount
                    cmd = Path.Command(
                        "G0",
                        {
                            "X": self.machine_state.X,
                            "Y": self.machine_state.Y,
                            "Z": approach_height,
                            "F": self.machine_state.G0F,
                        },
                    )
                    expanded.append(cmd)
                    self.machine_state.addCommand(cmd)
            elif cmd_name == "G83":
                # Full retract to R plane
                cmd = Path.Command(
                    "G0",
                    {
                        "X": self.machine_state.X,
                        "Y": self.machine_state.Y,
                        "Z": retract_z,
                        "F": self.machine_state.G0F,
                    },
                )
                expanded.append(cmd)
                self.machine_state.addCommand(cmd)

            current_depth = next_depth

        # Final retract
        if self.machine_state.Z != final_retract:
            cmd = Path.Command(
                "G0",
                {
                    "X": self.machine_state.X,
                    "Y": self.machine_state.Y,
                    "Z": final_retract,
                    "F": self.machine_state.G0F,
                },
            )
            expanded.append(cmd)
            self.machine_state.addCommand(cmd)

        return expanded

    def expand_commands(self, commands: List[Path.Command]) -> List[Path.Command]:
        """
        Expand a list of Path.Command objects.

        Args:
            commands: List of Path.Command objects

        Returns:
            List of expanded Path.Command objects
        """
        expanded = []
        for cmd in commands:
            expanded.extend(self.expand_command(cmd))
        return expanded

    def expand_path(self, path: Path.Path) -> Path.Path:
        """
        Expand drill cycles in a Path object.

        Args:
            path: Path.Path object containing commands

        Returns:
            New Path.Path object with expanded commands
        """
        expanded_commands = self.expand_commands(path.Commands)
        return Path.Path(expanded_commands)
