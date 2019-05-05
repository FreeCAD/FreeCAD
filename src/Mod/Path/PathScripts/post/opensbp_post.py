from __future__ import print_function
import datetime
from PathScripts import PostUtils

# ***************************************************************************
# *   (c) sliptonic (shopinthewoods@gmail.com) 2014                         *
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


TOOLTIP='''
This is an postprocessor file for the Path workbench. It will output path data
in a format suitable for OpenSBP controllers like shopbot.  This postprocessor,
once placed in the appropriate PathScripts folder, can be used directly from
inside FreeCAD, via the GUI importer or via python scripts with:

import Path
Path.write(object,"/path/to/file.ncc","post_opensbp")
'''

'''
DONE:
    uses native commands
    handles feed and jog moves
    handles XY, Z, and XYZ feed speeds
    handles arcs
    support for inch output
ToDo
    comments may not format correctly
    drilling.  Haven't looked at it.
    many other things

'''

TOOLTIP_ARGS='''
Arguments for opensbp:
    --comments          ... insert comments - mostly for debugging
    --inches            ... convert output to inches
    --no-header         ... suppress header output
    --no-show-editor    ... don't show editor, just save result
'''

now = datetime.datetime.now()

OUTPUT_COMMENTS = False
OUTPUT_HEADER = True
SHOW_EDITOR = True
COMMAND_SPACE = ","

# Preamble text will appear at the beginning of the GCODE output file.
PREAMBLE = ''''''
# Postamble text will appear following the last operation.
POSTAMBLE = ''''''

# Pre operation text will be inserted before every operation
PRE_OPERATION = ''''''

# Post operation text will be inserted after every operation
POST_OPERATION = ''''''

# Tool Change commands will be inserted before a tool change
TOOL_CHANGE = ''''''

# to distinguish python built-in open function from the one declared below
if open.__module__ in ['__builtin__','io']:
    pythonopen = open

CurrentState = {}

def getMetricValue(val):
    return val
def getImperialValue(val):
    return val / 25.4

GetValue = getMetricValue


def export(objectslist, filename, argstring):
    global OUTPUT_COMMENTS
    global OUTPUT_HEADER
    global SHOW_EDITOR
    global CurrentState
    global GetValue

    for arg in argstring.split():
        if arg == '--comments':
            OUTPUT_COMMENTS = True
        if arg == '--inches':
            GetValue = getImperialValue
        if arg == '--no-header':
            OUTPUT_HEADER = False
        if arg == '--no-show-editor':
            SHOW_EDITOR = False


    for obj in objectslist:
        if not hasattr(obj, "Path"):
            s = "the object " + obj.Name
            s += " is not a path. Please select only path and Compounds."
            print(s)
            return

    CurrentState = {
        'X': 0, 'Y': 0, 'Z': 0, 'F': 0, 'S': 0,
        'JSXY': 0, 'JSZ': 0, 'MSXY': 0, 'MSZ': 0
    }
    print("postprocessing...")
    gcode = ""

    # write header
    if OUTPUT_HEADER:
        gcode += linenumber() + "'Exported by FreeCAD\n"
        gcode += linenumber() + "'Post Processor: " + __name__ + "\n"
        gcode += linenumber() + "'Output Time:" + str(now) + "\n"

    # Write the preamble
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "'(begin preamble)\n"
    for line in PREAMBLE.splitlines(True):
        gcode += linenumber() + line

    for obj in objectslist:

        # do the pre_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + "'(begin operation: " + obj.Label + ")\n"
        for line in PRE_OPERATION.splitlines(True):
            gcode += linenumber() + line

        gcode += parse(obj)

        # do the post_op
        if OUTPUT_COMMENTS:
            gcode += linenumber() + "'(finish operation: " + obj.Label + ")\n"
        for line in POST_OPERATION.splitlines(True):
            gcode += linenumber() + line

    # do the post_amble
    if OUTPUT_COMMENTS:
        gcode += "'(begin postamble)\n"
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

    # Write the output
    gfile = pythonopen(filename, "w")
    gfile.write(final)
    gfile.close()


