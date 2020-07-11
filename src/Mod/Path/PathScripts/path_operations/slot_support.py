# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 Russell Johnson (russ4262) <russ4262@gmail.com>    *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

from __future__ import print_function

__title__ = "Path Slot Utilities"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Utility functions for the Slot operation."
__contributors__ = ""

# Standard
import math
# Third-party
# FreeCAD
import FreeCAD

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')


# Independent support methods
def _getCutSidePoints(base, v0, v1, a1, a2, b1, b2):
    ea1 = Part.makeLine(v0, a1)
    ea2 = Part.makeLine(a1, a2)
    ea3 = Part.makeLine(a2, v1)
    ea4 = Part.makeLine(v1, v0)
    boxA = Part.Face(Part.Wire([ea1, ea2, ea3, ea4]))
    cubeA = boxA.extrude(FreeCAD.Vector(0.0, 0.0, 1.0))
    cmnA = base.Shape.common(cubeA)
    eb1 = Part.makeLine(v0, b1)
    eb2 = Part.makeLine(b1, b2)
    eb3 = Part.makeLine(b2, v1)
    eb4 = Part.makeLine(v1, v0)
    boxB = Part.Face(Part.Wire([eb1, eb2, eb3, eb4]))
    cubeB = boxB.extrude(FreeCAD.Vector(0.0, 0.0, 1.0))
    cmnB = base.Shape.common(cubeB)
    if cmnA.Volume > cmnB.Volume:
        return (b1, b2)
    return (a1, a2)


def _isParallel(dYdX1, dYdX2):
    if dYdX1.add(dYdX2).Length == 0:
        return True
    if ((dYdX1.x + dYdX2.x) / 2.0 == dYdX1.x and
            (dYdX1.y + dYdX2.y) / 2.0 == dYdX1.y):
        return True
    return False


def _makePerpendicular(p1, p2, length, dYdX1, dYdX2):
    line = Part.makeLine(p1, p2)
    midPnt = line.CenterOfMass

    halfDist = length / 2.0
    if dYdX1:
        half = FreeCAD.Vector(dYdX1.x, dYdX1.y, 0.0).multiply(halfDist)
        n1 = midPnt.add(half)
        n2 = midPnt.sub(half)
        return (n1, n2)
    elif dYdX2:
        half = FreeCAD.Vector(dYdX2.x, dYdX2.y, 0.0).multiply(halfDist)
        n1 = midPnt.add(half)
        n2 = midPnt.sub(half)
        return (n1, n2)
    else:
        toEnd = p2.sub(p1)
        factor = halfDist / toEnd.Length
        perp = FreeCAD.Vector(-1 * toEnd.y, toEnd.x, 0.0)
        perp.normalize()
        perp.multiply(halfDist)
        n1 = midPnt.add(perp)
        n2 = midPnt.sub(perp)
        return (n1, n2)


def _xyToRadians(vectPnt):
    # Assumes Z value of vector is zero
    halfPi = math.pi / 2

    if vectPnt.y == 1 and vectPnt.x == 0:
        return halfPi
    if vectPnt.y == -1 and vectPnt.x == 0:
        return math.pi + halfPi
    if vectPnt.y == 0 and vectPnt.x == 1:
        return 0.0
    if vectPnt.y == 0 and vectPnt.x == -1:
        return math.pi

    x = abs(vectPnt.x)
    y = abs(vectPnt.y)
    rads = math.atan(y/x)
    if vectPnt.x > 0:
        if vectPnt.y > 0:
            return rads
        else:
            return (2 * math.pi) - rads
    if vectPnt.x < 0:
        if vectPnt.y > 0:
            return math.pi - rads
        else:
            return math.pi + rads


def _dXdYdZ(edge):
    '''Returns a vector with delta-X, delta-Y, and delta-Z between
    the two vertexes on the edge received.'''
    v1 = edge.Vertexes[0]
    v2 = edge.Vertexes[1]
    dX = v2.X - v1.X
    dY = v2.Y - v1.Y
    dZ = v2.Z - v1.Z
    return FreeCAD.Vector(dX, dY, dZ)


