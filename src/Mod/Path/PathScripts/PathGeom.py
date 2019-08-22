# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
import Part
import Path
import PathScripts.PathLog as PathLog
import math

from FreeCAD import Vector
from PySide import QtCore

__title__ = "PathGeom - geometry utilities for Path"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Functions to extract and convert between Path.Command and Part.Edge and utility functions to reason about them."

Tolerance = 0.000001

LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class Side:
    """Class to determine and define the side a Path is on, or Vectors are in relation to each other."""
    Left  = +1
    Right = -1
    Straight = 0
    On = 0

    @classmethod
    def toString(cls, side):
        """toString(side)
        Returns a string representation of the enum value."""
        if side == cls.Left:
            return 'Left'
        if side == cls.Right:
            return 'Right'
        return 'On'

    @classmethod
    def of(cls, ptRef, pt):
        """of(ptRef, pt)
        Determine the side of pt in relation to ptRef.
        If both Points are viewed as vectors with their origin in (0,0,0)
        then the two vectors either form a straight line (On) or pt
        lies in the left or right hemisphere in regards to ptRef."""
        d = -ptRef.x*pt.y + ptRef.y*pt.x
        if d < 0:
            return cls.Left
        if d > 0:
            return cls.Right
        return cls.Straight

CmdMoveRapid    = ['G0', 'G00']
CmdMoveStraight = ['G1', 'G01']
CmdMoveCW       = ['G2', 'G02']
CmdMoveCCW      = ['G3', 'G03']
CmdMoveArc      = CmdMoveCW + CmdMoveCCW
CmdMove         = CmdMoveStraight + CmdMoveArc

def isRoughly(float1, float2, error=Tolerance):
    """isRoughly(float1, float2, [error=Tolerance])
    Returns true if the two values are the same within a given error."""
    return math.fabs(float1 - float2) <= error

def pointsCoincide(p1, p2, error=Tolerance):
    """pointsCoincide(p1, p2, [error=Tolerance])
    Return True if two points are roughly identical (see also isRoughly)."""
    return isRoughly(p1.x, p2.x, error) and isRoughly(p1.y, p2.y, error) and isRoughly(p1.z, p2.z, error)

def edgesMatch(e0, e1, error=Tolerance):
    """edgesMatch(e0, e1, [error=Tolerance]
    Return true if the edges start and end at the same point and have the same type of curve."""
    if type(e0.Curve) != type(e1.Curve) or len(e0.Vertexes) != len(e1.Vertexes):
        return False
    return all(pointsCoincide(e0.Vertexes[i].Point, e1.Vertexes[i].Point, error) for i in range(len(e0.Vertexes)))

def edgeConnectsTo(edge, vector, error=Tolerance):
    """edgeConnectsTop(edge, vector, error=Tolerance)
    Returns True if edge connects to given vector."""
    return pointsCoincide(edge.valueAt(edge.FirstParameter), vector, error) or pointsCoincide(edge.valueAt(edge.LastParameter), vector, error)

def getAngle(vector):
    """getAngle(vector)
    Returns the angle [-pi,pi] of a vector using the X-axis as the reference.
    Positive angles for vertexes in the upper hemisphere (positive y values)
    and negative angles for the lower hemisphere."""
    a = vector.getAngle(Vector(1,0,0))
    if vector.y < 0:
        return -a
    return a

def diffAngle(a1, a2, direction = 'CW'):
    """diffAngle(a1, a2, [direction='CW'])
    Returns the difference between two angles (a1 -> a2) into a given direction."""
    if direction == 'CW':
        while a1 < a2:
            a1 += 2*math.pi
        a = a1 - a2
    else:
        while a2 < a1:
            a2 += 2*math.pi
        a = a2 - a1
    return a

