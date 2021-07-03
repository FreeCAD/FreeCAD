# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Schildkroet                                        *
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

__title__ = "Path Drilling Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Path Drilling operation."
__contributors__ = "IMBack!"


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectDrilling(PathCircularHoleBase.ObjectOp):
    '''Proxy object for Drilling operation.'''

    def circularHoleFeatures(self, obj):
        '''circularHoleFeatures(obj) ... drilling works on anything, turn on all Base geometries and Locations.'''
        return PathOp.FeatureBaseGeometry | PathOp.FeatureLocations | PathOp.FeatureCoolant

    def initCircularHoleOperation(self, obj):
        '''initCircularHoleOperation(obj) ... add drilling specific properties to obj.'''
        obj.addProperty("App::PropertyLength", "PeckDepth", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "Incremental Drill depth before retracting to clear chips"))
        obj.addProperty("App::PropertyBool", "PeckEnabled", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable pecking"))
        obj.addProperty("App::PropertyFloat", "DwellTime", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "The time to dwell between peck cycles"))
        obj.addProperty("App::PropertyBool", "DwellEnabled", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable dwell"))
        obj.addProperty("App::PropertyBool", "AddTipLength", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "Calculate the tip length and subtract from final depth"))
        obj.addProperty("App::PropertyEnumeration", "ReturnLevel", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "Controls how tool retracts Default=G99"))
        obj.addProperty("App::PropertyDistance", "RetractHeight", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height where feed starts and height during retract tool when path is finished while in a peck operation"))
        obj.addProperty("App::PropertyEnumeration", "ExtraOffset", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "How far the drill depth is extended"))

        obj.ReturnLevel = ['G99', 'G98']  # Canned Cycle Return Level
        obj.ExtraOffset = ['None', 'Drill Tip', '2x Drill Tip']  # Canned Cycle Return Level

    def circularHoleExecute(self, obj, holes):
        '''circularHoleExecute(obj, holes) ... generate drill operation for each hole in holes.'''
        PathLog.track()

        self.commandlist.append(Path.Command("(Begin Drilling)"))

        # rapid to clearance height
        self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))

        tiplength = 0.0
        if obj.ExtraOffset == 'Drill Tip':
            tiplength = PathUtils.drillTipLength(self.tool)
        elif obj.ExtraOffset == '2x Drill Tip':
            tiplength = PathUtils.drillTipLength(self.tool) * 2

        holes = PathUtils.sort_jobs(holes, ['x', 'y'])
        self.commandlist.append(Path.Command('G90'))
        self.commandlist.append(Path.Command(obj.ReturnLevel))

        cmd = "G81"
        cmdParams = {}
        cmdParams['Z'] = obj.FinalDepth.Value - tiplength
        cmdParams['F'] = self.vertFeed
        cmdParams['R'] = obj.RetractHeight.Value

        if obj.PeckEnabled and obj.PeckDepth.Value > 0:
            cmd = "G83"
            cmdParams['Q'] = obj.PeckDepth.Value
        elif obj.DwellEnabled and obj.DwellTime > 0:
            cmd = "G82"
            cmdParams['P'] = obj.DwellTime

        # parentJob = PathUtils.findParentJob(obj)
        # startHeight = obj.StartDepth.Value + parentJob.SetupSheet.SafeHeightOffset.Value
        startHeight = obj.StartDepth.Value + self.job.SetupSheet.SafeHeightOffset.Value

        for p in holes:
            params = {}
            params['X'] = p['x']
            params['Y'] = p['y']

            # move to hole location
            self.commandlist.append(Path.Command('G0', {'X': p['x'], 'Y': p['y'], 'F': self.horizRapid}))
            self.commandlist.append(Path.Command('G0', {'Z': startHeight, 'F': self.vertRapid}))
            self.commandlist.append(Path.Command('G1', {'Z': obj.StartDepth.Value, 'F': self.vertFeed}))

            # Update changes to parameters
            params.update(cmdParams)

            # Perform canned drilling cycle
            self.commandlist.append(Path.Command(cmd, params))

            # Cancel canned drilling cycle
            self.commandlist.append(Path.Command('G80'))
            self.commandlist.append(Path.Command('G0', {'Z': obj.SafeHeight.Value}))

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj, job) ... set default value for RetractHeight'''
        obj.ExtraOffset = "None"

        if hasattr(job.SetupSheet, 'RetractHeight'):
            obj.RetractHeight = job.SetupSheet.RetractHeight
        elif self.applyExpression(obj, 'RetractHeight', 'StartDepth+SetupSheet.SafeHeightOffset'):
            if not job:
                obj.RetractHeight = 10
            else:
                obj.RetractHeight.Value = obj.StartDepth.Value + 1.0

        if hasattr(job.SetupSheet, 'PeckDepth'):
            obj.PeckDepth = job.SetupSheet.PeckDepth
        elif self.applyExpression(obj, 'PeckDepth', 'OpToolDiameter*0.75'):
            obj.PeckDepth = 1

        if hasattr(job.SetupSheet, 'DwellTime'):
            obj.DwellTime = job.SetupSheet.DwellTime
        else:
            obj.DwellTime = 1

def SetupProperties():
    setup = []
    setup.append("PeckDepth")
    setup.append("PeckEnabled")
    setup.append("DwellTime")
    setup.append("DwellEnabled")
    setup.append("AddTipLength")
    setup.append("ReturnLevel")
    setup.append("ExtraOffset")
    setup.append("RetractHeight")
    return setup

def Create(name, obj = None):
    '''Create(name) ... Creates and returns a Drilling operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)

    obj.Proxy = ObjectDrilling(obj, name)
    if obj.Proxy:
        obj.Proxy.findAllHoles(obj)

    return obj