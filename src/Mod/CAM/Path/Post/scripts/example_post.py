# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

import datetime

TOOLTIP = """
This is an example postprocessor file for the Path workbench. It is used
to save a list of FreeCAD Path objects to a file.

Read the Path Workbench documentation to know how to convert Path objects
to GCode.
"""

now = datetime.datetime.now()

# to distinguish python built-in open function from the one declared below
if open.__module__ in ["__builtin__", "io"]:
    pythonopen = open


def export(objectslist, filename, argstring):
    "called when freecad exports a list of objects"
    if len(objectslist) > 1:
        print("This script is unable to write more than one Path object")
        return
    obj = objectslist[0]
    if not hasattr(obj, "Path"):
        print("the given object is not a path")
    gcode = obj.Path.toGCode()
    gcode = parse(gcode)
    gfile = pythonopen(filename, "w")
    gfile.write(gcode)
    gfile.close()


def parse(inputstring):
    "parse(inputstring): returns a parsed output string"
    print("postprocessing...")

    output = ""

    # write some stuff first
    output += "N10 ;time:" + str(now) + "\n"
    output += "N20 G17 G20 G80 G40 G90\n"
    output += "N30 (Exported by FreeCAD)\n"

    linenr = 100
    lastcommand = None
    # treat the input line by line
    lines = inputstring.split("\n")
    for line in lines:
        # split the G/M command from the arguments
        if " " in line:
            command, args = line.split(" ", 1)
        else:
            # no space found, which means there are no arguments
            command = line
            args = ""
        # add a line number
        output += "N" + str(linenr) + " "
        # only print the command if it is not the same as the last one
        if command != lastcommand:
            output += command + " "
        output += args + "\n"
        # increment the line number
        linenr += 10
        # store the latest command
        lastcommand = command

    # write some more stuff at the end
    output += "N" + str(linenr) + " M05\n"
    output += "N" + str(linenr + 10) + " M25\n"
    output += "N" + str(linenr + 20) + " G00 X-1.0 Y1.0\n"
    output += "N" + str(linenr + 30) + " G17 G80 G40 G90\n"
    output += "N" + str(linenr + 40) + " M99\n"

    print("done postprocessing.")
    return output


# print(__name__ + " gcode postprocessor loaded.")
