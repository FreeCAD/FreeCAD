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
    def getAngle(cls, vertex):
        """(vertex)
        Returns the angle [-pi,pi] of a vertex using the X-axis as the reference.
        Positive angles for vertexes in the upper hemishpere (positive y values)
        and negative angles for the lower hemishpere."""
        a = vertex.getAngle(FreeCAD.Vector(1,0,0))
        if vertex.y < 0:
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
        m = FreeCAD.Matrix()
        m.unity()

        p1 = edge.valueAt(edge.FirstParameter)
        p2 = edge.valueAt(edge.LastParameter)
        z = p1.z

        pd = p2 - p1
        dz = z1 - z0

        #print("arcToHelix(%.2f, %.2f): dz=%.2f, dy=%.2f, z=%.2f" % (z0 ,z1, dz, pd.y, z))

        m.A32 = dz / pd.y
        m.A34 = - m.A32
        if dz < 0:
            m.A34 *= p2.y
            m.A34 += z1 - z
        else:
            m.A34 *= p1.y
            m.A34 += z0 - z

        e = edge.transformGeometry(m).Edges[0]
        return e
