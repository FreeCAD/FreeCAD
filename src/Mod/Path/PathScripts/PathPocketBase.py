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

import PathScripts.PathAreaOp as PathAreaOp
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp

from PySide import QtCore

__title__ = "Base Path Pocket Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base class and implementation for Path pocket operations."

LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ObjectPocket(PathAreaOp.ObjectOp):
    '''Base class for proxy objects of all pocket operations.'''

    def areaOpFeatures(self, obj):
        '''areaOpFeatures(obj) ... Pockets have a FinishDepth and work on Faces'''
        return PathOp.FeatureBaseFaces | PathOp.FeatureFinishDepth | self.pocketOpFeatures(obj)

    def pocketOpFeatures(self, obj):
        # pylint: disable=unused-argument
        return 0

    def initPocketOp(self, obj):
        '''initPocketOp(obj) ... overwrite to initialize subclass.
        Can safely be overwritten by subclass.'''
        pass # pylint: disable=unnecessary-pass

    def pocketInvertExtraOffset(self):
        '''pocketInvertExtraOffset() ... return True if ExtraOffset's direction is inward.
        Can safely be overwritten by subclass.'''
        return False

    def initAreaOp(self, obj):
        '''initAreaOp(obj) ... create pocket specific properties.
        Do not overwrite, implement initPocketOp(obj) instead.'''
        PathLog.track()

        # Pocket Properties
        obj.addProperty("App::PropertyEnumeration", "CutMode", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part ClockWise (CW) or CounterClockWise (CCW)"))
        obj.CutMode = ['Climb', 'Conventional']
        obj.addProperty("App::PropertyDistance", "ExtraOffset", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Extra offset to apply to the operation. Direction is operation dependent."))
        obj.addProperty("App::PropertyEnumeration", "StartAt", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Start pocketing at center or boundary"))
        obj.StartAt = ['Center', 'Edge']
        obj.addProperty("App::PropertyPercent", "StepOver", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Percent of cutter diameter to step over on each pass"))
        obj.addProperty("App::PropertyFloat", "ZigZagAngle", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Angle of the zigzag pattern"))
        obj.addProperty("App::PropertyEnumeration", "OffsetPattern", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "Clearing pattern to use"))
        obj.OffsetPattern = ['ZigZag', 'Offset', 'Spiral', 'ZigZagOffset', 'Line', 'Grid', 'Triangle']
        obj.addProperty("App::PropertyBool", "MinTravel", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Use 3D Sorting of Path"))
        obj.addProperty("App::PropertyBool", "KeepToolDown", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Attempts to avoid unnecessary retractions."))

        self.initPocketOp(obj)

    def areaOpRetractTool(self, obj):
        PathLog.debug("retracting tool: %d" % (not obj.KeepToolDown))
        return not obj.KeepToolDown

    def areaOpUseProjection(self, obj):
        '''areaOpUseProjection(obj) ... return False'''
        return False

    def areaOpAreaParams(self, obj, isHole):
        '''areaOpAreaParams(obj, isHole) ... return dictionary with pocket's area parameters'''
        params = {}
        params['Fill'] = 0
        params['Coplanar'] = 0
        params['PocketMode'] = 1
        params['SectionCount'] = -1
        params['Angle'] = obj.ZigZagAngle
        params['FromCenter'] = (obj.StartAt == "Center")
        params['PocketStepover'] = (self.radius * 2) * (float(obj.StepOver)/100)
        extraOffset = obj.ExtraOffset.Value
        if self.pocketInvertExtraOffset():
            extraOffset = 0 - extraOffset
        params['PocketExtraOffset'] = extraOffset
        params['ToolRadius'] = self.radius

        Pattern = ['ZigZag', 'Offset', 'Spiral', 'ZigZagOffset', 'Line', 'Grid', 'Triangle']
        params['PocketMode'] = Pattern.index(obj.OffsetPattern) + 1
        return params

    def areaOpPathParams(self, obj, isHole):
        '''areaOpAreaParams(obj, isHole) ... return dictionary with pocket's path parameters'''
        params = {}

        CutMode = ['Conventional', 'Climb']
        params['orientation'] = CutMode.index(obj.CutMode)

        # if MinTravel is turned on, set path sorting to 3DSort
        # 3DSort shouldn't be used without a valid start point. Can cause
        # tool crash without it.
        #
        # ml: experimental feature, turning off for now (see https://forum.freecadweb.org/viewtopic.php?f=15&t=24422&start=30#p192458)
        # realthunder: I've fixed it with a new sorting algorithm, which I
        # tested fine, but of course need more test. Please let know if there is
        # any problem
        #
        if obj.MinTravel and obj.UseStartPoint and obj.StartPoint is not None:
            params['sort_mode'] = 3
            params['threshold'] = self.radius * 2
        return params

def SetupProperties():
    setup = []
    setup.append('CutMode')
    setup.append('ExtraOffset')
    setup.append('StepOver')
    setup.append('ZigZagAngle')
    setup.append('OffsetPattern')
    return setup
