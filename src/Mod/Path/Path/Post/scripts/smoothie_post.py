# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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


import argparse
import datetime
import Path.Post.Utils as PostUtils
import PathScripts.PathUtils as PathUtils
import FreeCAD
from FreeCAD import Units
import shlex

TOOLTIP = """
This is a postprocessor file for the Path workbench. It is used to
take a pseudo-G-code fragment outputted by a Path object, and output
real G-code suitable for a smoothieboard. This postprocessor, once placed
in the appropriate PathScripts folder, can be used directly from inside
FreeCAD, via the GUI importer or via python scripts with:

import smoothie_post
smoothie_post.export(object,"/path/to/file.ncc","")
"""

now = datetime.datetime.now()

parser = argparse.ArgumentParser(prog="linuxcnc", add_help=False)
parser.add_argument("--header", action="store_true", help="output headers (default)")
parser.add_argument("--no-header", action="store_true", help="suppress header output")
parser.add_argument("--comments", action="store_true", help="output comment (default)")
parser.add_argument(
    "--no-comments", action="store_true", help="suppress comment output"
)
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
    "--precision", default="4", help="number of digits of precision, default=4"
)
parser.add_argument(
    "--preamble",
    help='set commands to be issued before the first command, default="G17\nG90"',
)
parser.add_argument(
    "--postamble",
    help='set commands to be issued after the last command, default="M05\nG17 G90\nM2"',
)
parser.add_argument("--IP_ADDR", help="IP Address for machine target machine")
parser.add_argument(
    "--verbose",
    action="store_true",
    help='verbose output for debugging, default="False"',
)
parser.add_argument(
    "--inches", action="store_true", help="Convert output for US imperial mode (G20)"
)

TOOLTIP_ARGS = parser.format_help()

# These globals set common customization preferences
OUTPUT_COMMENTS = True
OUTPUT_HEADER = True
OUTPUT_LINE_NUMBERS = False
IP_ADDR = None
VERBOSE = False

SPINDLE_SPEED = 0.0

if FreeCAD.GuiUp:
    SHOW_EDITOR = True
else:
    SHOW_EDITOR = False
MODAL = False  # if true commands are suppressed if the same as previous line.
COMMAND_SPACE = " "
LINENR = 100  # line number starting value

# These globals will be reflected in the Machine configuration of the project
UNITS = "G21"  # G21 for metric, G20 for us standard
UNIT_SPEED_FORMAT = "mm/min"
UNIT_FORMAT = "mm"

MACHINE_NAME = "SmoothieBoard"
CORNER_MIN = {"x": 0, "y": 0, "z": 0}
CORNER_MAX = {"x": 500, "y": 300, "z": 300}

# Preamble text will appear at the beginning of the GCODE output file.
PREAMBLE = """G17 G90
"""

# Postamble text will appear following the last operation.
POSTAMBLE = """M05
G17 G90
M2
"""


# Pre operation text will be inserted before every operation
PRE_OPERATION = """"""

# Post operation text will be inserted after every operation
POST_OPERATION = """"""

# Tool Change commands will be inserted before a tool change
TOOL_CHANGE = """"""

# Number of digits after the decimal point
PRECISION = 5

# to distinguish python built-in open function from the one declared below
if open.__module__ in ["__builtin__", "io"]:
    pythonopen = open


def processArguments(argstring):
    global OUTPUT_HEADER
    global OUTPUT_COMMENTS
    global OUTPUT_LINE_NUMBERS
    global SHOW_EDITOR
    global IP_ADDR
    global VERBOSE
    global PRECISION
    global PREAMBLE
    global POSTAMBLE
    global UNITS
    global UNIT_SPEED_FORMAT
    global UNIT_FORMAT

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

        IP_ADDR = args.IP_ADDR
        VERBOSE = args.verbose

    except Exception:
        return False

    return True


def export(objectslist, filename, argstring):
    processArguments(argstring)
    global UNITS
    for obj in objectslist:
        if not hasattr(obj, "Path"):
            FreeCAD.Console.PrintError(
                "the object "
                + obj.Name
                + " is not a path. Please select only path and Compounds.\n"
            )
            return

    FreeCAD.Console.PrintMessage("postprocessing...\n")
    gcode = ""

    # Find the machine.
    # The user my have overridden post processor defaults in the GUI.  Make
    # sure we're using the current values in the Machine Def.
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
        FreeCAD.Console.PrintWarning("No machine found in this selection\n")

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

    if SHOW_EDITOR:
        dia = PostUtils.GCodeEditorDialog()
        dia.editor.setText(gcode)
        result = dia.exec_()
        if result:
            final = dia.editor.toPlainText()
        else:
            final = gcode
    else:
        final = gcode

    if IP_ADDR is not None:
        sendToSmoothie(IP_ADDR, final, filename)
    else:

        if not filename == "-":
            gfile = pythonopen(filename, "w")
            gfile.write(final)
            gfile.close()

    FreeCAD.Console.PrintMessage("done postprocessing.\n")
    return final


