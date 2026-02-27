# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2021 Schildkroet                                        *
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
import Path
import math

from FreeCAD import Vector
import Constants

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

__title__ = "Geom - geometry utilities for CAM"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Functions to extract and convert between Path.Command and Part.Edge and utility functions to reason about them."

Tolerance = 0.000001

translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class Side:
    """Class to determine and define the side a Path is on, or Vectors are in relation to each other."""

    Left = +1
    Right = -1
    Straight = 0
    On = 0

    @classmethod
    def toString(cls, side):
        """toString(side)
        Returns a string representation of the enum value."""
        if side == cls.Left:
            return "Left"
        if side == cls.Right:
            return "Right"
        return "On"

    @classmethod
    def of(cls, ptRef, pt):
        """of(ptRef, pt)
        Determine the side of pt in relation to ptRef.
        If both Points are viewed as vectors with their origin in (0,0,0)
        then the two vectors either form a straight line (On) or pt
        lies in the left or right hemisphere in regards to ptRef."""
        d = -ptRef.x * pt.y + ptRef.y * pt.x
        if d < 0:
            return cls.Left
        if d > 0:
            return cls.Right
        return cls.Straight


# Import G-code command constants from centralized CONSTANTS module
CmdMoveRapid = Constants.GCODE_MOVE_RAPID
CmdMoveStraight = Constants.GCODE_MOVE_STRAIGHT
CmdMoveCW = Constants.GCODE_MOVE_CW
CmdMoveCCW = Constants.GCODE_MOVE_CCW
CmdMoveDrill = Constants.GCODE_MOVE_DRILL
CmdMoveArc = Constants.GCODE_MOVE_ARC
CmdMoveMill = Constants.GCODE_MOVE_MILL
CmdMove = Constants.GCODE_MOVE
CmdMoveAll = Constants.GCODE_MOVE_ALL


def isRoughly(float1, float2, error=Tolerance):
    """isRoughly(float1, float2, [error=Tolerance])
    Returns true if the two values are the same within a given error."""
    return math.fabs(float1 - float2) <= error


def pointsCoincide(p1, p2, error=Tolerance):
    """pointsCoincide(p1, p2, [error=Tolerance])
    Return True if two points are roughly identical (see also isRoughly)."""
    return (
        isRoughly(p1.x, p2.x, error)
        and isRoughly(p1.y, p2.y, error)
        and isRoughly(p1.z, p2.z, error)
    )


def edgesMatch(e0, e1, error=Tolerance):
    """edgesMatch(e0, e1, [error=Tolerance]
    Return true if the edges start and end at the same point and have the same type of curve."""
    if type(e0.Curve) is not type(e1.Curve) or len(e0.Vertexes) != len(e1.Vertexes):
        return False
    return all(
        pointsCoincide(e0.Vertexes[i].Point, e1.Vertexes[i].Point, error)
        for i in range(len(e0.Vertexes))
    )


def edgeConnectsTo(edge, vector, error=Tolerance):
    """edgeConnectsTop(edge, vector, error=Tolerance)
    Returns True if edge connects to given vector."""
    return pointsCoincide(edge.valueAt(edge.FirstParameter), vector, error) or pointsCoincide(
        edge.valueAt(edge.LastParameter), vector, error
    )


def normalizeAngle(a):
    """normalizeAngle(a) ... return angle shifted into interval -pi <= a <= pi"""
    while a > math.pi:
        a = a - 2 * math.pi
    while a < -math.pi:
        a = a + 2 * math.pi
    return a


def getAngle(vector):
    """getAngle(vector)
    Returns the angle [-pi,pi] of a vector using the X-axis as the reference.
    Positive angles for vertexes in the upper hemisphere (positive y values)
    and negative angles for the lower hemisphere."""
    a = vector.getAngle(Vector(1, 0, 0))
    if vector.y < 0:
        return -a
    return a


def diffAngle(a1, a2, direction="CW"):
    """diffAngle(a1, a2, [direction='CW'])
    Returns the difference between two angles (a1 -> a2) into a given direction."""
    if direction == "CW":
        while a1 < a2:
            a1 += 2 * math.pi
        a = a1 - a2
    else:
        while a2 < a1:
            a2 += 2 * math.pi
        a = a2 - a1
    return a


