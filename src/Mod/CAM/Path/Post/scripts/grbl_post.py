# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2018, 2019 Gauthier Briere                              *
# *   Copyright (c) 2019, 2020 Schildkroet                                  *
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
import Path.Base.Util as PathUtil
import Path.Post.Utils as PostUtils
import PathScripts.PathUtils as PathUtils
import argparse
import datetime
import shlex
import re


TOOLTIP = """
Generate g-code from a Path that is compatible with the grbl controller.
import grbl_post
grbl_post.export(object, "/path/to/file.ncc")
"""


# ***************************************************************************
# * Globals set customization preferences
# ***************************************************************************

# Default values for command line arguments:
OUTPUT_COMMENTS = True  # default output of comments in output gCode file
OUTPUT_HEADER = True  # default output header in output gCode file
OUTPUT_LINE_NUMBERS = False  # default doesn't output line numbers in output gCode file
OUTPUT_BCNC = (
    False  # default doesn't add bCNC operation block headers in output gCode file
)
SHOW_EDITOR = True  # default show the resulting file dialog output in GUI
PRECISION = 3  # Default precision for metric (see http://linuxcnc.org/docs/2.7/html/gcode/overview.html#_g_code_best_practices)
TRANSLATE_DRILL_CYCLES = False  # If true, G81, G82 & G83 are translated in G0/G1 moves
PREAMBLE = """G17 G90
"""  # default preamble text will appear at the beginning of the gCode output file.
POSTAMBLE = """M5
G17 G90
M2
"""  # default postamble text will appear following the last operation.

SPINDLE_WAIT = 0  # no waiting after M3 / M4 by default
RETURN_TO = None  # no movements after end of program

# Customisation with no command line argument
MODAL = False  # if true commands are suppressed if the same as previous line.
LINENR = 100  # line number starting value
LINEINCR = 10  # line number increment
OUTPUT_TOOL_CHANGE = False  # default don't output M6 tool changes (comment it) as grbl currently does not handle it
DRILL_RETRACT_MODE = "G98"  # Default value of drill retractations (CURRENT_Z) other possible value is G99
MOTION_MODE = "G90"  # G90 for absolute moves, G91 for relative
UNITS = "G21"  # G21 for metric, G20 for us standard
UNIT_FORMAT = "mm"
UNIT_SPEED_FORMAT = "mm/min"
PRE_OPERATION = """"""  # Pre operation text will be inserted before every operation
POST_OPERATION = """"""  # Post operation text will be inserted after every operation
TOOL_CHANGE = """"""  # Tool Change commands will be inserted before a tool change

# ***************************************************************************
# * End of customization
# ***************************************************************************

# Parser arguments list & definition
parser = argparse.ArgumentParser(prog="grbl", add_help=False)
parser.add_argument("--comments", action="store_true", help="output comment (default)")
parser.add_argument(
    "--no-comments", action="store_true", help="suppress comment output"
)
parser.add_argument("--header", action="store_true", help="output headers (default)")
parser.add_argument("--no-header", action="store_true", help="suppress header output")
parser.add_argument(
    "--line-numbers", action="store_true", help="prefix with line numbers"
)
parser.add_argument(
    "--no-line-numbers",
    action="store_true",
    help="don't prefix with line numbers (default)",
)
parser.add_argument(
    "--show-editor",
    action="store_true",
    help="pop up editor before writing output (default)",
)
parser.add_argument(
    "--no-show-editor",
    action="store_true",
    help="don't pop up editor before writing output",
)
parser.add_argument(
    "--precision", default="3", help="number of digits of precision, default=3"
)
parser.add_argument(
    "--translate_drill",
    action="store_true",
    help="translate drill cycles G81, G82 & G83 in G0/G1 movements",
)
parser.add_argument(
    "--no-translate_drill",
    action="store_true",
    help="don't translate drill cycles G81, G82 & G83 in G0/G1 movements (default)",
)
parser.add_argument(
    "--preamble",
    help='set commands to be issued before the first command, default="G17 G90"',
)
parser.add_argument(
    "--postamble",
    help='set commands to be issued after the last command, default="M5\nG17 G90\n;M2"',
)
parser.add_argument(
    "--inches", action="store_true", help="Convert output for US imperial mode (G20)"
)
parser.add_argument(
    "--tool-change", action="store_true", help="Insert M6 for all tool changes"
)
parser.add_argument(
    "--wait-for-spindle",
    type=int,
    default=0,
    help="Wait for spindle to reach desired speed after M3 / M4, default=0",
)
parser.add_argument(
    "--return-to",
    default="",
    help="Move to the specified coordinates at the end, e.g. --return-to=0,0",
)
parser.add_argument(
    "--bcnc",
    action="store_true",
    help="Add Job operations as bCNC block headers. Consider suppressing existing comments: Add argument --no-comments",
)
parser.add_argument(
    "--no-bcnc", action="store_true", help="suppress bCNC block header output (default)"
)
TOOLTIP_ARGS = parser.format_help()


