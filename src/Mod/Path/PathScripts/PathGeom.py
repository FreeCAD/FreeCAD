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
import math
import Part
import Path

from FreeCAD import Vector

class Side:
    """Class to determine and define the side a Path is on, or Vectors are in relation to each other."""
    Left  = +1
    Right = -1
    Straight = 0
    On = 0

    @classmethod
    def toString(cls, side):
        """(side)
        Returns a string representation of the enum value."""
        if side == cls.Left:
            return 'Left'
        if side == cls.Right:
            return 'Right'
        return 'On'

    @classmethod
    def of(cls, ptRef, pt):
        """(ptRef, pt)
        Determine the side of pt in relation to ptRef.
        If both Points are viewed as vectors with their origin in (0,0,0)
        then the two vectors are either form a straigt line (On) or pt
        lies in the left or right hemishpere in regards to ptRef."""
        d = -ptRef.x*pt.y + ptRef.y*pt.x
        if d < 0:
            return cls.Left
        if d > 0:
            return cls.Right
        return cls.Straight

class PathGeom:
    """Class to transform Path Commands into Edges and Wire and back again.
    The interface might eventuallly become part of Path itself."""
    CmdMoveFast     = ['G0', 'G00']
    CmdMoveStraight = ['G1', 'G01']
    CmdMoveCW       = ['G2', 'G02']
    CmdMoveCCW      = ['G3', 'G03']
    CmdMoveArc      = CmdMoveCW + CmdMoveCCW
    CmdMove         = CmdMoveStraight + CmdMoveArc

    @classmethod
    def isRoughly(cls, float1, float2, error=0.0000001):
        """(float1, float2, [error=0.0000001])
        Returns true if the two values are the same within a given error."""
        return math.fabs(float1 - float2) <= error

    @classmethod
    def pointsCoincide(cls, p1, p2, error=0.0000001):
        """(p1, p2, [error=0.0000001])
        Return True if two points are roughly identical (see also isRoughly)."""
        return cls.isRoughly(p1.x, p2.x, error) and cls.isRoughly(p1.y, p2.y, error) and cls.isRoughly(p1.z, p2.z, error)

    @classmethod
    def edgeConnectsTo(cls, edge, vector):
        """(edge, vector)
        Returns True if edge connects to given vector."""
        return cls.pointsCoincide(edge.valueAt(edge.FirstParameter), vector) or cls.pointsCoincide(edge.valueAt(edge.LastParameter), vector)

    @classmethod
    def getAngle(cls, vector):
        """(vector)
        Returns the angle [-pi,pi] of a vector using the X-axis as the reference.
        Positive angles for vertexes in the upper hemishpere (positive y values)
        and negative angles for the lower hemishpere."""
        a = vector.getAngle(FreeCAD.Vector(1,0,0))
        if vector.y < 0:
            return -a
        return a

    @classmethod
    def diffAngle(cls, a1, a2, direction = 'CW'):
        """(a1, a2, [direction='CW'])
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

    @classmethod
    def commandEndPoint(cls, cmd, defaultPoint = Vector(), X='X', Y='Y', Z='Z'):
        """(cmd, [defaultPoint=Vector()], [X='X'], [Y='Y'], [Z='Z'])
        Extracts the end point from a Path Command."""
        x = cmd.Parameters.get(X, defaultPoint.x)
        y = cmd.Parameters.get(Y, defaultPoint.y)
        z = cmd.Parameters.get(Z, defaultPoint.z)
        return FreeCAD.Vector(x, y, z)

    @classmethod
    def xy(cls, point):
        """(point)
        Convenience function to return the projection of the Vector in the XY-plane."""
        return Vector(point.x, point.y, 0)

    @classmethod
    def cmdsForEdge(cls, edge, flip = False, useHelixForBSpline = True):
        """(edge, flip = False, useHelixForBSpline = True) -> List(Path.Command)
        Returns a list of Path.Command representing the given edge.
        If flip is True the edge is considered to be backwards.
        If useHelixForBSpline is True an Edge based on a BSplineCurve is considered
        to represent a helix and results in G2 or G3 command. Otherwise edge has
        no direct Path.Command mapping and will be approximated by straight segments.
        Approximation is also the approach for edges that are neither straight lines
        nor arcs (nor helixes)."""
        pt = edge.valueAt(edge.LastParameter) if not flip else edge.valueAt(edge.FirstParameter)
        params = {'X': pt.x, 'Y': pt.y, 'Z': pt.z}
        if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
            commands =  [Path.Command('G1', params)]
        else:
            if not flip:
                p1 = edge.valueAt(edge.FirstParameter)
                p3 = pt
            else:
                p1 = pt
                p3 = edge.valueAt(edge.LastParameter)
            p2 = edge.valueAt((edge.FirstParameter + edge.LastParameter)/2)
            if type(edge.Curve) == Part.Circle or (useHelixForBSpline and type(edge.Curve) == Part.BSplineCurve):
                if Side.Left == Side.of(p2 - p1, p3 - p2):
                    cmd = 'G3'
                else:
                    cmd = 'G2'
                #print("**** (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)" % (p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, p3.x, p3.y, p3.z))
                pd = Part.Circle(PathGeom.xy(p1), PathGeom.xy(p2), PathGeom.xy(p3)).Center

                pa = PathGeom.xy(p1)
                pb = PathGeom.xy(p2)
                pc = PathGeom.xy(p3)
                #print("**** (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)" % (pa.x, pa.y, pa.z, pc.x, pc.y, pc.z))
                #print("**** (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)" % (pb.x, pb.y, pb.z, pd.x, pd.y, pd.z))
                offset = Part.Circle(PathGeom.xy(p1), PathGeom.xy(p2), PathGeom.xy(p3)).Center - p1
                #print("**** (%.2f, %.2f, %.2f)" % (offset.x, offset.y, offset.z))
                params.update({'I': offset.x, 'J': offset.y, 'K': (p3.z - p1.z)/2})
                commands = [ Path.Command(cmd, params) ]
            else:
                eStraight = Part.Edge(Part.LineSegment(p1, p3))
                esP2 = eStraight.valueAt((eStraight.FirstParameter + eStraight.LastParameter)/2)
                deviation = (p2 - esP2).Length
                if cls.isRoughly(deviation, 0):
                    return [ Path.Command('G1', {'X': p3.x, 'Y': p3.y, 'Z': p3.z}) ]
                # at this point pixellation is all we can do
                commands = []
                segments = int(math.ceil((deviation / eStraight.Length) * 1000))
                #print("**** pixellation with %d segments" % segments)
                dParameter = (edge.LastParameter - edge.FirstParameter) / segments
                for i in range(0, segments):
                    if flip:
                        p = edge.valueAt(edge.LastParameter - (i + 1) * dParameter)
                    else:
                        p = edge.valueAt(edge.FirstParameter + (i + 1) * dParameter)
                    cmd = Path.Command('G1', {'X': p.x, 'Y': p.y, 'Z': p.z})
                    #print("***** %s" % cmd)
                    commands.append(cmd)
        #print commands
        return commands

    @classmethod
    def edgeForCmd(cls, cmd, startPoint):
        """(cmd, startPoint).
        Returns an Edge representing the given command, assuming a given startPoint."""

        endPoint = cls.commandEndPoint(cmd, startPoint)
        if (cmd.Name in cls.CmdMoveStraight) or (cmd.Name in cls.CmdMoveFast):
            if cls.pointsCoincide(startPoint, endPoint):
                return None
            return Part.Edge(Part.LineSegment(startPoint, endPoint))

        if cmd.Name in cls.CmdMoveArc:
            center = startPoint + cls.commandEndPoint(cmd, Vector(0,0,0), 'I', 'J', 'K')
            A = cls.xy(startPoint - center)
            B = cls.xy(endPoint - center)
            d = -B.x * A.y + B.y * A.x

            if d == 0:
                # we're dealing with half a circle here
                angle = cls.getAngle(A) + math.pi/2
                if cmd.Name in cls.CmdMoveCW:
                    angle -= math.pi
            else:
                C = A + B
                angle = cls.getAngle(C)

            R = A.Length
            #print("arc: p1=(%.2f, %.2f) p2=(%.2f, %.2f) -> center=(%.2f, %.2f)" % (startPoint.x, startPoint.y, endPoint.x, endPoint.y, center.x, center.y))
            #print("arc: A=(%.2f, %.2f) B=(%.2f, %.2f) -> d=%.2f" % (A.x, A.y, B.x, B.y, d))
            #print("arc: R=%.2f angle=%.2f" % (R, angle/math.pi))
            if startPoint.z == endPoint.z:
                midPoint = center + FreeCAD.Vector(math.cos(angle), math.sin(angle), 0) * R
                return Part.Edge(Part.Arc(startPoint, midPoint, endPoint))

            # It's a Helix
            #print('angle: A=%.2f B=%.2f' % (cls.getAngle(A)/math.pi, cls.getAngle(B)/math.pi))
            if cmd.Name in cls.CmdMoveCW:
                cw = True
            else:
                cw = False
            angle = cls.diffAngle(cls.getAngle(A), cls.getAngle(B), 'CW' if cw else 'CCW')
            height = endPoint.z - startPoint.z
            pitch = height * math.fabs(2 * math.pi / angle)
            if angle > 0:
                cw = not cw
            #print("Helix: R=%.2f h=%.2f angle=%.2f pitch=%.2f" % (R, height, angle/math.pi, pitch))
            helix = Part.makeHelix(pitch, height, R, 0, not cw)
            helix.rotate(Vector(), Vector(0,0,1), 180 * cls.getAngle(A) / math.pi)
            e = helix.Edges[0]
            helix.translate(startPoint - e.valueAt(e.FirstParameter))
            return helix.Edges[0]
        return None

    @classmethod
    def wireForPath(cls, path, startPoint = FreeCAD.Vector(0, 0, 0)):
        """(path, [startPoint=Vector(0,0,0)])
        Returns a wire representing all move commands found in the given path."""
        edges = []
        if hasattr(path, "Commands"):
            for cmd in path.Commands:
                edge = cls.edgeForCmd(cmd, startPoint)
                if edge:
                    edges.append(edge)
                    startPoint = cls.commandEndPoint(cmd, startPoint)
        return Part.Wire(edges)

    @classmethod
    def wiresForPath(cls, path, startPoint = FreeCAD.Vector(0, 0, 0)):
        """(path, [startPoint=Vector(0,0,0)])
        Returns a collection of wires, each representing a continuous cutting Path in path."""
        wires = []
        if hasattr(path, "Commands"):
            edges = []
            for cmd in path.Commands:
                if cmd.Name in cls.CmdMove:
                    edges.append(cls.edgeForCmd(cmd, startPoint))
                    startPoint = cls.commandEndPoint(cmd, startPoint)
                elif cmd.Name in cls.CmdMoveFast:
                    wires.append(Part.Wire(edges))
                    edges = []
                    startPoint = cls.commandEndPoint(cmd, startPoint)
            if edges:
                wires.append(Part.Wire(edges))
        return wires

    @classmethod
    def arcToHelix(cls, edge, z0, z1):
        """(edge, z0, z1)
        Assuming edge is an arc it'll return a helix matching the arc starting at z0 and rising/falling to z1."""


        p1 = edge.valueAt(edge.FirstParameter)
        p2 = edge.valueAt(edge.LastParameter)

        cmd = cls.cmdsForEdge(edge)[0]
        params = cmd.Parameters
        params.update({'Z': z1, 'K': (z1 - z0)/2})
        command = Path.Command(cmd.Name, params)

        #print("- (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f): %.2f:%.2f" % (edge.Vertexes[0].X, edge.Vertexes[0].Y, edge.Vertexes[0].Z, edge.Vertexes[1].X, edge.Vertexes[1].Y, edge.Vertexes[1].Z, z0, z1))
        #print("- %s -> %s" % (cmd, command))

        return cls.edgeForCmd(command, FreeCAD.Vector(p1.x, p1.y, z0))


    @classmethod
    def helixToArc(cls, edge, z = 0):
        """(edge, z=0)
        Returns the projection of the helix onto the XY-plane with a given offset."""
        p1 = edge.valueAt(edge.FirstParameter)
        p2 = edge.valueAt((edge.FirstParameter + edge.LastParameter)/2)
        p3 = edge.valueAt(edge.LastParameter)
        p01 = FreeCAD.Vector(p1.x, p1.y, z)
        p02 = FreeCAD.Vector(p2.x, p2.y, z)
        p03 = FreeCAD.Vector(p3.x, p3.y, z)
        return Part.Edge(Part.Arc(p01, p02, p03))

    @classmethod
    def splitArcAt(cls, edge, pt):
        """(edge, pt)
        Returns a list of 2 edges which together form the original arc split at the given point.
        The Vector pt has to represnt a point on the given arc."""
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

    @classmethod
    def splitEdgeAt(cls, edge, pt):
        """(edge, pt)
        Returns a list of 2 edges, forming the original edge split at the given point.
        The results are undefined if the Vector representing the point is not part of the edge."""
        # I could not get the OCC parameterAt and split to work ...
        # pt HAS to be on the edge, otherwise the results are undefined
        p1 = edge.valueAt(edge.FirstParameter)
        p2 = pt
        p3 = edge.valueAt(edge.LastParameter)
        edges = []

        if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
            # it's a line
            return [Part.Edge(Part.LineSegment(p1, p2)), Part.Edge(Part.LineSegment(p2, p3))]
        elif type(edge.Curve) == Part.Circle:
            # it's an arc
            return cls.splitArcAt(edge, pt)
        else:
            # it's a helix
            arc = cls.helixToArc(edge, 0)
            aes = cls.splitArcAt(arc, FreeCAD.Vector(pt.x, pt.y, 0))
            return [cls.arcToHelix(aes[0], p1.z, p2.z), cls.arcToHelix(aes[1], p2.z, p3.z)]

