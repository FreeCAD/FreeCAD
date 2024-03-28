# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *                 2023 Nicklas SB Karlsson <nk@nksb.eu>                   *
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
# *                                                                         *
# *  Modified 2021 by lcorley to support Crossfire(tm) plasma tables        *
# *  Made a copy on 2023-01-17 of crossfire_post.py with name               *
# *       fangling_post.py                                                  *
# *  Modified 2023-01-17- by Nicklas SB Karlsson to support FangLing plasma *
# *           cutters. Still untested on real cutter.                       *
# *                                                                         *
# ***************************************************************************/
from __future__ import print_function
import FreeCAD
from FreeCAD import Units
import Path
import argparse
import datetime
import shlex
#from PathScripts import PostUtils
import Path.Post.Utils as PostUtils
from PathScripts import PathUtils

TOOLTIP = '''
This is a postprocessor file for the Path workbench. It is used to
take a pseudo-G-code fragment outputted by a Path object, and output
real G-code suitable for FangLing plasma cutters.
This postprocessor, once placed in the appropriate PathScripts folder,
can be used directly from inside FreeCAD, via the GUI importer or
via python scripts with:

import FangLing_post
FangLing_post.export(object,"/path/to/file.ncc","")
'''

now = datetime.datetime.now()

parser = argparse.ArgumentParser(prog='FangLing', add_help=False)
parser.add_argument('--no-header', action='store_true', help='suppress header output')
parser.add_argument('--no-comments', action='store_true', help='suppress comment output')
parser.add_argument('--line-numbers', action='store_true', help='prefix with line numbers')
parser.add_argument('--no-show-editor', action='store_true', help='don\'t pop up editor before writing output')
parser.add_argument('--precision', default='3', help='number of digits of precision, default=3')
parser.add_argument('--preamble', help='set commands to be issued before the first command, default="G17\nG90"')
parser.add_argument('--postamble', help='set commands to be issued after the last command, default="M5\nG17 G90\nM2"')
parser.add_argument('--inches', action='store_true', help='Convert output for US imperial mode (G20)')
parser.add_argument('--modal', action='store_true', help='Output the Same G-command Name USE NonModal Mode')
parser.add_argument('--axis-modal', action='store_true', help='Output the Same Axis Value Mode')
parser.add_argument('--no-tlo', action='store_true', help='suppress tool length offset (G43) following tool changes')

TOOLTIP_ARGS = parser.format_help()

# These globals set common customization preferences
OUTPUT_COMMENTS = True
OUTPUT_HEADER = True
OUTPUT_LINE_NUMBERS = False
SHOW_EDITOR = True
MODAL = False  # if true commands are suppressed if the same as previous line.
USE_TLO = False # if true G43 will be output following tool changes
OUTPUT_DOUBLES = True  # if false duplicate axis values are suppressed if the same as previous line.
COMMAND_SPACE = " "
LINENR = 100  # line number starting value

# These globals will be reflected in the Machine configuration of the project
UNITS = "G21"  # G21 for metric, G20 for us standard
UNIT_SPEED_FORMAT = 'mm/min'
UNIT_FORMAT = 'mm'

MACHINE_NAME = "FangLing"
CORNER_MIN = {'x': 0, 'y': 0, 'z': 0}
CORNER_MAX = {'x': 500, 'y': 300, 'z': 300}
PRECISION = 3

# Preamble text will appear at the beginning of the GCODE output file.
#PREAMBLE = '''G17 G54 G40 G49 G80 G90'''
#Changed to match preamble produced by Fusion 360
PREAMBLE = '''G90
'''

# Postamble text will appear following the last operation.
POSTAMBLE = '''M8
G90 G40
M2
'''

# Pre operation text will be inserted before every operation
PRE_OPERATION = ''''''

# Post operation text will be inserted after every operation
POST_OPERATION = ''''''

# Tool Change commands will be inserted before a tool change
TOOL_CHANGE = ''''''

# to distinguish python built-in open function from the one declared below
if open.__module__ in ['__builtin__','io']:
    pythonopen = open


