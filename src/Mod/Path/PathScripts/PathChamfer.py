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
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import math

from PathScripts.PathGeom import PathGeom
from PySide import QtCore

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

def removeInsideEdges(edges, shape, offset):
    # An edge is considered to be inside of shape if the mid point is inside
    # Of the remaining edges we take the longest wire to be the engraving side
    # Looking for a circle with the start vertex as center marks and end
    #  starting from there follow the edges until a circle with the end vertex as center is found
    #  if the traversed edges include any oof the remainig from above, all those edges are remaining
    #  this is to also include edges which might partially be inside shape
    #  if they need to be discarded, split, that should happen in a post process
    # Depending on the Axis of the circle, and which side remains we know if the wire needs to be flipped
    def isInside(edge):
        if shape.Shape.isInside(edge.Vertexes[0].Point, offset/2, True) and shape.Shape.isInside(edge.Vertexes[-1].Point, offset/2, True):
            return True
        return False
    remaining = [e for e in edges if not isInside(e)]
    # of the ones remaining, the first and the last are the end offsets
    allFirst = [e.firstVertex().Point for e in remaining]
    allLast  = [e.lastVertex().Point for e in remaining]
    first = [f for f in allFirst if not f in allLast][0]
    last = [l for l in allLast if not l in allFirst][0]
    #return [e for e in remaining if not PathGeom.pointsCoincide(e.firstVertex().Point, first) and not PathGeom.pointsCoincide(e.lastVertex().Point, last)]
    return remaining

def orientWireForClimbMilling(w):
    face = Part.Face(w)
    cw  = 'Forward' == obj.ToolController.SpindleDir 
    wcw = 0 < face.Surface.Axis.z
    if cw != wcw:
        PathLog.track('flip wire')
        # This works because Path creation will flip the edges accordingly
        return Part.Wire([e for e in reversed(w.Edges)])
    PathLog.track('no flip', cw, wcw)
    return w

def offsetWire(obj, wire, base, offset):
    PathLog.track(obj.Label)

    w = wire.makeOffset2D(offset)
    if wire.isClosed():
        if not base.Shape.isInside(w.Edges[0].Vertexes[0].Point, offset/2, True):
            return orientWireForClimbMilling(w)
        w = wire.makeOffset2D(-offset)
        return orientWireForClimbMilling(w)

    edges = removeInsideEdges(w.Edges, base, offset)
    if not edges:
        w = wire.makeOffset2D(-offset)
        edges = removeInsideEdges(w.Edges, base, offset)
    points = []
    for e in edges:
        points
    # determine the start point
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

            for w in self.adjustWirePlacement(obj, base, basewires):
                wires.append(offsetWire(obj, w, base, offset))
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

