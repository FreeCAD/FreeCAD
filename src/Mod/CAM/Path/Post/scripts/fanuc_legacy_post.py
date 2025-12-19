# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2021 shadowbane1000 <tyler@colberts.us>                 *
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
# ***************************************************************************/

import FreeCAD
from FreeCAD import Units
import Path
import argparse
import datetime
import shlex
import os.path
import Path.Base.Util as PathUtil
import Path.Post.Utils as PostUtils
import PathScripts.PathUtils as PathUtils
from builtins import open as pyopen

TOOLTIP = """
This is a postprocessor file for the Path workbench. It is used to
take a pseudo-G-code fragment outputted by a Path object, and output
real G-code suitable should be suitable for most Fanuc controllers.
It has only been tested on a 21i-MB controller on a 3 axis mill.
This postprocessor, once placed in the appropriate PathScripts folder,
can be used directly from inside FreeCAD, via the GUI importer or via
python scripts with:

import fanuc_post
fanuc_post.export(object,"/path/to/file.ncc","")
"""

# Preamble text will appear at the beginning of the GCODE output file.
DEFAULT_PREAMBLE = """G17 G54 G40 G49 G80 G90
"""

# Postamble text will appear following the last operation.
DEFAULT_POSTAMBLE = """M05
G17 G54 G90 G80 G40
M30
"""

now = datetime.datetime.now()

parser = argparse.ArgumentParser(prog="fanuc", add_help=False)
parser.add_argument("--no-header", action="store_true", help="suppress header output")
parser.add_argument("--no-comments", action="store_true", help="suppress comment output")
parser.add_argument("--line-numbers", action="store_true", help="prefix with line numbers")
parser.add_argument(
    "--no-show-editor",
    action="store_true",
    help="don't pop up editor before writing output",
)
parser.add_argument("--precision", help="number of digits of precision, default=3 (mm) or 4 (in)")
parser.add_argument(
    "--preamble",
    help='set commands to be issued before the first command, default="'
    + DEFAULT_PREAMBLE.replace("\n", "\\n")
    + '"',
)
parser.add_argument(
    "--postamble",
    help='set commands to be issued after the last command, default="'
    + DEFAULT_POSTAMBLE.replace("\n", "\\n")
    + '"',
)
parser.add_argument(
    "--inches", action="store_true", help="Convert output for US imperial mode (G20)"
)
parser.add_argument(
    "--no-modal",
    action="store_true",
    help="Don't output the Same G-command Name USE NonModal Mode",
)
parser.add_argument(
    "--no-axis-modal", action="store_true", help="Don't output the Same Axis Value Mode"
)
parser.add_argument(
    "--no-tlo",
    action="store_true",
    help="suppress tool length offset (G43) following tool changes",
)
parser.add_argument(
    "--end-spindle-empty",
    action="store_true",
    help="place last tool in tool change carousel before postamble",
)

TOOLTIP_ARGS = parser.format_help()

# These globals set common customization preferences
OUTPUT_COMMENTS = True
OUTPUT_HEADER = True
OUTPUT_LINE_NUMBERS = False
SHOW_EDITOR = True
MODAL = True  # if true commands are suppressed if the same as previous line.
USE_TLO = True  # if true G43 will be output following tool changes
OUTPUT_DOUBLES = (
    False  # if false duplicate axis values are suppressed if the same as previous line.
)
COMMAND_SPACE = " "
LINENR = 100  # line number starting value

END_SPINDLE_EMPTY = False

# These globals will be reflected in the Machine configuration of the project
UNITS = "G21"  # G21 for metric, G20 for us standard
UNIT_SPEED_FORMAT = "mm/min"
UNIT_FORMAT = "mm"

MACHINE_NAME = "fanuc"
CORNER_MIN = {"x": 0, "y": 0, "z": 0}
CORNER_MAX = {"x": 500, "y": 300, "z": 300}
PRECISION = 3

# this global is used to pass spindle speed from the tool command into the machining command for
# rigid tapping.
tapSpeed = 0

PREAMBLE = DEFAULT_PREAMBLE
POSTAMBLE = DEFAULT_POSTAMBLE

# Pre operation text will be inserted before every operation
PRE_OPERATION = """"""

# Post operation text will be inserted after every operation
POST_OPERATION = """"""

# Tool Change commands will be inserted before a tool change
# Move to tool change Z position
TOOL_CHANGE = """G28 G91 Z0
"""

