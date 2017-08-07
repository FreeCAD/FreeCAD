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
import PathScripts.PathAreaOp as PathAreaOp
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils

from PySide import QtCore, QtGui

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

if FreeCAD.GuiUp:
    import FreeCADGui


# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

__title__ = "Path Mill Face Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path Face object and FreeCAD command"""


class ObjectFace(PathAreaOp.ObjectOp):

    def areaOpFeatures(self, obj):
        return PathOp.FeatureBaseFaces | PathOp.FeatureFinishDepth

    def initAreaOp(self, obj):

        # Face Properties
        obj.addProperty("App::PropertyEnumeration", "CutMode", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.CutMode = ['Climb', 'Conventional']
        obj.addProperty("App::PropertyDistance", "PassExtension", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "How far the cutter should extend past the boundary"))
        obj.addProperty("App::PropertyEnumeration", "StartAt", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Start Faceing at center or boundary"))
        obj.StartAt = ['Center', 'Edge']
        obj.addProperty("App::PropertyPercent", "StepOver", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Percent of cutter diameter to step over on each pass"))
        obj.addProperty("App::PropertyBool", "KeepToolDown", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Attempts to avoid unnecessary retractions."))
        obj.addProperty("App::PropertyBool", "ZigUnidirectional", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Lifts tool at the end of each pass to respect cut mode."))
        obj.addProperty("App::PropertyBool", "UseZigZag", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Use Zig Zag pattern to clear area."))
        obj.addProperty("App::PropertyFloat", "ZigZagAngle", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Angle of the zigzag pattern"))
        obj.addProperty("App::PropertyEnumeration", "BoundaryShape", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Shape to use for calculating Boundary"))
        obj.BoundaryShape = ['Perimeter', 'Boundbox']
        obj.addProperty("App::PropertyEnumeration", "OffsetPattern", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "clearing pattern to use"))
        obj.OffsetPattern = ['ZigZag', 'Offset', 'Spiral', 'ZigZagOffset', 'Line', 'Grid', 'Triangle']


    def areaOpOnChanged(self, obj, prop):
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

    def areaOpUseProjection(self, obj):
        return False

    def areaOpAreaParams(self, obj, isHole):
        params = {}
        params['Fill'] = 0
        params['Coplanar'] = 0
        params['PocketMode'] = 1
        params['SectionCount'] = -1
        params['Angle'] = obj.ZigZagAngle
        params['FromCenter'] = (obj.StartAt == "Center")
        params['PocketStepover'] = (self.radius * 2) * (float(obj.StepOver)/100)
        params['PocketExtraOffset'] = 0 - obj.PassExtension.Value

        Pattern = ['ZigZag', 'Offset', 'Spiral', 'ZigZagOffset', 'Line', 'Grid', 'Triangle']
        params['PocketMode'] = Pattern.index(obj.OffsetPattern) + 1

        params['ToolRadius'] = self.radius

        return params

    def areaOpPathParams(self, obj, isHole):
        params = {}
        params['feedrate'] = self.horizFeed
        params['feedrate_v'] = self.vertFeed
        params['verbose'] = True
        params['resume_height'] = obj.StepDown
        params['retraction'] = obj.ClearanceHeight.Value

        if obj.UseStartPoint and obj.StartPoint is not None:
            params['start'] = obj.StartPoint
        
        return params

    def areaOpShapes(self, obj):
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
            job = PathUtils.findParentJob(obj)
            if not job or not job.Base:
                return
            baseobject = job.Base
            planeshape = baseobject.Shape
            PathLog.debug("Working on a shape {}".format(baseobject.Name))

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
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectFace(obj)
    return obj
