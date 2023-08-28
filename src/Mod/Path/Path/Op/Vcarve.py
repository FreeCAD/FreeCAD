# -*- coding: utf-8 -*-
# ***************************************************************************
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
import Path.Op.Base as PathOp
import Path.Op.EngraveBase as PathEngraveBase
import PathScripts.PathUtils as PathUtils
import math
from PySide.QtCore import QT_TRANSLATE_NOOP


from PySide import QtCore

__doc__ = "Class and implementation of Path Vcarve operation"

PRIMARY = 0
SECONDARY = 1
EXTERIOR1 = 2
EXTERIOR2 = 3
COLINEAR = 4
TWIN = 5
BORDERLINE = 6

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate
_sorting = "global"


def _collectVoronoiWires(vd):
    edges = [e for e in vd.Edges if e.Color == PRIMARY]
    vertex = {}
    for e in edges:
        for v in e.Vertices:
            i = v.Index
            j = vertex.get(i, [])
            j.append(e)
            vertex[i] = j

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
        vLast = vFirst
        if len(vertex[vStart]):
            while vStart is not None:
                vLast = vStart
                edges = vertex[vStart]
                if len(edges) > 0:
                    edge = edges[0]
                    vStart = traverse(vStart, edge, we)
                else:
                    vStart = None
            wires.append(we)
        if len(vertex[vFirst]) == 0:
            knots = [v for v in knots if v != vFirst]
        if len(vertex[vLast]) == 0:
            knots = [v for v in knots if v != vLast]
    return wires


def _sortVoronoiWires(wires, start=FreeCAD.Vector(0, 0, 0)):
    def closestTo(start, point):
        p = None
        l = None
        for i in point:
            if l is None or l > start.distanceToPoint(point[i]):
                l = start.distanceToPoint(point[i])
                p = i
        return (p, l)

    begin = {}
    end = {}

    for i, w in enumerate(wires):
        begin[i] = w[0].Vertices[0].toPoint()
        end[i] = w[-1].Vertices[1].toPoint()

    result = []
    while begin:
        (bIdx, bLen) = closestTo(start, begin)
        (eIdx, eLen) = closestTo(start, end)
        if bLen < eLen:
            result.append(wires[bIdx])
            start = end[bIdx]
            del begin[bIdx]
            del end[bIdx]
        else:
            result.append([e.Twin for e in reversed(wires[eIdx])])
            start = begin[eIdx]
            del begin[eIdx]
            del end[eIdx]

    return result


class _Geometry(object):
    """POD class so the limits only have to be calculated once."""

    def __init__(self, zStart, zStop, zScale):
        self.start = zStart
        self.stop = zStop
        self.scale = zScale

    @classmethod
    def FromTool(cls, tool, zStart, zFinal):
        rMax = float(tool.Diameter) / 2.0
        rMin = float(tool.TipDiameter) / 2.0
        toolangle = math.tan(math.radians(tool.CuttingEdgeAngle.Value / 2.0))
        zScale = 1.0 / toolangle
        zStop = zStart - rMax * zScale
        zOff = rMin * zScale

        return _Geometry(zStart + zOff, max(zStop + zOff, zFinal), zScale)

    @classmethod
    def FromObj(cls, obj, model):
        zStart = model.Shape.BoundBox.ZMax
        finalDepth = obj.FinalDepth.Value

        return cls.FromTool(obj.ToolController.Tool, zStart, finalDepth)


def _calculate_depth(MIC, geom):
    # given a maximum inscribed circle (MIC) and tool angle,
    # return depth of cut relative to zStart.
    depth = geom.start - round(MIC * geom.scale, 4)
    Path.Log.debug("zStart value: {} depth: {}".format(geom.start, depth))

    return max(depth, geom.stop)


def _getPartEdge(edge, depths):
    dist = edge.getDistances()
    zBegin = _calculate_depth(dist[0], depths)
    zEnd = _calculate_depth(dist[1], depths)
    return edge.toShape(zBegin, zEnd)


