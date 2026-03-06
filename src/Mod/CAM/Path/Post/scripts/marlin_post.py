# SPDX-License-Identifier: LGPL-2.1-or-later

# *****************************************************************************
# *                                                                           *
# *   (c) sliptonic (shopinthewoods@gmail.com) 2014                           *
# *   (c) Gauthier Briere - 2018, 2019                                        *
# *   (c) Schildkroet - 2019-2020                                             *
# *   (c) Gary L Hasson - 2020                                                *
# *                                                                           *
# *   This file is part of the FreeCAD CAx development system.                *
# *                                                                           *
# *   This program is free software; you can redistribute it and/or modify    *
# *   it under the terms of the GNU Lesser General Public License (LGPL)      *
# *   as published by the Free Software Foundation; either version 2 of       *
# *   the License, or (at your option) any later version.                     *
# *   for detail see the LICENCE text file.                                   *
# *                                                                           *
# *   FreeCAD is distributed in the hope that it will be useful,              *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
# *   GNU Lesser General Public License for more details.                     *
# *                                                                           *
# *   You should have received a copy of the GNU Library General Public       *
# *   License along with FreeCAD; if not, write to the Free Software          *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307    *
# *   USA                                                                     *
# *                                                                           *
# *****************************************************************************


from datetime import datetime
import argparse
import shlex
import FreeCAD
from FreeCAD import Units
import Path
import Path.Base.Util as PathUtil
import Path.Post.Utils as PostUtils
import PathScripts.PathUtils as PathUtils
from builtins import open as pyopen

Revised = "2020-11-03"  # Revision date for this file.

# *****************************************************************************
# *   Due to the fundamentals of the FreeCAD pre-processor,                   *
# *   this post processor can only operate in the following modes:            *
# *   G90 Absolute positions                                                  *
# *   G21 Metric units (mm)                                                   *
# *   G17 XY plane (3 axis vertical milling only)                             *
# *                                                                           *
# *****************************************************************************


TOOLTIP = """
Generate g-code from a Path that is compatible with the Marlin controller.
import marlin_post
marlin_post.export(object, "/path/to/file.nc")
"""

# *****************************************************************************
# * Initial configuration, not changeable                                     *
# *****************************************************************************
MOTION_MODE = "G90"  # G90 only, for absolute moves
WORK_PLANE = "G17"  # G17 only, XY plane, for vertical milling
UNITS = "G21"  # G21 only, for metric
UNIT_FORMAT = "mm"
UNIT_FEED_FORMAT = "mm/min"

# *****************************************************************************
# * Initial configuration, changeable via command line arguments              *
# *****************************************************************************
PRECISION = 3  # Decimal places displayed for metric
DRILL_RETRACT_MODE = "G98"  # End of drill-cycle retractation type. G99
# is the alternative.
TRANSLATE_DRILL_CYCLES = True  # If true, G81, G82, and G83 are translated
# into G0/G1 moves
OUTPUT_TOOL_CHANGE = False  # Do not output M6 tool change (comment it)
RETURN_TO = None  # None = No movement at end of program
SPINDLE_WAIT = 3  # 0 == No waiting after M3 / M4
MODAL = False  # True: Commands are suppressed if they are
# the same as the previous line
LINENR = 100  # Line number starting value
LINEINCR = 10  # Line number increment
PRE_OPERATION = """"""  # Pre operation text will be inserted before
# every operation
POST_OPERATION = """"""  # Post operation text will be inserted after
# every operation
TOOL_CHANGE = """"""  # Tool Change commands will be inserted
# before a tool change

# Default preamble text will appear at the beginning of the gcode output file.
PREAMBLE = """"""

# Default postamble text will appear following the last operation.
POSTAMBLE = """M5
"""

