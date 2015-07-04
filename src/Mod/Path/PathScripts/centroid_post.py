# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************
''' example post for Centroid CNC mill'''
import FreeCAD
import datetime
now = datetime.datetime.now()
originfile = FreeCAD.ActiveDocument.FileName
import Path, PathScripts
from PathScripts import PostUtils


#***************************************************************************
# user editable stuff here

UNITS = "G20" #old style inch units for this shop
MACHINE_NAME = "BigMill"
CORNER_MIN = {'x':-609.6, 'y':-152.4, 'z':0 } #use metric for internal units
CORNER_MAX = {'x':609.6, 'y':152.4, 'z':304.8 } #use metric for internal units

SHOW_EDITOR = True
MODAL = True
COMMENT= ';' #centroid control comment symbol

HEADER = ""
HEADER += ";Exported by FreeCAD\n"
HEADER += ";Post Processor: " + __name__ +"\n"
HEADER += ";CAM file: "+originfile+"\n"
HEADER += ";Output Time:"+str(now)+"\n"

TOOLRETURN = '''M5 M25
G49 H0\n''' #spindle off,height offset canceled,spindle retracted (M25 is a centroid command to retract spindle)

ZAXISRETURN = '''G91 G28 X0 Z0
G90\n'''

SAFETYBLOCK = 'G90 G80 G40 G49\n'

AXIS_DECIMALS = 4
FEED_DECIMALS = 1
SPINDLE_DECIMALS = 0

FOOTER = 'M99'+'\n'

# don't edit with the stuff below the next line unless you know what you're doing :)
#***************************************************************************


if open.__module__ == '__builtin__':
    pythonopen = open

def export(selection,filename):
    params = ['X','Y','Z','A','B','I','J','F','H','S','T','Q','R','L'] #Using XY plane most of the time so skipping K
    for obj in selection:
        if not hasattr(obj,"Path"):
            print "the object " + obj.Name + " is not a path. Please select only path and Compounds."
            return
    myMachine = None
    for pathobj in selection:
        if hasattr(pathobj,"Group"): #We have a compound or selection.
            for p in pathobj.Group:
                if p.Name == "Machine":
                    myMachine = p
    if myMachine is None: 
        print "No machine found in this selection"
    else:
        if myMachine.MachineUnits == "Metric":
           UNITS = "G21"
        else:
           UNITS = "G20"

    gcode =''
    gcode+= HEADER
    gcode+= SAFETYBLOCK
    gcode+= UNITS+'\n'

    lastcommand = None
    gcode+= COMMENT+ selection[0].Description +'\n'

    gobjects = []
    for g in selection[0].Group:
        if g.Name <>'Machine': #filtering out gcode home position from Machine object
            gobjects.append(g)

    for obj in gobjects:
        for c in obj.Path.Commands:
            outstring = []
            command = c.Name

            if command[0]=='(':
                command = PostUtils.fcoms(command, COMMENT)

            outstring.append(command)
            if MODAL == True:
                if command == lastcommand:
                    outstring.pop(0) 
            if c.Parameters >= 1:
                for param in params:
                    if param in c.Parameters:
                        if param == 'F': 
                            outstring.append(param + PostUtils.fmt(c.Parameters['F'], FEED_DECIMALS,UNITS))
                        elif param == 'H':
                            outstring.append(param + str(int(c.Parameters['H'])))
                        elif param == 'S':
                            outstring.append(param + PostUtils.fmt(c.Parameters['S'], SPINDLE_DECIMALS,'G21')) #rpm is unitless-therefore I had to 'fake it out' by using metric units which don't get converted from entered value
                        elif param == 'T':
                            outstring.append(param + str(int(c.Parameters['T'])))
                        else:
                            outstring.append(param + PostUtils.fmt(c.Parameters[param],AXIS_DECIMALS,UNITS))
            outstr = str(outstring)
            outstr =outstr.replace('[','')
            outstr =outstr.replace(']','')
            outstr =outstr.replace("'",'')
            outstr =outstr.replace(",",'')
            gcode+= outstr + '\n'
            lastcommand = c.Name
    gcode+= TOOLRETURN
    gcode+= SAFETYBLOCK
    gcode+= FOOTER
    if SHOW_EDITOR:
        PostUtils.editor(gcode)
    gfile = pythonopen(filename,"wb")
    gfile.write(gcode)
    gfile.close()