# ***************************************************************************
# * Internal global variables
# ***************************************************************************
MOTION_COMMANDS = [
    "G0",
    "G00",
    "G1",
    "G01",
    "G2",
    "G02",
    "G3",
    "G03",
]  # Motion gCode commands definition
RAPID_MOVES = ["G0", "G00"]  # Rapid moves gCode commands definition
SUPPRESS_COMMANDS = []  # These commands are ignored by commenting them out
COMMAND_SPACE = " "
# Global variables storing current position
CURRENT_X = 0
CURRENT_Y = 0
CURRENT_Z = 0


# ***************************************************************************
# * to distinguish python built-in open function from the one declared below
if open.__module__ in ["__builtin__", "io"]:
    pythonopen = open


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
    global TRANSLATE_DRILL_CYCLES
    global OUTPUT_TOOL_CHANGE
    global SPINDLE_WAIT
    global RETURN_TO
    global OUTPUT_BCNC

    try:
        args = parser.parse_args(shlex.split(argstring))
        if args.no_header:
            OUTPUT_HEADER = False
        if args.header:
            OUTPUT_HEADER = True
        if args.no_comments:
            OUTPUT_COMMENTS = False
        if args.comments:
            OUTPUT_COMMENTS = True
        if args.no_line_numbers:
            OUTPUT_LINE_NUMBERS = False
        if args.line_numbers:
            OUTPUT_LINE_NUMBERS = True
        if args.no_show_editor:
            SHOW_EDITOR = False
        if args.show_editor:
            SHOW_EDITOR = True
        PRECISION = args.precision
        if args.preamble is not None:
            PREAMBLE = args.preamble
        if args.postamble is not None:
            POSTAMBLE = args.postamble
        if args.no_translate_drill:
            TRANSLATE_DRILL_CYCLES = False
        if args.translate_drill:
            TRANSLATE_DRILL_CYCLES = True
        if args.inches:
            UNITS = "G20"
            UNIT_SPEED_FORMAT = "in/min"
            UNIT_FORMAT = "in"
            PRECISION = 4
        if args.tool_change:
            OUTPUT_TOOL_CHANGE = True
        if args.wait_for_spindle > 0:
            SPINDLE_WAIT = args.wait_for_spindle
        if args.return_to != "":
            RETURN_TO = [int(v) for v in args.return_to.split(",")]
            if len(RETURN_TO) != 2:
                RETURN_TO = None
                print("--return-to coordinates must be specified as <x>,<y>, ignoring")
        if args.bcnc:
            OUTPUT_BCNC = True
        if args.no_bcnc:
            OUTPUT_BCNC = False

    except Exception as e:
        return False

    return True


# For debug...
def dump(obj):
    for attr in dir(obj):
        print("obj.%s = %s" % (attr, getattr(obj, attr)))