# *****************************************************************************
# * Initial gcode output options, changeable via command line arguments       *
# *****************************************************************************
OUTPUT_HEADER = True  # Output header in output gcode file
OUTPUT_COMMENTS = True  # Comments in output gcode file
OUTPUT_FINISH = False  # Include an operation finished comment
OUTPUT_PATH = False  # Include a Path: comment
OUTPUT_MARLIN_CONFIG = False  # Display expected #defines for Marlin config
OUTPUT_LINE_NUMBERS = False  # Output line numbers in output gcode file
OUTPUT_BCNC = False  # Add bCNC operation block headers in output
# gcode file
SHOW_EDITOR = True  # Display the resulting gcode file

# *****************************************************************************
# * Command line arguments                                                    *
# *****************************************************************************
parser = argparse.ArgumentParser(prog="marlin", add_help=False)
parser.add_argument("--header", action="store_true", help="output headers (default)")
parser.add_argument("--no-header", action="store_true", help="suppress header output")
parser.add_argument("--comments", action="store_true", help="output comment (default)")
parser.add_argument("--no-comments", action="store_true", help="suppress comment output")
parser.add_argument("--finish-comments", action="store_true", help="output finish-comment")
parser.add_argument(
    "--no-finish-comments",
    action="store_true",
    help="suppress finish-comment output (default)",
)
parser.add_argument("--path-comments", action="store_true", help="output path-comment")
parser.add_argument(
    "--no-path-comments",
    action="store_true",
    help="suppress path-comment output (default)",
)
parser.add_argument("--marlin-config", action="store_true", help="output #defines for Marlin")
parser.add_argument(
    "--no-marlin-config",
    action="store_true",
    help="suppress output #defines for Marlin (default)",
)
parser.add_argument("--line-numbers", action="store_true", help="prefix with line numbers")
parser.add_argument(
    "--no-line-numbers",
    action="store_true",
    help="do not prefix with line numbers (default)",
)
parser.add_argument(
    "--show-editor",
    action="store_true",
    help="pop up editor before writing output (default)",
)
parser.add_argument(
    "--no-show-editor",
    action="store_true",
    help="do not pop up editor before writing output",
)
parser.add_argument("--precision", default="3", help="number of digits of precision, default=3")
parser.add_argument(
    "--translate_drill",
    action="store_true",
    help="translate drill cycles G81, G82, G83 into G0/G1 movements (default)",
)
parser.add_argument(
    "--no-translate_drill",
    action="store_true",
    help="do not translate drill cycles G81, G82, G83 into G0/G1 movements",
)
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
parser.add_argument("--tool-change", action="store_true", help="Insert M6 for all tool changes")
parser.add_argument(
    "--wait-for-spindle",
    type=int,
    default=3,
    help="Wait for spindle to reach desired speed after M3 or M4, default=0",
)
parser.add_argument(
    "--return-to",
    default="",
    help='When done, move to, e.g. --return-to="3.175, 4.702, 50.915"',
)
parser.add_argument(
    "--bcnc",
    action="store_true",
    help="Add Job operations as bCNC block headers. \
    Consider suppressing existing comments: Add argument --no-comments",
)
parser.add_argument(
    "--no-bcnc", action="store_true", help="suppress bCNC block header output (default)"
)
TOOLTIP_ARGS = parser.format_help()

# *****************************************************************************
# * Marlin 2.x:                                                               *
# * Ignores commands that it does not implement.                              *
# * Some machining-related commands may conflict with gcodes that Marlin      *
# * has assigned to 3D printing commands.                                     *
# * Therefore, check FreeCAD gcodes for conflicts with Marlin.                *
# * Marlin 2.x ignores the ENTIRE COMMAND LINE if there is more than          *
# * one command per line.                                                     *
# *****************************************************************************

# *****************************************************************************
# * Internal global variables                                                 *
# *****************************************************************************
MOTION_COMMANDS = ["G0", "G00", "G1", "G01", "G2", "G02", "G3", "G03"]
RAPID_MOVES = ["G0", "G00"]  # Rapid moves gcode commands definition
SUPPRESS_COMMANDS = [""]  # These commands are ignored by commenting them out
COMMAND_SPACE = " "
# Global variables storing current position (Use None for safety.)
CURRENT_X = None
CURRENT_Y = None
CURRENT_Z = None


