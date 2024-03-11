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


"""
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
"""

import FreeCAD
import Path
import PathScripts.PathUtils as PathUtils
import os
import re
from PySide.QtCore import QT_TRANSLATE_NOOP

if FreeCAD.GuiUp:
    import Path.Op.Gui.Custom as PathCustomGui

    PathCustom = PathCustomGui.PathCustom
else:
    import Path.Op.Custom as PathCustom

translate = FreeCAD.Qt.translate


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class PathNoActiveDocumentException(Exception):
    """PathNoActiveDocumentException is raised when no active document is found."""

    def __init__(self):
        super().__init__("No active document")


class PathNoJobException(Exception):
    """PathNoJobException is raised when no target Job object is available."""

    def __init__(self):
        super().__init__("No job object")


# to distinguish python built-in open function from the one declared below
if open.__module__ in ["__builtin__", "io"]:
    pythonopen = open


def open(filename):
    """called when freecad opens a file."""
    Path.Log.track(filename)
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


def _isImportEnvironmentReady():
    """_isImportEnvironmentReady(docname)...
    Helper function to verify an active document exists, and that a Job object is available
    as a receiver for the Custom operation(s) that will be created as a result of the import process."""

    # Verify active document exists
    if FreeCAD.ActiveDocument is None:
        raise PathNoActiveDocumentException()

    # Verify a Job object is available, and add one if not
    if not PathUtils.GetJobs():
        raise PathNoJobException()

    return True


def parse(inputstring):
    "parse(inputstring): returns a parsed output string"

    supported = [
        "G0",
        "G00",
        "G1",
        "G01",
        "G2",
        "G02",
        "G3",
        "G03",
        "G81",
        "G82",
        "G83",
        "G90",
        "G91",
    ]

    axis = ["X", "Y", "Z", "A", "B", "C", "U", "V", "W"]

    FreeCAD.Console.PrintMessage("preprocessing...\n")
    Path.Log.track(inputstring)
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

    FreeCAD.Console.PrintMessage("done preprocessing.\n")
    return output


def _identifygcodeByToolNumberList(filename):
    """called when freecad imports a file"""
    Path.Log.track(filename)
    gcodeByToolNumberList = []

    gfile = pythonopen(filename)
    gcode = gfile.read()
    gfile.close()

    # Regular expression to match tool changes in the format 'M6 Tn'
    p = re.compile("[mM]+?\s?0?6\s?T\d*\s")

    # split the gcode on tool changes
    paths = re.split("([mM]+?\s?0?6\s?T\d*\s)", gcode)

    # iterate the gcode sections and add customs for each
    toolnumber = 0

    for path in paths:

        # if the section is a tool change, extract the tool number
        m = p.match(path)
        if m:
            toolnumber = int(m.group().split("T")[-1])
            continue

        # Parse the gcode and throw away any empty lists
        gcode = parse(path)
        if len(gcode) > 0:
            gcodeByToolNumberList.append((gcode, toolnumber))

    return gcodeByToolNumberList


def insert(filename, docname=None):
    """called when freecad imports a file"""
    Path.Log.track(filename)

    try:
        if not _isImportEnvironmentReady():
            return
    except PathNoActiveDocumentException:
        Path.Log.error(translate("CAM_Gcode_pre", "No active document"))
        return
    except PathNoJobException:
        Path.Log.error(translate("CAM_Gcode_pre", "No job object"))
        return

    # Create a Custom operation for each gcode-toolNumber pair
    for gcode, toolNumber in _identifygcodeByToolNumberList(filename):
        obj = PathCustom.Create("Custom")

        # Set the gcode and try to match a tool controller
        obj.Gcode = gcode
        obj.ToolController = matchToolController(obj, toolNumber)
        if docname:
            obj.Label = obj.Label + "_" + docname

        if FreeCAD.GuiUp:
            # Add a view provider to a Custom operation object
            obj.ViewObject.Proxy = PathCustomGui.PathOpGui.ViewProvider(
                obj.ViewObject, PathCustomGui.Command.res
            )
            obj.ViewObject.Proxy.setDeleteObjectsOnReject(False)

    FreeCAD.ActiveDocument.recompute()


print(__name__ + " gcode preprocessor loaded.")