def _normalizeVector(v):
    posTol = 0.0000000001
    negTol = -1 * posTol
    V = FreeCAD.Vector(v.x, v.y, v.z)
    V.normalize()
    x = V.x
    y = V.y
    z = V.z

    if V.x != 0 and abs(V.x) < posTol:
        x = 0.0
    if V.x != 1 and 1.0 - V.x < posTol:
        x = 1.0
    if V.x != -1 and -1.0 - V.x > negTol:
        x = -1.0

    if V.y != 0 and abs(V.y) < posTol:
        y = 0.0
    if V.y != 1 and 1.0 - V.y < posTol:
        y = 1.0
    if V.y != -1 and -1.0 - V.y > negTol:
        y = -1.0

    if V.z != 0 and abs(V.z) < posTol:
        z = 0.0
    if V.z != 1 and 1.0 - V.z < posTol:
        z = 1.0
    if V.z != -1 and -1.0 - V.z > negTol:
        z = -1.0

    return FreeCAD.Vector(x, y, z)


def _lowestVertexAsVector(shape):
    # find lowest vertex
    vMin = shape.Vertexes[0]
    zmin = vMin.Z
    same = [vMin]
    for V in shape.Vertexes:
        if V.Z < zmin:
            zmin = V.Z
            vMin = V
        elif V.Z == zmin:
            same.append(V)
    if len(same) > 1:
        X = [E.X for E in same]
        Y = [E.Y for E in same]
        avgX = sum(X) / len(X)
        avgY = sum(Y) / len(Y)
        return FreeCAD.Vector(avgX, avgY, zmin)
    else:
        return FreeCAD.Vector(V.X, V.Y, V.Z)


def _highestVertexAsVector(shape):
    # find highest vertex
    vMax = shape.Vertexes[0]
    zmax = vMax.Z
    same = [vMax]
    for V in shape.Vertexes:
        if V.Z > zmax:
            zmax = V.Z
            vMax = V
        elif V.Z == zmax:
            same.append(V)
    if len(same) > 1:
        X = [E.X for E in same]
        Y = [E.Y for E in same]
        avgX = sum(X) / len(X)
        avgY = sum(Y) / len(Y)
        return FreeCAD.Vector(avgX, avgY, zmax)
    else:
        return FreeCAD.Vector(V.X, V.Y, V.Z)


def _findLowestPointOnEdge(edge):
    tol = 0.0000001
    zMin = edge.BoundBox.ZMin
    # Test first vertex
    v = edge.Vertexes[0]
    if abs(v.Z - zMin) < tol:
        return FreeCAD.Vector(v.X, v.Y, v.Z)
    # Test second vertex
    v = edge.Vertexes[1]
    if abs(v.Z - zMin) < tol:
        return FreeCAD.Vector(v.X, v.Y, v.Z)
    # Test middle point of edge
    eMidLen = edge.Length / 2.0
    eMidPnt = edge.valueAt(edge.getParameterByLength(eMidLen))
    if abs(eMidPnt.z - zMin) < tol:
        return eMidPnt
    if edge.BoundBox.ZLength < 0.000000001:  # roughly horizontal edge
        return eMidPnt
    return _findLowestEdgePoint(E)


def _findLowestEdgePoint(edge):
    zMin = edge.BoundBox.ZMin
    eLen = edge.Length
    L0 = 0
    L1 = eLen
    p0 = None
    p1 = None
    cnt = 0
    while L1 - L0 > 0.00001 and cnt < 2000:
        adj = (L1 - L0) * 0.1
        # Get points at L0 and L1 along edge
        p0 = edge.valueAt(edge.getParameterByLength(L0))
        p1 = edge.valueAt(edge.getParameterByLength(L1))
        # Adjust points based on proximity to target depth
        diff0 = p0.z - zMin
        diff1 = p1.z - zMin
        if diff0 < diff1:
            L1 -= adj
        elif diff0 > diff1:
            L0 += adj
        else:
            L0 += adj
            L1 -= adj
        cnt += 1
    midLen = (L0 + L1) / 2.0
    return edge.valueAt(edge.getParameterByLength(midLen))