def processArguments(argstring):
    global OUTPUT_HEADER
    global OUTPUT_COMMENTS
    global OUTPUT_FINISH
    global OUTPUT_PATH
    global OUTPUT_MARLIN_CONFIG
    global OUTPUT_LINE_NUMBERS
    global SHOW_EDITOR
    global PREAMBLE
    global POSTAMBLE
    global UNITS
    global UNIT_FEED_FORMAT
    global UNIT_FORMAT
    global TRANSLATE_DRILL_CYCLES
    global OUTPUT_TOOL_CHANGE
    global SPINDLE_WAIT
    global RETURN_TO
    global OUTPUT_BCNC
    global PRECISION

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
        if args.no_finish_comments:
            OUTPUT_FINISH = False
        if args.finish_comments:
            OUTPUT_FINISH = True
        if args.no_path_comments:
            OUTPUT_PATH = False
        if args.path_comments:
            OUTPUT_PATH = True
        if args.no_marlin_config:
            OUTPUT_MARLIN_CONFIG = False
        if args.marlin_config:
            OUTPUT_MARLIN_CONFIG = True
        if args.no_line_numbers:
            OUTPUT_LINE_NUMBERS = False
        if args.line_numbers:
            OUTPUT_LINE_NUMBERS = True
        if args.no_show_editor:
            SHOW_EDITOR = False
        if args.show_editor:
            SHOW_EDITOR = True
        if args.preamble is not None:
            PREAMBLE = args.preamble.replace("\\n", "\n")
        if args.postamble is not None:
            POSTAMBLE = args.postamble.replace("\\n", "\n")
        if args.no_translate_drill:
            TRANSLATE_DRILL_CYCLES = False
        if args.translate_drill:
            TRANSLATE_DRILL_CYCLES = True
        if args.tool_change:
            OUTPUT_TOOL_CHANGE = True
        if args.return_to:
            RETURN_TO = args.return_to
            if RETURN_TO.find(",") == -1:
                RETURN_TO = None
                print("--return-to coordinates must be specified as:")
                print('--return-to "x.n,y.n,z.n"')
        if args.bcnc:
            OUTPUT_BCNC = True
        if args.no_bcnc:
            OUTPUT_BCNC = False
        SPINDLE_WAIT = args.wait_for_spindle
        PRECISION = args.precision

    except Exception as e:
        return False

    return True


# For debug...
def dump(obj):
    for attr in dir(obj):
        try:
            if attr.startswith("__"):
                continue
            print(">" + attr + "<")
            attr_text = "%s = %s" % (attr, getattr(obj, attr))
            if attr in ["HorizFeed", "VertFeed"]:
                print("==============\n", attr_text)
                if "mm/s" in attr_text:
                    print("===> metric values <===")
        except Exception:  # Insignificant errors
            # print('==>', obj, attr)
            pass


