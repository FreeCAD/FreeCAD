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


    def circularHoleExecute(self, obj, holes):
        PathLog.track()

        self.commandlist.append(Path.Command("(Begin Thread Milling)"))

        # rapid to clearance height
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

