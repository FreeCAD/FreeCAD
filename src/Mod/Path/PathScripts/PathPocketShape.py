# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathPocketBase as PathPocketBase

from PySide import QtCore

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')
TechDraw = LazyLoader('TechDraw', globals(), 'TechDraw')
math = LazyLoader('math', globals(), 'math')


__title__ = "Path Pocket Shape Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Class and implementation of shape based Pocket operation."


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


def endPoints(edgeOrWire):
    '''endPoints(edgeOrWire) ... return the first and last point of the wire or the edge, assuming the argument is not a closed wire.'''
    if Part.Wire == type(edgeOrWire):
        # edges = edgeOrWire.Edges
        pts = [e.valueAt(e.FirstParameter) for e in edgeOrWire.Edges]
        pts.extend([e.valueAt(e.LastParameter) for e in edgeOrWire.Edges])
        unique = []
        for p in pts:
            cnt = len([p2 for p2 in pts if PathGeom.pointsCoincide(p, p2)])
            if 1 == cnt:
                unique.append(p)
        return unique

    pfirst = edgeOrWire.valueAt(edgeOrWire.FirstParameter)
    plast = edgeOrWire.valueAt(edgeOrWire.LastParameter)
    if PathGeom.pointsCoincide(pfirst, plast):
        return None

    return [pfirst, plast]


def includesPoint(p, pts):
    '''includesPoint(p, pts) ... answer True if the collection of pts includes the point p'''
    for pt in pts:
        if PathGeom.pointsCoincide(p, pt):
            return True
    return False


def selectOffsetWire(feature, wires):
    '''selectOffsetWire(feature, wires) ... returns the Wire in wires which is does not intersect with feature'''
    closest = None
    for w in wires:
        dist = feature.distToShape(w)[0]
        if closest is None or dist > closest[0]:
            closest = (dist, w)
    if not closest is None:
        return closest[1]
    return None


def extendWire(feature, wire, length):
    '''extendWire(wire, length) ... return a closed Wire which extends wire by length'''
    PathLog.track(length)
    if length and length != 0:
        try:
            off2D = wire.makeOffset2D(length)
        except FreeCAD.Base.FreeCADError:
            return None

        endPts = endPoints(wire)
        if endPts:
            edges = [e for e in off2D.Edges if Part.Circle != type(e.Curve) or not includesPoint(e.Curve.Center, endPts)]
            wires = [Part.Wire(e) for e in Part.sortEdges(edges)]
            offset = selectOffsetWire(feature, wires)
            ePts = endPoints(offset)
            if ePts and len(ePts) > 1:
                l0 = (ePts[0] - endPts[0]).Length
                l1 = (ePts[1] - endPts[0]).Length
                edges = wire.Edges
                if l0 < l1:
                    edges.append(Part.Edge(Part.LineSegment(endPts[0], ePts[0])))
                    edges.extend(offset.Edges)
                    edges.append(Part.Edge(Part.LineSegment(endPts[1], ePts[1])))
                else:
                    edges.append(Part.Edge(Part.LineSegment(endPts[1], ePts[0])))
                    edges.extend(offset.Edges)
                    edges.append(Part.Edge(Part.LineSegment(endPts[0], ePts[1])))
                return Part.Wire(edges)
    return None


