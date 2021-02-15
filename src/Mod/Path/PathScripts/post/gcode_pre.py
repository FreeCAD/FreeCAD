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
# ***************************************************************************/


'''
This is an example preprocessor file for the Path workbench. Its aim is to
open a gcode file, parse its contents, and create the appropriate objects
in FreeCAD.

This preprocessor will split gcode on tool changes and create one or more
PathCustom objects in the job.  Tool Change commands themselves are not
preserved. It is up to the user to create and assign appropriate tool
controllers.

Only gcodes that are supported by Path are imported. Thus things like G43
are suppressed.

Importing gcode is inherently dangerous because context cannot be safely
assumed. The user should carefully examine the resulting gcode!

Read the Path Workbench documentation to know how to create Path objects
from GCode.
'''

import os
import FreeCAD
import PathScripts.PathUtils as PathUtils
import PathScripts.PathLog as PathLog
import re
import PathScripts.PathCustom as PathCustom
import PathScripts.PathCustomGui as PathCustomGui
import PathScripts.PathOpGui as PathOpGui

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


def matchToolController(op, toolnumber):
    """Try to match a tool controller in the job by number"""
    toolcontrollers = PathUtils.getToolControllers(op)
    for tc in toolcontrollers:
        if tc.ToolNumber == toolnumber:
            return tc
    return toolcontrollers[0]


def insert(filename, docname):
    "called when freecad imports a file"
    PathLog.track(filename)
    gfile = pythonopen(filename)
    gcode = gfile.read()
    gfile.close()

    # Regular expression to match tool changes in the format 'M6 Tn'
    p = re.compile('[mM]+?\s?0?6\s?T\d*\s')

    # split the gcode on tool changes
    paths = re.split('([mM]+?\s?0?6\s?T\d*\s)', gcode)

    # iterate the gcode sections and add customs for each
    toolnumber = 0

    for path in paths:

        # if the section is a tool change, extract the tool number
        m = p.match(path)
        if m:
            toolnumber = int(m.group().split('T')[-1])
            continue

        # Parse the gcode and throw away any empty lists
        gcode = parse(path)
        if len(gcode) == 0:
            continue

        # Create a custom and viewobject
        obj = PathCustom.Create("Custom")
        res = PathOpGui.CommandResources('Custom',
            PathCustom.Create, PathCustomGui.TaskPanelOpPage,
            'Path_Custom', 
            QtCore.QT_TRANSLATE_NOOP('Path_Custom', 'Custom'), '', ''
            )
        obj.ViewObject.Proxy = PathOpGui.ViewProvider(obj.ViewObject, res)
        obj.ViewObject.Proxy.setDeleteObjectsOnReject(False)

        # Set the gcode and try to match a tool controller
        obj.Gcode = gcode
        obj.ToolController = matchToolController(obj, toolnumber)

    FreeCAD.ActiveDocument.recompute()


def parse(inputstring):
    "parse(inputstring): returns a parsed output string"

    supported = ['G0', 'G00',
                 'G1', 'G01',
                 'G2', 'G02',
                 'G3', 'G03',
                 'G81', 'G82', 'G83',
                 'G90', 'G91']

    axis = ["X", "Y", "Z", "A", "B", "C", "U", "V", "W"]

    print("preprocessing...")
    PathLog.track(inputstring)
    # split the input by line
    lines = inputstring.splitlines()
    output = []
    lastcommand = None

    for lin in lines:
        # remove any leftover trailing and preceding spaces
        lin = lin.strip()

        # discard empty lines
        if not lin:
            continue

        # remove line numbers
        if lin[0].upper() in ["N"]:
            lin = lin.split(" ", 1)
            if len(lin) >= 1:
                lin = lin[1].strip()
            else:
                continue

        # Anything else not a G/M code or an axis move is ignored.
        if lin[0] not in ["G", "M", "X", "Y", "Z", "A", "B", "C", "U", "V", "W"]:
            continue

        # if the remaining line is supported, store it
        currcommand = lin.split()[0]

        if currcommand in supported:
            output.append(lin)
            lastcommand = currcommand

        # modal commands have no G or M but have axis moves. append those too.
        elif currcommand[0] in axis and lastcommand:
            output.append(lastcommand + " " + lin)

    print("done preprocessing.")
    return output


print(__name__ + " gcode preprocessor loaded.")
