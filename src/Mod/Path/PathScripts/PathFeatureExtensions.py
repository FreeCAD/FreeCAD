# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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
import Part
import math

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
PathUtils = LazyLoader('PathScripts.PathUtils', globals(), 'PathScripts.PathUtils')

from PySide import QtCore

__title__ = "Path Features Extensions"
__author__ = "sliptonic (Brad Collette)"
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

    if not length or length == 0:
        return None
    
    try:
        off2D = wire.makeOffset2D(length)
    except FreeCAD.Base.FreeCADError as ee:
        return None
    endPts = endPoints(wire)  # Assumes wire is NOT closed
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
    return Extension(obj, 
                     extObj,
                     extFeature,
                     extSub,
                     obj.ExtensionLengthDefault,
                     Extension.DirectionNormal)


def readObjExtensionFeature(obj):
    """readObjExtensionFeature(obj)...
    Return three item string tuples (base name, feature, subfeature) extracted from obj.ExtensionFeature"""
    extensions = []

    for extObj, features in obj.ExtensionFeature:
        for sub in features:
            extFeature, extSub = sub.split(':')
            extensions.append((extObj.Name, extFeature, extSub))
    return extensions


def getExtensions(obj):
    PathLog.debug("getExtenstions()")
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

    def __init__(self, op, obj, feature, sub, length, direction):
        PathLog.debug("Extension(%s, %s, %s, %.2f, %s" % (obj.Label, feature, sub, length, direction))
        self.op = op
        self.obj = obj
        self.feature = feature
        self.sub = sub
        self.length = length
        self.direction = direction
        self.extFaces = None
        self.isDebug = True if PathLog.getLevel(PathLog.thisModule()) == 4 else False

        self.avoid = False
        if sub.startswith("Avoid_"):
            self.avoid = True

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

    def getExtensionFaces(self, extensionWire):
        '''getExtensionFace(extensionWire)...
        A public helper method to retrieve the requested extension as a face,
        rather than a wire because some extensions require a face shape
        for definition that allows for two wires for boundary definition.
        '''

        if self.extFaces:
            return self.extFaces
        
        return [Part.Face(extensionWire)]

    def getWire(self):
        '''getWire()... Public method to retrieve the extension area, pertaining to the feature
        and sub element provided at class instantiation, as a closed wire.  If no closed wire
        is possible, a `None` value is returned.'''

        if self.sub[:6] == "Avoid_":
            feature = self.obj.Shape.getElement(self.feature)
            self.extFaces = [Part.Face(feature.Wires[0])]
            return feature.Wires[0]
        if self.sub[:7] == "Extend_":
            rtn = self._getOutlineWire()
            if rtn:
                return rtn
            else:
                PathLog.debug("no Outline Wire")
                return None
        if self.sub[:10] == "Waterline_":
            rtn = self._getWaterlineWire()
            if rtn:
                return rtn
            else:
                PathLog.debug("no Waterline Wire")
                return None
        else:
            return self._getRegularWire()

    def _getRegularWire(self):
        '''_getRegularWire()... Private method to retrieve the extension area, pertaining to the feature
        and sub element provided at class instantiation, as a closed wire.  If no closed wire
        is possible, a `None` value is returned.'''
        PathLog.track()

        length = self.length.Value
        if PathGeom.isRoughly(0, length) or not self.sub:
            PathLog.debug("no extension, length=%.2f, sub=%s" % (length, self.sub))
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
                    r = circle.Radius - length
                else:
                    r = circle.Radius + length

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
        elif sub.isClosed():
            PathLog.debug("Extending multi-edge closed wire")
            subFace = Part.Face(sub)
            featFace = Part.Face(feature.Wires[0])
            isOutside = True
            if not PathGeom.isRoughly(featFace.Area, subFace.Area):
                length = -1.0 * length
                isOutside = False

            try:
                off2D = sub.makeOffset2D(length)
            except FreeCAD.Base.FreeCADError as ee:
                return None
            
            if isOutside:
                self.extFaces = [Part.Face(off2D).cut(featFace)]
            else:
                self.extFaces = [subFace.cut(Part.Face(off2D))]
            return off2D

        PathLog.debug("Extending multi-edge open wire")
        return extendWire(feature, sub, length)

    def _getOutlineWire(self):
        '''_getOutlineWire()... Private method to retrieve an extended outline extension area,
        pertaining to the feature and sub element provided at class instantiation, as a closed wire.
        If no closed wire is possible, a `None` value is returned.'''
        PathLog.track()

        baseShape = self.obj.Shape
        face = baseShape.getElement(self.feature)
        useOutline = False
        msg = translate("PathAdaptive", "Extend Outline error")

        if hasattr(self.op, "UseOutline"):
            useOutline = self.op.UseOutline

        if useOutline:
            outFace = Part.Face(face.Wires[0])
            rawFace = getExtendOutlineFace(baseShape, outFace, self.length)

            if rawFace:
                extFace = rawFace.cut(outFace)
            else:
                PathLog.error(msg + " 1")
                extFace = outFace
        else:
            rawFace = getExtendOutlineFace(baseShape, face, self.length)

            if rawFace:
                extFace = rawFace.cut(face)
            else:
                PathLog.error(msg + " 2")
                extFace = face

        # Debug
        # Part.show(extFace)
        # FreeCAD.ActiveDocument.ActiveObject.Label = "outline_wire"

        if len(extFace.Wires) > 0:
            self.extFaces = [f for f in extFace.Faces]
            return extFace.Wires[0]

        return None

    def _getWaterlineWire(self):
        '''_getWaterlineWire()... Private method to retrieve a waterline extension area,
        pertaining to the feature and sub element provided at class instantiation, as a closed wire.
        Only waterline faces touching source face are returned as part of the waterline extension area.
        If no closed wire is possible, a `None` value is returned.'''
        PathLog.track()

        msg = translate("PathFeatureExtensions", "Waterline error")
        useOutline = False
        if hasattr(self.op, "UseOutline"):
            useOutline = self.op.UseOutline

        baseShape = self.obj.Shape
        if useOutline:
            face = Part.Face(baseShape.getElement(self.feature).Wire1)
        else:
            face = baseShape.getElement(self.feature)

        rawFace = getWaterlineFace(baseShape, face)

        if not rawFace:
            PathLog.error(msg + " 1")
            return None

        if rawFace:
            extFace = rawFace.cut(face)
        else:
            PathLog.error(msg + " 2")
            extFace = face

        # Debug
        # Part.show(extFace)
        # FreeCAD.ActiveDocument.ActiveObject.Label = "waterline_face"

        if len(extFace.Wires) > 0:
            self.extFaces = [f for f in extFace.Faces]
            return extFace.Wires[0]

        return None

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


