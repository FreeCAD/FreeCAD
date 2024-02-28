# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
# *   Copyright (c) 2018, 2019 Gauthier Briere                              *
# *   Copyright (c) 2019, 2020 Schildkroet                                  *
# *   Copyright (c) 2022 Larry Woestman <LarryWoestman2@gmail.com>          *
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

import re
from typing import Any, Callable, Dict, List, Tuple, Union

import FreeCAD
from FreeCAD import Units

import Path

# Define some types that are used throughout this file
CommandLine = List[str]
Gcode = List[str]
PathParameter = float
PathParameters = Dict[str, PathParameter]
Values = Dict[str, Any]

ParameterFunction = Callable[[Values, str, str, PathParameter, PathParameters], str]


def check_for_an_adaptive_op(
    values: Values,
    command: str,
    command_line: CommandLine,
    adaptive_op_variables: Tuple[bool, float, float],
) -> str:
    """Check to see if the current command is an adaptive op."""
    adaptiveOp: bool
    nl: str = "\n"
    opHorizRapid: float
    opVertRapid: float

    (adaptiveOp, opHorizRapid, opVertRapid) = adaptive_op_variables
    if values["OUTPUT_ADAPTIVE"] and adaptiveOp and command in values["RAPID_MOVES"]:
        if opHorizRapid and opVertRapid:
            return "G1"
        command_line.append(f"(Tool Controller Rapid Values are unset){nl}")
    return ""


def check_for_drill_translate(
    values: Values,
    gcode: Gcode,
    command: str,
    command_line: CommandLine,
    params: PathParameters,
    motion_location: PathParameters,
    drill_retract_mode: str,
) -> bool:
    """Check for drill commands to translate."""
    comment: str
    nl: str = "\n"

    if (
        values["TRANSLATE_DRILL_CYCLES"]
        and command in values["DRILL_CYCLES_TO_TRANSLATE"]
    ):
        if values["OUTPUT_COMMENTS"]:  # Comment the original command
            comment = create_comment(
                values,
                values["COMMAND_SPACE"]
                + format_command_line(values, command_line)
                + values["COMMAND_SPACE"],
            )
            gcode += f"{linenumber(values)}{comment}{nl}"
        # wrap this block to ensure that the value of values["MOTION_MODE"]
        # is restored in case of error
        try:
            drill_translate(
                values,
                gcode,
                command,
                params,
                motion_location,
                drill_retract_mode,
            )
        except (ArithmeticError, LookupError) as err:
            print("exception occurred", err)
        # drill_translate uses G90 mode internally, so need to
        # switch back to G91 mode if it was that way originally
        if values["MOTION_MODE"] == "G91":
            gcode.append(f"{linenumber(values)}G91{nl}")
        return True
    return False


def check_for_machine_specific_commands(
    values: Values, gcode: Gcode, command: str
) -> None:
    """Check for comments containing machine-specific commands."""
    m: object
    nl: str = "\n"
    raw_command: str

    if values["ENABLE_MACHINE_SPECIFIC_COMMANDS"]:
        m = re.match(r"^\(MC_RUN_COMMAND: ([^)]+)\)$", command)
        if m:
            raw_command = m.group(1)
            # pass literally to the controller
            gcode += f"{linenumber(values)}{raw_command}{nl}"


def check_for_spindle_wait(
    values: Values, gcode: Gcode, command: str, command_line: CommandLine
) -> None:
    """Check for commands that might need a wait command after them."""
    cmd: str
    nl: str = "\n"

    if values["SPINDLE_WAIT"] > 0 and command in ("M3", "M03", "M4", "M04"):
        gcode += f"{linenumber(values)}{format_command_line(values, command_line)}{nl}"
        cmd = format_command_line(values, ["G4", f'P{values["SPINDLE_WAIT"]}'])
        gcode += f"{linenumber(values)}{cmd}{nl}"


def check_for_suppressed_commands(
    values: Values, gcode: Gcode, command: str, command_line: CommandLine
) -> bool:
    """Check for commands that will be suppressed."""
    comment: str
    nl: str = "\n"

    if command in values["SUPPRESS_COMMANDS"]:
        if values["OUTPUT_COMMENTS"]:
            # convert the command to a comment
            comment = create_comment(
                values,
                values["COMMAND_SPACE"]
                + format_command_line(values, command_line)
                + values["COMMAND_SPACE"],
            )
            gcode += f"{linenumber(values)}{comment}{nl}"
        # remove the command
        return True
    return False


