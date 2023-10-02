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

import FreeCAD
from FreeCAD import Units

import Path
import Path.Post.Utils as PostUtils
import PathScripts.PathUtils as PathUtils

def create_comment(values, comment_string):
    """Create a comment from a string using the correct comment symbol."""
    if values["COMMENT_SYMBOL"] == "(":
        return f"({comment_string})"
    else:
        return values["COMMENT_SYMBOL"] + comment_string


def default_axis_parameter(values, command, param, param_value, currLocation):
    """Process an axis parameter."""
    if (
        not values["OUTPUT_DOUBLES"]
        and param in currLocation
        and currLocation[param] == param_value
    ):
        return None
    return format_for_axis(values, Units.Quantity(param_value, Units.Length))


def default_D_parameter(values, command, param, param_value, currLocation):
    """Process the D parameter."""
    if command in ("G41", "G42"):
        return str(int(param_value))
    if command in ("G41.1", "G42.1"):
        return format_for_axis(values, Units.Quantity(param_value, Units.Length))
    if command in ("G96", "G97"):
        return format_for_spindle(values, param_value)
    # anything else that is supported
    return str(float(param_value))


def default_F_parameter(values, command, param, param_value, currLocation):
    """Process the F parameter."""
    if (
        not values["OUTPUT_DOUBLES"]
        and param in currLocation
        and currLocation[param] == param_value
    ):
        return None
    # Many posts don't use rapid speeds, but eventually
    # there will be refactored posts that do, so this
    # "if statement" is being kept separate to make it
    # more obvious where to put that check.
    if command in values["RAPID_MOVES"]:
        return None
    feed = Units.Quantity(param_value, Units.Velocity)
    if feed.getValueAs(values["UNIT_SPEED_FORMAT"]) <= 0.0:
        return None
    return format_for_feed(values, feed)


def default_int_parameter(values, command, param, param_value, currLocation):
    """Process a parameter that is treated like an integer."""
    return str(int(param_value))


def default_length_parameter(values, command, param, param_value, currLocation):
    """Process a parameter that is treated like a length."""
    return format_for_axis(values, Units.Quantity(param_value, Units.Length))


def default_P_parameter(values, command, param, param_value, currLocation):
    """Process the P parameter."""
    if command in ("G2", "G02", "G3", "G03", "G5.2", "G5.3", "G10", "G54.1", "G59"):
        return str(int(param_value))
    if command in ("G4", "G04", "G76", "G82", "G86", "G89"):
        return str(float(param_value))
    if command in ("G5", "G05", "G64"):
        return format_for_axis(values, Units.Quantity(param_value, Units.Length))
    # anything else that is supported
    return str(param_value)


def default_Q_parameter(values, command, param, param_value, currLocation):
    """Process the Q parameter."""
    if command == "G10":
        return str(int(param_value))
    if command in ("G64", "G73", "G83"):
        return format_for_axis(values, Units.Quantity(param_value, Units.Length))


def default_S_parameter(values, command, param, param_value, currLocation):
    """Process the S parameter."""
    return format_for_spindle(values, param_value)


