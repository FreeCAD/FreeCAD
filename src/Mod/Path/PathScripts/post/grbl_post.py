#***************************************************************************
#*   (c) sliptonic (shopinthewoods@gmail.com) 2014                        *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/


TOOLTIP='''
Generate g-code from a Path that is compatible with the grbl controller.

import grbl_post
grbl_post.export(object,"/path/to/file.ncc")
'''

import FreeCAD
import PathScripts.PostUtils as PostUtils
import argparse
import datetime
import shlex
import traceback


now = datetime.datetime.now()

parser = argparse.ArgumentParser(prog='grbl', add_help=False)
parser.add_argument('--header', action='store_true', help='output headers (default)')
parser.add_argument('--no-header', action='store_true', help='suppress header output')
parser.add_argument('--comments', action='store_true', help='output comment (default)')
parser.add_argument('--no-comments', action='store_true', help='suppress comment output')
parser.add_argument('--line-numbers', action='store_true', help='prefix with line numbers')
parser.add_argument('--no-line-numbers', action='store_true', help='don\'t prefix with line numbers (default)')
parser.add_argument('--show-editor', action='store_true', help='pop up editor before writing output (default)')
parser.add_argument('--no-show-editor', action='store_true', help='don\'t pop up editor before writing output')
parser.add_argument('--precision', default='4', help='number of digits of precision, default=4')
parser.add_argument('--preamble', help='set commands to be issued before the first command, default="G17\nG90"')
parser.add_argument('--postamble', help='set commands to be issued after the last command, default="M05\nG17 G90\n; M2"')
parser.add_argument('--tool-change', help='0 ... suppress all tool change commands\n1 ... insert M6 for all tool changes\n2 ... insert M6 for all tool changes except the initial tool')

TOOLTIP_ARGS=parser.format_help()

#These globals set common customization preferences
OUTPUT_COMMENTS = True
OUTPUT_HEADER = True
OUTPUT_LINE_NUMBERS = False
OUTPUT_TOOL_CHANGE = False
SHOW_EDITOR = True
MODAL = False #if true commands are suppressed if the same as previous line.
COMMAND_SPACE = " "
LINENR = 100 #line number starting value

#These globals will be reflected in the Machine configuration of the project
UNITS = "G21" #G21 for metric, G20 for us standard
MACHINE_NAME = "GRBL"
CORNER_MIN = {'x':0, 'y':0, 'z':0 }
CORNER_MAX = {'x':500, 'y':300, 'z':300 }
PRECISION = 4

RAPID_MOVES = ['G0', 'G00']

#Preamble text will appear at the beginning of the GCODE output file.
PREAMBLE = '''G17 G90
'''

#Postamble text will appear following the last operation.
POSTAMBLE = '''M5
G17 G90
; M2
'''

# These commands are ignored by commenting them out
SUPPRESS_COMMANDS = [ 'G98', 'G80' ]

#Pre operation text will be inserted before every operation
PRE_OPERATION = ''''''

#Post operation text will be inserted after every operation
POST_OPERATION = ''''''

#Tool Change commands will be inserted before a tool change
TOOL_CHANGE = ''''''
SUPPRESS_TOOL_CHANGE=0


# to distinguish python built-in open function from the one declared below
if open.__module__ in ['__builtin__','io']:
    pythonopen = open


def processArguments(argstring):
    global OUTPUT_HEADER
    global OUTPUT_COMMENTS
    global OUTPUT_LINE_NUMBERS
    global OUTPUT_TOOL_CHANGE
    global SHOW_EDITOR
    global PRECISION
    global PREAMBLE
    global POSTAMBLE
    global SUPPRESS_TOOL_CHANGE

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
        if not args.preamble is None:
            PREAMBLE = args.preamble.replace('\\n', '\n')
        if not args.postamble is None:
            POSTAMBLE = args.postamble.replace('\\n', '\n')
        if not args.tool_change is None:
            OUTPUT_TOOL_CHANGE = int(args.tool_change) > 0
            SUPPRESS_TOOL_CHANGE = min(1, int(args.tool_change) - 1)
    except Exception as e:
        traceback.print_exc(e)
        return False

    return True