def isVertical(obj):
    """isVertical(obj) ... answer True if obj points into Z"""
    if isinstance(obj, FreeCAD.Vector):
        return isRoughly(obj.x, 0) and isRoughly(obj.y, 0)

    if obj.ShapeType == "Face":
        if isinstance(obj.Surface, Part.Plane):
            return isHorizontal(obj.Surface.Axis)
        if isinstance(obj.Surface, (Part.Cylinder, Part.Cone)):
            return isVertical(obj.Surface.Axis)
        if isinstance(obj.Surface, Part.Sphere):
            return True
        if isinstance(obj.Surface, Part.SurfaceOfExtrusion):
            return isVertical(obj.Surface.Direction)
        if isinstance(obj.Surface, Part.SurfaceOfRevolution):
            return isHorizontal(obj.Surface.Direction)
        if isinstance(obj.Surface, Part.BSplineSurface):
            # simple face after scale
            vertEdges = [e for e in obj.Edges if isVertical(e)]
            return len(vertEdges) == 2 and len(obj.Edges) == 4

        Path.Log.info(
            translate("PathGeom", "face %s not handled, assuming not vertical") % type(obj.Surface)
        )
        return None

    if obj.ShapeType == "Edge":
        if isinstance(obj.Curve, (Part.Line, Part.LineSegment)):
            return isVertical(obj.Vertexes[1].Point - obj.Vertexes[0].Point)
        if isinstance(obj.Curve, (Part.Circle, Part.Ellipse)):
            return isHorizontal(obj.Curve.Axis)
        if isinstance(obj.Curve, (Part.BezierCurve, Part.BSplineCurve)):
            # the current assumption is that
            # a curve is vertical if its end points are vertical
            return isVertical(obj.Curve.EndPoint - obj.Curve.StartPoint)

        Path.Log.info(
            translate("PathGeom", "edge %s not handled, assuming not vertical") % type(obj.Curve)
        )
        return None

    Path.Log.error(translate("PathGeom", "isVertical(%s) not supported") % obj)
    return None


def isHorizontal(obj):
    """isHorizontal(obj) ... answer True if obj points into X or Y"""
    if isinstance(obj, FreeCAD.Vector):
        return isRoughly(obj.z, 0)

    if obj.ShapeType == "Face":
        if isinstance(obj.Surface, Part.Plane):
            return isVertical(obj.Surface.Axis)
        if isinstance(obj.Surface, (Part.Cylinder, Part.Cone)):
            return isHorizontal(obj.Surface.Axis)
        if isinstance(obj.Surface, Part.Sphere):
            return True
        if isinstance(obj.Surface, Part.SurfaceOfExtrusion):
            return isHorizontal(obj.Surface.Direction)
        if isinstance(obj.Surface, Part.SurfaceOfRevolution):
            return isVertical(obj.Surface.Direction)
        return isRoughly(obj.BoundBox.ZLength, 0.0)

    if obj.ShapeType == "Edge":
        if isinstance(obj.Curve, (Part.Line, Part.LineSegment)):
            return isHorizontal(obj.Vertexes[1].Point - obj.Vertexes[0].Point)
        if isinstance(obj.Curve, (Part.Circle, Part.Ellipse)):
            # or isinstance(obj.Curve, Part.BSplineCurve):
            return isVertical(obj.Curve.Axis)
        return isRoughly(obj.BoundBox.ZLength, 0.0)

    Path.Log.error(translate("PathGeom", "isHorizontal(%s) not supported") % obj)
    return None


def commandEndPoint(cmd, defaultPoint=Vector(), X="X", Y="Y", Z="Z"):
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
    Path.Log.debug(
        "  pitch = %g %g (%.2f, %.2f, %.2f) -> %.2f"
        % (pitch, math.atan2(xy(d).Length, d.z), d.x, d.y, d.z, xy(d).Length)
    )
    speed = vSpeed + pitch * (hSpeed - vSpeed)
    if speed > hSpeed and speed > vSpeed:
        return max(hSpeed, vSpeed)
    if speed < hSpeed and speed < vSpeed:
        return min(hSpeed, vSpeed)
    return speed


