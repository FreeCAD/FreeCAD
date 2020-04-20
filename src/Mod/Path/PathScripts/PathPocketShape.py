# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2020 Schildkroet                                        *
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
import PathScripts.PathUtils as PathUtils
import math

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Draft = LazyLoader('Draft', globals(), 'Draft')
Part = LazyLoader('Part', globals(), 'Part')
TechDraw = LazyLoader('TechDraw', globals(), 'TechDraw')

from PySide import QtCore

__title__ = "Path Pocket Shape Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
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
        if closest is None or dist > closest[0]:  # pylint: disable=unsubscriptable-object
            closest = (dist, w)

    if closest is not None:
        return closest[1]

    return None


def extendWire(feature, wire, length):
    '''extendWire(wire, length) ... return a closed Wire which extends wire by length'''
    PathLog.track(length)
    if length and length != 0:
        off2D = wire.makeOffset2D(length)
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


class ObjectPocket(PathPocketBase.ObjectPocket):
    '''Proxy object for Pocket operation.'''

    def areaOpFeatures(self, obj):
        return super(ObjectPocket, self).areaOpFeatures(obj) | PathOp.FeatureLocations

    def initPocketOp(self, obj):
        '''initPocketOp(obj) ... setup receiver'''
        if not hasattr(obj, 'UseOutline'):
            obj.addProperty('App::PropertyBool', 'UseOutline', 'Pocket', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Uses the outline of the base geometry.'))
        if not hasattr(obj, 'ExtensionLengthDefault'):
            obj.addProperty('App::PropertyDistance', 'ExtensionLengthDefault', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Default length of extensions.'))
        if not hasattr(obj, 'ExtensionFeature'):
            obj.addProperty('App::PropertyLinkSubListGlobal', 'ExtensionFeature', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'List of features to extend.'))
        if not hasattr(obj, 'ExtensionCorners'):
            obj.addProperty('App::PropertyBool', 'ExtensionCorners', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'When enabled connected extension edges are combined to wires.'))
            obj.ExtensionCorners = True

        obj.setEditorMode('ExtensionFeature', 2)
        self.initRotationOp(obj)

    def initRotationOp(self, obj):
        '''initRotationOp(obj) ... setup receiver for rotation'''
        if not hasattr(obj, 'ReverseDirection'):
            obj.addProperty('App::PropertyBool', 'ReverseDirection', 'Rotation', QtCore.QT_TRANSLATE_NOOP('App::Property', 'Reverse direction of pocket operation.'))
        if not hasattr(obj, 'InverseAngle'):
            obj.addProperty('App::PropertyBool', 'InverseAngle', 'Rotation', QtCore.QT_TRANSLATE_NOOP('App::Property', 'Inverse the angle. Example: -22.5 -> 22.5 degrees.'))
        if not hasattr(obj, 'AttemptInverseAngle'):
            obj.addProperty('App::PropertyBool', 'AttemptInverseAngle', 'Rotation', QtCore.QT_TRANSLATE_NOOP('App::Property', 'Attempt the inverse angle for face access if original rotation fails.'))
        if not hasattr(obj, 'LimitDepthToFace'):
            obj.addProperty('App::PropertyBool', 'LimitDepthToFace', 'Rotation', QtCore.QT_TRANSLATE_NOOP('App::Property', 'Enforce the Z-depth of the selected face as the lowest value for final depth. Higher user values will be observed.'))

    def areaOpOnChanged(self, obj, prop):
        '''areaOpOnChanged(obj, porp) ... process operation specific changes to properties.'''
        if prop == 'EnableRotation':
            self.setEditorProperties(obj)

    def setEditorProperties(self, obj):
        if obj.EnableRotation == 'Off':
            obj.setEditorMode('ReverseDirection', 2)
            obj.setEditorMode('InverseAngle', 2)
            obj.setEditorMode('AttemptInverseAngle', 2)
            obj.setEditorMode('LimitDepthToFace', 2)
        else:
            obj.setEditorMode('ReverseDirection', 0)
            obj.setEditorMode('InverseAngle', 0)
            obj.setEditorMode('AttemptInverseAngle', 0)
            obj.setEditorMode('LimitDepthToFace', 0)

    def areaOpOnDocumentRestored(self, obj):
        '''opOnDocumentRestored(obj) ... adds the UseOutline property, others, if they doesn't exist.'''
        self.initPocketOp(obj)
        self.setEditorProperties(obj)

    def pocketInvertExtraOffset(self):
        return False

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... return shapes representing the solids to be removed.'''
        PathLog.track()
        PathLog.debug("----- areaOpShapes() in PathPocketShape.py")

        baseSubsTuples = []
        subCount = 0
        allTuples = []

        def planarFaceFromExtrusionEdges(face, trans):
            useFace = 'useFaceName'
            minArea = 0.0
            fCnt = 0
            clsd = []
            planar = False
            # Identify closed edges
            for edg in face.Edges:
                if edg.isClosed():
                    PathLog.debug('  -e.isClosed()')
                    clsd.append(edg)
                    planar = True

            # Attempt to create planar faces and select that with smallest area for use as pocket base
            if planar is True:
                planar = False
                for edg in clsd:
                    fCnt += 1
                    fName = sub + '_face_' + str(fCnt)
                    # Create planar face from edge
                    mFF = Part.Face(Part.Wire(Part.__sortEdges__([edg])))
                    if mFF.isNull():
                        PathLog.debug('Face(Part.Wire()) failed')
                    else:
                        if trans is True:
                            mFF.translate(FreeCAD.Vector(0, 0, face.BoundBox.ZMin - mFF.BoundBox.ZMin))

                        if FreeCAD.ActiveDocument.getObject(fName):
                            FreeCAD.ActiveDocument.removeObject(fName)

                        tmpFace = FreeCAD.ActiveDocument.addObject('Part::Feature', fName).Shape = mFF
                        tmpFace = FreeCAD.ActiveDocument.getObject(fName)
                        tmpFace.purgeTouched()

                        if minArea == 0.0:
                            minArea = tmpFace.Shape.Face1.Area
                            useFace = fName
                            planar = True
                        elif tmpFace.Shape.Face1.Area < minArea:
                            minArea = tmpFace.Shape.Face1.Area
                            FreeCAD.ActiveDocument.removeObject(useFace)
                            useFace = fName
                        else:
                            FreeCAD.ActiveDocument.removeObject(fName)

            if useFace != 'useFaceName':
                self.useTempJobClones(useFace)

            return (planar, useFace)

        def clasifySub(self, bs, sub):
            face = bs.Shape.getElement(sub)

            if type(face.Surface) == Part.Plane:
                PathLog.debug('type() == Part.Plane')
                if PathGeom.isVertical(face.Surface.Axis):
                    PathLog.debug('  -isVertical()')
                    # it's a flat horizontal face
                    self.horiz.append(face)
                    return True

                elif PathGeom.isHorizontal(face.Surface.Axis):
                    PathLog.debug('  -isHorizontal()')
                    self.vert.append(face)
                    return True

                else:
                    return False

            elif type(face.Surface) == Part.Cylinder and PathGeom.isVertical(face.Surface.Axis):
                PathLog.debug('type() == Part.Cylinder')
                # vertical cylinder wall
                if any(e.isClosed() for e in face.Edges):
                    PathLog.debug('  -e.isClosed()')
                    # complete cylinder
                    circle = Part.makeCircle(face.Surface.Radius, face.Surface.Center)
                    disk = Part.Face(Part.Wire(circle))
                    disk.translate(FreeCAD.Vector(0, 0, face.BoundBox.ZMin - disk.BoundBox.ZMin))
                    self.horiz.append(disk)
                    return True

                else:
                    PathLog.debug('  -none isClosed()')
                    # partial cylinder wall
                    self.vert.append(face)
                    return True

            elif type(face.Surface) == Part.SurfaceOfExtrusion:
                # extrusion wall
                PathLog.debug('type() == Part.SurfaceOfExtrusion')
                # Attempt to extract planar face from surface of extrusion
                (planar, useFace) = planarFaceFromExtrusionEdges(face, trans=True)
                # Save face object to self.horiz for processing or display error
                if planar is True:
                    uFace = FreeCAD.ActiveDocument.getObject(useFace)
                    self.horiz.append(uFace.Shape.Faces[0])
                    msg = translate('Path', "<b>Verify depth of pocket for '{}'.</b>".format(sub))
                    msg += translate('Path', "\n<br>Pocket is based on extruded surface.")
                    msg += translate('Path', "\n<br>Bottom of pocket might be non-planar and/or not normal to spindle axis.")
                    msg += translate('Path', "\n<br>\n<br><i>3D pocket bottom is NOT available in this operation</i>.")
                    PathLog.warning(msg)
                    # title = translate('Path', 'Depth Warning')
                    # self.guiMessage(title, msg, False)
                else:
                    PathLog.error(translate("Path", "Failed to create a planar face from edges in {}.".format(sub)))

            else:
                PathLog.debug('  -type(face.Surface): {}'.format(type(face.Surface)))
                return False

        if obj.Base:
            PathLog.debug('Processing... obj.Base')
            self.removalshapes = []  # pylint: disable=attribute-defined-outside-init

            if obj.EnableRotation == 'Off':
                stock = PathUtils.findParentJob(obj).Stock
                for (base, subList) in obj.Base:
                    baseSubsTuples.append((base, subList, 0.0, 'X', stock))
            else:
                for p in range(0, len(obj.Base)):
                    (base, subsList) = obj.Base[p]
                    isLoop = False

                    # First, check all subs collectively for loop of faces
                    if len(subsList) > 2:
                        (isLoop, norm, surf) = self.checkForFacesLoop(base, subsList)

                    if isLoop is True:
                        PathLog.debug("Common Surface.Axis or normalAt() value found for loop faces.")
                        rtn = False
                        subCount += 1
                        (rtn, angle, axis, praInfo) = self.faceRotationAnalysis(obj, norm, surf)  # pylint: disable=unused-variable
                        PathLog.debug("angle: {};  axis: {}".format(angle, axis))

                        if rtn is True:
                            faceNums = ""
                            for f in subsList:
                                faceNums += '_' + f.replace('Face', '')
                            (clnBase, angle, clnStock, tag) = self.applyRotationalAnalysis(obj, base, angle, axis, faceNums)  # pylint: disable=unused-variable

                            # Verify faces are correctly oriented - InverseAngle might be necessary
                            PathLog.debug("Checking if faces are oriented correctly after rotation...")
                            for sub in subsList:
                                face = clnBase.Shape.getElement(sub)
                                if type(face.Surface) == Part.Plane:
                                    if not PathGeom.isHorizontal(face.Surface.Axis):
                                        rtn = False
                                        PathLog.warning(translate("PathPocketShape", "Face appears to NOT be horizontal AFTER rotation applied."))
                                        break

                            if rtn is False:
                                PathLog.debug(translate("Path", "Face appears misaligned after initial rotation."))
                                if obj.InverseAngle is False:
                                    if obj.AttemptInverseAngle is True:
                                        (clnBase, clnStock, angle) = self.applyInverseAngle(obj, clnBase, clnStock, axis, angle)
                                    else:
                                        msg = translate("Path", "Consider toggling the 'InverseAngle' property and recomputing.")
                                        PathLog.warning(msg)

                            if angle < 0.0:
                                angle += 360.0

                            tup = clnBase, subsList, angle, axis, clnStock
                        else:
                            if self.warnDisabledAxis(obj, axis) is False:
                                PathLog.debug("No rotation used")
                            axis = 'X'
                            angle = 0.0
                            stock = PathUtils.findParentJob(obj).Stock
                            tup = base, subsList, angle, axis, stock
                        # Eif

                        allTuples.append(tup)
                        baseSubsTuples.append(tup)
                    # Eif

                    if isLoop is False:
                        PathLog.debug(translate('Path', "Processing subs individually ..."))
                        for sub in subsList:
                            subCount += 1
                            if 'Face' in sub:
                                rtn = False
                                face = base.Shape.getElement(sub)
                                if type(face.Surface) == Part.SurfaceOfExtrusion:
                                    # extrusion wall
                                    PathLog.debug('analyzing type() == Part.SurfaceOfExtrusion')
                                    # Attempt to extract planar face from surface of extrusion
                                    (planar, useFace) = planarFaceFromExtrusionEdges(face, trans=False)
                                    # Save face object to self.horiz for processing or display error
                                    if planar is True:
                                        base = FreeCAD.ActiveDocument.getObject(useFace)
                                        sub = 'Face1'
                                        PathLog.debug('  -successful face created: {}'.format(useFace))
                                    else:
                                        PathLog.error(translate("Path", "Failed to create a planar face from edges in {}.".format(sub)))

                                (norm, surf) = self.getFaceNormAndSurf(face)
                                (rtn, angle, axis, praInfo) = self.faceRotationAnalysis(obj, norm, surf)  # pylint: disable=unused-variable
                                PathLog.debug("initial {}".format(praInfo))

                                if rtn is True:
                                    faceNum = sub.replace('Face', '')
                                    (clnBase, angle, clnStock, tag) = self.applyRotationalAnalysis(obj, base, angle, axis, faceNum)
                                    # Verify faces are correctly oriented - InverseAngle might be necessary
                                    faceIA = clnBase.Shape.getElement(sub)
                                    (norm, surf) = self.getFaceNormAndSurf(faceIA)
                                    (rtn, praAngle, praAxis, praInfo2) = self.faceRotationAnalysis(obj, norm, surf)  # pylint: disable=unused-variable
                                    PathLog.debug("follow-up {}".format(praInfo2))

                                    if abs(praAngle) == 180.0:
                                        rtn = False
                                        if self.isFaceUp(clnBase, faceIA) is False:
                                            PathLog.debug('isFaceUp is False')
                                            angle -= 180.0

                                    if rtn is True:
                                        PathLog.debug(translate("Path", "Face appears misaligned after initial rotation."))
                                        if obj.InverseAngle is False:
                                            if obj.AttemptInverseAngle is True:
                                                (clnBase, clnStock, angle) = self.applyInverseAngle(obj, clnBase, clnStock, axis, angle)
                                            else:
                                                msg = translate("Path", "Consider toggling the 'InverseAngle' property and recomputing.")
                                                PathLog.warning(msg)

                                            if self.isFaceUp(clnBase, faceIA) is False:
                                                PathLog.debug('isFaceUp is False')
                                                angle += 180.0
                                    else:
                                        PathLog.debug("Face appears to be oriented correctly.")

                                    if angle < 0.0:
                                        angle += 360.0

                                    tup = clnBase, [sub], angle, axis, clnStock
                                else:
                                    if self.warnDisabledAxis(obj, axis) is False:
                                        PathLog.debug(str(sub) + ": No rotation used")
                                    axis = 'X'
                                    angle = 0.0
                                    stock = PathUtils.findParentJob(obj).Stock
                                    tup = base, [sub], angle, axis, stock
                                # Eif
                                allTuples.append(tup)
                                baseSubsTuples.append(tup)
                            else:
                                ignoreSub = base.Name + '.' + sub
                                PathLog.error(translate('Path', "Selected feature is not a Face. Ignoring: {}".format(ignoreSub)))

            for o in baseSubsTuples:
                self.horiz = []  # pylint: disable=attribute-defined-outside-init
                self.vert = []  # pylint: disable=attribute-defined-outside-init
                subBase = o[0]
                subsList = o[1]
                angle = o[2]
                axis = o[3]
                stock = o[4]

                for sub in subsList:
                    if 'Face' in sub:
                        if clasifySub(self, subBase, sub) is False:
                            PathLog.error(translate('PathPocket', 'Pocket does not support shape %s.%s') % (subBase.Label, sub))
                            if obj.EnableRotation != 'Off':
                                PathLog.warning(translate('PathPocket', 'Face might not be within rotation accessibility limits.'))

                # Determine final depth as highest value of bottom boundbox of vertical face,
                #   in case of uneven faces on bottom
                if len(self.vert) > 0:
                    vFinDep = self.vert[0].BoundBox.ZMin
                    for vFace in self.vert:
                        if vFace.BoundBox.ZMin > vFinDep:
                            vFinDep = vFace.BoundBox.ZMin
                    # Determine if vertical faces for a loop: Extract planar loop wire as new horizontal face.
                    self.vertical = PathGeom.combineConnectedShapes(self.vert) # pylint: disable=attribute-defined-outside-init
                    self.vWires = [TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1)) for shape in self.vertical] # pylint: disable=attribute-defined-outside-init
                    for wire in self.vWires:
                        w = PathGeom.removeDuplicateEdges(wire)
                        face = Part.Face(w)
                        # face.tessellate(0.1)
                        if PathGeom.isRoughly(face.Area, 0):
                            msg = translate('PathPocket', 'Vertical faces do not form a loop - ignoring')
                            PathLog.error(msg)
                            # title = translate("Path", "Face Selection Warning")
                            # self.guiMessage(title, msg, True)
                        else:
                            face.translate(FreeCAD.Vector(0, 0, vFinDep - face.BoundBox.ZMin))
                            self.horiz.append(face)
                            msg = translate('Path', 'Verify final depth of pocket shaped by vertical faces.')
                            PathLog.warning(msg)
                            # title = translate('Path', 'Depth Warning')
                            # self.guiMessage(title, msg, False)

                # add faces for extensions
                self.exts = [] # pylint: disable=attribute-defined-outside-init
                for ext in self.getExtensions(obj):
                    wire = ext.getWire()
                    if wire:
                        face = Part.Face(wire)
                        self.horiz.append(face)
                        self.exts.append(face)

                # move all horizontal faces to FinalDepth
                # for f in self.horiz:
                #     f.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - f.BoundBox.ZMin))

                # check all faces and see if they are touching/overlapping and combine those into a compound
                self.horizontal = [] # pylint: disable=attribute-defined-outside-init
                for shape in PathGeom.combineConnectedShapes(self.horiz):
                    shape.sewShape()
                    # shape.tessellate(0.1)
                    shpZMin = shape.BoundBox.ZMin
                    PathLog.debug('PathGeom.combineConnectedShapes shape.BoundBox.ZMin: {}'.format(shape.BoundBox.ZMin))
                    if obj.UseOutline:
                        wire = TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1))
                        wFace = Part.Face(wire)
                        if wFace.BoundBox.ZMin != shpZMin:
                            wFace.translate(FreeCAD.Vector(0, 0, shpZMin - wFace.BoundBox.ZMin))
                        self.horizontal.append(wFace)
                        PathLog.debug('PathGeom.combineConnectedShapes shape.BoundBox.ZMin: {}'.format(wFace.BoundBox.ZMin))
                    else:
                        self.horizontal.append(shape)

                # extrude all faces up to StartDepth and those are the removal shapes
                sD = obj.StartDepth.Value
                fD = obj.FinalDepth.Value
                clrnc = 0.5
                for face in self.horizontal:
                    afD = fD
                    useAngle = angle
                    shpZMin = face.BoundBox.ZMin
                    PathLog.debug('self.horizontal shpZMin: {}'.format(shpZMin))
                    if self.isFaceUp(subBase, face) is False:
                        useAngle += 180.0
                        invZ = (-2 * shpZMin) - clrnc
                        face.translate(FreeCAD.Vector(0.0, 0.0, invZ))
                        shpZMin = -1 * shpZMin
                    else:
                        face.translate(FreeCAD.Vector(0.0, 0.0, -1 * clrnc))
                    
                    if obj.LimitDepthToFace is True and obj.EnableRotation != 'Off':
                        if shpZMin > obj.FinalDepth.Value:
                            afD = shpZMin
                            if sD <= afD:
                                sD = afD + 1.0
                                msg = translate('PathPocketShape', 'Start Depth is lower than face depth. Setting to ')
                                PathLog.warning(msg + ' {} mm.'.format(sD))
                    else:
                        face.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - shpZMin))
                    
                    extent = FreeCAD.Vector(0, 0, sD - afD + clrnc)
                    extShp = face.removeSplitter().extrude(extent)
                    self.removalshapes.append((extShp, False, 'pathPocketShape', useAngle, axis, sD, afD))
                    PathLog.debug("Extent values are strDep: {}, finDep: {},  extrd: {}".format(sD, afD, extent))
                # Efor face
            # Efor

        else:
            # process the job base object as a whole
            PathLog.debug(translate("Path", 'Processing model as a whole ...'))
            finDep = obj.FinalDepth.Value
            strDep = obj.StartDepth.Value
            self.outlines = [Part.Face(TechDraw.findShapeOutline(base.Shape, 1, FreeCAD.Vector(0, 0, 1))) for base in self.model] # pylint: disable=attribute-defined-outside-init
            stockBB = self.stock.Shape.BoundBox

            self.removalshapes = [] # pylint: disable=attribute-defined-outside-init
            self.bodies = [] # pylint: disable=attribute-defined-outside-init
            for outline in self.outlines:
                outline.translate(FreeCAD.Vector(0, 0, stockBB.ZMin - 1))
                body = outline.extrude(FreeCAD.Vector(0, 0, stockBB.ZLength + 2))
                self.bodies.append(body)
                self.removalshapes.append((self.stock.Shape.cut(body), False, 'pathPocketShape', 0.0, 'X', strDep, finDep))

        for (shape, hole, sub, angle, axis, strDep, finDep) in self.removalshapes: # pylint: disable=unused-variable
            shape.tessellate(0.05)  # originally 0.1

        if self.removalshapes:
            obj.removalshape = self.removalshapes[0][0]

        return self.removalshapes

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... set default values'''
        obj.StepOver = 100
        obj.ZigZagAngle = 45
        obj.ExtensionCorners = True
        obj.UseOutline = False
        obj.ReverseDirection = False
        obj.InverseAngle = False
        obj.AttemptInverseAngle = True
        obj.LimitDepthToFace = True
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

    def checkForFacesLoop(self, base, subsList):
        '''checkForFacesLoop(base, subsList)...
            Accepts a list of face names for the given base.
            Checks to determine if they are looped together.
        '''
        PathLog.track()
        fCnt = 0
        go = True
        vertLoopFace = None
        tempNameList = []
        delTempNameList = 0
        saSum = FreeCAD.Vector(0.0, 0.0, 0.0)
        norm = FreeCAD.Vector(0.0, 0.0, 0.0)
        surf = FreeCAD.Vector(0.0, 0.0, 0.0)
        precision = 6

        def makeTempExtrusion(base, sub, fCnt):
            extName = 'tmpExtrude' + str(fCnt)
            wireName = 'tmpWire' + str(fCnt)
            wr = Part.Wire(Part.__sortEdges__(base.Shape.getElement(sub).Edges))
            if wr.isNull():
                PathLog.debug('No wire created from {}'.format(sub))
                return (False, 0, 0)
            else:
                tmpWire = FreeCAD.ActiveDocument.addObject('Part::Feature', wireName).Shape = wr
                tmpWire = FreeCAD.ActiveDocument.getObject(wireName)
                tmpExt = FreeCAD.ActiveDocument.addObject('Part::Extrusion', extName)
                tmpExt = FreeCAD.ActiveDocument.getObject(extName)
                tmpExt.Base = tmpWire
                tmpExt.DirMode = "Normal"
                tmpExt.DirLink = None
                tmpExt.LengthFwd = 10.0
                tmpExt.LengthRev = 0.0
                tmpExt.Solid = True
                tmpExt.Reversed = False
                tmpExt.Symmetric = False
                tmpExt.TaperAngle = 0.0
                tmpExt.TaperAngleRev = 0.0

                tmpExt.recompute()
                tmpExt.purgeTouched()
                tmpWire.purgeTouched()
                return (True, tmpWire, tmpExt)

        def roundValue(precision, val):
            # Convert VALxe-15 numbers to zero
            if PathGeom.isRoughly(0.0, val) is True:
                return 0.0
            # Convert VAL.99999999 to next integer
            elif math.fabs(val % 1) > 1.0 - PathGeom.Tolerance:
                return round(val)
            else:
                return round(val, precision)

        # Determine precision from Tolerance
        for i in range(0, 13):
            if PathGeom.Tolerance * (i * 10) == 1.0:
                precision = i
                break

        # Sub Surface.Axis values of faces
        # Vector of (0, 0, 0) will suggests a loop
        for sub in subsList:
            if 'Face' in sub:
                fCnt += 1
                saSum = saSum.add(base.Shape.getElement(sub).Surface.Axis)

        # Minimim of three faces required for loop to exist
        if fCnt < 3:
            go = False

        # Determine if all faces combined point toward loop center = False
        if PathGeom.isRoughly(0, saSum.x):
            if PathGeom.isRoughly(0, saSum.y):
                if PathGeom.isRoughly(0, saSum.z):
                    PathLog.debug("Combined subs suggest loop of faces. Checking ...")
                    go = True

        if go is True:
            lastExtrusion = None
            matchList = []
            go = False

            # Cycle through subs, extruding to solid for each
            for sub in subsList:
                if 'Face' in sub:
                    fCnt += 1
                    go = False

                    # Extrude face to solid
                    (rtn, tmpWire, tmpExt) = makeTempExtrusion(base, sub, fCnt)

                    # If success, record new temporary objects for deletion
                    if rtn is True:
                        tempNameList.append(tmpExt.Name)
                        tempNameList.append(tmpWire.Name)
                        delTempNameList += 1
                        if lastExtrusion is None:
                            lastExtrusion = tmpExt
                            rtn = True
                    else:
                        go = False
                        break

                    # Cycle through faces on each extrusion, looking for common normal faces for rotation analysis
                    if len(matchList) == 0:
                        for fc in lastExtrusion.Shape.Faces:
                            (norm, raw) = self.getFaceNormAndSurf(fc)
                            rnded = FreeCAD.Vector(roundValue(precision, raw.x), roundValue(precision, raw.y), roundValue(precision, raw.z))
                            if rnded.x == 0.0 or rnded.y == 0.0 or rnded.z == 0.0:
                                for fc2 in tmpExt.Shape.Faces:
                                    (norm2, raw2) = self.getFaceNormAndSurf(fc2) # pylint: disable=unused-variable
                                    rnded2 = FreeCAD.Vector(roundValue(precision, raw2.x), roundValue(precision, raw2.y), roundValue(precision, raw2.z))
                                    if rnded == rnded2:
                                        matchList.append(fc2)
                                        go = True
                    else:
                        for m in matchList:
                            (norm, raw) = self.getFaceNormAndSurf(m)
                            rnded = FreeCAD.Vector(roundValue(precision, raw.x), roundValue(precision, raw.y), roundValue(precision, raw.z))
                            for fc2 in tmpExt.Shape.Faces:
                                (norm2, raw2) = self.getFaceNormAndSurf(fc2)
                                rnded2 = FreeCAD.Vector(roundValue(precision, raw2.x), roundValue(precision, raw2.y), roundValue(precision, raw2.z))
                                if rnded.x == 0.0 or rnded.y == 0.0 or rnded.z == 0.0:
                                    if rnded == rnded2:
                                        go = True
                        # Eif
                    if go is False:
                        break
                    # Eif
                # Eif 'Face'
            # Efor
        if go is True:
            go = False
            if len(matchList) == 2:
                saTotal = FreeCAD.Vector(0.0, 0.0, 0.0)
                for fc in matchList:
                    (norm, raw) = self.getFaceNormAndSurf(fc)
                    rnded = FreeCAD.Vector(roundValue(precision, raw.x), roundValue(precision, raw.y), roundValue(precision, raw.z))
                    if (rnded.y > 0.0 or rnded.z > 0.0) and vertLoopFace is None:
                        vertLoopFace = fc
                    saTotal = saTotal.add(rnded)

                if saTotal == FreeCAD.Vector(0.0, 0.0, 0.0):
                    if vertLoopFace is not None:
                        go = True

        if go is True:
            (norm, surf) = self.getFaceNormAndSurf(vertLoopFace)
        else:
            PathLog.debug(translate('Path', 'Can not identify loop.'))

        if delTempNameList > 0:
            for tmpNm in tempNameList:
                FreeCAD.ActiveDocument.removeObject(tmpNm)

        return (go, norm, surf)


def SetupProperties():
    setup = PathPocketBase.SetupProperties()
    setup.append('UseOutline')
    setup.append('ExtensionLengthDefault')
    setup.append('ExtensionFeature')
    setup.append('ExtensionCorners')
    setup.append("ReverseDirection")
    setup.append("InverseAngle")
    setup.append("AttemptInverseAngle")
    setup.append("LimitDepthToFace")
    return setup


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Pocket operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject('Path::FeaturePython', name)
    obj.Proxy = ObjectPocket(obj, name)
    return obj