def check_for_tlo(
    values: Values, gcode: Gcode, command: str, params: PathParameters
) -> None:
    """Output a tool length command if USE_TLO is True."""
    nl: str = "\n"

    if command in ("M6", "M06") and values["USE_TLO"]:
        cmd = format_command_line(values, ["G43", f'H{str(int(params["T"]))}'])
        gcode += f"{linenumber(values)}{cmd}{nl}"


def check_for_tool_change(
    values: Values, gcode: Gcode, command: str, command_line: CommandLine
) -> bool:
    """Check for a tool change."""
    nl: str = "\n"

    if command in ("M6", "M06"):
        if values["OUTPUT_COMMENTS"]:
            comment = create_comment(values, "Begin toolchange")
            gcode += f"{linenumber(values)}{comment}{nl}"
        if values["OUTPUT_TOOL_CHANGE"]:
            if values["STOP_SPINDLE_FOR_TOOL_CHANGE"]:
                # stop the spindle
                gcode += f"{linenumber(values)}M5{nl}"
            for line in values["TOOL_CHANGE"].splitlines(False):
                gcode += f"{linenumber(values)}{line}{nl}"
        elif values["OUTPUT_COMMENTS"]:
            # convert the tool change to a comment
            comment = create_comment(
                values,
                values["COMMAND_SPACE"]
                + format_command_line(values, command_line)
                + values["COMMAND_SPACE"],
            )
            gcode += f"{linenumber(values)}{comment}{nl}"
            return True
    return False


def create_comment(values: Values, comment_string: str) -> str:
    """Create a comment from a string using the correct comment symbol."""
    if values["COMMENT_SYMBOL"] == "(":
        return f"({comment_string})"
    return values["COMMENT_SYMBOL"] + comment_string


def default_axis_parameter(
    values: Values,
    command: str,  # pylint: disable=unused-argument
    param: str,
    param_value: PathParameter,
    current_location: PathParameters,
) -> str:
    """Process an axis parameter."""
    if (
        not values["OUTPUT_DOUBLES"]
        and param in current_location
        and current_location[param] == param_value
    ):
        return ""
    return format_for_axis(values, Units.Quantity(param_value, Units.Length))


def default_D_parameter(
    values: Values,
    command: str,
    param: str,  # pylint: disable=unused-argument
    param_value: PathParameter,
    current_location: PathParameters,  # pylint: disable=unused-argument
) -> str:
    """Process the D parameter."""
    if command in ("G41", "G42"):
        return str(int(param_value))
    if command in ("G41.1", "G42.1"):
        return format_for_axis(values, Units.Quantity(param_value, Units.Length))
    if command in ("G96", "G97"):
        return format_for_spindle(values, param_value)
    # anything else that is supported
    return str(float(param_value))


def default_F_parameter(
    values: Values,
    command: str,
    param: str,
    param_value: PathParameter,
    current_location: PathParameters,
) -> str:
    """Process the F parameter."""
    if (
        not values["OUTPUT_DOUBLES"]
        and param in current_location
        and current_location[param] == param_value
    ):
        return ""
    # Many posts don't use rapid speeds, but eventually
    # there will be refactored posts that do, so this
    # "if statement" is being kept separate to make it
    # more obvious where to put that check.
    if command in values["RAPID_MOVES"]:
        return ""
    feed = Units.Quantity(param_value, Units.Velocity)
    if feed.getValueAs(values["UNIT_SPEED_FORMAT"]) <= 0.0:
        return ""
    return format_for_feed(values, feed)


def default_int_parameter(
    values: Values,  # pylint: disable=unused-argument
    command: str,  # pylint: disable=unused-argument
    param: str,  # pylint: disable=unused-argument
    param_value: PathParameter,
    current_location: PathParameters,  # pylint: disable=unused-argument
) -> str:
    """Process a parameter that is treated like an integer."""
    return str(int(param_value))


