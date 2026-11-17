# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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
Fanuc post processor, "machine" based.

Targets the Fanuc 0i / 16i / 18i / 21i / 30i mill controls and the many
controls that run Fanuc-compatible G-code. The base PostProcessor already
writes the G-code these controls read; this post adds what is Fanuc's own:

- the ``%`` tape marks and an ``O`` program number;
- rigid tapping: ``M29 S<rpm>`` immediately before a ``G84``/``G74`` block,
  whose F is pitch times spindle speed;
- canned cycles kept native, every block carrying its full parameter set;
- the tool length offset on the first Z move after a tool change
  (``G0 G43 Z50.000 H1``): a ``G43 H1`` block with no axis word moves the
  axis by the offset on a Fanuc;
- dwell in milliseconds (``G4 P1500``, ``G82 P500``), the Fanuc default;
- an optional parametric tool-length offset using the ``#4120`` system
  variable, for shops that program that way;
- an optional empty-spindle park (``M6 T0``) before the postamble;
- upper-case output for older controls.

Tilted work planes are the ``G68.2`` / ``G53.1`` / ``G69`` family, which is
the base PostProcessor's default plane command. A machine that declares the
TWP rotation strategy gets its planes declared; one that declares DWO gets
the rotary move and the rotated path, and puts its dynamic fixture offset
(``G54.2 Pn``) in the post-rotary block.
"""

import re
from typing import Any

import FreeCAD
import Path

import Constants
from Path.Post.scripts.generic_multi_axis_industrial_post import GenericMultiAxisIndustrial

try:
    # Tilted work planes: the post names its plane command once the base
    # post-processor knows what one is.
    from Path.Post.TiltedWorkPlane import PlaneCommand
except ImportError:
    PlaneCommand = None

translate = FreeCAD.Qt.translate

DEBUG = False
if DEBUG:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

Values = dict[str, Any]

POST_TYPE = "machine"

RIGID_TAP_MODE = "M29"

# A tool length offset block keeps its move here, G0 or G1 and its Z, where
# the duplicate suppression that runs later cannot remove them.
ANNOT_TLO_MOVE = "fanuc_tlo_move"
ANNOT_TLO_Z = "fanuc_tlo_z"

EXTRA_SUPPORTED = ["G54.1"] + GenericMultiAxisIndustrial.EXTRA_SUPPORTED


class FanucPost(GenericMultiAxisIndustrial):
    """Post processor for Fanuc mill controls and Fanuc-compatible G-code."""

    ROTATION_STRATEGIES = ("dwo", "twp")
    if PlaneCommand is not None:
        PLANE_COMMAND = PlaneCommand.G68_2

    RIGID_TAP_MODE = RIGID_TAP_MODE
    EXTRA_SUPPORTED = EXTRA_SUPPORTED

    # ------------------------------------------------------------------
    # Property schema
    # ------------------------------------------------------------------

    @classmethod
    def get_common_property_schema(cls):
        """Fanuc defaults for the common properties."""
        common_props = super().get_common_property_schema()

        for prop in common_props:
            name = prop["name"]
            if name == "preamble":
                prop["default"] = "G17 G54 G40 G49 G80 G90 G94"
            elif name == "postamble":
                prop["default"] = "M05\nG17 G54 G90 G80 G40\nM30"
            elif name == "safetyblock":
                # The preamble establishes the safe state; a separate block would repeat it.
                prop["default"] = ""
            elif name == "pre_rotary_move":
                prop["help"] = translate(
                    "CAM",
                    "G-code inserted before the rotary axes move, and before a tilted work "
                    "plane is aligned to with G53.1. Put the moves that bring the tool clear "
                    "of the part here, in machine coordinates: G53 G0 Z0 lifts the tool to "
                    "machine Z zero; G91 G28 Z0 followed by G90 references it. Left empty, "
                    "nothing clears the part before the table turns, and the sanity check "
                    "says so.",
                )
            elif name == "post_rotary_move":
                prop["help"] = translate(
                    "CAM",
                    "G-code inserted after the rotary axes have moved. Under the DWO rotation "
                    "strategy this is where the control's dynamic fixture offset is selected: "
                    "G54.2 P1 on a Fanuc with that option. Under TWP the plane command "
                    "handles the rotation and this block is usually empty.",
                )
            elif name == "twp_align":
                prop["help"] = translate(
                    "CAM",
                    "The line that points the tool along a declared plane's normal. Empty is "
                    "G53.1, which positions the rotary axes for the plane declared by G68.2. "
                    "30i-class controls with the option accept G53.6 to keep the tool tip in "
                    "place while the rotaries move.",
                )

        return common_props

    @classmethod
    def get_property_schema(cls):
        """Fanuc-specific wording for the properties the industrial posts share."""
        props = super().get_property_schema()

        for prop in props:
            name = prop["name"]
            if name == "dwell_in_milliseconds":
                prop["help"] = translate(
                    "CAM",
                    "Write the P word of G4 and of the dwelling canned cycles as an integer "
                    "number of milliseconds, the Fanuc default (parameter DWL = 0). Off, P is "
                    "written in seconds with decimals, for a control set to read it that way.",
                )
            elif name == "rigid_tapping":
                prop["label"] = translate("CAM", "Rigid Tapping (M29)")
                prop["help"] = translate(
                    "CAM",
                    "Write M29 S<rpm> immediately before every G84 or G74 block, so the "
                    "control synchronises the spindle with the feed. Off, the cycle runs "
                    "with a floating tap holder. The feed is pitch times spindle speed in "
                    "either case.",
                )

        return props

    # ------------------------------------------------------------------
    # Setup
    # ------------------------------------------------------------------

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Fanuc post processor"),
        tooltipargs=[],
        units="Metric",
    ) -> None:
        super().__init__(job=job, tooltip=tooltip, tooltipargs=tooltipargs, units=units)
        Path.Log.debug("Fanuc post processor initialized.")

    def init_values(self, values: Values) -> None:
        super().init_values(values)
        values["MACHINE_NAME"] = "Fanuc"
        values["POSTPROCESSOR_FILE_NAME"] = __name__

    # ------------------------------------------------------------------
    # Tool length offset
    # ------------------------------------------------------------------

    def _expand_tool_change(self, postables):
        """Write the tool length offset on the first Z move after the tool change.

        The base post writes G43 H<n> alone after M6. A Fanuc reads a G43
        block with no axis word as G43 G91 Z0 H<n> and moves the axis by the
        offset: at the Z home position, where a tool change leaves the tool,
        that is an overtravel. Only a control set to the shift type of
        compensation does not move. So the offset goes on the move that
        brings the tool to the part, G0 G43 Z<clearance> H<n>.

        The G43 the base post added stays where it is when no straight move
        with a Z follows it before the next tool change, or when an arc or a
        canned cycle with a Z comes first: a block with no offset at all is
        the larger risk.
        """
        super()._expand_tool_change(postables)

        linear = Constants.GCODE_MOVE_LINE
        for section_name, sublist in postables:
            pending = None  # (command list, index) of the G43 not yet placed
            edited = []
            for item in sublist:
                if not item.path:
                    continue
                commands = list(item.path.Commands)
                edited.append((item, commands))
                index = 0
                while index < len(commands):
                    cmd = commands[index]
                    if cmd.Name in Constants.GCODE_TOOL_LENGTH_OFFSET and cmd.Annotations.get(
                        Constants.ANNOT_ADDED_TLO
                    ):
                        pending = (commands, index)
                    elif pending is not None and cmd.Name in Constants.MCODE_TOOL_CHANGE:
                        pending = None
                    elif (
                        pending is not None
                        and cmd.Name in Constants.GCODE_MOVE_ALL
                        and "Z" in cmd.Parameters
                    ):
                        if cmd.Name in linear:
                            offset_list, offset_index = pending
                            offset = offset_list[offset_index]
                            commands[index] = Path.Command(
                                cmd.Name,
                                {**cmd.Parameters, "H": offset.Parameters["H"]},
                                {
                                    **cmd.Annotations,
                                    Constants.ANNOT_ADDED_TLO: True,
                                    ANNOT_TLO_MOVE: cmd.Name,
                                    ANNOT_TLO_Z: cmd.Parameters["Z"],
                                },
                            )
                            del offset_list[offset_index]
                            if offset_list is commands:
                                index -= 1
                        else:
                            Path.Log.warning(
                                f"{item.label}: the first Z move after the tool change is "
                                f"{cmd.Name}, so G43 stays in a block of its own"
                            )
                        pending = None
                    index += 1

            for item, commands in edited:
                if len(commands) != len(item.path.Commands) or any(
                    ANNOT_TLO_MOVE in c.Annotations for c in commands
                ):
                    item.path = Path.Path(commands)

    # ------------------------------------------------------------------
    # Command conversion
    # ------------------------------------------------------------------

    def convert_command_to_gcode(self, command: Path.Command) -> str:
        """A tool length offset block gets its move and its Z back: the
        duplicate suppression may have removed either, and without the Z the
        block moves the axis by the offset."""
        move = command.Annotations.get(ANNOT_TLO_MOVE)
        if move:
            command = Path.Command(
                move,
                {**command.Parameters, "Z": command.Annotations[ANNOT_TLO_Z]},
                command.Annotations,
            )
        return super().convert_command_to_gcode(command)

    def _convert_move(self, command: Path.Command) -> str:
        """G0 G43 Z<clearance> H<n>: the offset block always carries its Z."""
        if ANNOT_TLO_MOVE not in command.Annotations:
            return super()._convert_move(command)

        doubles = self.values["OUTPUT_DOUBLES"]
        self.values["OUTPUT_DOUBLES"] = True
        try:
            line = super()._convert_move(command)
        finally:
            self.values["OUTPUT_DOUBLES"] = doubles

        name = command.Name
        offset = Constants.GCODE_TOOL_LENGTH_OFFSET[0]
        return re.sub(
            rf"{re.escape(name)}(?![\d.])",
            name + self.values["COMMAND_SPACE"] + offset,
            line,
            count=1,
        )

    @property
    def tooltip(self):
        return """
        Fanuc post processor for the CAM workbench.

        Writes G-code for Fanuc 0i/16i/18i/21i/30i mill controls and the
        controls that read Fanuc G-code: % tape marks and an O number,
        native canned cycles with every parameter on every block, rigid
        tapping with M29, dwell in milliseconds, tilted work planes with
        G68.2 / G53.1 / G69 or dynamic work offsets with the rotary move.
        """


# Class alias for PostProcessorFactory: it looks for the title-cased post name.
Fanuc = FanucPost


def create(job, **kwargs):
    """Factory function to create a Fanuc postprocessor instance."""
    return FanucPost(job, **kwargs)
