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

import ArchPanel
import FreeCAD
import Part
import Path
import PathScripts.PathAreaOp as PathAreaOp
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import numpy

from PathScripts.PathUtils import depth_params
from PySide import QtCore

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

__title__ = "Path Profile Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path Profile object and FreeCAD command"""


class ObjectProfile(PathAreaOp.ObjectOp):

    def initOperation(self, obj):

        # Profile Properties
        obj.addProperty("App::PropertyEnumeration", "Side", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Side of edge that tool should cut"))
        obj.Side = ['Inside', 'Outside']  # side of profile that cutter is on in relation to direction of profile
        obj.addProperty("App::PropertyEnumeration", "Direction", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.Direction = ['CW', 'CCW']  # this is the direction that the profile runs
        obj.addProperty("App::PropertyBool", "UseComp", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "make True, if using Cutter Radius Compensation"))
        obj.addProperty("App::PropertyDistance", "OffsetExtra", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Extra value to stay away from final profile- good for roughing toolpath"))
        obj.addProperty("App::PropertyEnumeration", "JoinType", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Controls how tool moves around corners. Default=Round"))
        obj.JoinType = ['Round', 'Square', 'Miter']  # this is the direction that the Contour runs
        obj.addProperty("App::PropertyFloat", "MiterLimit", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Maximum distance before a miter join is truncated"))

        # Face specific Properties
        obj.addProperty("App::PropertyBool", "processHoles", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile holes as well as the outline"))
        obj.addProperty("App::PropertyBool", "processPerimeter", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile the outline"))
        obj.addProperty("App::PropertyBool", "processCircles", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile round holes"))

        obj.Proxy = self

    def opSetDefaultValues(self, obj):
        obj.Side = "Outside"
        obj.OffsetExtra = 0.0
        obj.Direction = "CW"
        obj.UseComp = True
        obj.JoinType = "Round"
        obj.MiterLimit = 0.1

        obj.processHoles = False
        obj.processCircles = False
        obj.processPerimeter = True

    def onOpChanged(self, obj, prop):
        if prop == "UseComp":
            if not obj.UseComp:
                obj.setEditorMode('Side', 2)
            else:
                obj.setEditorMode('Side', 0)

        if prop == "JoinType":
            obj.setEditorMode('MiterLimit', 2)
            if obj.JoinType == 'Miter':
                obj.setEditorMode('MiterLimit', 0)

    def opFeatures(self, obj):
        return PathAreaOp.FeatureTool | PathAreaOp.FeatureDepths | PathAreaOp.FeatureHeights | PathAreaOp.FeatureStartPoint | PathAreaOp.FeatureBaseFaces

    def opUseProjection(self, obj):
        return True

    def opAreaParams(self, obj, isHole):
        params = {}
        params['Fill'] = 0
        params['Coplanar'] = 2
        params['Offset'] = 0.0
        params['SectionCount'] = -1

        offsetval = 0

        if obj.UseComp:
            offsetval = self.radius + obj.OffsetExtra.Value

        if obj.Side == 'Inside':
            offsetval = 0 - offsetval

        if isHole:
            offsetval = 0 - offsetval

        params['Offset'] = offsetval

        jointype = ['Round', 'Square', 'Miter']
        params['JoinType'] = jointype.index(obj.JoinType)

        if obj.JoinType == 'Miter':
            params['MiterLimit'] = obj.MiterLimit
        return params

    def opPathParams(self, obj, isHole):
        params = {}

        # Reverse the direction for holes
        if isHole:
            direction = "CW" if obj.Direction == "CCW" else "CCW"
        else:
            direction = obj.Direction

        if direction == 'CCW':
            params['orientation'] = 0
        else:
            params['orientation'] = 1
        return params


    def opShapes(self, obj, commandlist):
        commandlist.append(Path.Command("(" + obj.Label + ")"))

        if obj.UseComp:
            commandlist.append(Path.Command("(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"))
        else:
            commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        job = PathUtils.findParentJob(obj)
        if not job or not job.Base:
            return

        baseobject = job.Base
        shapes = []

        if obj.Base:  # The user has selected subobjects from the base.  Process each.
            holes = []
            faces = []
            for b in obj.Base:
                for sub in b[1]:
                    shape = getattr(b[0].Shape, sub)
                    if isinstance(shape, Part.Face):
                        faces.append(shape)
                        if numpy.isclose(abs(shape.normalAt(0, 0).z), 1):  # horizontal face
                            holes += shape.Wires[1:]
                    else:
                        FreeCAD.Console.PrintWarning("found a base object which is not a face.  Can't continue.")
                        return

            for wire in holes:
                f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                drillable = PathUtils.isDrillable(baseobject.Shape, wire)
                if (drillable and obj.processCircles) or (not drillable and obj.processHoles):
                    env = PathUtils.getEnvelope(baseobject.Shape, subshape=f, depthparams=self.depthparams)
                    shapes.append((env, True))

            if len(faces) > 0:
                profileshape = Part.makeCompound(faces)

            if obj.processPerimeter:
                env = PathUtils.getEnvelope(baseobject.Shape, subshape=profileshape, depthparams=self.depthparams)
                shapes.append((env, False))

        else:  # Try to build targets from the job base
            if hasattr(baseobject, "Proxy"):
                if isinstance(baseobject.Proxy, ArchPanel.PanelSheet):  # process the sheet
                    if obj.processCircles or obj.processHoles:
                        for shape in baseobject.Proxy.getHoles(baseobject, transform=True):
                            for wire in shape.Wires:
                                drillable = PathUtils.isDrillable(baseobject.Proxy, wire)
                                if (drillable and obj.processCircles) or (not drillable and obj.processHoles):
                                    f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                                    env = PathUtils.getEnvelope(baseobject.Shape, subshape=f, depthparams=self.depthparams)
                                    shapes.append((env, True))

                    if obj.processPerimeter:
                        for shape in baseobject.Proxy.getOutlines(baseobject, transform=True):
                            for wire in shape.Wires:
                                f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                                env = PathUtils.getEnvelope(baseobject.Shape, subshape=f, depthparams=self.depthparams)
                                shapes.append((env, False))

        return shapes

def Create(name):
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectProfile(obj)
    return obj
