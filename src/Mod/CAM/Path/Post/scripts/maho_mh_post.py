# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Klaus Naumann <denkbrettl@gmail.com>               *
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

# flake8 --ignore E501 maho_mh_post.py

"""
Maho MH post processor, "machine" based.

Targets the Maho MH (Maho Horizontal) milling machines with a Philips 432
controller. This is so far tested on a V700 software MH600E machine.
Some of the code is inspired by the Dynapath postprocessor.

Specifics of this postprocessor:

- the first line of a program needs to be %PM followed by a line with the program number
  which must be between N1 and N9999999. The number can be entered on output, if
  nothing is entered, DEFAULT_PROGRAM_NUM is taken as default
- units are ``G70`` (inch) and ``G71`` (metric), not G20/G21
- an arc needs absolute coordinates for its center point in I, J and K
- a program needs the EOT character (\x04) at the end for the control to read it in properly via serial
- for machines with a tool changer M6 will be used for tools in the tool changer and M66 for all above the size
- G98/G99 are used to define the stock size and graphical window size for the simulation on the controller itself
- retract distance for chip-breaking in drilling cycles is configurable
- drill cycles are translated into the Maho specific GCode sequences and a comment with the original GCode is added before
"""

from typing import Any, Dict

import FreeCAD
import Path
import itertools
import Constants
from Path.Post.Processor import PostProcessor, SCOPE_MACHINE, SCOPE_JOB
from Machine.models.machine import OutputUnits
from Path.Post.CAMErrors import CAMAttributeError

translate = FreeCAD.Qt.translate

DEBUG = False
if DEBUG:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

# Define some types that are used throughout this file.
Values = Dict[str, Any]

POST_TYPE = "machine"

UNITS_METRIC = "G71"
UNITS_INCH = "G70"
DEFAULT_PROGRAM_NUM = "N90001"
DEFAULT_CHIP_BREAK_RETRACT = 0.5
DEFAULT_TOOL_CHANGER_SIZE = 0
DEFAULT_GRAPHICAL_VIEW_WINDOW_SIZE = 110
END_OF_TRANSMISSION = "\x04"
# Maho MH have an interesting amount of supported and non-supported
# gCode commands, so let's specify what works here
DEFAULT_SUPPORTED_COMMANDS = [
    "G0",
    "G00",
    "G1",
    "G01",
    "G2",
    "G02",
    "G3",
    "G03",
    "G4",
    "G04",
    "G17",
    "G18",
    "G19",
    "G40",
    "G41",
    "G42",
    "G43",
    "G52",
    "G54",
    "G55",
    "G56",
    "G57",
    "G58",
    "G59",
    "G70",
    "G71",
    "G73",
    "G79",
    "G80",
    "G81",
    "G82",
    "G83",
    "G84",
    "G85",
    "G88",
    "G89",
    "G90",
    "G91",
    "G92",
    "G93",
    "G94",
    "G95",
    "G98",
    "G99",
    "M0",
    "M00",
    "M2",
    "M02",
    "M3",
    "M03",
    "M4",
    "M04",
    "M5",
    "M05",
    "M6",
    "M06",
    "M7",
    "M07",
    "M8",
    "M08",
    "M9",
    "M09",
    "M30",
    "M66",
]