def initialize_properties(obj):
    """initialize_properties(obj)... Adds feature properties to object argument"""
    if not hasattr(obj, 'ExtensionLengthDefault'):
        obj.addProperty('App::PropertyDistance', 'ExtensionLengthDefault', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Default length of extensions.'))
    if not hasattr(obj, 'ExtensionFeature'):
        obj.addProperty('App::PropertyLinkSubListGlobal', 'ExtensionFeature', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'List of features to extend.'))
    if not hasattr(obj, 'ExtensionCorners'):
        obj.addProperty('App::PropertyBool', 'ExtensionCorners', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'When enabled connected extension edges are combined to wires.'))
        obj.ExtensionCorners = True

    obj.setEditorMode('ExtensionFeature', 2)


def set_default_property_values(obj, job):
    """set_default_property_values(obj, job) ... set default values for feature properties"""
    obj.ExtensionCorners = True
    obj.setExpression('ExtensionLengthDefault', 'OpToolDiameter / 2.0')


def SetupProperties():
    """SetupProperties()... Returns list of feature property names"""
    setup = ['ExtensionLengthDefault', 'ExtensionFeature',
             'ExtensionCorners']
    return setup


# Extend outline face generation function
def getExtendOutlineFace(base_shape, face, extension, remHoles=False, offset_tolerance=1e-4):
    """getExtendOutlineFace(obj, base_shape, face, extension, remHoles) ...
    Creates an extended face for the pocket, taking into consideration lateral
    collision with the greater base shape.
    Arguments are:
        parent base shape of face,
        target face,
        extension magnitude,
        remove holes boolean,
        offset tolerance = 1e-4 default
    The default value of 1e-4 for offset tolerance is the same default value
    at getOffsetArea() function definition.
    Return is an all access face extending the specified extension value from the source face.
    """

    # Make offset face per user-specified extension distance so as to allow full clearing of face where possible.
    offset_face = PathUtils.getOffsetArea(face,
                                extension,
                                removeHoles=remHoles,
                                plane=face,
                                tolerance=offset_tolerance)
    if not offset_face:
        PathLog.error("Failed to offset a selected face.")
        return None

    # Apply collision detection by limiting extended face using base shape
    depth = 0.2
    offset_ext = offset_face.extrude(FreeCAD.Vector(0.0, 0.0, depth))
    face_del = offset_face.extrude(FreeCAD.Vector(0.0, 0.0, -1.0 * depth))
    clear = base_shape.cut(face_del)
    available = offset_ext.cut(clear)
    available.removeSplitter()

    # Debug
    # Part.show(available)
    # FreeCAD.ActiveDocument.ActiveObject.Label = "available"

    # Identify bottom face of available volume
    zmin = available.BoundBox.ZMax
    bottom_faces = list()
    for f in available.Faces:
        bbx = f.BoundBox
        zNorm = abs(f.normalAt(0.0, 0.0).z)
        if (PathGeom.isRoughly(zNorm, 1.0) and
            PathGeom.isRoughly(bbx.ZMax - bbx.ZMin, 0.0) and
            PathGeom.isRoughly(bbx.ZMin, face.BoundBox.ZMin)):
            if bbx.ZMin < zmin:
                bottom_faces.append(f)

    if bottom_faces:
        extended = None
        for bf in bottom_faces:
            # Drop travel face to same height as source face
            diff = face.BoundBox.ZMax - bf.BoundBox.ZMax
            bf.translate(FreeCAD.Vector(0.0, 0.0, diff))
            cmn = bf.common(face)
            if hasattr(cmn, "Area") and cmn.Area > 0.0:
                extended = bf

        return extended

    PathLog.error("No bottom face for extend outline.")
    return None