def cmdsForEdge(edge, flip=False, hSpeed=0, vSpeed=0, tol=0.01):
    """cmdsForEdge(edge, flip=False) -> List(Path.Command)
    Returns a list of Path.Command representing the given edge.
    If flip is True the edge is considered to be backwards.
    Edge based on a Part.Line is results in G1 command.
    Horizontal Edge based on a Part.Circle is results in G2 or G3 command.
    Other edge has no direct Path.Command mapping
    and will be approximated by straight segments."""
    pt = edge.valueAt(edge.LastParameter) if not flip else edge.valueAt(edge.FirstParameter)
    params = {"X": pt.x, "Y": pt.y, "Z": pt.z}
    if isinstance(edge.Curve, (Part.Line, Part.LineSegment)):
        if hSpeed > 0 and vSpeed > 0:
            pt2 = (
                edge.valueAt(edge.FirstParameter) if not flip else edge.valueAt(edge.LastParameter)
            )
            params.update({"F": speedBetweenPoints(pt, pt2, hSpeed, vSpeed)})
        commands = [Path.Command("G1", params)]
    else:
        p1 = edge.valueAt(edge.FirstParameter) if not flip else edge.valueAt(edge.LastParameter)
        p2 = edge.valueAt((edge.FirstParameter + edge.LastParameter) / 2)
        p3 = pt

        if (
            hasattr(edge.Curve, "Axis")
            and isinstance(edge.Curve, Part.Circle)
            and isRoughly(edge.Curve.Axis.x, 0)
            and isRoughly(edge.Curve.Axis.y, 0)
        ):
            # This is an arc or a helix and it should be represented by a simple G2/G3 command
            if edge.Curve.Axis.z < 0:
                cmd = "G2" if not flip else "G3"
            else:
                cmd = "G3" if not flip else "G2"

            if pointsCoincide(p1, p3):
                # A full circle
                offset = edge.Curve.Center - pt
            else:
                # pd = Part.Circle(xy(p1), xy(p2), xy(p3)).Center
                # Path.Log.debug(
                #    "**** %s.%d: (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f) -> center=(%.2f, %.2f)"
                #    % (cmd, flip, p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, p3.x, p3.y, p3.z, pd.x, pd.y)
                # )

                # Have to calculate the center in the XY plane, using pd leads to an error if this is a helix
                pa = xy(p1)
                pb = xy(p2)
                pc = xy(p3)
                offset = Part.Circle(pa, pb, pc).Center - pa

                # Path.Log.debug(
                #    "**** (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)"
                #    % (pa.x, pa.y, pa.z, pc.x, pc.y, pc.z)
                # )
                # Path.Log.debug(
                #    "**** (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)"
                #    % (pb.x, pb.y, pb.z, pd.x, pd.y, pd.z)
                # )
            # Path.Log.debug("**** (%.2f, %.2f, %.2f)" % (offset.x, offset.y, offset.z))

            params.update({"I": offset.x, "J": offset.y, "K": (p3.z - p1.z) / 2})
            # G2/G3 commands are always performed at hSpeed
            if hSpeed > 0:
                params.update({"F": hSpeed})
            commands = [Path.Command(cmd, params)]

        else:
            # We're dealing with a helix or a more complex shape and it has to get approximated
            # by a number of straight segments
            points = edge.discretize(Deflection=tol)
            if flip:
                points = points[::-1]

            commands = []
            if points:
                p0 = points[0]
                for p in points[1:]:
                    params = {"X": p.x, "Y": p.y, "Z": p.z}
                    if hSpeed > 0 and vSpeed > 0:
                        params["F"] = speedBetweenPoints(p0, p, hSpeed, vSpeed)
                    cmd = Path.Command("G1", params)
                    # print("***** {}".format(cmd))
                    commands.append(cmd)
                    p0 = p
    # print commands
    return commands