# List of drill G codes where some parameters are required and their
# required parameters.
DRILL_OPERATION = ("G73", "G81", "G82", "G83", "G84", "G85")
DRILL_PARAM_REQ = ("L", "P", "Q", "R", "Z")


def processArguments(argstring):
    global OUTPUT_HEADER
    global OUTPUT_COMMENTS
    global OUTPUT_LINE_NUMBERS
    global SHOW_EDITOR
    global PRECISION
    global DEFAULT_PREAMBLE
    global DEFAULT_POSTAMBLE
    global PREAMBLE
    global POSTAMBLE
    global UNITS
    global UNIT_SPEED_FORMAT
    global UNIT_FORMAT
    global MODAL
    global USE_TLO
    global END_SPINDLE_EMPTY
    global OUTPUT_DOUBLES
    global LINENR

    try:
        args = parser.parse_args(shlex.split(argstring))
        if args.no_header:
            OUTPUT_HEADER = False
        else:
            OUTPUT_HEADER = True
        if args.no_comments:
            OUTPUT_COMMENTS = False
        else:
            OUTPUT_COMMENTS = True
        if args.line_numbers:
            OUTPUT_LINE_NUMBERS = True
            LINENR = 100
        else:
            OUTPUT_LINE_NUMBERS = False
        if args.no_show_editor:
            SHOW_EDITOR = False
        else:
            SHOW_EDITOR = True
        # print("Show editor = %s" % SHOW_EDITOR)  # Commented to reduce test noise
        if args.preamble is not None:
            PREAMBLE = args.preamble.replace("\\n", "\n")
        else:
            PREAMBLE = DEFAULT_PREAMBLE
        if args.postamble is not None:
            POSTAMBLE = args.postamble.replace("\\n", "\n")
        else:
            POSTAMBLE = DEFAULT_POSTAMBLE
        if args.inches:
            UNITS = "G20"
            UNIT_SPEED_FORMAT = "in/min"
            UNIT_FORMAT = "in"
            PRECISION = 4
        else:
            UNITS = "G21"
            UNIT_SPEED_FORMAT = "mm/min"
            UNIT_FORMAT = "mm"
            PRECISION = 3
        if args.precision:
            PRECISION = int(args.precision)
        if args.no_modal:
            MODAL = False
        else:
            MODAL = True
        if args.no_tlo:
            USE_TLO = False
        else:
            USE_TLO = True
        if args.no_axis_modal:
            OUTPUT_DOUBLES = True
        else:
            OUTPUT_DOUBLES = False
        if args.end_spindle_empty:
            END_SPINDLE_EMPTY = True
        else:
            END_SPINDLE_EMPTY = False

    except Exception:
        return False

    return True