def isVertical(obj):
    '''isVertical(obj) ... answer True if obj points into Z'''
    if type(obj) == FreeCAD.Vector:
        return isRoughly(obj.x, 0) and isRoughly(obj.y, 0)

    if obj.ShapeType == 'Face':
        if type(obj.Surface) == Part.Plane:
            return isHorizontal(obj.Surface.Axis)
        if type(obj.Surface) == Part.Cylinder or type(obj.Surface) == Part.Cone:
            return isVertical(obj.Surface.Axis)
        if type(obj.Surface) == Part.Sphere:
            return True
        if type(obj.Surface) == Part.SurfaceOfExtrusion:
            return isVertical(obj.Surface.Direction)
        if type(obj.Surface) == Part.SurfaceOfRevolution:
            return isHorizontal(obj.Surface.Direction)
        if type(obj.Surface) != Part.BSplineSurface:
            PathLog.info(translate('PathGeom', "face %s not handled, assuming not vertical") % type(obj.Surface))
        return None

    if obj.ShapeType == 'Edge':
        if type(obj.Curve) == Part.Line or type(obj.Curve) == Part.LineSegment:
            return isVertical(obj.Vertexes[1].Point - obj.Vertexes[0].Point)
        if type(obj.Curve) == Part.Circle or type(obj.Curve) == Part.Ellipse: # or type(obj.Curve) == Part.BSplineCurve:
            return isHorizontal(obj.Curve.Axis)
        if type(obj.Curve) == Part.BezierCurve:
            # the current assumption is that a bezier curve is vertical if its end points are vertical
            return isVertical(obj.Curve.EndPoint - obj.Curve.StartPoint)
        if type(obj.Curve) != Part.BSplineCurve:
            PathLog.info(translate('PathGeom', "edge %s not handled, assuming not vertical") % type(obj.Curve))
        return None

    PathLog.error(translate('PathGeom', "isVertical(%s) not supported") % obj)
    return None

def isHorizontal(obj):
    '''isHorizontal(obj) ... answer True if obj points into X or Y'''
    if type(obj) == FreeCAD.Vector:
        return isRoughly(obj.z, 0)

    if obj.ShapeType == 'Face':
        if type(obj.Surface) == Part.Plane:
            return isVertical(obj.Surface.Axis)
        if type(obj.Surface) == Part.Cylinder or type(obj.Surface) == Part.Cone:
            return isHorizontal(obj.Surface.Axis)
        if type(obj.Surface) == Part.Sphere:
            return True
        if type(obj.Surface) == Part.SurfaceOfExtrusion:
            return isHorizontal(obj.Surface.Direction)
        if type(obj.Surface) == Part.SurfaceOfRevolution:
            return isVertical(obj.Surface.Direction)
        return isRoughly(obj.BoundBox.ZLength, 0.0)

    if obj.ShapeType == 'Edge':
        if type(obj.Curve) == Part.Line or type(obj.Curve) == Part.LineSegment:
            return isHorizontal(obj.Vertexes[1].Point - obj.Vertexes[0].Point)
        if type(obj.Curve) == Part.Circle or type(obj.Curve) == Part.Ellipse: # or type(obj.Curve) == Part.BSplineCurve:
            return isVertical(obj.Curve.Axis)
        return isRoughly(obj.BoundBox.ZLength, 0.0)

    PathLog.error(translate('PathGeom', "isHorizontal(%s) not supported") % obj)
    return None


def commandEndPoint(cmd, defaultPoint = Vector(), X='X', Y='Y', Z='Z'):
    """commandEndPoint(cmd, [defaultPoint=Vector()], [X='X'], [Y='Y'], [Z='Z'])
    Extracts the end point from a Path Command."""
    x = cmd.Parameters.get(X, defaultPoint.x)
    y = cmd.Parameters.get(Y, defaultPoint.y)
    z = cmd.Parameters.get(Z, defaultPoint.z)
    return Vector(x, y, z)

def xy(point):
    """xy(point)
    Convenience function to return the projection of the Vector in the XY-plane."""
    return Vector(point.x, point.y, 0)