def drill_translate(values, cmd, params):
    """Translate drill cycles."""
    trBuff = ""
    nl = "\n"

    # cycle conversion
    # currently only cycles in XY are provided (G17)
    # other plains ZX (G18) and  YZ (G19) are not dealt with : Z drilling only.
    drill_X = Units.Quantity(params["X"], Units.Length)
    drill_Y = Units.Quantity(params["Y"], Units.Length)
    drill_Z = Units.Quantity(params["Z"], Units.Length)
    RETRACT_Z = Units.Quantity(params["R"], Units.Length)
    # R less than Z is error
    if RETRACT_Z < drill_Z:
        comment = create_comment(values, "Drill cycle error: R less than Z")
        trBuff += f"{linenumber(values)}{comment}{nl}"
        return trBuff

    if values["MOTION_MODE"] == "G91":  # G91 relative movements
        drill_X += values["CURRENT_X"]
        drill_Y += values["CURRENT_Y"]
        drill_Z += values["CURRENT_Z"]
        RETRACT_Z += values["CURRENT_Z"]

    if values["DRILL_RETRACT_MODE"] == "G98" and values["CURRENT_Z"] >= RETRACT_Z:
        RETRACT_Z = values["CURRENT_Z"]

    # get the other parameters
    drill_feedrate = Units.Quantity(params["F"], Units.Velocity)
    if cmd in ("G73", "G83"):
        drill_Step = Units.Quantity(params["Q"], Units.Length)
        # NIST 3.5.16.4 G83 Cycle:  "current hole bottom, backed off a bit."
        a_bit = drill_Step * 0.05
    elif cmd == "G82":
        drill_DwellTime = params["P"]

    # wrap this block to ensure machine's values["MOTION_MODE"] is restored
    # in case of error
    try:
        if values["MOTION_MODE"] == "G91":
            # force absolute coordinates during cycles
            trBuff += f"{linenumber(values)}G90{nl}"
        strG0_RETRACT_Z = f"G0 Z{format_for_axis(values, RETRACT_Z)}{nl}"
        strF_Feedrate = f" F{format_for_feed(values, drill_feedrate)}{nl}"
        # print(strF_Feedrate)

        # preliminary movement(s)
        if values["CURRENT_Z"] < RETRACT_Z:
            trBuff += f"{linenumber(values)}{strG0_RETRACT_Z}"
        num_x = format_for_axis(values, drill_X)
        num_y = format_for_axis(values, drill_Y)
        trBuff += f"{linenumber(values)}G0 X{num_x} Y{num_y}{nl}"
        if values["CURRENT_Z"] > RETRACT_Z:
            # NIST GCODE 3.5.16.1 Preliminary and In-Between Motion says G0 to RETRACT_Z
            # Here use G1 since retract height may be below surface !
            num_z = format_for_axis(values, RETRACT_Z)
            trBuff += f"{linenumber(values)}G1 Z{num_z}{strF_Feedrate}"
        last_Stop_Z = RETRACT_Z

        # drill moves
        if cmd in ("G81", "G82"):
            num_z = format_for_axis(values, drill_Z)
            trBuff += f"{linenumber(values)}G1 Z{num_z}{strF_Feedrate}"
            # pause where applicable
            if cmd == "G82":
                trBuff += f"{linenumber(values)}G4 P{str(drill_DwellTime)}{nl}"
            trBuff += f"{linenumber(values)}{strG0_RETRACT_Z}"
        elif cmd in ("G73", "G83"):
            if params["Q"] != 0:
                while 1:
                    if last_Stop_Z != RETRACT_Z:
                        # rapid move to just short of last drilling depth
                        clearance_depth = last_Stop_Z + a_bit
                        num_z = format_for_axis(values, clearance_depth)
                        trBuff += f"{linenumber(values)}G0 Z{num_z}{nl}"
                    next_Stop_Z = last_Stop_Z - drill_Step
                    if next_Stop_Z > drill_Z:
                        num_z = format_for_axis(values, next_Stop_Z)
                        trBuff += f"{linenumber(values)}G1 Z{num_z}{strF_Feedrate}"
                        if cmd == "G73":
                            # Rapid up "a small amount".
                            chip_breaker_height = (
                                next_Stop_Z + values["CHIPBREAKING_AMOUNT"]
                            )
                            num_z = format_for_axis(values, chip_breaker_height)
                            trBuff += f"{linenumber(values)}G0 Z{num_z}{nl}"
                        elif cmd == "G83":
                            # Rapid up to the retract height
                            trBuff += f"{linenumber(values)}{strG0_RETRACT_Z}"
                        last_Stop_Z = next_Stop_Z
                    else:
                        num_z = format_for_axis(values, drill_Z)
                        trBuff += f"{linenumber(values)}G1 Z{num_z}{strF_Feedrate}"
                        trBuff += f"{linenumber(values)}{strG0_RETRACT_Z}"
                        break
    except Exception as err:
        print("exception occurred", err)

    if values["MOTION_MODE"] == "G91":
        trBuff += f"{linenumber(values)}G91{nl}"  # Restore if changed

    return trBuff


def format_for_axis(values, number):
    """Format a number using the precision for an axis value."""
    return format(
        float(number.getValueAs(values["UNIT_FORMAT"])),
        f'.{str(values["AXIS_PRECISION"])}f',
    )


