# ***************************************************************************
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                        *
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
from __future__ import print_function
import FreeCAD
from FreeCAD import Units
import datetime
import PathScripts
import PathScripts.PostUtils as PostUtils

TOOLTIP = '''
This is a postprocessor file for the Path workbench. It is used to
take a pseudo-gcode fragment outputted by a Path object, and output
real GCode suitable for a centroid 3 axis mill. This postprocessor, once placed
in the appropriate PathScripts folder, can be used directly from inside
FreeCAD, via the GUI importer or via python scripts with:

import centroid_post
centroid_post.export(object,"/path/to/file.ncc","")
'''

TOOLTIP_ARGS = '''
Arguments for centroid:
    --header,--no-header             ... output headers (--header)
    --comments,--no-comments         ... output comments (--comments)
    --line-numbers,--no-line-numbers ... prefix with line numbers (--no-lin-numbers)
    --show-editor, --no-show-editor  ... pop up editor before writing output(--show-editor)
    --feed-precision=1               ... number of digits of precision for feed rate.  Default=1
    --axis-precision=4               ... number of digits of precision for axis moves.  Default=4
'''
now = datetime.datetime.now()

# These globals set common customization preferences
OUTPUT_COMMENTS = True
OUTPUT_HEADER = True
OUTPUT_LINE_NUMBERS = False
if FreeCAD.GuiUp:
    SHOW_EDITOR = True
else:
    SHOW_EDITOR = False
MODAL = False  # if true commands are suppressed if the same as previous line.

COMMAND_SPACE = " "
LINENR = 100  # line number starting value

# These globals will be reflected in the Machine configuration of the project
UNITS = "G20"  # G21 for metric, G20 for us standard
UNIT_FORMAT = 'mm/min'
MACHINE_NAME = "Centroid"
CORNER_MIN = {'x': -609.6, 'y': -152.4, 'z': 0}  # use metric for internal units
CORNER_MAX = {'x': 609.6, 'y': 152.4, 'z': 304.8}  # use metric for internal units
AXIS_PRECISION = 4
FEED_PRECISION = 1
SPINDLE_DECIMALS = 0

COMMENT = ";"

HEADER = '''
;Exported by FreeCAD
;Post Processor: {}
;CAM file: {}
;Output Time: {}
'''.format(__name__, FreeCAD.ActiveDocument.FileName, str(now))

# Preamble text will appear at the beginning of the GCODE output file.
PREAMBLE = '''G53 G00 G17
'''

# Postamble text will appear following the last operation.
POSTAMBLE = '''M99
'''

TOOLRETURN = '''M5 M25
G49 H0
'''  # spindle off,height offset canceled,spindle retracted (M25 is a centroid command to retract spindle)

ZAXISRETURN = '''G91 G28 X0 Z0
G90
'''

SAFETYBLOCK = '''G90 G80 G40 G49
'''

# Pre operation text will be inserted before every operation
PRE_OPERATION = ''''''

# Post operation text will be inserted after every operation
POST_OPERATION = ''''''

# Tool Change commands will be inserted before a tool change
TOOL_CHANGE = ''''''


# to distinguish python built-in open function from the one declared below
if open.__module__ in ['__builtin__', 'io']:
    pythonopen = open


def processArguments(argstring):
    # pylint: disable=global-statement
    global OUTPUT_HEADER
    global OUTPUT_COMMENTS
    global OUTPUT_LINE_NUMBERS
    global SHOW_EDITOR
    global AXIS_PRECISION
    global FEED_PRECISION

    for arg in argstring.split():
        if arg == '--header':
            OUTPUT_HEADER = True
        elif arg == '--no-header':
            OUTPUT_HEADER = False
        elif arg == '--comments':
            OUTPUT_COMMENTS = True
        elif arg == '--no-comments':
            OUTPUT_COMMENTS = False
        elif arg == '--line-numbers':
            OUTPUT_LINE_NUMBERS = True
        elif arg == '--no-line-numbers':
            OUTPUT_LINE_NUMBERS = False
        elif arg == '--show-editor':
            SHOW_EDITOR = True
        elif arg == '--no-show-editor':
            SHOW_EDITOR = False
        elif arg.split('=')[0] == '--axis-precision':
            AXIS_PRECISION = arg.split('=')[1]
        elif arg.split('=')[0] == '--feed-precision':
            FEED_PRECISION = arg.split('=')[1]