def _findHighestPointOnEdge(edge):
    tol = 0.0000001
    zMax = edge.BoundBox.ZMax
    # Test first vertex
    v = edge.Vertexes[0]
    if abs(zMax - v.Z) < tol:
        return FreeCAD.Vector(v.X, v.Y, v.Z)
    # Test second vertex
    v = edge.Vertexes[1]
    if abs(zMax - v.Z) < tol:
        return FreeCAD.Vector(v.X, v.Y, v.Z)
    # Test middle point of edge
    eMidLen = edge.Length / 2.0
    eMidPnt = edge.valueAt(edge.getParameterByLength(eMidLen))
    if abs(zMax - eMidPnt.z) < tol:
        return eMidPnt
    if edge.BoundBox.ZLength < 0.000000001:  # roughly horizontal edge
        return eMidPnt
    return _findHighestEdgePoint(E)


def _findHighestEdgePoint(edge):
    zMax = edge.BoundBox.ZMax
    eLen = edge.Length
    L0 = 0
    L1 = eLen
    p0 = None
    p1 = None
    cnt = 0
    while L1 - L0 > 0.00001 and cnt < 2000:
        adj = (L1 - L0) * 0.1
        # Get points at L0 and L1 along edge
        p0 = edge.valueAt(edge.getParameterByLength(L0))
        p1 = edge.valueAt(edge.getParameterByLength(L1))
        # Adjust points based on proximity to target depth
        diff0 = zMax - p0.z
        diff1 = zMax - p1.z
        if diff0 < diff1:
            L1 -= adj
        elif diff0 > diff1:
            L0 += adj
        else:
            L0 += adj
            L1 -= adj
        cnt += 1
    midLen = (L0 + L1) / 2.0
    return edge.valueAt(edge.getParameterByLength(midLen))


def _getBottomEdge(shape):
    EDGES = list()
    # Determine if selected face has a single bottom horizontal edge
    eCnt = len(shape.Edges)
    eZMin = shape.BoundBox.ZMin
    for ei in range(0, eCnt):
        E = shape.Edges[ei]
        if abs(E.BoundBox.ZMax - eZMin) < 0.00000001:
            EDGES.append(E)
    if len(EDGES) == 1:  # single bottom horiz. edge
        return EDGES[0]
    return False


def _getVertFaceType(shape):
    wires = list()

    bottomEdge = _getBottomEdge(shape)
    if bottomEdge:
        return ('Edge', bottomEdge)

    # Extract cross-section of face
    extFwd = (shape.BoundBox.ZLength * 2.2) + 10
    extShp = shape.extrude(FreeCAD.Vector(0.0, 0.0, extFwd))
    sliceZ = shape.BoundBox.ZMin + (extFwd / 2.0)
    slcs = extShp.slice(FreeCAD.Vector(0, 0, 1), sliceZ)
    for i in slcs:
        wires.append(i)
    if len(wires) > 0:
        isFace = False
        csWire = wires[0]
        if wires[0].isClosed():
            face = Part.Face(wires[0])
            if face.Area > 0:
                face.translate(FreeCAD.Vector(0.0, 0.0, shape.BoundBox.ZMin - face.BoundBox.ZMin))
                return ('Face', face)
        return ('Wire', wires[0])
    return False


def _getOppMidPoints(same):
    # Find mid-points between ends of equal, oppossing edges
    com1 = same[0].CenterOfMass
    com2 = same[1].CenterOfMass
    p1 = FreeCAD.Vector(com1.x, com1.y, 0.0)
    p2 = FreeCAD.Vector(com2.x, com2.y, 0.0)
    return (p1, p2)


def _extendLineSlot(p1, p2, begExt, endExt):
    if begExt:
        beg = p1.sub(p2)
        beg.normalize()
        beg.multiply(begExt)
        n1 = p1.add(beg)
    else:
        n1 = p1
    if endExt:
        end = p2.sub(p1)
        end.normalize()
        end.multiply(endExt)
        n2 = p2.add(end)
    else:
        n2 = p2
    return (n1, n2)


