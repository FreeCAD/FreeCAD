# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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
import Path
import PathScripts.PathCircularHoleBase as PathCircularHoleBase
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils

from PySide import QtCore

__title__ = "Path Thread Milling Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Path thread milling operation."

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


def radiiMetricInternal(majorDia, minorDia, toolDia, toolCrest = None):
    '''internlThreadRadius(majorDia, minorDia, toolDia, toolCrest) ... returns the maximum radius for thread.'''
    if toolCrest is None:
        toolCrest = 0.0
    # see https://www.amesweb.info/Screws/Internal-Metric-Thread-Dimensions-Chart.aspx
    H = ((majorDia - minorDia) / 2.0 ) * 1.6             # (D - d)/2 = 5/8 * H
    outerTip = majorDia / 2.0 + H / 8.0
    toolTip = outerTip - toolCrest * 0.8660254037844386  # math.sqrt(3)/2 ... 60deg triangle height
    return ((minorDia - toolDia) / 2, toolTip - toolDia / 2)

def threadPasses(count, radii, majorDia, minorDia, toolDia, toolCrest = None):
    minor, major = radii(majorDia, minorDia, toolDia, toolCrest)
    dr  = (major - minor) / count
    return [major - dr * (count - (i + 1)) for i in range(count)]

class ObjectThreadMilling(PathCircularHoleBase.ObjectOp):
    '''Proxy object for thread milling operation.'''

    LeftHand = 'LeftHand'
    RightHand = 'RightHand'
    ThreadTypeCustom = 'Custom'
    ThreadTypeMetricInternal = 'Metric - internal'
    DirectionClimb = 'Climb'

    ThreadOrientations = [LeftHand, RightHand]
    ThreadTypes = [ThreadTypeCustom, ThreadTypeMetricInternal]
    Directions =  [DirectionClimb]

    def circularHoleFeatures(self, obj):
        return PathOp.FeatureBaseGeometry

    def initCircularHoleOperation(self, obj):
        obj.addProperty("App::PropertyEnumeration", "ThreadOrientation", "Thread", QtCore.QT_TRANSLATE_NOOP("App::Property", "Set thread orientation"))
        obj.ThreadOrientation = self.ThreadOrientations
        obj.addProperty("App::PropertyEnumeration", "ThreadType", "Thread", QtCore.QT_TRANSLATE_NOOP("App::Property", "Currently only internal"))
        obj.ThreadType = self.ThreadTypes
        obj.addProperty("App::PropertyString", "ThreadName", "Thread", QtCore.QT_TRANSLATE_NOOP("App::Property", "Devfines which standard thread was chosen"))
        obj.addProperty("App::PropertyLength", "MajorDiameter", "Thread", QtCore.QT_TRANSLATE_NOOP("App::Property", "Set thread's major diameter"))
        obj.addProperty("App::PropertyLength", "MinorDiameter", "Thread", QtCore.QT_TRANSLATE_NOOP("App::Property", "Set thread's minor diameter"))
        obj.addProperty("App::PropertyLength", "Pitch", "Thread", QtCore.QT_TRANSLATE_NOOP("App::Property", "Set thread's pitch"))
        obj.addProperty("App::PropertyInteger", "ThreadFit", "Thread", QtCore.QT_TRANSLATE_NOOP("App::Property", "Set how many passes are used to cut the thread"))
        obj.addProperty("App::PropertyInteger", "Passes", "Mill", QtCore.QT_TRANSLATE_NOOP("App::Property", "Set how many passes are used to cut the thread"))
        obj.addProperty("App::PropertyEnumeration", "Direction", "Mill", QtCore.QT_TRANSLATE_NOOP("App::Property", "Direction of thread cutting operation"))
        obj.Direction = self.Directions

    def threadStartDepth(self, obj):
        if self.ThreadDirection == self.RightHand:
            if self.Direction == self.DirectionClimb:
                return self.FinalDepth
            return self.StartDepth
        if self.Direction == self.DirectionClimb:
            return self.StartDepth
        return self.FinalDepth

    def threadFinalDepth(self, obj):
        if self.ThreadDirection == self.RightHand:
            if self.Direction == self.DirectionClimb:
                return self.StartDepth
            return self.FinalDepth
        if self.Direction == self.DirectionClimb:
            return self.FinalDepth
        return self.StartDepth

    def threadDirectionCmd(self, obj):
        if self.ThreadDirection == self.RightHand:
            if self.Direction == self.DirectionClimb:
                return 'G2'
            return 'G3'
        if self.Direction == self.DirectionClimb:
            return 'G3'
        return 'G2'

    def threadPassRadii(self, obj):
        rMajor = (obj.MajorDiameter.Value - self.tool.Diameter) / 2.0
        rMinor = (obj.MinorDiameter.Value - self.tool.Diameter) / 2.0
        if obj.Passes < 1:
            obj.Passes = 1
        rPass  = (rMajor - rMinor) / obj.Passes
        passes = [rMajor]
        for i in range(1, obj.Passes):
            passes.append(rMajor - rPass * i)
        return list(reversed(passes))

    def executeThreadMill(self, obj, loc, cmd, zStart, zFinal, pitch):
        hPitch = obj.Pitch.Value / 2.0
        if zStart > zFinal:
            hPitch = -hPitch

        self.commandlist.append(Path.Command('G0', {'Z': zStart, 'F': self.vertRapid}))
        for r in threadPasses(obj.Passes, radiiMetricInternal, obj.MajorDiameter, obj.MinorDiameter, self.tool.Diameter, 0):
            pass

    def circularHoleExecute(self, obj, holes):
        PathLog.track()

        self.commandlist.append(Path.Command("(Begin Thread Milling)"))

        cmd = self.threadDirectionCmd(obj)
        zStart = self.threadStartDepth(obj).Value
        zFinal = self.threadFinalDepth(obj).Value
        pitch = obj.Pitch.Value
        if pitch <= 0:
            PathLog.error("Cannot create thread with pitch {}".format(pitch))
            return

        # rapid to clearance height
        for loc in holes:
            self.executeThreadMill(obj, loc, cmd, zStart, zFinal, pitch)

        self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))


    def opSetDefaultValues(self, obj, job):
        obj.ThreadOrientation = self.RightHand
        obj.ThreadType = self.ThreadTypeMetricInternal
        obj.ThreadFit = 50
        obj.Pitch = 1
        obj.Passes = 1
        obj.Direction = self.DirectionClimb


def SetupProperties():
    setup = []
    setup.append("ThreadOrientation")
    setup.append("ThreadType")
    setup.append("ThreadName")
    setup.append("ThreadFit")
    setup.append("MajorDiameter")
    setup.append("MinorDiameter")
    setup.append("Pitch")
    setup.append("Passes")
    setup.append("Direction")
    return setup


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a thread milling operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectThreadMilling(obj, name)
    if obj.Proxy:
        obj.Proxy.findAllHoles(obj)
    return obj