def export(objectslist, filename, argstring):

    if not processArguments(argstring):
        return None

    global UNITS
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT
    global MOTION_MODE
    global SUPPRESS_COMMANDS

    print("Post Processor: " + __name__ + " postprocessing...")
    gcode = ""

    # write header
    if OUTPUT_HEADER:
        gcode += linenumber() + "(Exported by FreeCAD)\n"
        gcode += linenumber() + "(Post Processor: " + __name__ + ")\n"
        gcode += linenumber() + "(Output Time:" + str(datetime.datetime.now()) + ")\n"

    # Check canned cycles for drilling
    if TRANSLATE_DRILL_CYCLES:
        if len(SUPPRESS_COMMANDS) == 0:
            SUPPRESS_COMMANDS = ["G99", "G98", "G80"]
        else:
            SUPPRESS_COMMANDS += ["G99", "G98", "G80"]

    # Write the preamble
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(Begin preamble)\n"
    for line in PREAMBLE.splitlines(True):
        gcode += linenumber() + line
    # verify if PREAMBLE have changed MOTION_MODE or UNITS
    if "G90" in PREAMBLE:
        MOTION_MODE = "G90"
    elif "G91" in PREAMBLE:
        MOTION_MODE = "G91"
    else:
        gcode += linenumber() + MOTION_MODE + "\n"
    if "G21" in PREAMBLE:
        UNITS = "G21"
        UNIT_FORMAT = "mm"
        UNIT_SPEED_FORMAT = "mm/min"
    elif "G20" in PREAMBLE:
        UNITS = "G20"
        UNIT_FORMAT = "in"
        UNIT_SPEED_FORMAT = "in/min"
    else:
        gcode += linenumber() + UNITS + "\n"

    for obj in objectslist:
        # Debug...
        # print("\n" + "*"*70)
        # dump(obj)
        # print("*"*70 + "\n")
        if not hasattr(obj, "Path"):
            print(
                "The object "
                + obj.Name
                + " is not a path. Please select only path and Compounds."
            )
            return

        # Skip inactive operations
        if PathUtil.opProperty(obj, "Active") is False:
            continue

        # do the pre_op
        if OUTPUT_BCNC:
            gcode += linenumber() + "(Block-name: " + obj.Label + ")\n"
            gcode += linenumber() + "(Block-expand: 0)\n"
            gcode += linenumber() + "(Block-enable: 1)\n"
        if OUTPUT_COMMENTS:
            gcode += linenumber() + "(Begin operation: " + obj.Label + ")\n"
        for line in PRE_OPERATION.splitlines(True):
            gcode += linenumber() + line

        # get coolant mode
        coolantMode = "None"
        if (
            hasattr(obj, "CoolantMode")
            or hasattr(obj, "Base")
            and hasattr(obj.Base, "CoolantMode")
        ):
            if hasattr(obj, "CoolantMode"):
                coolantMode = obj.CoolantMode
            else:
                coolantMode = obj.Base.CoolantMode

        # turn coolant on if required
        if OUTPUT_COMMENTS:
            if not coolantMode == "None":
                gcode += linenumber() + "(Coolant On:" + coolantMode + ")\n"
        if coolantMode == "Flood":
            gcode += linenumber() + "M8" + "\n"
        if coolantMode == "Mist":
            gcode += linenumber() + "M7" + "\n"

        # Parse the op
        gcode += parse(obj)

        # do the post_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + "(Finish operation: " + obj.Label + ")\n"
        for line in POST_OPERATION.splitlines(True):
            gcode += linenumber() + line

        # turn coolant off if required
        if not coolantMode == "None":
            if OUTPUT_COMMENTS:
                gcode += linenumber() + "(Coolant Off:" + coolantMode + ")\n"
            gcode += linenumber() + "M9" + "\n"

    if RETURN_TO:
        gcode += linenumber() + "G0 X%s Y%s\n" % tuple(RETURN_TO)

    # do the post_amble
    if OUTPUT_BCNC:
        gcode += linenumber() + "(Block-name: post_amble)\n"
        gcode += linenumber() + "(Block-expand: 0)\n"
        gcode += linenumber() + "(Block-enable: 1)\n"
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(Begin postamble)\n"
    for line in POSTAMBLE.splitlines(True):
        gcode += linenumber() + line

    # show the gCode result dialog
    if FreeCAD.GuiUp and SHOW_EDITOR:
        dia = PostUtils.GCodeEditorDialog()
        dia.editor.setText(gcode)
        result = dia.exec_()
        if result:
            final = dia.editor.toPlainText()
        else:
            final = gcode
    else:
        final = gcode

    print("Done postprocessing.")

    # write the file
    gfile = pythonopen(filename, "w")
    gfile.write(final)
    gfile.close()

    return final