def export(objectslist,filename,argstring):
    if not processArguments(argstring):
        return None

    global UNITS

    for obj in objectslist:
        if not hasattr(obj,"Path"):
            print("the object " + obj.Name + " is not a path. Please select only path and Compounds.")
            return

    print("postprocessing...")
    gcode = ""

    #Find the machine.
    #The user my have overridden post processor defaults in the GUI.  Make sure we're using the current values in the Machine Def.
    myMachine = None
    for pathobj in objectslist:
        if hasattr(pathobj,"Group"): #We have a compound or project.
            for p in pathobj.Group:
                if p.Name == "Machine":
                    myMachine = p
    if myMachine is None:
        print("No machine found in this project")
    else:
        if myMachine.MachineUnits == "Metric":
           UNITS = "G21"
        else:
           UNITS = "G20"


    # write header
    if OUTPUT_HEADER:
        gcode += linenumber() + "(Exported by FreeCAD)\n"
        gcode += linenumber() + "(Post Processor: " + __name__ +")\n"
        gcode += linenumber() + "(Output Time:"+str(now)+")\n"

    #Write the preamble
    if OUTPUT_COMMENTS: gcode += linenumber() + "(begin preamble)\n"
    for line in PREAMBLE.splitlines(True):
        gcode += linenumber() + line
    gcode += linenumber() + UNITS + "\n"

    for obj in objectslist:

        #do the pre_op
        if OUTPUT_COMMENTS: gcode += linenumber() + "(begin operation: " + obj.Label + ")\n"
        for line in PRE_OPERATION.splitlines(True):
            gcode += linenumber() + line

        gcode += parse(obj)

        #do the post_op
        if OUTPUT_COMMENTS: gcode += linenumber() + "(finish operation: " + obj.Label + ")\n"
        for line in POST_OPERATION.splitlines(True):
            gcode += linenumber() + line

    #do the post_amble

    if OUTPUT_COMMENTS: gcode += "(begin postamble)\n"
    for line in POSTAMBLE.splitlines(True):
        gcode += linenumber() + line

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

    gfile = pythonopen(filename,"w")
    gfile.write(gcode)
    gfile.close()


def linenumber():
    global LINENR
    if OUTPUT_LINE_NUMBERS == True:
        LINENR += 10
        return "N" + str(LINENR) + " "
    return ""

def parse(pathobj):
    out = ""
    lastcommand = None
    precision_string = '.' + str(PRECISION) +'f'
    global SUPPRESS_TOOL_CHANGE

    #params = ['X','Y','Z','A','B','I','J','K','F','S'] #This list control the order of parameters
    params = ['X','Y','Z','A','B','I','J','F','S','T','Q','R','L'] #linuxcnc doesn't want K properties on XY plane  Arcs need work.

    if hasattr(pathobj,"Group"): #We have a compound or project.
        if OUTPUT_COMMENTS: out += linenumber() + "(compound: " + pathobj.Label + ")\n"
        for p in pathobj.Group:
            out += parse(p)
        return out
    else: #parsing simple path

        if not hasattr(pathobj,"Path"): #groups might contain non-path things like stock.
            return out

        if OUTPUT_COMMENTS: out += linenumber() + "(Path: " + pathobj.Label + ")\n"

        for c in pathobj.Path.Commands:
            outstring = []
            command = c.Name
            outstring.append(command)
            # if modal: only print the command if it is not the same as the last one
            if MODAL == True:
                if command == lastcommand:
                    outstring.pop(0)


            # Now add the remaining parameters in order
            for param in params:
                if param in c.Parameters:
                    if param == 'F':
                        if command not in RAPID_MOVES:
                            outstring.append(param + format(c.Parameters['F'] * 60, '.2f'))
                    elif param == 'T':
                        outstring.append(param + str(int(c.Parameters['T'])))
                    else:
                        outstring.append(param + format(c.Parameters[param], precision_string))

            # store the latest command
            lastcommand = command

            # Check for Tool Change:
            if command == 'M6':
                if OUTPUT_COMMENTS:
                    out += linenumber() + "(begin toolchange)\n"
                if not OUTPUT_TOOL_CHANGE or SUPPRESS_TOOL_CHANGE > 0:
                    outstring.insert(0, ";")
                    SUPPRESS_TOOL_CHANGE = SUPPRESS_TOOL_CHANGE - 1
                else:
                    for line in TOOL_CHANGE.splitlines(True):
                        out += linenumber() + line

            if command == "message":
                if OUTPUT_COMMENTS == False:
                    out = []
                else:
                    outstring.pop(0) #remove the command

            if command in SUPPRESS_COMMANDS:
                outstring.insert(0, ";")

            #prepend a line number and append a newline
            if len(outstring) >= 1:
                if OUTPUT_LINE_NUMBERS:
                    outstring.insert(0,(linenumber()))

                #append the line to the final output
                for w in outstring:
                    out += w + COMMAND_SPACE
                out = out.strip() + "\n"

        return out


print(__name__ + " gcode postprocessor loaded.")

