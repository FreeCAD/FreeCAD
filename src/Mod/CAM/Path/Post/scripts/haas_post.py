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
Haas post processor, "machine" based.

Targets Haas mills with the Next Generation Control. The generic multi axis
industrial post already writes most of what a Haas reads: the ``%`` tape
marks, native canned cycles, dwell as an integer number of milliseconds, and
``G43 H<n>``. This post adds what is Haas's own:

- a five-digit ``O`` program number;
- rigid tapping with ``S<rpm>`` alone ahead of the ``G84``/``G74`` block. A
  Haas has no rigid-tap mode word: ``M29`` is an output relay there;
- extended work offsets as ``G154 P<n>``; a work plane that names
  ``G54.1 P<n>`` is written that way. ``G59.1`` to ``G59.9`` are refused,
  a Haas does not have them;
- retracts in machine coordinates with ``G53 G0 Z0`` instead of ``G28``,
  which a Haas ignores dynamic work offsets for and advises against.

Indexed (3+2) work, by the machine's rotation strategy:

- DWO: ``G255``, the rotary move, then ``G254``, so the control applies its
  rotary zero point offsets (settings 255 to 257) to the rotated path.
  ``G254`` is cancelled again before the rotaries next move and at the end of
  the program.
- TWP: ``G268 X Y Z I J K Q123`` declares the plane, ``G253`` points the
  spindle along it, ``G269`` cancels. Haas has this on the UMC and the
  gimbal-head machines only, not on a mill with an added trunnion.

