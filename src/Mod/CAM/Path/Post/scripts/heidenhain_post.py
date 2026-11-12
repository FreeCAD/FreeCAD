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
Heidenhain Klartext (conversational) post processor, "machine" based.

Klartext is not G-code.  Every Path.Command is translated into a Klartext
block; nothing is passed through verbatim.  The dialect targeted is the one
shared by the TNC 4xx, iTNC 530, TNC 620/640 and TNC7 families:

    0 BEGIN PGM NAME MM
    1 BLK FORM 0.1 Z X+0 Y+0 Z-10
    2 BLK FORM 0.2 X+100 Y+100 Z+0
    3 TOOL CALL 1 Z S8000
    4 M3
    5 L X+0 Y+0 Z+5 R0 FMAX
    6 L Z-2 R0 F200
    7 CC X+50 Y+50
    8 C X+100 Y+50 DR- R0 F600
    9 CYCL DEF 200 DRILLING ~
        Q200=+2 ;SET-UP CLEARANCE ~
        ...
   10 L X+10 Y+10 R0 FMAX M99
   11 M5
   12 END PGM NAME MM

Block numbers are physical line numbers starting at 0 and are always
emitted; a line ending in "~" continues the block on the next line.

Legacy controls (TNC 355/4xx) that do not know the Q-parameter cycles can
be served with the "legacy" cycle format, which emits CYCL DEF 1.x
PECKING and CYCL DEF 17.x RIGID TAPPING.
"""

import math
import re
from typing import Any, Dict, List, Optional

import FreeCAD
import Path

import Constants
from Path.Post.Processor import PostProcessor, SCOPE_MACHINE, SCOPE_JOB
from Path.Post.CAMErrors import CAMValueError
from Machine.models.machine import OutputUnits

translate = FreeCAD.Qt.translate

DEBUG = False
if DEBUG:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

Values = Dict[str, Any]

POST_TYPE = "machine"

CYCLE_FORMAT_Q = "q_parameter"
CYCLE_FORMAT_LEGACY = "legacy"

FIXTURE_OUTPUT_PRESET = "preset"
FIXTURE_OUTPUT_DATUM = "datum"
FIXTURE_OUTPUT_NONE = "none"


class HeidenhainPost(PostProcessor):
    """Klartext post processor for Heidenhain TNC controls.

    Every conversion hook is overridden, because no Klartext block looks
    like the G-code it comes from.  Modal G-codes that have no Klartext
    equivalent (G17, G20/G21, G80, G90, G94, G97, G98/G99) are accepted and
    produce no output; G40/G41/G42 only update the RL/RR/R0 word that is
    written on every subsequent contouring block.
    """

    # Commands accepted from the Path.  Anything else raises "Unsupported command".
    GCodeSupported = (
        Constants.GCODE_MOVE_RAPID
        + Constants.GCODE_MOVE_STRAIGHT
        + Constants.GCODE_MOVE_ARC
        + Constants.GCODE_DWELL
        + ["G73", "G81", "G82", "G83", "G85"]
        + ["G74", "G84"]
        + Constants.GCODE_CYCLE_CANCEL
        + Constants.GCODE_ABSOLUTE
        + Constants.GCODE_FEED_UNITS_PER_MIN
        + Constants.GCODE_SPINDLE_RPM
        + Constants.GCODE_RETURN_MODE
        + Constants.GCODE_TOOL_LENGTH_OFFSET
        + Constants.GCODE_CUTTER_COMPENSATION
        + Constants.GCODE_UNITS
        + ["G17"]
        + Constants.GCODE_FIXTURES
        + Constants.MCODE_STOP
        + Constants.MCODE_OPTIONAL_STOP
        + Constants.MCODE_END
        + Constants.MCODE_END_RESET
        + Constants.MCODE_SPINDLE_ON
        + Constants.MCODE_SPINDLE_OFF
        + Constants.MCODE_TOOL_CHANGE
        + Constants.MCODE_COOLANT
    )

    # Accepted, tracked by MachineState or by this class, but never written.
    GCodeSilent = set(
        Constants.GCODE_CYCLE_CANCEL
        + Constants.GCODE_ABSOLUTE
        + Constants.GCODE_FEED_UNITS_PER_MIN
        + Constants.GCODE_SPINDLE_RPM
        + Constants.GCODE_RETURN_MODE
        + Constants.GCODE_TOOL_LENGTH_OFFSET
        + Constants.GCODE_UNITS
        + ["G17"]
    )

    # G54 -> 1, G55 -> 2, ... G59.9 -> 15 (before FIXTURE_FIRST_NUMBER is applied)
    FixtureIndex = {name: index + 1 for index, name in enumerate(Constants.GCODE_FIXTURES)}

    CompensationWord = {"G40": "R0", "G41": "RL", "G42": "RR"}

    # ------------------------------------------------------------------
    # Property schema
    # ------------------------------------------------------------------

    @classmethod
    def get_common_property_schema(cls):
        """Klartext-specific defaults for the common properties."""
        common_props = super().get_common_property_schema()

        for prop in common_props:
            if prop["name"] == "file_extension":
                prop["default"] = "h"
            elif prop["name"] == "supports_tool_radius_compensation":
                prop["default"] = True
            elif prop["name"] == "supported_commands":
                prop["default"] = "\n".join(cls.GCodeSupported)
            elif prop["name"] == "drill_cycles_to_translate":
                # Every drill cycle CAM emits has a native Klartext cycle.
                prop["default"] = ""
            elif prop["name"] == "output_tool_length_offset":
                # TOOL CALL applies the length from the tool table; there is no G43.
                prop["default"] = False
            elif prop["name"] == "feed_precision":
                prop["default"] = 0
            elif prop["name"] == "spindle_decimals":
                prop["default"] = 0
            elif prop["name"] == "axis_precision":
                prop["default"] = 3

        return common_props

    @classmethod
    def get_property_schema(cls):
        """Heidenhain-specific properties."""
        return [
            {
                "name": "program_name",
                "scope": SCOPE_JOB,
                "type": "string",
                "label": translate("CAM", "Program Name"),
                "default": "",
                "help": translate(
                    "CAM",
                    "Name written in the BEGIN PGM and END PGM blocks. "
                    "Leave empty to derive it from the job label. "
                    "Characters other than letters, digits and underscore are replaced.",
                ),
            },
            {
                "name": "tool_axis",
                "scope": SCOPE_MACHINE,
                "type": "choice",
                "label": translate("CAM", "Tool Axis"),
                "choices": ["X", "Y", "Z"],
                "default": "Z",
                "help": translate(
                    "CAM",
                    "Spindle axis written in TOOL CALL and BLK FORM. "
                    "Arcs are always output in the XY plane.",
                ),
            },
            {
                "name": "rapid_feed_rate",
                "scope": SCOPE_MACHINE,
                "type": "float",
                "label": translate("CAM", "Rapid Feed Rate (0 = FMAX)"),
                "default": 0.0,
                "min": 0.0,
                "max": 999999.0,
                "decimals": 0,
                "help": translate(
                    "CAM",
                    "Feed rate in mm/min written on rapid blocks instead of FMAX. "
                    "Older controls do not accept FMAX. 0 writes FMAX.",
                ),
            },
            {
                "name": "cycle_format",
                "scope": SCOPE_MACHINE,
                "type": "choice",
                "label": translate("CAM", "Drill Cycle Format"),
                "choices": [CYCLE_FORMAT_Q, CYCLE_FORMAT_LEGACY],
                "default": CYCLE_FORMAT_Q,
                "help": translate(
                    "CAM",
                    "q_parameter: CYCL DEF 200/202/203/207 with Q parameters "
                    "(TNC 4xx and newer). "
                    "legacy: CYCL DEF 1.x PECKING and 17.x RIGID TAPPING "
                    "for controls without Q-parameter cycles.",
                ),
            },
            {
                "name": "drill_setup_clearance",
                "scope": SCOPE_MACHINE,
                "type": "float",
                "label": translate("CAM", "Drill Cycle Set-Up Clearance"),
                "default": 2.0,
                "min": 0.0,
                "max": 1000.0,
                "decimals": 3,
                "help": translate(
                    "CAM",
                    "Set-up clearance (Q200) in mm. The cycle's surface coordinate is "
                    "placed this far below the operation's retract height so that "
                    "feed motion starts at the retract height, as in the G-code cycle.",
                ),
            },
            {
                "name": "chip_break_retract",
                "scope": SCOPE_MACHINE,
                "type": "float",
                "label": translate("CAM", "Chip Breaking Retract"),
                "default": 0.5,
                "min": 0.0,
                "max": 100.0,
                "decimals": 3,
                "help": translate(
                    "CAM",
                    "Retraction distance (Q256) in mm used for G73 chip breaking "
                    "in CYCL DEF 203.",
                ),
            },
            {
                "name": "fixture_output",
                "scope": SCOPE_MACHINE,
                "type": "choice",
                "label": translate("CAM", "Fixture Output"),
                "choices": [FIXTURE_OUTPUT_PRESET, FIXTURE_OUTPUT_DATUM, FIXTURE_OUTPUT_NONE],
                "default": FIXTURE_OUTPUT_PRESET,
                "help": translate(
                    "CAM",
                    "preset: CYCL DEF 247 PRESETTING selects a preset table entry. "
                    "datum: CYCL DEF 7 DATUM SHIFT selects a datum table entry. "
                    "none: fixture commands are not output.",
                ),
            },
            {
                "name": "fixture_first_number",
                "scope": SCOPE_MACHINE,
                "type": "int",
                "label": translate("CAM", "Fixture Number for G54"),
                "default": 1,
                "min": 0,
                "max": 9999,
                "help": translate(
                    "CAM",
                    "Preset or datum number selected for G54. G55 selects the next one, and so on.",
                ),
            },
            {
                "name": "output_blk_form",
                "scope": SCOPE_JOB,
                "type": "bool",
                "label": translate("CAM", "Output BLK FORM"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write the stock bounding box as BLK FORM 0.1 / 0.2 blocks "
                    "after BEGIN PGM for graphic simulation on the control.",
                ),
            },
        ]

    # ------------------------------------------------------------------
    # Setup
    # ------------------------------------------------------------------

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Heidenhain Klartext post processor"),
        tooltipargs=[],
        units="Metric",
    ) -> None:
        super().__init__(job=job, tooltip=tooltip, tooltipargs=tooltipargs, units=units)
        self._reset_section_state()
        Path.Log.debug("Heidenhain post processor initialized.")

    def init_values(self, values: Values) -> None:
        super().init_values(values)
        values["MACHINE_NAME"] = "Heidenhain"
        values["POSTPROCESSOR_FILE_NAME"] = __name__

    def _merge_machine_config(self):
        """Force the values Klartext cannot do without."""
        super()._merge_machine_config()

        # Klartext comments are ";" blocks.
        self.values["COMMENT_SYMBOL"] = ";"

        # The block word (L, C, ...) carries the motion type, so a name-stripped
        # command could not be converted.
        self.values["OUTPUT_DUPLICATE_COMMANDS"] = True

        # Block numbers are physical line numbers, added in _convert_job_sections().
        self.values["OUTPUT_LINE_NUMBERS"] = False

        schema = {x["name"]: x for x in self.get_common_property_schema()}
        for property_name in ("supported_commands", "output_tool_length_offset"):
            self.values[property_name.upper()] = schema[property_name]["default"]

    def _reset_section_state(self):
        """State that is only valid within one output file."""
        self._compensation = "R0"
        self._last_cc = None
        self._last_cycle_def = None
        self._tool_call_speed = None

    def _convert_start_section(self, section_name, sublist):
        self._reset_section_state()
        super()._convert_start_section(section_name, sublist)

    # ------------------------------------------------------------------
    # Postable expansion
    # ------------------------------------------------------------------

    def _expand_postprocessor_commands(self, postables):
        """Carry the spindle speed of the M3/M4 following an M6 onto the M6.

        TOOL CALL takes the spindle speed, so the tool change needs to know it
        before the spindle command is converted.  With tool changes suppressed
        the M6 becomes a comment and the M3 alone sets the speed.
        """
        if not self.values.get("TOOL_CHANGE", True):
            return
        for section_name, sublist in postables:
            for item in sublist:
                if not item.path:
                    continue
                commands = list(item.path.Commands)
                changed = False
                for index, cmd in enumerate(commands):
                    if cmd.Name not in Constants.MCODE_TOOL_CHANGE or "S" in cmd.Parameters:
                        continue
                    for later in commands[index + 1 :]:
                        if later.Name in Constants.MCODE_TOOL_CHANGE:
                            break
                        if later.Name in Constants.MCODE_SPINDLE_ON and "S" in later.Parameters:
                            params = dict(cmd.Parameters)
                            params["S"] = later.Parameters["S"]
                            commands[index] = Path.Command(cmd.Name, params, cmd.Annotations)
                            changed = True
                            break
                if changed:
                    item.path = Path.Path(commands)

    def _expand_prefix(self, postables) -> None:
        """BEGIN PGM must be block 0, ahead of the header comments."""
        super()._expand_prefix(postables)

        commands = [self._as_is(f"BEGIN PGM {self._program_name()} {self._units_word()}")]
        commands.extend(self._blk_form_commands())

        for _, section in postables:
            section.insert(0, self._make_postable("Post: begin pgm", commands))

    def _expand_trailing_lines(self, postables) -> None:
        """END PGM must be the last block, after the postamble."""
        super()._expand_trailing_lines(postables)

        end = self._as_is(f"END PGM {self._program_name()} {self._units_word()}")
        for _, section in postables:
            section.append(self._make_postable("Post: end pgm", [end]))

    def _collect_unit_command(self) -> list:
        """Units are declared in BEGIN PGM; there is no G20/G21."""
        return None

    def _add_line_numbers(self, postables):
        """Klartext block numbers are added on the finished text, see _convert_job_sections()."""
        return

    def _convert_job_sections(self, postables):
        """Number every block from 0, leaving "~" continuation lines unnumbered."""
        sections = super()._convert_job_sections(postables)
        eol = self.values.get("END_OF_LINE_CHARS", "\n")

        numbered_sections = []
        for section_name, gcode in sections:
            numbered = []
            block_number = 0
            continuation = False
            for line in gcode.split(eol):
                if line.strip() == "":
                    continue
                if continuation:
                    numbered.append(line)
                else:
                    numbered.append(f"{block_number} {line}")
                    block_number += 1
                continuation = line.rstrip().endswith("~")
            numbered_sections.append((section_name, eol.join(numbered)))

        return numbered_sections

    # ------------------------------------------------------------------
    # Formatting helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _as_is(text: str) -> Path.Command:
        return Path.Command("", {}, {Constants.ANNOT_AS_IS: text})

    def _program_name(self) -> str:
        name = self.values.get("PROGRAM_NAME", "") or ""
        if not name and self._job is not None:
            name = getattr(self._job, "Label", "") or ""
        name = re.sub(r"[^A-Za-z0-9]+", "_", name).strip("_").upper()
        return name or "FREECAD"

    def _units_word(self) -> str:
        return "INCH" if self.values["OUTPUT_UNITS"] == OutputUnits.IMPERIAL else "MM"

    def _tool_axis(self) -> str:
        return self.values.get("TOOL_AXIS", "Z") or "Z"

    @staticmethod
    def _signed(text: str) -> str:
        """Klartext writes an explicit sign on every coordinate."""
        if text.startswith("-"):
            if float(text) == 0.0:
                return "+" + text[1:]
            return text
        if text.startswith("+"):
            return text
        return "+" + text

    def _axis(self, letter: str, value: float) -> str:
        """A signed axis word, e.g. X+10.000.  Rotary axes are degrees, never converted."""
        if letter in ("A", "B", "C"):
            text = f"{value:.{self.values['AXIS_PRECISION']}f}"
        else:
            text = self.format_parameter(letter, value)
        return f"{letter}{self._signed(text)}"

    def _length(self, value: float) -> str:
        """A signed length in output units, without an axis letter."""
        return self._signed(self.format_parameter("X", value))

    def _feed_value(self, feed_mm_per_sec: float) -> str:
        return self.format_parameter("F", feed_mm_per_sec)

    def _rapid_word(self) -> str:
        rate = self.values.get("RAPID_FEED_RATE", 0) or 0
        if rate > 0:
            if self.values["OUTPUT_UNITS"] == OutputUnits.IMPERIAL:
                rate = rate / 25.4
            return f"F{rate:.{self.values['FEED_PRECISION']}f}"
        return "FMAX"

    def _feed_word(self, command: Path.Command) -> Optional[str]:
        """F word for a feed block: the command's F, or the last programmed F."""
        feed = command.Parameters.get("F")
        if feed is None and self.machine_state is not None:
            feed = self.machine_state.F
        if not feed:
            return None
        return f"F{self._feed_value(feed)}"

    def _block_delete(self, command: Path.Command) -> str:
        return "/ " if command.Annotations.get("blockdelete") else ""

    def _error(self, message: str, command: Path.Command) -> CAMValueError:
        return CAMValueError(
            message,
            job=self._job,
            operation=self._operation,
            command=command,
            pp=self.values["MACHINE_NAME"],
        )

    def _current_position(self, command: Path.Command, letter: str, previous=False):
        """Absolute coordinate after (or before) the command, from the tracked state."""
        if previous:
            return self.machine_state.previous.get(letter)
        return getattr(self.machine_state, letter)

    # ------------------------------------------------------------------
    # BLK FORM
    # ------------------------------------------------------------------

    def _blk_form_commands(self) -> List[Path.Command]:
        if not self.values.get("OUTPUT_BLK_FORM", False):
            return []
        try:
            box = self._job.Stock.Shape.BoundBox
            corner_min = (box.XMin, box.YMin, box.ZMin)
            corner_max = (box.XMax, box.YMax, box.ZMax)
        except AttributeError:
            Path.Log.debug("No stock bounding box; BLK FORM not written")
            return []

        def corner(values):
            return " ".join(self._axis(letter, value) for letter, value in zip("XYZ", values))

        return [
            self._as_is(f"BLK FORM 0.1 {self._tool_axis()} {corner(corner_min)}"),
            self._as_is(f"BLK FORM 0.2 {corner(corner_max)}"),
        ]

    # ------------------------------------------------------------------
    # Linear motion: L blocks
    # ------------------------------------------------------------------

    def _line_block(self, command: Path.Command, rapid: bool) -> Optional[str]:
        params = command.Parameters
        words = [self._axis(letter, params[letter]) for letter in "XYZABC" if letter in params]
        if not words:
            # every coordinate suppressed as a duplicate: nothing to do
            return None

        words.append(self._compensation)
        feed = self._rapid_word() if rapid else self._feed_word(command)
        if feed:
            words.append(feed)

        return f"{self._block_delete(command)}L {' '.join(words)}"

    def _convert_rapid_move(self, command: Path.Command) -> str:
        return self._line_block(command, rapid=True)

    def _convert_linear_move(self, command: Path.Command) -> str:
        return self._line_block(command, rapid=False)

    # ------------------------------------------------------------------
    # Circular motion: CC / C / CR / CP blocks
    # ------------------------------------------------------------------

    def _convert_arc_move(self, command: Path.Command) -> str:
        params = command.Parameters
        clockwise = command.Name in Constants.GCODE_MOVE_CW
        direction = "DR-" if clockwise else "DR+"

        start = tuple(self._current_position(command, letter, previous=True) for letter in "XYZ")
        end = tuple(self._current_position(command, letter) for letter in "XYZ")
        if any(value is None for value in start + end):
            raise self._error(
                "Arc requires a known start position; program a move before the arc", command
            )

        trailing = [direction, self._compensation]
        if feed := self._feed_word(command):
            trailing.append(feed)
        prefix = self._block_delete(command)

        # Radius format: CR block, sign of R selects the minor or major arc.
        if "R" in params and "I" not in params and "J" not in params:
            radius = params["R"]
            if radius == 0:
                raise self._error("Arc radius must not be zero", command)
            radius_word = f"R{self._signed(self.format_parameter('R', radius))}"
            words = [self._axis("X", end[0]), self._axis("Y", end[1]), radius_word] + trailing
            return f"{prefix}CR {' '.join(words)}"

        if "I" not in params and "J" not in params:
            raise self._error("Arc requires I and J (or R)", command)

        center = (start[0] + params.get("I", 0.0), start[1] + params.get("J", 0.0))
        lines = []
        if self._last_cc is None or any(abs(a - b) > 1e-9 for a, b in zip(center, self._last_cc)):
            lines.append(f"{prefix}CC {self._axis('X', center[0])} {self._axis('Y', center[1])}")
            self._last_cc = center

        dz = end[2] - start[2]
        if abs(dz) < 1e-9:
            words = [self._axis("X", end[0]), self._axis("Y", end[1])] + trailing
            lines.append(f"{prefix}C {' '.join(words)}")
        else:
            sweep = self._sweep_angle(center, start, end, clockwise)
            angle = f"IPA+{sweep:.{self.values['AXIS_PRECISION']}f}"
            words = [angle, f"IZ{self._length(dz)}"] + trailing
            lines.append(f"{prefix}CP {' '.join(words)}")

        return "\n".join(lines)

    @staticmethod
    def _sweep_angle(center, start, end, clockwise: bool) -> float:
        """Degrees swept from start to end around center; a closed arc is 360."""
        if abs(start[0] - end[0]) < 1e-6 and abs(start[1] - end[1]) < 1e-6:
            return 360.0
        start_angle = math.degrees(math.atan2(start[1] - center[1], start[0] - center[0]))
        end_angle = math.degrees(math.atan2(end[1] - center[1], end[0] - center[0]))
        sweep = (start_angle - end_angle) if clockwise else (end_angle - start_angle)
        sweep = sweep % 360.0
        if sweep < 1e-6:
            sweep = 360.0
        return sweep

    # ------------------------------------------------------------------
    # Drill cycles
    # ------------------------------------------------------------------

    def _convert_drill_cycle(self, command: Path.Command) -> str:
        params = command.Parameters
        name = command.Name
        previous = self.machine_state.previous

        retract = params.get("R")
        bottom = params.get("Z")
        if retract is None or bottom is None:
            raise self._error(f"{name} requires R and Z", command)

        x = params.get("X", previous.get("X"))
        y = params.get("Y", previous.get("Y"))
        if x is None or y is None:
            raise self._error(f"{name} requires a known X and Y", command)

        feed = params.get("F", previous.get("F"))
        if not feed and name not in Constants.GCODE_MOVE_TAP:
            raise self._error(f"{name} requires a feed rate", command)

        setup = float(self.values.get("DRILL_SETUP_CLEARANCE", 2.0) or 0.0)
        surface = retract - setup
        depth = bottom - surface
        if depth >= 0:
            raise self._error(
                f"{name} final depth Z{bottom} is not below the retract height R{retract}", command
            )

        initial_z = previous.get("Z")
        second_clearance = 0.0
        if self.machine_state.ReturnMode != "R" and initial_z is not None and initial_z > retract:
            second_clearance = initial_z - retract

        if self.values.get("CYCLE_FORMAT") == CYCLE_FORMAT_LEGACY:
            definition = self._legacy_cycle_definition(command, setup, depth, feed)
            return self._legacy_cycle_call(
                command, definition, x, y, retract, initial_z, second_clearance
            )

        definition = self._q_cycle_definition(
            command, setup, depth, surface, second_clearance, feed
        )
        lines = []
        if definition != self._last_cycle_def:
            lines.append(definition)
            self._last_cycle_def = definition
        lines.append(f"L {self._axis('X', x)} {self._axis('Y', y)} R0 {self._rapid_word()} M99")
        return "\n".join(lines)

    def _q_block(self, title: str, parameters: List[tuple]) -> str:
        """A multi-line CYCL DEF with Q parameters and "~" continuation."""
        lines = [f"CYCL DEF {title} ~"]
        for index, (number, value, label) in enumerate(parameters):
            end = "" if index == len(parameters) - 1 else " ~"
            lines.append(f"    Q{number}={value} ;{label}{end}")
        return "\n".join(lines)

    def _q_cycle_definition(self, command, setup, depth, surface, second, feed) -> str:
        name = command.Name
        params = command.Parameters
        dwell = params.get("P", 0.0)
        time_precision = self.values["AXIS_PRECISION"]

        def length(value):
            return self._length(value)

        def seconds(value):
            return self._signed(f"{value:.{time_precision}f}")

        if name in ("G81", "G82", "G83"):
            plunge = params.get("Q", abs(depth)) if name == "G83" else abs(depth)
            return self._q_block(
                "200 DRILLING",
                [
                    (200, length(setup), "SET-UP CLEARANCE"),
                    (201, length(depth), "DEPTH"),
                    (206, self._feed_value(feed), "FEED RATE FOR PLNGNG"),
                    (202, length(plunge), "PLUNGING DEPTH"),
                    (210, seconds(0.0), "DWELL TIME AT TOP"),
                    (203, length(surface), "SURFACE COORDINATE"),
                    (204, length(second), "2ND SET-UP CLEARANCE"),
                    (211, seconds(dwell), "DWELL TIME AT DEPTH"),
                ],
            )

        if name == "G73":
            plunge = params.get("Q", abs(depth))
            breaks = max(1, math.ceil(abs(depth) / plunge)) if plunge > 0 else 1
            return self._q_block(
                "203 UNIVERSAL DRILLING",
                [
                    (200, length(setup), "SET-UP CLEARANCE"),
                    (201, length(depth), "DEPTH"),
                    (206, self._feed_value(feed), "FEED RATE FOR PLNGNG"),
                    (202, length(plunge), "PLUNGING DEPTH"),
                    (210, seconds(0.0), "DWELL TIME AT TOP"),
                    (203, length(surface), "SURFACE COORDINATE"),
                    (204, length(second), "2ND SET-UP CLEARANCE"),
                    (212, length(0.0), "DECREMENT"),
                    (213, f"+{breaks}", "NR OF CHIP BREAKS"),
                    (205, length(0.0), "MIN. PLUNGING DEPTH"),
                    (211, seconds(dwell), "DWELL TIME AT DEPTH"),
                    (208, self._feed_value(0.0), "RETRACTION FEED RATE"),
                    (
                        256,
                        length(self.values.get("CHIP_BREAK_RETRACT", 0.5)),
                        "DIST FOR CHIP BRKNG",
                    ),
                    (395, "+0", "DEPTH REFERENCE"),
                ],
            )

        if name in Constants.GCODE_MOVE_TAP:
            pitch = self._tapping_pitch(command)
            return self._q_block(
                "207 RIGID TAPPING NEW",
                [
                    (200, length(setup), "SET-UP CLEARANCE"),
                    (201, length(depth), "DEPTH OF THREAD"),
                    (239, length(pitch), "PITCH OF THREAD"),
                    (203, length(surface), "SURFACE COORDINATE"),
                    (204, length(second), "2ND SET-UP CLEARANCE"),
                ],
            )

        if name == "G85":
            return self._q_block(
                "202 BORING",
                [
                    (200, length(setup), "SET-UP CLEARANCE"),
                    (201, length(depth), "DEPTH"),
                    (206, self._feed_value(feed), "FEED RATE FOR PLNGNG"),
                    (211, seconds(dwell), "DWELL TIME AT DEPTH"),
                    (208, self._feed_value(feed), "RETRACTION FEED RATE"),
                    (203, length(surface), "SURFACE COORDINATE"),
                    (204, length(second), "2ND SET-UP CLEARANCE"),
                    (214, "+0", "DISENGAGING DIRECTN"),
                    (336, "+0", "ANGLE OF SPINDLE"),
                ],
            )

        raise self._error(f"Unsupported drill cycle {name}", command)

    def _tapping_pitch(self, command: Path.Command) -> float:
        """Thread pitch in mm; negative for a left-hand thread (G74)."""
        params = command.Parameters
        feed = params.get("F")
        if feed is None:
            raise self._error(f"{command.Name} requires F (the thread pitch)", command)

        if command.Annotations.get("operation", "") == "tapping":
            # The tapping generator writes the pitch itself into F.
            pitch = feed
        else:
            speed = params.get("S") or self.machine_state.previous.get("S")
            if not speed:
                raise self._error(
                    f"{command.Name} needs a spindle speed to derive the pitch from F", command
                )
            pitch = feed * 60.0 / speed

        if pitch <= 0:
            raise self._error(f"{command.Name} pitch must be positive", command)
        return -pitch if command.Name == "G74" else pitch

    def _legacy_cycle_definition(self, command, setup, depth, feed) -> str:
        name = command.Name
        params = command.Parameters
        precision = self.values["AXIS_PRECISION"]

        def length(value):
            return self.format_parameter("X", value)

        if name in ("G81", "G82", "G83", "G73"):
            plunge = params.get("Q", abs(depth)) if name in ("G83", "G73") else abs(depth)
            dwell = params.get("P", 0.0)
            return "\n".join(
                [
                    "CYCL DEF 1.0 PECKING",
                    f"CYCL DEF 1.1 SET UP {length(setup)}",
                    f"CYCL DEF 1.2 DEPTH {length(depth)}",
                    f"CYCL DEF 1.3 PECKG {length(-abs(plunge))}",
                    f"CYCL DEF 1.4 DWELL {dwell:.{precision}f}",
                    f"CYCL DEF 1.5 F{self._feed_value(feed)}",
                ]
            )

        if name in Constants.GCODE_MOVE_TAP:
            pitch = self._tapping_pitch(command)
            return "\n".join(
                [
                    "CYCL DEF 17.0 RIGID TAPPING",
                    f"CYCL DEF 17.1 SET UP {length(setup)}",
                    f"CYCL DEF 17.2 DEPTH {length(depth)}",
                    f"CYCL DEF 17.3 PITCH {self._length(pitch)}",
                ]
            )

        raise self._error(
            f"{name} has no legacy Klartext cycle; use the q_parameter format", command
        )

    def _legacy_cycle_call(
        self, command, definition, x, y, retract, initial_z, second_clearance
    ) -> str:
        """Legacy cycles start from wherever the tool is, so position it explicitly."""
        lines = []
        if definition != self._last_cycle_def:
            lines.append(definition)
            self._last_cycle_def = definition

        previous = self.machine_state.previous
        if previous.get("X") != x or previous.get("Y") != y:
            lines.append(f"L {self._axis('X', x)} {self._axis('Y', y)} R0 {self._rapid_word()}")
        if previous.get("Z") != retract:
            lines.append(f"L {self._axis('Z', retract)} R0 {self._rapid_word()}")
        lines.append("CYCL CALL")
        if second_clearance > 0:
            lines.append(f"L {self._axis('Z', initial_z)} R0 {self._rapid_word()}")
        return "\n".join(lines)

    # ------------------------------------------------------------------
    # Dwell, tool change, spindle, coolant, program control, fixtures
    # ------------------------------------------------------------------

    def _convert_dwell(self, command: Path.Command) -> str:
        seconds = command.Parameters.get("P", 0.0)
        value = self.format_parameter("P", seconds, "G4")
        return f"CYCL DEF 9.0 DWELL TIME\nCYCL DEF 9.1 DWELL {value}"

    def _convert_tool_change(self, command: Path.Command) -> str:
        params = command.Parameters
        if "T" not in params:
            raise self._error("Tool change requires T", command)
        words = [f"TOOL CALL {int(params['T'])}", self._tool_axis()]

        speed = params.get("S")
        if speed:
            words.append(f"S{self.format_parameter('S', speed)}")
            self._tool_call_speed = speed

        # The control's modal state is unknown after a tool change.
        self.machine_state.setState(None)
        self._compensation = "R0"
        return " ".join(words)

    def _convert_spindle_command(self, command: Path.Command) -> str:
        if command.Name in Constants.MCODE_SPINDLE_OFF:
            return "M5"

        lines = []
        speed = command.Parameters.get("S")
        if speed and (self._tool_call_speed is None or abs(speed - self._tool_call_speed) > 1e-9):
            # TOOL CALL without a tool number changes only the spindle speed.
            lines.append(f"TOOL CALL S{self.format_parameter('S', speed)}")
            self._tool_call_speed = speed

        lines.append("M3" if command.Name in Constants.MCODE_SPINDLE_CW else "M4")
        return "\n".join(lines)

    @staticmethod
    def _m_word(command: Path.Command) -> str:
        """M08 -> M8"""
        return f"M{int(command.Name[1:])}"

    def _convert_coolant_command(self, command: Path.Command) -> str:
        return self._m_word(command)

    def _convert_program_control(self, command: Path.Command) -> str:
        return self._m_word(command)

    def _convert_fixture(self, command: Path.Command) -> str:
        mode = self.values.get("FIXTURE_OUTPUT", FIXTURE_OUTPUT_PRESET)
        if mode == FIXTURE_OUTPUT_NONE:
            return None

        first = int(self.values.get("FIXTURE_FIRST_NUMBER", 1))
        number = self.FixtureIndex[command.Name] - 1 + first

        if mode == FIXTURE_OUTPUT_DATUM:
            return f"CYCL DEF 7.0 DATUM SHIFT\nCYCL DEF 7.1 #{number}"

        return self._q_block("247 PRESETTING", [(339, f"+{number}", "PRESET NUMBER")])

    # ------------------------------------------------------------------
    # Modal state and everything else
    # ------------------------------------------------------------------

    def _convert_modal_command(self, command: Path.Command) -> str:
        if command.Name in self.GCodeSilent:
            return None
        raise self._error(f"{command.Name} has no Klartext equivalent", command)

    def _convert_generic_command(self, command: Path.Command) -> str:
        name = command.Name

        # The base dispatcher only routes the expandable and extended cycles.
        if name in Constants.GCODE_MOVE_DRILL:
            return self._convert_drill_cycle(command)

        if name in self.CompensationWord:
            self._compensation = self.CompensationWord[name]
            return None

        if name in self.GCodeSilent:
            return None

        if name in Constants.MCODE_END + Constants.MCODE_END_RESET:
            return self._m_word(command)

        # Early tool prep: a bare Tn pre-selects the next tool.
        if name.startswith("T") and name[1:].isdigit():
            return f"TOOL DEF {int(name[1:])}"

        raise self._error(f"{name} has no Klartext equivalent", command)

    @property
    def tooltip(self):
        return """
        Heidenhain Klartext (conversational) post processor.

        Writes a numbered .h program: BEGIN/END PGM, BLK FORM from the
        stock, TOOL CALL, L/CC/C/CR/CP contour blocks with RL/RR/R0,
        drilling and tapping cycles (CYCL DEF 200/202/203/207 or the
        legacy CYCL DEF 1/17 format), dwell cycle 9 and preset or datum
        selection for fixtures.
        """


# Class alias for PostProcessorFactory: it looks for the title-cased post name.
Heidenhain = HeidenhainPost


def create(job, **kwargs):
    """Factory function to create a Heidenhain postprocessor instance."""
    return HeidenhainPost(job, **kwargs)
