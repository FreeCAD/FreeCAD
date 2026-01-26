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


class Dumper(PostProcessor):
    def __init__(self, job):
        super().__init__(
            job,
            tooltip=translate("CAM", "Dumper post processor"),
            tooltipargs=[],
            units="Metric",
        )
        Path.Log.debug("Dumper post processor initialized")

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        #
        super().init_values(values)
        values["POSTPROCESSOR_FILE_NAME"] = __name__
        values["MACHINE_NAME"] = "Dumper"

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

    def export(self):
        """Export the postables list with formatted output."""
        print("=" * 80)
        print("POSTABLES LIST")
        print("=" * 80)
        print()
        print(f"Job: {self._job.Label}")
        print(f"SplitOutput: {self._job.SplitOutput}")
        print(f"OrderOutputBy: {self._job.OrderOutputBy}")
        print(f"Fixtures: {self._job.Fixtures}")
        print()

        postables = super()._buildPostList()

        for idx, postable in enumerate(postables, 1):
            group_key = postable[0]
            objects = postable[1]

            # Format the group key display
            if group_key == "":
                display_key = "(empty string)"
            else:
                display_key = f'"{group_key}"'

            print(f"[{idx}] Postable Group: {display_key}")
            print(f"    Objects: {len(objects)}")
            print()

            for obj_idx, obj in enumerate(objects, 1):
                print(f"    [{obj_idx}] {obj.Label}")

                # Determine object type/role
                obj_type = None
                if type(obj).__name__ == "_TempObject":
                    obj_type = "Fixture Setup"
                    if hasattr(obj, "Path") and obj.Path and len(obj.Path.Commands) > 0:
                        fixture_cmd = obj.Path.Commands[0]
                        print(f"        Fixture: {fixture_cmd.Name}")
                elif hasattr(obj, "TypeId"):
                    # Check if it's a tool controller
                    if "ToolController" in obj.TypeId or obj.Name.startswith("TC"):
                        obj_type = "Tool Change"
                        if hasattr(obj, "ToolNumber"):
                            print(f"        Tool Number: {obj.ToolNumber}")
                        if hasattr(obj, "Label"):
                            print(f"        Tool: {obj.Label}")
                    else:
                        # It's an operation
                        obj_type = "Operation"
                        if hasattr(obj, "ToolController") and obj.ToolController:
                            tc = obj.ToolController
                            print(f"        ToolController: {tc.Label} (T{tc.ToolNumber})")
                        elif hasattr(obj, "ToolController"):
                            print(f"        ToolController: None")
                else:
                    obj_type = type(obj).__name__

                print(f"        Type: {obj_type}")

            print()

        print("=" * 80)
        print(f"Total Groups: {len(postables)}")
        total_objects = sum(len(p[1]) for p in postables)
        print(f"Total Objects: {total_objects}")
        print("=" * 80)

    @property
    def tooltip(self):

        tooltip = """
        This is a Dumper post processor.
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
