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
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathPreferencesPathDressup as PathPreferencesPathDressup

from PySide import QtCore


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class HoldingTagPreferences:
    DefaultHoldingTagWidth = 'DefaultHoldingTagWidth'
    DefaultHoldingTagHeight = 'DefaultHoldingTagHeight'
    DefaultHoldingTagAngle = 'DefaultHoldingTagAngle'
    DefaultHoldingTagRadius = 'DefaultHoldingTagRadius'
    DefaultHoldingTagCount = 'DefaultHoldingTagCount'

    @classmethod
    def defaultWidth(cls, ifNotSet):
        value = PathPreferences.preferences().GetFloat(cls.DefaultHoldingTagWidth, ifNotSet)
        if value == 0.0:
            return ifNotSet
        return value

    @classmethod
    def defaultHeight(cls, ifNotSet):
        value = PathPreferences.preferences().GetFloat(cls.DefaultHoldingTagHeight, ifNotSet)
        if value == 0.0:
            return ifNotSet
        return value

    @classmethod
    def defaultAngle(cls, ifNotSet=45.0):
        value = PathPreferences.preferences().GetFloat(cls.DefaultHoldingTagAngle, ifNotSet)
        if value < 10.0:
            return ifNotSet
        return value

    @classmethod
    def defaultCount(cls, ifNotSet=4):
        value = PathPreferences.preferences().GetUnsigned(cls.DefaultHoldingTagCount, ifNotSet)
        if value < 2:
            return float(ifNotSet)
        return float(value)

    @classmethod
    def defaultRadius(cls, ifNotSet=0.0):
        return PathPreferences.preferences().GetFloat(cls.DefaultHoldingTagRadius, ifNotSet)

    def __init__(self):
        if FreeCAD.GuiUp:
            import FreeCADGui
            self.form = FreeCADGui.PySideUic.loadUi(":/preferences/PathDressupHoldingTags.ui")
            self.label = translate("Path_DressupTag", 'Holding Tag')

    def loadSettings(self):
        self.form.ifWidth.setText(FreeCAD.Units.Quantity(self.defaultWidth(0), FreeCAD.Units.Length).UserString)
        self.form.ifHeight.setText(FreeCAD.Units.Quantity(self.defaultHeight(0), FreeCAD.Units.Length).UserString)
        self.form.dsbAngle.setValue(self.defaultAngle())
        self.form.ifRadius.setText(FreeCAD.Units.Quantity(self.defaultRadius(), FreeCAD.Units.Length).UserString)
        self.form.sbCount.setValue(self.defaultCount())

    def saveSettings(self):
        pref = PathPreferences.preferences()
        pref.SetFloat(self.DefaultHoldingTagWidth, FreeCAD.Units.Quantity(self.form.ifWidth.text()).Value)
        pref.SetFloat(self.DefaultHoldingTagHeight, FreeCAD.Units.Quantity(self.form.ifHeight.text()).Value)
        pref.SetFloat(self.DefaultHoldingTagAngle, self.form.dsbAngle.value())
        pref.SetFloat(self.DefaultHoldingTagRadius, FreeCAD.Units.Quantity(self.form.ifRadius.text()))
        pref.SetUnsigned(self.DefaultHoldingTagCount, self.form.sbCount.value())

    @classmethod
    def preferencesPage(cls):
        return HoldingTagPreferences()

PathPreferencesPathDressup.RegisterDressup(HoldingTagPreferences)