def linenumber():
    if not OUTPUT_LINE_NUMBERS:
        return ""
    global LINENR
    global LINEINCR
    s = "N" + str(LINENR) + " "
    LINENR += LINEINCR
    return s


def format_outstring(strTable):
    global COMMAND_SPACE
    # construct the line for the final output
    s = ""
    for w in strTable:
        s += w + COMMAND_SPACE
    s = s.strip()
    return s


def parse(pathobj):

    global DRILL_RETRACT_MODE
    global MOTION_MODE
    global CURRENT_X
    global CURRENT_Y
    global CURRENT_Z

    out = ""
    lastcommand = None
    precision_string = "." + str(PRECISION) + "f"

    params = [
        "X",
        "Y",
        "Z",
        "A",
        "B",
        "C",
        "U",
        "V",
        "W",
        "I",
        "J",
        "K",
        "F",
        "S",
        "T",
        "Q",
        "R",
        "L",
        "P",
    ]

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        if OUTPUT_COMMENTS:
            out += linenumber() + "(Compound: " + pathobj.Label + ")\n"
        for p in pathobj.Group:
            out += parse(p)
        return out

    else:  # parsing simple path
        if not hasattr(
            pathobj, "Path"
        ):  # groups might contain non-path things like stock.
            return out

        if OUTPUT_COMMENTS:
            out += linenumber() + "(Path: " + pathobj.Label + ")\n"

        for c in PathUtils.getPathWithPlacement(pathobj).Commands:
            outstring = []
            command = c.Name

            outstring.append(command)

            # if modal: only print the command if it is not the same as the last one
            if MODAL:
                if command == lastcommand:
                    outstring.pop(0)

            # Now add the remaining parameters in order
            for param in params:
                if param in c.Parameters:
                    if param == "F":
                        if command not in RAPID_MOVES:
                            speed = Units.Quantity(
                                c.Parameters["F"], FreeCAD.Units.Velocity
                            )
                            if speed.getValueAs(UNIT_SPEED_FORMAT) > 0.0:
                                outstring.append(
                                    param
                                    + format(
                                        float(speed.getValueAs(UNIT_SPEED_FORMAT)),
                                        precision_string,
                                    )
                                )
                    elif param in ["T", "H", "S"]:
                        outstring.append(param + str(int(c.Parameters[param])))
                    elif param in ["D", "P", "L"]:
                        outstring.append(param + str(c.Parameters[param]))
                    elif param in ["A", "B", "C"]:
                        outstring.append(
                            param + format(c.Parameters[param], precision_string)
                        )
                    else:  # [X, Y, Z, U, V, W, I, J, K, R, Q] (Conversion eventuelle mm/inches)
                        pos = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)
                        outstring.append(
                            param
                            + format(
                                float(pos.getValueAs(UNIT_FORMAT)), precision_string
                            )
                        )

            # store the latest command
            lastcommand = command

            # Memorizes the current position for calculating the related movements and the withdrawal plan
            if command in MOTION_COMMANDS:
                if "X" in c.Parameters:
                    CURRENT_X = Units.Quantity(c.Parameters["X"], FreeCAD.Units.Length)
                if "Y" in c.Parameters:
                    CURRENT_Y = Units.Quantity(c.Parameters["Y"], FreeCAD.Units.Length)
                if "Z" in c.Parameters:
                    CURRENT_Z = Units.Quantity(c.Parameters["Z"], FreeCAD.Units.Length)

            if command in ("G98", "G99"):
                DRILL_RETRACT_MODE = command

            if command in ("G90", "G91"):
                MOTION_MODE = command

            if TRANSLATE_DRILL_CYCLES:
                if command in ("G81", "G82", "G83"):
                    out += drill_translate(outstring, command, c.Parameters)
                    # Erase the line we just translated
                    outstring = []

            if SPINDLE_WAIT > 0:
                if command in ("M3", "M03", "M4", "M04"):
                    out += linenumber() + format_outstring(outstring) + "\n"
                    out += (
                        linenumber()
                        + format_outstring(["G4", "P%s" % SPINDLE_WAIT])
                        + "\n"
                    )
                    outstring = []

            # Check for Tool Change:
            if command in ("M6", "M06"):
                if OUTPUT_COMMENTS:
                    out += linenumber() + "(Begin toolchange)\n"
                if not OUTPUT_TOOL_CHANGE:
                    outstring.insert(0, "(")
                    outstring.append(")")
                else:
                    for line in TOOL_CHANGE.splitlines(True):
                        out += linenumber() + line

            if command == "message":
                if OUTPUT_COMMENTS is False:
                    out = []
                else:
                    outstring.pop(0)  # remove the command

            if command in SUPPRESS_COMMANDS:
                outstring.insert(0, "(")
                outstring.append(")")

            # prepend a line number and append a newline
            if len(outstring) >= 1:
                out += linenumber() + format_outstring(outstring) + "\n"

            # Check for comments containing machine-specific commands to pass literally to the controller
            m = re.match(r"^\(MC_RUN_COMMAND: ([^)]+)\)$", command)
            if m:
                raw_command = m.group(1)
                out += linenumber() + raw_command + "\n"

    return out


