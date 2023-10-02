# ***************************************************************************
# *   Copyright (c) 2020 sliptonic <shopinghewoods@gmail.com>               *
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

import FreeCAD
import Part
import Path
import PathScripts.PathUtils as PathUtils
import datetime
import importDXF

TOOLTIP = """
This is a postprocessor file for the Path workbench. It is used to
take a pseudo-G-code fragment outputted by a Path object, and output
a dxf file.
Operations are output to layers.
vertical moves are ignore
All path moves are flattened to z=0

Does NOT remove redundant lines.  If you have multiple step-downs in your
operation, you'll get multiple redundant lines in your dxf.

import dxf_post
"""

TOOLTIP_ARGS = """
Arguments for dxf:
"""
now = datetime.datetime.now()

# # These globals set common customization preferences
OUTPUT_HEADER = True

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


# to distinguish python built-in open function from the one declared below
if open.__module__ in ["__builtin__", "io"]:
    pythonopen = open


def processArguments(argstring):
    pass
    # global OUTPUT_HEADER


def export(objectslist, filename, argstring):
    doc = FreeCAD.ActiveDocument
    print("postprocessing...")
    layers = []
    processArguments(argstring)
    for i in objectslist:
        result = parse(i)
        if len(result) > 0:
            layername = i.Name
            grp = doc.addObject("App::DocumentObjectGroup", layername)
            for o in result:
                o.adjustRelativeLinks(grp)
                grp.addObject(o)
            layers.append(grp)

    dxfWrite(layers, filename)


def dxfWrite(objlist, filename):
    importDXF.export(objlist, filename)


def parse(pathobj):
    """accepts a Path object.  Returns a list of wires"""

    feedcommands = Path.Geom.CmdMove
    rapidcommands = Path.Geom.CmdMoveRapid

    edges = []
    objlist = []

    # Gotta start somewhere.  Assume 0,0,0
    curPoint = FreeCAD.Vector(0, 0, 0)
    for c in PathUtils.getPathWithPlacement(pathobj).Commands:
        Path.Log.debug("{} -> {}".format(curPoint, c))
        if "Z" in c.Parameters:
            newparams = c.Parameters
            newparams.pop("Z", None)
            flatcommand = Path.Command(c.Name, newparams)
            c.Parameters = newparams
        else:
            flatcommand = c

        # ignore gcode that isn't moving
        if flatcommand.Name not in feedcommands + rapidcommands:
            Path.Log.debug("non move")
            continue

        # ignore pure vertical feed and rapid
        if (
            flatcommand.Parameters.get("X", curPoint.x) == curPoint.x
            and flatcommand.Parameters.get("Y", curPoint.y) == curPoint.y
        ):
            Path.Log.debug("vertical")
            continue

        # feeding move.  Build an edge
        if flatcommand.Name in feedcommands:
            edges.append(Path.Geom.edgeForCmd(flatcommand, curPoint))
            Path.Log.debug("feeding move")

        # update the curpoint
        curPoint.x = flatcommand.Parameters.get("X", curPoint.x)
        curPoint.y = flatcommand.Parameters.get("Y", curPoint.y)

    if len(edges) > 0:
        candidates = Part.sortEdges(edges)
        for c in candidates:
            obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "Wire")
            obj.Shape = Part.Wire(c)
            objlist.append(obj)

    return objlist