class Extension(object):
    DirectionNormal    = 0
    DirectionX         = 1
    DirectionY         = 2

    def __init__(self, obj, feature, sub, length, direction):
        PathLog.debug("Extension(%s, %s, %s, %.2f, %s" % (obj.Label, feature, sub, length, direction))
        self.obj = obj
        self.feature = feature
        self.sub = sub
        self.length = length
        self.direction = direction
        self.extFaces = list()

    def getSubLink(self):
        return "%s:%s" % (self.feature, self.sub)

    def _extendEdge(self, feature, e0, direction):
        PathLog.track(feature, e0, direction)
        if isinstance(e0.Curve, Part.Line) or isinstance(e0.Curve, Part.LineSegment):
            e2 = e0.copy()
            off = self.length.Value * direction
            e2.translate(off)
            e2 = PathGeom.flipEdge(e2)
            e1 = Part.Edge(Part.LineSegment(e0.valueAt(e0.LastParameter), e2.valueAt(e2.FirstParameter)))
            e3 = Part.Edge(Part.LineSegment(e2.valueAt(e2.LastParameter), e0.valueAt(e0.FirstParameter)))
            wire = Part.Wire([e0, e1, e2, e3])
            self.wire = wire
            return wire
        return extendWire(feature, Part.Wire([e0]), self.length.Value)

    def _getEdgeNumbers(self):
        if 'Wire' in self.sub:
            numbers = [nr for nr in self.sub[5:-1].split(',')]
        else:
            numbers = [self.sub[4:]]
        PathLog.debug("_getEdgeNumbers() -> %s" % numbers)

        return numbers

    def _getEdgeNames(self):
        return ["Edge%s" % nr for nr in self._getEdgeNumbers()]

    def _getEdges(self):
        return [self.obj.Shape.getElement(sub) for sub in self._getEdgeNames()]

    def _getDirectedNormal(self, p0, normal):
        poffPlus = p0 + 0.01 * normal
        poffMinus = p0 - 0.01 * normal
        if not self.obj.Shape.isInside(poffPlus, 0.005, True):
            return normal
        if not self.obj.Shape.isInside(poffMinus, 0.005, True):
            return normal.negative()
        return None

    def _getDirection(self, wire):
        e0 = wire.Edges[0]
        midparam = e0.FirstParameter + 0.5 * (e0.LastParameter - e0.FirstParameter)
        tangent = e0.tangentAt(midparam)
        PathLog.track('tangent', tangent, self.feature, self.sub)
        normal = tangent.cross(FreeCAD.Vector(0, 0, 1))

        if PathGeom.pointsCoincide(normal, FreeCAD.Vector(0, 0, 0)):
            return None

        return self._getDirectedNormal(e0.valueAt(midparam), normal.normalize())

    def getExtensionFaces(self, extensionWire):
        '''getExtensionFace(extensionWire)...
        A public helper method to retrieve the requested extension as a face,
        rather than a wire becuase some extensions require a face shape
        for definition that allows for two wires for boundary definition.
        '''

        if self.extFaces:
            return self.extFaces
        
        return [Part.Face(extensionWire)]

    def getWire(self):
        '''getWire()... Public method to retrieve the extension area, pertaining to the feature
        and sub element provided at class instantiation, as a closed wire.  If no closed wire
        is possible, a `None` value is returned.'''
        PathLog.track()
        if PathGeom.isRoughly(0, self.length.Value) or not self.sub:
            PathLog.debug("no extension, length=%.2f, sub=%s" % (self.length.Value, self.sub))
            return None

        feature = self.obj.Shape.getElement(self.feature)
        edges = self._getEdges()
        sub = Part.Wire(Part.sortEdges(edges)[0])

        if 1 == len(edges):
            PathLog.debug("Extending single edge wire")
            edge = edges[0]
            if Part.Circle == type(edge.Curve):
                circle = edge.Curve
                # for a circle we have to figure out if it's a hole or a cylinder
                p0 = edge.valueAt(edge.FirstParameter)
                normal = (edge.Curve.Center - p0).normalize()
                direction = self._getDirectedNormal(p0, normal)
                if direction is None:
                    return None

                if PathGeom.pointsCoincide(normal, direction):
                    r = circle.Radius - self.length.Value
                else:
                    r = circle.Radius + self.length.Value
                # assuming the offset produces a valid circle - go for it
                if r > 0:
                    e3 = Part.makeCircle(r, circle.Center, circle.Axis, edge.FirstParameter * 180 / math.pi, edge.LastParameter * 180 / math.pi)
                    if endPoints(edge):
                        # need to construct the arc slice
                        e0 = Part.makeLine(edge.valueAt(edge.FirstParameter), e3.valueAt(e3.FirstParameter))
                        e2 = Part.makeLine(edge.valueAt(edge.LastParameter), e3.valueAt(e3.LastParameter))
                        return Part.Wire([e0, edge, e2, e3])

                    extWire = Part.Wire([e3])
                    self.extFaces = [self._makeCircularExtFace(edge, extWire)]
                    return extWire

                # the extension is bigger than the hole - so let's just cover the whole hole
                if endPoints(edge):
                    # if the resulting arc is smaller than the radius, create a pie slice
                    PathLog.track()
                    center = circle.Center
                    e0 = Part.makeLine(center, edge.valueAt(edge.FirstParameter))
                    e2 = Part.makeLine(edge.valueAt(edge.LastParameter), center)
                    return Part.Wire([e0, edge, e2])
                PathLog.track()
                return Part.Wire([edge])

            else:
                PathLog.track(self.feature, self.sub, type(edge.Curve), endPoints(edge))
                direction = self._getDirection(sub)
                if direction is None:
                    return None
            #    return self._extendEdge(feature, edge, direction)
            return self._extendEdge(feature, edges[0], direction)
        return extendWire(feature, sub, self.length.Value)

    def _makeCircularExtFace(self, edge, extWire):
        '''_makeCircularExtensionFace(edge, extWire)...
        Create proper circular extension face shape. Incoming edge is expected to be a circle.
        '''
        # Add original outer wire to cut faces if necessary
        edgeFace = Part.Face(Part.Wire([edge]))
        edgeFace.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - edgeFace.BoundBox.ZMin))
        extWireFace = Part.Face(extWire)
        extWireFace.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - extWireFace.BoundBox.ZMin))

        if extWireFace.Area >= edgeFace.Area:
            extensionFace = extWireFace.cut(edgeFace)
        else:
            extensionFace = edgeFace.cut(extWireFace)
        extensionFace.translate(FreeCAD.Vector(0.0, 0.0, edge.BoundBox.ZMin))

        return extensionFace
