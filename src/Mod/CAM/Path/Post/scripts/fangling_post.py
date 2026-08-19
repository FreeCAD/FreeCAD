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


class Fangling(PostProcessor):
    last_command = ""

    @classmethod
    def get_common_property_schema(cls):
        """Return common properties with Fangling defaults (uses base defaults)"""
        """Override common properties with Fangling-specific defaults"""
        return super().get_common_property_schema()

    @classmethod
    def get_property_schema(cls):
        return [
            {
                "name": "DWELL_TIME",
                "type": "float",
                "runtime": True,
                "label": translate("CAM", "Pierce Dwell Time"),
                "default": 1.0,
                "min": 0.1,
                "max": 30.0,
                "decimals": 2,
                "help": translate(
                    "CAM",
                    "Time to wait after torch on for pierce to happen before cutting moves",
                ),
            },
            {
                "name": "STRIP_Z",
                "type": "bool",
                "label": translate("CAM", "Strip Z Parameters"),
                "default": True,
                "help": translate("CAM", "By default the machine does not support Z parameters"),
            },
            {
                "name": "STRIP_F",
                "type": "bool",
                "label": translate("CAM", "Strip F Parameters"),
                "default": True,
                "help": translate("CAM", "By default the machine does not support F parameters"),
            },
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
            tooltip=translate("CAM", "Fangling post processor"),
            tooltipargs=[],
            units="Metric",
        )
        Path.Log.debug("Fangling post processor initialized")

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor"""
        #
        super().init_values(values)

        values["POSTPROCESSOR_FILE_NAME"] = __name__
        values["MACHINE_NAME"] = "Fangling"

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
        values["POSTAMBLE"] = """"""

    def _convert_tool_change(self, command: Path.Command):
        return ""

    def _convert_coolant_command(self, command: Path.Command):
        return ""

    def convert_command_to_gcode(self, command: Path.Command):
        out = ""
        if (
            "X" not in command.Parameters
            and "Y" not in command.Parameters
            and command.Name in ["G0", "G00", "G1", "G2", "G3", "G01", "G02", "G03"]
        ):
            # No horizontal move detected so removed
            return ""
        if command.Name in ["G0", "G00"] and self.last_command in [
            "G1",
            "G2",
            "G3",
            "G01",
            "G02",
            "G03",
        ]:
            # A rapid move following a positioning move indicates the cut is complete
            # print ("end of cut")
            if self.values["OUTPUT_COMMENTS"]:
                out += "(Torch Off)\n"
            out += "M08\n"
            self.last_command = command.Name
        elif command.Name in ["G1", "G2", "G3", "G01", "G02", "G03"] and self.last_command in [
            "G0",
            "G00",
        ]:
            # A positioning move following a rapid move indicates a new cut is starting
            # print ("new cut")
            if self.values["OUTPUT_COMMENTS"]:
                out += "(Torch On)\n"
            out += "M07\n"
            if self.values["OUTPUT_COMMENTS"]:
                out += "(Pierce Delay)\n"
            out += "G4 P" + str(self.values["DWELL_TIME"]) + "\n"
            self.last_command = command.Name
        if command.Name in ["G54", "G55", "G56", "G57", "G58", "G59"]:
            # remove coordinate space info
            return ""
        if command.Name in ["G0", "G00", "G1", "G01", "G2", "G02", "G3", "G03"]:
            new_command = Path.Command(command.Name)
            d = command.Parameters
            new_command.Parameters = {
                k: v
                for k, v in d.items()
                if (k != "Z" or not self.values["STRIP_Z"])
                and (k != "F" or not self.values["STRIP_F"])
            }
            self.last_command = new_command.Name
            return out + super().convert_command_to_gcode(new_command)

        return out + super().convert_command_to_gcode(command)

    @property
    def tooltip(self):

        tooltip = """
        Generate G-code from a Path that is compatible with the Fangling CNC controller for
        Plasma Cutters.
        Have a look at https://www.flcnc.com/
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