def speedBetweenPoints(p0, p1, hSpeed, vSpeed):
    if isRoughly(hSpeed, vSpeed):
        return hSpeed

    d = p1 - p0
    if isRoughly(0.0, d.z):
        return hSpeed
    if isRoughly(0.0, d.x) and isRoughly(0.0, d.y):
        return vSpeed
    # need to interpolate between hSpeed and vSpeed depending on the pitch
    pitch = 2 * math.atan2(xy(d).Length, math.fabs(d.z)) / math.pi
    while pitch < 0:
        pitch = pitch + 1
    while pitch > 1:
        pitch = pitch - 1
    print("  pitch = %g %g (%.2f, %.2f, %.2f) -> %.2f" % (pitch, math.atan2(xy(d).Length, d.z), d.x, d.y, d.z, xy(d).Length))
    speed = vSpeed + pitch * (hSpeed - vSpeed)
    if speed > hSpeed and speed > vSpeed:
        return max(hSpeed, vSpeed)
    if speed < hSpeed and speed < vSpeed:
        return min(hSpeed, vSpeed)
    return speed

def cmdsForEdge(edge, flip = False, useHelixForBSpline = True, segm = 50, hSpeed = 0, vSpeed = 0):
    """cmdsForEdge(edge, flip=False, useHelixForBSpline=True, segm=50) -> List(Path.Command)
    Returns a list of Path.Command representing the given edge.
    If flip is True the edge is considered to be backwards.
    If useHelixForBSpline is True an Edge based on a BSplineCurve is considered
    to represent a helix and results in G2 or G3 command. Otherwise edge has
    no direct Path.Command mapping and will be approximated by straight segments.
    segm is a factor for the segmentation of arbitrary curves not mapped to G1/2/3
    commands. The higher the value the more segments will be used."""
    pt = edge.valueAt(edge.LastParameter) if not flip else edge.valueAt(edge.FirstParameter)
    params = {'X': pt.x, 'Y': pt.y, 'Z': pt.z}
    if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
        if hSpeed > 0 and vSpeed > 0:
            pt2 = edge.valueAt(edge.FirstParameter) if not flip else edge.valueAt(edge.LastParameter)
            params.update({'F': speedBetweenPoints(pt, pt2, hSpeed, vSpeed)})
        commands =  [Path.Command('G1', params)]
    else:
        p1 = edge.valueAt(edge.FirstParameter) if not flip else edge.valueAt(edge.LastParameter)
        p2 = edge.valueAt((edge.FirstParameter + edge.LastParameter)/2)
        p3 = pt

        if (type(edge.Curve) == Part.Circle and isRoughly(edge.Curve.Axis.x, 0) and isRoughly(edge.Curve.Axis.y, 0)) or (useHelixForBSpline and type(edge.Curve) == Part.BSplineCurve):
            # This is an arc or a helix and it should be represented by a simple G2/G3 command
            if edge.Curve.Axis.z < 0:
                cmd = 'G2' if not flip else 'G3'
            else:
                cmd = 'G3' if not flip else 'G2'

            if pointsCoincide(p1, p3):
                # A full circle
                offset = edge.Curve.Center - pt
            else:
                pd = Part.Circle(xy(p1), xy(p2), xy(p3)).Center
                PathLog.debug("**** %s.%d: (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f) -> center=(%.2f, %.2f)" % (cmd, flip, p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, p3.x, p3.y, p3.z, pd.x, pd.y))

                # Have to calculate the center in the XY plane, using pd leads to an error if this is a helix
                pa = xy(p1)
                pb = xy(p2)
                pc = xy(p3)
                offset = Part.Circle(pa, pb, pc).Center - pa

                PathLog.debug("**** (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)" % (pa.x, pa.y, pa.z, pc.x, pc.y, pc.z))
                PathLog.debug("**** (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)" % (pb.x, pb.y, pb.z, pd.x, pd.y, pd.z))
            PathLog.debug("**** (%.2f, %.2f, %.2f)" % (offset.x, offset.y, offset.z))

            params.update({'I': offset.x, 'J': offset.y, 'K': (p3.z - p1.z)/2})
            # G2/G3 commands are always performed at hSpeed
            if hSpeed > 0:
                params.update({'F': hSpeed})
            commands = [ Path.Command(cmd, params) ]

        else:
            # We're dealing with a helix or a more complex shape and it has to get approximated
            # by a number of straight segments
            eStraight = Part.Edge(Part.LineSegment(p1, p3))
            esP2 = eStraight.valueAt((eStraight.FirstParameter + eStraight.LastParameter)/2)
            deviation = (p2 - esP2).Length
            if isRoughly(deviation, 0):
                return [ Path.Command('G1', {'X': p3.x, 'Y': p3.y, 'Z': p3.z}) ]
            # at this point pixellation is all we can do
            commands = []
            segments = int(math.ceil((deviation / eStraight.Length) * segm))
            #print("**** pixellation with %d segments" % segments)
            dParameter = (edge.LastParameter - edge.FirstParameter) / segments
            # starting point
            p0 = edge.valueAt(edge.LastParameter) if flip else edge.valueAt(edge.FirstParameter)
            for i in range(0, segments):
                if flip:
                    p = edge.valueAt(edge.LastParameter - (i + 1) * dParameter)
                else:
                    p = edge.valueAt(edge.FirstParameter + (i + 1) * dParameter)
                if hSpeed > 0 and vSpeed > 0:
                    params.update({'F': speedBetweenPoints(p0, p, hSpeed, vSpeed)})
                cmd = Path.Command('G1', {'X': p.x, 'Y': p.y, 'Z': p.z})
                #print("***** %s" % cmd)
                commands.append(cmd)
                p0 = p
    #print commands
    return commands

