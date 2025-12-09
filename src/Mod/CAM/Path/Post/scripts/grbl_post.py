# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 - 2025 Larry Woestman <LarryWoestman2@gmail.com>   *
# *   Copyright (c) 2024 Ondsel <development@ondsel.com>                    *
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

import argparse

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

#
# Define some types that are used throughout this file.
#
Defaults = Dict[str, bool]
Values = Dict[str, Any]
Visible = Dict[str, bool]


class Grbl(PostProcessor):
    """The Grbl post processor class."""

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Grbl post processor"),
        tooltipargs=[""],
        units="Metric",
    ) -> None:
        super().__init__(
            job=job,
            tooltip=tooltip,
            tooltipargs=tooltipargs,
            units=units,
        )
        Path.Log.debug("Grbl post processor initialized.")

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
        # If this is set to True, then commands that are placed in
        # comments that look like (MC_RUN_COMMAND: blah) will be output.
        #
        values["ENABLE_MACHINE_SPECIFIC_COMMANDS"] = True
        #
        # Used in the argparser code as the "name" of the postprocessor program.
        # This would normally show up in the usage message in the TOOLTIP_ARGS.
        #
        values["MACHINE_NAME"] = "Grbl"
        #
        # Default to outputting Path labels at the beginning of each Path.
        #
        values["OUTPUT_PATH_LABELS"] = True
        #
        # Default to not outputting M6 tool changes (comment it) as grbl
        # currently does not handle it.
        #
        values["OUTPUT_TOOL_CHANGE"] = False
        #
        # The order of the parameters.
        # Arcs may only work on the XY plane (this needs to be verified).
        #
        values["PARAMETER_ORDER"] = [
            "X",
            "Y",
            "Z",
            "A",
            "B",
            "C",
            "U",
            "V",
            "W",
            "I",
            "J",
            "F",
            "S",
            "T",
            "Q",
            "R",
            "L",
            "P",
        ]
        #
        # Any commands in this value will be output as the last commands in the G-code file.
        #
        values[
            "POSTAMBLE"
        ] = """M5
G17 G90
M2"""
        values["POSTPROCESSOR_FILE_NAME"] = __name__
        #
        # Any commands in this value will be output after the header and
        # safety block at the beginning of the G-code file.
        #
        values["PREAMBLE"] = """G17 G90"""
        #
        # Do not show the current machine units just before the PRE_OPERATION.
        #
        values["SHOW_MACHINE_UNITS"] = False
        #
        # Default to not outputting a G43 following tool changes
        #
        values["USE_TLO"] = False

    def init_argument_defaults(self, argument_defaults: Defaults) -> None:
        """Initialize which arguments (in a pair) are shown as the default argument."""
        super().init_argument_defaults(argument_defaults)
        #
        # Modify which argument to show as the default in flag-type arguments here.
        # If the value is True, the first argument will be shown as the default.
        # If the value is False, the second argument will be shown as the default.
        #
        # For example, if you want to show Metric mode as the default, use:
        #   argument_defaults["metric_inch"] = True
        #
        # If you want to show that "Don't pop up editor for writing output" is
        # the default, use:
        #   argument_defaults["show-editor"] = False.
        #
        # Note:  You also need to modify the corresponding entries in the "values" hash
        #        to actually make the default value(s) change to match.
        #
        argument_defaults["tlo"] = False
        argument_defaults["tool_change"] = False

    def init_arguments_visible(self, arguments_visible: Visible) -> None:
        """Initialize which argument pairs are visible in TOOLTIP_ARGS."""
        super().init_arguments_visible(arguments_visible)
        #
        # Modify the visibility of any arguments from the defaults here.
        #
        arguments_visible["bcnc"] = True
        arguments_visible["axis-modal"] = False
        arguments_visible["return-to"] = True
        arguments_visible["tlo"] = False
        arguments_visible["tool_change"] = True
        arguments_visible["translate_drill"] = True
        arguments_visible["wait-for-spindle"] = True

    @property
    def tooltip(self):
        tooltip: str = """
        This is a postprocessor file for the CAM workbench.
        It is used to take a pseudo-gcode fragment from a CAM object
        and output 'real' GCode suitable for a Grbl 3 axis mill.
        """
        return tooltip
