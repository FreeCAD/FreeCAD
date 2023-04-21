# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides various functions to work with offsets."""
## @package offsets
# \ingroup draftgeoutils
# \brief Provides various functions to work with offsets.

import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils

from draftgeoutils.general import geomType, vec
from draftgeoutils.geometry import get_normal
from draftgeoutils.wires import isReallyClosed
from draftgeoutils.intersections import wiresIntersect, connect

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def pocket2d(shape, offset):
    """Return a list of wires obtained from offsetting wires from the shape.

    Return a list of wires obtained from offsetting the wires
    from the given shape by the given offset, and intersection if needed.
    """
    # find the outer wire
    length = 0
    outerWire = None
    innerWires = []
    for w in shape.Wires:
        if w.BoundBox.DiagonalLength > length:
            outerWire = w
            length = w.BoundBox.DiagonalLength

    if not outerWire:
        return []

    for w in shape.Wires:
        if w.hashCode() != outerWire.hashCode():
            innerWires.append(w)

    o = outerWire.makeOffset(-offset)

    if not o.Wires:
        return []

    offsetWires = o.Wires
    # print("base offset wires:", offsetWires)
    if not innerWires:
        return offsetWires

    for innerWire in innerWires:
        i = innerWire.makeOffset(offset)
        if len(innerWire.Edges) == 1:
            e = innerWire.Edges[0]
            if isinstance(e.Curve, Part.Circle):
                e = Part.makeCircle(e.Curve.Radius + offset,
                                    e.Curve.Center,
                                    e.Curve.Axis)
                i = Part.Wire(e)
        if i.Wires:
            # print("offsetting island ", innerWire, " : ", i.Wires)
            for w in i.Wires:
                added = False
                # print("checking wire ",w)
                k = list(range(len(offsetWires)))
                for j in k:
                    # print("checking against existing wire ", j)
                    ow = offsetWires[j]
                    if ow:
                        if wiresIntersect(w, ow):
                            # print("intersect")
                            f1 = Part.Face(ow)
                            f2 = Part.Face(w)
                            f3 = f1.cut(f2)
                            # print("made new wires: ", f3.Wires)
                            offsetWires[j] = f3.Wires[0]
                            if len(f3.Wires) > 1:
                                # print("adding more")
                                offsetWires.extend(f3.Wires[1:])
                            added = True
                        else:
                            a = w.BoundBox
                            b = ow.BoundBox
                            if ((a.XMin <= b.XMin)
                                    and (a.YMin <= b.YMin)
                                    and (a.ZMin <= b.ZMin)
                                    and (a.XMax >= b.XMax)
                                    and (a.YMax >= b.YMax)
                                    and (a.ZMax >= b.ZMax)):
                                # print("this wire is bigger than "
                                #       "the outer wire")
                                offsetWires[j] = None
                                added = True
                            # else:
                            #   print("doesn't intersect")
                if not added:
                    # print("doesn't intersect with any other")
                    offsetWires.append(w)
    offsetWires = [o for o in offsetWires if o is not None]

    return offsetWires


def offset(edge, vector, trim=False):
    """Return a copy of the edge at a certain vector offset.

    If the edge is an arc, the vector will be added at its first point
    and a complete circle will be returned.

    None if there is a problem.
    """
    if (not isinstance(edge, Part.Shape)
            or not isinstance(vector, App.Vector)):
        return None

    if geomType(edge) == "Line":
        v1 = App.Vector.add(edge.Vertexes[0].Point, vector)
        v2 = App.Vector.add(edge.Vertexes[-1].Point, vector)
        return Part.LineSegment(v1, v2).toShape()

    elif geomType(edge) == "Circle":
        rad = edge.Vertexes[0].Point.sub(edge.Curve.Center)
        curve = Part.Circle(edge.Curve)
        curve.Radius = App.Vector.add(rad, vector).Length
        if trim:
            return Part.ArcOfCircle(curve,
                                    edge.FirstParameter,
                                    edge.LastParameter).toShape()
    elif geomType(edge) == "Ellipse":
        rad = edge.Vertexes[0].Point.sub(edge.Curve.Center)
        curve = edge.Curve.copy()
        if vector.getAngle(rad) < 1:
            curve.MajorRadius = curve.MajorRadius+vector.Length
            curve.MinorRadius = curve.MinorRadius+vector.Length
        else:
            curve.MajorRadius = curve.MajorRadius-vector.Length
            curve.MinorRadius = curve.MinorRadius-vector.Length
        return curve.toShape()
    else:
        return None