def edgeForCmd(cmd, startPoint):
    """edgeForCmd(cmd, startPoint).
    Returns an Edge representing the given command, assuming a given startPoint."""

    endPoint = commandEndPoint(cmd, startPoint)
    if (cmd.Name in CmdMoveStraight) or (cmd.Name in CmdMoveRapid):
        if pointsCoincide(startPoint, endPoint):
            return None
        return Part.Edge(Part.LineSegment(startPoint, endPoint))

    if cmd.Name in CmdMoveArc:
        center = startPoint + commandEndPoint(cmd, Vector(0,0,0), 'I', 'J', 'K')
        A = xy(startPoint - center)
        B = xy(endPoint - center)
        d = -B.x * A.y + B.y * A.x

        if isRoughly(d, 0, 0.005):
            PathLog.debug("Half circle arc at: (%.2f, %.2f, %.2f)" % (center.x, center.y, center.z))
            # we're dealing with half a circle here
            angle = getAngle(A) + math.pi/2
            if cmd.Name in CmdMoveCW:
                angle -= math.pi
        else:
            C = A + B
            angle = getAngle(C)
            PathLog.debug("Arc (%8f) at: (%.2f, %.2f, %.2f) -> angle=%f" % (d, center.x, center.y, center.z, angle / math.pi))

        R = A.Length
        PathLog.debug("arc: p1=(%.2f, %.2f) p2=(%.2f, %.2f) -> center=(%.2f, %.2f)" % (startPoint.x, startPoint.y, endPoint.x, endPoint.y, center.x, center.y))
        PathLog.debug("arc: A=(%.2f, %.2f) B=(%.2f, %.2f) -> d=%.2f" % (A.x, A.y, B.x, B.y, d))
        PathLog.debug("arc: R=%.2f angle=%.2f" % (R, angle/math.pi))
        if isRoughly(startPoint.z, endPoint.z):
            midPoint = center + Vector(math.cos(angle), math.sin(angle), 0) * R
            PathLog.debug("arc: (%.2f, %.2f) -> (%.2f, %.2f) -> (%.2f, %.2f)" % (startPoint.x, startPoint.y, midPoint.x, midPoint.y, endPoint.x, endPoint.y))
            return Part.Edge(Part.Arc(startPoint, midPoint, endPoint))

        # It's a Helix
        #print('angle: A=%.2f B=%.2f' % (getAngle(A)/math.pi, getAngle(B)/math.pi))
        if cmd.Name in CmdMoveCW:
            cw = True
        else:
            cw = False
        angle = diffAngle(getAngle(A), getAngle(B), 'CW' if cw else 'CCW')
        height = endPoint.z - startPoint.z
        pitch = height * math.fabs(2 * math.pi / angle)
        if angle > 0:
            cw = not cw
        #print("Helix: R=%.2f h=%.2f angle=%.2f pitch=%.2f" % (R, height, angle/math.pi, pitch))
        helix = Part.makeHelix(pitch, height, R, 0, not cw)
        helix.rotate(Vector(), Vector(0,0,1), 180 * getAngle(A) / math.pi)
        e = helix.Edges[0]
        helix.translate(startPoint - e.valueAt(e.FirstParameter))
        return helix.Edges[0]
    return None

