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
"""Provides various functions to work with fillets."""
## @package fillets
# \ingroup draftgeoutils
# \brief Provides various functions to work with fillets.

import math
import lazy_loader.lazy_loader as lz

import FreeCAD as App

from draftgeoutils.general import precision
from draftgeoutils.arcs import arcFrom2Pts
from draftgeoutils.wires import isReallyClosed

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def fillet(lEdges, r, chamfer=False):
    """Return a list of sorted edges describing a round corner.

    Author: Jacques-Antoine Gaudin
    """

    def getCurveType(edge, existingCurveType=None):
        """Build or complete a dictionary containing edges.

        The dictionary contains edges with keys 'Arc' and 'Line'.
        """
        if not existingCurveType:
            existingCurveType = {'Line': [],
                                 'Arc': []}
        if issubclass(type(edge.Curve), Part.LineSegment):
            existingCurveType['Line'] += [edge]
        elif issubclass(type(edge.Curve), Part.Line):
            existingCurveType['Line'] += [edge]
        elif issubclass(type(edge.Curve), Part.Circle):
            existingCurveType['Arc'] += [edge]
        else:
            raise ValueError("Edge's curve must be either Line or Arc")
        return existingCurveType

    rndEdges = lEdges[0:2]
    rndEdges = Part.__sortEdges__(rndEdges)

    if len(rndEdges) < 2:
        return rndEdges

    if r <= 0:
        print("DraftGeomUtils.fillet: Error: radius is negative.")
        return rndEdges

    curveType = getCurveType(rndEdges[0])
    curveType = getCurveType(rndEdges[1], curveType)

    lVertexes = rndEdges[0].Vertexes + [rndEdges[1].Vertexes[-1]]

    if len(curveType['Line']) == 2:
        # Deals with 2-line-edges lists
        U1 = lVertexes[0].Point.sub(lVertexes[1].Point)
        U1.normalize()

        U2 = lVertexes[2].Point.sub(lVertexes[1].Point)
        U2.normalize()

        alpha = U1.getAngle(U2)

        if chamfer:
            # correcting r value so the size of the chamfer = r
            beta = math.pi - alpha/2
            r = (r/2)/math.cos(beta)

        # Edges have same direction
        if (round(alpha, precision()) == 0
                or round(alpha - math.pi, precision()) == 0):
            print("DraftGeomUtils.fillet: Warning: "
                  "edges have same direction. Did nothing")
            return rndEdges

        dToCenter = r / math.sin(alpha/2.0)
        dToTangent = (dToCenter**2-r**2)**(0.5)
        dirVect = App.Vector(U1)
        dirVect.scale(dToTangent, dToTangent, dToTangent)
        arcPt1 = lVertexes[1].Point.add(dirVect)

        dirVect = U2.add(U1)
        dirVect.normalize()
        dirVect.scale(dToCenter - r, dToCenter - r, dToCenter - r)
        arcPt2 = lVertexes[1].Point.add(dirVect)

        dirVect = App.Vector(U2)
        dirVect.scale(dToTangent, dToTangent, dToTangent)
        arcPt3 = lVertexes[1].Point.add(dirVect)

        if (dToTangent > lEdges[0].Length) or (dToTangent > lEdges[1].Length):
            print("DraftGeomUtils.fillet: Error: radius value ", r,
                  " is too high")
            return rndEdges

        if chamfer:
            rndEdges[1] = Part.Edge(Part.LineSegment(arcPt1, arcPt3))
        else:
            rndEdges[1] = Part.Edge(Part.Arc(arcPt1, arcPt2, arcPt3))

        if lVertexes[0].Point == arcPt1:
            # fillet consumes entire first edge
            rndEdges.pop(0)
        else:
            rndEdges[0] = Part.Edge(Part.LineSegment(lVertexes[0].Point,
                                                     arcPt1))

        if lVertexes[2].Point != arcPt3:
            # fillet does not consume entire second edge
            rndEdges += [Part.Edge(Part.LineSegment(arcPt3,
                                                    lVertexes[2].Point))]

        return rndEdges

    elif len(curveType['Arc']) == 1:
        # Deals with lists containing an arc and a line
        if lEdges[0] in curveType['Arc']:
            lineEnd = lVertexes[2]
            arcEnd = lVertexes[0]
            arcFirst = True
        else:
            lineEnd = lVertexes[0]
            arcEnd = lVertexes[2]
            arcFirst = False
        arcCenter = curveType['Arc'][0].Curve.Center
        arcRadius = curveType['Arc'][0].Curve.Radius
        arcAxis = curveType['Arc'][0].Curve.Axis
        arcLength = curveType['Arc'][0].Length

        U1 = lineEnd.Point.sub(lVertexes[1].Point)
        U1.normalize()
        toCenter = arcCenter.sub(lVertexes[1].Point)
        if arcFirst:  # make sure the tangent points towards the arc
            T = arcAxis.cross(toCenter)
        else:
            T = toCenter.cross(arcAxis)

        projCenter = toCenter.dot(U1)
        if round(abs(projCenter), precision()) > 0:
            normToLine = U1.cross(T).cross(U1)
        else:
            normToLine = App.Vector(toCenter)
        normToLine.normalize()

        dCenterToLine = toCenter.dot(normToLine) - r

        if round(projCenter, precision()) > 0:
            newRadius = arcRadius - r
        elif (round(projCenter, precision()) < 0
              or (round(projCenter, precision()) == 0 and U1.dot(T) > 0)):
            newRadius = arcRadius + r
        else:
            print("DraftGeomUtils.fillet: Warning: "
                  "edges are already tangent. Did nothing")
            return rndEdges

        toNewCent = newRadius**2 - dCenterToLine**2
        if toNewCent > 0:
            toNewCent = abs(abs(projCenter) - toNewCent**(0.5))
        else:
            print("DraftGeomUtils.fillet: Error: radius value ", r,
                  " is too high")
            return rndEdges

        U1.scale(toNewCent, toNewCent, toNewCent)
        normToLine.scale(r, r, r)
        newCent = lVertexes[1].Point.add(U1).add(normToLine)

        arcPt1 = lVertexes[1].Point.add(U1)
        arcPt2 = lVertexes[1].Point.sub(newCent)
        arcPt2.normalize()
        arcPt2.scale(r, r, r)
        arcPt2 = arcPt2.add(newCent)

        if newRadius == arcRadius - r:
            arcPt3 = newCent.sub(arcCenter)
        else:
            arcPt3 = arcCenter.sub(newCent)
        arcPt3.normalize()
        arcPt3.scale(r, r, r)
        arcPt3 = arcPt3.add(newCent)
        arcPt = [arcPt1, arcPt2, arcPt3]

        # Warning: In the following I used a trick for calling
        # the right element in arcPt or V:
        # arcFirst is a boolean so - not arcFirst is -0 or -1
        # list[-1] is the last element of a list and list[0] the first
        # this way I don't have to proceed tests to know the position
        # of the arc
        myTrick = not arcFirst

        V = [arcPt3]
        V += [arcEnd.Point]

        toCenter.scale(-1, -1, -1)

        delLength = arcRadius * V[0].sub(arcCenter).getAngle(toCenter)
        if delLength > arcLength or toNewCent > curveType['Line'][0].Length:
            print("DraftGeomUtils.fillet: Error: radius value ", r,
                  " is too high")
            return rndEdges

        arcAsEdge = arcFrom2Pts(V[-arcFirst], V[-myTrick], arcCenter, arcAxis)

        V = [lineEnd.Point, arcPt1]
        lineAsEdge = Part.Edge(Part.LineSegment(V[-arcFirst], V[myTrick]))

        rndEdges[not arcFirst] = arcAsEdge
        rndEdges[arcFirst] = lineAsEdge
        if chamfer:
            rndEdges[1:1] = [Part.Edge(Part.LineSegment(arcPt[- arcFirst],
                                                        arcPt[- myTrick]))]
        else:
            rndEdges[1:1] = [Part.Edge(Part.Arc(arcPt[- arcFirst],
                                                arcPt[1],
                                                arcPt[- myTrick]))]

        return rndEdges

    elif len(curveType['Arc']) == 2:
        # Deals with lists of 2 arc-edges
        (arcCenter, arcRadius,
         arcAxis, arcLength,
         toCenter, T, newRadius) = [], [], [], [], [], [], []

        for i in range(2):
            arcCenter += [curveType['Arc'][i].Curve.Center]
            arcRadius += [curveType['Arc'][i].Curve.Radius]
            arcAxis += [curveType['Arc'][i].Curve.Axis]
            arcLength += [curveType['Arc'][i].Length]
            toCenter += [arcCenter[i].sub(lVertexes[1].Point)]

        T += [arcAxis[0].cross(toCenter[0])]
        T += [toCenter[1].cross(arcAxis[1])]
        CentToCent = toCenter[1].sub(toCenter[0])
        dCentToCent = CentToCent.Length

        sameDirection = (arcAxis[0].dot(arcAxis[1]) > 0)
        TcrossT = T[0].cross(T[1])

        if sameDirection:
            if round(TcrossT.dot(arcAxis[0]), precision()) > 0:
                newRadius += [arcRadius[0] + r]
                newRadius += [arcRadius[1] + r]
            elif round(TcrossT.dot(arcAxis[0]), precision()) < 0:
                newRadius += [arcRadius[0] - r]
                newRadius += [arcRadius[1] - r]
            elif T[0].dot(T[1]) > 0:
                newRadius += [arcRadius[0] + r]
                newRadius += [arcRadius[1] + r]
            else:
                print("DraftGeomUtils.fillet: Warning: "
                      "edges are already tangent. Did nothing")
                return rndEdges

        elif not sameDirection:
            if round(TcrossT.dot(arcAxis[0]), precision()) > 0:
                newRadius += [arcRadius[0] + r]
                newRadius += [arcRadius[1] - r]
            elif round(TcrossT.dot(arcAxis[0]), precision()) < 0:
                newRadius += [arcRadius[0] - r]
                newRadius += [arcRadius[1] + r]
            elif T[0].dot(T[1]) > 0:
                if arcRadius[0] > arcRadius[1]:
                    newRadius += [arcRadius[0] - r]
                    newRadius += [arcRadius[1] + r]
                elif arcRadius[1] > arcRadius[0]:
                    newRadius += [arcRadius[0] + r]
                    newRadius += [arcRadius[1] - r]
                else:
                    print("DraftGeomUtils.fillet: Warning: "
                          "arcs are coincident. Did nothing")
                    return rndEdges
            else:
                print("DraftGeomUtils.fillet: Warning: "
                      "edges are already tangent. Did nothing")
                return rndEdges

        if (newRadius[0] + newRadius[1] < dCentToCent
                or newRadius[0] - newRadius[1] > dCentToCent
                or newRadius[1] - newRadius[0] > dCentToCent):
            print("DraftGeomUtils.fillet: Error: radius value ", r,
                  " is too high")
            return rndEdges

        x = ((dCentToCent**2 + newRadius[0]**2 - newRadius[1]**2)
             / (2*dCentToCent))
        y = (newRadius[0]**2 - x**2)**(0.5)

        CentToCent.normalize()
        toCenter[0].normalize()
        toCenter[1].normalize()
        if abs(toCenter[0].dot(toCenter[1])) != 1:
            normVect = CentToCent.cross(CentToCent.cross(toCenter[0]))
        else:
            normVect = T[0]

        normVect.normalize()
        CentToCent.scale(x, x, x)
        normVect.scale(y, y, y)
        newCent = arcCenter[0].add(CentToCent.add(normVect))
        CentToNewCent = [newCent.sub(arcCenter[0]),
                         newCent.sub(arcCenter[1])]

        for i in range(2):
            CentToNewCent[i].normalize()
            if newRadius[i] == arcRadius[i] + r:
                CentToNewCent[i].scale(-r, -r, -r)
            else:
                CentToNewCent[i].scale(r, r, r)

        toThirdPt = lVertexes[1].Point.sub(newCent)
        toThirdPt.normalize()
        toThirdPt.scale(r, r, r)
        arcPt1 = newCent.add(CentToNewCent[0])
        arcPt2 = newCent.add(toThirdPt)
        arcPt3 = newCent.add(CentToNewCent[1])
        arcPt = [arcPt1, arcPt2, arcPt3]

        arcAsEdge = []
        for i in range(2):
            toCenter[i].scale(-1, -1, -1)
            delLength = (arcRadius[i]
                         * arcPt[-i].sub(arcCenter[i]).getAngle(toCenter[i]))
            if delLength > arcLength[i]:
                print("DraftGeomUtils.fillet: Error: radius value ", r,
                      " is too high")
                return rndEdges
            V = [arcPt[-i], lVertexes[-i].Point]
            arcAsEdge += [arcFrom2Pts(V[i-1], V[-i],
                                      arcCenter[i], arcAxis[i])]

        rndEdges[0] = arcAsEdge[0]
        rndEdges[1] = arcAsEdge[1]
        if chamfer:
            rndEdges[1:1] = [Part.Edge(Part.LineSegment(arcPt[0], arcPt[2]))]
        else:
            rndEdges[1:1] = [Part.Edge(Part.Arc(arcPt[0],
                                                arcPt[1],
                                                arcPt[2]))]

        return rndEdges


def filletWire(aWire, r, chamfer=False):
    """Fillet each angle of a wire with r as radius.

    If chamfer is true, a `chamfer` is made instead, and `r` is the
    size of the chamfer.
    """
    edges = aWire.Edges
    edges = Part.__sortEdges__(edges)
    filEdges = [edges[0]]

    for i in range(len(edges) - 1):
        result = fillet([filEdges[-1], edges[i+1]], r, chamfer)
        if len(result) > 2:
            filEdges[-1:] = result[0:3]
        else:
            filEdges[-1:] = result[0:2]

    if isReallyClosed(aWire):
        result = fillet([filEdges[-1], filEdges[0]], r, chamfer)
        if len(result) > 2:
            filEdges[-1:] = result[0:2]
            filEdges[0] = result[2]

    return Part.Wire(filEdges)

## @}