def processArguments(argstring):
    # pylint: disable=global-statement
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
            UNITS = 'G20'
            UNIT_SPEED_FORMAT = 'in/min'
            UNIT_FORMAT = 'in'
            PRECISION = 4
        if args.modal:
            MODAL = True
        if args.no_tlo:
            USE_TLO = False
        if args.axis_modal:
            print ('here')
            OUTPUT_DOUBLES = False

    except Exception: # pylint: disable=broad-except
        return False

    return True


def export(objectslist, filename, argstring):
    # pylint: disable=global-statement
    if not processArguments(argstring):
        return None
    global UNITS
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT
    global HORIZRAPID
    global VERTRAPID

    for obj in objectslist:
        if not hasattr(obj, "Path"):
            print("the object " + obj.Name + " is not a path. Please select only path and Compounds.")
            return None

    print("postprocessing...")
    gcode = ""

    # write header
    if OUTPUT_HEADER:
        gcode += linenumber() + "(Exported by FreeCAD)\n"
        gcode += linenumber() + "(Post Processor: " + __name__ + ")\n"
        gcode += linenumber() + "(Output Time:" + str(now) + ")\n"

    # Write the preamble
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(begin preamble)\n"
    for line in PREAMBLE.splitlines(False):
        gcode += linenumber() + line + "\n"
    gcode += linenumber() + UNITS + "\n"

    gcode += linenumber() + "G92 X0 Y0\n"

    for obj in objectslist:

        # Skip inactive operations
        if hasattr(obj, 'Active'):
            if not obj.Active:
                continue
        if hasattr(obj, 'Base') and hasattr(obj.Base, 'Active'):
            if not obj.Base.Active:
                continue

        # fetch machine details
        job = PathUtils.findParentJob(obj)

        if hasattr(job, "MachineUnits"):
            if job.MachineUnits == "Metric":
                UNITS = "G21"
                UNIT_FORMAT = 'mm'
                UNIT_SPEED_FORMAT = 'mm/min'
            elif job.MachineUnits == "Imperial":
                UNITS = "G20"
                UNIT_FORMAT = 'in'
                UNIT_SPEED_FORMAT = 'in/min'
            else:
                print("Machine units " + job.MachineUnits + " could not be handled.")
                return None

        if hasattr(job, "SetupSheet"):
            if hasattr(job.SetupSheet, "HorizRapid"):
                HORIZRAPID = Units.Quantity(job.SetupSheet.HorizRapid, FreeCAD.Units.Velocity)
            if hasattr(job.SetupSheet, "VertRapid"):
                VERTRAPID = Units.Quantity(job.SetupSheet.HorizRapid, FreeCAD.Units.Velocity)

        # process the operation gcode
        gcode += parse(obj)

    # do the post_amble
    if OUTPUT_COMMENTS:
        gcode += "(begin postamble)\n"
    for line in POSTAMBLE.splitlines(True):
        gcode += linenumber() + line

    if FreeCAD.GuiUp and SHOW_EDITOR:
        dia = PostUtils.GCodeEditorDialog()
        dia.editor.setText(gcode)
        if dia.exec_():
            final = dia.editor.toPlainText()
        else:
            final = gcode
    else:
        final = gcode

    print("done postprocessing.")

    writeFile(filename, final)

    return final


def linenumber():
    # pylint: disable=global-statement
    global LINENR
    if OUTPUT_LINE_NUMBERS:
        LINENR += 10
        return "N" + str(LINENR) + " "
    return ""