def wireForPath(path, startPoint = Vector(0, 0, 0)):
    """wireForPath(path, [startPoint=Vector(0,0,0)])
    Returns a wire representing all move commands found in the given path."""
    edges = []
    rapid = []
    if hasattr(path, "Commands"):
        for cmd in path.Commands:
            edge = edgeForCmd(cmd, startPoint)
            if edge:
                if cmd.Name in CmdMoveRapid:
                    rapid.append(edge)
                edges.append(edge)
                startPoint = commandEndPoint(cmd, startPoint)
    return (Part.Wire(edges), rapid)

def wiresForPath(path, startPoint = Vector(0, 0, 0)):
    """wiresForPath(path, [startPoint=Vector(0,0,0)])
    Returns a collection of wires, each representing a continuous cutting Path in path."""
    wires = []
    if hasattr(path, "Commands"):
        edges = []
        for cmd in path.Commands:
            if cmd.Name in CmdMove:
                edges.append(edgeForCmd(cmd, startPoint))
                startPoint = commandEndPoint(cmd, startPoint)
            elif cmd.Name in CmdMoveRapid:
                wires.append(Part.Wire(edges))
                edges = []
                startPoint = commandEndPoint(cmd, startPoint)
        if edges:
            wires.append(Part.Wire(edges))
    return wires

def arcToHelix(edge, z0, z1):
    """arcToHelix(edge, z0, z1)
    Assuming edge is an arc it'll return a helix matching the arc starting at z0 and rising/falling to z1."""


    p1 = edge.valueAt(edge.FirstParameter)
    # p2 = edge.valueAt(edge.LastParameter)

    cmd = cmdsForEdge(edge)[0]
    params = cmd.Parameters
    params.update({'Z': z1, 'K': (z1 - z0)/2})
    command = Path.Command(cmd.Name, params)

    #print("- (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f): %.2f:%.2f" % (edge.Vertexes[0].X, edge.Vertexes[0].Y, edge.Vertexes[0].Z, edge.Vertexes[1].X, edge.Vertexes[1].Y, edge.Vertexes[1].Z, z0, z1))
    #print("- %s -> %s" % (cmd, command))

    return edgeForCmd(command, Vector(p1.x, p1.y, z0))


def helixToArc(edge, z = 0):
    """helixToArc(edge, z=0)
    Returns the projection of the helix onto the XY-plane with a given offset."""
    p1 = edge.valueAt(edge.FirstParameter)
    p2 = edge.valueAt((edge.FirstParameter + edge.LastParameter)/2)
    p3 = edge.valueAt(edge.LastParameter)
    p01 = Vector(p1.x, p1.y, z)
    p02 = Vector(p2.x, p2.y, z)
    p03 = Vector(p3.x, p3.y, z)
    return Part.Edge(Part.Arc(p01, p02, p03))

def splitArcAt(edge, pt):
    """splitArcAt(edge, pt)
    Returns a list of 2 edges which together form the original arc split at the given point.
    The Vector pt has to represent a point on the given arc."""
    p1 = edge.valueAt(edge.FirstParameter)
    p2 = pt
    p3 = edge.valueAt(edge.LastParameter)
    edges = []

    p = edge.Curve.parameter(p2)
    #print("splitArcAt(%.2f, %.2f, %.2f): %.2f - %.2f - %.2f" % (pt.x, pt.y, pt.z, edge.FirstParameter, p, edge.LastParameter))

    p12 = edge.Curve.value((edge.FirstParameter + p)/2)
    p23 = edge.Curve.value((p + edge.LastParameter)/2)
    #print("splitArcAt: p12=(%.2f, %.2f, %.2f) p23=(%.2f, %.2f, %.2f)" % (p12.x, p12.y, p12.z, p23.x, p23.y, p23.z))

    edges.append(Part.Edge(Part.Arc(p1, p12, p2)))
    edges.append(Part.Edge(Part.Arc(p2, p23, p3)))

    return edges

