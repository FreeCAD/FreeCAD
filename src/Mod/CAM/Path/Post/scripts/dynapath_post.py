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
Dynapath post processor, "machine" based.

Targets the Autocon Dynapath Delta controls (Delta 20 through 60) in their
EIA/ISO mode, the dialect the two legacy Dynapath posts write. A Delta reads
G-code, but its own:

- a program is a sequence of events, one G or M code per block, with no
  spaces between words;
- the first line names the program, in parentheses, eight characters at most;
- a comment is a text event, ``(T)TEXT$``, the ``$`` closing the event;
- units are ``G70`` (inch) and ``G71`` (metric), not G20/G21;
- a fixture offset is an E code, ``E01`` for G54 through ``E06`` for G59;
- an arc carries the absolute coordinates of its center in I and J;
- a quill (canned) cycle takes its peck in K, its dwell in L seconds, and an
  optional second reference plane in O, the absolute Z the tool returns to
  between holes; the retract plane is R;
- a tapping cycle takes a feed rate, pitch times spindle speed, not a pitch;
- a dwell is ``L`` seconds;
- the control refuses a program whose first move after a tool change is not
  a full XYZ move, and it reads ``E`` on the last line as end of file when
  loading from external media.

The control does not read G98/G99: the retract between holes is R or O.
"""

import re
from typing import Any

import FreeCAD
import Path

import Constants
import Path.Base.Util as PathUtil
from Path.Post.Processor import PostProcessor, SCOPE_MACHINE, SCOPE_JOB
from Machine.models.machine import OutputUnits

translate = FreeCAD.Qt.translate

DEBUG = False
if DEBUG:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

Values = dict[str, Any]

POST_TYPE = "machine"

# Dynapath words in the order the legacy post wrote them. O is the second
# reference plane of a quill cycle; L is a dwell. K on an arc is never written.
PARAMETER_ORDER = "XYZABCUVWIJKFSTQORLPDH"

# G54 through G59 are E01 through E06; the extended offsets continue the series.
FIXTURE_CODES = {"G54": "E01", "G55": "E02", "G56": "E03", "G57": "E04", "G58": "E05", "G59": "E06"}
FIXTURE_CODES.update({f"G59.{n}": f"E{6 + n:02d}" for n in range(1, 10)})

UNITS_METRIC = "G71"
UNITS_INCH = "G70"
END_OF_FILE = "E"
PROGRAM_NAME_LENGTH = 8
SEQUENCE_NUMBER_DIGITS = 4
SEQUENCE_NUMBER_MAX = 9999

# The annotation that carries an operation's clearance height to its cycles.
ANNOT_SECOND_REFERENCE = "dynapath_o"

# Beyond the base list: the plane select the preamble carries, the unit words,
# and the program ends a Custom op may write.
EXTRA_SUPPORTED = (
    ["G17", UNITS_METRIC, UNITS_INCH] + Constants.MCODE_END + Constants.MCODE_END_RESET
)

CYCLES = tuple(Constants.GCODE_MOVE_DRILL + Constants.GCODE_DRILL_EXTENDED)


class DynapathPost(PostProcessor):
    """Post processor for the Dynapath Delta controls in EIA/ISO mode."""

    # ------------------------------------------------------------------
    # Property schema
    # ------------------------------------------------------------------

    @classmethod
    def get_common_property_schema(cls):
        """Dynapath defaults for the common properties."""
        common_props = super().get_common_property_schema()

        for prop in common_props:
            name = prop["name"]
            if name == "file_extension":
                prop["default"] = "ncc"
            elif name == "preamble":
                # One G code per event.
                prop["default"] = "G17\nG90\nG80\nG40"
            elif name == "postamble":
                prop["default"] = "M05\nG80\nG40\nG17\nG90\nM30"
            elif name == "safetyblock":
                prop["default"] = ""
            elif name == "pre_tool_change":
                prop["default"] = "M05"
            elif name == "drill_cycles_to_translate":
                # The Delta runs G81 through G85 natively; the chip-break cycle is
                # not documented for it. Translation happens only when the machine's
                # processing option asks for it.
                prop["default"] = "G73"
            elif name == "supports_tool_radius_compensation":
                prop["default"] = False
            elif name == "output_tool_length_offset":
                prop["default"] = False
                prop["help"] = translate(
                    "CAM",
                    "Write G43 H<tool> after every tool change. A Delta applies the tool "
                    "table's length with the T word; enable this only on a control set up "
                    "for separate T and H codes.",
                )
            elif name == "supported_commands":
                prop["default"] = "\n".join(prop["default"].split("\n") + EXTRA_SUPPORTED)
            elif name == "spindle_decimals":
                prop["default"] = 0
            elif name == "parameter_order":
                prop["default"] = PARAMETER_ORDER

        return common_props

    @classmethod
    def get_property_schema(cls):
        """Dynapath-specific properties."""
        return [
            {
                "name": "program_name",
                "scope": SCOPE_JOB,
                "type": "string",
                "label": translate("CAM", "Program Name"),
                "default": "",
                "help": translate(
                    "CAM",
                    "The name written in parentheses on the first line, which the control "
                    "files the program under. Eight characters at most, letters and digits; "
                    "empty takes the document's name.",
                ),
            },
            {
                "name": "uppercase_comments",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Upper-Case Comments"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write the program name and every text event in upper case, as the "
                    "control displays them.",
                ),
            },
            {
                "name": "absolute_arc_centers",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Absolute Arc Centers"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write I and J as the absolute coordinates of the arc center, which is "
                    "how a Delta reads them. Off, I and J are offsets from the start point.",
                ),
            },
            {
                "name": "second_reference_plane",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Second Reference Plane (O)"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write the operation's clearance height as the O word of every quill "
                    "cycle: the absolute Z the tool returns to before the rapid to the next "
                    "hole, so it clears a clamp the R plane would not. Off, the cycle "
                    "retracts to R between holes.",
                ),
            },
            {
                "name": "xyz_move_after_tool_change",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "XYZ Move After Tool Change"),
                "default": True,
                "help": translate(
                    "CAM",
                    "A Delta refuses a program whose first move after M6 does not command "
                    "all three axes. The first rapid with a Z after a tool change gets X0 Y0 "
                    "when it carries no XY of its own.",
                ),
            },
            {
                "name": "end_of_file_marker",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "End-of-File Marker (E)"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write E on the last line. The control reads it as end of file when "
                    "loading from external media and strips it from the stored program.",
                ),
            },
        ]

    # ------------------------------------------------------------------
    # Setup
    # ------------------------------------------------------------------

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Dynapath post processor"),
        tooltipargs=[],
        units="Metric",
    ) -> None:
        super().__init__(job=job, tooltip=tooltip, tooltipargs=tooltipargs, units=units)
        Path.Log.debug("Dynapath post processor initialized.")

    def init_values(self, values: Values) -> None:
        super().init_values(values)
        values["MACHINE_NAME"] = "Dynapath"
        values["POSTPROCESSOR_FILE_NAME"] = __name__

    def _merge_machine_config(self):
        super()._merge_machine_config()
        # A comment is a text event whatever the machine file says, and the
        # words of a block are not separated.
        self.values["COMMENT_SYMBOL"] = "("
        self.values["COMMAND_SPACE"] = ""
        # The machine's output option says G43; whether this control takes one is
        # the post's property.
        properties = getattr(self._machine, "postprocessor_properties", {}) or {}
        self.values["OUTPUT_TOOL_LENGTH_OFFSET"] = bool(
            properties.get("output_tool_length_offset", False)
        )

    # ------------------------------------------------------------------
    # Postable expansion
    # ------------------------------------------------------------------

    @staticmethod
    def _as_is(text: str) -> Path.Command:
        return Path.Command("", {}, {Constants.ANNOT_AS_IS: text})

    def _upper(self, text: str) -> str:
        return text.upper() if self.values.get("UPPERCASE_COMMENTS", True) else text

    def _program_name(self) -> str:
        """The name on the first line: the property, else the document's label,
        reduced to what the control files a program under."""
        name = (self.values.get("PROGRAM_NAME") or "").strip()
        if not name and self._job is not None:
            document = getattr(self._job, "Document", None)
            name = getattr(document, "Label", "") or ""
        name = re.sub(r"[^A-Za-z0-9]", "", name)[:PROGRAM_NAME_LENGTH]
        return self._upper(name)

    @staticmethod
    def _clearance_height(source):
        """The clearance height of the operation a posted item came from, in mm."""
        if source is None:
            return None
        for obj in (source, PathUtil.baseOp(source)):
            height = getattr(obj, "ClearanceHeight", None)
            if height is not None:
                return float(getattr(height, "Value", height))
        return None

    def _expand_postprocessor_commands(self, postables):
        """Mark up the cycles and the tool changes before anything else sees them.

        Every quill cycle is a modal barrier, so each hole carries its full
        parameter set, and carries its operation's clearance height for the O
        word. The first rapid with a Z after a tool change is completed to a
        full XYZ move.
        """
        second_plane = self.values.get("SECOND_REFERENCE_PLANE", True)
        xyz_after_change = self.values.get("XYZ_MOVE_AFTER_TOOL_CHANGE", True) and self.values.get(
            "TOOL_CHANGE", True
        )
        for section_name, sublist in postables:
            xyz_pending = False
            for item in sublist:
                if not item.path:
                    continue
                clearance = self._clearance_height(item.source) if second_plane else None
                commands = []
                changed = False
                for cmd in item.path.Commands:
                    if cmd.Name in CYCLES:
                        annotations = {**cmd.Annotations, Constants.ANNOT_MODAL_BARRIER: True}
                        if clearance is not None:
                            annotations[ANNOT_SECOND_REFERENCE] = clearance
                        cmd = Path.Command(cmd.Name, cmd.Parameters, annotations)
                        changed = True
                    elif cmd.Name in Constants.MCODE_TOOL_CHANGE:
                        xyz_pending = xyz_after_change
                    elif (
                        xyz_pending
                        and cmd.Name in Constants.GCODE_MOVE_RAPID
                        and "Z" in cmd.Parameters
                    ):
                        xyz_pending = False
                        params = dict(cmd.Parameters)
                        if "X" not in params and "Y" not in params:
                            params.update({"X": 0.0, "Y": 0.0})
                        cmd = Path.Command(
                            cmd.Name,
                            params,
                            {**cmd.Annotations, Constants.ANNOT_MODAL_BARRIER: True},
                        )
                        changed = True
                    commands.append(cmd)
                if changed:
                    item.path = Path.Path(commands)

    def _expand_prefix(self, postables) -> None:
        """The program name comes before everything, header included."""
        super()._expand_prefix(postables)
        name = self._program_name()
        if not name:
            return
        for _, section in postables:
            section.insert(0, self._make_postable("Post: program name", [self._as_is(f"({name})")]))

    def _collect_unit_command(self) -> list:
        """G71 or G70, in place of G21/G20."""
        if self.values["OUTPUT_UNITS"] == OutputUnits.IMPERIAL:
            word = UNITS_INCH
        else:
            word = UNITS_METRIC
        return Path.Command(word, {}, {Constants.ANNOT_ALLOW_UNSUPPORTED: True})

    def _expand_trailing_lines(self, postables) -> None:
        """The end-of-file marker is the last line, after the postamble."""
        super()._expand_trailing_lines(postables)
        if not self.values.get("END_OF_FILE_MARKER", True):
            return
        for _, section in postables:
            section.append(self._make_postable("Post: end of file", [self._as_is(END_OF_FILE)]))

    # ------------------------------------------------------------------
    # Command conversion
    # ------------------------------------------------------------------

    def _add_line_numbers(self, postables):
        """Every event carries a sequence number, the preamble and the text
        events included, so the numbers go on the finished text; see
        _convert_job_sections()."""
        return

    def _convert_job_sections(self, postables):
        """Number every line but the program name and the end-of-file marker,
        N zero-padded to four digits, a block delete slash before the N."""
        sections = super()._convert_job_sections(postables)
        if not self.values.get("OUTPUT_LINE_NUMBERS", False):
            return sections
        eol = self.values.get("END_OF_LINE_CHARS", "\n")
        prefix = self.values.get("LINE_NUMBER_PREFIX", "N")
        start = int(self.values.get("LINE_NUMBER_START", 1) or 1)
        increment = int(self.values.get("LINE_INCREMENT", 1) or 1)
        name_line = f"({self._program_name()})" if self._program_name() else None

        numbered_sections = []
        for section_name, gcode in sections:
            numbered = []
            number = start
            lines = gcode.split(eol)
            for index, line in enumerate(lines):
                if line.strip() == "":
                    continue
                if (index == 0 and line == name_line) or line == END_OF_FILE:
                    numbered.append(line)
                    continue
                word = f"{prefix}{number:0{SEQUENCE_NUMBER_DIGITS}d}"
                if line.startswith("/"):
                    numbered.append(f"/{word}{line[1:]}")
                else:
                    numbered.append(f"{word}{line}")
                number += increment
            numbered_sections.append((section_name, eol.join(numbered)))
        return numbered_sections

    def _convert_comment(self, command: Path.Command) -> str:
        """A text event: (T)TEXT$. A $ in the text would close the event early
        and the control would read the rest of the line as a new event, so $ and
        parentheses are removed: the control has no escape for them."""
        if not self.values["OUTPUT_COMMENTS"]:
            return None
        name = command.Name
        text = name[1:-1] if name.startswith("(") and name.endswith(")") else name[1:]
        text = re.sub(r"[()$]", "", text).strip()
        block_delete = "/" if command.Annotations.get("blockdelete") else ""
        return f"{block_delete}(T){self._upper(text)}$"

    def _convert_fixture(self, command: Path.Command) -> str:
        """G54 through G59 and the extended offsets are E codes."""
        code = FIXTURE_CODES.get(command.Name)
        if code is None:
            return super()._convert_fixture(command)
        return code

    def _convert_modal_command(self, command: Path.Command) -> str:
        """The control has no G98/G99; the retract between holes is R or O."""
        if command.Name in Constants.GCODE_RETURN_MODE:
            return None
        return super()._convert_modal_command(command)

    def _convert_dwell(self, command: Path.Command) -> str:
        """A dwell is L seconds, on its own, as the legacy post writes it."""
        seconds = command.Parameters.get("P")
        if seconds is None:
            return None
        return f"L{self._seconds(seconds)}"

    def _convert_arc_move(self, command: Path.Command) -> str:
        """I and J name the center absolutely; there is no K on a Delta arc."""
        params = {k: v for k, v in command.Parameters.items() if k != "K"}
        if self.values.get("ABSOLUTE_ARC_CENTERS", True):
            previous = self.machine_state.previous
            for word, axis in (("I", "X"), ("J", "Y")):
                if word in params:
                    start = previous.get(axis)
                    if start is None:
                        start = getattr(self.machine_state, axis, 0.0) or 0.0
                    params[word] = params[word] + start
        return self._convert_move(Path.Command(command.Name, params, command.Annotations))

    def _convert_drill_cycle(self, command: Path.Command) -> str:
        """A quill cycle in Dynapath words: K for the peck, L for the dwell, O
        for the second reference plane, a feed rate on a tap, every parameter
        on every hole."""
        command = self._tapping_to_speed(command)
        params = dict(command.Parameters)
        # FreeCAD's L is a repeat count the control has no word for.
        params.pop("L", None)
        if command.Name in Constants.GCODE_MOVE_TAP:
            # The spindle is already running at this speed.
            params.pop("S", None)
        if "Q" in params:
            params["K"] = params.pop("Q")
        if "P" in params:
            params["L"] = params.pop("P")
        second = command.Annotations.get(ANNOT_SECOND_REFERENCE)
        if second is not None and "O" not in params:
            params["O"] = float(second)

        doubles = self.values["OUTPUT_DOUBLES"]
        self.values["OUTPUT_DOUBLES"] = True
        try:
            return self._convert_move(Path.Command(command.Name, params, command.Annotations))
        finally:
            self.values["OUTPUT_DOUBLES"] = doubles

    @staticmethod
    def _seconds(value) -> str:
        text = f"{float(value):.3f}".rstrip("0").rstrip(".")
        return text if text else "0"

    def format_parameter(self, param_name, value, command_name=None):
        """O is a Z coordinate; L on a cycle is a dwell in seconds."""
        if param_name == "O":
            return super().format_parameter("Z", value, command_name)
        if param_name == "L" and command_name in CYCLES:
            return self._seconds(value)
        return super().format_parameter(param_name, value, command_name)

    # ------------------------------------------------------------------
    # Sanity checks
    # ------------------------------------------------------------------

    def sanity_check_methods(self):
        return super().sanity_check_methods() + [self._sanity_sequence_numbers]

    def _sanity_sequence_numbers(self, job):
        """The control's sequence numbers stop at N9999. A long program with
        numbering on runs past them; say so before the control does."""
        if not self.values.get("OUTPUT_LINE_NUMBERS", False):
            return []
        start = int(self.values.get("LINE_NUMBER_START", 1) or 1)
        increment = int(self.values.get("LINE_INCREMENT", 1) or 1)
        blocks = 0
        for op in self._operations:
            if not PathUtil.activeForOp(op):
                continue
            path = getattr(op, "Path", None)
            if path:
                blocks += sum(1 for c in path.Commands if not c.Name.startswith("("))
        last = start + increment * max(blocks - 1, 0)
        if last <= SEQUENCE_NUMBER_MAX:
            return []
        return [
            self._create_squawk(
                "WARNING",
                translate(
                    "CAM",
                    "Sequence numbers would reach N{last}; a Dynapath stops at N{max}. Turn "
                    "line numbers off, lower the increment, or split the output.",
                ).format(last=last, max=SEQUENCE_NUMBER_MAX),
            )
        ]

    @property
    def tooltip(self):
        return """
        Dynapath post processor for the CAM workbench.

        Writes EIA/ISO programs for the Dynapath Delta controls: a program
        name line, (T)...$ text events, G70/G71 units, E fixture codes,
        absolute arc centers, quill cycles with K, L and O words, tapping
        at pitch times spindle speed, an XYZ move after every tool change
        and the E end-of-file marker.
        """


# Class alias for PostProcessorFactory: it looks for the title-cased post name.
Dynapath = DynapathPost


def create(job, **kwargs):
    """Factory function to create a Dynapath postprocessor instance."""
    return DynapathPost(job, **kwargs)
