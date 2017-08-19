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

__title__ = "Path Mill Face Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of Mill Facing operation."


if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ObjectFace(PathPocketBase.ObjectPocket):
    '''Proxy object for Mill Facing operation.'''

    def initPocketOp(self, obj):
        '''initPocketOp(obj) ... create facing specific properties'''
        obj.addProperty("App::PropertyEnumeration", "BoundaryShape", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Shape to use for calculating Boundary"))
        obj.BoundaryShape = ['Perimeter', 'Boundbox']

    def pocketInvertExtraOffset(self):
        return True

    def areaOpOnChanged(self, obj, prop):
        '''areaOpOnChanged(obj, prop) ... facing specific depths calculation.'''
        PathLog.track(prop)
        if prop == "StepOver" and obj.StepOver == 0:
            obj.StepOver = 1

        # default depths calculation not correct for facing
        if prop == "Base" and len(obj.Base) == 1:
            base, sub = obj.Base[0]
            shape = base.Shape.getElement(sub[0])
            d = PathUtils.guessDepths(shape, None)
            obj.ClearanceHeight = d.clearance_height
            obj.SafeHeight = d.safe_height + 1
            obj.StartDepth = d.safe_height
            obj.FinalDepth = d.start_depth

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... return top face'''
        # Facing is done either against base objects
        if obj.Base:
            PathLog.debug("obj.Base: {}".format(obj.Base))
            faces = []
            for b in obj.Base:
                for sub in b[1]:
                    shape = getattr(b[0].Shape, sub)
                    if isinstance(shape, Part.Face):
                        faces.append(shape)
                    else:
                        PathLog.debug('The base subobject is not a face')
                        return
            planeshape = Part.makeCompound(faces)
            PathLog.debug("Working on a collection of faces {}".format(faces))

        # If no base object, do planing of top surface of entire model
        else:
            planeshape = self.baseobject.Shape
            PathLog.debug("Working on a shape {}".format(self.baseobject.Name))

        # if user wants the boundbox, calculate that
        PathLog.debug("Boundary Shape: {}".format(obj.BoundaryShape))
        bb = planeshape.BoundBox
        if obj.BoundaryShape == 'Boundbox':
            bbperim = Part.makeBox(bb.XLength, bb.YLength, 1, FreeCAD.Vector(bb.XMin, bb.YMin, bb.ZMin), FreeCAD.Vector(0, 0, 1))
            env = PathUtils.getEnvelope(partshape=bbperim, depthparams=self.depthparams)
        else:
            env = PathUtils.getEnvelope(partshape=planeshape, depthparams=self.depthparams)

        return [(env, False)]

    def areaOpSetDefaultValues(self, obj):
        '''areaOpSetDefaultValues(obj) ... initialize mill facing properties'''
        obj.StepOver = 50
        obj.ZigZagAngle = 45.0

        # need to overwrite the default depth calculations for facing
        job = PathUtils.findParentJob(obj)
        if job and job.Base:
            d = PathUtils.guessDepths(job.Base.Shape, None)
            obj.ClearanceHeight = d.clearance_height
            obj.SafeHeight = d.safe_height + 1
            obj.StartDepth = d.safe_height
            obj.FinalDepth = d.start_depth

def Create(name):
    '''Create(name) ... Creates and returns a Mill Facing operation.'''
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectFace(obj)
    return obj
