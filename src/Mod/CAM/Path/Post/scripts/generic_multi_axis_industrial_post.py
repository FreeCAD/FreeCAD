# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Fae Corrigan <propsmonsterproductions@gmail.com>
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
out_tooltip = """
Generic Multi Axis Industrial Post, "machine" based.

Covers common code between several industrial cnc machines to reduce duplication. 
It is not intended to be used standalone

Features:
- the ``%`` tape marks and an ``O`` program number;

- canned cycles kept native, every block carrying its full parameter set;
- dwell in milliseconds (``G4 P1500``, ``G82 P500``),
- an optional parametric tool-length offset using the ``#4120`` system
  variable, for shops that program that way;
- an optional empty-spindle park (``M6 T0``) before the postamble;
- rigid tapping: ``S<rpm>`` immediately before a ``G84``/``G74`` block,
  whose F is pitch times spindle speed;
  Note: set RIGID_TAP_MODE at the top of the file if there is a specific command 
  for it on your machine.
  eg. RIGID_TAP_MODE ="M29" for Fanuc machines 
- upper-case output for older controls.

The post leaves DWO vs TWP ambigous to be set by the child post processor. 

"""
import re
from typing import Any

import FreeCAD
import Path

import Constants
import Path.Base.Util as PathUtil
from Path.Post.Processor import (
    PostProcessor,
    SCOPE_MACHINE,
    SCOPE_JOB,
    _HeaderBuilder,
    _sanitize_comment,
)
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
TAPE_MARK = "%"

# Beyond the base list: the program ends a Custom op may write. Child posts add their own.
EXTRA_SUPPORTED = Constants.MCODE_END + Constants.MCODE_END_RESET


class GenericMultiAxisIndustrial(PostProcessor):
    """Post processor for Generic Multi Axis Industrial Machines. 
    Not intended to be used standalone"""
    # Canned cycles whose P word is a dwell.
    DwellCycles = ("G82", "G88", "G89", "G84", "G74", "G76")

    # The mode command written before S<rpm> ahead of a rigid tap, "M29" on a
    # Fanuc. Empty when the control only wants the S word. Child posts set this.
    RIGID_TAP_MODE = ""

    # Commands added to the supported list beyond the base's. Child posts
    # extend this.
    EXTRA_SUPPORTED = EXTRA_SUPPORTED

    # ------------------------------------------------------------------
    # Property schema
    # ------------------------------------------------------------------

    @classmethod
    def get_common_property_schema(cls):
        """Defaults for the common properties that the industrial controls share."""
        common_props = super().get_common_property_schema()

        for prop in common_props:
            name = prop["name"]
            if name == "file_extension":
                prop["default"] = "nc"
            elif name == "pre_tool_change":
                prop["default"] = "M05\nG28 G91 Z0\nG90"
            elif name == "drill_cycles_to_translate":
                # Every cycle CAM emits is native on these controls.
                prop["default"] = ""
            elif name == "supports_tool_radius_compensation":
                prop["default"] = True
            elif name == "supported_commands":
                prop["default"] = "\n".join(prop["default"].split("\n") + cls.EXTRA_SUPPORTED)
            elif name == "spindle_decimals":
                # S takes an integer.
                prop["default"] = 0

        return common_props

    @classmethod
    def get_property_schema(cls):
        """Properties that the industrial controls share. Child posts adjust the
        help text, add their own properties, and may reorder the list."""
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
                    "number of milliseconds. Off, P is written in seconds with decimals, for "
                    "a control set to read it that way.",
                ),
            },
            {
                "name": "rigid_tapping",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Rigid Tapping"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write S<rpm> immediately before every G84 or G74 block, so the "
                    "control synchronises the spindle with the feed.\n"
                    "Off, the cycle runs with a floating tap holder.\n"
                    "The feed is pitch times spindle speed in either case",
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

    def _merge_machine_config(self):
        super()._merge_machine_config()
        # Comments are parenthesised, whatever the machine file says.
        self.values["COMMENT_SYMBOL"] = "("

    # ------------------------------------------------------------------
    # Postable expansion
    # ------------------------------------------------------------------

    @staticmethod
    def _as_is(text: str) -> Path.Command:
        return Path.Command("", {}, {Constants.ANNOT_AS_IS: text})

    def _expand_postprocessor_commands(self, postables):
        """Mark up the tapping and drilling cycles before anything else sees them.

        A rigid tap gets its RIGID_TAP_MODE S<rpm> block (just S<rpm> when the
        mode is empty), in its own command so it is line-numbered like any
        other. The speed is the cycle's own S or the last spindle speed
        commanded before it. A run of taps ends in G80, which the base
        terminator only writes for the drilling cycles; on these controls the
        tap is modal like any cycle and G80 is what drops rigid mode. Every
        canned cycle is made a modal barrier, so a run of holes keeps the cycle
        word and its full parameter set on every block: the control treats a
        coordinate-only block inside a cycle as another hole, but operators
        expect to read the cycle on each line, and a control set to non-modal
        cycles needs it.
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
                                self.RIGID_TAP_MODE,
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

    def _end_spindle_empty_block(self):
        """The lines that park an empty spindle: stop, retract, M6 T0."""
        retract = [
            line
            for line in (self.values.get("END_SPINDLE_EMPTY_RETRACT") or "").split("\n")
            if line.strip()
        ]
        return ["M05"] + retract + ["M6 T0"]

    def _expand_trailing_lines(self, postables) -> None:
        """Empty-spindle park before the postamble; the closing tape mark after it."""
        if self.values.get("END_SPINDLE_EMPTY", False):
            block = self._end_spindle_empty_block()
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

    def format_parameter(self, param_name, value, command_name=None):  # TODO
        """P is a dwell in whole milliseconds on these controls, unless the control
        is set otherwise; on G54.1 it is the extended WCS offset number."""
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

        A rigid tap's speed is on its own RIGID_TAP_MODE S<rpm> block; the cycle
        block does not repeat it.
        """
        command = self._tapping_to_speed(command)
        if (
            command.Name in Constants.GCODE_MOVE_TAP
            and self.values.get("RIGID_TAPPING", True)
            and "S" in command.Parameters
        ):
            params = {k: v for k, v in command.Parameters.items() if k != "S"}
            command = Path.Command(command.Name, params, command.Annotations)

        doubles = self.values["OUTPUT_DOUBLES"] #TODO why does this need to check doubles?
        self.values["OUTPUT_DOUBLES"] = True
        try:
            return self._convert_move(command)
        finally:
            self.values["OUTPUT_DOUBLES"] = doubles

    def _convert_generic_command(self, command: Path.Command) -> str:  # TODO
        """Rigid tapping S<rpm>: the S must be written even when it repeats the spindle's."""
        if command.Name == self.RIGID_TAP_MODE:
            words = []
            if (n := command.Parameters.get("N")) is not None:
                words.append(f"{self.values['LINE_NUMBER_PREFIX']}{int(n):d}")
            words.append(self.RIGID_TAP_MODE)
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
        mode = f" ({self.RIGID_TAP_MODE})" if self.RIGID_TAP_MODE else ""
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
                                    "rigid tapping{mode} needs one",
                                ).format(op=op.Label, cmd=command.Name, mode=mode),
                            )
                        )
                        break
        return squawks