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
#*   This file has been modified from Sliptonis original Linux CNC post    *
#*   for use with a Dynapath 20 controller all changes and Modifications   *
#*   (c) Linden (Linden@aktfast.net) 2016                                  *
#*                                                                         *
#***************************************************************************/
from __future__ import print_function

TOOLTIP='''
This is a postprocessor file for the Path workbench. It is used to
take a pseudo-gcode fragment outputted by a Path object, and output
real GCode suitable for a Tree Journyman 325 3 axis mill with Dynapath 20 controller in MM.
This is a work in progress and very few of the functions available on the Dynapath have been
implemented at this time.
This postprocessor, once placed in the appropriate PathScripts folder, can be used directly
from inside FreeCAD, via the GUI importer or via python scripts with:

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
Add file name in brackets limited to 8 alpha numeric no spaces all caps as first line in file
Change Q to K For depth of peck on G83
Fix tool change
Limit comments length and characters to Uppercase, alpha numeric and spaces add / prior to comments

import linuxcnc_post
linuxcnc_post.export(object,"/path/to/file.ncc","")
'''

import datetime
now = datetime.datetime.now()
from PathScripts import PostUtils

#These globals set common customization preferences
OUTPUT_COMMENTS = True
OUTPUT_HEADER = False
OUTPUT_LINE_NUMBERS = False
SHOW_EDITOR = True
MODAL = False #if true commands are suppressed if the same as previous line.
COMMAND_SPACE = " "
LINENR = 1 #line number starting value

#These globals will be reflected in the Machine configuration of the project
UNITS = "G21" #G21 for metric, G20 for us standard
MACHINE_NAME = "Tree MM"
CORNER_MIN = {'x':-340, 'y':0, 'z':0 }
CORNER_MAX = {'x':340, 'y':-355, 'z':-150 }

#Preamble text will appear at the beginning of the GCODE output file.
PREAMBLE = '''G17
G90
;G90.1 ;needed for simulation only
G80
G40
'''

#Postamble text will appear following the last operation.
POSTAMBLE = '''M09
M05
G80
G40
G17
G90
M30
'''


#Pre operation text will be inserted before every operation
PRE_OPERATION = ''''''

#Post operation text will be inserted after every operation
POST_OPERATION = ''''''

#Tool Change commands will be inserted before a tool change
TOOL_CHANGE = ''''''


# to distinguish python built-in open function from the one declared below
if open.__module__ in ['__builtin__','io']:
    pythonopen = open


def export(objectslist,filename,argstring):
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
        if hasattr(pathobj,"MachineName"):
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

    gfile = pythonopen(filename,"w")
    gfile.write(final)
    gfile.close()


def linenumber():
    global LINENR
    if OUTPUT_LINE_NUMBERS == True:
        LINENR += 1
        return "N" + str(LINENR) + " "
    return ""

def parse(pathobj):
    out = ""
    lastcommand = None

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
                        outstring.append(param + format(c.Parameters['F'], '.0f'))
                    elif param == 'S':
                        outstring.append(param + format(c.Parameters[param], '.0f'))
                    elif param == 'T':
                        outstring.append(param + format(c.Parameters['T'], '.0f'))
                    else:
                        outstring.append(param + format(c.Parameters[param], '.3f'))

            # store the latest command
            lastcommand = command

            # Check for Tool Change:
            if command == 'M6':
                if OUTPUT_COMMENTS: out += linenumber() + "(begin toolchange)\n"
                for line in TOOL_CHANGE.splitlines(True):
                    out += linenumber() + line

            if command == "message":
                if OUTPUT_COMMENTS == False:
                    out = []
                else:
                    outstring.pop(0) #remove the command

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