class ObjectVcarve(PathEngraveBase.ObjectOp):
    """Proxy class for Vcarve operation."""

    def opFeatures(self, obj):
        """opFeatures(obj) ... return all standard features and edges based geometries"""
        return (
            PathOp.FeatureTool
            | PathOp.FeatureHeights
            | PathOp.FeatureDepths
            | PathOp.FeatureBaseFaces
            | PathOp.FeatureCoolant
        )

    def setupAdditionalProperties(self, obj):
        if not hasattr(obj, "BaseShapes"):
            obj.addProperty(
                "App::PropertyLinkList",
                "BaseShapes",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Additional base objects to be engraved"
                ),
            )
        obj.setEditorMode("BaseShapes", 2)  # hide

    def initOperation(self, obj):
        """initOperation(obj) ... create vcarve specific properties."""
        obj.addProperty(
            "App::PropertyFloat",
            "Discretize",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The deflection value for discretizing arcs"
            ),
        )
        obj.addProperty(
            "App::PropertyFloat",
            "Colinear",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Cutoff for removing colinear segments (degrees). \
                        default=10.0.",
            ),
        )
        obj.addProperty(
            "App::PropertyFloat",
            "Tolerance",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Vcarve Tolerance"),
        )
        obj.Colinear = 10.0
        obj.Discretize = 0.01
        obj.Tolerance = Path.Preferences.defaultGeometryTolerance()
        self.setupAdditionalProperties(obj)

    def opOnDocumentRestored(self, obj):
        # upgrade ...
        self.setupAdditionalProperties(obj)

    def _getPartEdges(self, obj, vWire, geom):
        edges = []
        for e in vWire:
            edges.append(_getPartEdge(e, geom))
        return edges

    def buildPathMedial(self, obj, faces):
        """constructs a medial axis path using openvoronoi"""

        def insert_many_wires(vd, wires):
            for wire in wires:
                Path.Log.debug("discretize value: {}".format(obj.Discretize))
                pts = wire.discretize(QuasiDeflection=obj.Discretize)
                ptv = [FreeCAD.Vector(p.x, p.y) for p in pts]
                ptv.append(ptv[0])

                for i in range(len(pts)):
                    vd.addSegment(ptv[i], ptv[i + 1])

        def cutWire(edges):
            path = []
            path.append(Path.Command("G0 Z{}".format(obj.SafeHeight.Value)))
            e = edges[0]
            p = e.valueAt(e.FirstParameter)
            path.append(
                Path.Command("G0 X{} Y{} Z{}".format(p.x, p.y, obj.SafeHeight.Value))
            )
            hSpeed = obj.ToolController.HorizFeed.Value
            vSpeed = obj.ToolController.VertFeed.Value
            path.append(
                Path.Command("G1 X{} Y{} Z{} F{}".format(p.x, p.y, p.z, vSpeed))
            )
            for e in edges:
                path.extend(Path.Geom.cmdsForEdge(e, hSpeed=hSpeed, vSpeed=vSpeed))

            return path

        voronoiWires = []
        for f in faces:
            vd = Path.Voronoi.Diagram()
            insert_many_wires(vd, f.Wires)

            vd.construct()

            for e in vd.Edges:
                if e.isPrimary():
                    if e.isBorderline():
                        e.Color = BORDERLINE
                    else:
                        e.Color = PRIMARY
                else:
                    e.Color = SECONDARY
            vd.colorExterior(EXTERIOR1)
            vd.colorExterior(
                EXTERIOR2,
                lambda v: not f.isInside(
                    v.toPoint(f.BoundBox.ZMin), obj.Tolerance, True
                ),
            )
            vd.colorColinear(COLINEAR, obj.Colinear)
            vd.colorTwins(TWIN)

            wires = _collectVoronoiWires(vd)
            if _sorting != "global":
                wires = _sortVoronoiWires(wires)
            voronoiWires.extend(wires)

        if _sorting == "global":
            voronoiWires = _sortVoronoiWires(voronoiWires)

        geom = _Geometry.FromObj(obj, self.model[0])

        pathlist = []
        pathlist.append(Path.Command("(starting)"))
        for w in voronoiWires:
            pWire = self._getPartEdges(obj, w, geom)
            if pWire:
                wires.append(pWire)
                pathlist.extend(cutWire(pWire))
        self.commandlist = pathlist

    def opExecute(self, obj):
        """opExecute(obj) ... process engraving operation"""
        Path.Log.track()

        if not hasattr(obj.ToolController.Tool, "CuttingEdgeAngle"):
            Path.Log.error(
                translate(
                    "Path_Vcarve",
                    "VCarve requires an engraving cutter with a cutting edge angle",
                )
            )

        if obj.ToolController.Tool.CuttingEdgeAngle >= 180.0:
            Path.Log.error(
                translate(
                    "Path_Vcarve", "Engraver cutting edge angle must be < 180 degrees."
                )
            )
            return

        try:
            faces = []

            for base in obj.BaseShapes:
                faces.extend(base.Shape.Faces)

            for base in obj.Base:
                for sub in base[1]:
                    shape = getattr(base[0].Shape, sub)
                    if isinstance(shape, Part.Face):
                        faces.append(shape)

            if not faces:
                for model in self.model:
                    if model.isDerivedFrom(
                        "Sketcher::SketchObject"
                    ) or model.isDerivedFrom("Part::Part2DObject"):
                        faces.extend(model.Shape.Faces)

            if faces:
                self.buildPathMedial(obj, faces)
            else:
                Path.Log.error(
                    translate(
                        "PathVcarve",
                        "The Job Base Object has no engraveable element. Engraving operation will produce no output.",
                    )
                )

        except Exception as e:
            Path.Log.error(
                "Error processing Base object. Engraving operation will produce no output."
            )

    def opUpdateDepths(self, obj, ignoreErrors=False):
        """updateDepths(obj) ... engraving is always done at the top most z-value"""
        job = PathUtils.findParentJob(obj)
        self.opSetDefaultValues(obj, job)

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj) ... set depths for vcarving"""
        if PathOp.FeatureDepths & self.opFeatures(obj):
            if job and len(job.Model.Group) > 0:
                bb = job.Proxy.modelBoundBox(job)
                obj.OpStartDepth = bb.ZMax
                obj.OpFinalDepth = job.Stock.Shape.BoundBox.ZMin
            else:
                obj.OpFinalDepth = -0.1

    def isToolSupported(self, obj, tool):
        """isToolSupported(obj, tool) ... returns True if v-carve op can work with tool."""
        return (
            hasattr(tool, "Diameter")
            and hasattr(tool, "CuttingEdgeAngle")
            and hasattr(tool, "TipDiameter")
        )


def SetupProperties():
    return ["Discretize"]


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Vcarve operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectVcarve(obj, name, parentJob)
    return obj
