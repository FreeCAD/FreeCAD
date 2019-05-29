# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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

# SCRIPT NOTES:
# - Need test models for testing vertical faces scenarios. Currently, I think they will fail with rotation.
# - Need to group VERTICAL faces per axis_angle tag just like horizontal faces.
#       Then, need to run each grouping through 
#       PathGeom.combineConnectedShapes(vertical) algorithm grouping
# - Need to add face boundbox analysis code to vertical axis_angle
#       section to identify highest zMax for all faces included in group
# - Need to implement judgeStartDepth() within rotational depth calculations
# - FUTURE: Re-iterate PathAreaOp.py need to relocate rotational settings 
#       to Job setup, under Machine settings tab

import FreeCAD
import Part
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathPocketBase as PathPocketBase
# import PathScripts.PathUtil as PathUtil
# import PathScripts.PathUtils as PathUtils
import TechDraw
import math
# import sys

from PySide import QtCore

__title__ = "Path Pocket Shape Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of shape based Pocket operation."
__contributors__ = "russ4262 (Russell Johnson)"
__created__ = "2017"
__scriptVersion__ = "1i testing"
__lastModified__ = "2019-05-06 16:55 CST"

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


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
    return [e.valueAt(edgeOrWire.FirstParameter), e.valueAt(edgeOrWire.LastParameter)]


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
    if closest is not None:
        return closest[1]
    return None


def extendWire(feature, wire, length):
    '''extendWire(wire, length) ... return a closed Wire which extends wire by length'''
    try:
        off2D = wire.makeOffset2D(length)
    except Exception as e:
        msg = "\nThe selected face cannot be used.\nYou must select the bottom face of the pocket area.\nextendWire() in PathPocketShape.py"
        PathLog.error(e)
        PathLog.error(msg)
        return False
    else:
        endPts = endPoints(wire)
        edges = [e for e in off2D.Edges if Part.Circle != type(e.Curve) or not includesPoint(e.Curve.Center, endPts)]
        wires = [Part.Wire(e) for e in Part.sortEdges(edges)]
        offset = selectOffsetWire(feature, wires)
        ePts = endPoints(offset)
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