def move(command):
    global CurrentState

    txt = ""

    # if 'F' in command.Parameters:
    #     txt += feedrate(command)

    axis = ""
    for p in ['X', 'Y', 'Z']:
        if p in command.Parameters:
            if command.Parameters[p] != CurrentState[p]:
                axis += p

    if 'F' in command.Parameters:
        speed = command.Parameters['F']
        if command.Name in ['G1', 'G01']:  # move
            movetype = "MS"
        else:  # jog
            movetype = "JS"
        zspeed = ""
        xyspeed = ""
        if 'Z' in axis:
            speedKey = "{}Z".format(movetype)
            speedVal = GetValue(speed)
            if CurrentState[speedKey] != speedVal:
                CurrentState[speedKey] = speedVal
                zspeed = "{:f}".format(speedVal)
        if ('X' in axis) or ('Y' in axis):
            speedKey = "{}XY".format(movetype)
            speedVal = GetValue(speed)
            if CurrentState[speedKey] != speedVal:
                CurrentState[speedKey] = speedVal
                xyspeed = "{:f}".format(speedVal)
        if zspeed or xyspeed:
            txt += "{},{},{}\n".format(movetype, xyspeed, zspeed)

    if command.Name in ['G0', 'G00']:
        pref = "J"
    else:
        pref = "M"

    if axis == "X":
        txt += pref + "X"
        txt += "," + format(GetValue(command.Parameters["X"]), '.4f')
        txt += "\n"
    elif axis == "Y":
        txt += pref + "Y"
        txt += "," + format(GetValue(command.Parameters["Y"]), '.4f')
        txt += "\n"
    elif axis == "Z":
        txt += pref + "Z"
        txt += "," + format(GetValue(command.Parameters["Z"]), '.4f')
        txt += "\n"
    elif axis == "XY":
        txt += pref + "2"
        txt += "," + format(GetValue(command.Parameters["X"]), '.4f')
        txt += "," + format(GetValue(command.Parameters["Y"]), '.4f')
        txt += "\n"
    elif axis == "XZ":
        txt += pref + "3"
        txt += "," + format(GetValue(command.Parameters["X"]), '.4f')
        txt += ","
        txt += "," + format(GetValue(command.Parameters["Z"]), '.4f')
        txt += "\n"
    elif axis == "XYZ":
        txt += pref + "3"
        txt += "," + format(GetValue(command.Parameters["X"]), '.4f')
        txt += "," + format(GetValue(command.Parameters["Y"]), '.4f')
        txt += "," + format(GetValue(command.Parameters["Z"]), '.4f')
        txt += "\n"
    elif axis == "YZ":
        txt += pref + "3"
        txt += ","
        txt += "," + format(GetValue(command.Parameters["Y"]), '.4f')
        txt += "," + format(GetValue(command.Parameters["Z"]), '.4f')
        txt += "\n"
    elif axis == "":
        print("warning: skipping duplicate move.")
    else:
        print(CurrentState)
        print(command)
        print("I don't know how to handle '{}' for a move.".format(axis))

    return txt


def arc(command):
    if command.Name == 'G2':  # CW
        dirstring = "1"
    else:  # G3 means CCW
        dirstring = "-1"
    txt = "CG,,"
    txt += format(GetValue(command.Parameters['X']), '.4f') + ","
    txt += format(GetValue(command.Parameters['Y']), '.4f') + ","
    txt += format(GetValue(command.Parameters['I']), '.4f') + ","
    txt += format(GetValue(command.Parameters['J']), '.4f') + ","
    txt += "T" + ","
    txt += dirstring
    txt += "\n"
    return txt


def tool_change(command):
    txt = ""
    if OUTPUT_COMMENTS:
        txt += "'a tool change happens now\n"
    for line in TOOL_CHANGE.splitlines(True):
        txt += line
    txt += "&ToolName=" + str(int(command.Parameters['T']))
    txt += "\n"
    txt += "&Tool=" + str(int(command.Parameters['T']))
    txt += "\n"
    return txt


def comment(command):
    print("a comment")
    return


def spindle(command):
    txt = ""
    if command.Name == "M3":  # CW
        pass
    else:
        pass
    txt += "TR," + str(command.Parameters['S']) + "\n"
    txt += "C6\n"
    txt += "PAUSE 2\n"
    return txt


# Supported Commands
scommands = {
    "G0": move,
    "G1": move,
    "G2": arc,
    "G3": arc,
    "M6": tool_change,
    "M3": spindle,
    "G00": move,
    "G01": move,
    "G02": arc,
    "G03": arc,
    "M06": tool_change,
    "M03": spindle,
    "message": comment
}


def parse(pathobj):
    global CurrentState

    output = ""
    params = ['X', 'Y', 'Z', 'A', 'B', 'I', 'J', 'K', 'F', 'S', 'T']
    # Above list controls the order of parameters

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        if OUTPUT_COMMENTS:
            output += linenumber() + "'(compound: " + pathobj.Label + ")\n"
        for p in pathobj.Group:
            output += parse(p)
    else:  # parsing simple path
        # groups might contain non-path things like stock.
        if not hasattr(pathobj, "Path"):
            return output
        if OUTPUT_COMMENTS:
            output += linenumber() + "'(Path: " + pathobj.Label + ")\n"
        for c in pathobj.Path.Commands:
            command = c.Name
            if command in scommands:
                output += scommands[command](c)
                if c.Parameters:
                    CurrentState.update(c.Parameters)
            elif command[0] == '(':
                output += "' " + command + "\n"
            else:
                print("I don't know what the hell the command: ",end='')
                print(command + " means.  Maybe I should support it.")
    return output


def linenumber():
    return ""


print(__name__ + " gcode postprocessor loaded.")