def drill_translate(outstring, cmd, params):
    global DRILL_RETRACT_MODE
    global MOTION_MODE
    global CURRENT_X
    global CURRENT_Y
    global CURRENT_Z
    global UNITS
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT


    strFormat = "." + str(PRECISION) + "f"
    strG0_Initial_Z=("G0 Z" + format(float(CURRENT_Z.getValueAs(UNIT_FORMAT)), strFormat) + "\n")

    trBuff = ""

    if OUTPUT_COMMENTS:  # Comment the original command
        outstring[0] = "(" + outstring[0]
        outstring[-1] = outstring[-1] + ")"
        trBuff += linenumber() + format_outstring(outstring) + "\n"

    # cycle conversion
    # currently only cycles in XY are provided (G17)
    # other plains ZX (G18) and  YZ (G19) are not dealt with : Z drilling only.
    drill_X = Units.Quantity(params["X"], FreeCAD.Units.Length)
    drill_Y = Units.Quantity(params["Y"], FreeCAD.Units.Length)
    drill_Z = Units.Quantity(params["Z"], FreeCAD.Units.Length)
    RETRACT_Z = Units.Quantity(params["R"], FreeCAD.Units.Length)
    # R less than Z is error
    if RETRACT_Z < drill_Z:
        trBuff += linenumber() + "(drill cycle error: R less than Z )\n"
        return trBuff

    if MOTION_MODE == "G91":  # G91 relative movements
        drill_X += CURRENT_X
        drill_Y += CURRENT_Y
        drill_Z += CURRENT_Z
        RETRACT_Z += CURRENT_Z

