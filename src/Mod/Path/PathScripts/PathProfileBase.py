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
import Path
import PathScripts.PathAreaOp as PathAreaOp
import PathScripts.PathUtils as PathUtils
import PathScripts.PathLog as PathLog

from PySide import QtCore

"""Path Profile from base features"""

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui


# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

__title__ = "Path Profile Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path Profile object and FreeCAD command for operating on sets of features"""


class ObjectProfile(PathAreaOp.ObjectOp):

    def opFeatures(self, obj):
        return PathAreaOp.FeatureTool | PathAreaOp.FeatureDepths | PathAreaOp.FeatureHeights | PathAreaOp.FeatureStartPoint

    def initOperation(self, obj):
        # Profile Properties
        obj.addProperty("App::PropertyEnumeration", "Side", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Side of edge that tool should cut"))
        obj.Side = ['Outside', 'Inside']  # side of profile that cutter is on in relation to direction of profile
        obj.addProperty("App::PropertyEnumeration", "Direction", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.Direction = ['CW', 'CCW']  # this is the direction that the profile runs
        obj.addProperty("App::PropertyBool", "UseComp", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "make True, if using Cutter Radius Compensation"))

        obj.addProperty("App::PropertyDistance", "OffsetExtra", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Extra value to stay away from final profile- good for roughing toolpath"))
        obj.addProperty("App::PropertyEnumeration", "JoinType", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Controls how tool moves around corners. Default=Round"))
        obj.JoinType = ['Round', 'Square', 'Miter']  # this is the direction that the Profile runs
        obj.addProperty("App::PropertyFloat", "MiterLimit", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Maximum distance before a miter join is truncated"))
        obj.setEditorMode('MiterLimit', 2)

        obj.Proxy = self

    def onOpChanged(self, obj, prop):
        if prop == "UseComp":
            if not obj.UseComp:
                obj.setEditorMode('Side', 2)
            else:
                obj.setEditorMode('Side', 0)

        if prop == 'JoinType':
            if obj.JoinType == 'Miter':
                obj.setEditorMode('MiterLimit', 0)
            else:
                obj.setEditorMode('MiterLimit', 2)

    def opAreaParams(self, obj, isHole):
        params = {}
        params['Fill'] = 0
        params['Coplanar'] = 0
        params['SectionCount'] = -1

        if obj.UseComp:
            if obj.Side == 'Inside':
                params['Offset'] = 0 - self.radius+obj.OffsetExtra.Value
            else:
                params['Offset'] = self.radius+obj.OffsetExtra.Value
        else:
            params['Offset'] = 0.0

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

    def opUseProjection(self, obj):
        return True

    def opSetDefaultValues(self, obj):
        obj.Side = "Outside"
        obj.OffsetExtra = 0.0
        obj.Direction = "CW"
        obj.UseComp = True
        obj.JoinType = "Round"
        obj.MiterLimit = 0.1