def _makeOffsetArc(p1, p2, center, newRadius):
    n1 = p1.sub(center).normalize()
    n2 = p2.sub(center).normalize()
    n1.multiply(newRadius)
    n2.multiply(newRadius)
    p1 = n1.add(center)
    p2 = n2.add(center)
    return (p1, p2)


def _extendArcSlot(self, addDebugShp,
                   p1, p2, arcCenter, newRadius, begExt, endExt):
    cancel = True
    n1 = p1
    n2 = p2
    newRadius = self.newRadius
    arcCenter = self.arcCenter

    def getArcChord(length, rads):
        rads = abs(length / newRadius)
        x = newRadius * math.cos(rads)
        y = newRadius * math.sin(rads)
        a = FreeCAD.Vector(newRadius, 0.0, 0.0)
        b = FreeCAD.Vector(x, y, 0.0)
        c = FreeCAD.Vector(0.0, 0.0, 0.0)
        return Part.makeLine(a, b)

    if begExt or endExt:
        cancel = False
    if cancel:
        return (p1, p2)

    # Convert extension to radians
    origin = FreeCAD.Vector(0.0, 0.0, 0.0)
    if begExt:
        # Create arc representing extension
        rads = abs(begExt / newRadius)
        line = getArcChord(begExt, rads)

        rotToRads = _xyToRadians(p1.sub(arcCenter))
        if begExt < 1:
            rotToRads -= rads
        rotToDeg = math.degrees(rotToRads)

        line.rotate(origin, FreeCAD.Vector(0, 0, 1), rotToDeg)
        line.translate(arcCenter)
        addDebugShp(line, 'ExtendStart')
        v1 = line.Vertexes[1]
        if begExt < 1:
            v1 = line.Vertexes[0]
        n1 = FreeCAD.Vector(v1.X, v1.Y, 0.0)

    if endExt:
        # Create arc representing extension
        rads = abs(endExt / newRadius)
        line = getArcChord(endExt, rads)

        rotToRads = _xyToRadians(p2.sub(arcCenter)) - rads
        if endExt < 1:
            rotToRads += rads
        rotToDeg = math.degrees(rotToRads)

        line.rotate(origin, FreeCAD.Vector(0, 0, 1), rotToDeg)
        line.translate(arcCenter)
        addDebugShp(line, 'ExtendEnd')
        v1 = line.Vertexes[0]
        if endExt < 1:
            v1 = line.Vertexes[1]
        n2 = FreeCAD.Vector(v1.X, v1.Y, 0.0)

    return (n1, n2)
# End of independent support methods


# Additional `OperationSlot` class methods relocated here
def _processFeatureForDouble(self, shape, sub, pNum):
    p = None
    dYdX = None
    cat = sub[:4]
    Ref = getattr(self.obj, 'Reference' + str(pNum))
    if cat == 'Face':
        BE = _getBottomEdge(shape)
        if BE:
            self.bottomEdges.append(BE)
        # calculate slope of face
        V0 = shape.Vertexes[0]
        v1 = shape.CenterOfMass
        temp = FreeCAD.Vector(v1.x - V0.X, v1.y - V0.Y, 0.0)
        dYdX = _normalizeVector(temp)

        # Determine normal vector for face
        norm = shape.normalAt(0.0, 0.0)
        # FreeCAD.Console.PrintMessage('{} normal {}.\n'.format(sub, norm))
        if norm.z != 0:
            msg = translate('PathSlot',
                'The selected face is not oriented vertically:')
            FreeCAD.Console.PrintError(msg + ' {}.\n'.format(sub))
            return False

        if Ref == 'Center of Mass':
            comS = shape.CenterOfMass
            p = FreeCAD.Vector(comS.x, comS.y, 0.0)
        elif Ref == 'Center of BoundBox':
            comS = shape.BoundBox.Center
            p = FreeCAD.Vector(comS.x, comS.y, 0.0)
        elif Ref == 'Lowest Point':
            p = _lowestVertexAsVector(shape)
        elif Ref == 'Highest Point':
            p = _highestVertexAsVector(shape)

    elif cat == 'Edge':
        # calculate slope between end vertexes
        v0 = shape.Edges[0].Vertexes[0]
        v1 = shape.Edges[0].Vertexes[1]
        temp = FreeCAD.Vector(v1.X - v0.X, v1.Y - v0.Y, 0.0)
        dYdX = _normalizeVector(temp)

        if Ref == 'Center of Mass':
            comS = shape.CenterOfMass
            p = FreeCAD.Vector(comS.x, comS.y, 0.0)
        elif Ref == 'Center of BoundBox':
            comS = shape.BoundBox.Center
            p = FreeCAD.Vector(comS.x, comS.y, 0.0)
        elif Ref == 'Lowest Point':
            p = _findLowestPointOnEdge(shape)
        elif Ref == 'Highest Point':
            p = _findHighestPointOnEdge(shape)

    elif cat == 'Vert':
        V = shape.Vertexes[0]
        p = FreeCAD.Vector(V.X, V.Y, 0.0)

    if p:
        return (p, dYdX, cat)

    return False


