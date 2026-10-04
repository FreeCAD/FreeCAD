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

"""
This post processor inherits from the base post processor class with the following changes:
- Adds a Use Alternative Tool Change parameter that lets the user decide to use M0 instead of M6 for tool changes
    for compatibility for some older machines
- Converts M7 to M10 for mist coolant on
- Converts M9 to M11 when mist coolant is being used
- Strips G21 commands
"""

from typing import Any
from Path.Post.Processor import PostProcessor
import Path
import FreeCAD
import Constants

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate

Values = dict[str, Any]

POST_TYPE = "machine"


class Estlcam(PostProcessor):
    coolant_End_Command = ""

    @classmethod
    def get_common_property_schema(cls):
        common_props = super().get_common_property_schema()

        for prop in common_props:
            name = prop["name"]
            if name == "file_extension":
                prop["default"] = "nc"
            if name == "ignored_commands":
                prop["default"] = "G21"
            elif name == "preamble":
                prop["default"] = ""
            elif name == "postamble":
                prop["default"] = "M05\n"
            elif name == "supported_commands":
                prop["default"] = (
                    "G0\nG00\nG1\nG01\nG2\nG02\nG3\nG03\nG4\nG04\nG54\nM0\nM00\nM1\nM01\nM3\nM03\nM4\nM04\nM5\nM05\nM6\nM06\nM7\nM07\nM8\nM08\nM9\nM09\nM10\nM11\nG98\nG80\nG81\nG82\nG83\nG84\nG85"
                )
        return common_props

    @classmethod
    def get_property_schema(cls):
        return [
            {
                "name": "TOOL_CHANGE_USE_ALTCMD",
                "type": "bool",
                "runtime": False,
                "label": translate("CAM", "Use Alternative Tool Change"),
                "default": False,
                "help": translate("CAM", "Use M0 instead of M6 for tool changes"),
            },
        ]

    def __init__(self, job):
        super().__init__(
            job=job,
            tooltip=translate("CAM", "Estlcam post processor"),
            tooltipargs=[],
            units="Metric",
        )
        Path.Log.debug("Estlcam post processor initialized")

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor"""
        super().init_values(values)

        values["POSTPROCESSOR_FILE_NAME"] = __name__
        values["MACHINE_NAME"] = "Estlcam"

        if self._machine and hasattr(self._machine, "postprocessor_properties"):
            props = self._machine.postprocessor_properties
            values["TOOL_CHANGE_USE_ALTCMD"] = props.get("TOOL_CHANGE_USE_ALTCMD", False)
        else:
            values["TOOL_CHANGE_USE_ALTCMD"] = False

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
        values["POSTAMBLE"] = """M5"""

    def convert_command_to_gcode(self, command: Path.Command):
        if command.Name in Constants.GCODE_UNITS_METRIC:
            return ""
        if command.Name in Constants.MCODE_COOLANT_MIST:
            self.coolant_End_Command = "M11"
            return "M10"
        if command.Name in Constants.MCODE_COOLANT_FLOOD:
            self.coolant_End_Command = "M9"
            return "M8"

        if command.Name in Constants.MCODE_COOLANT_OFF:  # expand M9 for flood, M11 for mist
            return self.coolant_End_Command
        if (
            command.Name in Constants.MCODE_TOOL_CHANGE
            and self._machine.postprocessor_properties.get("TOOL_CHANGE_USE_ALTCMD", False)
        ):
            gcode_return = super().convert_command_to_gcode(command)
            outstring = gcode_return.replace("M6", "M0")
            outstring = outstring.replace("M06", "M0")
            return outstring
        return super().convert_command_to_gcode(command)

    @property
    def tooltip(self):

        tooltip = """
        Generate G-code from a Path that is compatible with the Estlcam CNC controller.
        Have a look at https://www.estlcam.de/steuerung_cnc_programme_en.php
        This post processor inherits from the base post processor class with the following changes:
        - Adds a Use Alternative Tool Change parameter that lets the user decide to use M0 instead of M6 for tool changes for compatibility for some older machines
        - Converts M7 to M10 for mist coolant on
        - Converts M9 to M11 when mist coolant is being used
        - Strips G21 commands
        """
        return tooltip
