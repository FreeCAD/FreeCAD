# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""A tool holder's outline as a sketch, for File > Open, Import and Export of .fcholder files.

Opening a .fcholder builds a sketch of its outline and a VarSet of its name, type, notes and
source. Exporting that sketch writes a .fcholder. The sketch lies on the XZ plane: X is the radius
out from the tool's axis and Z the height up from the face the bit leaves the holder, in mm. The
outline runs from the axis at the bottom, out and up, back to the axis at the top; a closed
profile may also run down the axis, which is left out."""

import builtins
import json
import os

import FreeCAD
import Part
import Path
import Sketcher

from FreeCAD import Placement, Rotation, Vector
from Path.Tool.holder.models import ToolHolder, dumps

# the VarSet properties holding a holder's details, and the .fcholder fields they are
DETAILS = (
    ("HolderName", "name", "The holder's name, as tools list it"),
    ("Type", "holder_type", "What kind of holder it is, such as ISO30-ER32 or Shrink fit"),
    ("Notes", "notes", "Anything worth knowing about it"),
    ("Source", "source", "Where its sizes came from"),
)
# a point this near the axis is on it, mm
ON_AXIS = 1e-6
# arcs and curves are written as straight segments no farther than this from them, mm
DEFLECTION = 0.01


def newHolderDocument(doc):
    """newHolderDocument(doc) ... Add an empty holder outline sketch on the XZ plane and a VarSet
    for its details to doc. Return (sketch, varset)."""
    varset = doc.addObject("App::VarSet", "Holder")
    for prop, _, tip in DETAILS:
        varset.addProperty("App::PropertyString", prop, "Holder", tip)
    sketch = doc.addObject("Sketcher::SketchObject", "Outline")
    # the XZ plane: the sketch's X is the radius, its Y the height up from the face
    sketch.Placement = Placement(Vector(), Rotation(Vector(1, 0, 0), 90))
    return sketch, varset


def drawOutline(sketch, profile):
    """drawOutline(sketch, profile) ... Draw profile, (radius, height) points, in sketch as one
    closed outline, joined back down the axis."""
    points = [Vector(r, z, 0) for r, z in profile]
    if (points[-1] - points[0]).Length > ON_AXIS:
        points.append(points[0])
    lines = []
    for start, end in zip(points, points[1:]):
        if (end - start).Length > ON_AXIS:
            lines.append(sketch.addGeometry(Part.LineSegment(start, end), False))
    for i, j in zip(lines, lines[1:] + lines[:1]):
        sketch.addConstraint(Sketcher.Constraint("Coincident", i, 2, j, 1))


def outline(sketch):
    """outline(sketch) ... Return the holder outline sketch draws, as (radius, height) points from
    the axis at the bottom, out and up, to the axis at the top. Raises ValueError for a sketch
    that draws no such outline."""
    shape = sketch.Shape
    if shape.isNull() or not shape.Edges:
        raise ValueError(f"{sketch.Label} draws nothing")
    wires = Part.sortEdges(shape.Edges)
    if len(wires) != 1:
        raise ValueError(f"{sketch.Label} draws {len(wires)} separate outlines, not one")
    wire = Part.Wire(wires[0])
    local = sketch.Placement.inverse()
    points = []
    for p in wire.discretize(Deflection=DEFLECTION):
        p = local.multVec(p)
        point = (0.0 if abs(p.x) < ON_AXIS else p.x, 0.0 if abs(p.y) < ON_AXIS else p.y)
        if not points or point != points[-1]:
            points.append(point)
    closed = wire.isClosed() and len(points) > 1 and points[0] == points[-1]
    if closed:
        points.pop()
    onAxis = [i for i, (r, _) in enumerate(points) if r == 0.0]
    if len(onAxis) < 2:
        raise ValueError(f"{sketch.Label} does not start and end on the tool's axis (X = 0)")
    if closed:
        # start at the lowest point on the axis, heading out from it
        start = min(onAxis, key=lambda i: points[i][1])
        points = points[start:] + points[:start]
        if points[1][0] == 0.0:
            points = points[:1] + points[1:][::-1]
        end = next(i for i in range(1, len(points)) if points[i][0] == 0.0)
        points = points[: end + 1]
    elif points[0][1] > points[-1][1]:
        points.reverse()
    if points[0][0] != 0.0 or points[-1][0] != 0.0:
        raise ValueError(f"{sketch.Label} does not start and end on the tool's axis (X = 0)")
    return points


def _details(doc):
    """Return the holder details the document's VarSet holds, by .fcholder field."""
    for obj in doc.Objects:
        if obj.isDerivedFrom("App::VarSet") and hasattr(obj, DETAILS[0][0]):
            return {field: getattr(obj, prop, "") for prop, field, _ in DETAILS}
    return {}


