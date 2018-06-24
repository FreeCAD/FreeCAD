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

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

def orientWire(w, forward=True):
    face = Part.Face(w)
    cw = 0 < face.Surface.Axis.z
    if forward != cw:
        PathLog.track('orientWire - needs flipping')
        return PathGeom.flipWire(w)
    PathLog.track('orientWire - ok')
    return w

def isCircleAt(edge, center):
    if Circel == type(edge.Curve) or ArcOfCircle == type(edge.Curve):
        return PathGeom.pointsCoincide(edge.Curve.Center, center)
    return False

def offsetWire(wire, base, offset, forward):
    PathLog.track('offsetWire')

    w = wire.makeOffset2D(offset)
    if 1 == len(w.Edges):
        e = w.Edges[0]
        e.Placement = FreeCAD.Placement()
        w = Part.Wire(e)
    

    if wire.isClosed():
        if not base.isInside(w.Edges[0].Vertexes[0].Point, offset/2, True):
            PathLog.track('closed - outside')
            return orientWire(w, forward)
        PathLog.track('closed - inside')
        w = wire.makeOffset2D(-offset)
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

class ObjectChamfer(PathEngraveBase.ObjectOp):
    '''Proxy class for Chamfer operation.'''

    def opFeatures(self, obj):
        return PathOp.FeatureTool | PathOp.FeatureHeights | PathOp.FeatureBaseEdges | PathOp.FeatureBaseFaces

    def initOperation(self, obj):
        obj.addProperty("App::PropertyDistance", "Width",      "Chamfer", QtCore.QT_TRANSLATE_NOOP("PathChamfer", "The desired width of the chamfer"))
        obj.addProperty("App::PropertyDistance", "ExtraDepth", "Chamfer", QtCore.QT_TRANSLATE_NOOP("PathChamfer", "The additional depth of the tool path"))
        obj.addProperty("App::PropertyEnumeration", "Join",    "Chamfer", QtCore.QT_TRANSLATE_NOOP("PathChamfer", "How to join chamfer segments"))
        obj.Join = ['Round', 'Miter']

    def opExecute(self, obj):
        angle = self.tool.CuttingEdgeAngle
        if 0 == angle:
            angle = 180
        tan = math.tan(math.radians(angle/2))

        toolDepth = 0 if 0 == tan else obj.Width.Value / tan
        extraDepth = obj.ExtraDepth.Value
        depth = toolDepth + extraDepth
        toolOffset = self.tool.FlatRadius
        extraOffset = self.tool.Diameter/2 - obj.Width.Value if 180 == angle else obj.ExtraDepth.Value / tan
        offset = toolOffset + extraOffset

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
                wires.append(offsetWire(w, base.Shape, offset, True))

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

