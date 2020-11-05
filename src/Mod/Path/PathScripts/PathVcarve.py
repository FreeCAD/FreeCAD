# -*- coding: utf-8 -*-
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathUtils as PathUtils
import PathScripts.PathGeom as PathGeom
import PathScripts.PathPreferences as PathPreferences

import traceback

import math

from PySide import QtCore

__doc__ = "Class and implementation of Path Vcarve operation"

PRIMARY   = 0
SECONDARY = 1
EXTERIOR1 = 2
EXTERIOR2 = 3
COLINEAR  = 4
TWIN      = 5

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule(PathLog.thisModule())

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


VD = []
Vertex = {}

_sorting = 'global'

def _collectVoronoiWires(vd):
    edges = [e for e in vd.Edges if e.Color == PRIMARY]
    vertex = {}
    for e in edges:
        for v in e.Vertices:
            i = v.Index
            j = vertex.get(i, [])
            j.append(e)
            vertex[i] = j
    Vertex.clear()
    for v in vertex:
        Vertex[v] = vertex[v]

    # knots are the start and end points of a wire
    knots = [i for i in vertex if len(vertex[i]) == 1]
    knots.extend([i for i in vertex if len(vertex[i]) > 2])
    if len(knots) == 0:
        for i in vertex:
            if len(vertex[i]) > 0:
                knots.append(i)
                break

    def consume(v, edge):
        vertex[v] = [e for e in vertex[v] if e.Index != edge.Index]
        return len(vertex[v]) == 0

    def traverse(vStart, edge, edges):
        if vStart == edge.Vertices[0].Index:
            vEnd = edge.Vertices[1].Index
            edges.append(edge)
        else:
            vEnd = edge.Vertices[0].Index
            edges.append(edge.Twin)

        consume(vStart, edge)
        if consume(vEnd, edge):
            return None
        return vEnd

    wires = []
    while knots:
        we = []
        vFirst = knots[0]
        vStart = vFirst
        vLast  = vFirst
        if len(vertex[vStart]):
            while not vStart is None:
                vLast  = vStart
                edges  = vertex[vStart]
                if len(edges) > 0:
                    edge   = edges[0]
                    vStart = traverse(vStart, edge, we)
                else:
                    vStart = None
            wires.append(we)
        if len(vertex[vFirst]) == 0:
            knots = [v for v in knots if v != vFirst]
        if len(vertex[vLast]) == 0:
            knots = [v for v in knots if v != vLast]
    return wires

def _sortVoronoiWires(wires, start = FreeCAD.Vector(0, 0, 0)):
    def closestTo(start, point):
        p = None
        l = None
        for i in point:
            if l is None or l > start.distanceToPoint(point[i]):
                l = start.distanceToPoint(point[i])
                p = i
        return (p, l)


    begin = {}
    end   = {}

    for i, w in enumerate(wires):
        begin[i] = w[ 0].Vertices[0].toPoint()
        end[i]   = w[-1].Vertices[1].toPoint()

    result = []
    while begin:
        (bIdx, bLen) = closestTo(start, begin)
        (eIdx, eLen) = closestTo(start, end)
        if bLen < eLen:
            result.append(wires[bIdx])
            start =   end[bIdx]
            del     begin[bIdx]
            del       end[bIdx]
        else:
            result.append([e.Twin for e in reversed(wires[eIdx])])
            start = begin[eIdx]
            del     begin[eIdx]
            del       end[eIdx]

    return result

