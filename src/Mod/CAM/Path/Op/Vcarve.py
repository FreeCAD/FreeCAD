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

__doc__ = "Class and implementation of CAM Vcarve operation"

PRIMARY = 0
SECONDARY = 1
EXTERIOR1 = 2
EXTERIOR2 = 3
COLINEAR = 4
TWIN = 5
BORDERLINE = 6

# There is a bug in logging library. To enable debugging - set True also in Gui/Vcarve.py

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


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
        length = None
        for i in point:
            if length is None or length > start.distanceToPoint(point[i]):
                length = start.distanceToPoint(point[i])
                p = i
        return (p, length)

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

    def __init__(self, zStart, zStop, zScale, zStepDown):
        self.start = zStart
        self.stop = zStop
        self.scale = zScale
        self.stepDown = zStepDown
        self.stepDownPass = 1

        # offset is used in finishing passes to override
        # any calculated vcarving depths. Usually going deeper 0.1-0.2 mm on finishing pass can help
        # remove "fuzzy skin" or other imperfections.
        self.offset = 0

    def incrementStepDownDepth(self, maximumUsableDepth):
        """
        Increase stepDown depth before starting new carving pass.
        :returns: True if successful, False if maximum depth achieved
        """

        # do not allow one to increase depth if we are already at stop depth
        if self.maximumDepth == self.stop:
            return False

        # do not allow one to increase depth if we are already at
        # maximum usable depth

        if self.maximumDepth <= maximumUsableDepth:
            return False

        self.stepDownPass += 1
        return True

    @property
    def maximumDepth(self):
        """
        Return maximum vcarving depth computed from step down setting and pass number
        """

        if self.stepDown == 0:
            return self.stop

        return max(self.stop, self.start - (self.stepDownPass * self.stepDown))

    @classmethod
    def FromTool(cls, tool, zStart, zFinal, zStepDown=0):
        rMax = float(tool.Diameter) / 2.0
        rMin = float(tool.TipDiameter) / 2.0
        toolangle = math.tan(math.radians(tool.CuttingEdgeAngle.Value / 2.0))
        zScale = 1.0 / toolangle
        zStop = zStart - rMax * zScale
        zOff = rMin * zScale

        return _Geometry(zStart + zOff, max(zStop + zOff, zFinal), zScale, zStepDown)

    @classmethod
    def FromObj(cls, obj, model):
        if obj.BaseShapes and hasattr(obj.BaseShapes[0], "Shape"):
            zStart = obj.BaseShapes[0].Shape.BoundBox.ZMax
        elif obj.Base and obj.Base[0][0] and hasattr(obj.Base[0][0], "Shape"):
            if len(obj.Base[0]) > 1 and "Face" in obj.Base[0][1][0]:
                faceName = obj.Base[0][1][0]
                faceIndex = int(faceName.replace("Face", "")) - 1
                face = obj.Base[0][0].Shape.Faces[faceIndex]
                zStart = face.BoundBox.ZMax
            else:
                zStart = obj.Base[0][0].Shape.BoundBox.ZMax
        else:
            zStart = model.Shape.BoundBox.ZMax
            Path.Log.error("Base object not set")
        finalDepth = obj.FinalDepth.Value
        stepDown = abs(obj.StepDown.Value)

        return cls.FromTool(obj.ToolController.Tool, zStart, finalDepth, stepDown)


def _calculate_depth(MIC, geom):
    # given a maximum inscribed circle (MIC) and tool angle,
    # return depth of cut relative to zStart.
    depth = geom.start - round(MIC * geom.scale, 4)

    return max(depth, geom.maximumDepth) + geom.offset


def _get_maximumUsableDepth(wires, geom):
    """
    Calculate maximum engraving depth for a list of wires
    belonging to one face.
    """

    def _get_depth(MIC, geom):
        """Similar logic to _calculate_depth but without stepdown and offset calculations"""
        depth = geom.start - round(MIC * geom.scale, 4)
        return max(depth, geom.stop)

    min_depth = None

    for wire in wires:
        for edge in wire:
            dist = edge.getDistances()
            depth = min(_get_depth(dist[0], geom), _get_depth(dist[1], geom))

            if min_depth is None:
                min_depth = depth
            else:
                min_depth = min(min_depth, depth)

    return min_depth


def _getPartEdge(edge, geom):
    dist = edge.getDistances()
    zBegin = _calculate_depth(dist[0], geom)
    zEnd = _calculate_depth(dist[1], geom)
    return edge.toShape(zBegin, zEnd)


def _getPartEdges(obj, vWire, geom):
    edges = []
    for e in vWire:
        edges.append(_getPartEdge(e, geom))
    return edges


