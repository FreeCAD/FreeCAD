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
# - Need test models for testing vertical faces scenarios.
# - Need to group VERTICAL faces per axis_angle tag just like horizontal faces.
# Then, need to run each grouping through 
# PathGeom.combineConnectedShapes(vertical) algorithm grouping
# - Need to add face boundbox analysis code to vertical axis_angle
# section to identify highest zMax for all faces included in group
# - Need to implement judgeStartDepth() within rotational depth calculations
# - FUTURE: Re-iterate PathAreaOp.py need to relocate rotational settings 
# to Job setup, under Machine settings tab

import FreeCAD
import Part
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathPocketBase as PathPocketBase
import TechDraw
import math

from PySide import QtCore

__title__ = "Path Pocket Shape Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of shape based Pocket operation."
__contributors__ = "mlampert [FreeCAD], russ4262 (Russell Johnson)"
__created__ = "2017"
__scriptVersion__ = "2b testing"
__lastModified__ = "2019-06-03 03:18 CST"

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
        PathLog.error("error: extendWire() off2D")
        PathLog.error(e)
        return False
    else:
        endPts = endPoints(wire)
        edges = [e for e in off2D.Edges if Part.Circle != type(e.Curve) or not includesPoint(e.Curve.Center, endPts)]
        wires = [Part.Wire(e) for e in Part.sortEdges(edges)]
        offset = selectOffsetWire(feature, wires)
        ePts = endPoints(offset)
        try:
            l0 = (ePts[0] - endPts[0]).Length
        except Exception as ee:
            PathLog.error("error: extendWire() l0")
            PathLog.error(ee)
            return False
        else:
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
        try:
            normal = tangent.cross(FreeCAD.Vector(0, 0, 1)).normalize()
        except:
            PathLog.error(translate('PathPocket', 'Unable to getDirection(wire).'))
            return None
        else:
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
            # obj.ExtensionCorners = True

        if not hasattr(obj, 'B_AxisErrorOverride'):
            obj.addProperty('App::PropertyBool', 'B_AxisErrorOverride', 'Rotation', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Match B rotations to model (error in FreeCAD rendering).'))
        if not hasattr(obj, 'ReverseDirection'):
            obj.addProperty('App::PropertyBool', 'ReverseDirection', 'Rotation', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Reverse direction of pocket operation.'))

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
        import Draft

        baseSubsTuples = []
        subCount = 0
        allTuples = []

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

        def sortTuplesByIndex(TupleList, tagIdx): # return (TagList, GroupList)
            # Separate elements, regroup by orientation (axis_angle combination)
            TagList = ['X34.2']
            GroupList = [[(2.3, 3.4, 'X')]]
            for tup in TupleList:
                if tup[tagIdx] in TagList:
                    # Determine index of found string
                    i = 0
                    for orn in TagList:
                        if orn == tup[4]:
                            break
                        i += 1
                    GroupList[i].append(tup)
                else:
                    TagList.append(tup[4])  # add orientation entry
                    GroupList.append([tup])  # add orientation entry
            # Remove temp elements
            TagList.pop(0)
            GroupList.pop(0)
            return (TagList, GroupList)

        if obj.Base:
            PathLog.debug('obj.Base exists.  Processing...')
            self.removalshapes = []
            self.horiz = []
            vertical = []

            # ----------------------------------------------------------------------
            if obj.EnableRotation != 'Off':
                for p in range(0, len(obj.Base)):
                    (base, subsList) = obj.Base[p]
                    for sub in subsList:
                        if 'Face' in sub:
                            strDep = obj.StartDepth.Value
                            finDep = obj.FinalDepth.Value
                            rtn = False

                            face = base.Shape.getElement(sub)
                            (rtn, angle, axis, praInfo) = self.pocketRotationAnalysis(obj, face, prnt=True)
                            PathLog.info("praInfo: \n" + str(praInfo))

                            if rtn is True:
                                PathLog.debug(str(sub) + ": rotating model to make face normal at (0,0,1) ...")

                                # Create a temporary clone of model for rotational use.
                                rndAng = round(angle, 8)
                                if rndAng < 0.0:  # neg sign is converted to underscore in clone name creation.
                                    tag = axis + '_' + str(math.fabs(rndAng)).replace('.', '_')
                                else:
                                    tag = axis + str(rndAng).replace('.', '_')
                                clnNm = base.Name + '_' + tag
                                if clnNm not in self.cloneNames:
                                    self.cloneNames.append(clnNm)
                                    PathLog.debug("tmp clone created: " + str(clnNm))
                                    FreeCAD.ActiveDocument.addObject('Part::Feature', clnNm).Shape = base.Shape
                                newBase = FreeCAD.ActiveDocument.getObject(clnNm)

                                # Determine Z translation values
                                if axis == 'X':
                                    bZ = math.sin(math.radians(angle)) * newBase.Placement.Base.y
                                    vect = FreeCAD.Vector(1, 0, 0)
                                elif axis == 'Y':
                                    bZ = math.sin(math.radians(angle)) * newBase.Placement.Base.x
                                    if obj.B_AxisErrorOverride is True:
                                        bZ = -1 * bZ
                                    vect = FreeCAD.Vector(0, 1, 0)
                                # Rotate base to such that Surface.Axis of pocket bottom is Z=1
                                base = Draft.rotate(newBase, angle, center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect, copy=False)
                                # face = base.Shape.getElement(sub)
                            else:
                                PathLog.debug(str(sub) + ": no rotation used")
                                axis = 'X'
                                angle = 0.0
                                tag = axis + str(angle).replace('.', '_')
                                # face = base.Shape.getElement(sub)
                            # Eif
                            PathLog.debug("base.Name: " + str(base.Name))
                            tup = base, sub, tag, angle, axis
                            allTuples.append(tup)
                        # Eif
                        subCount += 1
                    # Efor
                # Efor
                (hTags, hGrps) = sortTuplesByIndex(allTuples, 2) # return (TagList, GroupList)
                subList = []
                for o in range(0, len(hTags)):
                    PathLog.debug('hTag: {}'.format(hTags[o]))
                    subList = []
                    for (base, sub, tag, angle, axis) in hGrps[o]:
                        subList.append(sub)
                    pair = base, subList, angle, axis
                    baseSubsTuples.append(pair)
                # Efor                       
            else:
                PathLog.info("Use Rotation feature(property) is 'Off'.")
                for (base, subList) in obj.Base:
                    baseSubsTuples.append((base, subList, 'pathPocketShape', 0.0, 'X'))

            # ----------------------------------------------------------------------
            # for o in obj.Base:
            for o in baseSubsTuples:
                PathLog.debug('Base item: {}'.format(o))
                base = o[0]
                angle = o[2]
                axis = o[3]

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

                # --------- Originally NOT in FOR loop above --------------
                self.vertical = PathGeom.combineConnectedShapes(vertical)
                self.vWires = [TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1)) for shape in self.vertical]
                for wire in self.vWires:
                    w = PathGeom.removeDuplicateEdges(wire)
                    face = Part.Face(w)
                    # face.tessellate(0.1)
                    if PathGeom.isRoughly(face.Area, 0):
                        PathLog.error(translate('PathPocket', 'Vertical faces do not form a loop - ignoring'))
                    else:
                        self.horiz.append(face)

                # add faces for extensions
                self.exts = []
                for ext in self.getExtensions(obj):
                    wire = Part.Face(ext.getWire())
                    if wire:
                        face = Part.Face(wire)
                        self.horiz.append(face)
                        self.exts.append(face)
                # Efor

                # move all horizontal faces to FinalDepth
                for f in self.horiz:
                    # f.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - f.BoundBox.ZMin))
                    finDep = judgeFinalDepth(obj, face.BoundBox.ZMin)
                    f.translate(FreeCAD.Vector(0, 0, finDep - f.BoundBox.ZMin))

                # check all faces and see if they are touching/overlapping and combine those into a compound
                self.horizontal = []
                for shape in PathGeom.combineConnectedShapes(self.horiz):
                    shape.sewShape()
                    # shape.tessellate(0.1)
                    if obj.UseOutline:
                        wire = TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1))
                        wire.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - wire.BoundBox.ZMin))
                        self.horizontal.append(Part.Face(wire))
                    else:
                        self.horizontal.append(shape)

                # self.removalshapes = [(face.removeSplitter().extrude(extent), False) for face in self.horizontal]
                for face in self.horizontal:
                    # Over-write default final depth value, leaves manual override by user
                    strDep = judgeStartDepth(obj, face.BoundBox.ZMax)
                    #strDep = judgeStartDepth(obj, face.BoundBox.ZMax + self.leadIn)
                    #if strDep < base.Shape.BoundBox.ZMax:
                    #    strDep = base.Shape.BoundBox.ZMax
                    finDep = judgeFinalDepth(obj, face.BoundBox.ZMin)

                    if strDep <= finDep:
                        strDep = finDep + self.leadIn
                        # FreeCAD.Units.Quantity(z, FreeCAD.Units.Length)
                        # FreeCAD.Units.Quantity(self.form.ifWidth.text()).Value
                        title = translate("Path", "Depth Warning")
                        msg = "Start depth <= final depth.\nIncrease the start depth.\nPocket depth is {} mm.".format(finDep)
                        PathLog.error(msg)
                        self.guiMessage(title, msg)  # GUI messages
                    else:
                        obj.StartDepth.Value = strDep
                    obj.FinalDepth.Value = finDep

                    # extrude all faces up to StartDepth and those are the removal shapes
                    # extent = FreeCAD.Vector(0, 0, obj.StartDepth.Value - obj.FinalDepth.Value)
                    extent = FreeCAD.Vector(0, 0, strDep - finDep)
                    self.removalshapes.append((face.removeSplitter().extrude(extent), False, 'pathPocketShape', angle, axis))
            # Efor

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
                # self.removalshapes.append((self.stock.Shape.cut(body), False))
                self.removalshapes.append((self.stock.Shape.cut(body), False, 'pathPocketShape', 0.0, 'X'))

        for (shape, hole, sub, angle, axis) in self.removalshapes:
            shape.tessellate(0.05) # originally 0.1

        if self.removalshapes:
            obj.removalshape = self.removalshapes[0][0]
        return self.removalshapes

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... set default values'''
        obj.StepOver = 100
        obj.ZigZagAngle = 45
        if job and job.Stock:
            bb = job.Stock.Shape.BoundBox
            obj.OpFinalDepth = bb.ZMin
            obj.OpStartDepth = bb.ZMax
        obj.ExtensionCorners = True
        obj.setExpression('ExtensionLengthDefault', 'OpToolDiameter / 2')

    def createExtension(self, obj, extObj, extFeature, extSub):
        return Extension(extObj, extFeature, extSub, obj.ExtensionLengthDefault, Extension.DirectionNormal)

    def getExtensions(self, obj):
        extensions = []
        i = 0
        for extObj,features in obj.ExtensionFeature:
            for sub in features:
                extFeature, extSub = sub.split(':')
                extensions.append(self.createExtension(obj, extObj, extFeature, extSub))
                i = i + 1
        return extensions

    def setExtensions(self, obj, extensions):
        PathLog.track(obj.Label, len(extensions))
        obj.ExtensionFeature = [(ext.obj, ext.getSubLink()) for ext in extensions]

def SetupProperties():
    return PathPocketBase.SetupProperties() + [ 'UseOutline', 'ExtensionCorners' ]

def Create(name, obj = None):
    '''Create(name) ... Creates and returns a Pocket operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject('Path::FeaturePython', name)
    proxy = ObjectPocket(obj, name)
    return obj
