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
import PathScripts.PathPreferences as PathPreferences
import PySide


class AdvancedPreferencesPage:
    def __init__(self, parent=None):
        self.form = FreeCADGui.PySideUic.loadUi(":preferences/Advanced.ui")
        self.form.WarningSuppressAllSpeeds.stateChanged.connect(self.updateSelection)
        self.form.EnableAdvancedOCLFeatures.stateChanged.connect(self.updateSelection)

    def saveSettings(self):
        PathPreferences.setPreferencesAdvanced(
            self.form.EnableAdvancedOCLFeatures.isChecked(),
            self.form.WarningSuppressAllSpeeds.isChecked(),
            self.form.WarningSuppressRapidSpeeds.isChecked(),
            self.form.WarningSuppressSelectionMode.isChecked(),
            self.form.WarningSuppressOpenCamLib.isChecked(),
        )

    def loadSettings(self):
        self.form.WarningSuppressAllSpeeds.setChecked(
            PathPreferences.suppressAllSpeedsWarning()
        )
        self.form.WarningSuppressRapidSpeeds.setChecked(
            PathPreferences.suppressRapidSpeedsWarning(False)
        )
        self.form.WarningSuppressSelectionMode.setChecked(
            PathPreferences.suppressSelectionModeWarning()
        )
        self.form.EnableAdvancedOCLFeatures.setChecked(
            PathPreferences.advancedOCLFeaturesEnabled()
        )
        self.form.WarningSuppressOpenCamLib.setChecked(
            PathPreferences.suppressOpenCamLibWarning()
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