def export(objectslist, filename, argstring):
    if not processArguments(argstring):
        return None

    global UNITS
    global UNIT_FORMAT
    global UNIT_FEED_FORMAT
    global MOTION_MODE
    global SUPPRESS_COMMANDS

    # print("Post Processor: " + __name__ + " postprocessing...")  # Commented to reduce test noise
    gcode = ""

    # Write header:
    if OUTPUT_HEADER:
        gcode += linenumber() + "(Exported by FreeCAD)\n"
        gcode += linenumber() + "(Post Processor: " + __name__
        gcode += ".py, version: " + Revised + ")\n"
        gcode += linenumber() + "(Output Time:" + str(datetime.now()) + ")\n"

    # Suppress drill-cycle commands:
    if TRANSLATE_DRILL_CYCLES:
        SUPPRESS_COMMANDS += ["G80", "G98", "G99"]

    # Write the preamble:
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(Begin preamble)\n"
    for line in PREAMBLE.splitlines():
        gcode += linenumber() + line + "\n"

    # Write these settings AFTER the preamble,
    # to prevent the preamble from changing these:
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(Default Configuration)\n"
    gcode += linenumber() + MOTION_MODE + "\n"
    gcode += linenumber() + UNITS + "\n"
    gcode += linenumber() + WORK_PLANE + "\n"

    for obj in objectslist:
        # Debug...
        # print('\n' + '*'*70 + '\n')
        # dump(obj)
        # print('\n' + '*'*70 + '\n')
        if not hasattr(obj, "Path"):
            print(
                "The object " + obj.Name + " is not a path. Please select only path and Compounds."
            )
            return

        # Skip inactive operations:
        if not PathUtil.activeForOp(obj):
            continue

        # Do the pre_op:
        if OUTPUT_BCNC:
            gcode += linenumber() + "(Block-name: " + obj.Label + ")\n"
            gcode += linenumber() + "(Block-expand: 0)\n"
            gcode += linenumber() + "(Block-enable: 1)\n"
        if OUTPUT_COMMENTS:
            gcode += linenumber() + "(Begin operation: " + obj.Label + ")\n"
        for line in PRE_OPERATION.splitlines(True):
            gcode += linenumber() + line

        # Get coolant mode:
        coolantMode = PathUtil.coolantModeForOp(obj)

        # Turn coolant on if required:
        if OUTPUT_COMMENTS:
            if not coolantMode == "None":
                gcode += linenumber() + "(Coolant On:" + coolantMode + ")\n"
        if coolantMode == "Flood":
            gcode += linenumber() + "M8\n"
        if coolantMode == "Mist":
            gcode += linenumber() + "M7\n"

        # Parse the op:
        gcode += parse(obj)

        # Do the post_op:
        if OUTPUT_COMMENTS and OUTPUT_FINISH:
            gcode += linenumber() + "(Finish operation: " + obj.Label + ")\n"
        for line in POST_OPERATION.splitlines(True):
            gcode += linenumber() + line

        # Turn coolant off if previously enabled:
        if not coolantMode == "None":
            if OUTPUT_COMMENTS:
                gcode += linenumber() + "(Coolant Off:" + coolantMode + ")\n"
            gcode += linenumber() + "M9\n"

    # Do the post_amble:
    if OUTPUT_BCNC:
        gcode += linenumber() + "(Block-name: post_amble)\n"
        gcode += linenumber() + "(Block-expand: 0)\n"
        gcode += linenumber() + "(Block-enable: 1)\n"
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(Begin postamble)\n"
    for line in POSTAMBLE.splitlines():
        gcode += linenumber() + line + "\n"

    # Optionally add a final XYZ position to the end of the gcode:
    if RETURN_TO:
        first_comma = RETURN_TO.find(",")
        last_comma = RETURN_TO.rfind(",")  # == first_comma if only one comma
        ref_X = " X" + RETURN_TO[0:first_comma].strip()

        # Z is optional:
        if last_comma != first_comma:
            ref_Z = " Z" + RETURN_TO[last_comma + 1 :].strip()
            ref_Y = " Y" + RETURN_TO[first_comma + 1 : last_comma].strip()
        else:
            ref_Z = ""
            ref_Y = " Y" + RETURN_TO[first_comma + 1 :].strip()

        gcode += linenumber() + "G0" + ref_X + ref_Y + ref_Z + "\n"

    # Optionally add recommended Marlin 2.x configuration to gcode file:
    if OUTPUT_MARLIN_CONFIG:
        gcode += linenumber() + "(Marlin 2.x Configuration)\n"
        gcode += linenumber() + "(The following should be enabled in)\n"
        gcode += linenumber() + "(the configuration files of Marlin 2.x)\n"
        gcode += linenumber() + "(#define ARC_SUPPORT)\n"
        gcode += linenumber() + "(#define CNC_COORDINATE_SYSTEMS)\n"
        gcode += linenumber() + "(#define PAREN_COMMENTS)\n"
        gcode += linenumber() + "(#define GCODE_MOTION_MODES)\n"
        gcode += linenumber() + "(#define G0_FEEDRATE)\n"
        gcode += linenumber() + "(define VARIABLE_G0_FEEDRATE)\n"

    # Show the gcode result dialog:
    if FreeCAD.GuiUp and SHOW_EDITOR:
        dia = PostUtils.GCodeEditorDialog()
        dia.editor.setPlainText(gcode)
        result = dia.exec_()
        if result:
            final = dia.editor.toPlainText()
        else:
            final = gcode
    else:
        final = gcode

    # print("Done postprocessing.")  # Commented to reduce test noise

    # Write the file:
    if not filename == "-":
        gfile = pyopen(filename, "w")
        gfile.write(final)
        gfile.close()

    return final