def format_for_feed(values, number):
    """Format a number using the precision for a feed rate."""
    return format(
        float(number.getValueAs(values["UNIT_SPEED_FORMAT"])),
        f'.{str(values["FEED_PRECISION"])}f',
    )


def format_for_spindle(values, number):
    """Format a number using the precision for a spindle speed."""
    return format(float(number), f'.{str(values["SPINDLE_DECIMALS"])}f')


def format_outstring(values, strTable):
    """Construct the line for the final output."""
    s = ""
    for w in strTable:
        s += f'{w}{values["COMMAND_SPACE"]}'
    s = s.strip()
    return s


def init_parameter_functions(parameter_functions):
    """Initialize a list of parameter functions.

    These functions are called in the PostUtilsParse.parse_a_path
    function to return the appropriate parameter value.
    """
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
    parameter_functions.update(default_parameter_functions)


def linenumber(values, space=None):
    """Output the next line number if appropriate."""
    if values["OUTPUT_LINE_NUMBERS"]:
        if space is None:
            space = values["COMMAND_SPACE"]
        line_num = str(values["line_number"])
        values["line_number"] += values["LINE_INCREMENT"]
        return f"N{line_num}{space}"
    return ""


def parse_a_group(values, pathobj):
    """Parse a Group (compound, project, or simple path)."""
    nl = "\n"
    out = ""

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        if values["OUTPUT_COMMENTS"]:
            comment = create_comment(values, f"Compound: {pathobj.Label}")
            out += f"{linenumber(values)}{comment}{nl}"
        for p in pathobj.Group:
            out += parse_a_group(values, p)
    else:  # parsing simple path
        # groups might contain non-path things like stock.
        if not hasattr(pathobj, "Path"):
            return out
        if values["OUTPUT_PATH_LABELS"] and values["OUTPUT_COMMENTS"]:
            comment = create_comment(values, f"Path: {pathobj.Label}")
            out += f"{linenumber(values)}{comment}{nl}"
        out += parse_a_path(values, pathobj)
    return out


