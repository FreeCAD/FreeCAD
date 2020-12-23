# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import Part
import PathScripts.PathLog as PathLog
import PathScripts.PathPocketBase as PathPocketBase
import PathScripts.PathUtils as PathUtils

from PySide import QtCore
import numpy

__title__ = "Path Mill Face Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of Mill Facing operation."
__contributors__ = "russ4262 (Russell Johnson)"
__created__ = "2014"
__scriptVersion__ = "1d usable"
__lastModified__ = "2019-07-22 23:49 CST"


LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectFace(PathPocketBase.ObjectPocket):
    '''Proxy object for Mill Facing operation.'''

    def initPocketOp(self, obj):
        '''initPocketOp(obj) ... create facing specific properties'''
        obj.addProperty("App::PropertyEnumeration", "BoundaryShape", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Shape to use for calculating Boundary"))
        obj.BoundaryShape = ['Perimeter', 'Boundbox', 'Stock']

        if not hasattr(obj, 'ExcludeRaisedAreas'):
            obj.addProperty("App::PropertyBool", "ExcludeRaisedAreas", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Exclude milling raised areas inside the face."))

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
            obj.OpStartDepth = job.Stock.Shape.BoundBox.ZMax

            if len(obj.Base) >= 1:
                print('processing')
                sublist = []
                for i in obj.Base:
                    o = i[0]
                    for s in i[1]:
                        sublist.append(o.Shape.getElement(s))

            # If the operation has a geometry identified the Finaldepth
            # is the top of the bboundbox which includes all features.
            # Otherwise, top of part.

                obj.OpFinalDepth = Part.makeCompound(sublist).BoundBox.ZMax
            else:
                obj.OpFinalDepth = job.Proxy.modelBoundBox(job).ZMax

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... return top face'''
        # Facing is done either against base objects
        holeShape = None

        if obj.Base:
            PathLog.debug("obj.Base: {}".format(obj.Base))
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
                        if oneBase[0] is not b[0]:
                            oneBase[1] = False
                        if numpy.isclose(abs(shape.normalAt(0, 0).z), 1):  # horizontal face
                            for wire in shape.Wires[1:]:
                                if obj.ExcludeRaisedAreas is True:
                                    ip = self.isPocket(b[0], shape, wire)
                                    if ip is False:
                                        holes.append((b[0].Shape, wire))
                                else:
                                    holes.append((b[0].Shape, wire))
                    else:
                        PathLog.error('The base subobject, "{}," is not a face. Ignoring "{}."'.format(sub, sub))

            if obj.ExcludeRaisedAreas is True and len(holes) > 0:
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
        if obj.BoundaryShape == 'Boundbox':
            bbperim = Part.makeBox(bb.XLength, bb.YLength, 1, FreeCAD.Vector(bb.XMin, bb.YMin, bb.ZMin), FreeCAD.Vector(0, 0, 1))
            env = PathUtils.getEnvelope(partshape=bbperim, depthparams=self.depthparams)
            if obj.ExcludeRaisedAreas is True and oneBase[1] is True:
                includedFaces = self.getAllIncludedFaces(oneBase[0], env, faceZ=minHeight)
                if len(includedFaces) > 0:
                    includedShape = Part.makeCompound(includedFaces)
                    includedEnv = PathUtils.getEnvelope(oneBase[0].Shape, subshape=includedShape, depthparams=self.depthparams)
                    env = env.cut(includedEnv)
        elif obj.BoundaryShape == 'Stock':
            stock = PathUtils.findParentJob(obj).Stock.Shape
            env = stock
            if obj.ExcludeRaisedAreas is True and oneBase[1] is True:
                includedFaces = self.getAllIncludedFaces(oneBase[0], stock, faceZ=minHeight)
                if len(includedFaces) > 0:
                    stockEnv = PathUtils.getEnvelope(partshape=stock, depthparams=self.depthparams)
                    includedShape = Part.makeCompound(includedFaces)
                    includedEnv = PathUtils.getEnvelope(oneBase[0].Shape, subshape=includedShape, depthparams=self.depthparams)
                    env = stockEnv.cut(includedEnv)
        else:
            env = PathUtils.getEnvelope(partshape=planeshape, depthparams=self.depthparams)

        if holeShape is not None:
            PathLog.info("Processing holes...")
            holeEnv = PathUtils.getEnvelope(partshape=holeShape, depthparams=self.depthparams)
            newEnv = env.cut(holeEnv)
            return [(newEnv, False)]
        else:
            return [(env, False)]

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... initialize mill facing properties'''
        obj.StepOver = 50
        obj.ZigZagAngle = 45.0
        obj.ExcludeRaisedAreas = False

        # need to overwrite the default depth calculations for facing
        if job and len(job.Model.Group) > 0:
            obj.OpStartDepth = job.Stock.Shape.BoundBox.ZMax
            obj.OpFinalDepth = job.Proxy.modelBoundBox(job).ZMax

            # If the operation has a geometry identified the Finaldepth
            # is the top of the bboundbox which includes all features.
            if len(obj.Base) >= 1:
                obj.OpFinalDepth = Part.makeCompound(obj.Base).BoundBox.ZMax

    def isPocket(self, b, f, w):
        e = w.Edges[0]
        for fi in range(0, len(b.Shape.Faces)):
            face = b.Shape.Faces[fi]
            for ei in range(0, len(face.Edges)):
                edge = face.Edges[ei]
                if e.isSame(edge) is True:
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
        # eZMin = env.BoundBox.ZMin
        eZMin = faceZ
        # eZMax = env.BoundBox.ZMax

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
            # fZMin = face.BoundBox.ZMin
            fZMax = face.BoundBox.ZMax
            if fZMax > eZMin:
                if isOverlap(fXMin, fXMax, eXMin, eXMax) is True:
                    if isOverlap(fYMin, fYMax, eYMin, eYMax) is True:
                        incl = True
            if incl is True:
                included.append(face)
        return included


def SetupProperties():
    setup = PathPocketBase.SetupProperties()
    setup.append("BoundaryShape")
    setup.append("ExcludeRaisedAreas")
    return setup


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Mill Facing operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectFace(obj, name)
    return obj
