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
import sys

from PySide import QtCore

__title__ = "Path Pocket Shape Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of shape based Pocket operation."

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class Extension(object):
    DirectionNormal    = 0
    DirectionX         = 1
    DirectionY         = 2

    def __init__(self, obj, sub, length, direction):
        self.obj = obj
        self.sub = sub
        self.length = length
        self.direction = direction

    def extendEdge(self, e0, direction):
        e2 = e0.copy()
        off = self.length.Value * direction
        e2.translate(off)
        print("extendEdge(%.2f, %.2f, %.2f)" % (off.x, off.y, off.z))
        e2 = PathGeom.flipEdge(e2)
        e1 = Part.Edge(Part.LineSegment(e0.valueAt(e0.LastParameter), e2.valueAt(e2.FirstParameter)))
        e3 = Part.Edge(Part.LineSegment(e2.valueAt(e2.LastParameter), e0.valueAt(e0.FirstParameter)))
        wire = Part.Wire([e0, e1, e2, e3])
        self.wire = wire
        return wire

    def getWire(self):
        if PathGeom.isRoughly(0, self.length.Value):
            return None

        feature = self.obj.Shape.getElement(self.sub)
        if Part.Edge == type(feature):
            e0 = feature
            print("getEdgeCorners(%s)" % self.sub)
            midparam = e0.FirstParameter + 0.5 * (e0.LastParameter - e0.FirstParameter)
            tangent = e0.tangentAt(midparam)
            normal = tangent.cross(FreeCAD.Vector(0, 0, 1)).normalize()
            poffPlus  = e0.valueAt(midparam) + 0.01 * normal
            poffMinus = e0.valueAt(midparam) - 0.01 * normal
            if not self.obj.Shape.isInside(poffPlus, 0.005, True):
                print('poffPlus')
                return self.extendEdge(e0, normal)
            if not self.obj.Shape.isInside(poffMinus, 0.005, True):
                print('poffMinus')
                return self.extendEdge(e0, normal.negative())
        else:
            print("getCorners(%s)" % self.sub)


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
        if not hasattr(obj, 'ExtensionFeatures'):
            obj.addProperty('App::PropertyLinkSubListGlobal', 'ExtensionFeature', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'List of features to extend.'))
        if not hasattr(obj, 'ExtensionLength'):
            obj.addProperty('App::PropertyFloatList', 'ExtensionLength', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'List of extension lenght of corresponding feature.'))
        if not hasattr(obj, 'ExtensionDirection'):
            obj.addProperty('App::PropertyIntegerList', 'ExtensionDirection', 'Extension', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'List of extension direction of corresponding feature.'))

        obj.setEditorMode('ExtensionFeature', 2)
        obj.setEditorMode('ExtensionLength', 2)
        obj.setEditorMode('ExtensionDirection', 2)

    def opOnDocumentRestored(self, obj):
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


            # move all horizontal faces to FinalDepth
            for f in self.horiz:
                f.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - f.BoundBox.ZMin))

            # check all faces and see if they are touching/overlapping and combine those into a compound
            self.horizontal = []
            for shape in PathGeom.combineConnectedShapes(self.horiz):
                shape.sewShape()
                shape.tessellate(0.1)
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

        for (shape,hole) in self.removalshapes:
            shape.tessellate(0.1)

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
        obj.ExtensionLengthDefault = obj.OpToolDiameter / 2

    def createExtension(self, obj, extObj, extSub):
        return Extension(extObj, extSub, obj.ExtensionLengthDefault, Extension.DirectionNormal)

    def getExtensions(self, obj):
        extensions = []
        i = 0
        for extObj,features in obj.ExtensionFeature:
            for extSub in features:
                extensions.append(self.createExtension(obj, extObj, extSub))
                i = i + 1
        return extensions

    def setExtensions(self, obj, extensions):
        features = {}
        for ext in extensions:
            subs = features.get(ext.obj, [])
            subs.append(ext.sub)
            features[ext.obj] = subs
        PathLog.track('setExtension', obj.Label)
        obj.ExtensionFeature = [(ext.obj, ext.sub) for ext in extensions]

def SetupProperties():
    return PathPocketBase.SetupProperties() + [ 'UseOutline' ]

def Create(name, obj = None):
    '''Create(name) ... Creates and returns a Pocket operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject('Path::FeaturePython', name)
    proxy = ObjectPocket(obj, name)
    return obj
