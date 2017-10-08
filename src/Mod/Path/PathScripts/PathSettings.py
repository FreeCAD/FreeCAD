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
import Path
import PathScripts.PathLog as PathLog
import PySide

__title__ = "Settings for a Job."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "A container for all default values and job specific configuration values."

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule()
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

def translate(context, text, disambig=None):
    return PySide.QtCore.QCoreApplication.translate(context, text, disambig)

class Default:
    HorizRapid = 'DefaultHorizRapid'
    VertRapid = 'DefaultVertRapid'
    SafeHeight = 'DefaultSafeHeight'
    ClearanceHeight = 'DefaultClearanceHeight'

class Settings:

    def __init__(self, obj):
        self.obj = obj

    def setup(self):
        self.obj.Settings = self.obj.Document.addObject('Spreadsheet::Sheet', 'Settings')
        self.obj.Settings.set('A2', translate('PathSettings', 'Tool Rapid Speeds'))
        self.createSetting(3, Default.HorizRapid, '0 mm/s', translate('PathSettings', 'Horizontal'), translate('PathSettings', 'Default speed for horizzontal rapid moves.'))
        self.createSetting(4, Default.VertRapid,  '0 mm/s', translate('PathSettings', 'Vertical'),   translate('PathSettings', 'Default speed for vertical rapid moves.'))
        self.obj.Settings.set('A6', translate('PathSettings', 'Operation Heights'))
        self.createSetting(7, Default.SafeHeight,      '3 mm',  translate('PathSettings', 'Safe Height'),      translate('PathSettings', 'Default value added to StartDepth used for the safe height.'))
        self.createSetting(8, Default.ClearanceHeight, '5 mm',  translate('PathSettings', 'Clearance Height'), translate('PathSettings', 'Default value added to StartDepth used for the clearance height.'))

    def updateSetting(self, name, value):
        cell = self.obj.Settings.getCellFromAlias(name)
        PathLog.debug("updateSetting(%s, %s): %s" % (name, value, cell))
        self.obj.Settings.set(cell, value)

    def createSetting(self, row, name, value, label, desc):
        labelCell = "B%d" % row
        valueCell = "C%d" % row
        descCell  = "D%d" % row
        PathLog.debug("createSetting(%d, %s, %s): %s" % (row, name, value, valueCell))
        self.obj.Settings.set(labelCell, label)
        self.obj.Settings.set(valueCell, value)
        self.obj.Settings.setAlias(valueCell, name)
        self.obj.Settings.set(descCell, desc)

