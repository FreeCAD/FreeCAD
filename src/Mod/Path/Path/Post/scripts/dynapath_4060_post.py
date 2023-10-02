# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 luvtofish <luvtofish@gmail.com>                    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENSE text file.                                 *
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
# *   This file has been modified from Sliptonic original LinuxCNC post     *
# *   for use with Dynapath Delta 40M,50M,60M controllers. All changes      *
# *   and modifications (c) luvtofish (luvtofish@gmail.com) 2022            *
# ***************************************************************************

import FreeCAD
from FreeCAD import Units
import Path
import argparse
import datetime
import shlex
import Path.Post.Utils as PostUtils
import PathScripts.PathUtils as PathUtils

TOOLTIP = """
This is a post processor file for the FreeCAD Path workbench. It is used to
take a pseudo-G-code fragment outputted by a Path object, and output
real G-code suitable for Dynapath Delta 40,50, & 60 Controls. It has been written
and tested on FreeCAD Path workbench bundled with FreeCAD v21.
This post processor, once placed in the appropriate PathScripts folder, can be
used directly from inside FreeCAD, via the GUI importer or via python scripts with:

import delta_4060_post
delta_4060_post.export(object,"/path/to/file.ncc","")
"""

parser = argparse.ArgumentParser(prog="delta_4060", add_help=False)
parser.add_argument("--no-header", action="store_true", help="suppress header output")
parser.add_argument(
    "--no-comments", action="store_true", help="suppress comment output"
)
parser.add_argument(
    "--line-numbers", action="store_true", help="prefix with line numbers"
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
    "--preamble",
    help='set commands to be issued before the first command, default="G17\nG90\nG80\nG40"',
)
parser.add_argument(
    "--postamble",
    help='set commands to be issued after the last command, default="M09\nM05\nG80\nG40\nG17\nG90\nM30"',
)
parser.add_argument(
    "--inches", action="store_true", help="Convert output for US imperial mode (G70)"
)
parser.add_argument(
    "--modal",
    action="store_true",
    help="Suppress outputting the Same G-command",
)
parser.add_argument(
    "--axis-modal",
    action="store_true",
    help="Suppress outputting identical axis position",
)

TOOLTIP_ARGS = parser.format_help()

now = datetime.datetime.now()

# These globals set common customization preferences
OUTPUT_COMMENTS = True
OUTPUT_HEADER = True
OUTPUT_LINE_NUMBERS = False
SHOW_EDITOR = True
MODAL = False  # if true commands are suppressed if the same as previous line.
OUTPUT_DOUBLES = (
    True  # if false duplicate axis values are suppressed if the same as previous line.
)
COMMAND_SPACE = ""
LINENR = 0  # Line number starting value.
DWELL_TIME = 1  # Number of seconds to allow spindle to come up to speed.
RETRACT_MODE = False
QCYCLE_RANGE = (
    "G81",
    "G82",
    "G83",
    "G84",
    "G85",
)  # Quill Cycle range for conditional to enable 2nd reference plane.
SPINDLE_SPEED = 0
# These globals will be reflected in the Machine configuration of the project
UNITS = "G71"  # G71 for metric, G70 for US standard
UNIT_SPEED_FORMAT = "mm/min"
UNIT_FORMAT = "mm"
MACHINE_NAME = "Delta 4060"
CORNER_MIN = {"x": 0, "y": 0, "z": 0}
CORNER_MAX = {"x": 660, "y": 355, "z": 152}
PRECISION = 3
ABSOLUTE_CIRCLE_CENTER = True
# Dynapath requires absolute coordinates for arcs.
# possible values:
#                  True      use absolute values for the circle center in commands G2, G3
#                  False     values for I & J are incremental from last point

# Create Map / for renaming Fixture Commands from Gxx to Exx as needed by Dynapath.
GCODE_MAP = {
    "G54": "E01",
    "G55": "E02",
    "G56": "E03",
    "G57": "E04",
    "G58": "E05",
    "G59": "E06",
}

# Preamble text will appear at the beginning of the GCODE output file.
PREAMBLE = """G17
G90
G80
G40
"""

# Postamble text will appear following the last operation.
POSTAMBLE = """M05
G80
G40
G17
G90
M30
"""
# Create following variable for use with the 2nd reference plane.
clearanceHeight = None

