# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
from FreeCAD import Units
import Path
import argparse
import datetime
import shlex
import Path.Base.Util as PathUtil
import Path.Post.Utils as PostUtils
import PathScripts.PathUtils as PathUtils
from builtins import open as pyopen

TOOLTIP = """
This is a postprocessor file for the Path workbench. It is used to
take a pseudo-G-code fragment outputted by a Path object, and output
real G-code suitable for a linuxcnc 3 axis mill. This postprocessor, once placed
in the appropriate PathScripts folder, can be used directly from inside
FreeCAD, via the GUI importer or via python scripts with:

import linuxcnc_legacy_post
linuxcnc_legacy_post.export(object,"/path/to/file.ncc","")
"""

# Preamble text will appear at the beginning of the GCODE output file.
PREAMBLE = """G17 G54 G40 G49 G80 G90
"""

# Postamble text will appear following the last operation.
POSTAMBLE = """M05
G17 G54 G90 G80 G40
M2
"""

now = datetime.datetime.now()

parser = argparse.ArgumentParser(prog="linuxcnc", add_help=False)
parser.add_argument("--no-header", action="store_true", help="suppress header output")
parser.add_argument("--no-comments", action="store_true", help="suppress comment output")
parser.add_argument("--line-numbers", action="store_true", help="prefix with line numbers")
parser.add_argument(
    "--no-show-editor",
    action="store_true",
    help="don't pop up editor before writing output",
)
parser.add_argument("--precision", default="3", help="number of digits of precision, default=3")
parser.add_argument(
    "--preamble",
    help='set commands to be issued before the first command, default="'
    + PREAMBLE.replace("\n", "\\n")
    + '"',
)
parser.add_argument(
    "--postamble",
    help='set commands to be issued after the last command, default="'
    + POSTAMBLE.replace("\n", "\\n")
    + '"',
)
parser.add_argument(
    "--inches", action="store_true", help="Convert output for US imperial mode (G20)"
)
parser.add_argument(
    "--modal",
    action="store_true",
    help="Output the Same G-command Name USE NonModal Mode",
)
parser.add_argument("--axis-modal", action="store_true", help="Output the Same Axis Value Mode")
parser.add_argument(
    "--no-tlo",
    action="store_true",
    help="suppress tool length offset (G43) following tool changes",
)
parser.add_argument("--rigid-tap", action="store_true", help="Enable G33.1 rigid tapping cycle")

TOOLTIP_ARGS = parser.format_help()

# These globals set common customization preferences
OUTPUT_COMMENTS = True
OUTPUT_HEADER = True
OUTPUT_LINE_NUMBERS = False
SHOW_EDITOR = True
MODAL = False  # if true commands are suppressed if the same as previous line.
USE_TLO = True  # if true G43 will be output following tool changes
OUTPUT_DOUBLES = True  # if false duplicate axis values are suppressed if the same as previous line.
COMMAND_SPACE = " "
LINENR = 100  # line number starting value

# These globals will be reflected in the Machine configuration of the project
UNITS = "G21"  # G21 for metric, G20 for us standard
UNIT_SPEED_FORMAT = "mm/min"
UNIT_FORMAT = "mm"

MACHINE_NAME = "LinuxCNC"
CORNER_MIN = {"x": 0, "y": 0, "z": 0}
CORNER_MAX = {"x": 500, "y": 300, "z": 300}
PRECISION = 3

RIGID_TAP = False

# Pre operation text will be inserted before every operation
PRE_OPERATION = """"""

# Post operation text will be inserted after every operation
POST_OPERATION = """"""

# Tool Change commands will be inserted before a tool change
TOOL_CHANGE = """"""