def _lineCollisionCheck(self, p1, p2):
    '''Make simple circle with diameter of tool, at start point.
    Extrude it latterally along path.
    Extrude it vertically.
    Check for collision with model.'''
    # Make path travel of tool as 3D solid.
    rad = self.parent.tool.Diameter / 2.0

    def getPerp(p1, p2, dist):
        toEnd = p2.sub(p1)
        perp = FreeCAD.Vector(-1 * toEnd.y, toEnd.x, 0.0)
        if perp.x == 0 and perp.y == 0:
            return perp
        perp.normalize()
        perp.multiply(dist)
        return perp

    # Make first cylinder
    ce1 = Part.Wire(Part.makeCircle(rad, p1).Edges)
    C1 = Part.Face(ce1)
    zTrans = self.obj.FinalDepth.Value - C1.BoundBox.ZMin
    C1.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
    extFwd = self.obj.StartDepth.Value - self.obj.FinalDepth.Value
    extVect = FreeCAD.Vector(0.0, 0.0, extFwd)
    startShp = C1.extrude(extVect)

    if p2.sub(p1).Length > 0:
        # Make second cylinder
        ce2 = Part.Wire(Part.makeCircle(rad, p2).Edges)
        C2 = Part.Face(ce2)
        zTrans = self.obj.FinalDepth.Value - C2.BoundBox.ZMin
        C2.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
        endShp = C2.extrude(extVect)

        # Make extruded rectangle to connect cylinders
        perp = getPerp(p1, p2, rad)
        v1 = p1.add(perp)
        v2 = p1.sub(perp)
        v3 = p2.sub(perp)
        v4 = p2.add(perp)
        e1 = Part.makeLine(v1, v2)
        e2 = Part.makeLine(v2, v3)
        e3 = Part.makeLine(v3, v4)
        e4 = Part.makeLine(v4, v1)
        edges = Part.__sortEdges__([e1, e2, e3, e4])
        rectFace = Part.Face(Part.Wire(edges))
        zTrans = self.obj.FinalDepth.Value - rectFace.BoundBox.ZMin
        rectFace.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
        boxShp = rectFace.extrude(extVect)

        # Fuse two cylinders and box together
        part1 = startShp.fuse(boxShp)
        pathTravel = part1.fuse(endShp)
    else:
        pathTravel = startShp

    self.parent._addDebugObject(pathTravel, 'PathTravel')

    # Check for collision with model
    try:
        cmn = self._base.Shape.common(pathTravel)
        if cmn.Volume > 0.000001:
            return True
    except Exception:
        PathLog.debug('Failed to complete path collision check.')

    return False