def parse(pathobj):
    # pylint: disable=global-statement
    global PRECISION
    global MODAL
    global OUTPUT_DOUBLES
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT

    out = ""
    #print(pathobj.Path.toGCode())
    lastcommand = None
    precision_string = '.' + str(PRECISION) + 'f' # 'g' instead of 'f' removes trailing zeroes
    currLocation = {}  # keep track for no doubles

    # the order of parameters
    # mach3_4 doesn't want K properties on XY plane  Arcs need work.
    #params = ['X', 'Y', 'Z', 'A', 'B', 'C', 'I', 'J', 'F', 'S', 'T', 'Q', 'R', 'L', 'H', 'D', 'P']
    # FangLing plasma cutters doesn't use Z values
    params = ['X', 'Y', 'A', 'B', 'C', 'I', 'J', 'F', 'S', 'T', 'Q', 'R', 'L', 'H', 'D', 'P'] # It have not been checked which parameters FangLing accept
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

        if 'Adaptive' in pathobj.Name:
            adaptiveOp = True
            if hasattr(pathobj, 'ToolController'):
                if hasattr(pathobj.ToolController, 'HorizRapid') and pathobj.ToolController.HorizRapid > 0:
                    opHorizRapid = Units.Quantity(pathobj.ToolController.HorizRapid, FreeCAD.Units.Velocity)
                else:
                    FreeCAD.Console.PrintWarning('Tool Controller Horizontal Rapid Values are unset'+ '\n')

                if hasattr(pathobj.ToolController, 'VertRapid') and pathobj.ToolController.VertRapid > 0:
                    opVertRapid = Units.Quantity(pathobj.ToolController.VertRapid, FreeCAD.Units.Velocity)
                else:
                    FreeCAD.Console.PrintWarning('Tool Controller Vertical Rapid Values are unset'+ '\n')

        #print(pathobj.Path.Commands)
        for c in pathobj.Path.Commands:

            outstring = []
            command = c.Name

            if c.Name in ["G0", "G00"] and lastcommand in ["G1", "G2", "G3"]:
                # A rapid move following a positioning move indicates the cut is complete
                # print ("end of cut")
                if OUTPUT_COMMENTS:
                    out += linenumber() + " (Torch Off)\n"
                out += linenumber() + " M08\n"
            if c.Name in ["G1", "G2", "G3"] and lastcommand in ["G0", "G00"]:
                # A positioning move following a rapid move indicates a new cut is starting
                # print ("new cut")
                if OUTPUT_COMMENTS:
                    out += linenumber() + " (Torch On)\n"
                out += linenumber() + " M07\n"
                if OUTPUT_COMMENTS:
                    out += linenumber() + " (Pierce Delay)\n"
                out += linenumber() + " G4 P1\n"

            if adaptiveOp and c.Name in ["G0", "G00"]:
                if opHorizRapid and opVertRapid:
                    command = 'G1'
                else:
                    outstring.append('(Tool Controller Rapid Values are unset)' + '\n')

            # Filter out vertical moves since FangLing plasma cutters does not support them
            if command in ["G0", "G00"] and len(c.Parameters) == 1 and 'Z' in c.Parameters:
                #print(c, c.Name, len(c.Parameters), list(c.Parameters))
                print('vertical move detected and removed')
            if command in ["G54"]:
                #print(c, c.Name, len(c.Parameters), list(c.Parameters))
                print('Set coordinate system removed')
            else:
                if command in ["G0", "G00"] and not 'X' in c.Parameters and not 'Y' in c.Parameters:
                    print('No horizontal move detected so removed')
                else:
                    outstring.append(command)

            # if modal: suppress the command if it is the same as the last one
            if MODAL is True:
                if command == lastcommand:
                    outstring.pop(0)

            if c.Name[0] == '(' and not OUTPUT_COMMENTS: # command is a comment
                continue

            # Now add the remaining parameters in order
            for param in params:
                if param in c.Parameters:
                    if param == 'F':
                        continue
                    elif param in ['T', 'H', 'D', 'S']:
                        outstring.append(param + str(int(c.Parameters[param])))
                    elif not OUTPUT_DOUBLES and param in currLocation and currLocation[param] == c.Parameters[param]:
                        continue
                    else:
                        pos = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)
                        outstring.append(
                            param + format(float(pos.getValueAs(UNIT_FORMAT)), precision_string))

            # store the latest command
            lastcommand = command
            currLocation.update(c.Parameters)

            # Check for Tool Change:
            # Tool changes not supported on FangLing plasma cutters
            if command == 'M6':
                outstring.pop(0) # remove the M6
                outstring.pop(0) # remove the tool number
                # stop the spindle
                #out += linenumber() + " M5\n"
                #for line in TOOL_CHANGE.splitlines(True):
                #out += linenumber() + line

                # add height offset
                # No TLO available in g-code
                #if USE_TLO:
                #    tool_height = '\nG43 H' + str(int(c.Parameters['T']))
                #    outstring.append(tool_height)
                print('M6 tool change removed')

            if command == "message":
                if not OUTPUT_COMMENTS:
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


def writeFile(filename, final):
    if not filename == '-':
        gfile = pythonopen(filename, "w")
        gfile.write(final)
        gfile.close()

    return
