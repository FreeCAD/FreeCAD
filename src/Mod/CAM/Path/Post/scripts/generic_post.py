# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2024 Ondsel <development@ondsel.com>                    *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

from typing import Any, Dict
from Path.Post.Processor import PostProcessor
import Path
import FreeCAD

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate

debug = False
if debug:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

Values = Dict[str, Any]


class Generic(PostProcessor):
    def __init__(self, job):
        super().__init__(
            job,
            tooltip=translate("CAM", "Generic post processor"),
            tooltipargs=[],
            units="Metric",
        )
        Path.Log.debug("Generic post processor initialized")

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        #
        super().init_values(values)
        values["POSTPROCESSOR_FILE_NAME"] = __name__
        values["MACHINE_NAME"] = "Generic"

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

    @property
    def tooltip(self):

        tooltip = """
        This is a generic post processor.
        It exposes functionality of the base post processor.
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