def linenumber():
    if not OUTPUT_LINE_NUMBERS:
        return ""
    global LINENR
    global LINEINCR
    LINENR += LINEINCR
    return "N" + str(LINENR) + " "


def format_outlist(strTable):
    # construct the line for the final output
    global COMMAND_SPACE
    s = ""
    for w in strTable:
        s += w + COMMAND_SPACE
    return s.strip()


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

    else:  # Parsing simple path
        # groups might contain non-path things like stock.
        if not hasattr(pathobj, "Path"):
            return out

        if OUTPUT_COMMENTS and OUTPUT_PATH:
            out += linenumber() + "(Path: " + pathobj.Label + ")\n"

        for c in PathUtils.getPathWithPlacement(pathobj).Commands:
            outlist = []
            command = c.Name
            outlist.append(command)
            # Debug:
            # print('pathobj.Path.Commands:', c)

            # If modal is True, delete duplicate commands:
            if MODAL:
                if command == lastcommand:
                    outlist.pop(0)

            # Add the remaining parameters in order:
            for param in params:
                if param in c.Parameters:
                    if param == "F":
                        if command not in RAPID_MOVES:
                            feedRate = Units.Quantity(c.Parameters["F"], FreeCAD.Units.Velocity)
                            if feedRate.getValueAs(UNIT_FEED_FORMAT) > 0.0:
                                outlist.append(
                                    param
                                    + format(
                                        float(feedRate.getValueAs(UNIT_FEED_FORMAT)),
                                        precision_string,
                                    )
                                )
                    elif param in ["T", "H", "D", "S", "P", "L"]:
                        outlist.append(param + str(c.Parameters[param]))
                    elif param in ["A", "B", "C"]:
                        outlist.append(param + format(c.Parameters[param], precision_string))
                    # [X, Y, Z, U, V, W, I, J, K, R, Q]
                    else:
                        pos = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)
                        outlist.append(
                            param + format(float(pos.getValueAs(UNIT_FORMAT)), precision_string)
                        )

            # Store the latest command:
            lastcommand = command

            # Capture the current position for subsequent calculations:
            if command in MOTION_COMMANDS:
                if "X" in c.Parameters:
                    CURRENT_X = Units.Quantity(c.Parameters["X"], FreeCAD.Units.Length)
                if "Y" in c.Parameters:
                    CURRENT_Y = Units.Quantity(c.Parameters["Y"], FreeCAD.Units.Length)
                if "Z" in c.Parameters:
                    CURRENT_Z = Units.Quantity(c.Parameters["Z"], FreeCAD.Units.Length)

            if command in ("G98", "G99"):
                DRILL_RETRACT_MODE = command

            if TRANSLATE_DRILL_CYCLES:
                if command in ("G81", "G82", "G83"):
                    out += drill_translate(outlist, command, c.Parameters)
                    # Erase the line just translated:
                    outlist = []

            if SPINDLE_WAIT > 0:
                if command in ("M3", "M03", "M4", "M04"):
                    out += linenumber() + format_outlist(outlist) + "\n"
                    # Marlin: P for milliseconds, S for seconds, change P to S
                    out += linenumber()
                    out += format_outlist(["G4", "S%s" % SPINDLE_WAIT])
                    out += "\n"
                    outlist = []

            # Check for Tool Change:
            if command in ("M6", "M06"):
                if OUTPUT_COMMENTS:
                    out += linenumber() + "(Begin toolchange)\n"
                if OUTPUT_TOOL_CHANGE:
                    for line in TOOL_CHANGE.splitlines(True):
                        out += linenumber() + line
                if not OUTPUT_TOOL_CHANGE and OUTPUT_COMMENTS:
                    outlist[0] = "(" + outlist[0]
                    outlist[-1] = outlist[-1] + ")"
                if not OUTPUT_TOOL_CHANGE and not OUTPUT_COMMENTS:
                    outlist = []

            if command == "message":
                if OUTPUT_COMMENTS:
                    outlist.pop(0)  # remove the command
                else:
                    out = []

            if command in SUPPRESS_COMMANDS:
                outlist[0] = "(" + outlist[0]
                outlist[-1] = outlist[-1] + ")"

            # Remove embedded comments:
            if not OUTPUT_COMMENTS:
                tmplist = []
                list_index = 0
                while list_index < len(outlist):
                    left_index = outlist[list_index].find("(")
                    if left_index == -1:  # Not a comment
                        tmplist.append(outlist[list_index])
                    else:  # This line contains a comment, and possibly more
                        right_index = outlist[list_index].find(")")
                        comment_area = outlist[list_index][left_index : right_index + 1]
                        line_minus_comment = outlist[list_index].replace(comment_area, "").strip()
                        if line_minus_comment:
                            # Line contained more than just a comment
                            tmplist.append(line_minus_comment)
                    list_index += 1
                # Done removing comments
                outlist = tmplist

            # Prepend a line number and append a newline
            if len(outlist) >= 1:
                out += linenumber() + format_outlist(outlist) + "\n"

    return out