Not proven on a machine. In particular the ``G268`` angle convention is read
from the example in the Haas manual; see Path.Post.TiltedWorkPlane.
"""

from typing import Any

import FreeCAD
import Path

from Path.Post.CAMErrors import CAMValueError
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

# How a work plane or a Custom op spells an extended work offset, and how a
# Haas reads one.
EXTENDED_OFFSET_IN = "G54.1"
EXTENDED_OFFSET_OUT = "G154"
EXTENDED_OFFSET_MAX = 99

DWO_ON = "G254"
DWO_OFF = "G255"
DWO_ON_LABEL = "Post: dynamic work offset on"
DWO_OFF_LABEL = "Post: dynamic work offset off"

# Machine-coordinate retract. G53 is non-modal and ignores the tool length
# offset, a dynamic work offset and a feature coordinate system.
RETRACT = "G53 G0 Z0"

EXTRA_SUPPORTED = [
    EXTENDED_OFFSET_IN,
    EXTENDED_OFFSET_OUT,
] + GenericMultiAxisIndustrial.EXTRA_SUPPORTED

# LinuxCNC's extra work offsets. A Haas has G54-G59, then G154 P1-P99.
NOT_ON_A_HAAS = [f"G59.{n}" for n in range(1, 10)]


class HaasPost(GenericMultiAxisIndustrial):
    """Post processor for Haas mills with the Next Generation Control."""

    ROTATION_STRATEGIES = ("dwo", "twp")
    if PlaneCommand is not None:
        PLANE_COMMAND = PlaneCommand.G268

    # No mode word: the S block alone goes ahead of the tap.
    RIGID_TAP_MODE = ""
    EXTRA_SUPPORTED = EXTRA_SUPPORTED
    PROGRAM_NUMBER_DIGITS = 5

    # ------------------------------------------------------------------
    # Property schema
    # ------------------------------------------------------------------

    @classmethod
    def get_common_property_schema(cls):
        """Haas defaults for the common properties."""
        common_props = super().get_common_property_schema()

        for prop in common_props:
            name = prop["name"]
            if name == "preamble":
                prop["default"] = "G17 G40 G49 G80 G90 G94"
            elif name == "postamble":
                prop["default"] = f"M05\n{RETRACT}\nM30"
            elif name == "safetyblock":
                # The preamble establishes the safe state; a separate block would repeat it.
                prop["default"] = ""
            elif name == "pre_tool_change":
                prop["default"] = f"M05\n{RETRACT}"
            elif name == "supported_commands":
                prop["default"] = "\n".join(
                    c for c in prop["default"].split("\n") if c not in NOT_ON_A_HAAS
                )
            elif name == "pre_rotary_move":
                prop["default"] = RETRACT
                prop["help"] = translate(
                    "CAM",
                    "G-code inserted before the rotary axes move, before G255 and before a "
                    "feature coordinate system is aligned to with G253. Put the moves that "
                    "bring the tool clear of the part here, in machine coordinates: "
                    "G53 G0 Z0 lifts the tool to machine Z zero. Do not use G28 here; "
                    "the control ignores a dynamic work offset for it.",
                )
            elif name == "post_rotary_move":
                prop["help"] = translate(
                    "CAM",
                    "G-code inserted after the rotary axes have moved. Under the DWO rotation "
                    "strategy the post writes G254 itself, before this block. Usually empty.",
                )
            elif name == "twp_align":
                prop["help"] = translate(
                    "CAM",
                    "The line that points the spindle along a declared feature coordinate "
                    "system. Empty is G253, which moves the rotary axes for the plane "
                    "declared by G268.",
                )

        return common_props

    @classmethod
    def get_property_schema(cls):
        """Haas-specific wording and limits for the properties the industrial posts share."""
        props = super().get_property_schema()

        for prop in props:
            name = prop["name"]
            if name == "program_number":
                prop["max"] = 99999
                prop["help"] = translate(
                    "CAM",
                    "The O number written after the leading % mark, zero-padded to 5 "
                    "digits. 0 writes no O line. The control files the program under this "
                    "number when it is loaded. O09000 to O09999 are reserved by the control.",
                )
            elif name == "dwell_in_milliseconds":
                prop["help"] = translate(
                    "CAM",
                    "Write the P word of G4 and of the dwelling canned cycles as an integer "
                    "number of milliseconds. A Haas reads a P without a decimal point as "
                    "milliseconds and one with a decimal point as seconds. Off, P is written "
                    "in seconds with decimals.",
                )
            elif name == "rigid_tapping":
                prop["help"] = translate(
                    "CAM",
                    "Write S<rpm> in a block of its own immediately before every G84 or G74 "
                    "block, so the control synchronises the spindle with the feed. A Haas "
                    "has no rigid-tap mode word. Off, the speed is written on the cycle "
                    "block. The feed is pitch times spindle speed in either case.",
                )
            elif name == "end_spindle_empty_retract":
                prop["default"] = RETRACT

        return props

    # ------------------------------------------------------------------
    # Setup
    # ------------------------------------------------------------------

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Haas post processor"),
        tooltipargs=[],
        units="Metric",
    ) -> None:
        super().__init__(job=job, tooltip=tooltip, tooltipargs=tooltipargs, units=units)
        Path.Log.debug("Haas post processor initialized.")

    def init_values(self, values: Values) -> None:
        super().init_values(values)
        values["MACHINE_NAME"] = "Haas"
        values["POSTPROCESSOR_FILE_NAME"] = __name__

    # ------------------------------------------------------------------
    # Dynamic work offset
    # ------------------------------------------------------------------

    def _pose_change_postables(
        self, strategy, placement, positions, declared, rotaries_move, *args, **kwargs
    ):
        """Under DWO, G255 before the rotary move and G254 after it.

        The control wants G254 cancelled before any rotary motion, the work
        offset selected before G254, and the rotary motion complete before
        it. The base puts the fixture ahead of the rotary move, so the order
        holds. G255 is written every time, whether or not G254 is active: the
        post does not assume the control's state after a tool change. G254 is
        left out at the home pose, where there is nothing to offset.
        """
        from Machine.models.machine import RotationStrategy

        # The remaining arguments are the base's own and pass through untouched.
        items = super()._pose_change_postables(
            strategy, placement, positions, declared, rotaries_move, *args, **kwargs
        )
        if strategy != RotationStrategy.DWO or not rotaries_move:
            return items
        moves = [i for i, item in enumerate(items) if item.item_type == "rotation"]
        if not moves:
            return items
        first, last = moves[0], moves[-1]
        result = items[:first]
        result.append(self._make_postable(DWO_OFF_LABEL, DWO_OFF))
        result.extend(items[first : last + 1])
        if any(abs(angle) > 1e-9 for angle in (positions or {}).values()):
            result.append(self._make_postable(DWO_ON_LABEL, DWO_ON))
        result.extend(items[last + 1 :])
        return result

    def _expand_workplane_frames(self, postables):
        """Cancel a dynamic work offset that is still on when a section ends.

        M30 cancels it too, but the postamble comes first and is the user's:
        it must not run under an offset it does not know about.
        """
        result = super()._expand_workplane_frames(postables)
        for _, section in result:
            state = [item.label for item in section if item.label in (DWO_ON_LABEL, DWO_OFF_LABEL)]
            if state and state[-1] == DWO_ON_LABEL:
                section.append(self._make_postable(DWO_OFF_LABEL, DWO_OFF))
        return result

    # ------------------------------------------------------------------
    # Command conversion
    # ------------------------------------------------------------------

    def format_parameter(self, param_name, value, command_name=None):
        """On G154 the P word is the offset number, not a dwell."""
        if param_name == "P" and command_name == EXTENDED_OFFSET_OUT:
            return str(int(value))
        return super().format_parameter(param_name, value, command_name)

    def convert_command_to_gcode(self, command: Path.Command) -> str:
        """G54.1 P<n> is G154 P<n> on a Haas."""
        if command.Name in (EXTENDED_OFFSET_IN, EXTENDED_OFFSET_OUT):
            number = command.Parameters.get("P")
            if number is None or not 1 <= int(number) <= EXTENDED_OFFSET_MAX:
                raise CAMValueError(
                    translate(
                        "CAM",
                        "{cmd} needs an offset number P from 1 to {max}; a Haas has "
                        "G154 P1 to P{max}",
                    ).format(cmd=command.Name, max=EXTENDED_OFFSET_MAX),
                    job=self._job,
                    command=command,
                    pp=self.values["MACHINE_NAME"],
                )
            if command.Name == EXTENDED_OFFSET_IN:
                command = Path.Command(EXTENDED_OFFSET_OUT, command.Parameters, command.Annotations)
        return super().convert_command_to_gcode(command)

    # ------------------------------------------------------------------
    # Sanity checks
    # ------------------------------------------------------------------

    def sanity_check_methods(self):
        return super().sanity_check_methods() + [self._sanity_plane_needs_tool_length_offset]

    def _sanity_plane_needs_tool_length_offset(self, job):
        """G268 alarms unless G43 is active; say so when the machine writes none."""
        from Machine.models.machine import RotationStrategy

        if self._rotation_strategy() != RotationStrategy.TWP:
            return []
        machine = self._machine
        if machine.processing.tool_change and machine.output.output_tool_length_offset:
            return []
        return [
            self._create_squawk(
                "WARNING",
                translate(
                    "CAM",
                    "The machine declares the tilted work plane strategy, but the program "
                    "writes no tool length offset. A Haas needs G43 active before G268.",
                ),
            )
        ]

    @property
    def tooltip(self):
        return """
        Haas post processor for the CAM workbench.

        Writes G-code for Haas mills with the Next Generation Control:
        % tape marks and a 5-digit O number, native canned cycles with
        every parameter on every block, rigid tapping with S<rpm> ahead of
        the cycle, dwell in milliseconds, G154 extended work offsets,
        G53 retracts, and indexed work with dynamic work offsets
        (G254 / G255) or feature coordinate systems (G268 / G253 / G269).
        """


# Class alias for PostProcessorFactory: it looks for the title-cased post name.
Haas = HaasPost