# to distinguish python built-in open function from the one declared below
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
    global MODAL
    global OUTPUT_DOUBLES

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
            print("Show editor = %r" % (SHOW_EDITOR))
        if args.precision is not None:
            PRECISION = args.precision
        if args.preamble is not None:
            PREAMBLE = args.preamble
        if args.postamble is not None:
            POSTAMBLE = args.postamble
        if args.inches:
            UNITS = "G70"
            UNIT_SPEED_FORMAT = "in/min"
            UNIT_FORMAT = "in"
            PRECISION = 3
        if args.modal:
            MODAL = True
            print("Command duplicates suppressed")
        if args.axis_modal:
            OUTPUT_DOUBLES = False
            print("Doubles suppressed")

    except Exception:
        return False

    return True


def export(objectslist, filename, argstring):
    if not processArguments(argstring):
        return None
    global UNITS
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT
    global clearanceHeight

    for obj in objectslist:
        if not hasattr(obj, "Path"):
            print(
                "the object "
                + obj.Name
                + " is not a path. Please select only path and Compounds."
            )
            return None

    print("postprocessing...")
    gcode = ""

    # write header
    if OUTPUT_HEADER:
        gcode += "(%s)\n" % str.upper(
            obj.Document.Label[0:8]
        )  # Added program name entry and limited to 8 chars as first line in program.
        # Insert "T" in front of comments to signify "textfield" (comment).
        gcode += linenumber() + "(T)" + "EXPORTED BY FREECAD$\n"
        gcode += linenumber() + "(T)" + str.upper("Post Processor: " + __name__) + "$\n"
        gcode += linenumber() + "(T)" + str.upper("Output Time:") + str(now) + "$\n"

    # Write the preamble
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(T)" + "BEGIN PREAMBLE$\n"
    for line in PREAMBLE.splitlines(True):
        gcode += linenumber() + line
    gcode += linenumber() + UNITS + "\n"

    for obj in objectslist:

        # Skip inactive operations
        if hasattr(obj, "Active"):
            if not obj.Active:
                continue
        if hasattr(obj, "Base") and hasattr(obj.Base, "Active"):
            if not obj.Base.Active:
                continue
        if hasattr(obj, "ClearanceHeight"):
            clearanceHeight = obj.ClearanceHeight.Value

        # Fix fixture Offset label. Needed by Dynapath.
        if obj.Label in GCODE_MAP:
            obj.Label = GCODE_MAP[obj.Label]

        # do the pre_op. Inserts "(T)" in comment to signify textfield.
        if OUTPUT_COMMENTS:
            gcode += (
                linenumber()
                + "(T)"
                + str.upper("begin operation: " + obj.Label)
                + "$\n"
            )
            gcode += (
                linenumber()
                + "(T)"
                + str.upper("machine units: ")
                + "%s/%s$\n"
                % (str.upper(UNIT_SPEED_FORMAT[0:2]), str.upper(UNIT_SPEED_FORMAT[3:6]))
            )

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
                gcode += (
                    linenumber()
                    + "(T)"
                    + str.upper("Coolant On:" + coolantMode)
                    + "$\n"
                )
        if coolantMode == "Flood":
            gcode += linenumber() + "M8" + "\n"
        if coolantMode == "Mist":
            gcode += linenumber() + "M7" + "\n"

        # process the operation gcode
        gcode += parse(obj)

        # do the post_op
        if OUTPUT_COMMENTS:
            gcode += (
                linenumber()
                + "(T)"
                + str.upper("finish operation: " + obj.Label)
                + "$\n"
            )

        # turn coolant off if required
        if not coolantMode == "None":
            if OUTPUT_COMMENTS:
                gcode += (
                    linenumber()
                    + "(T)"
                    + str.upper("Coolant Off:" + coolantMode)
                    + "$\n"
                )
            gcode += linenumber() + "M9" + "\n"

    # do the post_amble
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(T)" + "BEGIN POSTAMBLE$\n"
    for line in POSTAMBLE.splitlines(True):
        gcode += linenumber() + line
    # Following is required by Dynapath Controls to signify "EOF" when loading in to control
    # from external media. The control strips the "E" off as part of the load process.
    gcode += "E\n"

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

    print("done postprocessing.")

    if not filename == "-":
        gfile = pythonopen(filename, "w")
        gfile.write(final)
        gfile.close()

    return final


# Modified for Dynapath to Include a 4 digit field that begins with an "N".
def linenumber():
    global LINENR
    if OUTPUT_LINE_NUMBERS is True:
        LINENR += 1
        return "N" + "%04d" % (LINENR)  # Added formatting for 4 digit line number.
    return ""


