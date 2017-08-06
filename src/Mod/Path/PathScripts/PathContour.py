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

import ArchPanel
import FreeCAD
import Part
import Path
import PathScripts.PathAreaOp as PathAreaOp
import PathScripts.PathLog as PathLog

from PathScripts import PathUtils
from PySide import QtCore

FreeCAD.setLogLevel('Path.Area', 0)

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

__title__ = "Path Contour Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path Contour object and FreeCAD command"""


class ObjectContour(PathAreaOp.ObjectOp):

    def opFeatures(self, obj):
        return PathAreaOp.FeatureTool | PathAreaOp.FeatureDepths | PathAreaOp.FeatureHeights | PathAreaOp.FeatureStartPoint

    def opUseProjection(self, obj):
        return True

    def initOperation(self, obj):
        PathLog.track()

        # Contour Properties
        obj.addProperty("App::PropertyEnumeration", "Direction", "Contour", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.Direction = ['CW', 'CCW']  # this is the direction that the Contour runs
        obj.addProperty("App::PropertyBool", "UseComp", "Contour", QtCore.QT_TRANSLATE_NOOP("App::Property", "make True, if using Cutter Radius Compensation"))

        obj.addProperty("App::PropertyDistance", "OffsetExtra", "Contour", QtCore.QT_TRANSLATE_NOOP("App::Property", "Extra value to stay away from final Contour- good for roughing toolpath"))

        obj.addProperty("App::PropertyEnumeration", "JoinType", "Contour", QtCore.QT_TRANSLATE_NOOP("App::Property", "Controls how tool moves around corners. Default=Round"))
        obj.JoinType = ['Round', 'Square', 'Miter']  # this is the direction that the Contour runs
        obj.addProperty("App::PropertyFloat", "MiterLimit", "Contour", QtCore.QT_TRANSLATE_NOOP("App::Property", "Maximum distance before a miter join is truncated"))
        obj.setEditorMode('MiterLimit', 2)

        self.endVector = None

    def opOnChanged(self, obj, prop):
        PathLog.track('prop: {}  state: {}'.format(prop, obj.State))
        if prop == 'JoinType':
            if obj.JoinType == 'Miter':
                obj.setEditorMode('MiterLimit', 0)
            else:
                obj.setEditorMode('MiterLimit', 2)

    def opSetDefaultValues(self, obj):
        obj.Direction   = "CW"
        obj.UseComp     = True
        obj.OffsetExtra = 0.0
        obj.JoinType    = "Round"
        obj.MiterLimit  = 0.1


    def opShapes(self, obj, commandlist):
        if obj.UseComp:
            commandlist.append(Path.Command("(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"))
        else:
            commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        job = PathUtils.findParentJob(obj)

        if job is None:
            return
        baseobject = job.Base
        if baseobject is None:
            return

        isPanel = False
        if hasattr(baseobject, "Proxy"):
            if isinstance(baseobject.Proxy, ArchPanel.PanelSheet):  # process the sheet
                isPanel = True
                baseobject.Proxy.execute(baseobject)
                shapes = baseobject.Proxy.getOutlines(baseobject, transform=True)
                for shape in shapes:
                    f = Part.makeFace([shape], 'Part::FaceMakerSimple')
                    thickness = baseobject.Group[0].Source.Thickness
                    return [(f.extrude(FreeCAD.Vector(0, 0, thickness)), False)]

        if hasattr(baseobject, "Shape") and not isPanel:
            return [(PathUtils.getEnvelope(partshape=baseobject.Shape, subshape=None, depthparams=self.depthparams), False)]

    def opAreaParams(self, obj, isHole):
        params = {'Fill': 0, 'Coplanar': 2}

        if obj.UseComp is False:
            params['Offset'] = 0.0
        else:
            params['Offset'] = self.radius + obj.OffsetExtra.Value

        jointype = ['Round', 'Square', 'Miter']
        params['JoinType'] = jointype.index(obj.JoinType)

        if obj.JoinType == 'Miter':
            params['MiterLimit'] = obj.MiterLimit
        return params

    def opPathParams(self, obj, isHole):
        params = {}
        if obj.Direction == 'CCW':
            params['orientation'] = 0
        else:
            params['orientation'] = 1
        return params

def Create(name):
    obj   = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectContour(obj)
    return obj
