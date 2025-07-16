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
Values = Dict[str, Any]
Visible = Dict[str, bool]


class Refactored_Centroid(PostProcessor):
    """The Refactored Centroid post processor class."""

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Refactored Centroid post processor"),
        tooltipargs=[""],
        units="Metric",
    ) -> None:
        super().__init__(
            job=job,
            tooltip=tooltip,
            tooltipargs=tooltipargs,
            units=units,
        )
        Path.Log.debug("Refactored Centroid post processor initialized.")

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        #
        super().init_values(values)
        #
        # Set any values here that need to override the default values set
        # in the parent routine.
        #
        # Use 4 digits for axis precision by default.
        #
        values["AXIS_PRECISION"] = 4
        values["DEFAULT_AXIS_PRECISION"] = 4
        values["DEFAULT_INCH_AXIS_PRECISION"] = 4
        #
        # Use ";" as the comment symbol
        #
        values["COMMENT_SYMBOL"] = ";"
        #
        # Use 1 digit for feed precision by default.
        #
        values["FEED_PRECISION"] = 1
        values["DEFAULT_FEED_PRECISION"] = 1
        values["DEFAULT_INCH_FEED_PRECISION"] = 1
        #
        # This value usually shows up in the post_op comment as "Finish operation:".
        # Change it to "End" to produce "End operation:".
        #
        values["FINISH_LABEL"] = "End"
        #
        # If this value is True, then a list of tool numbers
        # with their labels are output just before the preamble.
        #
        values["LIST_TOOLS_IN_PREAMBLE"] = True
        #
        # Used in the argparser code as the "name" of the postprocessor program.
        # This would normally show up in the usage message in the TOOLTIP_ARGS.
        #
        values["MACHINE_NAME"] = "Centroid"
        #
        # This list controls the order of parameters in a line during output.
        # centroid doesn't want K properties on XY plane; Arcs need work.
        #
        values["PARAMETER_ORDER"] = [
            "X",
            "Y",
            "Z",
            "A",
            "B",
            "I",
            "J",
            "F",
            "S",
            "T",
            "Q",
            "R",
            "L",
            "H",
        ]
        #
        # Any commands in this value will be output as the last commands
        # in the G-code file.
        #
        values["POSTAMBLE"] = """M99"""
        values["POSTPROCESSOR_FILE_NAME"] = __name__
        #
        # Any commands in this value will be output after the header and
        # safety block at the beginning of the G-code file.
        #
        values["PREAMBLE"] = """G53 G00 G17"""
        #
        # Output any messages.
        #
        values["REMOVE_MESSAGES"] = False
        #
        # Any commands in this value are output after the header but before the preamble,
        # then again after the TOOLRETURN but before the POSTAMBLE.
        #
        values["SAFETYBLOCK"] = """G90 G80 G40 G49"""
        #
        # Do not show the current machine units just before the PRE_OPERATION.
        #
        values["SHOW_MACHINE_UNITS"] = False
        #
        # Do not show the current operation label just before the PRE_OPERATION.
        #
        values["SHOW_OPERATION_LABELS"] = False
        #
        # Do not output an M5 command to stop the spindle for tool changes.
        #
        values["STOP_SPINDLE_FOR_TOOL_CHANGE"] = False
        #
        # spindle off, height offset canceled, spindle retracted
        # (M25 is a centroid command to retract spindle)
        #
        values[
            "TOOLRETURN"
        ] = """M5
M25
G49 H0"""
        #
        # Default to not outputting a G43 following tool changes
        #
        values["USE_TLO"] = False
        #
        # This was in the original centroid postprocessor file
        # but does not appear to be used anywhere.
        #
        # ZAXISRETURN = """G91 G28 X0 Z0 G90"""
        #

    def init_arguments_visible(self, arguments_visible: Visible) -> None:
        """Initialize which argument pairs are visible in TOOLTIP_ARGS."""
        super().init_arguments_visible(arguments_visible)
        #
        # Modify the visibility of any arguments from the defaults here.
        #
        arguments_visible["axis-modal"] = False
        arguments_visible["precision"] = False
        arguments_visible["tlo"] = False

    @property
    def tooltip(self):
        tooltip: str = """
        This is a postprocessor file for the CAM workbench.
        It is used to take a pseudo-gcode fragment from a CAM object
        and output 'real' GCode suitable for a centroid 3 axis mill.
        """
        return tooltip
