# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2018-2019 Gauthier Briere                               *
# *   Copyright (c) 2019-2020 Schildkroet                                   *
# *   Copyright (c) 2020 Gary L Hasson                                      *
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

Values = Dict[str, Any]

POST_TYPE = "machine"


class Marlin(PostProcessor):
    """Marlin 2.x post processor.

    Marlin firmware interprets G4 P as milliseconds and G4 S as seconds,
    which differs from standard G-code where G4 P is seconds.  This post
    processor converts all G4 dwell commands to use the S parameter so
    that dwell times are in seconds as expected by the CAM pipeline.

    All other behaviour is handled by the base PostProcessor class and
    machine configuration.
    """

    @classmethod
    def get_common_property_schema(cls):
        """Override common properties with Marlin-specific defaults."""
        common_props = super().get_common_property_schema()

        for prop in common_props:
            if prop["name"] == "file_extension":
                prop["default"] = "gcode"
            elif prop["name"] == "preamble":
                prop["default"] = "G90\nG21\nG17"
            elif prop["name"] == "postamble":
                prop["default"] = "M5"

        return common_props

    @classmethod
    def get_property_schema(cls):
        """Return schema for Marlin-specific configurable properties."""
        return []

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Marlin post processor"),
        tooltipargs=[],
        units="Metric",
    ) -> None:
        super().__init__(
            job=job,
            tooltip=tooltip,
            tooltipargs=tooltipargs,
            units=units,
        )
        Path.Log.debug("Marlin post processor initialized.")

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        super().init_values(values)

        values["MACHINE_NAME"] = "Marlin"
        values["POSTPROCESSOR_FILE_NAME"] = __name__

        # Marlin does not support tool changes
        values["OUTPUT_TOOL_CHANGE"] = False
        # Marlin does not support tool length offsets
        values["USE_TLO"] = False

    def _convert_dwell(self, command: Path.Command) -> str:
        """Convert G4 dwell commands, replacing P (seconds) with S (seconds).

        Marlin interprets G4 P as milliseconds and G4 S as seconds.
        The FreeCAD pipeline always creates G4 P<seconds>, so we swap
        the parameter letter to S for correct Marlin behaviour.
        """
        from Path.Post.UtilsParse import format_command_line

        params = command.Parameters
        annotations = command.Annotations
        block_delete_string = "/" if annotations.get("blockdelete") else ""

        command_line = [command.Name]

        # Swap P → S for the dwell time
        if "P" in params:
            precision = self.values.get("AXIS_PRECISION") or 3
            command_line.append(f"S{params['P']:.{precision}f}")
        elif "S" in params:
            # Already using S — pass through
            precision = self.values.get("AXIS_PRECISION") or 3
            command_line.append(f"S{params['S']:.{precision}f}")

        formatted_line = format_command_line(self.values, command_line)
        return f"{block_delete_string}{formatted_line}"
