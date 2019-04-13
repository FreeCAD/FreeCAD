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

import FreeCAD
import Part
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathPocketBase as PathPocketBase
import PathScripts.PathUtil as PathUtil
import PathScripts.PathUtils as PathUtils
import TechDraw
import math
import sys

from PySide import QtCore

__title__ = "Path Pocket Shape Operation"
__author__ = "sliptonic (Brad Collette)"
__contributors__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of shape based Pocket operation."
__scriptVersion__ = "1a Stable"
__created__ = "2017"
__lastModified__ = "2019-04-13 13:17 CST"

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

def endPoints(edgeOrWire):
    '''endPoints(edgeOrWire) ... return the first and last point of the wire or the edge, assuming the argument is not a closed wire.'''
    if Part.Wire == type(edgeOrWire):
        edges = edgeOrWire.Edges
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
    if not closest is None:
        return closest[1]
    return None


def extendWire(feature, wire, length):
    '''extendWire(wire, length) ... return a closed Wire which extends wire by length'''
    off2D = wire.makeOffset2D(length)
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
    DirectionNormal    = 0
    DirectionX         = 1
    DirectionY         = 2

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
        poffPlus  = e0.valueAt(midparam) + 0.01 * normal
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

        obj.setEditorMode('ExtensionFeature', 2)

    def opOnDocumentRestored(self, obj):
        '''opOnDocumentRestored(obj) ... adds the UseOutline property if it doesn't exist.'''
        self.initPocketOp(obj)

    def pocketInvertExtraOffset(self):
        return False

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... return shapes representing the solids to be removed.'''
        PathLog.track()

        #print("\n\nareaOpShapes() in PathPocketShape.py - Rotating base objects")
        reset = False
        if obj.Base:
            PathLog.debug("base items exist.  Processing...")
            self.removalshapes = []
            self.horiz = []
            vertical = []

            for o in obj.Base:
                PathLog.debug("Base item: {}".format(o))
                base = o[0]
                reset = False
                baseRot = base.Placement
                for sub in o[1]:
                    if "Face" in sub:
                        print("face '" + str(sub) + "'  " + str(base.Shape.getElement(sub).Placement))
                        # Determine if rotation is needed
                        (rtn, angle, axis, rotate) = self.areaOpDetermineFaceRotationNeeded(obj, base, sub)  # tuple returned, and unpacked
                        if rtn == True:
                            axisVect = FreeCAD.Vector(1,0,0)
                            if axis == 'Y':
                                axisVect = FreeCAD.Vector(0,1,0)
                            base.Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0), FreeCAD.Rotation(axisVect, angle))
                            base.recompute()
                            trans = angle, axis
                            strDep = base.Shape.BoundBox.ZMax
                            finDep = base.Shape.getElement(sub).BoundBox.ZMin  #base.Shape.getElement(sub).Placement.Base.z
                            depths = strDep, finDep # define tuple
                            reset = True
                            print("FaceRotationNeeded: TRUE")
                        else:
                            trans = 0.0, 'X'
                            strDep = base.Shape.BoundBox.ZMax
                            finDep = base.Shape.getElement(sub).BoundBox.ZMin  #base.Shape.getElement(sub).Placement.Base.z
                            #print("base post-rotation: No rotation needed")
                        depths = strDep, finDep # define tuple

                        face = base.Shape.getElement(sub)
                        if type(face.Surface) == Part.Plane and PathGeom.isVertical(face.Surface.Axis):
                            # it's a flat horizontal face
                            tup = face, depths, trans, rotate
                            self.horiz.append(tup)
                            #print("type(face.Surface) == Part.Plane")
                        elif type(face.Surface) == Part.Cylinder and PathGeom.isVertical(face.Surface.Axis):
                            # vertical cylinder wall
                            if any(e.isClosed() for e in face.Edges):
                                # complete cylinder
                                circle = Part.makeCircle(face.Surface.Radius, face.Surface.Center)
                                disk = Part.Face(Part.Wire(circle))
                                tup = disk, depths, trans, rotate
                                self.horiz.append(tup)
                                #print("type(face.Surface) == Part.Cylinder : e.isClosed()")
                            else:
                                # partial cylinder wall
                                #tup = face, depths, trans, rotate
                                vertical.append(face)
                                #print("type(face.Surface) == Part.Cylinder : e.isClosed() ELSE")
                        elif type(face.Surface) == Part.Plane and PathGeom.isHorizontal(face.Surface.Axis):
                            #tup = face, depths, trans, rotate
                            vertical.append(face)
                            #print("type(face.Surface) == Part.Plane : PathGeom.isHorizontal")
                        else:
                            print("Type of shape not supported: " + str(type(face.Surface)))
                            print("face.Surface.Axis: " + str(face.Surface.Axis))
                            PathLog.error(translate('PathPocket', "Pocket does not support shape %s.%s") % (base.Label, sub))
                    
                    # Reset base rotation to original orientation, if needed
                    if reset == True:
                        base.Placement = baseRot
                        base.recompute()


            self.vertical = PathGeom.combineConnectedShapes(vertical)
            self.vWires = [TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1)) for shape in self.vertical]
            for wire in self.vWires:
                w = PathGeom.removeDuplicateEdges(wire)
                face = Part.Face(w)
                face.tessellate(0.05)  # Russ4262 0.1 original
                if PathGeom.isRoughly(face.Area, 0):
                    PathLog.error(translate('PathPocket', 'Vertical faces do not form a loop - ignoring'))
                else:
                    strDep = face.BoundBox.ZMax  #base.Shape.BoundBox.ZMax
                    finDep = face.BoundBox.ZMin  #face.Placement.Base.z,  #base.Shape.getElement(sub).Placement.Base.z
                    depths = strDep, finDep # define tuple
                    tup = face, depths, 'X', 0.0
                    self.horiz.append(tup)

            '''
            # move all horizontal faces to FinalDepth
            for (shape, ang, ax, axVec) in self.horiz:
                trnslt = obj.FinalDepth.Value - shape.BoundBox.ZMin
                print("trnslt '" + str(trnslt))
                shape.translate(FreeCAD.Vector(0, 0, trnslt))
            '''

            # Separate elements, regroup by orientation (axis-angle combination)
            orientationGroups = ['X34.2']
            tupGroups = [[(2.3, 3.4, 'X')]]
            for tup in self.horiz:
                (face, dps, (ang, ax), axVec) = tup
                a = round(ang, 6)
                orntn = ax + str(a)
                if orntn in orientationGroups:
                    # Determine index of found string
                    i = 0
                    for orn in orientationGroups:
                        if orn == orntn:
                            break
                        i += 1
                    tupGroups[i].append(tup)
                else:
                    orientationGroups.append(orntn)  # add orientation entry
                    tupGroups.append([tup])  # add orientation entry
            # Remove temp elements
            orientationGroups.pop(0)
            tupGroups.pop(0)
            
            # check all faces in each group
            self.horizontal = []
            shpList = []
            for o in range(0, len(orientationGroups)):
                shpList = []
                for (face, ang, ax, axVec) in tupGroups[o]:  #face, depths, trans, rotate
                    shpList.append(face)
                # check all faces and see if they are touching/overlapping and combine those into a compound
                # Original Code in For loop
                for shape in PathGeom.combineConnectedShapes(shpList):
                    shape.sewShape()
                    shape.tessellate(0.05) # Russ4262 0.1 original
                    if obj.UseOutline:
                        wire = TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1))
                        wire.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - wire.BoundBox.ZMin))
                        tup = Part.Face(wire), ang, ax, axVec
                        self.horizontal.append(tup)
                    else:
                        tup = shape, ang, ax, axVec
                        self.horizontal.append(tup)
            # Eol

            # extrude all faces up to StartDepth and those are the removal shapes
            #extent = FreeCAD.Vector(0, 0, obj.StartDepth.Value - obj.FinalDepth.Value)
            #self.removalshapes = [(face.removeSplitter().extrude(extent), False) for face in self.horizontal]
            for (face, depths, trans, rotate) in self.horizontal:  #face, depths, trans, rotate
                (strt, fin) = depths
                (ang, ax) = trans
                # extrude all faces up to StartDepth and those are the removal shapes
                #diffStrtFin = obj.StartDepth.Value - obj.FinalDepth.Value
                diffStrtFin = strt - fin + 5.0
                #if ang != rotate:
                #    diffStrtFin = strt - fin + 5.0
                extent = FreeCAD.Vector(0, 0, diffStrtFin)
                tup = face.removeSplitter().extrude(extent), False, depths, trans, rotate
                self.removalshapes.append(tup)

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
                self.removalshapes.append((self.stock.Shape.cut(body), False, (25.0, 0.0), (0.0, 'X'), 0.0))

        for (shape, hole, ang, ax, axVec) in self.removalshapes:
            shape.tessellate(0.05) # Russ4262 0.1 original

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
