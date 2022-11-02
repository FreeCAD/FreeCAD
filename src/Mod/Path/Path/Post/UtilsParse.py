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


def create_comment(comment_string, comment_symbol):
    """Create a comment from a string using the correct comment symbol."""
    if comment_symbol == "(":
        comment_string = "(" + comment_string + ")"
    else:
        comment_string = comment_symbol + comment_string
    return comment_string


def drill_translate(values, outstring, cmd, params):
    """Translate drill cycles."""
    axis_precision_string = "." + str(values["AXIS_PRECISION"]) + "f"
    feed_precision_string = "." + str(values["FEED_PRECISION"]) + "f"

    trBuff = ""

    if values["OUTPUT_COMMENTS"]:  # Comment the original command
        trBuff += (
            linenumber(values)
            + create_comment(
                values["COMMAND_SPACE"]
                + format_outstring(values, outstring)
                + values["COMMAND_SPACE"],
                values["COMMENT_SYMBOL"],
            )
            + "\n"
        )

    # cycle conversion
    # currently only cycles in XY are provided (G17)
    # other plains ZX (G18) and  YZ (G19) are not dealt with : Z drilling only.
    drill_X = Units.Quantity(params["X"], FreeCAD.Units.Length)
    drill_Y = Units.Quantity(params["Y"], FreeCAD.Units.Length)
    drill_Z = Units.Quantity(params["Z"], FreeCAD.Units.Length)
    RETRACT_Z = Units.Quantity(params["R"], FreeCAD.Units.Length)
    # R less than Z is error
    if RETRACT_Z < drill_Z:
        trBuff += (
            linenumber(values)
            + create_comment(
                "Drill cycle error: R less than Z", values["COMMENT_SYMBOL"]
            )
            + "\n"
        )
        return trBuff

    if values["MOTION_MODE"] == "G91":  # G91 relative movements
        drill_X += values["CURRENT_X"]
        drill_Y += values["CURRENT_Y"]
        drill_Z += values["CURRENT_Z"]
        RETRACT_Z += values["CURRENT_Z"]

    if values["DRILL_RETRACT_MODE"] == "G98" and values["CURRENT_Z"] >= RETRACT_Z:
        RETRACT_Z = values["CURRENT_Z"]

    # get the other parameters
    drill_feedrate = Units.Quantity(params["F"], FreeCAD.Units.Velocity)
    if cmd in ("G73", "G83"):
        drill_Step = Units.Quantity(params["Q"], FreeCAD.Units.Length)
        # NIST 3.5.16.4 G83 Cycle:  "current hole bottom, backed off a bit."
        a_bit = drill_Step * 0.05
    elif cmd == "G82":
        drill_DwellTime = params["P"]

    # wrap this block to ensure machine's values["MOTION_MODE"] is restored in case of error
    # try:
    if values["MOTION_MODE"] == "G91":
        # force absolute coordinates during cycles
        trBuff += linenumber(values) + "G90\n"

    strG0_RETRACT_Z = (
        "G0 Z"
        + format(
            float(RETRACT_Z.getValueAs(values["UNIT_FORMAT"])), axis_precision_string
        )
        + "\n"
    )
    strF_Feedrate = (
        " F"
        + format(
            float(drill_feedrate.getValueAs(values["UNIT_SPEED_FORMAT"])),
            feed_precision_string,
        )
        + "\n"
    )
    # print(strF_Feedrate)

    # preliminary movement(s)
    if values["CURRENT_Z"] < RETRACT_Z:
        trBuff += linenumber(values) + strG0_RETRACT_Z
    trBuff += (
        linenumber(values)
        + "G0 X"
        + format(
            float(drill_X.getValueAs(values["UNIT_FORMAT"])), axis_precision_string
        )
        + " Y"
        + format(
            float(drill_Y.getValueAs(values["UNIT_FORMAT"])), axis_precision_string
        )
        + "\n"
    )
    if values["CURRENT_Z"] > RETRACT_Z:
        # NIST GCODE 3.5.16.1 Preliminary and In-Between Motion says G0 to RETRACT_Z
        # Here use G1 since retract height may be below surface !
        trBuff += (
            linenumber(values)
            + "G1 Z"
            + format(
                float(RETRACT_Z.getValueAs(values["UNIT_FORMAT"])),
                axis_precision_string,
            )
            + strF_Feedrate
        )
    last_Stop_Z = RETRACT_Z

    # drill moves
    if cmd in ("G81", "G82"):
        trBuff += (
            linenumber(values)
            + "G1 Z"
            + format(
                float(drill_Z.getValueAs(values["UNIT_FORMAT"])), axis_precision_string
            )
            + strF_Feedrate
        )
        # pause where applicable
        if cmd == "G82":
            trBuff += linenumber(values) + "G4 P" + str(drill_DwellTime) + "\n"
        trBuff += linenumber(values) + strG0_RETRACT_Z
    else:  # "G73" or "G83"
        if params["Q"] != 0:
            while 1:
                if last_Stop_Z != RETRACT_Z:
                    # rapid move to just short of last drilling depth
                    clearance_depth = last_Stop_Z + a_bit
                    trBuff += (
                        linenumber(values)
                        + "G0 Z"
                        + format(
                            float(clearance_depth.getValueAs(values["UNIT_FORMAT"])),
                            axis_precision_string,
                        )
                        + "\n"
                    )
                next_Stop_Z = last_Stop_Z - drill_Step
                if next_Stop_Z > drill_Z:
                    trBuff += (
                        linenumber(values)
                        + "G1 Z"
                        + format(
                            float(next_Stop_Z.getValueAs(values["UNIT_FORMAT"])),
                            axis_precision_string,
                        )
                        + strF_Feedrate
                    )
                    if cmd == "G73":
                        # Rapid up "a small amount".
                        chip_breaker_height = (
                            next_Stop_Z + values["CHIPBREAKING_AMOUNT"]
                        )
                        trBuff += (
                            linenumber(values)
                            + "G0 Z"
                            + format(
                                float(
                                    chip_breaker_height.getValueAs(
                                        values["UNIT_FORMAT"]
                                    )
                                ),
                                axis_precision_string,
                            )
                            + "\n"
                        )
                    else:  # "G83"
                        # Rapid up to the retract height
                        trBuff += linenumber(values) + strG0_RETRACT_Z
                    last_Stop_Z = next_Stop_Z
                else:
                    trBuff += (
                        linenumber(values)
                        + "G1 Z"
                        + format(
                            float(drill_Z.getValueAs(values["UNIT_FORMAT"])),
                            axis_precision_string,
                        )
                        + strF_Feedrate
                    )
                    trBuff += linenumber(values) + strG0_RETRACT_Z
                    break

    # except Exception:
    # print("exception occurred")
    # pass

    if values["MOTION_MODE"] == "G91":
        trBuff += linenumber(values) + "G91\n"  # Restore if changed

    return trBuff