def _arcCollisionCheck(self, p1, p2, arcCenter, arcRadius):
    '''Make simple circle with diameter of tool, at start and end points.
    Make arch face between circles. Fuse and extrude it vertically.
    Check for collision with model.'''
    # Make path travel of tool as 3D solid.
    rad = self.parent.tool.Diameter / 2.0
    extFwd = self.obj.StartDepth.Value - self.obj.FinalDepth.Value
    extVect = FreeCAD.Vector(0.0, 0.0, extFwd)

    if self.isArc == 1:
        # full circular slot
        # make outer circle
        oCircle = Part.makeCircle(arcRadius + rad, arcCenter)
        oWire = Part.Wire(oCircle.Edges[0])
        outer = Part.Face(oWire)
        # make inner circle
        iRadius = arcRadius - rad
        if iRadius > 0:
            iCircle = Part.makeCircle(iRadius, arcCenter)
            iWire = Part.Wire(iCircle.Edges[0])
            inner = Part.Face(iWire)
            # Cut outer with inner
            path = outer.cut(inner)
        else:
            path = outer
        zTrans = self.obj.FinalDepth.Value - path.BoundBox.ZMin
        path.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
        pathTravel = path.extrude(extVect)
    else:
        # arc slot
        # Make first cylinder
        ce1 = Part.Wire(Part.makeCircle(rad, p1).Edges)
        C1 = Part.Face(ce1)
        zTrans = self.obj.FinalDepth.Value - C1.BoundBox.ZMin
        C1.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
        startShp = C1.extrude(extVect)
        # self.parent._addDebugObject(startShp, 'StartCyl')

        # Make second cylinder
        ce2 = Part.Wire(Part.makeCircle(rad, p2).Edges)
        C2 = Part.Face(ce2)
        zTrans = self.obj.FinalDepth.Value - C2.BoundBox.ZMin
        C2.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
        endShp = C2.extrude(extVect)
        # self.parent._addDebugObject(endShp, 'EndCyl')

        # Make wire with inside and outside arcs, and lines on ends.
        # Convert wire to face, then extrude
        import draftgeoutils.arcs as Arcs
        # Arc 1 - inside
        # verify offset does not force radius < 0
        newRadius = arcRadius - rad
        # PathLog.debug('arcRadius, newRadius: {}, {}'.format(arcRadius, newRadius))
        if newRadius <= 0:
            msg = translate('PathSlot',
                    'Current offset value is not possible.')
            FreeCAD.Console.PrintError(msg + '\n')
            return False
        else:
            (pA, pB) = _makeOffsetArc(p1, p2, arcCenter, newRadius)
            arc_inside = Arcs.arcFrom2Pts(pA, pB, arcCenter)

        # Arc 2 - outside
        # verify offset does not force radius < 0
        newRadius = arcRadius + rad
        # PathLog.debug('arcRadius, newRadius: {}, {}'.format(arcRadius, newRadius))
        if newRadius <= 0:
            msg = translate('PathSlot',
                    'Current offset value is not possible.')
            FreeCAD.Console.PrintError(msg + '\n')
            return False
        else:
            (pC, pD) = _makeOffsetArc(p1, p2, arcCenter, newRadius)
            arc_outside = Arcs.arcFrom2Pts(pC, pD, arcCenter)

        # Make end lines to connect arcs
        vA = arc_inside.Vertexes[0]
        vB = arc_inside.Vertexes[1]
        vC = arc_outside.Vertexes[1]
        vD = arc_outside.Vertexes[0]
        pa = FreeCAD.Vector(vA.X, vA.Y, 0.0)
        pb = FreeCAD.Vector(vB.X, vB.Y, 0.0)
        pc = FreeCAD.Vector(vC.X, vC.Y, 0.0)
        pd = FreeCAD.Vector(vD.X, vD.Y, 0.0)

        # Make closed arch face and extrude
        e1 = Part.makeLine(pb, pc)
        e2 = Part.makeLine(pd, pa)
        edges = Part.__sortEdges__([arc_inside, e1, arc_outside, e2])
        rectFace = Part.Face(Part.Wire(edges))
        zTrans = self.obj.FinalDepth.Value - rectFace.BoundBox.ZMin
        rectFace.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
        boxShp = rectFace.extrude(extVect)
        # self.parent._addDebugObject(boxShp, 'ArcBox')

        # Fuse two cylinders and box together
        part1 = startShp.fuse(boxShp)
        pathTravel = part1.fuse(endShp)

    self.parent._addDebugObject(pathTravel, 'PathTravel')

    # Check for collision with model
    try:
        cmn = self._base.Shape.common(pathTravel)
        if cmn.Volume > 0.000001:
            return True
    except Exception:
        PathLog.debug('Failed to complete path collision check.')

    return False
# Eclass methods