def edgeForCmd(cmd, startPoint):
    """edgeForCmd(cmd, startPoint).
    Returns an Edge representing the given command, assuming a given startPoint."""

    # Path.Log.debug("cmd: {}".format(cmd))
    # Path.Log.debug("startpoint {}".format(startPoint))

    endPoint = commandEndPoint(cmd, startPoint)
    if (cmd.Name in CmdMoveStraight) or (cmd.Name in CmdMoveRapid) or (cmd.Name in CmdMoveDrill):
        if pointsCoincide(startPoint, endPoint):
            return None
        return Part.Edge(Part.LineSegment(startPoint, endPoint))

    if cmd.Name in CmdMoveArc:
        center = startPoint + commandEndPoint(cmd, Vector(0, 0, 0), "I", "J", "K")
        A = xy(startPoint - center)
        B = xy(endPoint - center)
        d = -B.x * A.y + B.y * A.x

        if isRoughly(d, 0, 0.005):
            # Path.Log.debug(
            #    "Half circle arc at: (%.2f, %.2f, %.2f)" % (center.x, center.y, center.z)
            # )
            # we're dealing with half a circle here
            angle = getAngle(A) + math.pi / 2
            if cmd.Name in CmdMoveCW:
                angle -= math.pi
        else:
            C = A + B
            angle = getAngle(C)
            # Path.Log.debug(
            #    "Arc (%8f) at: (%.2f, %.2f, %.2f) -> angle=%f"
            #    % (d, center.x, center.y, center.z, angle / math.pi)
            # )

        R = A.Length
        # Path.Log.debug(
        #    "arc: p1=(%.2f, %.2f) p2=(%.2f, %.2f) -> center=(%.2f, %.2f)"
        #    % (startPoint.x, startPoint.y, endPoint.x, endPoint.y, center.x, center.y)
        # )
        # Path.Log.debug("arc: A=(%.2f, %.2f) B=(%.2f, %.2f) -> d=%.2f" % (A.x, A.y, B.x, B.y, d))
        # Path.Log.debug("arc: R=%.2f angle=%.2f" % (R, angle / math.pi))
        if isRoughly(startPoint.z, endPoint.z):
            midPoint = center + Vector(math.cos(angle), math.sin(angle), 0) * R
            # Path.Log.debug(
            #    "arc: (%.2f, %.2f) -> (%.2f, %.2f) -> (%.2f, %.2f)"
            #    % (
            #        startPoint.x,
            #        startPoint.y,
            #        midPoint.x,
            #        midPoint.y,
            #        endPoint.x,
            #        endPoint.y,
            #    )
            # )
            # Path.Log.debug("StartPoint:{}".format(startPoint))
            # Path.Log.debug("MidPoint:{}".format(midPoint))
            # Path.Log.debug("EndPoint:{}".format(endPoint))

            if pointsCoincide(startPoint, endPoint, 0.001):
                return Part.makeCircle(R, center, FreeCAD.Vector(0, 0, 1))
            else:
                return Part.Edge(Part.Arc(startPoint, midPoint, endPoint))

        # It's a Helix
        # print('angle: A=%.2f B=%.2f' % (getAngle(A)/math.pi, getAngle(B)/math.pi))
        if cmd.Name in CmdMoveCW:
            cw = True
        else:
            cw = False
        angle = diffAngle(getAngle(A), getAngle(B), "CW" if cw else "CCW")
        height = endPoint.z - startPoint.z
        pitch = height * math.fabs(2 * math.pi / angle)
        if angle > 0:
            cw = not cw
        # print("Helix: R=%.2f h=%.2f angle=%.2f pitch=%.2f" % (R, height, angle/math.pi, pitch))
        helix = Part.makeHelix(pitch, height, R, 0, not cw)
        helix.rotate(Vector(), Vector(0, 0, 1), 180 * getAngle(A) / math.pi)
        e = helix.Edges[0]
        helix.translate(startPoint - e.valueAt(e.FirstParameter))
        return helix.Edges[0]
    return None


def wireForPath(path, startPoint=Vector(0, 0, 0)):
    """wireForPath(path, [startPoint=Vector(0,0,0)])
    Returns a wire representing all move commands found in the given path."""
    edges = []
    rapid = []
    rapid_indexes = set()
    if hasattr(path, "Commands"):
        for cmd in path.Commands:
            edge = edgeForCmd(cmd, startPoint)
            if edge:
                if cmd.Name in CmdMoveRapid:
                    rapid.append(edge)
                    rapid_indexes.add(len(edges))
                edges.append(edge)
                startPoint = commandEndPoint(cmd, startPoint)
    if not edges:
        return (None, rapid, rapid_indexes)
    return (Part.Wire(edges), rapid, rapid_indexes)


def wiresForPath(path, startPoint=Vector(0, 0, 0)):
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
                if len(edges) > 0:
                    wires.append(Part.Wire(edges))
                    edges = []
                startPoint = commandEndPoint(cmd, startPoint)
        if edges:
            wires.append(Part.Wire(edges))
    return wires


def arcToHelix(edge, z0, z1):
    """arcToHelix(edge, z0, z1)
    Assuming edge is an arc it'll return a helix matching the arc starting at z0 and rising/falling to z1.
    """

    p1 = edge.valueAt(edge.FirstParameter)
    # p2 = edge.valueAt(edge.LastParameter)

    cmd = cmdsForEdge(edge)[0]
    params = cmd.Parameters
    params.update({"Z": z1, "K": (z1 - z0) / 2})
    command = Path.Command(cmd.Name, params)

    # print("- (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f): %.2f:%.2f" % (edge.Vertexes[0].X, edge.Vertexes[0].Y, edge.Vertexes[0].Z, edge.Vertexes[1].X, edge.Vertexes[1].Y, edge.Vertexes[1].Z, z0, z1))
    # print("- %s -> %s" % (cmd, command))

    return edgeForCmd(command, Vector(p1.x, p1.y, z0))