def default_length_parameter(
    values: Values,
    command: str,  # pylint: disable=unused-argument
    param: str,  # pylint: disable=unused-argument
    param_value: PathParameter,
    current_location: PathParameters,  # pylint: disable=unused-argument
) -> str:
    """Process a parameter that is treated like a length."""
    return format_for_axis(values, Units.Quantity(param_value, Units.Length))


def default_P_parameter(
    values: Values,
    command: str,
    param: str,  # pylint: disable=unused-argument
    param_value: PathParameter,
    current_location: PathParameters,  # pylint: disable=unused-argument
) -> str:
    """Process the P parameter."""
    if command in ("G2", "G02", "G3", "G03", "G5.2", "G5.3", "G10", "G54.1", "G59"):
        return str(int(param_value))
    if command in ("G4", "G04", "G76", "G82", "G86", "G89"):
        return str(float(param_value))
    if command in ("G5", "G05", "G64"):
        return format_for_axis(values, Units.Quantity(param_value, Units.Length))
    # anything else that is supported
    return str(param_value)


def default_Q_parameter(
    values: Values,
    command: str,
    param: str,  # pylint: disable=unused-argument
    param_value: PathParameter,
    current_location: PathParameters,  # pylint: disable=unused-argument
) -> str:
    """Process the Q parameter."""
    if command == "G10":
        return str(int(param_value))
    if command in ("G64", "G73", "G83"):
        return format_for_axis(values, Units.Quantity(param_value, Units.Length))
    return ""


def default_S_parameter(
    values: Values,
    command: str,  # pylint: disable=unused-argument
    param: str,  # pylint: disable=unused-argument
    param_value: PathParameter,
    current_location: PathParameters,  # pylint: disable=unused-argument
) -> str:
    """Process the S parameter."""
    return format_for_spindle(values, param_value)


def determine_adaptive_op(values: Values, pathobj) -> Tuple[bool, float, float]:
    """Determine if the pathobj contains an Adaptive operation."""
    nl = "\n"
    adaptiveOp: bool = False
    opHorizRapid: float = 0.0
    opVertRapid: float = 0.0

    if values["OUTPUT_ADAPTIVE"] and "Adaptive" in pathobj.Name:
        adaptiveOp = True
        if hasattr(pathobj, "ToolController"):
            tc = pathobj.ToolController
            if hasattr(tc, "HorizRapid") and tc.HorizRapid > 0:
                opHorizRapid = Units.Quantity(tc.HorizRapid, Units.Velocity)
            else:
                FreeCAD.Console.PrintWarning(
                    f"Tool Controller Horizontal Rapid Values are unset{nl}"
                )
            if hasattr(tc, "VertRapid") and tc.VertRapid > 0:
                opVertRapid = Units.Quantity(tc.VertRapid, Units.Velocity)
            else:
                FreeCAD.Console.PrintWarning(
                    f"Tool Controller Vertical Rapid Values are unset{nl}"
                )
    return (adaptiveOp, opHorizRapid, opVertRapid)


