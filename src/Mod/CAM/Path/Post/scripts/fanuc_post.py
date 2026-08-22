# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Fae Corrigan
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

from Path.Post.Processor import PostProcessor
import Path
import FreeCAD
import Constants

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


POST_TYPE = "machine"


class Fanuc(PostProcessor):
    @classmethod
    def get_common_property_schema(cls):
        """Return common properties with Fanuc defaults (uses base defaults)"""
        """Override common properties with Fanuc-specific defaults"""
        common_props = super().get_common_property_schema()

        return common_props

    @classmethod
    def get_property_schema(cls):
        return [
            {
                "name": "END_SPINDLE_EMPTY",
                "type": "bool",
                "runtime": True,
                "label": translate("CAM", "End Spindle Empty"),
                "default": False,
                "help": translate("CAM", ""),
            },
        ]

    def __init__(self, job):
        super().__init__(
            job=job,
            tooltip=translate("CAM", "Fanuc post processor"),
            tooltipargs=[],
            units="Metric",
        )
        Path.Log.debug("Fanuc post processor initialized")

    def init_values(self, values: dict) -> None:
        """Initialize values that are used throughout the postprocessor"""

        super().init_values(values)

        values["POSTPROCESSOR_FILE_NAME"] = __name__
        values["MACHINE_NAME"] = "Fanuc"

        # Set any values here that need to override the default values set
        # in the parent routine.
        #
        # Any commands in this value will be output after the header and
        # safety block at the beginning of the G-code file.
        #
        values["PREAMBLE"] = """"""
        #
        # Any commands in this value will be output as the last commands
        # in the G-code file.
        #
        values["POSTAMBLE"] = """"""

    def convert_command_to_gcode(self, command: Path.Command):
        if command.Name == "G20":
            self._units = "Imperial"
        if command.Name == "G21":
            self._units = "Metric"

        if command.Name in ["G74", "G84"]:
            out = ""
            pitch_mm = float(command.Parameters["F"])
            tapSpeed = None
            # Convert pitch to inches if needed
            if self._units == "Imperial":  # imperial
                pitch = pitch_mm / 25.4
            else:
                pitch = pitch_mm
            if command.Parameters["S"]:
                tapSpeed = int(command.Parameters["S"])
            out += "M29 S" + str(tapSpeed) + "\n"

            feed_rate = None

            # Calculate feed rate as distance per minute
            if tapSpeed is not None:
                feed_rate = pitch * tapSpeed
            else:
                # No spindle speed found, output pitch as F
                feed_rate = pitch

            new_command = Path.Command(command.Name)
            new_command.Parameters = command.Parameters
            if feed_rate:
                new_command.Parameters["F"] = feed_rate

            out += super().convert_command_to_gcode(new_command)
            out += "\n G80"

            return out

        return super().convert_command_to_gcode(command)

    def _expand_trailing_lines(self, postables):
        """Append post_job and postamble lines, to each section."""
        trailing = []
        if (lines := self.values["POST_JOB"]) is not None and lines != "":
            trailing.append(self._make_postable("Post: post_job", lines))
        if (lines := self.values["POSTAMBLE"]) is not None and lines != "":
            if self.values["END_SPINDLE_EMPTY"]:
                trailing.append(
                    self._make_postable("Post: postamble spindle empty", ["M05", "M6 T0\n"])
                )

            trailing.append(self._make_postable("Post: postamble", lines))

        if trailing:
            for _, section in postables:
                section.extend(trailing)

    def _convert_drill_cycle(self, command: Path.Command):
        """
        Converts a drill cycle command to gcode. Overridden to ensure drill cycles always include Q and R

        This method can be overridden by derived postprocessors to customize rapid move handling.
        """
        from Path.Post.UtilsParse import format_command_line

        # Extract command components
        command_name = command.Name
        params = command.Parameters
        annotations = command.Annotations

        # Check for blockdelete annotation
        block_delete_string = "/" if annotations.get("blockdelete") else ""  # FIXME: never set

        # Build command line
        command_line = []
        command_line.append(command_name)

        # Format parameters with clean, stateless implementation
        parameter_order = self.values.get(
            "PARAMETER_ORDER",
            # FIXME: dry
            ["X", "Y", "Z", "A", "B", "C", "F", "I", "J", "K", "R", "Q", "P", "S", "T"],
        )

        for parameter in parameter_order:
            if parameter in params:
                # Check if we should suppress duplicate parameters
                current_value = params[parameter]
                # cannot skip over parameters in drill cycles

                formatted_value = self.format_parameter(parameter, current_value)
                command_line.append(f"{parameter}{formatted_value}")

        # Suppress commands where all parameters were removed by duplicate suppression
        # or parameter_order exclusion (e.g., Z suppression for wire EDM).
        # A bare move (G0, G1, G2, G3) or dwell (G4) with no parameters is meaningless.
        if params and len(command_line) == 1:
            return None

        # Format the command line
        formatted_line = format_command_line(self.values, command_line)

        # Combine block delete and formatted command (no line numbers)
        gcode_string = f"{block_delete_string}{formatted_line}"

        return gcode_string

    def _expand_tool_length_offset(self, postables):
        """Inject or remove G43 tool length offset commands.

        When OUTPUT_TOOL_LENGTH_OFFSET is True, adds G43 commands after M6
        tool change commands in operations and tool change items.

        When OUTPUT_TOOL_LENGTH_OFFSET is False, removes any existing G43
        commands from operation paths.

        Simplified single-pass implementation.
        """
        output_tool_length_offset = self.values["OUTPUT_TOOL_LENGTH_OFFSET"]
        Path.Log.debug(f"OUTPUT_TOOL_LENGTH_OFFSET value: {output_tool_length_offset}")

        def edit(section_name, item, cmd, section_state):
            # suppress
            if not output_tool_length_offset:
                if cmd.Name in Constants.GCODE_TOOL_LENGTH_OFFSET:
                    return 0, [Path.Command(f"(TLO suppressed {cmd.toGCode()})")]
                else:
                    return None, None

            # add
            else:
                if cmd.Name in Constants.MCODE_TOOL_CHANGE and "T" in cmd.Parameters:
                    line = "G91 G0 G43 G54 Z-[#[2000+#4120]] H#4120 \n G90"
                    command = Path.Command("", {}, {Constants.ANNOT_AS_IS: line})
                    return 1, [command]
                else:
                    return None, None

        self._edit_command_list(postables, edit)

    @property
    def tooltip(self):

        tooltip = """
        Generate G-code from a Path that is compatible with the Fanuc CNC controller.
        Have a look at WEBSITEPATH
        """
        return tooltip

    @property
    def tooltipArgs(self):
        argtooltip = super().tooltipArgs

        # One could add additional arguments here.
        # argtooltip += '''
        # --arg1: This is the first argument
        # --arg2: This is the second argument

        # '''
        return argtooltip

    @property
    def units(self):
        return self._units
