# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
import Path
import PathScripts.PathAreaOp as PathAreaOp
import PathScripts.PathLog as PathLog

from PathScripts import PathUtils
from PySide import QtCore

"""Path Pocket object and FreeCAD command"""

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectPocket(PathAreaOp.ObjectOp):

    def opFeatures(self, obj):
        return PathAreaOp.FeatureTool | PathAreaOp.FeatureDepths | PathAreaOp.FeatureHeights | PathAreaOp.FeatureStartPoint | PathAreaOp.FeatureBaseFaces | PathAreaOp.FeatureFinishDepth

    def initOperation(self, obj):
        PathLog.track()

        # Pocket Properties
        obj.addProperty("App::PropertyEnumeration", "CutMode", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.CutMode = ['Climb', 'Conventional']
        obj.addProperty("App::PropertyDistance", "MaterialAllowance", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Amount of material to leave"))
        obj.addProperty("App::PropertyEnumeration", "StartAt", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Start pocketing at center or boundary"))
        obj.StartAt = ['Center', 'Edge']
        obj.addProperty("App::PropertyPercent", "StepOver", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Percent of cutter diameter to step over on each pass"))
        obj.addProperty("App::PropertyFloat", "ZigZagAngle", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Angle of the zigzag pattern"))
        obj.addProperty("App::PropertyEnumeration", "OffsetPattern", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "clearing pattern to use"))
        obj.OffsetPattern = ['ZigZag', 'Offset', 'Spiral', 'ZigZagOffset', 'Line', 'Grid', 'Triangle']
        obj.addProperty("App::PropertyBool", "MinTravel", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Use 3D Sorting of Path"))

    def opUseProjection(self, obj):
        return False

    def opAreaParams(self, obj, isHole):
        params = {}
        params['Fill'] = 0
        params['Coplanar'] = 0
        params['PocketMode'] = 1
        params['SectionCount'] = -1
        params['Angle'] = obj.ZigZagAngle
        params['FromCenter'] = (obj.StartAt == "Center")
        params['PocketStepover'] = (self.radius * 2) * (float(obj.StepOver)/100)
        params['PocketExtraOffset'] = obj.MaterialAllowance.Value
        params['ToolRadius'] = self.radius

        Pattern = ['ZigZag', 'Offset', 'Spiral', 'ZigZagOffset', 'Line', 'Grid', 'Triangle']
        params['PocketMode'] = Pattern.index(obj.OffsetPattern) + 1
        return params

    def opPathParams(self, obj, isHole):
        params = {}

        # if MinTravel is turned on, set path sorting to 3DSort
        # 3DSort shouldn't be used without a valid start point. Can cause
        # tool crash without it.
        if obj.MinTravel and obj.UseStartPoint and obj.StartPoint is not None:
            params['sort_mode'] = 2
        return params

    def opShapes(self, obj, commandlist):
        PathLog.track()

        job = PathUtils.findParentJob(obj)
        if not job or not job.Base:
            return
        baseobject = job.Base

        if obj.Base:
            PathLog.debug("base items exist.  Processing...")
            removalshapes = []
            for b in obj.Base:
                PathLog.debug("Base item: {}".format(b))
                for sub in b[1]:
                    if "Face" in sub:
                        shape = Part.makeCompound([getattr(b[0].Shape, sub)])
                    else:
                        edges = [getattr(b[0].Shape, sub) for sub in b[1]]
                        shape = Part.makeFace(edges, 'Part::FaceMakerSimple')

                    env = PathUtils.getEnvelope(baseobject.Shape, subshape=shape, depthparams=self.depthparams)
                    obj.removalshape = env.cut(baseobject.Shape)
                    removalshapes.append((obj.removalshape, False))
        else:  # process the job base object as a whole
            PathLog.debug("processing the whole job base object")

            env = PathUtils.getEnvelope(baseobject.Shape, subshape=None, depthparams=self.depthparams)
            obj.removalshape = env.cut(baseobject.Shape)
            removalshapes = [(obj.removalshape, False)]
        return removalshapes

    def opSetDefaultValues(self, obj):
        obj.StepOver = 100
        obj.ZigZagAngle = 45


def Create(name):
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectPocket(obj)
    return obj
