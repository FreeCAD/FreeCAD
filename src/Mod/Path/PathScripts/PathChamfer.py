# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathEngraveBase as PathEngraveBase
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import math

from PySide import QtCore

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

def orientWire(w, forward=True):
    '''orientWire(w, forward=True) ... orients given wire in a specific direction.
    If forward = True (the default) the wire is oriented clockwise, looking down the negative Z axis.
    If forward = False the wire is oriented counter clockwise.
    If forward = None the orientation is determined by the order in which the edges appear in the wire.'''
    # first, we must ensure all edges are oriented the same way
    # one would thing this is the way it should be, but it turns out it isn't
    # on top of that, when creating a face the axis of the face seems to depend
    # the axis of any included arcs, and not in the order of the edges
    e0 = w.Edges[0]
    # well, even the very first edge could be misoriented, so let's try and connect it to the second
    if 1 < len(w.Edges):
        last = e0.valueAt(e0.LastParameter)
        e1 = w.Edges[1]
        if not PathGeom.pointsCoincide(last, e1.valueAt(e1.FirstParameter)) and not PathGeom.pointsCoincide(last, e1.valueAt(e1.LastParameter)):
            e0 = PathGeom.flipEdge(e0)

    edges = [e0]
    last = e0.valueAt(e0.LastParameter)
    for e in w.Edges[1:]:
        edge = e if PathGeom.pointsCoincide(last, e.valueAt(e.FirstParameter)) else PathGeom.flipEdge(e)
        edges.append(edge)
        last = edge.valueAt(edge.LastParameter)
    wire = Part.Wire(edges)
    if forward is not None:
        # now that we have a wire where all edges are oriented in the same way which
        # also matches their order - we can create a face and get it's axis to determine
        # the orientation of the wire - which is all we need here
        face = Part.Face(wire)
        cw = 0 < face.Surface.Axis.z
        if forward != cw:
            PathLog.track('orientWire - needs flipping')
            return PathGeom.flipWire(wire)
        PathLog.track('orientWire - ok')
    return wire

def isCircleAt(edge, center):
    '''isCircleAt(edge, center) ... helper function returns True if edge is a circle at the given center.'''
    if Circel == type(edge.Curve) or ArcOfCircle == type(edge.Curve):
        return PathGeom.pointsCoincide(edge.Curve.Center, center)
    return False

def offsetWire(wire, base, offset, forward):
    '''offsetWire(wire, base, offset, forward) ... offsets the wire away from base and orients the wire accordingly.
    The function tries to avoid most of the pitfalls of Part.makeOffset2D which is possible because all offsetting
    happens in the XY plane.
    '''
    PathLog.track('offsetWire')

    if 1 == len(wire.Edges):
        edge = wire.Edges[0]
        curve = edge.Curve
        if Part.Circle == type(curve) and wire.isClosed():
            # it's a full circle and there are some problems with that, see
            # http://www.freecadweb.org/wiki/Part%20Offset2D
            # it's easy to construct them manually though
            z = -1 if forward else 1
            edge = Part.makeCircle(curve.Radius + offset, curve.Center, FreeCAD.Vector(0, 0, z))
            if base.isInside(edge.Vertexes[0].Point, offset/2, True):
                if offset > curve.Radius or PathGeom.isRoughly(offset, curve.Radius):
                    # offsetting a hole by its own radius (or more) makes the hole vanish
                    return None
                edge = Part.makeCircle(curve.Radius - offset, curve.Center, FreeCAD.Vector(0, 0, -z))
            w = Part.Wire([edge])
            return w
        if Part.Line == type(curve) or Part.LineSegment == type(curve):
            # offsetting a single edge doesn't work because there is an infinite
            # possible planes into which the edge could be offset
            # luckily, the plane here must be the XY-plane ...
            n = (edge.Vertexes[1].Point - edge.Vertexes[0].Point).cross(FreeCAD.Vector(0, 0, 1))
            o = n.normalize() * offset
            edge.translate(o)
            if base.isInside(edge.valueAt((edge.FirstParameter + edge.LastParameter)/2), offset/2, True):
                edge.translate(-2 * o)
            w = Part.Wire([edge])
            return orientWire(w, forward)
        # if we get to this point the assumption is that makeOffset2D can deal with the edge
        pass

    w = wire.makeOffset2D(offset)

    if wire.isClosed():
        if not base.isInside(w.Edges[0].Vertexes[0].Point, offset/2, True):
            PathLog.track('closed - outside')
            return orientWire(w, forward)
        PathLog.track('closed - inside')
        try:
            w = wire.makeOffset2D(-offset)
        except:
            # most likely offsetting didn't work because the wire is a hole
            # and the offset is too big - making the hole vanish
            return None
        return orientWire(w, forward)

    # An edge is considered to be inside of shape if the mid point is inside
    # Of the remaining edges we take the longest wire to be the engraving side
    # Looking for a circle with the start vertex as center marks and end
    #  starting from there follow the edges until a circle with the end vertex as center is found
    #  if the traversed edges include any oof the remainig from above, all those edges are remaining
    #  this is to also include edges which might partially be inside shape
    #  if they need to be discarded, split, that should happen in a post process
    # Depending on the Axis of the circle, and which side remains we know if the wire needs to be flipped

    # find edges that are not inside the shape
    def isInside(edge):
        if shape.Shape.isInside(edge.Vertexes[0].Point, offset/2, True) and shape.Shape.isInside(edge.Vertexes[-1].Point, offset/2, True):
            return True
        return False
    outside = [e for e in edges if not isInside(e)]
    # discard all edges that are not part of the longest wire
    longestWire = None
    for w in [Part.Wire(el) for el in Part.sortEdges(outside)]:
        if not longestWire or longestWire.Length < w.Length:
            longestWire = w

    # find the start and end point
    start = wire.Vertexes[0].Point
    end = wire.Vertexes[-1].Point

    collectLeft = False
    collectRight = False
    leftSideEdges = []
    rightSideEdges = []

    for e in (w.Edges + w.Edges):
        if isCircleAt(e, start):
            if PathGeom.pointsCoincide(e.Curve.Axis, FreeCAD.Vector(0, 0, 1)):
                if not collectLeft and leftSideEdges:
                    break
                collectLeft = True
                collectRight = False
            else:
                if not collectRight and rightSideEdges:
                    break
                collectLeft = False
                collectRight = True
        elif isCircleAt(e, end):
            if PathGeom.pointsCoincide(e.Curve.Axis, FreeCAD.Vector(0, 0, 1)):
                if not collectRight and rightSideEdges:
                    break
                collectLeft = False
                collectRight = True
            else:
                if not collectLeft and leftSideEdges:
                    break
                collectLeft = True
                collectRight = False
        elif collectLeft:
            leftSideEdges.append(e)
        elif collectRight:
            rightSideEdges.append(e)

    edges = leftSideEdges
    for e in longestWire.Edges:
        for e0 in rightSideEdges:
            if PathGeom.edgesMatch(e, e0):
                if forward:
                    edges = [PathGeom.flipEdge(edge) for edge in rightSideEdges]
                return Part.Wire(edges)

    if not forward:
        edges = [PathGeom.flipEdge(edge) for edge in rightSideEdges]
    return Part.Wire(edges)

