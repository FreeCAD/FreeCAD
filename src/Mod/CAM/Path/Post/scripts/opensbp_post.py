# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileCopyrightText: 2026 Alan Grover <awgrover@gmail.com>

# ***************************************************************************
# *   Copyright (c) 2026 Alan Grover <awgrover@gmail.com>                   *
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

"""
OpenSBP Post Processor for ShopBot Controllers, "machine" based
"""

import operator
import math
import re
import textwrap
from typing import Any, Dict

import FreeCAD
import Path

Path.Log.debug(f"### RELOADED {__file__}")
import Constants

from Path.Post.Processor import PostProcessor
from Path.Post.GcodeProcessingUtils import insert_line_numbers

translate = FreeCAD.Qt.translate

DEBUG = False


# Set logging level based on DEBUG flag
def _setup_logging():
    if DEBUG:
        Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
        Path.Log.trackModule(Path.Log.thisModule())
    else:
        Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


_setup_logging()

# Define types
Values = Dict[str, Any]

POST_TYPE = "machine"


class OpenSBPPost(PostProcessor):
    """
    OpenSBP postprocessor for ShopBot controllers.

    OpenSBP can use most gcodes (see GCodeKnow), with a few translated to opensbp
    """

    # This list is specific to shopbot, not from Mod/CAM/Constants.py
    # from https://shopbottools.com/wp-content/uploads/2024/01/SBG-00142-User-Guide-20150317.pdf
    # We want to translate some of the natively-supported gcodes (e.g. M2), so we omit them here.
    # It includes commands that Operations shouldn't generate (cf. Constants.GCODE_NON_CONFORMING)
    # It may include commands that Post/Processor.py shouldn't generate (cf. Constants.GCODE_SUPPORTED and Constants.MCODE_SUPPORTED and Constants.GCODE_NON_CONFORMING)
    # Compatible should just pass-through
    GCodeNative = set(
        # M10/M11 is clamp-on/clamp-off
        "G0 G00 G1 G01 G4 G04 G20 G21 G28 G29 G38.2 G92 M0 M00 M1 M01 M03 M5 M05 M8 M08 M9 M09 M10 M11".split(
            " "
        )
    )
    # NB: these are the generated strings, e.g. if an M02 generates to "M2", it counts
    GCodeLineNumberRequired = set("M2 M3 M4 M5".split(" "))
    GCodeLineNumberRequiredParameters = set("Z".split(" "))  # if modal, the parameter is first

    # Suppressed/Tolerated
    # because G54 is a default Job value, but shopbot has no concept
    # G98/G99/G80 should have been consumed by drillcycleexpander FIXME
    GCodeSuppressed = set("G54".split(" "))

    # Unsupported
    GCodeUnsupported = set(
        "G40 G41 G42 G43 "
        "G55 G56 G57 G58 G59 G59.1 G59.2 G59.3 G59.4 G59.5 G59.6 G59.7 G59.8 G59.9 "  # work-offsets
        "G74 "
        "G93 G94 G95 "  # opensbp only does units/sec
        "G96 G97 "  # spindle control?
        "M4 ".rstrip().split(  # ccw speed. We could support this, requires spindle-control on the machine
            " "
        )
    )

    # Others require translation
    # FIXME: is M2/M30 program-end supposed to be in GCODE_SUPPORTED?
    GCodeTranslate = set(
        "G2 G02 G3 G03 G73 G80 G81 G83 G98 G99 M2 M02 M3 M03 M5 M05 M6 M06 M30".split(" ")
    )
    GCodeKnown = GCodeTranslate | GCodeNative | GCodeSuppressed
    if GCodeKnown & GCodeUnsupported:
        raise Exception(
            f"Internal: you screwed up and have a value in both GCodeKnown & GCodeUnsupported: {GCodeKnown & GCodeUnsupported}"
        )

    # gcodes that are supported but shouldn't be used by CAM or Post/Processing
    GCodeDontUse = GCodeKnown - set(
        Constants.GCODE_SUPPORTED + Constants.MCODE_SUPPORTED + Constants.GCODE_NON_CONFORMING
    )

    # What we should support
    GCodeSupported = GCodeKnown - GCodeDontUse - GCodeUnsupported

    @classmethod
    def get_common_property_schema(cls):
        """Override .values common properties with OpenSBP-specific defaults.
        Which only apply if .values is not set by a machine-field, or dialog, etc.
        To override a property, use _merge_machine_config()
        """
        common_props = super().get_common_property_schema()

        # Override defaults for OpenSBP
        for prop in common_props:
            if prop["name"] == "file_extension":
                prop["default"] = "sbp"

            # FIXME: show but don't allow edit in UI

            elif prop["name"] == "supported_commands":
                # actually, we could allow reducing this list, but not expanding it
                prop["default"] = "\n".join(cls.GCodeSupported)
            elif prop["name"] == "drill_cycles_to_translate":
                prop["default"] = "\n".join(
                    Constants.GCODE_DRILL_EXTENDED + Constants.GCODE_MOVE_DRILL
                )
            elif prop["name"] == "translate_drill_cycles":
                prop["default"] = True
            elif prop["name"] == "output_tool_length_offset":
                prop["default"] = False

        return common_props

    @classmethod
    def get_property_schema(cls):
        """Return schema for OpenSBP-specific configurable properties."""
        return [
            {
                "name": "automatic_tool_changer",
                "type": "bool",
                "label": translate("CAM", "Automatic Tool Changer"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Enable if machine has automatic tool changer. "
                    "If disabled, tool changes will pause for manual intervention.",
                ),
            },
            {
                "name": "automatic_spindle",
                "type": "bool",
                "label": translate("CAM", "Automatic Spindle Control"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Enable if machine has automatic spindle speed control. "
                    "If disabled, spindle commands will prompt for manual adjustment.",
                ),
            },
            # FIXME: should be a general option
            {
                "name": "suppressed_commands",
                "type": "text",
                "label": translate("CAM", "Suppressed (tolerated) G-code Commands"),
                "default": "\n".join(cls.GCodeSuppressed),
                "help": translate(
                    "CAM",
                    "List of G-code commands tolerated but suppressed by this post-processor (one per line). "
                    "Commands this list will be filtered out",
                ),
            },
        ]

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "OpenSBP post processor for ShopBot controllers"),
        tooltipargs=[],
        units="Metric",
    ) -> None:
        super().__init__(
            job=job,
            tooltip=tooltip,
            tooltipargs=tooltipargs,
            units=units,
        )
        Path.Log.debug("OpenSBP post processor initialized.")

        self._first_probe_open = True  # for probe-subroutines only once

        # Track current speeds for OpenSBP (separate XY and Z speeds)
        self._current_move_speed_xy = None
        self._current_move_speed_z = None
        self._current_jog_speed_xy = None
        self._current_jog_speed_z = None

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        super().init_values(values)

        # OpenSBP-specific settings
        values["MACHINE_NAME"] = "ShopBot"
        values["POSTPROCESSOR_FILE_NAME"] = __name__

    def _merge_machine_config(self):
        """Override .values initial setup, and .postprocessor_properties"""

        # Override ._machine so far (bundle has copied to .postprocessor_properties)

        # .values & custom initial setup
        super()._merge_machine_config()

        # Override .values

        self.values["COMMENT_SYMBOL"] = "'"

        # schema by [name]
        schema = {x["name"]: x for x in self.get_common_property_schema()}

        # These schema defaults are r/o: force them
        for property_name in (
            "supported_commands drill_cycles_to_translate"
            " translate_drill_cycles output_tool_length_offset".split(" ")
        ):
            self.values[property_name.upper()] = schema[property_name]["default"]

    def convert_command_to_gcode(self, command: Path.Command) -> str:

        # FIXME: should be in Processor class
        if command.Name in self.values["SUPPRESSED_COMMANDS"].split("\n"):
            Path.Log.debug(f"opensbp suppressed {command}")
            return None

        # FIXME: optional blockdelete emulation w/"if somevariable"
        if command.Annotations.get("blockdelete", False):
            raise ValueError(f"opensbp does not support blockdelete, at {command.toGCode()}")

        return super().convert_command_to_gcode(command)

    def _convert_move(self, command):
        # FIXME: use Path.Command world _add_line_numbers when implemented
        gcode = super()._convert_move(command)

        if self.values["OUTPUT_LINE_NUMBERS"]:
            # It will be taken care of later (everything line-numbered)
            return gcode

        # We have to do this in string world
        result = []
        gcode_lines = gcode.split("\n")
        for line in gcode_lines:
            command_name, *_ = line.split(" ", 1)

            if (
                command_name in self.GCodeLineNumberRequired
                # modal can omit the command, leaving a Zn... as the first parameter -> head of string
                or command_name[0] in self.GCodeLineNumberRequiredParameters
            ):
                # Line-numbering can't work properly, the progress isn't saved anywhere after
                # calling insert_line_numbers()
                start = self.values["LINE_NUMBER_START"]
                increment = self.values["LINE_INCREMENT"]
                result.append("' LN required")
                result.extend(insert_line_numbers(gcode.split("\n"), start, increment))
            else:
                result.append(line)

        return "\n".join(result)

    def _convert_arc_move(self, command):
        """
        Convert arc moves (G2/G3) that change Z to OpenSBP CG command.
        Non-changing-Z is passed through as G2/G3.

        OpenSBP CG format: CG,,X,Y,I,J,"T",direction[,plunge]
        where:
        - direction is 1 for CW (G2) or -1 for CCW (G3)
        - T is literal
        - plunge is optional Z movement (relative, sign inverted)

        Note: ShopBot only supports arcs in XY plane with I,J offsets.
        If Z is present, it's converted to a helical arc with plunge parameter.
        NB: Plunge is relative, so we need the current Z position
        """

        params = command.Parameters

        # We may be axis-modal
        machine_state_params = self._modal_state  # FIXME self.machine_state.getState()
        params.update(
            {
                p: machine_state_params[p]
                for p in "XYZF"
                if params.get(p, None) is None and machine_state_params[p] is not None
            }
        )

        # notably, not R format, and no repetitions (P)
        AllowedParameters = set("XYZIJFN")

        if illegal := [x for x in params if x not in AllowedParameters]:
            # FIXME: what is the right way to report error? How to include context?
            raise ValueError(
                f"Only {''.join(AllowedParameters)} allowed for {command.Name}, saw {illegal} in {command}"
            )
        if missing := [x for x in AllowedParameters - {"N"} if x not in params]:
            raise ValueError(
                f"Requires XYZIFJ for a {command.Name}, missing {missing} in {command} (and in machine-state {machine_state_params})"
            )

        RequiredState = "XYZ"
        if modal_missing := [p for p in RequiredState if machine_state_params[p] is None]:
            raise ValueError(
                f"Arcs require a previous {''.join(modal_missing)} (from some movement) for {command}"
            )

        # GCODE if no dZ

        if (
            params["Z"] == machine_state_params["Z"]
        ):  # nb: works ok if Z is omitted, and state.Z is None (never seen)
            return super()._convert_arc_move(command)

        # HELIX, requires opensbp CG, command

        # We'll work in internal mm units till the final stringification

        direction = "1" if command.Name in ["G2", "G02"] else "-1"
        x_val, y_val = params["X"], params["Y"]
        i_val, j_val = params["I"], params["J"]
        z_val = params["Z"]
        # shopbot helical is relative-Z, inverted sign
        plunge = machine_state_params["Z"] - z_val

        output = []

        def arc_length_3d(center, start, end, clockwise):
            # FIXME: is there an existing fn for this?
            """center, start, end: (x, y, z) tuples
            clockwise: True for G2, False for G3
            Returns length-in-xy-plane, total_length
            """

            # FIXME: reverse direction if ccw, and test

            cx, cy, cz = center
            sx, sy, sz = start
            ex, ey, ez = end

            # ---- XY arc angle ----

            a0 = math.atan2(sy - cy, sx - cx)
            a1 = math.atan2(ey - cy, ex - cx)

            dtheta = abs(a1 - a0)

            r = math.hypot(sx - cx, sy - cy)

            arc_xy = abs(r * dtheta)

            # ---- total length is just as-if a triangle of a=arc_yx, b=dz, c=hypot
            dz = ez - sz

            # ---- true helical arc length ----
            result = (arc_xy, math.hypot(arc_xy, dz))
            return result

        def calculate_arc_speed(command_name, command_params, last_position):
            """Have to project F onto XY plane, and Z axis
            last_position is some dict with X,Y,Z,F
            command_params is some dict with XYZIJF

            return vs-speed_command
            """
            # On at least some shopbots, issuing a VS with a Z value will stutter
            # But, it seems that a CG with plunge also stutters
            # So, we issue VS...\nCG... anyway
            # It is possible to track the VS(XY,Z) and not issue the VS if no change
            # would require machine_state that holds VS(XY,Z), and only tracks runs of VS
            #   ( any other command invalidates VS(XY,Z) )
            # FIXME: ABC speeds not handled

            # we use vectors [x,y,z] so we can `map` (instead of a dict)
            start_position = [last_position[a] for a in "XYZ"]
            center_offset = [command_params[a] for a in "IJ"]
            center_offset.append(0)  # Z offset, as if K=0
            center = list(map(operator.add, start_position, center_offset))
            end_position = [command_params[a] for a in "XYZ"]

            #
            z_distance = abs(start_position[2] - end_position[2])
            xy_distance, total_distance = arc_length_3d(
                center,
                start_position,
                end_position,
                command_name == "G02",  # is it clockwise
            )

            # Nothing to do
            if abs(xy_distance) < 1e-5:
                return None

            distances_for_speed = [xy_distance, z_distance]

            # now we just need proportion of F in xy, and proportion of F in Z
            vs_speeds = [command_params["F"] / d for d in distances_for_speed]

            # opensbp native commands are units/sec, and Path.Command is too
            # but opensbp gcode is units/min (as is self._machine.feedrate_per_second)
            # format_parameter is going to *60 so we have to /60
            return f"VS,{self.format_parameter('F', vs_speeds[0]/60)},{self.format_parameter('F', vs_speeds[1]/60)}"

        last_position = self._modal_state  # FIXME: self.machine_state.getState()
        speed_command = calculate_arc_speed(command.Name, params, last_position=last_position)
        if speed_command:
            output.append(speed_command)

        output.append(
            f"CG,,{self.format_parameter('X',x_val)},{self.format_parameter('Y',y_val)},{self.format_parameter('X',i_val)},{self.format_parameter('Y',j_val)},T,{direction},{self.format_parameter('Z',plunge)}"
        )

        return "\n".join(output) if output else None

    def _convert_tool_change(self, command):
        """
        Convert tool change (M6) to OpenSBP tool commands.

        Supports both automatic and manual tool changers based on configuration.
        """
        params = command.Parameters
        tool_num = int(params.get("T", 0))

        output = []

        # Check if automatic tool changer is enabled
        has_atc = self.values["AUTOMATIC_TOOL_CHANGER"]

        # If the toolchange was generated by a ToolController, we have the name
        # but, if it was elsewhere, we may not
        # FIXME: can we look up the Tool name?
        tool_name = command.Annotations.get(
            "tool_name", str(tool_num)
        )  # FIXME: we want the Tool name, not tc name
        # FIXME: sanitize tool_name for string

        if has_atc:
            # Automatic tool changer
            output.append(f"&ToolName={tool_name}")
            output.append(f"&Tool={tool_num}")  # FIXME: we want the Tool name, not tc name
        else:
            # Manual tool change - pause and prompt
            output.append(f"'Manual tool change to T{tool_num}: {tool_name}")
            output.append(f"&ToolName={tool_name}")
            output.append(f"&Tool={tool_num}")
            output.append("PAUSE")

        return "\n".join(output)

    def _convert_spindle_command(self, command):
        """
        Convert spindle commands (M3/M4/M5) to OpenSBP TR command.

        Supports both automatic and manual spindle control based on configuration.
        """
        params = command.Parameters
        has_auto_spindle = self.values["AUTOMATIC_SPINDLE"]

        if command.Name in ["M5", "M05"]:
            # Spindle off
            if has_auto_spindle:
                return "TR,0"
            else:
                return "'Turn spindle OFF manually\nPAUSE"

        # Spindle on (M3/M4)
        rpm = int(params.get("S", 0))

        output = []

        if has_auto_spindle:
            # Automatic spindle control
            formatted = self.format_parameter(
                "S", rpm
            )  # .rstrip('0').rstrip('.') # FIXME: example trailing zero
            output.append(f"TR,{formatted}")

            # FIXME: the default behavior is no delay unless spindle_wait is specified, default delay?
            spindle = self._machine.get_spindle_by_index(0)
            if not (spindle and spindle.spindle_wait > 0):
                output.append("PAUSE 2")  # Wait for spindle to reach speed
        else:
            # Manual spindle control - prompt user
            output.append(f"'Set spindle to {rpm} RPM ({rpm/60:.1f}Hz) and start manually")
            output.append("PAUSE")

        return "\n".join(output)

    def _convert_program_control(self, command: Path.Command) -> str:
        if command.Name in (Constants.MCODE_END + Constants.MCODE_END_RESET):
            return "END"
        else:
            return super()._convert_program_control(command)

    def _quote(self, string):
        """Return a string that is safe for double-quotes (for opensbp)"""
        # very conservative: only alpha-numeric and /-_.
        return re.sub(r"[^A-Za-z0-9/_ .-]", "", string)

    def _convert_probe_open(self, command):
        """We need to setup for this probe-sequence,
        provide subroutines for this/other probe-sequences.
        The command should be a comment, and is already handled by
        a _convert_comment().
        But, has an annotation for the file-name from the Probe Operation
        """

        # we allow "/", "../", etc., in the filename
        # but not things like "c:".
        filename = command.Annotations["probe_open"]
        if "." not in filename:
            # default .txt (really "space delimited values")
            filename += ".txt"
        filename = self._quote(filename)

        rez = [
            # we already handled the probe-open comment
            "C#,90",  # Loads "my variables", notably &my_ZzeroInput
            f'OPEN "{filename}" FOR OUTPUT as #1',
        ]

        # only insert subroutines once
        if self._first_probe_open:
            self._first_probe_open = False
            self.values["POST_JOB"] += textwrap.dedent("""\
                GOTO SkipProbeSubRoutines
                CaptureZPos:
                  ' for g38.2 probe, write the data on probe-contact
                  ' and set flag for didn't-fail
                  ' xyzab
                  WRITE #1; %(1); " "; %(2); " "; %(3); " "; %(4); " "; %(5)
                  &hit = 1
                  RETURN
                FailedToTouch:
                  ' for g38.2 probe, when
                  ' failed to trigger w/in movement
                  MSGBOX(Failed to touch...Exiting,16,Probe Failed) # fixme: which job/op label, and file?
                  END
                SkipProbeSubRoutines:
            """).rstrip()

        return "\n".join(rez)

    def _convert_probe_close(self, command):
        return textwrap.dedent("""\
            '(PROBECLOSE)
            'Clear probe-switch-trigger
            ON INPUT(&my_ZzeroInput, 1)
            CLOSE #1
        """).rstrip()

    def _convert_probe(self, command):
        """
        Converts a probe command (G38.2) to gcode.
        _convert_probe_open(command) was already called to start the sequence
        Probe.opExecute generated various move commands, and are handled as normal.
        _convert_probe_close(command) will-be called to end the sequence
        """

        # We are being strict here, Z motion only
        required = {p: v for p, v in command.Parameters.items() if p in "ZF"}
        # FIXME: allow default F from MachineState when implemented?
        if len(required) != 2:
            raise Exception(f"A probing move (G38.2) must have a Z and F, only saw: {command}")
        if len(command.Parameters) > 2:
            raise Exception(f"A probing move (G38.2) should only have Z and F, saw {command}")

        # G1, we aren't jogging, we are doing a slow, deliberate move, i.e. ~"feed".
        probe_movement = self._convert_move(Path.Command("G1", required))

        # &hit is set to 1 if the touch happens (see subroutine in _convert_probe_open)
        rez = textwrap.dedent(f"""\
            &hit = 0
            ON INPUT(&my_ZzeroInput, 1) GOSUB CaptureZPos
            {probe_movement}
            IF &hit = 0 THEN GOTO FailedToTouch
        """).rstrip()

        return rez

    def _optimize_gcode(self, gcode_lines) -> str:
        # There may be opensbp in the stream
        # so, you can't know the state for modal and axis-modal
        # FIXME: this override goes away when Processor's does

        disable = "OUTPUT_DUPLICATE_COMMANDS FILTER_INEFFICIENT_MOVES OUTPUT_LINE_NUMBERS".split(
            " "
        )
        was = {k: self.values[k] for k in disable}

        for k in disable:
            self.values[k] = False
        self.values["OUTPUT_DUPLICATE_COMMANDS"] = True

        try:
            return super()._optimize_gcode(gcode_lines)
        finally:
            for k in disable:
                self.values[k] = was[k]

    @property
    def tooltip(self):
        tooltip: str = """
        This is a postprocessor file for the CAM workbench.
        It is used to take a pseudo-gcode fragment from a CAM object
        and output OpenSBP code suitable for ShopBot CNC controllers.

        OpenSBP use GCode for most moves, native for some functionality
        CG for arcs, TR for spindle speed, etc.
        """
        return tooltip


# Class alias for PostProcessorFactory
# The factory looks for a class with title-cased postname (e.g., "Opensbp")
Opensbp = OpenSBPPost


# Factory function for creating the postprocessor
def create(job, **kwargs):
    """
    Factory function to create an OpenSBP postprocessor instance.
    """
    return OpenSBPPost(job, **kwargs)