def drill_translate(
    values: Values,
    gcode: Gcode,
    command: str,
    params: PathParameters,
    motion_location: PathParameters,
    drill_retract_mode: str,
) -> None:
    """Translate drill cycles.

    Currently only cycles in XY are provided (G17).
    XZ (G18) and YZ (G19) are not dealt with.
    In other words only Z drilling can be translated.
    """
    cmd: str
    comment: str
    drill_x: float
    drill_y: float
    drill_z: float
    motion_z: float
    nl: str = "\n"
    retract_z: float
    F_feedrate: str
    G0_retract_z: str

    if values["MOTION_MODE"] == "G91":
        # force absolute coordinates during cycles
        gcode.append(f"{linenumber(values)}G90{nl}")

    drill_x = Units.Quantity(params["X"], Units.Length)
    drill_y = Units.Quantity(params["Y"], Units.Length)
    drill_z = Units.Quantity(params["Z"], Units.Length)
    retract_z = Units.Quantity(params["R"], Units.Length)
    if retract_z < drill_z:  # R less than Z is error
        comment = create_comment(values, "Drill cycle error: R less than Z")
        gcode.append(f"{linenumber(values)}{comment}{nl}")
        return
    motion_z = Units.Quantity(motion_location["Z"], Units.Length)
    if values["MOTION_MODE"] == "G91":  # relative movements
        drill_x += Units.Quantity(motion_location["X"], Units.Length)
        drill_y += Units.Quantity(motion_location["Y"], Units.Length)
        drill_z += motion_z
        retract_z += motion_z
    if drill_retract_mode == "G98" and motion_z >= retract_z:
        retract_z = motion_z

    cmd = format_command_line(values, ["G0", f"Z{format_for_axis(values, retract_z)}"])
    G0_retract_z = f"{cmd}{nl}"
    cmd = format_for_feed(values, Units.Quantity(params["F"], Units.Velocity))
    F_feedrate = f'{values["COMMAND_SPACE"]}F{cmd}{nl}'

    # preliminary movement(s)
    if motion_z < retract_z:
        gcode.append(f"{linenumber(values)}{G0_retract_z}")
    cmd = format_command_line(
        values,
        [
            "G0",
            f"X{format_for_axis(values, drill_x)}",
            f"Y{format_for_axis(values, drill_y)}",
        ],
    )
    gcode.append(f"{linenumber(values)}{cmd}{nl}")
    if motion_z > retract_z:
        # NIST GCODE 3.5.16.1 Preliminary and In-Between Motion says G0 to retract_z
        # Here use G1 since retract height may be below surface !
        cmd = format_command_line(
            values, ["G1", f"Z{format_for_axis(values, retract_z)}"]
        )
        gcode.append(f"{linenumber(values)}{cmd}{F_feedrate}")

        # drill moves
    if command in ("G81", "G82"):
        output_G81_G82_drill_moves(
            values, gcode, command, params, drill_z, F_feedrate, G0_retract_z
        )
    elif command in ("G73", "G83"):
        output_G73_G83_drill_moves(
            values, gcode, command, params, drill_z, retract_z, F_feedrate, G0_retract_z
        )


def format_command_line(values: Values, command_line: CommandLine) -> str:
    """Construct the command line for the final output."""
    return values["COMMAND_SPACE"].join(command_line)


def format_for_axis(values: Values, number) -> str:
    """Format a number using the precision for an axis value."""
    return str(
        format(
            float(number.getValueAs(values["UNIT_FORMAT"])),
            f'.{str(values["AXIS_PRECISION"])}f',
        )
    )


def format_for_feed(values: Values, number) -> str:
    """Format a number using the precision for a feed rate."""
    return str(
        format(
            float(number.getValueAs(values["UNIT_SPEED_FORMAT"])),
            f'.{str(values["FEED_PRECISION"])}f',
        )
    )


def format_for_spindle(values: Values, number) -> str:
    """Format a number using the precision for a spindle speed."""
    return str(format(float(number), f'.{str(values["SPINDLE_DECIMALS"])}f'))


def init_parameter_functions(parameter_functions: Dict[str, ParameterFunction]) -> None:
    """Initialize a list of parameter functions.

    These functions are called in the UtilsParse.parse_a_path
    function to return the appropriate parameter value.
    """
    default_parameter_functions: Dict[str, ParameterFunction]
    parameter: str

    default_parameter_functions = {
        "A": default_axis_parameter,
        "B": default_axis_parameter,
        "C": default_axis_parameter,
        "D": default_D_parameter,
        "E": default_length_parameter,
        "F": default_F_parameter,
        # "G" is reserved for G-code commands
        "H": default_int_parameter,
        "I": default_length_parameter,
        "J": default_length_parameter,
        "K": default_length_parameter,
        "L": default_int_parameter,
        # "M" is reserved for M-code commands
        # "N" is reserved for the line numbers
        # "O" is reserved for the line numbers for subroutines
        "P": default_P_parameter,
        "Q": default_Q_parameter,
        "R": default_length_parameter,
        "S": default_S_parameter,
        "T": default_int_parameter,
        "U": default_axis_parameter,
        "V": default_axis_parameter,
        "W": default_axis_parameter,
        "X": default_axis_parameter,
        "Y": default_axis_parameter,
        "Z": default_axis_parameter,
        # "$" is used by LinuxCNC (and others?) to designate which spindle
    }
    for (
        parameter
    ) in default_parameter_functions:  # pylint: disable=consider-using-dict-items
        parameter_functions[parameter] = default_parameter_functions[parameter]