def helixToArc(edge, z=0):
    """helixToArc(edge, z=0)
    Returns the projection of the helix onto the XY-plane with a given offset."""
    p1 = edge.valueAt(edge.FirstParameter)
    p2 = edge.valueAt((edge.FirstParameter + edge.LastParameter) / 2)
    p3 = edge.valueAt(edge.LastParameter)
    p01 = Vector(p1.x, p1.y, z)
    p02 = Vector(p2.x, p2.y, z)
    p03 = Vector(p3.x, p3.y, z)
    return Part.Edge(Part.Arc(p01, p02, p03))


def splitArcAt(edge, pt):
    """splitArcAt(edge, pt)
    Returns a list of 2 edges which together form the original arc split at the given point.
    The Vector pt has to represent a point on the given arc."""
    p = edge.Curve.parameter(pt)
    e0 = Part.Arc(edge.Curve.copy(), edge.FirstParameter, p).toShape()
    e1 = Part.Arc(edge.Curve.copy(), p, edge.LastParameter).toShape()
    return [e0, e1]


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

    if isinstance(edge.Curve, (Part.Line, Part.LineSegment)):
        # it's a line
        return [
            Part.Edge(Part.LineSegment(p1, p2)),
            Part.Edge(Part.LineSegment(p2, p3)),
        ]
    elif isinstance(edge.Curve, Part.Circle):
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
        Path.Log.debug("shapes: {}".format(shapes))
        for shape in shapes:
            connected = [f for f in combined if isRoughly(shape.distToShape(f)[0], 0.0)]
            Path.Log.debug(
                "  {}: connected: {} dist: {}".format(
                    len(combined),
                    connected,
                    [shape.distToShape(f)[0] for f in combined],
                )
            )
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


def flipEdge(edge):
    """flipEdge(edge)
    Flips given edge around so the new Vertexes[0] was the old Vertexes[-1] and vice versa, without changing the shape.
    Currently only lines, line segments, circles and arcs are supported."""

    if isinstance(edge.Curve, Part.Line) and not edge.Vertexes:
        return Part.Edge(
            Part.Line(edge.valueAt(edge.LastParameter), edge.valueAt(edge.FirstParameter))
        )
    elif isinstance(edge.Curve, (Part.Line, Part.LineSegment)):
        return Part.Edge(Part.LineSegment(edge.Vertexes[-1].Point, edge.Vertexes[0].Point))
    elif isinstance(edge.Curve, Part.Circle):
        # Create an inverted circle
        circle = Part.Circle(edge.Curve.Center, -edge.Curve.Axis, edge.Curve.Radius)
        # Rotate the circle appropriately so it starts at edge.valueAt(edge.LastParameter)
        circle.rotate(
            FreeCAD.Placement(
                circle.Center,
                circle.Axis,
                180 - math.degrees(edge.LastParameter + edge.Curve.AngleXU),
            )
        )
        # Now the edge always starts at 0 and LastParameter is the value range
        arc = Part.Edge(circle, 0, edge.LastParameter - edge.FirstParameter)
        return arc
    elif isinstance(edge.Curve, (Part.BSplineCurve, Part.BezierCurve)):
        if isinstance(edge.Curve, Part.BSplineCurve):
            spline = edge.Curve
        else:
            spline = edge.Curve.toBSpline()

        mults = spline.getMultiplicities()
        weights = spline.getWeights()
        knots = spline.getKnots()
        poles = spline.getPoles()
        perio = spline.isPeriodic()
        ratio = spline.isRational()
        degree = spline.Degree

        ma = max(knots)
        mi = min(knots)
        knots = [ma + mi - k for k in knots]

        mults.reverse()
        weights.reverse()
        poles.reverse()
        knots.reverse()

        flipped = Part.BSplineCurve()
        flipped.buildFromPolesMultsKnots(poles, mults, knots, perio, degree, weights, ratio)

        return Part.Edge(flipped, ma + mi - edge.LastParameter, ma + mi - edge.FirstParameter)
    elif isinstance(edge.Curve, Part.OffsetCurve):
        return edge.reversed()

    Path.Log.warning(translate("PathGeom", "%s not supported for flipping") % type(edge.Curve))