def export(objectslist, filename, argstring):
    # pylint: disable=global-statement
    processArguments(argstring)
    for i in objectslist:
        print(i.Name)
    global UNITS
    global UNIT_FORMAT

    print("postprocessing...")
    gcode = ""

    # write header
    if OUTPUT_HEADER:
        gcode += HEADER

    gcode += SAFETYBLOCK

    # Write the preamble
    if OUTPUT_COMMENTS:
        for item in objectslist:
            if isinstance(item.Proxy, PathScripts.PathToolController.ToolController):
                gcode += ";T{}={}\n".format(item.ToolNumber, item.Name)
        gcode += linenumber() + ";begin preamble\n"
    for line in PREAMBLE.splitlines(True):
        gcode += linenumber() + line

    gcode += linenumber() + UNITS + "\n"

    for obj in objectslist:
        # do the pre_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + ";begin operation\n"
        for line in PRE_OPERATION.splitlines(True):
            gcode += linenumber() + line

        gcode += parse(obj)

        # do the post_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + ";end operation: %s\n" % obj.Label
        for line in POST_OPERATION.splitlines(True):
            gcode += linenumber() + line

    # do the post_amble

    if OUTPUT_COMMENTS:
        gcode += ";begin postamble\n"
    for line in TOOLRETURN.splitlines(True):
        gcode += linenumber() + line
    for line in SAFETYBLOCK.splitlines(True):
        gcode += linenumber() + line
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

    print("done postprocessing.")

    if not filename == '-':
        gfile = pythonopen(filename, "w")
        gfile.write(final)
        gfile.close()

    return final


def linenumber():
    # pylint: disable=global-statement
    global LINENR
    if OUTPUT_LINE_NUMBERS is True:
        LINENR += 10
        return "N" + str(LINENR) + " "
    return ""


def parse(pathobj):
    out = ""
    lastcommand = None
    axis_precision_string = '.' + str(AXIS_PRECISION) + 'f'
    feed_precision_string = '.' + str(FEED_PRECISION) + 'f'
    # params = ['X','Y','Z','A','B','I','J','K','F','S'] #This list control
    # the order of parameters
    # centroid doesn't want K properties on XY plane  Arcs need work.
    params = ['X', 'Y', 'Z', 'A', 'B', 'I', 'J', 'F', 'S', 'T', 'Q', 'R', 'L', 'H']

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

        for c in pathobj.Path.Commands:
            commandlist = []  # list of elements in the command, code and params.
            command = c.Name  # command M or G code or comment string

            if command[0] == '(':
                command = PostUtils.fcoms(command, COMMENT)

            commandlist.append(command)
            # if modal: only print the command if it is not the same as the
            # last one
            if MODAL is True:
                if command == lastcommand:
                    commandlist.pop(0)

            # Now add the remaining parameters in order
            for param in params:
                if param in c.Parameters:
                    if param == 'F':
                        if c.Name not in ["G0", "G00"]:  # centroid doesn't use rapid speeds
                            speed = Units.Quantity(c.Parameters['F'], FreeCAD.Units.Velocity)
                            commandlist.append(
                                param + format(float(speed.getValueAs(UNIT_FORMAT)), feed_precision_string))
                    elif param == 'H':
                        commandlist.append(param + str(int(c.Parameters['H'])))
                    elif param == 'S':
                        commandlist.append(param + PostUtils.fmt(c.Parameters['S'], SPINDLE_DECIMALS, "G21"))
                    elif param == 'T':
                        commandlist.append(param + str(int(c.Parameters['T'])))
                    else:
                        commandlist.append(
                            param + format(c.Parameters[param], axis_precision_string))
            outstr = str(commandlist)
            outstr = outstr.replace('[', '')
            outstr = outstr.replace(']', '')
            outstr = outstr.replace("'", '')
            outstr = outstr.replace(",", '')

            # store the latest command
            lastcommand = command

            # Check for Tool Change:
            if command == 'M6':
                # if OUTPUT_COMMENTS:
                #     out += linenumber() + "(begin toolchange)\n"
                for line in TOOL_CHANGE.splitlines(True):
                    out += linenumber() + line

            # if command == "message":
            #     if OUTPUT_COMMENTS is False:
            #         out = []
            #     else:
            #         commandlist.pop(0)  # remove the command

            # prepend a line number and append a newline
            if len(commandlist) >= 1:
                if OUTPUT_LINE_NUMBERS:
                    commandlist.insert(0, (linenumber()))

                # append the line to the final output
                for w in commandlist:
                    out += w + COMMAND_SPACE
                out = out.strip() + "\n"

        return out


print(__name__ + " gcode postprocessor loaded.")
