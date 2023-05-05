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
# *   This file has been modified from Sliptonic original Linux CNC post    *
# *   for use with a Dynapath 20 controller all changes and Modifications   *
# *   (c) Linden (Linden@aktfast.net) 2016                                  *
# *                                                                         *
# *   Additional changes 2020 adding arguments to control units, precision  *
# *   editor, header, etc. Now uses FreeCAD unit system to correctly        *
# *   set Feed rate - sliptonic                                             *
# *                                                                         *
# *                                                                         *
# ***************************************************************************


import FreeCAD
from FreeCAD import Units
import Path.Post.Utils as PostUtils
import PathScripts.PathUtils as PathUtils
import argparse
import datetime
import shlex

TOOLTIP = """
This is a postprocessor file for the Path workbench. It is used to
take a pseudo-G-code fragment outputted by a Path object, and output
real G-code suitable for a Tree Journyman 325 3 axis mill with Dynapath 20
controller in MM. This is a work in progress and very few of the functions
available on the Dynapath have been implemented at this time.
This postprocessor, once placed in the appropriate PathScripts folder,
can be used directly from inside FreeCAD, via the GUI importer or via python
scripts with:

Done
Coordinate Decimal limited to 3 places
Feed limited to hole number no decimal
Speed Limited to hole number no decimal
Machine Travel Limits Set to approximate values
Line numbers start at one and incremental by 1
Set preamble
Set postamble

To Do
Change G20 and G21 to G70 and G71 for metric or imperial units
Convert arcs to absolute
Strip comments and white spaces
Add file name in brackets limited to 8 alpha numeric no spaces all caps as
first line in file
Change Q to K For depth of peck on G83
Fix tool change
Limit comments length and characters to Uppercase, alpha numeric and
spaces add / prior to comments

import dynapath_post
dynapath_post.export(object,"/path/to/file.ncc","")
"""

parser = argparse.ArgumentParser(prog="dynapath_post", add_help=False)
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
    "--inches", action="store_true", help="Convert output for US imperial mode (G20)"
)


TOOLTIP_ARGS = parser.format_help()

now = datetime.datetime.now()

# These globals set common customization preferences
OUTPUT_COMMENTS = True
OUTPUT_HEADER = True
OUTPUT_LINE_NUMBERS = False
SHOW_EDITOR = True
MODAL = False  # if true commands are suppressed if the same as previous line.
COMMAND_SPACE = " "
LINENR = 1  # line number starting value

UNIT_SPEED_FORMAT = "mm/min"
UNIT_FORMAT = "mm"

# These globals will be reflected in the Machine configuration of the project
UNITS = "G21"  # G21 for metric, G20 for us standard
MACHINE_NAME = "Tree MM"
CORNER_MIN = {"x": -340, "y": 0, "z": 0}
CORNER_MAX = {"x": 340, "y": -355, "z": -150}

# Preamble text will appear at the beginning of the GCODE output file.
PREAMBLE = """G17
G90
;G90.1 ;needed for simulation only
G80
G40
"""

# Postamble text will appear following the last operation.
POSTAMBLE = """M09
M05
G80
G40
G17
G90
M30
"""


# Pre operation text will be inserted before every operation
PRE_OPERATION = """"""

# Post operation text will be inserted after every operation
POST_OPERATION = """"""

# Tool Change commands will be inserted before a tool change
TOOL_CHANGE = """"""


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
    global USE_TLO
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
        print("Show editor = %d" % SHOW_EDITOR)
        PRECISION = args.precision
        if args.preamble is not None:
            PREAMBLE = args.preamble
        if args.postamble is not None:
            POSTAMBLE = args.postamble
        if args.inches:
            UNITS = "G20"
            UNIT_SPEED_FORMAT = "in/min"
            UNIT_FORMAT = "in"
            PRECISION = 4

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
                "the object "
                + obj.Name
                + " is not a path. Please select only path and Compounds."
            )
            return

    print("postprocessing...")
    gcode = ""

    # Find the machine.
    # The user my have overridden post processor defaults in the GUI.  Make sure we're using the current values in the Machine Def.
    myMachine = None
    for pathobj in objectslist:
        if hasattr(pathobj, "MachineName"):
            myMachine = pathobj.MachineName
        if hasattr(pathobj, "MachineUnits"):
            if pathobj.MachineUnits == "Metric":
                UNITS = "G21"
            else:
                UNITS = "G20"
    if myMachine is None:
        print("No machine found in this selection")

    # write header
    if OUTPUT_HEADER:
        gcode += linenumber() + "(Exported by FreeCAD)\n"
        gcode += linenumber() + "(Post Processor: " + __name__ + ")\n"
        gcode += linenumber() + "(Output Time:" + str(now) + ")\n"

    # Write the preamble
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(begin preamble)\n"
    for line in PREAMBLE.splitlines(True):
        gcode += linenumber() + line
    gcode += linenumber() + UNITS + "\n"

    for obj in objectslist:

        # do the pre_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + "(begin operation: " + obj.Label + ")\n"
        for line in PRE_OPERATION.splitlines(True):
            gcode += linenumber() + line

        gcode += parse(obj)

        # do the post_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + "(finish operation: " + obj.Label + ")\n"
        for line in POST_OPERATION.splitlines(True):
            gcode += linenumber() + line

    # do the post_amble

    if OUTPUT_COMMENTS:
        gcode += "(begin postamble)\n"
    for line in POSTAMBLE.splitlines(True):
        gcode += linenumber() + line

    print("show editor: {}".format(SHOW_EDITOR))
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

    gfile = pythonopen(filename, "w")
    gfile.write(final)
    gfile.close()


def linenumber():
    global LINENR
    if OUTPUT_LINE_NUMBERS is True:
        LINENR += 1
        return "N" + str(LINENR) + " "
    return ""


def parse(pathobj):
    global PRECISION
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT

    out = ""
    lastcommand = None

    precision_string = "." + str(PRECISION) + "f"

    # params = ['X','Y','Z','A','B','I','J','K','F','S'] #This list control the order of parameters
    params = ["X", "Y", "Z", "A", "B", "I", "J", "F", "S", "T", "Q", "R", "L"]

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        if OUTPUT_COMMENTS:
            out += linenumber() + "(compound: " + pathobj.Label + ")\n"
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
            if MODAL is True:
                if command == lastcommand:
                    outstring.pop(0)

            # Now add the remaining parameters in order
            for param in params:
                if param in c.Parameters:
                    if param == "F":
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
                    elif param == "S":
                        outstring.append(
                            param + format(c.Parameters[param], precision_string)
                        )
                    elif param == "T":
                        outstring.append(
                            param + format(c.Parameters["T"], precision_string)
                        )
                    else:
                        pos = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)
                        outstring.append(
                            param
                            + format(
                                float(pos.getValueAs(UNIT_FORMAT)), precision_string
                            )
                        )

            # store the latest command
            lastcommand = command

            # Check for Tool Change:
            if command == "M6":
                if OUTPUT_COMMENTS:
                    out += linenumber() + "(begin toolchange)\n"
                for line in TOOL_CHANGE.splitlines(True):
                    out += linenumber() + line

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
                out = out.strip() + "\n"

        return out


# print(__name__ + " gcode postprocessor loaded.")