#    if DRILL_RETRACT_MODE == "G98" and CURRENT_Z >= RETRACT_Z:
#        RETRACT_Z = CURRENT_Z

    # get the other parameters
    drill_feedrate = Units.Quantity(params["F"], FreeCAD.Units.Velocity)
    if cmd == "G83":
        drill_Step = Units.Quantity(params["Q"], FreeCAD.Units.Length)
        a_bit = (
            drill_Step * 0.05
        )  # NIST 3.5.16.4 G83 Cycle:  "current hole bottom, backed off a bit."
    elif cmd == "G82":
        drill_DwellTime = params["P"]

    # wrap this block to ensure machine MOTION_MODE is restored in case of error
    try:
        if MOTION_MODE == "G91":
            trBuff += linenumber() + "G90\n"  # force absolute coordinates during cycles

        strG0_RETRACT_Z = (
            "G0 Z" + format(float(RETRACT_Z.getValueAs(UNIT_FORMAT)), strFormat) + "\n"
        )
        strF_Feedrate = (
            " F"
            + format(float(drill_feedrate.getValueAs(UNIT_SPEED_FORMAT)), ".2f")
            + "\n"
        )
        print(strF_Feedrate)

        # preliminary movement(s)
        if CURRENT_Z < RETRACT_Z:
            trBuff += linenumber() + strG0_RETRACT_Z
        trBuff += (
            linenumber()
            + "G0 X"
            + format(float(drill_X.getValueAs(UNIT_FORMAT)), strFormat)
            + " Y"
            + format(float(drill_Y.getValueAs(UNIT_FORMAT)), strFormat)
            + "\n"
        )
        if CURRENT_Z > RETRACT_Z:
            # NIST GCODE 3.5.16.1 Preliminary and In-Between Motion says G0 to RETRACT_Z.
            trBuff += (
                linenumber()
                + "G0 Z"
                + format(float(RETRACT_Z.getValueAs(UNIT_FORMAT)), strFormat)
                + strF_Feedrate
            )
        last_Stop_Z = RETRACT_Z

        # drill moves
        if cmd in ("G81", "G82"):
            trBuff += (
                linenumber()
                + "G1 Z"
                + format(float(drill_Z.getValueAs(UNIT_FORMAT)), strFormat)
                + strF_Feedrate
            )
            # pause where applicable
            if cmd == "G82":
                trBuff += linenumber() + "G4 P" + str(drill_DwellTime) + "\n"
            trBuff += linenumber() + strG0_RETRACT_Z
        else:  # 'G83'
            if params["Q"] != 0:
                while 1:
                    if last_Stop_Z != RETRACT_Z:
                        clearance_depth = (
                            last_Stop_Z + a_bit
                        )  # rapid move to just short of last drilling depth
                        trBuff += (
                            linenumber()
                            + "G0 Z"
                            + format(
                                float(clearance_depth.getValueAs(UNIT_FORMAT)),
                                strFormat,
                            )
                            + "\n"
                        )
                    next_Stop_Z = last_Stop_Z - drill_Step
                    if next_Stop_Z > drill_Z:
                        trBuff += (
                            linenumber()
                            + "G1 Z"
                            + format(
                                float(next_Stop_Z.getValueAs(UNIT_FORMAT)), strFormat
                            )
                            + strF_Feedrate
                        )
                        trBuff += linenumber() + strG0_RETRACT_Z
                        last_Stop_Z = next_Stop_Z
                    else:
                        trBuff += (
                            linenumber()
                            + "G1 Z"
                            + format(float(drill_Z.getValueAs(UNIT_FORMAT)), strFormat)
                            + strF_Feedrate
                        )

                        if DRILL_RETRACT_MODE == "G98" :
                            trBuff += (linenumber() + strG0_Initial_Z)
                        else:
                            trBuff += (linenumber() + strG0_RETRACT_Z)
                        break

    except Exception as e:
        pass

    if MOTION_MODE == "G91":
        trBuff += linenumber() + "G91"  # Restore if changed

    return trBuff


# print(__name__ + ": GCode postprocessor loaded.")