def dump(obj):
    """For debug..."""
    for attr in dir(obj):
        print("obj.%s = %s" % (attr, getattr(obj, attr)))


def format_outstring(values, strTable):
    """Construct the line for the final output."""
    s = ""
    for w in strTable:
        s += w + values["COMMAND_SPACE"]
    s = s.strip()
    return s


def linenumber(values, space=None):
    """Output the next line number if appropriate."""
    if values["OUTPUT_LINE_NUMBERS"]:
        if space is None:
            space = values["COMMAND_SPACE"]
        line_num = str(values["line_number"])
        values["line_number"] += values["LINE_INCREMENT"]
        return "N" + line_num + space
    return ""


def parse(values, pathobj):
    """Parse a Path."""
    out = ""
    lastcommand = None
    axis_precision_string = "." + str(values["AXIS_PRECISION"]) + "f"
    feed_precision_string = "." + str(values["FEED_PRECISION"]) + "f"

    currLocation = {}  # keep track for no doubles
    firstmove = Path.Command("G0", {"X": -1, "Y": -1, "Z": -1, "F": 0.0})
    currLocation.update(firstmove.Parameters)  # set First location Parameters

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        if values["OUTPUT_COMMENTS"]:
            comment = create_comment(
                "Compound: " + pathobj.Label, values["COMMENT_SYMBOL"]
            )
            out += linenumber(values) + comment + "\n"
        for p in pathobj.Group:
            out += parse(values, p)
        return out
    else:  # parsing simple path

        # groups might contain non-path things like stock.
        if not hasattr(pathobj, "Path"):
            return out

        if values["OUTPUT_PATH_LABELS"] and values["OUTPUT_COMMENTS"]:
            comment = create_comment("Path: " + pathobj.Label, values["COMMENT_SYMBOL"])
            out += linenumber(values) + comment + "\n"

        if values["OUTPUT_ADAPTIVE"]:
            adaptiveOp = False
            opHorizRapid = 0
            opVertRapid = 0
            if "Adaptive" in pathobj.Name:
                adaptiveOp = True
                if hasattr(pathobj, "ToolController"):
                    if (
                        hasattr(pathobj.ToolController, "HorizRapid")
                        and pathobj.ToolController.HorizRapid > 0
                    ):
                        opHorizRapid = Units.Quantity(
                            pathobj.ToolController.HorizRapid, FreeCAD.Units.Velocity
                        )
                    else:
                        FreeCAD.Console.PrintWarning(
                            "Tool Controller Horizontal Rapid Values are unset" + "\n"
                        )
                    if (
                        hasattr(pathobj.ToolController, "VertRapid")
                        and pathobj.ToolController.VertRapid > 0
                    ):
                        opVertRapid = Units.Quantity(
                            pathobj.ToolController.VertRapid, FreeCAD.Units.Velocity
                        )
                    else:
                        FreeCAD.Console.PrintWarning(
                            "Tool Controller Vertical Rapid Values are unset" + "\n"
                        )

        for c in pathobj.Path.Commands:

            # List of elements in the command, code, and params.
            outstring = []
            # command M or G code or comment string
            command = c.Name
            if command[0] == "(":
                if values["OUTPUT_COMMENTS"]:
                    if values["COMMENT_SYMBOL"] != "(":
                        command = PostUtils.fcoms(command, values["COMMENT_SYMBOL"])
                else:
                    continue
            if values["OUTPUT_ADAPTIVE"]:
                if adaptiveOp and command in values["RAPID_MOVES"]:
                    if opHorizRapid and opVertRapid:
                        command = "G1"
                    else:
                        outstring.append(
                            "(Tool Controller Rapid Values are unset)" + "\n"
                        )

            outstring.append(command)

            # if modal: suppress the command if it is the same as the last one
            if values["MODAL"]:
                if command == lastcommand:
                    outstring.pop(0)

            # Now add the remaining parameters in order
            for param in values["PARAMETER_ORDER"]:
                if param in c.Parameters:
                    if param == "F" and (
                        currLocation[param] != c.Parameters[param]
                        or values["OUTPUT_DOUBLES"]
                    ):
                        # centroid and linuxcnc don't use rapid speeds
                        if command not in values["RAPID_MOVES"]:
                            speed = Units.Quantity(
                                c.Parameters["F"], FreeCAD.Units.Velocity
                            )
                            if speed.getValueAs(values["UNIT_SPEED_FORMAT"]) > 0.0:
                                outstring.append(
                                    param
                                    + format(
                                        float(
                                            speed.getValueAs(
                                                values["UNIT_SPEED_FORMAT"]
                                            )
                                        ),
                                        feed_precision_string,
                                    )
                                )
                        else:
                            continue
                    elif param in ("H", "L", "T"):
                        outstring.append(param + str(int(c.Parameters[param])))
                    elif param == "D":
                        if command in ("G41", "G42"):
                            outstring.append(param + str(int(c.Parameters[param])))
                        elif command in ("G41.1", "G42.1"):
                            pos = Units.Quantity(
                                c.Parameters[param], FreeCAD.Units.Length
                            )
                            outstring.append(
                                param
                                + format(
                                    float(pos.getValueAs(values["UNIT_FORMAT"])),
                                    axis_precision_string,
                                )
                            )
                        elif command in ("G96", "G97"):
                            outstring.append(
                                param
                                + PostUtils.fmt(
                                    c.Parameters[param],
                                    values["SPINDLE_DECIMALS"],
                                    "G21",
                                )
                            )
                        else:  # anything else that is supported
                            outstring.append(param + str(float(c.Parameters[param])))
                    elif param == "P":
                        if command in (
                            "G2",
                            "G02",
                            "G3",
                            "G03",
                            "G5.2",
                            "G5.3",
                            "G10",
                            "G54.1",
                            "G59",
                        ):
                            outstring.append(param + str(int(c.Parameters[param])))
                        elif command in ("G4", "G04", "G76", "G82", "G86", "G89"):
                            outstring.append(param + str(float(c.Parameters[param])))
                        elif command in ("G5", "G05", "G64"):
                            pos = Units.Quantity(
                                c.Parameters[param], FreeCAD.Units.Length
                            )
                            outstring.append(
                                param
                                + format(
                                    float(pos.getValueAs(values["UNIT_FORMAT"])),
                                    axis_precision_string,
                                )
                            )
                        else:  # anything else that is supported
                            outstring.append(param + str(c.Parameters[param]))
                    elif param == "Q":
                        if command == "G10":
                            outstring.append(param + str(int(c.Parameters[param])))
                        elif command in ("G64", "G73", "G83"):
                            pos = Units.Quantity(
                                c.Parameters[param], FreeCAD.Units.Length
                            )
                            outstring.append(
                                param
                                + format(
                                    float(pos.getValueAs(values["UNIT_FORMAT"])),
                                    axis_precision_string,
                                )
                            )
                    elif param == "S":
                        outstring.append(
                            param
                            + PostUtils.fmt(
                                c.Parameters[param], values["SPINDLE_DECIMALS"], "G21"
                            )
                        )
                    else:
                        if (
                            (not values["OUTPUT_DOUBLES"])
                            and (param in currLocation)
                            and (currLocation[param] == c.Parameters[param])
                        ):
                            continue
                        else:
                            pos = Units.Quantity(
                                c.Parameters[param], FreeCAD.Units.Length
                            )
                            outstring.append(
                                param
                                + format(
                                    float(pos.getValueAs(values["UNIT_FORMAT"])),
                                    axis_precision_string,
                                )
                            )

            if values["OUTPUT_ADAPTIVE"]:
                if adaptiveOp and command in values["RAPID_MOVES"]:
                    if opHorizRapid and opVertRapid:
                        if "Z" not in c.Parameters:
                            outstring.append(
                                "F"
                                + format(
                                    float(
                                        opHorizRapid.getValueAs(
                                            values["UNIT_SPEED_FORMAT"]
                                        )
                                    ),
                                    axis_precision_string,
                                )
                            )
                        else:
                            outstring.append(
                                "F"
                                + format(
                                    float(
                                        opVertRapid.getValueAs(
                                            values["UNIT_SPEED_FORMAT"]
                                        )
                                    ),
                                    axis_precision_string,
                                )
                            )

            # store the latest command
            lastcommand = command

            currLocation.update(c.Parameters)
            # Memorizes the current position for calculating the related movements
            # and the withdrawal plan
            if command in values["MOTION_COMMANDS"]:
                if "X" in c.Parameters:
                    values["CURRENT_X"] = Units.Quantity(
                        c.Parameters["X"], FreeCAD.Units.Length
                    )
                if "Y" in c.Parameters:
                    values["CURRENT_Y"] = Units.Quantity(
                        c.Parameters["Y"], FreeCAD.Units.Length
                    )
                if "Z" in c.Parameters:
                    values["CURRENT_Z"] = Units.Quantity(
                        c.Parameters["Z"], FreeCAD.Units.Length
                    )

            if command in ("G98", "G99"):
                values["DRILL_RETRACT_MODE"] = command

            if command in ("G90", "G91"):
                values["MOTION_MODE"] = command

            if values["TRANSLATE_DRILL_CYCLES"]:
                if command in values["DRILL_CYCLES_TO_TRANSLATE"]:
                    out += drill_translate(values, outstring, command, c.Parameters)
                    # Erase the line we just translated
                    outstring = []

            if values["SPINDLE_WAIT"] > 0:
                if command in ("M3", "M03", "M4", "M04"):
                    out += (
                        linenumber(values) + format_outstring(values, outstring) + "\n"
                    )
                    out += (
                        linenumber(values)
                        + format_outstring(
                            values, ["G4", "P%s" % values["SPINDLE_WAIT"]]
                        )
                        + "\n"
                    )
                    outstring = []

            # Check for Tool Change:
            if command in ("M6", "M06"):
                if values["OUTPUT_COMMENTS"]:
                    comment = create_comment(
                        "Begin toolchange", values["COMMENT_SYMBOL"]
                    )
                    out += linenumber(values) + comment + "\n"
                if values["OUTPUT_TOOL_CHANGE"]:
                    if values["STOP_SPINDLE_FOR_TOOL_CHANGE"]:
                        # stop the spindle
                        out += linenumber(values) + "M5\n"
                    for line in values["TOOL_CHANGE"].splitlines(False):
                        out += linenumber(values) + line + "\n"
                else:
                    if values["OUTPUT_COMMENTS"]:
                        # convert the tool change to a comment
                        comment = create_comment(
                            values["COMMAND_SPACE"]
                            + format_outstring(values, outstring)
                            + values["COMMAND_SPACE"],
                            values["COMMENT_SYMBOL"],
                        )
                        out += linenumber(values) + comment + "\n"
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
                        values["COMMAND_SPACE"]
                        + format_outstring(values, outstring)
                        + values["COMMAND_SPACE"],
                        values["COMMENT_SYMBOL"],
                    )
                    out += linenumber(values) + comment + "\n"
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
                out += "\n"

            # add height offset
            if command in ("M6", "M06") and values["USE_TLO"]:
                out += linenumber(values) + "G43 H" + str(int(c.Parameters["T"])) + "\n"

            # Check for comments containing machine-specific commands
            # to pass literally to the controller
            if values["ENABLE_MACHINE_SPECIFIC_COMMANDS"]:
                m = re.match(r"^\(MC_RUN_COMMAND: ([^)]+)\)$", command)
                if m:
                    raw_command = m.group(1)
                    out += linenumber(values) + raw_command + "\n"

        return out
