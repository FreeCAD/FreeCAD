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
"""Provides various functions to sort lists of edges."""
## @package sort_edges
# \ingroup draftgeoutils
# \brief Provides various functions to sort lists of edges.

import lazy_loader.lazy_loader as lz

from draftgeoutils.general import geomType
from draftgeoutils.edges import findMidpoint, isLine, invert

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def sortEdges(edges):
    """Sort edges. Deprecated. Use Part.__sortEdges__ instead."""
    raise DeprecationWarning("Deprecated. Use Part.__sortEdges__ instead")

    if len(edges) < 2:
        return edges

    # Build a dictionary of edges according to their end points.
    # Each entry is a set of edges that starts, or ends, at the
    # given vertex hash.
    sdict = dict()
    edict = dict()
    nedges = []
    for e in edges:
        if hasattr(e, "Length"):
            if e.Length != 0:
                sdict.setdefault(e.Vertexes[0].hashCode(), []).append(e)
                edict.setdefault(e.Vertexes[-1].hashCode(), []).append(e)
                nedges.append(e)

    if not nedges:
        print("DraftGeomUtils.sortEdges: zero-length edges")
        return edges

    # Find the start of the path.  The start is the vertex that appears
    # in the sdict dictionary but not in the edict dictionary, and has
    # only one edge ending there.
    startedge = None
    for v, se in sdict.items():
        if v not in edict and len(se) == 1:
            startedge = se
            break

    # The above may not find a start vertex; if the start edge is reversed,
    # the start vertex will appear in edict (and not sdict).
    if not startedge:
        for v, se in edict.items():
            if v not in sdict and len(se) == 1:
                startedge = se
                break

    # If we still have no start vertex, it was a closed path.  If so, start
    # with the first edge in the supplied list
    if not startedge:
        startedge = nedges[0]
        v = startedge.Vertexes[0].hashCode()

    # Now build the return list by walking the edges starting at the start
    # vertex we found.  We're done when we've visited each edge, so the
    # end check is simply the count of input elements (that works for closed
    # as well as open paths).
    ret = list()
    # store the hash code of the last edge, to avoid picking the same edge back
    eh = None
    for _ in range(len(nedges)):
        try:
            eset = sdict[v]
            e = eset.pop()
            if not eset:
                del sdict[v]
            if e.hashCode() == eh:
                raise KeyError
            v = e.Vertexes[-1].hashCode()
            eh = e.hashCode()
        except KeyError:
            try:
                eset = edict[v]
                e = eset.pop()
                if not eset:
                    del edict[v]
                if e.hashCode() == eh:
                    raise KeyError
                v = e.Vertexes[0].hashCode()
                eh = e.hashCode()
                e = invert(e)
            except KeyError:
                print("DraftGeomUtils.sortEdges failed - running old version")
                return sortEdgesOld(edges)
        ret.append(e)

    return ret


def sortEdgesOld(lEdges, aVertex=None):
    """Sort edges. Deprecated. Use Part.__sortEdges__ instead."""
    raise DeprecationWarning("Deprecated. Use Part.__sortEdges__ instead")

    # There is no reason to limit this to lines only because
    # every non-closed edge always has exactly two vertices (wmayer)
    # for e in lEdges:
    #     if not isinstance(e.Curve,Part.LineSegment):
    #         print("Sortedges cannot treat wired containing curves yet.")
    #         return lEdges

    def lookfor(aVertex, inEdges):
        """Look for a vertex in the list of edges.

        Returns count, the position of the instance
        the position in the instance and the instance of the Edge.
        """
        count = 0
        linstances = []  # lists the instances of aVertex
        for i in range(len(inEdges)):
            for j in range(2):
                if aVertex.Point == inEdges[i].Vertexes[j-1].Point:
                    instance = inEdges[i]
                    count += 1
                    linstances += [i, j-1, instance]
        return [count] + linstances

    if len(lEdges) < 2:
        if aVertex is None:
            return lEdges
        else:
            result = lookfor(aVertex, lEdges)
            if result[0] != 0:
                if aVertex.Point == result[3].Vertexes[0].Point:
                    return lEdges
                else:
                    if geomType(result[3]) == "Line":
                        return [Part.LineSegment(aVertex.Point,
                                                 result[3].Vertexes[0].Point).toShape()]
                    elif geomType(result[3]) == "Circle":
                        mp = findMidpoint(result[3])
                        return [Part.Arc(aVertex.Point,
                                         mp,
                                         result[3].Vertexes[0].Point).toShape()]
                    elif (geomType(result[3]) == "BSplineCurve"
                          or geomType(result[3]) == "BezierCurve"):
                        if isLine(result[3].Curve):
                            return [Part.LineSegment(aVertex.Point,
                                                     result[3].Vertexes[0].Point).toShape()]
                        else:
                            return lEdges
                    else:
                        return lEdges

    olEdges = []  # ol stands for ordered list
    if aVertex is None:
        for i in range(len(lEdges)*2):
            if len(lEdges[i/2].Vertexes) > 1:
                result = lookfor(lEdges[i/2].Vertexes[i % 2], lEdges)
                if result[0] == 1:  # Have we found an end ?
                    olEdges = sortEdgesOld(lEdges,
                                           result[3].Vertexes[result[2]])
                    return olEdges
        # if the wire is closed there is no end so choose 1st Vertex
        # print("closed wire, starting from ",lEdges[0].Vertexes[0].Point)
        return sortEdgesOld(lEdges, lEdges[0].Vertexes[0])
    else:
        # print("looking ",aVertex.Point)
        result = lookfor(aVertex, lEdges)
        if result[0] != 0:
            del lEdges[result[1]]
            _next = sortEdgesOld(lEdges,
                                 result[3].Vertexes[-((-result[2])^1)])
            # print("result ", result[3].Vertexes[0].Point, "    ",
            #       result[3].Vertexes[1].Point, " compared to ",aVertex.Point)
            if aVertex.Point == result[3].Vertexes[0].Point:
                # print("keeping")
                olEdges += [result[3]] + _next
            else:
                # print("inverting", result[3].Curve)
                if geomType(result[3]) == "Line":
                    newedge = Part.LineSegment(aVertex.Point,
                                               result[3].Vertexes[0].Point).toShape()
                    olEdges += [newedge] + _next
                elif geomType(result[3]) == "Circle":
                    mp = findMidpoint(result[3])
                    newedge = Part.Arc(aVertex.Point,
                                       mp,
                                       result[3].Vertexes[0].Point).toShape()
                    olEdges += [newedge] + _next
                elif (geomType(result[3]) == "BSplineCurve"
                      or geomType(result[3]) == "BezierCurve"):
                    if isLine(result[3].Curve):
                        newedge = Part.LineSegment(aVertex.Point,
                                                   result[3].Vertexes[0].Point).toShape()
                        olEdges += [newedge] + _next
                    else:
                        olEdges += [result[3]] + _next
                else:
                    olEdges += [result[3]] + _next
            return olEdges
        else:
            return []

## @}