# Waterline extension face generation function
def getWaterlineFace(base_shape, face):
    """getWaterlineFace(base_shape, face) ...
    Creates a waterline extension face for the target face,
    taking into consideration the greater base shape.
    Arguments are: parent base shape and target face.
    Return is a waterline face at height of the target face.
    """
    faceHeight = face.BoundBox.ZMin

    # Get envelope of model to height of face, then fuse with model and refine the shape
    baseBB = base_shape.BoundBox
    depthparams = PathUtils.depth_params(
        clearance_height=faceHeight,
        safe_height=faceHeight,
        start_depth=faceHeight,
        step_down=math.floor(faceHeight - baseBB.ZMin + 2.0),
        z_finish_step=0.0,
        final_depth=baseBB.ZMin,
        user_depths=None)
    env = PathUtils.getEnvelope(partshape=base_shape, subshape=None, depthparams=depthparams)
    # Get top face(s) of envelope at face height
    rawList = list()
    for f in env.Faces:
        if PathGeom.isRoughly(f.BoundBox.ZMin, faceHeight):
            rawList.append(f)
    # make compound and extrude downward
    rawComp = Part.makeCompound(rawList)
    rawCompExtNeg = rawComp.extrude(FreeCAD.Vector(0.0, 0.0, baseBB.ZMin - faceHeight - 1.0))
    # Cut off bottom of base shape at face height
    topSolid = base_shape.cut(rawCompExtNeg)

    # Get intersection with base shape
    # The commented version returns waterlines that only intersects horizontal faces at same height as target face
    # cmn = base_shape.common(rawComp)
    # waterlineShape = cmn.cut(topSolid)
    # return waterlineShape

    # This version returns more of a true waterline flowing from target face
    waterlineShape = rawComp.cut(topSolid)
    faces = list()
    for f in waterlineShape.Faces:
        cmn = face.common(f)
        if hasattr(cmn, "Area") and cmn.Area > 0.0:
            faces.append(f)
    if faces:
        return Part.makeCompound(faces)

    return None
