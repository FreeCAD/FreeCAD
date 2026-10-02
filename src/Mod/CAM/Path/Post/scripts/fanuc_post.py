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
import Path.Base.Util as PathUtil
from Path.Post.Processor import PostProcessor, SCOPE_MACHINE, SCOPE_JOB
from Path.Post.CAMErrors import CAMValueError
from Path.Post.UtilsParse import format_command_line

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

# Fanuc has no K on a G17 arc; a helix carries its pitch in Z.
PARAMETER_ORDER = Constants.PARAMETER_ORDER.replace("K", "")

TAPE_MARK = "%"
RIGID_TAP_MODE = "M29"

# Beyond the base list: the plane select a preamble carries, the extended work
# offsets a work plane may name, and the program ends a Custom op may write.
EXTRA_SUPPORTED = ["G17", "G54.1"] + Constants.MCODE_END + Constants.MCODE_END_RESET


class FanucPost(PostProcessor):
    """Post processor for Fanuc mill controls and Fanuc-compatible G-code."""

    ROTATION_STRATEGIES = ("dwo", "twp")
    if PlaneCommand is not None:
        PLANE_COMMAND = PlaneCommand.G68_2

    # Fanuc canned cycles whose P word is a dwell.
    DwellCycles = ("G82", "G88", "G89", "G84", "G74", "G76")

    # ------------------------------------------------------------------
    # Property schema
    # ------------------------------------------------------------------

    @classmethod
    def get_common_property_schema(cls):
        """Fanuc defaults for the common properties."""
        common_props = super().get_common_property_schema()

        for prop in common_props:
            name = prop["name"]
            if name == "file_extension":
                prop["default"] = "nc"
            elif name == "preamble":
                prop["default"] = "G17 G54 G40 G49 G80 G90 G94"
            elif name == "postamble":
                prop["default"] = "M05\nG17 G54 G90 G80 G40\nM30"
            elif name == "safetyblock":
                # The preamble establishes the safe state; a separate block would repeat it.
                prop["default"] = ""
            elif name == "pre_tool_change":
                prop["default"] = "M05\nG28 G91 Z0\nG90"
            elif name == "drill_cycles_to_translate":
                # Every cycle CAM emits is native on a Fanuc.
                prop["default"] = ""
            elif name == "supports_tool_radius_compensation":
                prop["default"] = True
            elif name == "supported_commands":
                prop["default"] = "\n".join(prop["default"].split("\n") + EXTRA_SUPPORTED)
            elif name == "spindle_decimals":
                # S takes an integer.
                prop["default"] = 0
            elif name == "parameter_order":
                prop["default"] = PARAMETER_ORDER
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
        """Fanuc-specific properties."""
        return [
            {
                "name": "program_number",
                "scope": SCOPE_JOB,
                "type": "int",
                "label": translate("CAM", "Program Number"),
                "default": 0,
                "min": 0,
                "max": 99999999,
                "help": translate(
                    "CAM",
                    "The O number written after the leading % mark, zero-padded to four "
                    "digits. 0 writes no O line. The control files the program under this "
                    "number when it is loaded.",
                ),
            },
            {
                "name": "wrap_in_percent",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Tape Marks (%)"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write % as the first and last line, as tape-format DNC expects.",
                ),
            },
            {
                "name": "uppercase_output",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Upper-Case Output"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write the whole program in upper case, comments included. Older controls "
                    "read nothing else.",
                ),
            },
            {
                "name": "dwell_in_milliseconds",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Dwell in Milliseconds"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write the P word of G4 and of the dwelling canned cycles as an integer "
                    "number of milliseconds, the Fanuc default (parameter DWL = 0). Off, P is "
                    "written in seconds with decimals, for a control set to read it that way.",
                ),
            },
            {
                "name": "rigid_tapping",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Rigid Tapping (M29)"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write M29 S<rpm> immediately before every G84 or G74 block, so the "
                    "control synchronises the spindle with the feed. Off, the cycle runs "
                    "with a floating tap holder. The feed is pitch times spindle speed in "
                    "either case.",
                ),
            },
            {
                "name": "tool_length_offset_macro",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Parametric Tool Length Offset"),
                "default": False,
                "help": translate(
                    "CAM",
                    "After a tool change write G91 G0 G43 G54 Z-[#[2000+#4120]] H#4120 and "
                    "G90 instead of G43 H<n>. For shops whose offset table keeps the negated "
                    "length of tool n in #[2000+n]; #4120 is the active T code.",
                ),
            },
            {
                "name": "end_spindle_empty",
                "scope": SCOPE_JOB,
                "type": "bool",
                "label": translate("CAM", "Park Empty Spindle at End"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Before the postamble stop the spindle, retract, and write M6 T0 so the "
                    "last tool goes back to the magazine. Not every control accepts T0; "
                    "leave this off unless yours does.",
                ),
            },
            {
                "name": "end_spindle_empty_retract",
                "scope": SCOPE_MACHINE,
                "type": "text",
                "label": translate("CAM", "Empty-Spindle Retract"),
                "default": "G28 G91 Z0\nG90",
                "help": translate(
                    "CAM",
                    "The retract written between M05 and M6 T0 when the empty-spindle park "
                    "is on.",
                ),
            },
        ]

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

    def _merge_machine_config(self):
        super()._merge_machine_config()
        # Fanuc comments are parenthesised, whatever the machine file says.
        self.values["COMMENT_SYMBOL"] = "("

    # ------------------------------------------------------------------
    # Postable expansion
    # ------------------------------------------------------------------

    @staticmethod
    def _as_is(text: str) -> Path.Command:
        return Path.Command("", {}, {Constants.ANNOT_AS_IS: text})

    def _expand_postprocessor_commands(self, postables):
        """Mark up the tapping and drilling cycles before anything else sees them.

        A rigid tap gets its M29 S<rpm> block, in its own command so it is
        line-numbered like any other. The speed is the cycle's own S or the
        last spindle speed commanded before it. A run of taps ends in G80,
        which the base terminator only writes for the drilling cycles; on a
        Fanuc the tap is modal like any cycle and G80 is what drops rigid
        mode. Every canned cycle is made a modal barrier, so a run of holes
        keeps the cycle word and its full parameter set on every block: a
        Fanuc treats a coordinate-only block inside a cycle as another hole,
        but operators expect to read the cycle on each line, and a control set
        to non-modal cycles needs it.
        """
        rigid = self.values.get("RIGID_TAPPING", True)
        cycles = set(Constants.GCODE_MOVE_DRILL + Constants.GCODE_DRILL_EXTENDED)
        cancel = Constants.GCODE_CYCLE_CANCEL[0]

        for section_name, sublist in postables:
            spindle_speed = 0.0
            for item in sublist:
                if not item.path:
                    continue
                commands = []
                changed = False
                tapping = False
                for cmd in item.path.Commands:
                    if tapping and cmd.Name not in Constants.GCODE_MOVE_TAP:
                        if cmd.Name not in Constants.GCODE_CYCLE_CANCEL:
                            commands.append(Path.Command(cancel))
                            changed = True
                        tapping = False
                    if cmd.Name in Constants.MCODE_SPINDLE_ON and "S" in cmd.Parameters:
                        spindle_speed = float(cmd.Parameters["S"])
                    if cmd.Name in cycles:
                        cmd = Path.Command(
                            cmd.Name,
                            cmd.Parameters,
                            {**cmd.Annotations, Constants.ANNOT_MODAL_BARRIER: True},
                        )
                        changed = True
                    if rigid and cmd.Name in Constants.GCODE_MOVE_TAP:
                        speed = float(cmd.Parameters.get("S", 0) or 0) or spindle_speed
                        if speed <= 0:
                            raise CAMValueError(
                                translate(
                                    "CAM",
                                    "Rigid tapping needs a spindle speed: {op} taps with {cmd} "
                                    "and no S has been commanded",
                                ).format(op=item.label, cmd=cmd.Name),
                                job=self._job,
                                operation=item.source,
                                command=cmd,
                                pp=self.values["MACHINE_NAME"],
                            )
                        commands.append(
                            Path.Command(
                                RIGID_TAP_MODE,
                                {"S": speed},
                                {Constants.ANNOT_ALLOW_UNSUPPORTED: True},
                            )
                        )
                        changed = True
                        tapping = True
                    commands.append(cmd)
                if tapping:
                    commands.append(Path.Command(cancel))
                    changed = True
                if changed:
                    item.path = Path.Path(commands)

    def _expand_prefix(self, postables) -> None:
        """The tape mark and the O number come before everything, header included."""
        super()._expand_prefix(postables)

        leading = []
        if self.values.get("WRAP_IN_PERCENT", True):
            leading.append(self._as_is(TAPE_MARK))
        number = int(self.values.get("PROGRAM_NUMBER", 0) or 0)
        if number > 0:
            line = f"O{number:04d}"
            if self.values["OUTPUT_COMMENTS"] and self._job is not None:
                label = re.sub(r"[()]", "", getattr(self._job, "Label", "") or "")
                if label:
                    line += f" ({label})"
            leading.append(self._as_is(line))
        if not leading:
            return
        for _, section in postables:
            section.insert(0, self._make_postable("Post: tape start", leading))

    def _expand_trailing_lines(self, postables) -> None:
        """Empty-spindle park before the postamble; the closing tape mark after it."""
        if self.values.get("END_SPINDLE_EMPTY", False):
            retract = [
                line
                for line in (self.values.get("END_SPINDLE_EMPTY_RETRACT") or "").split("\n")
                if line.strip()
            ]
            block = ["M05"] + retract + ["M6 T0"]
            for _, section in postables:
                section.append(self._make_postable("Post: empty spindle", block))

        super()._expand_trailing_lines(postables)

        if self.values.get("WRAP_IN_PERCENT", True):
            for _, section in postables:
                section.append(self._make_postable("Post: tape end", [self._as_is(TAPE_MARK)]))

    def _expand_tool_length_offset_post_command(self, item, command):
        """G43 H<n>, or the macro form that reads the offset table itself."""
        if not self.values.get("TOOL_LENGTH_OFFSET_MACRO", False):
            return super()._expand_tool_length_offset_post_command(item, command)
        return [
            self._as_is("G91 G0 G43 G54 Z-[#[2000+#4120]] H#4120"),
            self._as_is("G90"),
        ]

    # ------------------------------------------------------------------
    # Command conversion
    # ------------------------------------------------------------------

    def format_parameter(self, param_name, value, command_name=None):
        """P is a dwell in whole milliseconds on a Fanuc, unless the control is
        set otherwise; on G54.1 it is the offset number."""
        if param_name == "P":
            if command_name == "G54.1":
                return str(int(value))
            if command_name in Constants.GCODE_P_IS_DWELL and self.values.get(
                "DWELL_IN_MILLISECONDS", True
            ):
                return str(round(float(value) * 1000))
        return super().format_parameter(param_name, value, command_name)

    def _convert_drill_cycle(self, command: Path.Command) -> str:
        """Every cycle block carries its full parameter set, whatever the
        duplicate setting: a hole is a hole, and a control set to non-modal
        cycles drills nothing from a block missing its Z or R.

        A rigid tap's speed is on its M29; the cycle block does not repeat it.
        """
        command = self._tapping_to_speed(command)
        if (
            command.Name in Constants.GCODE_MOVE_TAP
            and self.values.get("RIGID_TAPPING", True)
            and "S" in command.Parameters
        ):
            params = {k: v for k, v in command.Parameters.items() if k != "S"}
            command = Path.Command(command.Name, params, command.Annotations)

        doubles = self.values["OUTPUT_DOUBLES"]
        self.values["OUTPUT_DOUBLES"] = True
        try:
            return self._convert_move(command)
        finally:
            self.values["OUTPUT_DOUBLES"] = doubles

    def _convert_generic_command(self, command: Path.Command) -> str:
        """M29 S<rpm>: the S must be written even when it repeats the spindle's."""
        if command.Name == RIGID_TAP_MODE:
            words = []
            if (n := command.Parameters.get("N")) is not None:
                words.append(f"{self.values['LINE_NUMBER_PREFIX']}{int(n):d}")
            words.append(RIGID_TAP_MODE)
            words.append(f"S{self.format_parameter('S', command.Parameters['S'])}")
            return format_command_line(self.values, words)
        return super()._convert_generic_command(command)

    def _convert_job_sections(self, postables):
        sections = super()._convert_job_sections(postables)
        if not self.values.get("UPPERCASE_OUTPUT", True):
            return sections
        return [(name, gcode.upper()) for name, gcode in sections]

    # ------------------------------------------------------------------
    # Sanity checks
    # ------------------------------------------------------------------

    def sanity_check_methods(self):
        return super().sanity_check_methods() + [self._sanity_rigid_tapping]

    def _sanity_rigid_tapping(self, job):
        """A tap without a spindle speed cannot run rigid; the export refuses it,
        so say so where the operator reads the report."""
        if not self.values.get("RIGID_TAPPING", True):
            return []
        squawks = []
        for op in self._operations:
            if not PathUtil.activeForOp(op):
                continue
            path = getattr(op, "Path", None)
            if not path:
                continue
            spindle = 0.0
            for command in path.Commands:
                if command.Name in Constants.MCODE_SPINDLE_ON:
                    spindle = float(command.Parameters.get("S", 0) or 0)
                if command.Name in Constants.GCODE_MOVE_TAP:
                    speed = float(command.Parameters.get("S", 0) or 0) or spindle
                    tc = getattr(op, "ToolController", None)
                    if speed <= 0 and tc is not None:
                        speed = float(getattr(tc, "SpindleSpeed", 0) or 0)
                    if speed <= 0:
                        squawks.append(
                            self._create_squawk(
                                "WARNING",
                                translate(
                                    "CAM",
                                    "'{op}' taps with {cmd} but commands no spindle speed; "
                                    "rigid tapping (M29) needs one",
                                ).format(op=op.Label, cmd=command.Name),
                            )
                        )
                        break
        return squawks

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