# Eclass


class ObjectPocket(PathPocketBase.ObjectPocket):
    '''Proxy object for Pocket operation.'''

    def areaOpFeatures(self, obj):
        return super(self.__class__, self).areaOpFeatures(obj) | PathOp.FeatureLocations

    def initPocketOp(self, obj):
        '''initPocketOp(obj) ... setup receiver'''
        if not hasattr(obj, 'UseOutline'):
            obj.addProperty('App::PropertyBool', 'UseOutline', 'Pocket', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Uses the outline of the base geometry.'))
            obj.UseOutline = False
        if not hasattr(obj, 'ExtensionLengthDefault'):
            obj.addProperty('App::PropertyDistance', 'ExtensionLengthDefault', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Default length of extensions.'))
        if not hasattr(obj, 'ExtensionFeature'):
            obj.addProperty('App::PropertyLinkSubListGlobal', 'ExtensionFeature', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'List of features to extend.'))
        if not hasattr(obj, 'ExtensionCorners'):
            obj.addProperty('App::PropertyBool', 'ExtensionCorners', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'When enabled connected extension edges are combined to wires.'))
            obj.ExtensionCorners = True

        obj.setEditorMode('ExtensionFeature', 2)

    def areaOpOnDocumentRestored(self, obj):
        '''opOnDocumentRestored(obj) ... adds the UseOutline property if it doesn't exist.'''
        self.initPocketOp(obj)

    def pocketInvertExtraOffset(self):
        return False

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... return shapes representing the solids to be removed.'''
        PathLog.track()

        if obj.Base:
            PathLog.debug('base items exist.  Processing...')
            self.removalshapes = []
            self.horiz = []
            vertical = []
            for o in obj.Base:
                PathLog.debug('Base item: {}'.format(o))
                base = o[0]
                for sub in o[1]:
                    if 'Face' in sub:
                        face = base.Shape.getElement(sub)
                        if type(face.Surface) == Part.Plane and PathGeom.isVertical(face.Surface.Axis):
                            # it's a flat horizontal face
                            self.horiz.append(face)
                        elif type(face.Surface) == Part.Cylinder and PathGeom.isVertical(face.Surface.Axis):
                            # vertical cylinder wall
                            if any(e.isClosed() for e in face.Edges):
                                # complete cylinder
                                circle = Part.makeCircle(face.Surface.Radius, face.Surface.Center)
                                disk = Part.Face(Part.Wire(circle))
                                self.horiz.append(disk)
                            else:
                                # partial cylinder wall
                                vertical.append(face)
                        elif type(face.Surface) == Part.Plane and PathGeom.isHorizontal(face.Surface.Axis):
                            vertical.append(face)
                        else:
                            PathLog.error(translate('PathPocket', 'Pocket does not support shape %s.%s') % (base.Label, sub))

            self.vertical = PathGeom.combineConnectedShapes(vertical)
            self.vWires = [TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1)) for shape in self.vertical]
            for wire in self.vWires:
                w = PathGeom.removeDuplicateEdges(wire)
                face = Part.Face(w)
                face.tessellate(0.1)
                if PathGeom.isRoughly(face.Area, 0):
                    PathLog.error(translate('PathPocket', 'Vertical faces do not form a loop - ignoring'))
                else:
                    self.horiz.append(face)

            # add faces for extensions
            self.exts = [] # pylint: disable=attribute-defined-outside-init
            for ext in self.getExtensions(obj):
                wire = ext.getWire()
                if wire:
                    for face in ext.getExtensionFaces(wire):
                        self.horiz.append(face)
                        self.exts.append(face)

            # Place all self.horiz faces into same working plane
            for h in self.horiz:
                h.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - h.BoundBox.ZMin))

            # check all faces and see if they are touching/overlapping and combine those into a compound
            self.horizontal = []
            for shape in PathGeom.combineConnectedShapes(self.horiz):
                shape.sewShape()
                shape.tessellate(0.05)  # originally 0.1
                if obj.UseOutline:
                    wire = TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1))
                    wire.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - wire.BoundBox.ZMin))
                    self.horizontal.append(Part.Face(wire))
                else:
                    self.horizontal.append(shape)

            # extrude all faces up to StartDepth and those are the removal shapes
            extent = FreeCAD.Vector(0, 0, obj.StartDepth.Value - obj.FinalDepth.Value)
            self.removalshapes = [(face.removeSplitter().extrude(extent), False) for face in self.horizontal]

        else:  # process the job base object as a whole
            PathLog.debug("processing the whole job base object")
            self.outlines = [Part.Face(TechDraw.findShapeOutline(base.Shape, 1, FreeCAD.Vector(0, 0, 1))) for base in self.model]
            stockBB = self.stock.Shape.BoundBox

            self.removalshapes = []
            self.bodies = []
            for outline in self.outlines:
                outline.translate(FreeCAD.Vector(0, 0, stockBB.ZMin - 1))
                body = outline.extrude(FreeCAD.Vector(0, 0, stockBB.ZLength + 2))
                self.bodies.append(body)
                self.removalshapes.append((self.stock.Shape.cut(body), False))

        for (shape, hole) in self.removalshapes:
            shape.tessellate(0.05)  # originally 0.1

        if self.removalshapes:
            obj.removalshape = self.removalshapes[0][0]
        return self.removalshapes

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... set default values'''
        obj.StepOver = 100
        obj.ZigZagAngle = 45
        obj.UseOutline = False
        obj.ExtensionCorners = True
        if job and job.Stock:
            bb = job.Stock.Shape.BoundBox
            obj.OpFinalDepth = bb.ZMin
            obj.OpStartDepth = bb.ZMax
        obj.setExpression('ExtensionLengthDefault', 'OpToolDiameter / 2')

    def createExtension(self, obj, extObj, extFeature, extSub):
        return Extension(extObj, extFeature, extSub, obj.ExtensionLengthDefault, Extension.DirectionNormal)

    def getExtensions(self, obj):
        extensions = []
        i = 0
        for extObj, features in obj.ExtensionFeature:
            for sub in features:
                extFeature, extSub = sub.split(':')
                extensions.append(self.createExtension(obj, extObj, extFeature, extSub))
                i = i + 1
        return extensions

    def setExtensions(self, obj, extensions):
        PathLog.track(obj.Label, len(extensions))
        obj.ExtensionFeature = [(ext.obj, ext.getSubLink()) for ext in extensions]


def SetupProperties():
    setup = PathPocketBase.SetupProperties()
    setup.append('UseOutline')
    setup.append('ExtensionLengthDefault')
    setup.append('ExtensionFeature')
    setup.append('ExtensionCorners')
    return setup


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Pocket operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject('Path::FeaturePython', name)

    obj.Proxy = ObjectPocket(obj, name)

    return obj