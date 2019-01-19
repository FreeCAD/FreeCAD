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
import PathScripts.PathAreaOp as PathAreaOp
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp

from PySide import QtCore

__title__ = "Base Path Profile Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base class and implementation for Path profile operations."

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectProfile(PathAreaOp.ObjectOp):
    '''Base class for proxy objects of all profile operations.'''

    def initAreaOp(self, obj):
        '''initAreaOp(obj) ... creates all profile specific properties.
        Do not overwrite.'''
        # Profile Properties
        obj.addProperty("App::PropertyEnumeration", "Side", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Side of edge that tool should cut"))
        obj.Side = ['Outside', 'Inside']  # side of profile that cutter is on in relation to direction of profile
        obj.addProperty("App::PropertyEnumeration", "Direction", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.Direction = ['CW', 'CCW']  # this is the direction that the profile runs
        obj.addProperty("App::PropertyBool", "UseComp", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Make True, if using Cutter Radius Compensation"))

        obj.addProperty("App::PropertyDistance", "OffsetExtra", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Extra value to stay away from final profile- good for roughing toolpath"))
        obj.addProperty("App::PropertyEnumeration", "JoinType", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Controls how tool moves around corners. Default=Round"))
        obj.JoinType = ['Round', 'Square', 'Miter']  # this is the direction that the Profile runs
        obj.addProperty("App::PropertyFloat", "MiterLimit", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Maximum distance before a miter join is truncated"))
        obj.setEditorMode('MiterLimit', 2)

    def areaOpOnChanged(self, obj, prop):
        '''areaOpOnChanged(obj, prop) ... updates Side and MiterLimit visibility depending on changed properties.
        Do not overwrite.'''
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

    def areaOpOnDocumentRestored(self, obj):
        for prop in ['UseComp', 'JoinType']:
            self.areaOpOnChanged(obj, prop)

    def areaOpAreaParams(self, obj, isHole):
        '''areaOpAreaParams(obj, isHole) ... returns dictionary with area parameters.
        Do not overwrite.'''
        params = {}
        params['Fill'] = 0
        params['Coplanar'] = 0
        params['SectionCount'] = -1

        offset = 0.0
        if obj.UseComp:
            offset = self.radius + obj.OffsetExtra.Value
        if obj.Side == 'Inside':
            offset = 0 - offset
        if isHole:
            offset = 0 - offset
        params['Offset'] = offset

        jointype = ['Round', 'Square', 'Miter']
        params['JoinType'] = jointype.index(obj.JoinType)

        if obj.JoinType == 'Miter':
            params['MiterLimit'] = obj.MiterLimit

        return params

    def areaOpPathParams(self, obj, isHole):
        '''areaOpPathParams(obj, isHole) ... returns dictionary with path parameters.
        Do not overwrite.'''
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
        if not obj.UseComp:
            if direction == 'CCW':
                params['orientation'] = 1
            else:
                params['orientation'] = 0

        return params

    def areaOpUseProjection(self, obj):
        '''areaOpUseProjection(obj) ... returns True'''
        return True

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... sets default values.
        Do not overwrite.'''
        obj.Side = "Outside"
        obj.OffsetExtra = 0.0
        obj.Direction = "CW"
        obj.UseComp = True
        obj.JoinType = "Round"
        obj.MiterLimit = 0.1

def SetupProperties():
    setup = []
    setup.append('Side')
    setup.append('OffsetExtra')
    setup.append('Direction')
    setup.append('UseComp')
    setup.append('JoinType')
    setup.append('MiterLimit')
    return setup