def linenumber(values: Values, space: Union[str, None] = None) -> str:
    """Output the next line number if appropriate."""
    line_num: str

    if not values["OUTPUT_LINE_NUMBERS"]:
        return ""
    if space is None:
        space = values["COMMAND_SPACE"]
    line_num = str(values["line_number"])
    values["line_number"] += values["LINE_INCREMENT"]
    return f"N{line_num}{space}"


def output_G73_G83_drill_moves(
    values: Values,
    gcode: Gcode,
    command: str,
    params: PathParameters,
    drill_z: float,
    retract_z: float,
    F_feedrate: str,
    G0_retract_z: str,
) -> None:
    """Output the movement G code for G73 and G83."""
    a_bit: float
    chip_breaker_height: float
    clearance_depth: float
    cmd: str
    drill_step: float
    last_stop_z: float
    next_stop_z: float
    nl: str = "\n"

    last_stop_z = retract_z
    drill_step = Units.Quantity(params["Q"], Units.Length)
    # NIST 3.5.16.4 G83 Cycle:  "current hole bottom, backed off a bit."
    a_bit = drill_step * 0.05
    if drill_step != 0:
        while True:
            if last_stop_z != retract_z:
                # rapid move to just short of last drilling depth
                clearance_depth = last_stop_z + a_bit
                cmd = format_command_line(
                    values,
                    ["G0", f"Z{format_for_axis(values, clearance_depth)}"],
                )
                gcode.append(f"{linenumber(values)}{cmd}{nl}")
            next_stop_z = last_stop_z - drill_step
            if next_stop_z > drill_z:
                cmd = format_command_line(
                    values, ["G1", f"Z{format_for_axis(values, next_stop_z)}"]
                )
                gcode.append(f"{linenumber(values)}{cmd}{F_feedrate}")
                if command == "G73":
                    # Rapid up "a small amount".
                    chip_breaker_height = next_stop_z + values["CHIPBREAKING_AMOUNT"]
                    cmd = format_command_line(
                        values,
                        [
                            "G0",
                            f"Z{format_for_axis(values, chip_breaker_height)}",
                        ],
                    )
                    gcode.append(f"{linenumber(values)}{cmd}{nl}")
                elif command == "G83":
                    # Rapid up to the retract height
                    gcode.append(f"{linenumber(values)}{G0_retract_z}")
                last_stop_z = next_stop_z
            else:
                cmd = format_command_line(
                    values, ["G1", f"Z{format_for_axis(values, drill_z)}"]
                )
                gcode.append(f"{linenumber(values)}{cmd}{F_feedrate}")
                gcode.append(f"{linenumber(values)}{G0_retract_z}")
                break


def output_G81_G82_drill_moves(
    values: Values,
    gcode: Gcode,
    command: str,
    params: PathParameters,
    drill_z: float,
    F_feedrate: str,
    G0_retract_z: str,
) -> None:
    """Output the movement G code for G81 and G82."""
    cmd: str
    nl: str = "\n"

    cmd = format_command_line(values, ["G1", f"Z{format_for_axis(values, drill_z)}"])
    gcode.append(f"{linenumber(values)}{cmd}{F_feedrate}")
    # pause where applicable
    if command == "G82":
        cmd = format_command_line(values, ["G4", f'P{str(params["P"])}'])
        gcode.append(f"{linenumber(values)}{cmd}{nl}")
    gcode.append(f"{linenumber(values)}{G0_retract_z}")


def parse_a_group(values: Values, gcode: Gcode, pathobj) -> None:
    """Parse a Group (compound, project, or simple path)."""
    comment: str
    nl: str = "\n"

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        if values["OUTPUT_COMMENTS"]:
            comment = create_comment(values, f"Compound: {pathobj.Label}")
            gcode += f"{linenumber(values)}{comment}{nl}"
        for p in pathobj.Group:
            parse_a_group(values, gcode, p)
    else:  # parsing simple path
        # groups might contain non-path things like stock.
        if not hasattr(pathobj, "Path"):
            return
        if values["OUTPUT_PATH_LABELS"] and values["OUTPUT_COMMENTS"]:
            comment = create_comment(values, f"Path: {pathobj.Label}")
            gcode += f"{linenumber(values)}{comment}{nl}"
        parse_a_path(values, gcode, pathobj)