def processArguments(argstring):
    global OUTPUT_HEADER
    global OUTPUT_COMMENTS
    global OUTPUT_LINE_NUMBERS
    global SHOW_EDITOR
    global PRECISION
    global PREAMBLE
    global POSTAMBLE
    global UNITS
    global UNIT_SPEED_FORMAT
    global UNIT_FORMAT
    global MODAL
    global USE_TLO
    global OUTPUT_DOUBLES
    global RIGID_TAP

    try:
        args = parser.parse_args(shlex.split(argstring))
        if args.no_header:
            OUTPUT_HEADER = False
        if args.no_comments:
            OUTPUT_COMMENTS = False
        if args.line_numbers:
            OUTPUT_LINE_NUMBERS = True
        if args.no_show_editor:
            SHOW_EDITOR = False
        # print("Show editor = %d" % SHOW_EDITOR)  # Commented to reduce test noise
        PRECISION = args.precision
        if args.preamble is not None:
            PREAMBLE = args.preamble.replace("\\n", "\n")
        if args.postamble is not None:
            POSTAMBLE = args.postamble.replace("\\n", "\n")
        if args.inches:
            UNITS = "G20"
            UNIT_SPEED_FORMAT = "in/min"
            UNIT_FORMAT = "in"
            PRECISION = 4
        if args.modal:
            MODAL = True
        if args.no_tlo:
            USE_TLO = False
        if args.axis_modal:
            print("here")
            OUTPUT_DOUBLES = False
        if args.rigid_tap:
            RIGID_TAP = True

    except Exception:
        return False

    return True


def export(objectslist, filename, argstring):
    if not processArguments(argstring):
        return None
    global UNITS
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT

    for obj in objectslist:
        if not hasattr(obj, "Path"):
            print(
                "the object " + obj.Name + " is not a path. Please select only path and Compounds."
            )
            return None

    # print("postprocessing...")  # Commented to reduce test noise
    gcode = ""

    # write header
    if OUTPUT_HEADER:
        gcode += linenumber() + "(Exported by FreeCAD)\n"
        gcode += linenumber() + "(Post Processor: " + __name__ + ")\n"
        gcode += linenumber() + "(Output Time:" + str(now) + ")\n"

    # Write the preamble
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(begin preamble)\n"
    for line in PREAMBLE.splitlines():
        gcode += linenumber() + line + "\n"
    gcode += linenumber() + UNITS + "\n"

    for obj in objectslist:
        # Skip inactive operations
        if not PathUtil.activeForOp(obj):
            continue

        # do the pre_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + "(begin operation: %s)\n" % obj.Label
            gcode += linenumber() + "(machine units: %s)\n" % (UNIT_SPEED_FORMAT)
        for line in PRE_OPERATION.splitlines(True):
            gcode += linenumber() + line

        # get coolant mode
        coolantMode = PathUtil.coolantModeForOp(obj)

        # turn coolant on if required
        if OUTPUT_COMMENTS:
            if not coolantMode == "None":
                gcode += linenumber() + "(Coolant On:" + coolantMode + ")\n"
        if coolantMode == "Flood":
            gcode += linenumber() + "M8" + "\n"
        if coolantMode == "Mist":
            gcode += linenumber() + "M7" + "\n"

        # process the operation gcode
        gcode += parse(obj)

        # do the post_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + "(finish operation: %s)\n" % obj.Label
        for line in POST_OPERATION.splitlines(True):
            gcode += linenumber() + line

        # turn coolant off if required
        if not coolantMode == "None":
            if OUTPUT_COMMENTS:
                gcode += linenumber() + "(Coolant Off:" + coolantMode + ")\n"
            gcode += linenumber() + "M9" + "\n"

    # do the post_amble
    if OUTPUT_COMMENTS:
        gcode += "(begin postamble)\n"
    for line in POSTAMBLE.splitlines():
        gcode += linenumber() + line + "\n"

    if FreeCAD.GuiUp and SHOW_EDITOR:
        final = gcode
        if len(gcode) > 100000:
            print("Skipping editor since output is greater than 100kb")
        else:
            dia = PostUtils.GCodeEditorDialog()
            dia.editor.setPlainText(gcode)
            result = dia.exec_()
            if result:
                final = dia.editor.toPlainText()
    else:
        final = gcode

    # print("done postprocessing.")  # Commented to reduce test noise

    if not filename == "-":
        gfile = pyopen(filename, "w")
        gfile.write(final)
        gfile.close()

    return final


def linenumber():
    global LINENR
    if OUTPUT_LINE_NUMBERS is True:
        LINENR += 10
        return "N" + str(LINENR) + " "
    return ""