def sendToSmoothie(ip, GCODE, fname):
    import sys
    import socket
    import os

    fname = os.path.basename(fname)
    FreeCAD.Console.PrintMessage("sending to smoothie: {}\n".format(fname))

    f = GCODE.rstrip()
    filesize = len(f)
    # make connection to sftp server
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(4.0)
    s.connect((ip, 115))
    tn = s.makefile(mode="rw")

    # read startup prompt
    ln = tn.readline()
    if not ln.startswith("+"):
        FreeCAD.Console.PrintMessage("Failed to connect with sftp: {}\n".format(ln))
        sys.exit()

    if VERBOSE:
        print("RSP: " + ln.strip())

    # Issue initial store command
    tn.write("STOR OLD /sd/" + fname + "\n")
    tn.flush()

    ln = tn.readline()
    if not ln.startswith("+"):
        FreeCAD.Console.PrintError("Failed to create file: {}\n".format(ln))
        sys.exit()

    if VERBOSE:
        print("RSP: " + ln.strip())

    # send size of file
    tn.write("SIZE " + str(filesize) + "\n")
    tn.flush()

    ln = tn.readline()
    if not ln.startswith("+"):
        FreeCAD.Console.PrintError("Failed: {}\n".format(ln))
        sys.exit()

    if VERBOSE:
        print("RSP: " + ln.strip())

    cnt = 0
    # now send file
    for line in f.splitlines(1):
        tn.write(line)
        if VERBOSE:
            cnt += len(line)
            print("SND: " + line.strip())
            print(str(cnt) + "/" + str(filesize) + "\r", end="")

    tn.flush()

    ln = tn.readline()
    if not ln.startswith("+"):
        FreeCAD.Console.PrintError("Failed to save file: {}\n".format(ln))
        sys.exit()

    if VERBOSE:
        print("RSP: " + ln.strip())

    # exit
    tn.write("DONE\n")
    tn.flush()
    tn.close()

    FreeCAD.Console.PrintMessage("Upload complete\n")


def linenumber():
    global LINENR
    if OUTPUT_LINE_NUMBERS is True:
        LINENR += 10
        return "N" + str(LINENR) + " "
    return ""


def parse(pathobj):
    global SPINDLE_SPEED

    out = ""
    lastcommand = None
    precision_string = "." + str(PRECISION) + "f"

    # params = ['X','Y','Z','A','B','I','J','K','F','S'] #This list control
    # the order of parameters
    # linuxcnc doesn't want K properties on XY plane  Arcs need work.
    params = ["X", "Y", "Z", "A", "B", "I", "J", "F", "S", "T", "Q", "R", "L"]

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

        for c in PathUtils.getPathWithPlacement(pathobj).Commands:
            outstring = []
            command = c.Name
            outstring.append(command)
            # if modal: only print the command if it is not the same as the
            # last one
            if MODAL is True:
                if command == lastcommand:
                    outstring.pop(0)

            # Now add the remaining parameters in order
            for param in params:
                if param in c.Parameters:
                    if param == "F":
                        if c.Name not in [
                            "G0",
                            "G00",
                        ]:  # linuxcnc doesn't use rapid speeds
                            speed = Units.Quantity(
                                c.Parameters["F"], FreeCAD.Units.Velocity
                            )
                            outstring.append(
                                param
                                + format(
                                    float(speed.getValueAs(UNIT_SPEED_FORMAT)),
                                    precision_string,
                                )
                            )
                    elif param == "T":
                        outstring.append(param + str(c.Parameters["T"]))
                    elif param == "S":
                        outstring.append(param + str(c.Parameters["S"]))
                        SPINDLE_SPEED = c.Parameters["S"]
                    else:
                        pos = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)
                        outstring.append(
                            param
                            + format(
                                float(pos.getValueAs(UNIT_FORMAT)), precision_string
                            )
                        )
            if command in ["G1", "G01", "G2", "G02", "G3", "G03"]:
                outstring.append("S" + str(SPINDLE_SPEED))

            # store the latest command
            lastcommand = command

            # Check for Tool Change:
            if command == "M6":
                # if OUTPUT_COMMENTS:
                #     out += linenumber() + "(begin toolchange)\n"
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