class ObjectVcarve(PathEngraveBase.ObjectOp):
    '''Proxy class for Vcarve operation.'''

    def opFeatures(self, obj):
        '''opFeatures(obj) ... return all standard features and edges based geomtries'''
        return PathOp.FeatureTool | PathOp.FeatureHeights | PathOp.FeatureBaseFaces | PathOp.FeatureCoolant

    def setupAdditionalProperties(self, obj):
        if not hasattr(obj, 'BaseShapes'):
            obj.addProperty("App::PropertyLinkList", "BaseShapes", "Path",
                            QtCore.QT_TRANSLATE_NOOP("PathVcarve",
                                "Additional base objects to be engraved"))
        obj.setEditorMode('BaseShapes', 2)  # hide
        if not hasattr(obj, 'BaseObject'):
            obj.addProperty("App::PropertyLink", "BaseObject", "Path",
                            QtCore.QT_TRANSLATE_NOOP("PathVcarve",
                            "Additional base objects to be engraved"))
        obj.setEditorMode('BaseObject', 2)  # hide

    def initOperation(self, obj):
        '''initOperation(obj) ... create vcarve specific properties.'''
        obj.addProperty("App::PropertyFloat", "Discretize", "Path",
                        QtCore.QT_TRANSLATE_NOOP("PathVcarve",
                        "The deflection value for discretizing arcs"))
        obj.addProperty("App::PropertyFloat", "Threshold", "Path",
                        QtCore.QT_TRANSLATE_NOOP("PathVcarve",
                        "cutoff for removing colinear segments (degrees). \
                        default=10.0."))
        obj.addProperty("App::PropertyFloat", "Tolerance", "Path",
                QtCore.QT_TRANSLATE_NOOP("PathVcarve", ""))
        obj.Threshold = 10.0
        obj.Discretize = 0.01
        obj.Tolerance = PathPreferences.defaultGeometryTolerance()
        self.setupAdditionalProperties(obj)

    def opOnDocumentRestored(self, obj):
        # upgrade ...
        self.setupAdditionalProperties(obj)

    def _calculate_depth(self, MIC, zStart, zStop, zScale):
        # given a maximum inscribed circle (MIC) and tool angle,
        # return depth of cut relative to zStart.
        depth = zStart - round(MIC / zScale, 4)
        PathLog.debug('zStart value: {} depth: {}'.format(zStart, depth))
        return depth if depth > zStop else zStop

    def _getPartEdge(self, edge, zStart, zStop, zScale):
        dist = edge.getDistances()
        return edge.toShape(self._calculate_depth(dist[0], zStart, zStop, zScale), self._calculate_depth(dist[1], zStart, zStop, zScale))

    def _getPartEdges(self, obj, vWire):
        # pre-calculate the depth limits - pre-mature optimisation ;)
        r = float(obj.ToolController.Tool.Diameter) / 2
        toolangle = obj.ToolController.Tool.CuttingEdgeAngle
        zStart = self.model[0].Shape.BoundBox.ZMin
        zStop  = zStart - r / math.tan(math.radians(toolangle/2))
        zScale = 1.0 / math.tan(math.radians(toolangle / 2))

        edges = []
        for e in vWire:
            edges.append(self._getPartEdge(e, zStart, zStop, zScale))
        return edges

    def buildPathMedial(self, obj, Faces):
        '''constructs a medial axis path using openvoronoi'''

        def insert_many_wires(vd, wires):
            for wire in wires:
                PathLog.debug('discretize value: {}'.format(obj.Discretize))
                pts = wire.discretize(QuasiDeflection=obj.Discretize)
                ptv = [FreeCAD.Vector(p.x, p.y) for p in pts]
                ptv.append(ptv[0])

                for i in range(len(pts)):
                    vd.addSegment(ptv[i], ptv[i+1])

        def cutWire(edges):
            path = []
            path.append(Path.Command("G0 Z{}".format(obj.SafeHeight.Value)))
            e = edges[0]
            p = e.valueAt(e.FirstParameter)
            path.append(Path.Command("G0 X{} Y{} Z{}".format(p.x, p.y,
                obj.SafeHeight.Value)))
            c = Path.Command("G1 X{} Y{} Z{} F{}".format(p.x, p.y, p.z,
                obj.ToolController.HorizFeed.Value))
            path.append(c)
            for e in edges:
                path.extend(PathGeom.cmdsForEdge(e,
                    hSpeed=obj.ToolController.HorizFeed.Value))

            return path

        VD.clear()
        voronoiWires = []
        for f in Faces:
            vd = Path.Voronoi()
            insert_many_wires(vd, f.Wires)

            vd.construct()

            for e in vd.Edges:
                e.Color = PRIMARY if e.isPrimary() else SECONDARY
            vd.colorExterior(EXTERIOR1)
            vd.colorExterior(EXTERIOR2,
                lambda v: not f.isInside(v.toPoint(f.BoundBox.ZMin),
                obj.Tolerance, True))
            vd.colorColinear(COLINEAR, obj.Threshold)
            vd.colorTwins(TWIN)

            wires = _collectVoronoiWires(vd);
            if _sorting != 'global':
                wires = _sortVoronoiWires(wires)
            voronoiWires.extend(wires)
            VD.append((f, vd, wires))

        if _sorting == 'global':
            voronoiWires = _sortVoronoiWires(voronoiWires)

        pathlist = []
        pathlist.append(Path.Command("(starting)"))
        for w in voronoiWires:
            pWire = self._getPartEdges(obj, w)
            if pWire:
                wires.append(pWire)
                pathlist.extend(cutWire(pWire))
        self.commandlist = pathlist

    def opExecute(self, obj):
        '''opExecute(obj) ... process engraving operation'''
        PathLog.track()

        if not hasattr(obj.ToolController.Tool, "CuttingEdgeAngle"):
            FreeCAD.Console.PrintError(
                translate("Path_Vcarve", "VCarve requires an engraving \
                           cutter with CuttingEdgeAngle") + "\n")

        if obj.ToolController.Tool.CuttingEdgeAngle >= 180.0:
            FreeCAD.Console.PrintError(
                translate("Path_Vcarve",
                    "Engraver Cutting Edge Angle must be < 180 degrees.") + "\n")
            return
        try:
            if obj.Base:
                PathLog.track()
                for base in obj.Base:
                    faces = []
                    for sub in base[1]:
                        shape = getattr(base[0].Shape, sub)
                        if isinstance(shape, Part.Face):
                            faces.append(shape)

                modelshape = Part.makeCompound(faces)

            elif len(self.model) == 1 and self.model[0].isDerivedFrom('Sketcher::SketchObject') or \
                    self.model[0].isDerivedFrom('Part::Part2DObject'):
                PathLog.track()

                modelshape = self.model[0].Shape
            self.buildPathMedial(obj, modelshape.Faces)

        except Exception as e:
            PathLog.error(e)
            traceback.print_exc()
            PathLog.error(translate('PathVcarve', 'The Job Base Object has \
no engraveable element. Engraving \
operation will produce no output.'))
            raise e

    def opUpdateDepths(self, obj, ignoreErrors=False):
        '''updateDepths(obj) ... engraving is always done at the top most z-value'''
        job = PathUtils.findParentJob(obj)
        self.opSetDefaultValues(obj, job)


def SetupProperties():
    return ["Discretize"]


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Vcarve operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    ObjectVcarve(obj, name)
    return obj