def splitEdgeAt(edge, pt):
    """splitEdgeAt(edge, pt)
    Returns a list of 2 edges, forming the original edge split at the given point.
    The results are undefined if the Vector representing the point is not part of the edge."""
    # I could not get the OCC parameterAt and split to work ...
    # pt HAS to be on the edge, otherwise the results are undefined
    p1 = edge.valueAt(edge.FirstParameter)
    p2 = pt
    p3 = edge.valueAt(edge.LastParameter)
    # edges = []

    if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
        # it's a line
        return [Part.Edge(Part.LineSegment(p1, p2)), Part.Edge(Part.LineSegment(p2, p3))]
    elif type(edge.Curve) == Part.Circle:
        # it's an arc
        return splitArcAt(edge, pt)
    else:
        # it's a helix
        arc = helixToArc(edge, 0)
        aes = splitArcAt(arc, Vector(pt.x, pt.y, 0))
        return [arcToHelix(aes[0], p1.z, p2.z), arcToHelix(aes[1], p2.z, p3.z)]

def combineConnectedShapes(shapes):
    done = False
    while not done:
        done = True
        combined = []
        PathLog.debug("shapes: {}".format(shapes))
        for shape in shapes:
            connected = [f for f in combined if isRoughly(shape.distToShape(f)[0], 0.0)]
            PathLog.debug("  {}: connected: {} dist: {}".format(len(combined), connected, [shape.distToShape(f)[0] for f in combined]))
            if connected:
                combined = [f for f in combined if f not in connected]
                connected.append(shape)
                combined.append(Part.makeCompound(connected))
                done = False
            else:
                combined.append(shape)
        shapes = combined
    return shapes

def removeDuplicateEdges(wire):
    unique = []
    for e in wire.Edges:
        if not any(edgesMatch(e, u) for u in unique):
            unique.append(e)
    return Part.Wire(unique)

OddsAndEnds = []

def flipEdge(edge):
    '''flipEdge(edge)
    Flips given edge around so the new Vertexes[0] was the old Vertexes[-1] and vice versa, without changing the shape.
    Currently only lines, line segments, circles and arcs are supported.'''

    if Part.Line == type(edge.Curve) and not edge.Vertexes:
        return Part.Edge(Part.Line(edge.valueAt(edge.LastParameter), edge.valueAt(edge.FirstParameter)))
    elif Part.Line == type(edge.Curve) or Part.LineSegment == type(edge.Curve):
        return Part.Edge(Part.LineSegment(edge.Vertexes[-1].Point, edge.Vertexes[0].Point))
    elif Part.Circle == type(edge.Curve):
        # Create an inverted circle
        circle = Part.Circle(edge.Curve.Center, -edge.Curve.Axis, edge.Curve.Radius)
        # Rotate the circle appropriately so it starts at edge.valueAt(edge.LastParameter)
        circle.rotate(FreeCAD.Placement(circle.Center, circle.Axis, 180 - math.degrees(edge.LastParameter + edge.Curve.AngleXU)))
        # Now the edge always starts at 0 and LastParameter is the value range
        arc = Part.Edge(circle, 0, edge.LastParameter - edge.FirstParameter)
        return arc
    elif Part.BSplineCurve == type(edge.Curve):
        spline = edge.Curve

        mults = spline.getMultiplicities()
        weights = spline.getWeights()
        knots = spline.getKnots()
        poles = spline.getPoles()
        perio = spline.isPeriodic()
        ratio = spline.isRational()
        degree = spline.Degree

        ma = max(knots)
        mi = min(knots)
        knots = [ma+mi-k for k in knots]

        mults.reverse()
        weights.reverse()
        poles.reverse()
        knots.reverse()

        flipped = Part.BSplineCurve()
        flipped.buildFromPolesMultsKnots(poles, mults , knots, perio, degree, weights, ratio)

        return Part.Edge(flipped)

    global OddsAndEnds # pylint: disable=global-statement
    OddsAndEnds.append(edge)
    PathLog.warning(translate('PathGeom', "%s not support for flipping") % type(edge.Curve))

def flipWire(wire):
    '''Flip the entire wire and all its edges so it is being processed the other way around.'''
    edges = [flipEdge(e) for e in wire.Edges]
    edges.reverse()
    return Part.Wire(edges)
