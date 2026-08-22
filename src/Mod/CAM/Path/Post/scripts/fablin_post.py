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

from typing import Any, Dict
from Path.Post.Processor import PostProcessor
import Path
import FreeCAD

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate

Values = Dict[str, Any]

POST_TYPE = "machine"


class Fablin(PostProcessor):
    coolant_End_Command = ""

    @classmethod
    def get_common_property_schema(cls):
        """Return common properties with Fablin defaults (uses base defaults)"""
        """Override common properties with Fablin-specific defaults"""
        common_props = super().get_common_property_schema()
        return common_props

    @classmethod
    def get_property_schema(cls):
        return [
            {
                "name": "min_feed_rate",
                "type": "float",
                "label": translate("CAM", "Minimum Feed Rate"),
                "default": 5.0,
                "min": 0.0,
                "max": 100000.0,
                "decimals": 2,
                "help": translate(
                    "CAM",
                    "Feed rates (in the current output units/min) below this value "
                    "abort posting - this usually indicates a missing feed rate",
                ),
            },
            {
                "name": "min_spindle_speed",
                "type": "float",
                "label": translate("CAM", "Minimum Spindle Speed"),
                "default": 5.0,
                "min": 0.0,
                "max": 20000.0,
                "decimals": 0,
                "help": translate(
                    "CAM",
                    "Spindle speeds (in RPM) below this value abort posting - this "
                    "usually indicates a missing spindle speed",
                ),
            },
        ]

    def __init__(self, job):
        super().__init__(
            job=job,
            tooltip=translate("CAM", "Fablin post processor"),
            tooltipargs=[],
            units="Metric",
        )
        Path.Log.debug("Fablin post processor initialized")

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor"""
        #
        super().init_values(values)

        values["POSTPROCESSOR_FILE_NAME"] = __name__
        values["MACHINE_NAME"] = "Fablin"

        if self._machine and hasattr(self._machine, "postprocessor_properties"):
            props = self._machine.postprocessor_properties
            values["MIN_FEED_RATE"] = props.get("min_feed_rate", 5.0)
            values["MIN_SPINDLE_SPEED"] = props.get("min_spindle_speed", 5.0)
        else:
            values["MIN_FEED_RATE"] = 5.0
            values["MIN_SPINDLE_SPEED"] = 5.0

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

        return

    def convert_command_to_gcode(self, command: Path.Command):
        if command.Name == "T":
            return
        return super().convert_command_to_gcode(command)

    def _convert_tool_change(self, command: Path.Command):  # ignore any tool change text
        return ""

    def _convert_fixture(self, command: Path.Command):  # ignore any G54-59 commands
        return ""

    @property
    def tooltip(self):

        tooltip = """
        Generate G-code from a Path that is compatible with the Fablin CNC controller.
        Have a look at https://github.com/FABtotum/FABlin
        """
        return tooltip

    @property
    def tooltipArgs(self):
        argtooltip = super().tooltipArgs

        # One could add additional arguments here.
        # argtooltip += """
        # --arg1: This is the first argument
        # --arg2: This is the second argument

        # """
        return argtooltip

    @property
    def units(self):
        return self._units
