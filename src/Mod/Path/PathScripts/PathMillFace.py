# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

from __future__ import print_function

import FreeCAD
import PathScripts.PathLog as PathLog
import PathScripts.PathPocketBase as PathPocketBase
import PathScripts.PathUtils as PathUtils

from PySide import QtCore
import numpy

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')

__title__ = "Path Mill Face Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Class and implementation of Mill Facing operation."
__contributors__ = "russ4262 (Russell Johnson)"


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule()


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectFace(PathPocketBase.ObjectPocket):
    '''Proxy object for Mill Facing operation.'''

    def initPocketOp(self, obj):
        '''initPocketOp(obj) ... create facing specific properties'''
        obj.addProperty("App::PropertyEnumeration", "BoundaryShape", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Shape to use for calculating Boundary"))
        obj.addProperty("App::PropertyBool", "ClearEdges", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Clear edges of surface (Only applicable to BoundBox)"))
        if not hasattr(obj, 'ExcludeRaisedAreas'):
            obj.addProperty("App::PropertyBool", "ExcludeRaisedAreas", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Exclude milling raised areas inside the face."))

        obj.BoundaryShape = ['Boundbox', 'Face Region', 'Perimeter', 'Stock']

    def pocketInvertExtraOffset(self):
        return True

    def areaOpOnChanged(self, obj, prop):
        '''areaOpOnChanged(obj, prop) ... facing specific depths calculation.'''
        PathLog.track(prop)
        if prop == "StepOver" and obj.StepOver == 0:
            obj.StepOver = 1

        # default depths calculation not correct for facing
        if prop == "Base":
            job = PathUtils.findParentJob(obj)
            if job:
                obj.OpStartDepth = job.Stock.Shape.BoundBox.ZMax

            if len(obj.Base) >= 1:
                PathLog.debug('processing')
                sublist = []
                for i in obj.Base:
                    o = i[0]
                    for s in i[1]:
                        sublist.append(o.Shape.getElement(s))

            # If the operation has a geometry identified the Finaldepth
            # is the top of the bboundbox which includes all features.
            # Otherwise, top of part.

                obj.OpFinalDepth = Part.makeCompound(sublist).BoundBox.ZMax
            elif job:
                obj.OpFinalDepth = job.Proxy.modelBoundBox(job).ZMax

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... return top face'''
        # Facing is done either against base objects
        holeShape = None

        PathLog.debug('depthparams: {}'.format([i for i in self.depthparams]))

        if obj.Base:
            PathLog.debug("obj.Base: {}".format(obj.Base))
            self.removalshapes = []
            faces = []
            holes = []
            holeEnvs = []
            oneBase = [obj.Base[0][0], True]
            sub0 = getattr(obj.Base[0][0].Shape, obj.Base[0][1][0])
            minHeight = sub0.BoundBox.ZMax

            for b in obj.Base:
                for sub in b[1]:
                    shape = getattr(b[0].Shape, sub)
                    if isinstance(shape, Part.Face):
                        faces.append(shape)
                        if shape.BoundBox.ZMin < minHeight:
                            minHeight = shape.BoundBox.ZMin
                        # Limit to one model base per operation
                        if oneBase[0] is not b[0]:
                            oneBase[1] = False
                        if numpy.isclose(abs(shape.normalAt(0, 0).z), 1):  # horizontal face
                            # Analyze internal closed wires to determine if raised or a recess
                            for wire in shape.Wires[1:]:
                                if obj.ExcludeRaisedAreas:
                                    ip = self.isPocket(b[0], shape, wire)
                                    if ip is False:
                                        holes.append((b[0].Shape, wire))
                                else:
                                    holes.append((b[0].Shape, wire))
                    else:
                        PathLog.warning('The base subobject, "{0}," is not a face. Ignoring "{0}."'.format(sub))

            if obj.ExcludeRaisedAreas and len(holes) > 0:
                for shape, wire in holes:
                    f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                    env = PathUtils.getEnvelope(shape, subshape=f, depthparams=self.depthparams)
                    holeEnvs.append(env)
                    holeShape = Part.makeCompound(holeEnvs)

            PathLog.debug("Working on a collection of faces {}".format(faces))
            planeshape = Part.makeCompound(faces)

        # If no base object, do planing of top surface of entire model
        else:
            planeshape = Part.makeCompound([base.Shape for base in self.model])
            PathLog.debug("Working on a shape {}".format(obj.Label))

        # Find the correct shape depending on Boundary shape.
        PathLog.debug("Boundary Shape: {}".format(obj.BoundaryShape))
        bb = planeshape.BoundBox

        # Apply offset for clearing edges
        offset = 0
        if obj.ClearEdges:
            offset = self.radius + 0.1

        bb.XMin = bb.XMin - offset
        bb.YMin = bb.YMin - offset
        bb.XMax = bb.XMax + offset
        bb.YMax = bb.YMax + offset

        if obj.BoundaryShape == 'Boundbox':
            bbperim = Part.makeBox(bb.XLength, bb.YLength, 1, FreeCAD.Vector(bb.XMin, bb.YMin, bb.ZMin), FreeCAD.Vector(0, 0, 1))
            env = PathUtils.getEnvelope(partshape=bbperim, depthparams=self.depthparams)
            if obj.ExcludeRaisedAreas and oneBase[1]:
                includedFaces = self.getAllIncludedFaces(oneBase[0], env, faceZ=minHeight)
                if len(includedFaces) > 0:
                    includedShape = Part.makeCompound(includedFaces)
                    includedEnv = PathUtils.getEnvelope(oneBase[0].Shape, subshape=includedShape, depthparams=self.depthparams)
                    env = env.cut(includedEnv)
        elif obj.BoundaryShape == 'Stock':
            stock = PathUtils.findParentJob(obj).Stock.Shape
            env = stock

            if obj.ExcludeRaisedAreas and oneBase[1]:
                includedFaces = self.getAllIncludedFaces(oneBase[0], stock, faceZ=minHeight)
                if len(includedFaces) > 0:
                    stockEnv = PathUtils.getEnvelope(partshape=stock, depthparams=self.depthparams)
                    includedShape = Part.makeCompound(includedFaces)
                    includedEnv = PathUtils.getEnvelope(oneBase[0].Shape, subshape=includedShape, depthparams=self.depthparams)
                    env = stockEnv.cut(includedEnv)
        elif obj.BoundaryShape == 'Perimeter':
            if obj.ClearEdges:
                psZMin = planeshape.BoundBox.ZMin
                ofstShape = PathUtils.getOffsetArea(planeshape,
                                                    self.radius * 1.25,
                                                    plane=planeshape)
                ofstShape.translate(FreeCAD.Vector(0.0, 0.0, psZMin - ofstShape.BoundBox.ZMin))
                env = PathUtils.getEnvelope(partshape=ofstShape, depthparams=self.depthparams)
            else:
                env = PathUtils.getEnvelope(partshape=planeshape, depthparams=self.depthparams)
        elif obj.BoundaryShape == 'Face Region':
            import PathScripts.PathSurfaceSupport as PathSurfaceSupport
            baseShape = oneBase[0].Shape
            psZMin = planeshape.BoundBox.ZMin
            ofstShape = PathUtils.getOffsetArea(planeshape,
                                                self.tool.Diameter * 1.1,
                                                plane=planeshape)
            ofstShape.translate(FreeCAD.Vector(0.0, 0.0, psZMin - ofstShape.BoundBox.ZMin))

            # Calculate custom depth params for removal shape envelope, with start and final depth buffers
            custDepthparams = self._customDepthParams(obj, obj.StartDepth.Value + 0.2, obj.FinalDepth.Value - 0.1)  # only an envelope
            ofstShapeEnv = PathUtils.getEnvelope(partshape=ofstShape, depthparams=custDepthparams)
            env = ofstShapeEnv.cut(baseShape)
            env.translate(FreeCAD.Vector(0.0, 0.0, -0.000001))  # lower removal shape into buffer zone

        if holeShape:
            PathLog.debug("Processing holes and face ...")
            holeEnv = PathUtils.getEnvelope(partshape=holeShape, depthparams=self.depthparams)
            newEnv = env.cut(holeEnv)
            tup = newEnv, False, 'pathMillFace'
        else:
            PathLog.debug("Processing solid face ...")
            tup = env, False, 'pathMillFace'

        self.removalshapes.append(tup)
        obj.removalshape = self.removalshapes[0][0]  # save removal shape

        return self.removalshapes

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... initialize mill facing properties'''
        obj.StepOver = 50
        obj.ZigZagAngle = 45.0
        obj.ExcludeRaisedAreas = False
        obj.ClearEdges = False

        # need to overwrite the default depth calculations for facing
        if job and len(job.Model.Group) > 0:
            obj.OpStartDepth = job.Stock.Shape.BoundBox.ZMax
            obj.OpFinalDepth = job.Proxy.modelBoundBox(job).ZMax

            # If the operation has a geometry identified the Finaldepth
            # is the top of the boundbox which includes all features.
            if len(obj.Base) >= 1:
                shapes = []
                for base, subs in obj.Base:
                    for s in subs:
                        shapes.append(getattr(base.Shape, s))
                obj.OpFinalDepth = Part.makeCompound(shapes).BoundBox.ZMax

    def isPocket(self, b, f, w):
        e = w.Edges[0]
        for fi in range(0, len(b.Shape.Faces)):
            face = b.Shape.Faces[fi]
            for ei in range(0, len(face.Edges)):
                edge = face.Edges[ei]
                if e.isSame(edge):
                    if f is face:
                        # Alternative: run loop to see if all edges are same
                        pass  # same source face, look for another
                    else:
                        if face.CenterOfMass.z < f.CenterOfMass.z:
                            return True
        return False

    def getAllIncludedFaces(self, base, env, faceZ):
        included = []

        eXMin = env.BoundBox.XMin
        eXMax = env.BoundBox.XMax
        eYMin = env.BoundBox.YMin
        eYMax = env.BoundBox.YMax
        eZMin = faceZ

        def isOverlap(fMn, fMx, eMn, eMx):
            if fMx > eMn:
                if fMx <= eMx:
                    return True
                elif fMx >= eMx and fMn <= eMx:
                    return True
            if fMn < eMx:
                if fMn >= eMn:
                    return True
                elif fMn <= eMn and fMx >= eMn:
                    return True
            return False

        for fi in range(0, len(base.Shape.Faces)):
            incl = False
            face = base.Shape.Faces[fi]
            fXMin = face.BoundBox.XMin
            fXMax = face.BoundBox.XMax
            fYMin = face.BoundBox.YMin
            fYMax = face.BoundBox.YMax
            fZMax = face.BoundBox.ZMax

            if fZMax > eZMin:
                if isOverlap(fXMin, fXMax, eXMin, eXMax):
                    if isOverlap(fYMin, fYMax, eYMin, eYMax):
                        incl = True
            if incl:
                included.append(face)
        return included


def SetupProperties():
    setup = PathPocketBase.SetupProperties()
    setup.append("BoundaryShape")
    setup.append("ExcludeRaisedAreas")
    setup.append("ClearEdges")
    return setup


def Create(name, obj=None, parentJob=None):
    '''Create(name) ... Creates and returns a Mill Facing operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectFace(obj, name, parentJob)
    return obj