def flipWire(wire):
    """Flip the entire wire and all its edges so it is being processed the other way around."""
    edges = [flipEdge(e) for e in wire.Edges]
    edges.reverse()
    Path.Log.debug(edges)
    return Part.Wire(edges)


def makeBoundBoxFace(bBox, offset=0.0, zHeight=0.0):
    """makeBoundBoxFace(bBox, offset=0.0, zHeight=0.0)...
    Function to create boundbox face, with possible extra offset and custom Z-height."""
    p1 = FreeCAD.Vector(bBox.XMin - offset, bBox.YMin - offset, zHeight)
    p2 = FreeCAD.Vector(bBox.XMax + offset, bBox.YMin - offset, zHeight)
    p3 = FreeCAD.Vector(bBox.XMax + offset, bBox.YMax + offset, zHeight)
    p4 = FreeCAD.Vector(bBox.XMin - offset, bBox.YMax + offset, zHeight)

    L1 = Part.makeLine(p1, p2)
    L2 = Part.makeLine(p2, p3)
    L3 = Part.makeLine(p3, p4)
    L4 = Part.makeLine(p4, p1)

    return Part.Face(Part.Wire([L1, L2, L3, L4]))


# Method to combine faces if connected
def combineHorizontalFaces(faces):
    """combineHorizontalFaces(faces)...
    This function successfully identifies and combines multiple connected faces and
    works on multiple independent faces with multiple connected faces within the list.
    The return value is a list of simplified faces.
    The Adaptive op is not concerned with which hole edges belong to which face.

    Attempts to do the same shape connecting failed with TechDraw.findShapeOutline() and
    Path.Geom.combineConnectedShapes(), so this algorithm was created.
    """
    horizontal = list()
    offset = 10.0
    topFace = None
    innerFaces = list()

    # Verify all incoming faces are at Z=0.0
    for f in faces:
        if f.BoundBox.ZMin != 0.0:
            f.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - f.BoundBox.ZMin))

    # Make offset compound boundbox solid and cut incoming face extrusions from it
    allFaces = Part.makeCompound(faces)
    if hasattr(allFaces, "Area") and isRoughly(allFaces.Area, 0.0):
        msg = translate(
            "PathGeom",
            "Zero working area to process. Check your selection and settings.",
        )
        Path.Log.info(msg)
        return horizontal

    afbb = allFaces.BoundBox
    bboxFace = makeBoundBoxFace(afbb, offset, -5.0)
    bboxSolid = bboxFace.extrude(FreeCAD.Vector(0.0, 0.0, 10.0))
    extrudedFaces = list()
    for f in faces:
        extrudedFaces.append(f.extrude(FreeCAD.Vector(0.0, 0.0, 6.0)))

    # Fuse all extruded faces together
    allFacesSolid = extrudedFaces.pop()
    for i in range(len(extrudedFaces)):
        temp = extrudedFaces.pop().fuse(allFacesSolid)
        allFacesSolid = temp
    cut = bboxSolid.cut(allFacesSolid)

    # Debug
    # Part.show(cut)
    # FreeCAD.ActiveDocument.ActiveObject.Label = "cut"

    # Identify top face and floating inner faces that are the holes in incoming faces
    for f in cut.Faces:
        fbb = f.BoundBox
        if isRoughly(fbb.ZMin, 5.0) and isRoughly(fbb.ZMax, 5.0):
            if (
                isRoughly(afbb.XMin - offset, fbb.XMin)
                and isRoughly(afbb.XMax + offset, fbb.XMax)
                and isRoughly(afbb.YMin - offset, fbb.YMin)
                and isRoughly(afbb.YMax + offset, fbb.YMax)
            ):
                topFace = f
            else:
                innerFaces.append(f)

    if not topFace:
        return horizontal

    outer = [Part.Face(w) for w in topFace.Wires[1:]]

    if outer:
        for f in outer:
            f.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - f.BoundBox.ZMin))

        if innerFaces:
            # inner = [Part.Face(f.Wire1) for f in innerFaces]
            inner = innerFaces

            for f in inner:
                f.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - f.BoundBox.ZMin))
            innerComp = Part.makeCompound(inner)
            outerComp = Part.makeCompound(outer)
            cut = outerComp.cut(innerComp)
            for f in cut.Faces:
                horizontal.append(f)
        else:
            horizontal = outer

    return horizontal
