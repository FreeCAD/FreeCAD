# ***************************************************************************
# *   (c) Yorik van Havre (yorik@uncreated.net) 2014                        *
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


'''
This is an example preprocessor file for the Path workbench. Its aim is to
open a gcode file, parse its contents, and create the appropriate objects
in FreeCAD.

Read the Path Workbench documentation to know how to create Path objects
from GCode.
'''

import os
import Path
import FreeCAD
import PathScripts.PathUtils
import PathScripts.PathLog as PathLog
import re

# LEVEL = PathLog.Level.DEBUG
LEVEL = PathLog.Level.INFO
PathLog.setLevel(LEVEL, PathLog.thisModule())

if LEVEL == PathLog.Level.DEBUG:
    PathLog.trackModule(PathLog.thisModule())


# to distinguish python built-in open function from the one declared below
if open.__module__ in ['__builtin__', 'io']:
    pythonopen = open


def open(filename):
    "called when freecad opens a file."
    PathLog.track(filename)
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    insert(filename, doc.Name)


def insert(filename, docname):
    "called when freecad imports a file"
    PathLog.track(filename)
    gfile = pythonopen(filename)
    gcode = gfile.read()
    gfile.close()
    # split on tool changes
    paths = re.split('(?=[mM]+\s?0?6)', gcode)
    # if there are any tool changes combine the preamble with the default tool 
    if len(paths) > 1:
        paths = ["\n".join(paths[0:2])] +  paths[2:]
    for path in paths:
        gcode = parse(path)
        doc = FreeCAD.getDocument(docname)
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Custom")
        PathScripts.PathCustom.ObjectCustom(obj)
        obj.ViewObject.Proxy = 0
        obj.Gcode = gcode
        PathScripts.PathUtils.addToJob(obj)
        obj.ToolController = PathScripts.PathUtils.findToolController(obj)
    FreeCAD.ActiveDocument.recompute()


def parse(inputstring):
    "parse(inputstring): returns a parsed output string"
    print("preprocessing...")
    PathLog.track(inputstring)
    # split the input by line
    lines = inputstring.split("\n")
    output = [] #""
    lastcommand = None 

    for lin in lines:
        # remove any leftover trailing and preceding spaces
        lin = lin.strip()
        if not lin:
            # discard empty lines
            continue
        if lin[0].upper() in ["N"]:
            # remove line numbers
            lin = lin.split(" ", 1)
            if len(lin) >= 1:
                lin = lin[1].strip()
            else:
                continue

        if lin[0] in ["(", "%", "#", ";"]:
            # discard comment and other non strictly gcode lines
            continue
        if lin[0].upper() in ["G", "M"]:
            # found a G or M command: we store it
            #output += lin + "\n"
            output.append(lin) # + "\n"
            last = lin[0].upper()
            for c in lin[1:]:
                if not c.isdigit():
                    break
                else:
                    last += c
            lastcommand = last
        elif lastcommand:
            # no G or M command: we repeat the last one
            output.append(lastcommand + " " + lin) # + "\n"

    print("done preprocessing.")
    return output

print(__name__ + " gcode preprocessor loaded.")
