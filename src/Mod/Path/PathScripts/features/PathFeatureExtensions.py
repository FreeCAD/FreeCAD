# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 Markus Lampert (mlamptert)                         *
# *   Copyright (c) 2020 Russell Johnson (russ4262) <russ4262@gmail.com>    *
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
import math

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')
PathUtils = LazyLoader('PathScripts.PathUtils', globals(), 'PathScripts.PathUtils')

from PySide import QtCore

__title__ = "Path Features Extensions"
__author__ = "Markus Lampert (mlampert)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Class and implementation of face extensions features."

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
        if closest is None or dist > closest[0]:  # pylint: disable=unsubscriptable-object
            closest = (dist, w)

    if closest is not None:
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


def createExtension(obj, extObj, extFeature, extSub):
    return Extension(extObj,
                                extFeature,
                                extSub,
                                obj.ExtensionLengthDefault,
                                Extension.DirectionNormal)


def getExtensions(obj):
    extensions = []
    i = 0
    for extObj, features in obj.ExtensionFeature:
        for sub in features:
            extFeature, extSub = sub.split(':')
            extensions.append(createExtension(obj, extObj, extFeature, extSub))
            i = i + 1
    return extensions


def setExtensions(obj, extensions):
    PathLog.track(obj.Label, len(extensions))
    obj.ExtensionFeature = [(ext.obj, ext.getSubLink()) for ext in extensions]


class Extension(object):
    DirectionNormal = 0
    DirectionX      = 1
    DirectionY      = 2

    def __init__(self, obj, feature, sub, length, direction):
        PathLog.debug("Extension(%s, %s, %s, %.2f, %s" % (obj.Label, feature, sub, length, direction))
        self.obj = obj
        self.feature = feature
        self.sub = sub
        self.length = length
        self.direction = direction

        self.wire = None

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

    def getWire(self):
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

                    return Part.Wire([e3])

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
# Eclass


# All access extension code
def _get_all_access_face(obj, base_shape, face, extension, discretize_factor=1.0):
    """_get_all_access_face(obj, base_shape, face, extension, discretize_factor=1.0) ...
    Creates an extended face for the pocket, taking into consideration lateral
    collision with the greater base shape.
    Arguments are:
        path object, base shape of face, target face,
        extension magnitude, discretize factor
    Return is an all access face extending the specified extension value from the source face.
    """
    ofst_tolernc=1e-4

    # Discretize outer wire and convert to face to reduce number of edges in temp face
    # pnts = face.Wires[0].discretize(QuasiDeflection=discretize_factor)
    pnts = face.Wires[0].discretize(Deflection=discretize_factor)
    pnts.append(FreeCAD.Vector(pnts[1].x, pnts[1].y, pnts[1].z))
    pnt_cnt = len(pnts)
    segs = [Part.makeLine(pnts[i], pnts[i + 1]) for i in range(1, pnt_cnt - 1)]
    temp_face = Part.Face(Part.Wire(segs))

    # Make offset face per user-specified extension distance so as to allow full clearing of face where possible.
    offset_face = PathUtils.getOffsetArea(temp_face,
                                extension,
                                removeHoles=False,
                                plane=temp_face,
                                tolerance=ofst_tolernc)

    # Apply collision detection by limiting extended face using base shape
    offset_ext = offset_face.extrude(FreeCAD.Vector(0.0, 0.0, 1.0))
    face_del = offset_face.extrude(FreeCAD.Vector(0.0, 0.0, -1.0))
    clear = base_shape.cut(face_del)
    available = offset_ext.cut(clear)
    available.removeSplitter()

    # Identify bottom outer wire of available volume
    bottom_wire = None
    zmin = face.BoundBox.ZMin
    for w in available.Wires:
        if abs(w.BoundBox.ZMax - zmin) < 1e-6:
            if w.isClosed():
                bottom_wire = w
                break
    if bottom_wire:
        top_face = Part.Face(bottom_wire)
    else:
        return False

    # Respect holes in face if requested, restoring to all access face
    avoid_holes = True
    if hasattr(obj, "UseOutline"):
        avoid_holes = not obj.UseOutline
    wire_cnt = len(face.Wires)
    if avoid_holes and wire_cnt > 1:
        for i in range(1, wire_cnt):
            hole_face = Part.Face(face.Wires[i])
            cut = top_face.cut(hole_face)
            top_face = cut

    # Drop travel face to same height as source face
    diff = face.BoundBox.ZMax - top_face.BoundBox.ZMax
    top_face.translate(FreeCAD.Vector(0.0, 0.0, diff))

    return top_face


def initialize_properties(obj):
    """initialize_properties(obj)... Adds feature propeties to object argument"""
    if not hasattr(obj, 'ExtensionLengthDefault'):
        obj.addProperty('App::PropertyDistance', 'ExtensionLengthDefault', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Default length of extensions.'))
    if not hasattr(obj, 'ExtensionFeature'):
        obj.addProperty('App::PropertyLinkSubListGlobal', 'ExtensionFeature', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'List of features to extend.'))
    if not hasattr(obj, 'ExtensionCorners'):
        obj.addProperty('App::PropertyBool', 'ExtensionCorners', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'When enabled connected extension edges are combined to wires.'))
        obj.ExtensionCorners = True
    if not hasattr(obj, "AllAccessExtension"):
        obj.addProperty("App::PropertyLength",
                        "AllAccessExtension",
                        "Extension",
                        QtCore.QT_TRANSLATE_NOOP('PathPocketShape','Extends the face by this value where physically possible, enabling all physical access available to clear the face. A zero value disables and the suggested max value is tool diameter.'))

    obj.setEditorMode('ExtensionFeature', 2)


def set_default_property_values(obj, job):
    """set_default_property_values(obj, job) ... set default values for feature properties"""
    obj.ExtensionCorners = True
    obj.setExpression('ExtensionLengthDefault', 'OpToolDiameter / 2')
    obj.AllAccessExtension.Value = 0.0  # Zero value disables feature


def SetupProperties():
    """SetupProperties()... Returns list of feature property names"""
    setup = ['ExtensionLengthDefault', 'ExtensionFeature',
             'ExtensionCorners', 'AllAccessExtension']
    return setup