def parse(pathobj):
    global PRECISION
    global MODAL
    global OUTPUT_DOUBLES
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT
    global RETRACT_MODE
    global DWELL_TIME
    global clearanceHeight

    lastX = 0
    lastY = 0
    lastZ = 0
    out = ""
    lastcommand = None
    precision_string = "." + str(PRECISION) + "f"
    currLocation = {}  # keep track for no doubles

    # the order of parameters
    # Added O and L since these are needed by Dynapath.
    # "O" is used as a 2nd reference plane in canned Cycles.
    # "L" is equivalent to "P" for dwell entries.
    params = [
        "X",
        "Y",
        "Z",
        "A",
        "B",
        "C",
        "I",
        "J",
        "K",
        "F",
        "S",
        "T",
        "Q",
        "O",
        "R",
        "L",
        "P",
    ]

    firstmove = Path.Command("G0", {"X": -1, "Y": -1, "Z": -1, "F": 0.0})
    currLocation.update(firstmove.Parameters)  # set first location parameters

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        if OUTPUT_COMMENTS:
            out += (
                linenumber() + "(T)" + str.upper("compound: " + pathobj.Label) + "$\n"
            )
        for p in pathobj.Group:
            out += parse(p)
        return out
    else:  # parsing simple path

        # groups might contain non-path things like stock.
        if not hasattr(pathobj, "Path"):
            return out

        for c in PathUtils.getPathWithPlacement(pathobj).Commands:
            outstring = []
            command = c.Name
            # Convert G54-G59 Fixture offsets to E01-E06 for Dynapath Delta Control
            if command in GCODE_MAP:
                command = GCODE_MAP[command]

            # Inserts "(T)" in front of comments to signify textfield (Dynapath)
            if command.startswith("("):
                if OUTPUT_COMMENTS:
                    command = "(T)" + str.upper(command) + "$"

            outstring.append(command)

            # if modal: suppress the command if it is the same as the last one
            if MODAL is True:
                if command == lastcommand:
                    outstring.pop(0)

            if c.Name[0] == "(" and not OUTPUT_COMMENTS:  # command is a comment
                continue

            # Now add the remaining parameters in order
            for param in params:
                if param in c.Parameters:
                    if param == "F" and (
                        currLocation[param] != c.Parameters[param] or OUTPUT_DOUBLES
                    ):
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
                    # Inserts "X0 Y0" in front of a G0 Z + clearanceHeight movement.
                    # This fixes an error thrown by Dynapath due to missing and
                    # required XYZ move after Tool change.
                    elif param == "Z" and (
                        c.Parameters["Z"] == clearanceHeight
                        and c.Parameters["Z"] != lastZ
                    ):
                        x = 0
                        y = 0
                        outstring.insert(
                            1,
                            "X"
                            + PostUtils.fmt(x, PRECISION, UNITS)
                            + "Y"
                            + PostUtils.fmt(y, PRECISION, UNITS),
                        )
                        outstring.append(
                            param + PostUtils.fmt(c.Parameters["Z"], PRECISION, UNITS)
                        )
                    elif param == "X" and (command in QCYCLE_RANGE):
                        pos = Units.Quantity(c.Parameters["X"], FreeCAD.Units.Length)
                        outstring.append(
                            param
                            + format(
                                float(pos.getValueAs(UNIT_FORMAT)), precision_string
                            )
                        )
                    elif param == "Y" and (command in QCYCLE_RANGE):
                        pos = Units.Quantity(c.Parameters["Y"], FreeCAD.Units.Length)
                        outstring.append(
                            param
                            + format(
                                float(pos.getValueAs(UNIT_FORMAT)), precision_string
                            )
                        )
                    # Remove X and Y between QCYCLE's since we already included them.
                    # This is needed to prevent Path of inserting additional XY codes between
                    # Canned cycle holes.
                    elif lastcommand in QCYCLE_RANGE and (param == "X" or "Y"):
                        outstring = []
                    elif param == "S":
                        SPINDLE_SPEED = c.Parameters["S"]
                        outstring.append(
                            param + "{:.0f}".format(c.Parameters["S"])
                        )  # Added formatting to strip trailing .000 from RPM (needed by dynapath)
                    elif param == "T":
                        outstring.append(
                            param + "{:.0f}".format(c.Parameters["T"])
                        )  # Added formatting to strip trailing .000 from Tool number (needed by dynapath)
                    elif param == "I" and (command == "G2" or command == "G3"):
                        # Convert incremental arc center to absolute in I and J
                        # Dynapath requires "absolute" arcs in (G2,G3)
                        i = c.Parameters["I"]
                        if ABSOLUTE_CIRCLE_CENTER:
                            i += lastX
                            outstring.append(param + PostUtils.fmt(i, PRECISION, UNITS))
                    elif param == "J" and (command == "G2" or command == "G3"):
                        # Convert incremental arc center to absolute in I and J
                        j = c.Parameters["J"]
                        if ABSOLUTE_CIRCLE_CENTER:
                            j += lastY
                            outstring.append(param + PostUtils.fmt(j, PRECISION, UNITS))
                    elif param == "K" and (command == "G2" or command == "G3"):
                        # Convert incremental arc center to absolute in K (Z axis arc)
                        k = c.Parameters["K"]
                        if ABSOLUTE_CIRCLE_CENTER:
                            k += lastZ
                        if command == (
                            "G18" or "G19"
                        ):  # Dynapath supports G18/G19 for Z axis arcs in Y or X.
                            outstring.append(param + PostUtils.fmt(k, PRECISION, UNITS))
                    # Converts "Q" to "K" as needed by Dynapath.
                    elif param == "Q":
                        pos = Units.Quantity(c.Parameters["Q"], FreeCAD.Units.Length)
                        outstring.append(
                            "K"
                            + format(
                                float(pos.getValueAs(UNIT_FORMAT)), precision_string
                            )
                        )
                    # Following inserts a 2nd reference plane in all canned cycles (dynapath).
                    # This provides the ability to manually go in and bump up the "O" offset in
                    # order to avoid obstacles. The "O" overrides "R", so set them both equal if you
                    # don't need the 2nd reference plane.
                    elif (param == "R") and ((command in QCYCLE_RANGE)):
                        pos = Units.Quantity(
                            pathobj.ClearanceHeight.Value, FreeCAD.Units.Length
                        )
                        outstring.insert(
                            6,
                            "O"
                            + format(
                                float(pos.getValueAs(UNIT_FORMAT)), precision_string
                            ),
                        )  # Insert "O" param for 2nd reference plane (Clearance Height)
                        pos = Units.Quantity(c.Parameters["R"], FreeCAD.Units.Length)
                        outstring.append(
                            param
                            + format(
                                float(pos.getValueAs(UNIT_FORMAT)), precision_string
                            )
                        )  # First Reference plan (Safe Height)
                    elif param == "P":
                        outstring.append(
                            "L" + format(c.Parameters[param], precision_string)
                        )  # Converts "P" to "L" for dynapath.
                    else:
                        if (
                            (not OUTPUT_DOUBLES)
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
                                    float(pos.getValueAs(UNIT_FORMAT)), precision_string
                                )
                            )
            # save the last X, Y values
            if "X" in c.Parameters:
                lastX = c.Parameters["X"]
            if "Y" in c.Parameters:
                lastY = c.Parameters["Y"]
            if "Z" in c.Parameters:
                lastZ = c.Parameters["Z"]

            # store the latest command
            lastcommand = command
            currLocation.update(c.Parameters)

            # Check for Tool Change:
            if command == "M6":
                if OUTPUT_COMMENTS:
                    out += linenumber() + "(T)" + "BEGIN TOOLCHANGE$\n"

            if command == "message":
                if OUTPUT_COMMENTS is False:
                    out = []
                else:
                    outstring.pop(0)  # remove the command
            # G98 is not used by dynapath and G99 is not used in Drilling/Boring/Tapping.
            if c.Name == "G98" or (c.Name == "G99" and pathobj.Label == "Drilling"):
                outstring = []

            # prepend a line number and append a newline
            if len(outstring) >= 1:
                if OUTPUT_LINE_NUMBERS:
                    outstring.insert(0, (linenumber()))

                # append the line to the final output
                for w in outstring:
                    out += w.strip() + COMMAND_SPACE
                out += "\n"

            # Add a DWELL after spindle start based on RPM.
            # Used by Dynapath for implementing a scalable dwell time.
            if DWELL_TIME > 0.0:
                if command in ["M3", "M03", "M4", "M04"]:
                    DWELL_TIME = (SPINDLE_SPEED / 100) / 10
                    out += linenumber() + "L%.1f\n" % DWELL_TIME

        return out