def _holderSketch(objects):
    """Return the sketch to export from the objects picked: the one picked, else the only sketch
    in their document."""
    for obj in objects:
        if obj.isDerivedFrom("Sketcher::SketchObject"):
            return obj
    doc = objects[0].Document if objects else FreeCAD.ActiveDocument
    sketches = [o for o in doc.Objects if o.isDerivedFrom("Sketcher::SketchObject")] if doc else []
    if len(sketches) != 1:
        raise ValueError("Pick the sketch of the holder's outline to export")
    return sketches[0]


def read(filename):
    """read(filename) ... Return the ToolHolder a .fcholder file holds."""
    # builtins: this module's own open() is File > Open
    with builtins.open(filename, encoding="utf-8") as f:
        data = json.load(f)
    return ToolHolder.from_dict(os.path.splitext(os.path.basename(filename))[0], data)


def insert(filename, docname):
    """insert(filename, docname) ... File > Import: add the holder's outline sketch and details
    to the document named docname."""
    holder = read(filename)
    doc = FreeCAD.getDocument(docname)
    sketch, varset = newHolderDocument(doc)
    for prop, field, _ in DETAILS:
        setattr(varset, prop, getattr(holder, field) or "")
    drawOutline(sketch, holder.profile)
    doc.recompute()
    return sketch


def open(filename):
    """open(filename) ... File > Open: a new document with the holder's outline sketch and
    details."""
    doc = FreeCAD.newDocument(os.path.splitext(os.path.basename(filename))[0])
    sketch = insert(filename, doc.Name)
    showProfile(doc)
    return sketch


def showProfile(doc):
    """showProfile(doc) ... Look at the holder's outline from the front, its profile, fitted to
    the view. Done at once, without animation: FreeCAD fits the view again after File > Open,
    and that then has nothing left to move."""
    if not FreeCAD.GuiUp:
        return
    import FreeCADGui

    gui = FreeCADGui.getDocument(doc.Name)
    view = getattr(gui, "ActiveView", None) if gui else None
    if view is None or not hasattr(view, "viewFront"):
        return
    animated = view.isAnimationEnabled()
    view.setAnimationEnabled(False)
    try:
        view.viewFront()
        view.fitAll()
    finally:
        view.setAnimationEnabled(animated)


def export(objects, filename):
    """export(objects, filename) ... File > Export: write the outline sketch picked, with the
    details its document's VarSet holds, as a .fcholder file named filename."""
    holderId = os.path.splitext(os.path.basename(filename))[0]
    try:
        sketch = _holderSketch(objects)
        details = _details(sketch.Document)
        holder = ToolHolder(
            id=holderId,
            name=details.get("name") or holderId,
            profile=outline(sketch),
            holder_type=details.get("holder_type", ""),
            notes=details.get("notes", ""),
            source=details.get("source", ""),
        )
    except ValueError as e:
        Path.Log.error(str(e))
        return
    with builtins.open(filename, "w", encoding="utf-8") as f:
        f.write(dumps(holder.to_dict()))