# *****************************************************************************
# * As of Marlin 2.0.7.bugfix, canned drill cycles do not exist.              *
# * The following code converts FreeCAD's canned drill cycles into            *
# * gcode that Marlin can use.                                                *
# *****************************************************************************
def drill_translate(outlist, cmd, params):
    global DRILL_RETRACT_MODE
    global MOTION_MODE
    global CURRENT_X
    global CURRENT_Y
    global CURRENT_Z
    global UNITS
    global UNIT_FORMAT
    global UNIT_FEED_FORMAT

    class Drill:  # Using a class is necessary for the nested functions.
        gcode = ""

    strFormat = "." + str(PRECISION) + "f"

    if OUTPUT_COMMENTS:  # Comment the original command
        outlist[0] = "(" + outlist[0]
        outlist[-1] = outlist[-1] + ")"
        Drill.gcode += linenumber() + format_outlist(outlist) + "\n"

    # Cycle conversion only converts the cycles in the XY plane (G17).
    # --> ZX (G18) and YZ (G19) planes produce false gcode.
    drill_X = Units.Quantity(params["X"], FreeCAD.Units.Length)
    drill_Y = Units.Quantity(params["Y"], FreeCAD.Units.Length)
    drill_Z = Units.Quantity(params["Z"], FreeCAD.Units.Length)
    drill_R = Units.Quantity(params["R"], FreeCAD.Units.Length)
    drill_F = Units.Quantity(params["F"], FreeCAD.Units.Velocity)
    if cmd == "G82":
        drill_DwellTime = params["P"]
    elif cmd == "G83":
        drill_Step = Units.Quantity(params["Q"], FreeCAD.Units.Length)

    # R less than Z is error
    if drill_R < drill_Z:
        Drill.gcode += linenumber() + "(drill cycle error: R less than Z )\n"
        return Drill.gcode

    # Z height to retract to when drill cycle is done:
    if DRILL_RETRACT_MODE == "G98" and CURRENT_Z > drill_R:
        RETRACT_Z = CURRENT_Z
    else:
        RETRACT_Z = drill_R

    # Z motion nested functions:
    def rapid_Z_to(new_Z):
        Drill.gcode += linenumber() + "G0 Z"
        Drill.gcode += format(float(new_Z.getValueAs(UNIT_FORMAT)), strFormat) + "\n"

    def feed_Z_to(new_Z):
        Drill.gcode += linenumber() + "G1 Z"
        Drill.gcode += format(float(new_Z.getValueAs(UNIT_FORMAT)), strFormat) + " F"
        Drill.gcode += format(float(drill_F.getValueAs(UNIT_FEED_FORMAT)), ".2f") + "\n"

    # Make sure that Z is not below RETRACT_Z:
    if CURRENT_Z < RETRACT_Z:
        rapid_Z_to(RETRACT_Z)

    # Rapid to hole position XY:
    Drill.gcode += linenumber() + "G0 X"
    Drill.gcode += format(float(drill_X.getValueAs(UNIT_FORMAT)), strFormat) + " Y"
    Drill.gcode += format(float(drill_Y.getValueAs(UNIT_FORMAT)), strFormat) + "\n"

    # Rapid to R:
    rapid_Z_to(drill_R)

    # *************************************************************************
    # * Drill cycles:                                                         *
    # * G80 Cancel the drill cycle                                            *
    # * G81 Drill full depth in one pass                                      *
    # * G82 Drill full depth in one pass, and pause at the bottom             *
    # * G83 Drill in pecks, raising the drill to R height after each peck     *
    # * In preparation for a rapid to the next hole position:                 *
    # * G98 After the hole has been drilled, retract to the initial Z value   *
    # * G99 After the hole has been drilled, retract to R height              *
    # * Select G99 only if safe to move from hole to hole at the R height     *
    # *************************************************************************
    if cmd in ("G81", "G82"):
        feed_Z_to(drill_Z)  # Drill hole in one step
        if cmd == "G82":  # Dwell time delay at the bottom of the hole
            Drill.gcode += linenumber() + "G4 S" + str(drill_DwellTime) + "\n"
            # Marlin uses P for milliseconds, S for seconds, change P to S

    elif cmd == "G83":  # Peck drill cycle:
        chip_Space = drill_Step * 0.5
        next_Stop_Z = drill_R - drill_Step
        while next_Stop_Z >= drill_Z:
            feed_Z_to(next_Stop_Z)  # Drill one peck of depth

            # Set next depth, next_Stop_Z is still at the current hole depth
            if (next_Stop_Z - drill_Step) >= drill_Z:
                # Rapid up to clear chips:
                rapid_Z_to(drill_R)
                # Rapid down to just above last peck depth:
                rapid_Z_to(next_Stop_Z + chip_Space)
                # Update next_Stop_Z to next depth:
                next_Stop_Z -= drill_Step
            elif next_Stop_Z == drill_Z:
                break  # Done
            else:  # More to drill, but less than drill_Step
                # Rapid up to clear chips:
                rapid_Z_to(drill_R)
                # Rapid down to just above last peck depth:
                rapid_Z_to(next_Stop_Z + chip_Space)
                # Dril remainder of the hole depth:
                feed_Z_to(drill_Z)
                break  # Done
    rapid_Z_to(RETRACT_Z)  # Done, retract the drill

    return Drill.gcode


# print(__name__ + ': GCode postprocessor loaded.')

# PEP8 format passed using: http://pep8online.com/, which primarily covers
# indentation and line length. Some other aspects of PEP8 which have not
# been applied yet may be applied in future updates.