def parse_a_path(values, pathobj):
    """Parse a simple Path."""
    nl = "\n"
    out = ""

    adaptiveOp = False
    opHorizRapid = 0
    opVertRapid = 0

    lastcommand = None
    firstmove = Path.Command("G0", {"X": -1, "Y": -1, "Z": -1, "F": 0.0})
    currLocation = {}  # keep track for no doubles
    currLocation.update(firstmove.Parameters)  # set First location Parameters

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

    for c in PathUtils.getPathWithPlacement(pathobj).Commands:

        # List of elements in the command, code, and params.
        outstring = []
        # command may contain M code, G code, comment string, etc.
        command = c.Name
        if command[0] == "(":
            if not values["OUTPUT_COMMENTS"]:
                continue
            if values["COMMENT_SYMBOL"] != "(" and len(command) > 2:
                command = create_comment(values, command[1:-1])

        if (
            values["OUTPUT_ADAPTIVE"]
            and adaptiveOp
            and command in values["RAPID_MOVES"]
        ):
            if opHorizRapid and opVertRapid:
                command = "G1"
            else:
                outstring.append(f"(Tool Controller Rapid Values are unset){nl}")

        outstring.append(command)

        # if modal: suppress the command if it is the same as the last one
        if values["MODAL"] and command == lastcommand:
            outstring.pop(0)

        # Now add the remaining parameters in order
        for parameter in values["PARAMETER_ORDER"]:
            if parameter in c.Parameters:
                parameter_value = values["PARAMETER_FUNCTIONS"][parameter](
                    values, command, parameter, c.Parameters[parameter], currLocation
                )
                if parameter_value:
                    outstring.append(f"{parameter}{parameter_value}")

        if (
            values["OUTPUT_ADAPTIVE"]
            and adaptiveOp
            and command in values["RAPID_MOVES"]
            and opHorizRapid
            and opVertRapid
        ):
            if "Z" not in c.Parameters:
                param_num = format_for_feed(values, opHorizRapid)
            else:
                param_num = format_for_feed(values, opVertRapid)
            outstring.append(f"F{param_num}")

        # store the latest command
        lastcommand = command
        currLocation.update(c.Parameters)
        # Memorizes the current position for calculating the related movements
        # and the withdrawal plan
        if command in values["MOTION_COMMANDS"]:
            if "X" in c.Parameters:
                values["CURRENT_X"] = Units.Quantity(c.Parameters["X"], Units.Length)
            if "Y" in c.Parameters:
                values["CURRENT_Y"] = Units.Quantity(c.Parameters["Y"], Units.Length)
            if "Z" in c.Parameters:
                values["CURRENT_Z"] = Units.Quantity(c.Parameters["Z"], Units.Length)

        if command in ("G98", "G99"):
            values["DRILL_RETRACT_MODE"] = command
        elif command in ("G90", "G91"):
            values["MOTION_MODE"] = command

        if (
            values["TRANSLATE_DRILL_CYCLES"]
            and command in values["DRILL_CYCLES_TO_TRANSLATE"]
        ):
            if values["OUTPUT_COMMENTS"]:  # Comment the original command
                comment = create_comment(
                    values,
                    values["COMMAND_SPACE"]
                    + format_outstring(values, outstring)
                    + values["COMMAND_SPACE"],
                )
                out += f"{linenumber(values)}{comment}{nl}"
            out += drill_translate(values, command, c.Parameters)
            # Erase the line we just translated
            outstring = []

        if values["SPINDLE_WAIT"] > 0 and command in ("M3", "M03", "M4", "M04"):
            out += f"{linenumber(values)}{format_outstring(values, outstring)}{nl}"
            num = format_outstring(values, ["G4", f'P{values["SPINDLE_WAIT"]}'])
            out += f"{linenumber(values)}{num}{nl}"
            outstring = []

        # Check for Tool Change:
        if command in ("M6", "M06"):
            if values["OUTPUT_COMMENTS"]:
                comment = create_comment(values, "Begin toolchange")
                out += f"{linenumber(values)}{comment}{nl}"
            if values["OUTPUT_TOOL_CHANGE"]:
                if values["STOP_SPINDLE_FOR_TOOL_CHANGE"]:
                    # stop the spindle
                    out += f"{linenumber(values)}M5{nl}"
                for line in values["TOOL_CHANGE"].splitlines(False):
                    out += f"{linenumber(values)}{line}{nl}"
            elif values["OUTPUT_COMMENTS"]:
                # convert the tool change to a comment
                comment = create_comment(
                    values,
                    values["COMMAND_SPACE"]
                    + format_outstring(values, outstring)
                    + values["COMMAND_SPACE"],
                )
                out += f"{linenumber(values)}{comment}{nl}"
                outstring = []

        if command == "message" and values["REMOVE_MESSAGES"]:
            if values["OUTPUT_COMMENTS"] is False:
                out = []
            else:
                outstring.pop(0)  # remove the command

        if command in values["SUPPRESS_COMMANDS"]:
            if values["OUTPUT_COMMENTS"]:
                # convert the command to a comment
                comment = create_comment(
                    values,
                    values["COMMAND_SPACE"]
                    + format_outstring(values, outstring)
                    + values["COMMAND_SPACE"],
                )
                out += f"{linenumber(values)}{comment}{nl}"
            # remove the command
            outstring = []

        # prepend a line number and append a newline
        if len(outstring) >= 1:
            if values["OUTPUT_LINE_NUMBERS"]:
                # In this case we don't want a space after the line number
                # because the space is added in the join just below.
                outstring.insert(0, (linenumber(values, "")))

            # append the line to the final output
            out += values["COMMAND_SPACE"].join(outstring)
            # Note: Do *not* strip `out`, since that forces the allocation
            # of a contiguous string & thus quadratic complexity.
            out += f"{nl}"

        # add height offset
        if command in ("M6", "M06") and values["USE_TLO"]:
            out += f'{linenumber(values)}G43 H{str(int(c.Parameters["T"]))}{nl}'

        # Check for comments containing machine-specific commands
        # to pass literally to the controller
        if values["ENABLE_MACHINE_SPECIFIC_COMMANDS"]:
            m = re.match(r"^\(MC_RUN_COMMAND: ([^)]+)\)$", command)
            if m:
                raw_command = m.group(1)
                out += f"{linenumber(values)}{raw_command}{nl}"
    return out