def toolDepthAndOffset(width, extraDepth, tool):
    '''toolDepthAndOffset(width, extraDepth, tool) ... return tuple for given parameters.'''
    angle = tool.CuttingEdgeAngle
    if 0 == angle:
        angle = 180
    tan = math.tan(math.radians(angle/2))

    toolDepth = 0 if 0 == tan else width / tan
    extraDepth = extraDepth
    depth = toolDepth + extraDepth
    toolOffset = tool.FlatRadius
    extraOffset = tool.Diameter/2 - width if 180 == angle else extraDepth / tan
    offset = toolOffset + extraOffset
    return (depth, offset)

class ObjectChamfer(PathEngraveBase.ObjectOp):
    '''Proxy class for Chamfer operation.'''

    def opFeatures(self, obj):
        return PathOp.FeatureTool | PathOp.FeatureHeights | PathOp.FeatureBaseEdges | PathOp.FeatureBaseFaces

    def initOperation(self, obj):
        obj.addProperty('App::PropertyDistance',    'Width',      'Chamfer', QtCore.QT_TRANSLATE_NOOP('PathChamfer', 'The desired width of the chamfer'))
        obj.addProperty('App::PropertyDistance',    'ExtraDepth', 'Chamfer', QtCore.QT_TRANSLATE_NOOP('PathChamfer', 'The additional depth of the tool path'))
        obj.addProperty('App::PropertyEnumeration', 'Join',       'Chamfer', QtCore.QT_TRANSLATE_NOOP('PathChamfer', 'How to join chamfer segments'))
        obj.Join = ['Round', 'Miter']
        obj.setEditorMode('Join', 2) # hide for now

    def opExecute(self, obj):
        (depth, offset) = toolDepthAndOffset(obj.Width.Value, obj.ExtraDepth.Value, self.tool)

        self.basewires = []
        self.adjusted_basewires = []
        wires = []
        for base, subs in obj.Base:
            edges = []
            basewires = []
            for f in subs:
                sub = base.Shape.getElement(f)
                if type(sub) == Part.Edge:
                    edges.append(sub)
                elif sub.Wires:
                    basewires.extend(sub.Wires)
                else:
                    basewires.append(Part.Wire(sub.Edges))
            self.edges = edges
            for edgelist in Part.sortEdges(edges):
                basewires.append(Part.Wire(edgelist))

            self.basewires.extend(basewires)

            for w in self.adjustWirePlacement(obj, base, basewires):
                self.adjusted_basewires.append(w)
                wire = offsetWire(w, base.Shape, offset, True)
                if wire:
                    wires.append(wire)

        self.wires = wires
        self.buildpathocc(obj, wires, [depth], True)

    def opSetDefaultValues(self, obj):
        obj.Width = '1 mm'
        obj.ExtraDepth = '0.1 mm'
        obj.Join = 'Round'

def Create(name):
    '''Create(name) ... Creates and returns a Chamfer operation.'''
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectChamfer(obj)
    return obj

