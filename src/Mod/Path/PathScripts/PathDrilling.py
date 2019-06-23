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
__url__ = "http://www.freecadweb.org"
__doc__ = "Path Drilling operation."

LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectDrilling(PathCircularHoleBase.ObjectOp):
    '''Proxy object for Drilling operation.'''

    def circularHoleFeatures(self, obj):
        '''circularHoleFeatures(obj) ... drilling works on anything, turn on all Base geometries and Locations.'''
        return PathOp.FeatureBaseGeometry | PathOp.FeatureLocations

    def initCircularHoleOperation(self, obj):
        '''initCircularHoleOperation(obj) ... add drilling specific properties to obj.'''
        obj.addProperty("App::PropertyLength", "PeckDepth", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "Incremental Drill depth before retracting to clear chips"))
        obj.addProperty("App::PropertyBool", "PeckEnabled", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable pecking"))
        obj.addProperty("App::PropertyFloat", "DwellTime", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "The time to dwell between peck cycles"))
        obj.addProperty("App::PropertyBool", "DwellEnabled", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable dwell"))
        obj.addProperty("App::PropertyBool", "AddTipLength", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "Calculate the tip length and subtract from final depth"))
        obj.addProperty("App::PropertyEnumeration", "ReturnLevel", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "Controls how tool retracts Default=G98"))
        obj.ReturnLevel = ['G98', 'G99']  # this is the direction that the Contour runs

        obj.addProperty("App::PropertyDistance", "RetractHeight", "Drill", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height where feed starts and height during retract tool when path is finished"))

    def circularHoleExecute(self, obj, holes):
        '''circularHoleExecute(obj, holes) ... generate drill operation for each hole in holes.'''
        PathLog.track()

        self.commandlist.append(Path.Command("(Begin Drilling)"))

        # rapid to clearance height
        self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))

        tiplength = 0.0
        if obj.AddTipLength:
            tiplength = PathUtils.drillTipLength(self.tool)

        holes = PathUtils.sort_jobs(holes, ['x', 'y'])
        self.commandlist.append(Path.Command('G90'))
        self.commandlist.append(Path.Command(obj.ReturnLevel))

        # ml: I'm not sure whey these were here, they seem redundant
        ## rapid to first hole location, with spindle still retracted:
        #p0 = holes[0]
        #self.commandlist.append(Path.Command('G0', {'X': p0['x'], 'Y': p0['y'], 'F': self.horizRapid}))
        ## move tool to clearance plane
        #self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))

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

        for p in holes:
            params = {}
            params['X'] = p['x']
            params['Y'] = p['y']
            params.update(cmdParams)
            self.commandlist.append(Path.Command(cmd, params))

        self.commandlist.append(Path.Command('G80'))

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj, job) ... set default value for RetractHeight'''
        obj.RetractHeight = 10

def SetupProperties():
    setup = []
    setup.append("PeckDepth")
    setup.append("PeckEnabled")
    setup.append("DwellTime")
    setup.append("DwellEnabled")
    setup.append("AddTipLength")
    setup.append("ReturnLevel")
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