def parse(pathobj):
    global PRECISION
    global MODAL
    global OUTPUT_DOUBLES
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT

    out = ""
    lastcommand = None
    precision_string = "." + str(PRECISION) + "f"
    currLocation = {}  # keep track for no doubles

    # the order of parameters
    # linuxcnc doesn't want K properties on XY plane  Arcs need work.
    params = [
        "X",
        "Y",
        "Z",
        "A",
        "B",
        "C",
        "I",
        "J",
        "F",
        "S",
        "T",
        "Q",
        "R",
        "L",
        "H",
        "D",
        "P",
    ]
    firstmove = Path.Command("G0", {"X": -1, "Y": -1, "Z": -1, "F": 0.0})
    currLocation.update(firstmove.Parameters)  # set First location Parameters

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        # if OUTPUT_COMMENTS:
        #     out += linenumber() + "(compound: " + pathobj.Label + ")\n"
        for p in pathobj.Group:
            out += parse(p)
        return out
    else:  # parsing simple path
        # groups might contain non-path things like stock.
        if not hasattr(pathobj, "Path"):
            return out

        # if OUTPUT_COMMENTS:
        #     out += linenumber() + "(" + pathobj.Label + ")\n"

        # The following "for" statement was fairly recently added
        # but seems to be using the A, B, and C parameters in ways
        # that don't appear to be compatible with how the PATH code
        # uses the A, B, and C parameters.  I have reverted the
        # change here until we can figure out what it going on.
        #
        # for c in PathUtils.getPathWithPlacement(pathobj).Commands:
        for c in pathobj.Path.Commands:
            outstring = []
            command = c.Name
            outstring.append(command)

            # if modal: suppress the command if it is the same as the last one
            if MODAL is True:
                if command == lastcommand:
                    outstring.pop(0)

            if c.Name.startswith("(") and not OUTPUT_COMMENTS:  # command is a comment
                continue

            # Check for G80, G98, G99 with rigid tapping and annotation
            if (
                command in ("G80", "G98", "G99")
                and RIGID_TAP
                and hasattr(c, "Annotations")
                and c.Annotations.get("operation") == "tapping"
            ):
                continue  # Skip this command

            # Handle G84/G74 tapping cycles
            if command in ("G84", "G74") and "F" in c.Parameters:
                pitch_mm = float(c.Parameters["F"])
                c.Parameters.pop("F")  # Remove F from output, we'll handle it

                # Get spindle speed (from S param or last known value)
                spindle_speed = None
                if "S" in c.Parameters:
                    spindle_speed = float(c.Parameters["S"])
                    c.Parameters.pop("S")

                # Convert pitch to inches if needed
                if UNITS == "G20":  # imperial
                    pitch = pitch_mm / 25.4
                else:
                    pitch = pitch_mm

                # Rigid tapping logic
                if RIGID_TAP:
                    # Output initial tapping command
                    outstring[0] = "G33.1"
                    outstring.append("K" + format(pitch, precision_string))

                    if "Z" in c.Parameters:
                        outstring.append("Z" + format(float(c.Parameters["Z"]), precision_string))

                    # Output the tapping line
                    if len(outstring) >= 1:
                        if OUTPUT_LINE_NUMBERS:
                            outstring.insert(0, (linenumber()))
                        for w in outstring:
                            out += w + COMMAND_SPACE
                        out += "\n"

                    if "P" in c.Parameters:
                        # Issue spindle stop
                        out += linenumber() + "M5\n"
                        # Issue dwell with P value
                        out += linenumber() + f"G04 P{c.Parameters['P']}\n"

                    # Now handle reverse out and spindle restore
                    if command == "G84":
                        # Reverse spindle (M4) with spindle speed
                        out += linenumber() + "M4\n"
                        # Repeat tapping command to reverse out, use R for Z
                        reverse_z = c.Parameters.get("R")
                        if reverse_z is not None:
                            pos = Units.Quantity(reverse_z, FreeCAD.Units.Length)
                            reverse_z = float(pos.getValueAs(UNIT_FORMAT))
                            out += (
                                linenumber()
                                + f"G33.1 K{format(pitch, precision_string)} Z{format(float(reverse_z), precision_string)}\n"
                            )
                        else:
                            out += linenumber() + f"G33.1 K{format(pitch, precision_string)}\n"
                        # Restore original spindle direction (M3) with spindle speed
                        out += linenumber() + "M3\n"
                    elif command == "G74":
                        # Forward spindle (M3) with spindle speed
                        out += linenumber() + "M3\n"
                        # Repeat tapping command to reverse out, use R for Z
                        reverse_z = c.Parameters.get("R")
                        if reverse_z is not None:
                            pos = Units.Quantity(reverse_z, FreeCAD.Units.Length)
                            reverse_z = float(pos.getValueAs(UNIT_FORMAT))
                            out += (
                                linenumber()
                                + f"G33.1 K{format(pitch, precision_string)} Z{format(float(reverse_z), precision_string)}\n"
                            )
                        else:
                            out += linenumber() + f"G33.1 K{format(pitch, precision_string)}\n"
                        # Restore original spindle direction (M4) with spindle speed
                        out += linenumber() + "M4\n"

                    continue  # Skip the rest of the parameter output for this command

                else:
                    # Calculate feed rate
                    if spindle_speed is not None:
                        feed_rate = pitch * spindle_speed
                        speed = Units.Quantity(feed_rate, UNIT_SPEED_FORMAT)
                        outstring.append(
                            "F"
                            + format(float(speed.getValueAs(UNIT_SPEED_FORMAT)), precision_string)
                        )
                    else:
                        # No spindle speed found, output pitch as F
                        outstring.append("F" + format(pitch, precision_string))

            # Now add the remaining parameters in order
            for param in params:
                if param in c.Parameters:
                    if param == "F" and (
                        currLocation[param] != c.Parameters[param] or OUTPUT_DOUBLES
                    ):
                        if c.Name not in [
                            "G0",
                            "G00",
                        ]:  # linuxcnc doesn't use rapid speeds
                            speed = Units.Quantity(c.Parameters["F"], FreeCAD.Units.Velocity)
                            if speed.getValueAs(UNIT_SPEED_FORMAT) > 0.0:
                                outstring.append(
                                    param
                                    + format(
                                        float(speed.getValueAs(UNIT_SPEED_FORMAT)),
                                        precision_string,
                                    )
                                )
                        else:
                            continue
                    elif param == "T":
                        outstring.append(param + str(int(c.Parameters["T"])))
                    elif param == "H":
                        outstring.append(param + str(int(c.Parameters["H"])))
                    elif param == "D":
                        outstring.append(param + str(int(c.Parameters["D"])))
                    elif param == "S":
                        outstring.append(param + str(int(c.Parameters["S"])))
                    else:
                        if (
                            (not OUTPUT_DOUBLES)
                            and (param in currLocation)
                            and (currLocation[param] == c.Parameters[param])
                        ):
                            continue
                        else:
                            if param in ("A", "B", "C"):
                                outstring.append(
                                    param + format(float(c.Parameters[param]), precision_string)
                                )
                            else:
                                pos = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)
                                outstring.append(
                                    param
                                    + format(float(pos.getValueAs(UNIT_FORMAT)), precision_string)
                                )

            # store the latest command
            lastcommand = command
            currLocation.update(c.Parameters)

            # Check for Tool Change:
            if command == "M6":
                # stop the spindle
                out += linenumber() + "M5\n"
                for line in TOOL_CHANGE.splitlines(True):
                    out += linenumber() + line

                # add height offset
                if USE_TLO:
                    tool_height = "\nG43 H" + str(int(c.Parameters["T"]))
                    outstring.append(tool_height)

            if command == "message":
                if OUTPUT_COMMENTS is False:
                    out = []
                else:
                    outstring.pop(0)  # remove the command

            # prepend a line number and append a newline
            if len(outstring) >= 1:
                if OUTPUT_LINE_NUMBERS:
                    outstring.insert(0, (linenumber()))

                # append the line to the final output
                for w in outstring:
                    out += w + COMMAND_SPACE
                out += "\n"

        return out


# print(__name__ + " gcode postprocessor loaded.")