class Extension(object):
    DirectionNormal = 0
    DirectionX = 1
    DirectionY = 2

    def __init__(self, obj, feature, sub, length, direction):
        self.obj = obj
        self.feature = feature
        self.sub = sub
        self.length = length
        self.direction = direction

    def getSubLink(self):
        return "%s:%s" % (self.feature, self.sub)

    def extendEdge(self, feature, e0, direction):
        if Part.Line == type(e0.Curve) or Part.LineSegment == type(e0.Curve):
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

    def getEdgeNumbers(self):
        if 'Wire' in self.sub:
            return [nr for nr in self.sub[5:-1].split(',')]
        return [self.sub[4:]]

    def getEdgeNames(self):
        return ["Edge%s" % nr for nr in self.getEdgeNumbers()]

    def getEdges(self):
        return [self.obj.Shape.getElement(sub) for sub in self.getEdgeNames()]

    def getDirection(self, wire):
        e0 = wire.Edges[0]
        midparam = e0.FirstParameter + 0.5 * (e0.LastParameter - e0.FirstParameter)
        tangent = e0.tangentAt(midparam)
        normal = tangent.cross(FreeCAD.Vector(0, 0, 1)).normalize()
        poffPlus = e0.valueAt(midparam) + 0.01 * normal
        poffMinus = e0.valueAt(midparam) - 0.01 * normal
        if not self.obj.Shape.isInside(poffPlus, 0.005, True):
            return normal
        if not self.obj.Shape.isInside(poffMinus, 0.005, True):
            return normal.negative()
        return None

    def getWire(self):
        if PathGeom.isRoughly(0, self.length.Value) or not self.sub:
            return None

        feature = self.obj.Shape.getElement(self.feature)
        edges = self.getEdges()
        sub = Part.Wire(edges)

        if 1 == len(edges):
            direction = self.getDirection(sub)
            if direction is None:
                return None
            return self.extendEdge(feature, edges[0], direction)
        return extendWire(feature, sub, self.length.Value)


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
        if not hasattr(obj, 'B_AxisErrorOverride'):
            obj.addProperty('App::PropertyBool', 'B_AxisErrorOverride', 'Path', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Match B rotations to model (error in FreeCAD rendering).'))
            obj.B_AxisErrorOverride = False
        if not hasattr(obj, 'ReverseDirection'):
            obj.addProperty('App::PropertyBool', 'ReverseDirection', 'Path', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Reverse direction of pocket operation.'))
            obj.ReverseDirection = False

        obj.setEditorMode('ExtensionFeature', 2)

    def opOnDocumentRestored(self, obj):
        '''opOnDocumentRestored(obj) ... adds the UseOutline property if it doesn't exist.'''
        self.initPocketOp(obj)

    def pocketInvertExtraOffset(self):
        return False

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... return shapes representing the solids to be removed.'''
        PathLog.track()
        PathLog.debug("areaOpShapes() in PathPocketShape.py")

        def judgeFinalDepth(obj, fD):
            if obj.FinalDepth.Value >= fD:
                return obj.FinalDepth.Value
            else:
                return fD

        def judgeStartDepth(obj, sD):
            if obj.StartDepth.Value >= sD:
                return obj.StartDepth.Value
            else:
                return sD

        def analyzeVerticalFaces(self, obj, vertTuples):
            hT = []
            # base = FreeCAD.ActiveDocument.getObject(self.modelName)

            # Separate elements, regroup by orientation (axis_angle combination)
            vTags = ['X34.2']
            vGrps = [[(2.3, 3.4, 'X')]]
            for tup in vertTuples:
                (face, sub, angle, axis, tag, strDep, finDep, trans) = tup
                if tag in vTags:
                    # Determine index of found string
                    i = 0
                    for orn in vTags:
                        if orn == tag:
                            break
                        i += 1
                    vGrps[i].append(tup)
                else:
                    vTags.append(tag)  # add orientation entry
                    vGrps.append([tup])  # add orientation entry
            # Remove temp elements
            vTags.pop(0)
            vGrps.pop(0)

            # check all faces in each axis_angle group
            shpList = []
            zmaxH = 0.0
            for o in range(0, len(vTags)):
                shpList = []
                zmaxH = vGrps[o][0].BoundBox.ZMax
                for (face, sub, angle, axis, tag, strDep, finDep, trans) in vGrps[o]:
                    shpList.append(face)
                    # Identify tallest face to use as zMax
                    if face.BoundBox.ZMax > zmaxH:
                        zmaxH = face.BoundBox.ZMax
                # check all faces and see if they are touching/overlapping and combine those into a compound
                # Original Code in For loop
                self.vertical = PathGeom.combineConnectedShapes(shpList)
                self.vWires = [TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1)) for shape in self.vertical]
                for wire in self.vWires:
                    w = PathGeom.removeDuplicateEdges(wire)
                    face = Part.Face(w)
                    face.tessellate(0.05)
                    if PathGeom.isRoughly(face.Area, 0):
                        PathLog.error(translate('PathPocket', 'Vertical faces do not form a loop - ignoring'))
                    else:
                        strDep = zmaxH + self.leadIn  # base.Shape.BoundBox.ZMax
                        finDep = judgeFinalDepth(obj, face.BoundBox.ZMin)
                        tup = face, sub, angle, axis, tag, strDep, finDep, trans
                        hT.append(tup)
            # Eol
            return hT

        if obj.Base:
            PathLog.debug('base items exist.  Processing...')
            self.removalshapes = []
            self.horiz = []
            vertical = []
            horizTuples = []
            vertTuples = []
            axis = 'X'
            angle = 0.0
            reset = False
            resetPlacement = None
            trans = FreeCAD.Vector(0.0, 0.0, 0.0)

            for o in obj.Base:
                PathLog.debug('Base item: {}'.format(o))
                base = o[0]

                # Limit sub faces to children of single Model object.
                if self.modelName is None:
                    self.modelName = base.Name
                else:
                    if base.Name != self.modelName:
                        for sub in o[1]:
                            PathLog.error(sub + " is not a part of Model object: " + self.modelName)
                        o[1] = []
                        PathLog.error("Only processing faces on a single Model object per operation.")
                        PathLog.error("You will need to separate faces per Model object within the Job.")

                startBase = FreeCAD.Vector(base.Placement.Base.x, base.Placement.Base.y, base.Placement.Base.z)
                startAngle = base.Placement.Rotation.Angle
                startAxis = base.Placement.Rotation.Axis
                startRotation = FreeCAD.Rotation(startAxis, startAngle)
                resetPlacement = FreeCAD.Placement(startBase, startRotation)
                for sub in o[1]:
                    if 'Face' in sub:
                        PathLog.debug('sub: {}'.format(sub))
                        # Determine angle of rotation needed to make normal vector = (0,0,1)
                        strDep = obj.StartDepth.Value
                        finDep = obj.FinalDepth.Value
                        trans = FreeCAD.Vector(0.0, 0.0, 0.0)
                        rtn = False

                        if obj.UseRotation != 'Off':
                            (rtn, angle, axis) = self.pocketRotationAnalysis(obj, base, sub, prnt=True)

                        if rtn is True:
                            reset = True
                            PathLog.debug(str(sub) + ": rotating model to make face normal at (0,0,1) ...")
                            if axis == 'X':
                                bX = 0.0
                                bY = 0.0
                                bZ = math.sin(math.radians(angle)) * base.Placement.Base.y
                                vect = FreeCAD.Vector(1, 0, 0)
                            elif axis == 'Y':
                                bX = 0.0
                                bY = 0.0
                                bZ = math.sin(math.radians(angle)) * base.Placement.Base.x
                                if obj.B_AxisErrorOverride is True:
                                    bZ = -1 * bZ
                                vect = FreeCAD.Vector(0, 1, 0)
                            # Rotate base to such that Surface.Axis of pocket bottom is Z=1
                            base.Placement.Rotation = FreeCAD.Rotation(vect, angle)
                            base.recompute()
                            trans = FreeCAD.Vector(bX, bY, bZ)
                        else:
                            axis = 'X'
                            angle = 0.0
                        tag = axis + str(round(angle, 7))
                        face = base.Shape.getElement(sub)

                        if type(face.Surface) == Part.Plane and PathGeom.isVertical(face.Surface.Axis):
                            # it's a flat horizontal face
                            PathLog.debug(" == Part.Plane: isVertical")
                            # Adjust start and finish depths for pocket
                            strDep = base.Shape.BoundBox.ZMax + self.leadIn
                            finDep = judgeFinalDepth(obj, face.BoundBox.ZMin)
                            # Over-write default final depth value, leaves manual override by user
                            obj.StartDepth.Value = trans.z + strDep
                            obj.FinalDepth.Value = trans.z + finDep

                            tup = face, sub, angle, axis, tag, strDep, finDep, trans
                            horizTuples.append(tup)
                        elif type(face.Surface) == Part.Cylinder and PathGeom.isVertical(face.Surface.Axis):
                            PathLog.debug("== Part.Cylinder")
                            # vertical cylinder wall
                            if any(e.isClosed() for e in face.Edges):
                                PathLog.debug("e.isClosed()")
                                # complete cylinder
                                circle = Part.makeCircle(face.Surface.Radius, face.Surface.Center)
                                disk = Part.Face(Part.Wire(circle))

                                # Adjust start and finish depths for pocket
                                strDep = face.BoundBox.ZMax + self.leadIn  # base.Shape.BoundBox.ZMax + self.leadIn
                                finDep = judgeFinalDepth(obj, face.BoundBox.ZMin)
                                # Over-write default final depth value, leaves manual override by user
                                obj.StartDepth.Value = trans.z + strDep
                                obj.FinalDepth.Value = trans.z + finDep

                                tup = disk, sub, angle, axis, tag, strDep, finDep, trans
                                horizTuples.append(tup)
                            else:
                                # partial cylinder wall
                                vertical.append(face)

                                # Adjust start and finish depths for pocket
                                strDep = face.BoundBox.ZMax + self.leadIn  # base.Shape.BoundBox.ZMax + self.leadIn
                                finDep = judgeFinalDepth(obj, face.BoundBox.ZMin)
                                # Over-write default final depth value, leaves manual override by user
                                obj.StartDepth.Value = trans.z + strDep
                                obj.FinalDepth.Value = trans.z + finDep
                                tup = face, sub, angle, axis, tag, strDep, finDep, trans
                                vertTuples.append(tup)

                                PathLog.debug(sub + "is vertical after rotation.")
                        elif type(face.Surface) == Part.Plane and PathGeom.isHorizontal(face.Surface.Axis):
                            vertical.append(face)

                            # Adjust start and finish depths for pocket
                            strDep = face.BoundBox.ZMax + self.leadIn  # base.Shape.BoundBox.ZMax + self.leadIn
                            finDep = judgeFinalDepth(obj, face.BoundBox.ZMin)
                            # Over-write default final depth value, leaves manual override by user
                            obj.StartDepth.Value = trans.z + strDep
                            obj.FinalDepth.Value = trans.z + finDep
                            tup = face, sub, angle, axis, tag, strDep, finDep, trans
                            vertTuples.append(tup)
                            PathLog.debug(sub + "is vertical after rotation.")
                        else:
                            PathLog.error(translate('PathPocket', 'Pocket does not support shape %s.%s') % (base.Label, sub))

                        if reset is True:
                            base.Placement.Rotation = startRotation
                            base.recompute()
                            reset = False
                    # End IF
                # End FOR
                base.Placement = resetPlacement
                base.recompute()
            # End FOR

            # Analyze vertical faces via PathGeom.combineConnectedShapes()
            # hT = analyzeVerticalFaces(self, obj, vertTuples)
            # horizTuples.extend(hT)

            # This section will be replaced analyzeVerticalFaces(self, obj, vertTuples) above
            self.vertical = PathGeom.combineConnectedShapes(vertical)
            self.vWires = [TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0.0, 0.0, 1.0)) for shape in self.vertical]
            for wire in self.vWires:
                w = PathGeom.removeDuplicateEdges(wire)
                face = Part.Face(w)
                face.tessellate(0.05)
                if PathGeom.isRoughly(face.Area, 0):
                    PathLog.error(translate('PathPocket', 'Vertical faces do not form a loop - ignoring'))
                else:
                    # self.horiz.append(face)
                    strDep = base.Shape.BoundBox.ZMax + self.leadIn
                    finDep = judgeFinalDepth(obj, face.BoundBox.ZMin)
                    tup = face, 'vertFace', 0.0, 'X', 'X0.0', strDep, finDep, FreeCAD.Vector(0.0, 0.0, 0.0)
                    horizTuples.append(tup)

            # add faces for extensions
            self.exts = []
            for ext in self.getExtensions(obj):
                wire = Part.Face(ext.getWire())
                if wire:
                    face = Part.Face(wire)
                    # self.horiz.append(face)
                    strDep = base.Shape.BoundBox.ZMax + self.leadIn
                    finDep = judgeFinalDepth(obj, face.BoundBox.ZMin)
                    tup = face, 'vertFace', 0.0, 'X', 'X0.0', strDep, finDep, FreeCAD.Vector(0.0, 0.0, 0.0)
                    horizTuples.append(tup)
                    self.exts.append(face)

            # move all horizontal faces to FinalDepth
            for (face, sub, angle, axis, tag, strDep, finDep, trans) in horizTuples:
                # face.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - face.BoundBox.ZMin))
                if angle <= 0.0:
                    if axis == 'X':
                        face.translate(FreeCAD.Vector(0, trans.z, trans.z + finDep - face.BoundBox.ZMin))
                    elif axis == 'Y':
                        face.translate(FreeCAD.Vector(-1 * trans.z, 0, trans.z + finDep - face.BoundBox.ZMin))
                else:
                    if axis == 'X':
                        face.translate(FreeCAD.Vector(0, -1 * trans.z, trans.z + finDep - face.BoundBox.ZMin))
                    elif axis == 'Y':
                        face.translate(FreeCAD.Vector(trans.z, 0, trans.z + finDep - face.BoundBox.ZMin))

            # Separate elements, regroup by orientation (axis_angle combination)
            hTags = ['X34.2']
            hGrps = [[(2.3, 3.4, 'X')]]
            for tup in horizTuples:
                (face, sub, angle, axis, tag, strDep, finDep, trans) = tup
                if tag in hTags:
                    # Determine index of found string
                    i = 0
                    for orn in hTags:
                        if orn == tag:
                            break
                        i += 1
                    hGrps[i].append(tup)
                else:
                    hTags.append(tag)  # add orientation entry
                    hGrps.append([tup])  # add orientation entry
            # Remove temp elements
            hTags.pop(0)
            hGrps.pop(0)

            # check all faces in each axis_angle group
            self.horizontal = []
            shpList = []
            for o in range(0, len(hTags)):
                PathLog.debug('hTag: {}'.format(hTags[o]))
                shpList = []
                for (face, sub, angle, axis, tag, strDep, finDep, trans) in hGrps[o]:
                    shpList.append(face)
                # check all faces and see if they are touching/overlapping and combine those into a compound
                # Original Code in For loop
                for shape in PathGeom.combineConnectedShapes(shpList):
                    shape.sewShape()
                    # shape.tessellate(0.05) # Russ4262 0.1 original
                    if obj.UseOutline:
                        wire = TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1))
                        wire.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - wire.BoundBox.ZMin))
                        PathLog.debug(" -obj.UseOutline: obj.FinalDepth.Value" + str(obj.FinalDepth.Value))
                        PathLog.debug(" -obj.UseOutline: wire.BoundBox.ZMin" + str(wire.BoundBox.ZMin))
                        # shape.tessellate(0.05) # Russ4262 0.1 original
                        face = Part.Face(wire)
                        tup = face, sub, angle, axis, tag, strDep, finDep, trans
                        self.horizontal.append(tup)
                    else:
                        # Re-pair shape to tuple set
                        for (face, sub, angle, axis, tag, strDep, finDep, trans) in hGrps[o]:
                            if shape is face:
                                tup = face, sub, angle, axis, tag, strDep, finDep, trans
                                self.horizontal.append(tup)
                                break
            # Eol

            # extrude all faces up to StartDepth and those are the removal shapes
            for (face, sub, angle, axis, tag, strDep, finDep, trans) in self.horizontal:
                # extent = FreeCAD.Vector(0, 0, obj.StartDepth.Value - obj.FinalDepth.Value)
                extent = FreeCAD.Vector(0, 0, strDep - finDep)
                shp = face.removeSplitter().extrude(extent)
                # tup = shp, False, sub, angle, axis, tag, strDep, finDep, trans
                tup = shp, False, sub, angle, axis  # shape, isHole, sub, angle, axis
                self.removalshapes.append(tup)

        else:  # process the job base object as a whole
            PathLog.debug("processing the whole job base object")
            self.outlines = [Part.Face(TechDraw.findShapeOutline(base.Shape, 1, FreeCAD.Vector(0, 0, 1))) for base in self.model]
            stockBB = self.stock.Shape.BoundBox
            PathLog.debug(" -Using outlines; no obj.Base")

            self.removalshapes = []
            self.bodies = []
            for outline in self.outlines:
                outline.translate(FreeCAD.Vector(0, 0, stockBB.ZMin - 1))
                body = outline.extrude(FreeCAD.Vector(0, 0, stockBB.ZLength + 2))
                self.bodies.append(body)
                self.removalshapes.append((self.stock.Shape.cut(body), False, 'outline', 0.0, 'X'))

        for (shape, isHole, sub, angle, axis) in self.removalshapes:
            shape.tessellate(0.05)

        if self.removalshapes:
            obj.removalshape = self.removalshapes[0][0]
        return self.removalshapes

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... set default values'''

        obj.StepOver = 100
        obj.ZigZagAngle = 45
        obj.B_AxisErrorOverride = False
        obj.ReverseDirection = False
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
    return PathPocketBase.SetupProperties() + ['UseOutline', 'ExtensionCorners']


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Pocket operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject('Path::FeaturePython', name)
    proxy = ObjectPocket(obj, name)
    return obj