def export(objectslist, filename, argstring):
    if not processArguments(argstring):
        return None
    global UNITS
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT
    global HORIZRAPID
    global VERTRAPID

    for obj in objectslist:
        if not hasattr(obj, "Path"):
            print(
                "the object " + obj.Name + " is not a path. Please select only path and Compounds."
            )
            return None

    # print("postprocessing...")  # Commented to reduce test noise
    gcode = ""

    gcode += "%\n"

    # write header
    if OUTPUT_HEADER:
        # Get current version info
        major = int(FreeCAD.ConfigGet("BuildVersionMajor"))
        minor = int(FreeCAD.ConfigGet("BuildVersionMinor"))

        # the filename variable always contain "-", so unable to
        # provide more accurate information.
        gcode += "(" + "FREECAD-FILENAME-GOES-HERE" + ", " + "JOB-NAME-GOES-HERE" + ")\n"
        gcode += (
            linenumber() + "(POST PROCESSOR: FANUC USING FREECAD %d.%d" % (major, minor) + ")\n"
        )
        gcode += linenumber() + "(OUTPUT TIME:" + str(now).upper() + ")\n"

    # Write the preamble
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(BEGIN PREAMBLE)\n"
    for line in PREAMBLE.splitlines():
        gcode += linenumber() + line + "\n"
    gcode += linenumber() + UNITS + "\n"

    for obj in objectslist:

        # to stay compatible with FreeCAD 1.0
        def activeForOp(obj):
            # The activeForOp method is available since 2025-05-04 /
            # commit 1e87d8e6681b755b9757f94b1201e50eb84b28a2
            if hasattr(PathUtil, "activeForOp"):
                return PathUtil.activeForOp(obj)
            if hasattr(obj, "Active"):
                return obj.Active
            if hasattr(obj, "Base") and hasattr(obj.Base, "Active"):
                return obj.Base.Active
            return True

        # Skip inactive operations
        if not activeForOp(obj):
            continue

        # do the pre_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + "(BEGIN OPERATION: %s)\n" % obj.Label.upper()
            gcode += linenumber() + "(MACHINE UNITS: %s)\n" % (UNIT_SPEED_FORMAT.upper())
        for line in PRE_OPERATION.splitlines(True):
            gcode += linenumber() + line

        # to stay compatible with FreeCAD 1.0
        def coolantModeForOp(obj):
            # The coolantModeForOp method is available since
            # 2025-05-04 / commit
            # 1e87d8e6681b755b9757f94b1201e50eb84b28a2
            if hasattr(PathUtil, "coolantModeForOp"):
                return PathUtil.coolantModeForOp(obj)
            if (
                hasattr(obj, "CoolantMode")
                or hasattr(obj, "Base")
                and hasattr(obj.Base, "CoolantMode")
            ):
                if hasattr(obj, "CoolantMode"):
                    return obj.CoolantMode
                else:
                    return obj.Base.CoolantMode
            return "None"

        # get coolant mode
        coolantMode = coolantModeForOp(obj)

        # turn coolant on if required
        if OUTPUT_COMMENTS:
            if not coolantMode == "None":
                gcode += linenumber() + "(COOLANT ON:" + coolantMode.upper() + ")\n"
        if coolantMode == "Flood":
            gcode += linenumber() + "M8" + "\n"
        if coolantMode == "Mist":
            gcode += linenumber() + "M7" + "\n"

        # process the operation gcode
        gcode += parse(obj)

        # do the post_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + "(FINISH OPERATION: %s)\n" % obj.Label.upper()
        for line in POST_OPERATION.splitlines(True):
            gcode += linenumber() + line

        # turn coolant off if required
        if not coolantMode == "None":
            if OUTPUT_COMMENTS:
                gcode += linenumber() + "(COOLANT OFF:" + coolantMode.upper() + ")\n"
            gcode += linenumber() + "M9" + "\n"

    if END_SPINDLE_EMPTY:
        if OUTPUT_COMMENTS:
            gcode += "(BEGIN MAKING SPINDLE EMPTY)\n"
        gcode += linenumber() + "M05\n"
        for line in TOOL_CHANGE.splitlines(True):
            gcode += linenumber() + line
        gcode += linenumber() + "M6 T0\n"
    # do the post_amble
    if OUTPUT_COMMENTS:
        gcode += "(BEGIN POSTAMBLE)\n"
    for line in POSTAMBLE.splitlines():
        gcode += linenumber() + line + "\n"

    if FreeCAD.GuiUp and SHOW_EDITOR:
        dia = PostUtils.GCodeEditorDialog()

        # Workaround for 1.1 while we wait for
        # https://github.com/FreeCAD/FreeCAD/pull/26008 to be merged.
        if hasattr(dia.editor, "setPlainText"):
            dia.editor.setPlainText(gcode)
        else:
            dia.editor.setText(gcode)
        result = dia.exec_()
        if result:
            final = dia.editor.toPlainText()
        else:
            final = gcode
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
    global DRILL_OPERATION
    global DRILL_PARAM_REQ
    global MODAL
    global OUTPUT_DOUBLES
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT
    global tapSpeed

    out = ""
    lastcommand = None
    precision_string = "." + str(PRECISION) + "f"
    currLocation = {}  # keep track for no doubles
    print("Startup!")

    # the order of parameters
    # arcs need work.  original code from mach3_4 doesn't want K properties on XY plane.  Not sure
    # what fanuc does here.
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

        commands = PathUtils.getPathWithPlacement(pathobj).Commands
        for index, c in enumerate(commands):

            outstring = []
            outsuffix = []
            command = c.Name
            if index + 1 == len(commands):
                nextcommand = ""
            else:
                nextcommand = commands[index + 1].Name

            if adaptiveOp and c.Name in ["G0", "G00"]:
                if opHorizRapid and opVertRapid:
                    command = "G1"
                else:
                    outstring.append("(TOOL CONTROLLER RAPID VALUES ARE UNSET)" + "\n")

            # suppress moves in fixture selection
            if pathobj.Label == "Fixture":
                if command == "G0":
                    continue

            # if tool a tap, we thread tap, so stop the spindle for now.
            # This only trigger when pathobj is a ToolController.
            if command == "M03" or command == "M3":
                if hasattr(pathobj, "Tool") and pathobj.Tool.ShapeName.lower() == "tap":
                    tapSpeed = int(pathobj.SpindleSpeed)
                    continue

            # Convert drill cycles to tap cycles if tool is a tap.
            # This only trigger when pathobj is a Operation.
            if command == "G81" or command == "G83":
                if (
                    hasattr(pathobj, "ToolController")
                    and pathobj.ToolController.Tool.ShapeName.lower() == "tap"
                ):
                    command = "G84"
                    out += linenumber() + "G95\n"
                    paramstring = ""
                    for param in ["X", "Y"]:
                        if param in c.Parameters:
                            if (
                                (not OUTPUT_DOUBLES)
                                and (param in currLocation)
                                and (currLocation[param] == c.Parameters[param])
                            ):
                                continue
                            else:
                                pos = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)
                                paramstring += (
                                    " "
                                    + param
                                    + format(
                                        float(pos.getValueAs(UNIT_FORMAT)),
                                        precision_string,
                                    )
                                )
                    if paramstring != "":
                        out += linenumber() + "G00" + paramstring + "\n"

                    if "S" in c.Parameters:
                        tapSpeed = int(c.Parameters["S"])
                    out += "M29 S" + str(tapSpeed) + "\n"

                    for param in ["Z", "R"]:
                        if param in c.Parameters:
                            if (
                                (not OUTPUT_DOUBLES)
                                and (param in currLocation)
                                and (currLocation[param] == c.Parameters[param])
                            ):
                                continue
                            else:
                                pos = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)
                                paramstring += (
                                    " "
                                    + param
                                    + format(
                                        float(pos.getValueAs(UNIT_FORMAT)),
                                        precision_string,
                                    )
                                )
                    # in this mode, F is the distance per revolution of the thread (pitch)
                    # P is the dwell time in seconds at the bottom of the thread
                    # Q is the peck depth of the threading operation
                    for param in ["F", "P", "Q"]:
                        if param in c.Parameters:
                            value = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)
                            paramstring += (
                                " "
                                + param
                                + format(
                                    float(value.getValueAs(UNIT_FORMAT)),
                                    precision_string,
                                )
                            )

                    out += linenumber() + "G84" + paramstring + "\n"
                    out += linenumber() + "G80\n"
                    out += linenumber() + "G94\n"
                    continue

            outstring.append(command)

            # if modal: suppress the command if it is the same as the last one
            if MODAL is True:
                if command == lastcommand:
                    outstring.pop(0)

            # suppress a G80 between two identical command
            if command == "G80" and lastcommand == nextcommand:
                continue

            if c.Name.startswith("(") and not OUTPUT_COMMENTS:  # command is a comment
                continue

            # Now add the remaining parameters in order
            for param in params:
                if param in c.Parameters:
                    if param == "F" and (
                        currLocation[param] != c.Parameters[param] or OUTPUT_DOUBLES
                    ):
                        if c.Name not in [
                            "G0",
                            "G00",
                        ]:  # fanuc doesn't use rapid speeds
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
                            and currLocation[param] == c.Parameters[param]
                            and (
                                command not in DRILL_OPERATION
                                or (command in DRILL_OPERATION and param not in DRILL_PARAM_REQ)
                            )
                        ):
                            continue
                        else:
                            pos = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)
                            outstring.append(
                                param + format(float(pos.getValueAs(UNIT_FORMAT)), precision_string)
                            )

            if adaptiveOp and c.Name in ["G0", "G00"]:
                if opHorizRapid and opVertRapid:
                    if "Z" not in c.Parameters:
                        outstring.append(
                            "F"
                            + format(
                                float(opHorizRapid.getValueAs(UNIT_SPEED_FORMAT)),
                                precision_string,
                            )
                        )
                    else:
                        outstring.append(
                            "F"
                            + format(
                                float(opVertRapid.getValueAs(UNIT_SPEED_FORMAT)),
                                precision_string,
                            )
                        )

            # store the latest command
            lastcommand = command
            currLocation.update(c.Parameters)

            # Check for Tool Change:
            if command == "M6":
                # stop the spindle
                out += linenumber() + "M05\n"
                for line in TOOL_CHANGE.splitlines(True):
                    out += linenumber() + line

                # add height offset
                if USE_TLO:
                    outsuffix.append("G91 G0 G43 G54 Z-[#[2000+#4120]] H#4120")
                    outsuffix.append("G90")

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
                out += COMMAND_SPACE.join(outstring).upper()
                out = out.strip() + "\n"
            if len(outsuffix) >= 1:
                for line in outsuffix:
                    out += linenumber() + line + "\n"

        return out


# print(__name__ + " gcode postprocessor loaded.")
