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


class Test(PostProcessor):
    """The Test post processor class."""

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Test post processor"),
        tooltipargs=[""],
        units="Metric",
    ) -> None:
        super().__init__(
            job=job,
            tooltip=tooltip,
            tooltipargs=tooltipargs,
            units=units,
        )
        Path.Log.debug("Test post processor initialized")

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        #
        super().init_values(values)
        #
        # Set any values here that need to override the default values set
        # in the parent routine.
        #
        # Used in the argparser code as the "name" of the postprocessor program.
        #
        values["MACHINE_NAME"] = "test"
        #
        # Don't output comments by default.
        #
        values["OUTPUT_COMMENTS"] = False
        #
        # Don't output the header by default.
        #
        values["OUTPUT_HEADER"] = False
        #
        # Convert M56 tool change commands to comments,
        # which are then suppressed by default.
        #
        values["OUTPUT_TOOL_CHANGE"] = False
        values["POSTPROCESSOR_FILE_NAME"] = __name__
        #
        # Do not show the editor by default since we are testing.
        #
        values["SHOW_EDITOR"] = False
        #
        # Don't show the current machine units by default.
        #
        values["SHOW_MACHINE_UNITS"] = False
        #
        # Don't show the current operation label by default.
        #
        values["SHOW_OPERATION_LABELS"] = False
        #
        # Don't output an M5 command to stop the spindle after an M6 tool change by default.
        #
        values["STOP_SPINDLE_FOR_TOOL_CHANGE"] = False
        #
        # Don't output a G43 tool length command following tool changes by default.
        #
        values["USE_TLO"] = False

    def init_arguments_visible(self, arguments_visible: Visible) -> None:
        """Initialize which argument pairs are visible in TOOLTIP_ARGS."""
        super().init_arguments_visible(arguments_visible)
        #
        # Modify the visibility of any arguments from the defaults here.
        #
        # Make all arguments invisible by default.
        #
        for key in iter(arguments_visible):
            arguments_visible[key] = False

    @property
    def tooltip(self):
        tooltip: str = """
        This is a postprocessor file for the CAM workbench.  It is used
        to test the postprocessor code.  It probably isn't useful for "real" gcode.
        """
        return tooltip