class ObjectVcarve(PathEngraveBase.ObjectOp):
    """Proxy class for Vcarve operation."""

    def opFeatures(self, obj):
        """opFeatures(obj) ... return all standard features and edges based geometries"""
        return (
            PathOp.FeatureTool
            | PathOp.FeatureHeights
            | PathOp.FeatureDepths
            | PathOp.FeatureStepDown
            | PathOp.FeatureBaseFaces
            | PathOp.FeatureCoolant
        )

    def setupAdditionalProperties(self, obj):
        if not hasattr(obj, "BaseShapes"):
            obj.addProperty(
                "App::PropertyLinkList",
                "BaseShapes",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Additional base objects to be engraved"),
            )
        obj.setEditorMode("BaseShapes", 2)  # hide

        if not hasattr(obj, "OptimizeMovements"):

            obj.addProperty(
                "App::PropertyBool",
                "OptimizeMovements",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Optimize movements"),
            )
            obj.OptimizeMovements = False

        if not hasattr(obj, "FinishingPass"):
            obj.addProperty(
                "App::PropertyBool",
                "FinishingPass",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Add finishing pass"),
            )
            obj.FinishingPass = False

        if not hasattr(obj, "FinishingPassZOffset"):
            obj.addProperty(
                "App::PropertyDistance",
                "FinishingPassZOffset",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Finishing pass Z offset"),
            )

            obj.FinishingPassZOffset = "0.00"

    def initOperation(self, obj):
        """initOperation(obj) ... create vcarve specific properties."""
        obj.addProperty(
            "App::PropertyFloat",
            "Discretize",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The deflection value for discretizing arcs"),
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
        obj.Discretize = 0.25
        obj.Tolerance = Path.Preferences.defaultGeometryTolerance()
        self.setupAdditionalProperties(obj)

    def opOnDocumentRestored(self, obj):
        # upgrade ...
        self.setupAdditionalProperties(obj)

    def buildMedialWires(self, obj, faces):
        """
        constructs a medial axis path using openvoronoi
        :returns: dictionary - each face object is a key containing list of wires"""

        wires_by_face = dict()
        self.voronoiDebugCache = dict()

        def is_exterior(vertex, face):
            vector = FreeCAD.Vector(vertex.toPoint(face.BoundBox.ZMin))
            (u, v) = face.Surface.parameter(vector)
            # isPartOfDomain is faster than face.IsInside(...)
            return not face.isPartOfDomain(u, v)

        def insert_many_wires(vd, wires):
            for wire in wires:
                Path.Log.debug("discretize value: {}".format(obj.Discretize))
                pts = wire.discretize(QuasiDeflection=obj.Discretize)
                ptv = [FreeCAD.Vector(p.x, p.y) for p in pts]
                # Check over the last point before just closing the polygon
                # by adding the start again.  If the discretizer was aiming
                # for the last point and missed by a little bit, closing the
                # polygon as is could result in OpenVoronoi truncating the
                # coordinates to a self-intersecting polygon which is invalid.
                # Instead, if the last point is close to the first, remove it
                # and let the final append close the polygon.
                # See issue 8064
                if len(ptv) > 0:
                    dist = ptv[-1].distanceToPoint(ptv[0])
                    if dist < FreeCAD.Base.Precision.confusion():
                        Path.Log.debug(
                            "Removing bad carve point: {} from polygon origin".format(dist)
                        )
                        del ptv[-1]
                ptv.append(ptv[0])

                for i in range(len(ptv) - 1):
                    vd.addSegment(ptv[i], ptv[i + 1])

        for f in faces:
            voronoiWires = []
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

            # filter our colinear edged so there are fewer ones
            # to iterate over in colorExterior which is slow
            vd.colorColinear(COLINEAR, obj.Colinear)

            vd.colorExterior(EXTERIOR1)
            vd.colorExterior(EXTERIOR2, lambda v: is_exterior(v, f))

            # if colorTwin is done before colorExterior we seem to have
            # much more weird exterior edges needed to be filtered out,
            # keep it here to be safe
            vd.colorTwins(TWIN)

            wires = _collectVoronoiWires(vd)
            wires = _sortVoronoiWires(wires)
            voronoiWires.extend(wires)

            wires_by_face[f] = voronoiWires
            self.voronoiDebugCache = wires_by_face

        return wires_by_face

    def buildCommandList(self, obj, faces):
        """
        Build command list to cut wires - based on voronoi
        wire list from buildMedialWires
        """

        def getCurrentPosition(wire):
            """
            Calculate CNC head position assuming it reached the end of the wire
            """

            if not wire:
                return None

            lastEdge = wire[-1]
            return lastEdge.valueAt(lastEdge.LastParameter)

        def cutWires(wires, pathlist, optimizeMovements=False):
            currentPosition = None
            for w in wires:
                pWire = _getPartEdges(obj, w, geom)
                if pWire:
                    pathlist.extend(_cutWire(pWire, currentPosition))

                    # movement optimization only works if we provide current head position
                    if optimizeMovements:
                        currentPosition = getCurrentPosition(pWire)

        def canSkipRepositioning(currentPosition, newPosition):
            """
            Calculate if it makes sense to raise head to safe height and reposition before
            starting to cut another edge
            """

            if not currentPosition:
                return False

            # get vertex position on X/Y plane only
            v0 = FreeCAD.Base.Vector(currentPosition.x, currentPosition.y)
            v1 = FreeCAD.Base.Vector(newPosition.x, newPosition.y)

            return v0.distanceToPoint(v1) <= 0.5

        def _cutWire(wire, currentPosition=None):
            path = []

            e = wire[0]
            newPosition = e.valueAt(e.FirstParameter)

            # raise and reposition the head only if new wire starts further than 0.5 mm
            # from current head position
            if not canSkipRepositioning(currentPosition, newPosition):
                path.append(Path.Command("G0 Z{}".format(obj.SafeHeight.Value)))
                path.append(
                    Path.Command(
                        "G0 X{} Y{} Z{}".format(newPosition.x, newPosition.y, obj.SafeHeight.Value)
                    )
                )

            hSpeed = obj.ToolController.HorizFeed.Value
            vSpeed = obj.ToolController.VertFeed.Value
            path.append(
                Path.Command(
                    "G1 X{} Y{} Z{} F{}".format(newPosition.x, newPosition.y, newPosition.z, vSpeed)
                )
            )
            for e in wire:
                path.extend(Path.Geom.cmdsForEdge(e, hSpeed=hSpeed, vSpeed=vSpeed))

            return path

        pathlist = []
        pathlist.append(Path.Command("(starting)"))

        geom = _Geometry.FromObj(obj, self.model[0])

        # iterate over each face separately
        for face, wires in self.buildMedialWires(obj, faces).items():

            # If using depth step-down, calculate maximum usable depth for current face.
            # This is done to avoid adding additional step-down engraving passes when it
            # would make no sense as depth is limited by Maximum Inscribed Circle anyway.

            maximumUsableDepth = geom.stop

            if geom.stepDown > 0:
                _maximumUsableDepth = _get_maximumUsableDepth(wires, geom)
                if _maximumUsableDepth is not None:
                    maximumUsableDepth = _maximumUsableDepth
                    Path.Log.debug(f"Maximum usable depth for current face: {maximumUsableDepth}")

            # first pass
            cutWires(wires, pathlist, obj.OptimizeMovements)

            # subsequent stepDown depth passes (if any)
            while geom.incrementStepDownDepth(maximumUsableDepth):
                cutWires(wires, pathlist, obj.OptimizeMovements)

            # add finishing pass if enabled

            if obj.FinishingPass:
                geom.offset = obj.FinishingPassZOffset.Value

                cutWires(wires, pathlist, obj.OptimizeMovements)

        self.commandlist = pathlist

    def opExecute(self, obj):
        """opExecute(obj) ... process engraving operation"""
        Path.Log.track()

        self.voronoiDebugCache = None

        if obj.ToolController is None:
            return

        if not hasattr(obj.ToolController.Tool, "CuttingEdgeAngle"):
            Path.Log.info(
                translate(
                    "CAM_Vcarve",
                    "VCarve requires an engraving cutter with a cutting edge angle",
                )
            )
            return

        if obj.ToolController.Tool.CuttingEdgeAngle >= 180.0:
            Path.Log.info(
                translate("CAM_Vcarve", "Engraver cutting edge angle must be < 180 degrees.")
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
                    if model.isDerivedFrom("Sketcher::SketchObject") or model.isDerivedFrom(
                        "Part::Part2DObject"
                    ):
                        faces.extend(model.Shape.Faces)

            if faces:
                self.buildCommandList(obj, faces)
            else:
                Path.Log.error(
                    translate(
                        "PathVcarve",
                        "The Job Base Object has no engraveable element. Engraving operation will produce no output.",
                    )
                )

        except Exception:
            Path.Log.warning(
                "Error processing Base object. Engraving operation will produce no output."
            )
            import traceback

            Path.Log.error(f"Engraving operation exception: {traceback.format_exc()}")

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

    def debugVoronoi(self, obj):
        """Debug function to display calculated voronoi edges"""

        if not getattr(self, "voronoiDebugCache", None):
            Path.Log.error("debugVoronoi: empty debug cache. Recompute VCarve operation first")
            return

        vPart = FreeCAD.activeDocument().addObject("App::Part", f"{obj.Name}-VoronoiDebug")

        wiresToShow = []

        for face, wires in self.voronoiDebugCache.items():
            for wire in wires:
                currentPartWire = Part.Wire()
                currentPartWire.fixTolerance(0.01)
                for edge in wire:
                    currentEdge = edge.toShape()

                    for v in currentEdge.Vertexes:
                        v.fixTolerance(0.1)

                    currentPartWire.add(currentEdge)
                wiresToShow.append(currentPartWire)

        for w in wiresToShow:
            vPart.addObject(Part.show(w))


def SetupProperties():
    return ["Discretize"]


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Vcarve operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectVcarve(obj, name, parentJob)
    return obj