class Maho_Mh(PostProcessor):
    """
    The Maho MH post processor class.
    Maho MH machines use Philips 432 machine control systems.

    This post processor is completely new and lacks thorough testing. Use at your own risk!
    """

    @classmethod
    def get_common_property_schema(cls):
        """Override common properties with Maho MH-specific defaults."""
        common_props = super().get_common_property_schema()

        # Override defaults for Maho MH
        for prop in common_props:
            if prop["name"] == "supports_tool_radius_compensation":
                prop["default"] = True
            elif prop["name"] == "preamble":
                prop["default"] = "G17\nG52"
            elif prop["name"] == "postamble":
                # Stop spindle
                # Stop coolant
                # Set XY plane
                # Deactivate radius compensation
                # Program end
                prop["default"] = "M5\nM9\nG17\nG40\nM30"
            elif prop["name"] == "safetyblock":
                prop["default"] = ""
            elif prop["name"] == "supported_commands":
                prop["default"] = "\n".join(DEFAULT_SUPPORTED_COMMANDS)
        return common_props

    @classmethod
    def get_property_schema(cls):
        """Return schema for Maho MH-specific configurable properties."""
        return [
            {
                "name": "tool_changer_size",
                "scope": SCOPE_MACHINE,
                "type": "int",
                "label": translate("CAM", "Tool changer size"),
                "default": DEFAULT_TOOL_CHANGER_SIZE,
                "min": 0,
                "help": translate(
                    "CAM",
                    "If your machine has a tool changer, specify its size here. "
                    "M6 will be used for tools in the tool changer and M66 for all above the size.",
                ),
            },
            {
                "name": "emit_graphical_view_setup",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Emit graphical view setup commands"),
                "default": True,
                "help": translate(
                    "CAM",
                    "If your controller supports it, this will allow you to do a graphical simulation "
                    "on the machine itself before running the program.",
                ),
            },
            {
                "name": "graphical_view_window_size",
                "scope": SCOPE_MACHINE,
                "type": "float",
                "label": translate("CAM", "Graphical view window size (%)"),
                "default": 110,
                "min": 101,
                "help": translate(
                    "CAM",
                    "When emitting the graphical view setup commands, make the size of the view window "
                    "this percentage of the stock size.",
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
                    "Write I, J and K as the absolute coordinates of the arc center, which is "
                    "how the Philips 432 expects them.",
                ),
            },
            {
                "name": "program_number",
                "scope": SCOPE_JOB,
                "type": "string",
                "label": translate("CAM", "Program Number"),
                "default": "N90001",
                "help": translate(
                    "CAM",
                    "The program number on the first line after the %PM start. "
                    "Must be between N1 and N9999999. If left empty, N90001 is the default.",
                ),
            },
            {
                "name": "chip_break_retract",
                "scope": SCOPE_JOB,
                "type": "float",
                "label": translate("CAM", "Chip break retract (mm)"),
                "default": 0.5,
                "min": 0,
                "help": translate(
                    "CAM",
                    "When choosing 'Chip break' in drilling cycles, the drill will retract "
                    "by this amount of millimeters per peck.",
                ),
            },
        ]

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Maho MH post processor"),
        tooltipargs=[],
        units="Metric",
    ) -> None:
        super().__init__(
            job=job,
            tooltip=tooltip,
            tooltipargs=tooltipargs,
            units=units,
        )
        Path.Log.debug("Maho MH post processor initialized.")

    def init_values(self, values: Values) -> None:
        super().init_values(values)
        values["MACHINE_NAME"] = "Maho MH"
        values["POSTPROCESSOR_FILE_NAME"] = __name__

    @property
    def tooltip(self):
        tooltip: str = """
        This is a postprocessor file for the CAM workbench.
        It is used to take a pseudo-gcode fragment from a CAM object
        and output 'real' GCode suitable for a Maho MH 3 axis mill.
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

    # ------------------------------------------------------------------
    # Postable expansion
    # ------------------------------------------------------------------

    @staticmethod
    def _as_is(text: str) -> Path.Command:
        return Path.Command("", {}, {Constants.ANNOT_AS_IS: text})

    @staticmethod
    def _as_comment(text: str) -> Path.Command:
        return Path.Command(f"({text})", {}, {})

    @staticmethod
    def _safe_get(params, param: str, cmd: Path.Command):
        if param not in params:
            raise Exception(f"{cmd.Name} command missing '{param}' parameter")
        else:
            return params.get(param)

    def _add_line_numbers(self, postables) -> None:
        """Add N word if we are line-numbering
        Subclasses are expected to render N as line-number (if present)
            the default _convert_move() will do that
        Numbering does not know about the subclass inserting/removing commands/lines,
            so is NOT the physical line-number.

        Subclasses can override to customize.
        """
        if not self.values["OUTPUT_LINE_NUMBERS"]:
            return

        Path.Log.track("Line numbering")

        start = self.values["LINE_NUMBER_START"]
        increment = self.values["LINE_INCREMENT"]

        for section_name, sublist in postables:
            # per section
            line_number = itertools.count(start, increment)
            numbering = True

            for item in sublist:
                # Philips 432 controllers need line numbers for all lines, except for the
                # program start and name
                if "program name" in item.label:
                    numbering = False

                if item.item_type == "str":
                    str_lines = item.data["str"].splitlines(keepends=True)
                    item.data["str"] = ""
                    for line in str_lines:
                        stripped = line.strip()
                        if stripped and not stripped.startswith("%"):
                            if line.startswith("/"):
                                item.data["str"] += f"/N{next(line_number)} {line[1:]}"
                            else:
                                item.data["str"] += f"N{next(line_number)} {line}"
                        else:
                            item.data["str"] += line

                # number Path.Command's
                elif item.path:
                    new_commands = []
                    for command in item.path.Commands:
                        if numbering:
                            if command.Name.startswith("(") and not self.values.get(
                                "OUTPUT_COMMENTS", True
                            ):
                                new_commands.append(command)
                                continue
                            # Have to remake, because we change Parameters
                            # _convert_move() and _convert_comment() do the LINE_NUMBER_PREFIX
                            new_params = {"N": next(line_number)}
                            new_params.update(command.Parameters)
                            new_commands.append(
                                Path.Command(command.Name, new_params, command.Annotations)
                            )
                        else:
                            new_commands.append(command)
                    item.path = Path.Path(new_commands)
                numbering = True

    @staticmethod
    def _clearance_height(source):
        """The clearance height of the operation a posted item came from, in mm."""
        if source is None:
            return None
        import Path.Base.Util as PathUtil

        for obj in (source, getattr(source, "source", None)):
            if obj is None:
                continue
            for target in (obj, PathUtil.baseOp(obj)):
                height = getattr(target, "ClearanceHeight", None)
                if height is not None:
                    return float(getattr(height, "Value", height))
        return None

    def _expand_canned_cycles(self, postables):
        """Canned drill cycles in Philips 432 only use two commands and no termination

        A drill cycle is done with defining the drill parameters through G8x
        and is then executed through calling G79 with the position for the operation.

        So in G17 instead of
        G0 Z10.000
        G0 X30.000 Y8.000
        G0 Z4.000
        G99
        G81 X30.000 Y8.000 Z-15.000 F200.000 R4.000
        G80
        G0 Z10.000

        one would write

        G0 Z10.000
        G0 X30.000 Y8.000
        G0 Z4.000
        G81 Y4.000 Z-15.000 F200.000
        G79 X30.000 Y8.000 Z0.000
        G0 Z10.000

        """
        super()._expand_canned_cycles(postables)

        # Track the previous Z value for the B word of the drill cycle.
        # When an operation starts with a drill cycle, fall back to its ClearanceHeight.
        previous_z = None
        for section_name, sublist in postables:
            for item in sublist:

                if not item.path:
                    continue

                if not (commands := item.path.Commands):
                    continue

                op_clearance = self._clearance_height(item.source) or self._clearance_height(item)
                if previous_z is None and op_clearance is not None:
                    previous_z = op_clearance

                # We build a new command list as we need to delete
                # and insert commands and it's messy to do this while working
                # on the command list
                new_commands = []

                # True if G98 false if G99
                return_initial = False
                for cmd in commands:
                    # Remove G98 if it's a return command (which doesn't have parameters)
                    # in order to leave the graphical view setup commands alone
                    if cmd.Name in Constants.GCODE_RETURN_INITIAL and not cmd.Parameters:
                        return_initial = True
                        continue
                    # Remove G99
                    elif cmd.Name in Constants.GCODE_RETURN_R and not cmd.Parameters:
                        return_initial = False
                        continue
                    # Remove G80
                    elif cmd.Name in Constants.GCODE_CYCLE_CANCEL:
                        continue

                    elif cmd.Name in Constants.GCODE_MOVE_DRILL + Constants.GCODE_DRILL_EXTENDED:
                        orig_params = cmd.Parameters
                        cbr = None  # chip break retract
                        cycle_def_params = {}
                        if cmd.Name in ["G81", "G82"]:
                            cycle_def_cmd = "G81"
                        elif cmd.Name in ["G83", "G84", "G85"]:
                            cycle_def_cmd = cmd.Name
                        elif cmd.Name in ["G73"]:
                            cycle_def_cmd = "G83"
                            cbr = self.values.get("CHIP_BREAK_RETRACT")
                            if cbr is None:
                                cbr = DEFAULT_CHIP_BREAK_RETRACT
                        else:
                            raise Exception(f"Drill cycle {cmd.Name} not supported")

                        # Set dwell time if given
                        if "P" in orig_params:
                            cycle_def_params["X"] = orig_params.get("P")

                        # Set peck depth if given
                        if "Q" in orig_params:
                            cycle_def_params["K"] = orig_params.get("Q")
                            # I is a "reduction value" that would be deducted from every
                            # peck move. As FreeCAD doesn't have this concept, we'll set it to
                            # zero so all pecks have the same depth
                            cycle_def_params["I"] = 0
                            if cbr is not None:
                                cycle_def_params["J"] = cbr

                        # For thread cutting we need a ramp parameter, which is 1% of the spindle speed
                        # Should this be configurable?
                        if cmd.Name in ["G84"]:
                            s_val = orig_params.get("S")
                            if (
                                s_val is None
                                and hasattr(self, "machine_state")
                                and getattr(self.machine_state, "S", None)
                            ):
                                s_val = self.machine_state.S
                            if s_val is None:
                                s_val = self._safe_get(orig_params, "S", cmd)
                            cycle_def_params["I"] = round(s_val / 100)

                        # safety height
                        cycle_def_params["Y"] = self._safe_get(orig_params, "R", cmd)
                        # drill depth
                        # FIXME: G18 safe?
                        cycle_def_params["Z"] = self._safe_get(orig_params, "Z", cmd)
                        # retract height (G98)
                        if return_initial:
                            z_ref = previous_z if previous_z is not None else op_clearance
                            if z_ref is not None and z_ref - cycle_def_params["Y"] > 0:
                                cycle_def_params["B"] = z_ref - cycle_def_params["Y"]

                        # feed
                        cycle_def_params["F"] = self._safe_get(orig_params, "F", cmd)

                        # Add a comment here with the original command, as reference
                        new_commands.append(self._as_comment(cmd.toGCode().split(";")[0]))
                        new_commands.append(Path.Command(cycle_def_cmd, cycle_def_params))
                        # Drill height start is always 0 as depth is given as absolute
                        # FIXME: Does Z = 0 work for G18 here? Probably needs to be swapped with Y
                        cycle_run_params = {
                            "X": self._safe_get(orig_params, "X", cmd),
                            "Y": self._safe_get(orig_params, "Y", cmd),
                            "Z": 0,
                        }
                        new_commands.append(Path.Command("G79", cycle_run_params))
                        continue

                    # Push everything else through normally
                    else:
                        if "Z" in cmd.Parameters:
                            previous_z = cmd.Parameters.get("Z")
                        new_commands.append(cmd)

                item.path.Commands = new_commands

    def _expand_prefix(self, postables) -> None:
        """The program start and number comes before everything, header included."""
        super()._expand_prefix(postables)
        raw_num = str(self.values.get("PROGRAM_NUMBER") or "").strip() or DEFAULT_PROGRAM_NUM
        prog_num = raw_num if raw_num.upper().startswith("N") else f"N{raw_num}"
        for section_name, sublist in postables:
            # Add program name and number
            sublist.insert(
                0,
                self._make_postable(
                    "Post: program name", [self._as_is("%PM"), self._as_is(f"{prog_num}")]
                ),
            )
            # Add graphical view setup if option is enabled
            for num, item in enumerate(sublist):
                if (
                    self.values.get("EMIT_GRAPHICAL_VIEW_SETUP", True)
                    and "Post: preamble" in item.label
                ):
                    try:
                        stock = getattr(self._job, "Stock", None)
                        if not stock or not getattr(stock, "Shape", None):
                            break
                        if hasattr(stock.Shape, "isNull") and stock.Shape.isNull():
                            break
                        box = getattr(stock.Shape, "BoundBox", None)
                        if not box:
                            break
                        x_size = box.XMax - box.XMin
                        y_size = box.YMax - box.YMin
                        z_size = box.ZMax - box.ZMin
                        if x_size <= 0 or y_size <= 0 or z_size <= 0:
                            break
                        size_factor = (
                            self.values.get(
                                "GRAPHICAL_VIEW_WINDOW_SIZE", DEFAULT_GRAPHICAL_VIEW_WINDOW_SIZE
                            )
                            / 100
                        )
                        x_offset = (x_size * size_factor - x_size) / 2
                        y_offset = (y_size * size_factor - y_size) / 2
                        z_offset = (z_size * size_factor - z_size) / 2
                        # view window size
                        g98 = Path.Command(
                            "G98",
                            {
                                "X": box.XMin - x_offset,
                                "Y": box.YMin - y_offset,
                                "Z": box.ZMin - z_offset,
                                "I": box.XMax + x_offset,
                                "J": box.YMax + y_offset,
                                "K": box.ZMax + z_offset,
                            },
                            {},
                        )
                        # stock size
                        g99 = Path.Command(
                            "G99",
                            {
                                "X": box.XMin,
                                "Y": box.YMin,
                                "Z": box.ZMin,
                                "I": box.XMax,
                                "J": box.YMax,
                                "K": box.ZMax,
                            },
                            {},
                        )
                        sublist.insert(
                            num + 1, self._make_postable("Post: graphical view setup", [g98, g99])
                        )
                        break
                    except (AttributeError, Exception) as err:
                        Path.Log.warning(f"Could not emit graphical view setup: {err}")
                        break

    def _collect_unit_command(self) -> list:
        """G71 or G70, in place of G21/G20."""
        if self.values["OUTPUT_UNITS"] == OutputUnits.METRIC:
            return Path.Command(UNITS_METRIC, {}, {Constants.ANNOT_ALLOW_UNSUPPORTED: True})
        elif self.values["OUTPUT_UNITS"] == OutputUnits.IMPERIAL:
            return Path.Command(UNITS_INCH, {}, {Constants.ANNOT_ALLOW_UNSUPPORTED: True})
        else:
            raise CAMAttributeError(
                f"Must have _machine.output.units (in {self._machine.name}) as one of [{OutputUnits.METRIC},{OutputUnits.IMPERIAL}], someone replaced the default with: {self.values['OUTPUT_UNITS'].__class__.__name__} {self.values['OUTPUT_UNITS']}",
                job=self._job,
                operation=self._operation,
                pp=self.values["MACHINE_NAME"],
            )

    def _optimize_duplicates_doubles(self, postables):
        """I, J and K need the center in absolute values"""
        absolute = self.values.get("ABSOLUTE_ARC_CENTERS", True)
        position = {"X": 0.0, "Y": 0.0, "Z": 0.0}
        for _, sublist in postables:
            for item in sublist:
                if not item.path:
                    continue
                commands = []
                changed = False
                for cmd in item.path.Commands:
                    if cmd.Name in Constants.GCODE_MOVE_ARC:
                        params = {k: v for k, v in cmd.Parameters.items()}
                        if absolute:
                            for word, axis in (("I", "X"), ("J", "Y"), ("K", "Z")):
                                if word in params:
                                    params[word] = params[word] + position[axis]
                        cmd = Path.Command(cmd.Name, params, cmd.Annotations)
                        changed = True
                    for axis in position:
                        if axis in cmd.Parameters:
                            position[axis] = cmd.Parameters[axis]
                    commands.append(cmd)
                if changed:
                    item.path = Path.Path(commands)
        super()._optimize_duplicates_doubles(postables)

    def _expand_trailing_lines(self, postables) -> None:
        """Finish the program with the EOT char"""
        super()._expand_trailing_lines(postables)
        for _, section in postables:
            program_end = [
                self._as_comment(
                    "The next line is the EOT character, which is important for Philips 432 controllers. Do not remove."
                ),
                self._as_is(END_OF_TRANSMISSION),
            ]
            section.append(self._make_postable("Post: program end", program_end))

    def _convert_comment(self, command: Path.Command) -> str:
        """All lines need line numbers"""
        cmd_str = super()._convert_comment(command)
        if not cmd_str:
            return cmd_str
        if self.values["OUTPUT_LINE_NUMBERS"]:
            params = command.Parameters
            if params.get("N", None) is not None:
                prefix = self.values["LINE_NUMBER_PREFIX"]
                return f"{prefix}{int(params['N']):d} {cmd_str}"
        return cmd_str

    def _convert_tool_change(self, command: Path.Command) -> str:
        """If the machine has a tool changer, all tools that are not in the tool changer
        need to be swapped using the M66 command. The machines are smart enough to know that
        when a tool was swapped in with M66 and the next one comes with M6 from the tool changer,
        then the manual tool needs to be manually removed first.
        """
        tool_changer_size = self.values.get("TOOL_CHANGER_SIZE", DEFAULT_TOOL_CHANGER_SIZE)
        if tool_changer_size is None:
            tool_changer_size = DEFAULT_TOOL_CHANGER_SIZE
        if tool_changer_size > 0:
            tool_num = command.Parameters.get("T", None)
            if tool_num is not None and tool_num > tool_changer_size:
                command = Path.Command("M66", command.Parameters, command.Annotations)
        return super()._convert_tool_change(command)
