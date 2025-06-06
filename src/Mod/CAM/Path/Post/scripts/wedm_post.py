# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   Reabased changes from relative_post.py to linuxcnc_post.py            *
# *   and updated functionality for old EDM machines by alromh87            *
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
real G-code suitable for a Wire EDM CNC. This postprocessor, once placed
in the appropriate PathScripts folder, can be used directly from inside
FreeCAD, via the GUI importer or via python scripts with:

import wedm_post
wedm_post.export(object,"/path/to/file.ncc","")
"""

now = datetime.datetime.now()

parser = argparse.ArgumentParser(prog="wedm", add_help=False)
parser.add_argument("--no-header", action="store_true", help="suppress header output")
parser.add_argument("--no-comments", action="store_true", help="suppress comment output")
parser.add_argument(
    "--comments-character", default="", help="Use provided character before comments"
)
parser.add_argument(
    "--command-space", default=" ", help="Use provided character as space in commands"
)
parser.add_argument("--endline-character", default="", help="Use provided character at end of line")
parser.add_argument("--line-numbers", action="store_true", help="prefix with line numbers")
parser.add_argument(
    "--no-show-editor",
    action="store_true",
    help="don't pop up editor before writing output",
)
parser.add_argument("--scale", default="1", help="Scale factor for coordinates")
parser.add_argument("--precision", default="3", help="number of digits of precision, default=3")
parser.add_argument("--fixed-length", default="0", help="use fixed length coordinates, default=0")
parser.add_argument(
    "--preamble",
    help='set commands to be issued before the first command, default="G17 G54 G40 G49 G80 G90\\n"',
)
parser.add_argument(
    "--postamble",
    help='set commands to be issued after the last command, default="M05\\nG17 G54 G90 G80 G40\\nM2\\n"',
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
parser.add_argument(
    "--use-rapids",
    action="store_true",
    help="Allow G0 on output",
)
parser.add_argument(
    "--omit-units",
    action="store_true",
    help="Don't output units G20/G21",
)
parser.add_argument("--relative", action="store_true", help="Generate Relative GCODE")
parser.add_argument(
    "--two-digit-codes",
    action="store_true",
    help="Add trailing 0 to codes lower than 10 (G1 -> G01)",
)
parser.add_argument("--force-sign", action="store_true", help="Always add sign to coordinates")
parser.add_argument(
    "--ignore-operations",
    default="",
    help="Ignore provided operations, use Labels and separate with ','",
)

TOOLTIP_ARGS = parser.format_help()

# These globals set common customization preferences
OUTPUT_COMMENTS = True
COMMENT_CHAR = ""
COMMAND_SPACE = " "
ENDLINE = ""
OUTPUT_HEADER = True
OUTPUT_LINE_NUMBERS = False
SHOW_EDITOR = True
MODAL = False  # if true commands are suppressed if the same as previous line.
USE_TLO = True  # if true G43 will be output following tool changes
OUTPUT_DOUBLES = True  # if false duplicate axis values are suppressed if the same as previous line.
LINENR = 100  # line number starting value
USE_RAPIDS = False
OMIT_UNITS = False
RELATIVE_GCODE = False
TWO_DIGIT_CODES = False
FORCE_SIGN = False

# These globals will be reflected in the Machine configuration of the project
UNITS = "G21"  # G21 for metric, G20 for us standard
UNIT_SPEED_FORMAT = "mm/min"
UNIT_FORMAT = "mm"

MACHINE_NAME = "Wire EDM"
CORNER_MIN = {"x": 0, "y": 0, "z": 0}
CORNER_MAX = {"x": 500, "y": 300, "z": 300}
PRECISION = 3

# Preamble text will appear at the beginning of the GCODE output file.
PREAMBLE = """G17 G54 G40 G49 G80 G90
"""


# Postamble text will appear following the last operation.
POSTAMBLE = """M05
G17 G54 G90 G80 G40
M2
"""

# Pre operation text will be inserted before every operation
PRE_OPERATION = """"""

# Post operation text will be inserted after every operation
POST_OPERATION = """"""

# Tool Change commands will be inserted before a tool change
TOOL_CHANGE = """"""


def processArguments(argstring):
    global OUTPUT_HEADER
    global OUTPUT_COMMENTS
    global COMMENT_CHAR
    global COMMAND_SPACE
    global ENDLINE
    global OUTPUT_LINE_NUMBERS
    global SHOW_EDITOR
    global SCALE
    global PRECISION
    global FIXED_LENGTH
    global PREAMBLE
    global POSTAMBLE
    global UNITS
    global UNIT_SPEED_FORMAT
    global UNIT_FORMAT
    global MODAL
    global USE_TLO
    global OUTPUT_DOUBLES
    global LOCATION  # keep track for incremental
    global USE_RAPIDS
    global OMIT_UNITS
    global RELATIVE_GCODE
    global TWO_DIGIT_CODES
    global FORCE_SIGN
    global IGNORE_OPERATIONS

    try:
        args = parser.parse_args(shlex.split(argstring))
        if args.no_header:
            OUTPUT_HEADER = False
        if args.no_comments:
            OUTPUT_COMMENTS = False
        COMMENT_CHAR = args.comments_character
        COMMAND_SPACE = args.command_space
        ENDLINE = args.endline_character
        if args.line_numbers:
            OUTPUT_LINE_NUMBERS = True
        if args.no_show_editor:
            SHOW_EDITOR = False
        SCALE = int(args.scale)
        PRECISION = args.precision
        FIXED_LENGTH = int(args.fixed_length)
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
            OUTPUT_DOUBLES = False
        if args.use_rapids:
            USE_RAPIDS = True
        if args.omit_units:
            OMIT_UNITS = True
        if args.relative:
            print("relative")
            RELATIVE_GCODE = True
            # PREAMBLE = PREAMBLE.replace("G90", "")
            # PREAMBLE = PREAMBLE.replace("G91", "")
            PREAMBLE += "\nG91"
            LOCATION = {"X": 0, "Y": 0, "Z": 0}
        if args.two_digit_codes:
            TWO_DIGIT_CODES = True
        if args.force_sign:
            FORCE_SIGN = True
        IGNORE_OPERATIONS = args.ignore_operations.split(",")

    except Exception as e:
        print(e)
        return False

    return True


def export(objectslist, filename, argstring):
    if not processArguments(argstring):
        return None
    global UNITS
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT
    global MACHINE_NAME

    for obj in objectslist:
        if not hasattr(obj, "Path"):
            print(
                "the object " + obj.Name + " is not a path. Please select only path and Compounds."
            )
            return None

    print("postprocessing...")
    gcode = ""

    # write header
    if OUTPUT_HEADER:
        gcode += linenumber() + COMMENT_CHAR + "(Exported by FreeCAD)\n"
        gcode += linenumber() + COMMENT_CHAR + "(Post Processor: " + __name__ + ")\n"
        gcode += linenumber() + COMMENT_CHAR + "(Output Time:" + str(now) + ")\n"

    # Write the preamble
    if OUTPUT_COMMENTS:
        gcode += linenumber() + COMMENT_CHAR + "(begin preamble)\n"
    for line in PREAMBLE.splitlines():
        gcode += linenumber() + line + "\n"
    if not OMIT_UNITS:
        gcode += linenumber() + UNITS + ENDLINE + "\n"

    for obj in objectslist:

        # Skip inactive operations
        if not PathUtil.activeForOp(obj):
            continue

        # fetch machine details
        job = PathUtils.findParentJob(obj)

        # TODO: Not sure of the use
        if hasattr(job, "MachineName"):
            MACHINE_NAME = job.MachineName

        if hasattr(job, "MachineUnits"):
            print(job.MachineUnits)
            if job.MachineUnits == "Metric":
                UNITS = "G21"
                UNIT_FORMAT = "mm"
                UNIT_SPEED_FORMAT = "mm/min"
            else:
                UNITS = "G20"
                UNIT_FORMAT = "in"
                UNIT_SPEED_FORMAT = "in/min"

        # ignore selected operations
        if obj.Label in IGNORE_OPERATIONS:
            continue
        # do the pre_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + COMMENT_CHAR + "(begin operation: %s)\n" % obj.Label
            gcode += (
                linenumber()
                + COMMENT_CHAR
                + "(machine: %s, %s)\n" % (MACHINE_NAME, UNIT_SPEED_FORMAT)
            )
        for line in PRE_OPERATION.splitlines(True):
            gcode += linenumber() + line

        # get coolant mode
        coolantMode = PathUtil.coolantModeForOp(obj)

        # turn coolant on if required
        if OUTPUT_COMMENTS:
            if not coolantMode == "None":
                gcode += linenumber() + COMMENT_CHAR + "(Coolant On:" + coolantMode + ")\n"
        if coolantMode == "Flood":
            gcode += linenumber() + "M8" + "\n"
        if coolantMode == "Mist":
            gcode += linenumber() + "M7" + "\n"

        # process the operation gcode
        gcode += parse(obj)

        # do the post_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + COMMENT_CHAR + "(finish operation: %s)\n" % obj.Label
        for line in POST_OPERATION.splitlines(True):
            gcode += linenumber() + line

        # turn coolant off if required
        if not coolantMode == "None":
            if OUTPUT_COMMENTS:
                gcode += linenumber() + COMMENT_CHAR + "(Coolant Off:" + coolantMode + ")\n"
            gcode += linenumber() + "M9" + "\n"

    # do the post_amble
    if OUTPUT_COMMENTS:
        gcode += linenumber() + COMMENT_CHAR + "(begin postamble)\n"
    for line in POSTAMBLE.splitlines():
        gcode += linenumber() + line + "\n"

    if FreeCAD.GuiUp and SHOW_EDITOR:
        final = gcode
        if len(gcode) > 100000:
            print("Skipping editor since output is greater than 100kb")
        else:
            dia = PostUtils.GCodeEditorDialog()
            dia.editor.setText(gcode)
            result = dia.exec_()
            if result:
                final = dia.editor.toPlainText()
    else:
        final = gcode

    print("done postprocessing.")

    if not filename == "-":
        gfile = pyopen(filename, "w")
        gfile.write(final)
        gfile.close()

    return final


def linenumber():
    global LINENR
    if OUTPUT_LINE_NUMBERS is True:
        LINENR += 10
        return "N" + str(LINENR) + COMMAND_SPACE
    return ""


def parse(pathobj):
    global PRECISION
    global MODAL
    global OUTPUT_DOUBLES
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT
    global RELATIVE_GCODE
    global LOCATION

    out = ""
    lastcommand = None
    precision_string = "." + str(PRECISION) + "f"
    currLocation = {}  # keep track for incremental

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
    firstmove = Path.Command("G0", {"X": 0, "Y": 0, "Z": 0, "F": 0.0})
    currLocation.update(firstmove.Parameters)  # set First location Parameters

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        # if OUTPUT_COMMENTS:
        #     out += linenumber() + COMMENT_CHAR + "(compound: " + pathobj.Label + ")\n"
        for p in pathobj.Group:
            out += parse(p)
        return out
    else:  # parsing simple path

        # groups might contain non-path things like stock.
        if not hasattr(pathobj, "Path"):
            return out

        # if OUTPUT_COMMENTS:
        #     out += linenumber() + COMMENT_CHAR + "(" + pathobj.Label + ")\n"

        for c in PathUtils.getPathWithPlacement(pathobj).Commands:
            # For Debug Only
            #     if OUTPUT_COMMENTS:
            #         out += linenumber() + COMMENT_CHAR + "(" + str(c) + ")\n"

            outstring = []
            command = c.Name
            if not USE_RAPIDS and (command == "G0" or command == "G00"):
                command = "G1"
            outstring.append(command)

            # if modal: suppress the command if it is the same as the last one
            if MODAL is True:
                if command == lastcommand:
                    outstring.pop(0)

            if command.startswith("("):  # command is a comment
                if OUTPUT_COMMENTS:  # Edit comment  with COMMENT_CHAR
                    outstring.insert(0, COMMENT_CHAR)
                else:
                    continue

            move_comands = ["G0", "G00", "G1", "G01"]
            # Now add the remaining parameters in order
            for param in params:
                if param in c.Parameters:
                    if param == "F" and (
                        currLocation[param] != c.Parameters[param] or OUTPUT_DOUBLES
                    ):
                        if command not in [
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
                    # For wire EDM we ignore values for Z
                    elif param == "Z":
                        continue
                    else:
                        if (
                            (not OUTPUT_DOUBLES)
                            and (param in currLocation)
                            and (currLocation[param] == c.Parameters[param])
                        ):
                            continue
                        else:
                            if RELATIVE_GCODE and (param != "I" and param != "J"):
                                pos = Units.Quantity(
                                    c.Parameters[param] - currLocation[param], FreeCAD.Units.Length
                                )
                                print(
                                    f"currlocation: {currLocation[param]} param: {c.Parameters[param]} pos: {pos}"
                                )
                                if pos == 0:
                                    # Remove no movement
                                    continue
                            else:
                                pos = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)

                            pos = pos * SCALE
                            sign = ""
                            if pos >= 0 and FORCE_SIGN:
                                sign = "+"

                            stringout = sign + format(
                                float(pos.getValueAs(UNIT_FORMAT)), precision_string
                            )

                            # Remove unneeded 0s on incremental moves this is needed since some numbers are zero only after applying precision
                            if (
                                RELATIVE_GCODE
                                and command in move_comands
                                and (float(stringout) == 0)
                            ):
                                continue
                            # Force trailing zeros
                            if FIXED_LENGTH > 0:
                                extra = 1 if stringout[0] in ["+", "-"] else 0
                                stringout = stringout.zfill(FIXED_LENGTH + extra)
                            outstring.append(param + stringout)

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
                if len(outstring) == 1 and (outstring[0] == "G0" or outstring[0] == "G1"):
                    # Don't write empty moves (Generated when Z moves are ignored for EDM)
                    print("Ignoring: " + command + ":" + outstring[0])
                    continue
                if TWO_DIGIT_CODES and len(command) == 2 and outstring[0] == command:
                    outstring[0] = command[0] + "0" + command[1]
                if OUTPUT_LINE_NUMBERS:
                    outstring.insert(0, (linenumber()))

                # append the line to the final output
                start = True
                for w in outstring:
                    if start:
                        start = False
                    else:
                        out += COMMAND_SPACE
                    out += w
                # Note: Do *not* strip `out`, since that forces the allocation
                # of a contiguous string & thus quadratic complexity.
                out += ENDLINE + "\n"

            # store the latest command after written
            lastcommand = command

        return out


# print(__name__ + " gcode postprocessor loaded.")
