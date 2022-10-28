# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCADGui
import Path

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class AdvancedPreferencesPage:
    def __init__(self, parent=None):
        self.form = FreeCADGui.PySideUic.loadUi(":preferences/Advanced.ui")
        self.form.WarningSuppressAllSpeeds.stateChanged.connect(self.updateSelection)
        self.form.EnableAdvancedOCLFeatures.stateChanged.connect(self.updateSelection)

    def saveSettings(self):
        Path.Preferences.setPreferencesAdvanced(
            self.form.EnableAdvancedOCLFeatures.isChecked(),
            self.form.WarningSuppressAllSpeeds.isChecked(),
            self.form.WarningSuppressRapidSpeeds.isChecked(),
            self.form.WarningSuppressSelectionMode.isChecked(),
            self.form.WarningSuppressOpenCamLib.isChecked(),
            self.form.WarningSuppressVelocity.isChecked(),
        )

    def loadSettings(self):
        Path.Log.track()
        self.form.WarningSuppressAllSpeeds.setChecked(
            Path.Preferences.suppressAllSpeedsWarning()
        )
        self.form.WarningSuppressRapidSpeeds.setChecked(
            Path.Preferences.suppressRapidSpeedsWarning(False)
        )
        self.form.WarningSuppressSelectionMode.setChecked(
            Path.Preferences.suppressSelectionModeWarning()
        )
        self.form.EnableAdvancedOCLFeatures.setChecked(
            Path.Preferences.advancedOCLFeaturesEnabled()
        )
        self.form.WarningSuppressOpenCamLib.setChecked(
            Path.Preferences.suppressOpenCamLibWarning()
        )
        self.form.WarningSuppressVelocity.setChecked(
            Path.Preferences.suppressVelocity()
        )
        self.updateSelection()

    def updateSelection(self, state=None):
        self.form.WarningSuppressOpenCamLib.setEnabled(
            self.form.EnableAdvancedOCLFeatures.isChecked()
        )

        if self.form.WarningSuppressAllSpeeds.isChecked():
            self.form.WarningSuppressRapidSpeeds.setChecked(True)
            self.form.WarningSuppressRapidSpeeds.setEnabled(False)
        else:
            self.form.WarningSuppressRapidSpeeds.setEnabled(True)
