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


'''
This is an postprocessor file for the Path workbench. It will output path data in a format suitable for OpenSBP controllers like shopbot.  This postprocessor, once placed in the appropriate PathScripts folder, can be used directly from inside FreeCAD,
via the GUI importer or via python scripts with:

import Path
Path.write(object,"/path/to/file.ncc","post_opensbp")
'''

import datetime
now = datetime.datetime.now()
from PathScripts import PostUtils

OUTPUT_COMMENTS = False
OUTPUT_HEADER = True
SHOW_EDITOR = True
COMMAND_SPACE = ","

#Preamble text will appear at the beginning of the GCODE output file.
PREAMBLE = '''
'''
#Postamble text will appear following the last operation.
POSTAMBLE = '''
'''
 
#Pre operation text will be inserted before every operation
PRE_OPERATION = '''
'''
 
#Post operation text will be inserted after every operation
POST_OPERATION = '''
'''

#Tool Change commands will be inserted before a tool change
TOOL_CHANGE = ''''A tool change is about to happen
'''


def move(commandline):
    print "processing a move"
    txt = ""
    if commandline['F'] != None : #Feed Rate has changed
        print "command contains an F"
        txt += feedrate(commandline)
        
    if commandline['command'] == 'G0': 
        txt += "J3"
    else:
        txt += "M3"

    for p in ['X','Y','Z']:
        if commandline[p] == None:
            txt += "," + format(CurrentState[p], '.4f')
        else:
            txt += "," + format(eval(commandline[p]), '.4f')
            CurrentState[p] = eval(commandline[p])
    txt += "\n"
    return txt



def feedrate(commandline):
    #figure out what kind of feed rate we're talking about jog or move
    NOCHANGE = False
    txt = ""
    setspeed = eval(commandline['F'])
    if commandline['command'] == 'G1': #move
        movetype = "MS"
    else:  #jog
        movetype = "JS"
    print "movetype: " + movetype
    
    if commandline['X'] == None:
        newX = CurrentState['X']
    else:
        newX = eval(commandline['X'])

    if commandline['Y'] == None: 
        newY = CurrentState['Y']
    else:
        newY = eval(commandline['Y'])

    if commandline['Z'] == None: 
        newZ = CurrentState['Z']
    else:
        newZ = eval(commandline['Z'])

    if newX == CurrentState['X'] and newY == CurrentState['Y']:
        # ZMove only
        AXISMOVE = "Z"

        if CurrentState[movetype+'Z'] == setspeed:
            NOCHANGE = True
    else:
        AXISMOVE = "XY"
        if CurrentState[movetype+'XY'] == setspeed:
            NOCHANGE = True
    if AXISMOVE == "XY" and newZ != CurrentState['Z']:
        AXISMOVE = "XYZ"
        if CurrentState[movetype+'XY'] == setspeed and CurrentState[movetype+'Z'] == setspeed:
            NOCHANGE = True
    print "axismove: " + AXISMOVE
    #figure out if it has actually changed.
    if NOCHANGE == True:
        txt = ""
    else: #something changed
        if AXISMOVE == "XY":
            txt += movetype + "," + format(setspeed, '.4f')
            CurrentState[movetype+'XY'] = setspeed
        elif AXISMOVE == "Z":
            txt += movetype + ",," + format(setspeed, '.4f')
            CurrentState[movetype+'Z'] = setspeed
        else: #XYZMOVE
            txt += movetype + "," + format(setspeed, '.4f') + "," + format(setspeed, '.4f') 
            print txt
            CurrentState[movetype+'XY'] = setspeed 
            CurrentState[movetype+'Z'] = setspeed
        
        txt += "\n"

    return txt


def arc(commandline):
    if commandline['command'] == 'G2': #CW
        dirstring = "1"
    else: #G3 means CCW
        dirstring = "-1"
    txt = "CG,," 
    txt += format(eval(commandline['X']), '.4f') + "," 
    txt += format(eval(commandline['Y']), '.4f') + "," 
    txt += format(eval(commandline['I']), '.4f') + ","
    txt += format(eval(commandline['J']), '.4f') + ","
    txt += "T" + ","
    txt += dirstring
    txt += "\n"
    return txt

def tool_change(commandline):
    print "tool change"
    txt = ""
    if OUTPUT_COMMENTS: txt += "'a tool change happens now\n"
    for line in TOOL_CHANGE.splitlines(True):
        txt += line
    txt += "&ToolName = " + commandline['T']
    txt += "\n"   
    txt += "&Tool=" + commandline['T']
    txt += "\n"

    return txt

def comment(commandline):
    print "a comment"

def spindle(commandline):
    txt =""
    if commandline['command'] == "M3": #CW
        pass
    else:
        pass
    txt += "TR," + commandline['S']
    return txt

#Supported Commands
scommands = {"G0": move,
    "G1": move,
    "G2": arc,
    "G3": arc,
    "M6": tool_change,
    "M3": spindle,
    "message": comment
    }

CurrentState = {'X':0, 'Y':0, 'Z':0, 'F':0, 'S':0, 'JSXY':0, 'JSZ':0, 'MSXY':0, 'MSZ':0}

def parse(inputstring):
    "parse(inputstring): returns a parsed output string"
    print "postprocessing..."
    
    output = ""
    params = ['X','Y','Z','A','B','I','J','K','F','S','T'] #This list control the order of parameters
 
    # write some stuff first
    if OUTPUT_HEADER:
        print "outputting header"
        output += "'Exported by FreeCAD\n"
        output += "'Post Processor: " + __name__ +"\n"
        output += "'Output Time:"+str(now)+"\n"

    #Write the preamble 
    if OUTPUT_COMMENTS: output += "'begin preamble\n"
    for line in PREAMBLE.splitlines(True):
        output += line

    # treat the input line by line
    lines = inputstring.splitlines(True)

    for line in lines:
        commandline = PostUtils.stringsplit(line)
        command = commandline['command']        
        try:
            print commandline
            print "command: " + command
            print command in scommands
            output += scommands[command](commandline)
        except:
            print "I don't know what the hell the command:  " + command + " means.  Maybe I should support it."

    print "finished"    
    # write some more stuff at the end
    if OUTPUT_COMMENTS: output += "'begin postamble\n" 
    for line in POSTAMBLE.splitlines(True):
        output += line

    if SHOW_EDITOR:
        dia = PostUtils.GCodeEditorDialog()
        dia.editor.setText(output)
        result = dia.exec_()
        if result:
            final = dia.editor.toPlainText()
        else:
            final = output
    else:
        final = output

    print "done postprocessing."
    return final


print __name__ + " gcode postprocessor loaded."