def parse_a_path(values: Values, gcode: Gcode, pathobj) -> None:
    """Parse a simple Path."""
    adaptive_op_variables: Tuple[bool, float, float]
    cmd: str
    command: str
    command_line: CommandLine
    current_location: PathParameters = {}  # keep track for no doubles
    drill_retract_mode: str = "G98"
    lastcommand: str = ""
    motion_location: PathParameters = {}  # keep track of last motion location
    nl: str = "\n"
    parameter: str
    parameter_value: str

    current_location.update(
        Path.Command("G0", {"X": -1, "Y": -1, "Z": -1, "F": 0.0}).Parameters
    )
    adaptive_op_variables = determine_adaptive_op(values, pathobj)

    for c in pathobj.Path.Commands:
        command = c.Name
        command_line = []

        # Modify the command name if necessary
        if command[0] == "(":
            if not values["OUTPUT_COMMENTS"]:
                continue
            if values["COMMENT_SYMBOL"] != "(" and len(command) > 2:
                command = create_comment(values, command[1:-1])
        cmd = check_for_an_adaptive_op(
            values, command, command_line, adaptive_op_variables
        )
        if cmd:
            command = cmd
        # Add the command name to the command line
        command_line.append(command)
        # if modal: suppress the command if it is the same as the last one
        if values["MODAL"] and command == lastcommand:
            command_line.pop(0)

        # Now add the remaining parameters in order
        for parameter in values["PARAMETER_ORDER"]:
            if parameter in c.Parameters:
                parameter_value = values["PARAMETER_FUNCTIONS"][parameter](
                    values,
                    command,
                    parameter,
                    c.Parameters[parameter],
                    current_location,
                )
                if parameter_value:
                    command_line.append(f"{parameter}{parameter_value}")

        set_adaptive_op_speed(
            values, command, command_line, c.Parameters, adaptive_op_variables
        )
        # Remember the current command
        lastcommand = command
        # Remember the current location
        current_location.update(c.Parameters)
        if command in ("G90", "G91"):
            # Remember the motion mode
            values["MOTION_MODE"] = command
        elif command in ("G98", "G99"):
            # Remember the drill retract mode for drill_translate
            drill_retract_mode = command
        if command in values["MOTION_COMMANDS"]:
            # Remember the current location for drill_translate
            motion_location.update(c.Parameters)
        if check_for_drill_translate(
            values,
            gcode,
            command,
            command_line,
            c.Parameters,
            motion_location,
            drill_retract_mode,
        ):
            command_line = []
        check_for_spindle_wait(values, gcode, command, command_line)
        if check_for_tool_change(values, gcode, command, command_line):
            command_line = []
        if check_for_suppressed_commands(values, gcode, command, command_line):
            command_line = []
        # Add a line number to the front and a newline to the end of the command line
        if command_line:
            gcode += (
                f"{linenumber(values)}{format_command_line(values, command_line)}{nl}"
            )
        check_for_tlo(values, gcode, command, c.Parameters)
        check_for_machine_specific_commands(values, gcode, command)


def set_adaptive_op_speed(
    values: Values,
    command: str,
    command_line: CommandLine,
    params: PathParameters,
    adaptive_op_variables: Tuple[bool, float, float],
) -> None:
    """Set the appropriate feed speed for an adaptive op."""
    adaptiveOp: bool
    opHorizRapid: float
    opVertRapid: float
    param_num: str

    (adaptiveOp, opHorizRapid, opVertRapid) = adaptive_op_variables
    if (
        values["OUTPUT_ADAPTIVE"]
        and adaptiveOp
        and command in values["RAPID_MOVES"]
        and opHorizRapid
        and opVertRapid
    ):
        if "Z" not in params:
            param_num = format_for_feed(values, opHorizRapid)
        else:
            param_num = format_for_feed(values, opVertRapid)
        command_line.append(f"F{param_num}")