def offsetWire(wire, dvec, bind=False, occ=False,
               widthList=None, offsetMode=None, alignList=[],
               normal=None, basewireOffset=0):
    """Offset the wire along the given vector.

    Parameters
    ----------
    wire as a sorted list of edges (the list is used directly), or as a
    wire or a face (Draft Wire with MakeFace True or False supported).

    The vector will be applied at the first vertex of the wire. If bind
    is True (and the shape is open), the original wire and the offsetted one
    are bound by 2 edges, forming a face.

    If widthList is provided (values only, not lengths - i.e. no unit),
    each value will be used to offset each corresponding edge in the wire.

    The 1st value overrides 'dvec' for 1st segment of wire;
    if a value is zero, value of 'widthList[0]' will follow;
    if widthList[0]' == 0, but dvec still provided, dvec will be followed

    offsetMode="BasewireMode" or None

    If alignList is provided,
    each value will be used to offset each corresponding edge
    in the wire with corresponding index.

    'basewireOffset' corresponds to 'offset' in ArchWall which offset
    the basewire before creating the wall outline

    OffsetWire() is now aware of width and align per edge
    Primarily for use with ArchWall based on Sketch object

    To Do
    -----
    `dvec` vector to offset is now derived (and can be ignored)
    in this function if widthList and alignList are provided
    - 'dvec' to be obsolete in future?
    """
    if isinstance(wire, list) and isinstance(wire[0], Part.Edge):
        edges = wire.copy()
        wire = Part.Wire(edges)
        closed = wire.isClosed()
    elif isinstance(wire, Part.Wire):
        # Draft_Offset can fail when directly offsetting a Sketch wire. We need
        # to sort the edges. And because Part.__sortEdges__() can remove edges,
        # we need to create a new wire as well.
        edges = Part.__sortEdges__(wire.Edges)
        wire = Part.Wire(edges)
        closed = wire.isClosed()
    elif isinstance(wire, Part.Face):
        # We also need to sort the edges of a face.
        edges = Part.__sortEdges__(wire.OuterWire.Edges)
        closed = True
    elif isinstance(wire, Part.Edge):
        edges = [wire]
        closed = wire.isClosed()
    else:
        print("Either Part.Wire or Part.Edges should be provided, "
              "returning None")
        return None

    # For sketch with a number of wires, getNormal() may result
    # in different direction for each wire.
    # The 'normal' parameter, if provided e.g. by ArchWall,
    # allows normal over different wires e.g. in a Sketch be consistent
    # (over different calls of this function)
    if normal:
        norm = normal
    else:
        norm = get_normal(wire)  # norm = Vector(0, 0, 1)
        # for backward compatibility with previous getNormal implementation
        if norm is None:
            norm = App.Vector(0, 0, 1)

    nedges = []
    if occ:
        length = abs(dvec.Length)
        if not length:
            return None

        if wire.Wires:
            wire = wire.Wires[0]
        else:
            wire = Part.Wire(edges)

        try:
            off = wire.makeOffset(length)
        except Part.OCCError:
            return None
        else:
            return off

    # vec of first edge depends on its geometry
    e = edges[0]

    # Make a copy of alignList - to avoid changes in this function
    # become starting input of next call of this function?
    # https://www.dataquest.io/blog/tutorial-functions-modify-lists-dictionaries-python/
    # alignListC = alignList.copy()  # Only Python 3
    alignListC = list(alignList)  # Python 2 and 3

    # Check the direction / offset of starting edge
    firstDir = None
    try:
        if alignListC[0] == 'Left':
            firstDir = 1
            firstAlign = 'Left'
        elif alignListC[0] == 'Right':
            firstDir = -1
            firstAlign = 'Right'
        elif alignListC[0] == 'Center':
            firstDir = 1
            firstAlign = 'Center'
    except IndexError:
        # Should no longer happen for ArchWall
        # as aligns are 'filled in' by ArchWall
        pass

    # If not provided by alignListC checked above, check the direction
    # of offset in dvec (not 'align').

    # TODO Should check if dvec is provided or not
    # ('legacy/backward-compatible' mode)
    if not firstDir:
        # need to test against Part.Circle, not Part.ArcOfCircle
        if isinstance(e.Curve, (Part.Circle,Part.Ellipse)):
            v0 = e.tangentAt(e.FirstParameter).cross(norm)
        else:
            v0 = vec(e).cross(norm)
        # check against dvec provided for the offset direction
        # would not know if dvec is vector of width (Left/Right Align)
        # or width/2 (Center Align)
        v0.normalize()
        v1 = App.Vector(dvec).normalize()
        if v0.isEqual(v1, 0.0001):
            # "Left Offset" (Left Align or 'left offset' in Centre Align)
            firstDir = 1
            firstAlign = 'Left'
            alignListC.append('Left')
        elif v0.isEqual(v1.negative(), 0.0001):
            # "Right Offset" (Right Align or 'right offset' in Centre Align)
            firstDir = -1
            firstAlign = 'Right'
            alignListC.append('Right')
        else:
            print(" something wrong with firstDir ")
            firstAlign = 'Left'
            alignListC.append('Left')

    for i in range(len(edges)):
        # make a copy so it do not reverse the self.baseWires edges
        # pointed to by _Wall.getExtrusionData()?
        curredge = edges[i].copy()

        # record first edge's Orientation, Dir, Align and set Delta
        if i == 0:
            # TODO Could be edge.Orientation in fact
            # "Forward" or "Reversed"
            firstOrientation = curredge.Vertexes[0].Orientation
            curOrientation = firstOrientation
            curDir = firstDir
            curAlign = firstAlign
            delta = dvec

        # record current edge's Orientation, and set Delta
        if i != 0:  # else:
            # TODO Should also calculate 1st edge direction above
            if isinstance(curredge.Curve, Part.Circle):
                delta = curredge.tangentAt(curredge.FirstParameter).cross(norm)
            else:
                delta = vec(curredge).cross(norm)
            # TODO Could be edge.Orientation in fact
            curOrientation = curredge.Vertexes[0].Orientation

        # Consider individual edge width
        if widthList:  # ArchWall should now always provide widthList
            try:
                if widthList[i] > 0:
                    delta = DraftVecUtils.scaleTo(delta, widthList[i])
                elif dvec:
                    delta = DraftVecUtils.scaleTo(delta, dvec.Length)
                else:
                    # just hardcoded default value as ArchWall would provide
                    # if dvec is not provided either
                    delta = DraftVecUtils.scaleTo(delta, 200)
            except Part.OCCError:
                if dvec:
                    delta = DraftVecUtils.scaleTo(delta, dvec.Length)
                else:
                    # just hardcoded default value as ArchWall would provide
                    # if dvec is not provided either
                    delta = DraftVecUtils.scaleTo(delta, 200)
        else:
            delta = DraftVecUtils.scaleTo(delta, dvec.Length)

        # Consider individual edge Align direction
        # - ArchWall should now always provide alignList
        if i == 0:
            if alignListC[0] == 'Center':
                delta = DraftVecUtils.scaleTo(delta, delta.Length/2)
            # No need to do anything for 'Left' and 'Right' as original dvec
            # have set both the direction and amount of offset correct
            # elif alignListC[i] == 'Left':  #elif alignListC[i] == 'Right':
        if i != 0:
            try:
                if alignListC[i] == 'Left':
                    curDir = 1
                    curAlign = 'Left'
                elif alignListC[i] == 'Right':
                    curDir = -1
                    curAlign = 'Right'
                    delta = delta.negative()
                elif alignListC[i] == 'Center':
                    curDir = 1
                    curAlign = 'Center'
                    delta = DraftVecUtils.scaleTo(delta, delta.Length/2)
            except IndexError:
                curDir = firstDir
                curAlign = firstAlign
                if firstAlign == 'Right':
                    delta = delta.negative()
                elif firstAlign == 'Center':
                    delta = DraftVecUtils.scaleTo(delta, delta.Length/2)

        # Consider whether generating the 'offset wire' or the 'base wire'
        if offsetMode is None:
            # Consider if curOrientation and/or curDir match their
            # firstOrientation/firstDir - to determine whether
            # and how to offset the current edge

            # This is a xor
            if (curOrientation == firstOrientation) != (curDir == firstDir):
                if curAlign in ['Left', 'Right']:
                    # ArchWall has an Offset properties for user to offset
                    # the basewire before creating the base profile of wall
                    # (not applicable to 'Center' align)
                    if basewireOffset:
                        delta = DraftVecUtils.scaleTo(delta, basewireOffset)
                        nedge = offset(curredge,delta,trim=True)
                    else:
                        nedge = curredge
                elif curAlign == 'Center':
                    delta = delta.negative()
                    nedge = offset(curredge, delta, trim=True)
            else:
                # if curAlign in ['Left', 'Right']:
                # elif curAlign == 'Center': # Both conditions same result.
                # ArchWall has an Offset properties for user to offset
                # the basewire before creating the base profile of wall
                # (not applicable to 'Center' align)
                if basewireOffset:
                    if curAlign in ['Left', 'Right']:
                        delta = DraftVecUtils.scaleTo(delta,
                                                      delta.Length + basewireOffset)
                    #else: # elif curAlign == 'Center': #pass # no need to add basewireOffset
                nedge = offset(curredge, delta, trim=True)

            # TODO arc always in counter-clockwise directinon
            # ... ( not necessarily 'reversed')
            if curOrientation == "Reversed":
                # need to test against Part.Circle, not Part.ArcOfCircle
                if not isinstance(curredge.Curve, Part.Circle):
                    # if not arc/circle, assume straight line, reverse it
                    nedge = Part.Edge(nedge.Vertexes[1], nedge.Vertexes[0])
                else:
                    # if arc/circle
                    # Part.ArcOfCircle(edge.Curve,
                    #                  edge.FirstParameter, edge.LastParameter,
                    #                  edge.Curve.Axis.z > 0)
                    midParameter = nedge.FirstParameter + (nedge.LastParameter - nedge.FirstParameter)/2
                    midOfArc = nedge.valueAt(midParameter)
                    nedge = Part.ArcOfCircle(nedge.Vertexes[1].Point,
                                             midOfArc,
                                             nedge.Vertexes[0].Point).toShape()
                    # TODO any better solution than to calculate midpoint
                    # of arc to reverse?

        elif offsetMode in ["BasewireMode"]:
            if (not (curOrientation == firstOrientation)
                    != (curDir == firstDir)):
                if curAlign in ['Left', 'Right']:
                    # ArchWall has an Offset properties for user to offset
                    # the basewire before creating the base profile of wall
                    # (not applicable to 'Center' align)
                    if basewireOffset:
                        delta = DraftVecUtils.scaleTo(delta, basewireOffset)
                        nedge = offset(curredge, delta, trim=True)
                    else:
                        nedge = curredge
                elif curAlign == 'Center':
                    delta = delta.negative()
                    nedge = offset(curredge, delta, trim=True)
            else:
                if curAlign in ['Left', 'Right']:
                    # ArchWall has an Offset properties for user to offset
                    # the basewire before creating the base profile of wall
                    # (not applicable to 'Center' align)
                    if basewireOffset:
                        delta = DraftVecUtils.scaleTo(delta,
                                                      delta.Length + basewireOffset)
                    nedge = offset(curredge, delta, trim=True)

                elif curAlign == 'Center':
                    nedge = offset(curredge, delta, trim=True)
            if curOrientation == "Reversed":
                # need to test against Part.Circle, not Part.ArcOfCircle
                if not isinstance(curredge.Curve, Part.Circle):
                    # if not arc/circle, assume straight line, reverse it
                    nedge = Part.Edge(nedge.Vertexes[1], nedge.Vertexes[0])
                else:
                    # if arc/circle
                    # Part.ArcOfCircle(edge.Curve,
                    #                  edge.FirstParameter,
                    #                  edge.LastParameter,
                    #                  edge.Curve.Axis.z > 0)
                    midParameter = nedge.FirstParameter + (nedge.LastParameter - nedge.FirstParameter)/2
                    midOfArc = nedge.valueAt(midParameter)
                    nedge = Part.ArcOfCircle(nedge.Vertexes[1].Point,
                                             midOfArc,
                                             nedge.Vertexes[0].Point).toShape()
                    # TODO any better solution than to calculate midpoint
                    # of arc to reverse?
        else:
            print(" something wrong ")
            return None
        if not nedge:
            return None

        nedges.append(nedge)

    if len(edges) > 1:
        nedges = connect(nedges, closed)
    else:
        nedges = Part.Wire(nedges[0])

    if bind and not closed:
        e1 = Part.LineSegment(edges[0].Vertexes[0].Point,
                              nedges[0].Vertexes[0].Point).toShape()
        e2 = Part.LineSegment(edges[-1].Vertexes[-1].Point,
                              nedges[-1].Vertexes[-1].Point).toShape()
        alledges = edges.extend(nedges)
        alledges = alledges.extend([e1, e2])
        w = Part.Wire(alledges)
        return w
    else:
        return nedges

## @}
