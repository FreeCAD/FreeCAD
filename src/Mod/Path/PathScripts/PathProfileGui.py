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

__title__ = "Path Profile Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Profile operation page controller and command implementation."
__contributors__ = ""

# Standard
# Third-party
from PySide import QtCore
# FreeCAD
import FreeCAD
import FreeCADGui
import PathScripts.PathGui as PathGui
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathProfile as PathProfile

# Supported features
FeatureSide       = 0x01
FeatureProcessing = 0x02


def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


def debugMsg(msg):
    DEBUG = False
    if DEBUG:
        FreeCAD.Console.PrintMessage(__name__ + ':: ' + msg + '\n')


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    '''Base class for profile operation page controllers. Two sub features are supported:
        FeatureSide       ... Is the Side property exposed in the UI
        FeatureProcessing ... Are the processing check boxes supported by the operation
    '''

    def initPage(self, obj):
        self.setTitle("Profile - " + obj.Label)

    def profileFeatures(self):
        '''profileFeatures() ... return which of the optional profile features are supported.
        Currently two features are supported and returned:
            FeatureSide       ... Is the Side property exposed in the UI
            FeatureProcessing ... Are the processing check boxes supported by the operation
        .'''
        return FeatureSide | FeatureProcessing

    def getForm(self):
        '''getForm() ... returns UI customized according to profileFeatures()'''
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpProfileFullEdit.ui")
        return form

    def getFields(self, obj):
        '''getFields(obj) ... transfers values from UI to obj's proprties'''
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        obj.Side = str(self.form.cutSide.currentText())
        obj.Direction = str(self.form.direction.currentText())

        PathGui.updateInputField(obj, 'OffsetExtra', self.form.extraOffset)
        obj.EnableRotation = str(self.form.enableRotation.currentText())

        obj.UseComp = self.form.useCompensation.isChecked()
        obj.UseStartPoint = self.form.useStartPoint.isChecked()

        obj.processHoles = self.form.processHoles.isChecked()
        obj.processPerimeter = self.form.processPerimeter.isChecked()
        obj.processCircles = self.form.processCircles.isChecked()

    def setFields(self, obj):
        '''setFields(obj) ... transfers obj's property values to UI'''
        self.sync_combobox_with_enumerations()  # Also updates self.propEnums

        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        self.selectInComboBox(obj.Side, self.form.cutSide)
        self.selectInComboBox(obj.Direction, self.form.direction)
        self.form.extraOffset.setText(FreeCAD.Units.Quantity(obj.OffsetExtra.Value, FreeCAD.Units.Length).UserString)
        self.selectInComboBox(obj.EnableRotation, self.form.enableRotation)

        self.form.useCompensation.setChecked(obj.UseComp)
        self.form.useStartPoint.setChecked(obj.UseStartPoint)
        self.form.processHoles.setChecked(obj.processHoles)
        self.form.processPerimeter.setChecked(obj.processPerimeter)
        self.form.processCircles.setChecked(obj.processCircles)

        self.updateVisibility()

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals for updating obj'''
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)
        signals.append(self.form.cutSide.currentIndexChanged)
        signals.append(self.form.direction.currentIndexChanged)
        signals.append(self.form.extraOffset.editingFinished)
        signals.append(self.form.enableRotation.currentIndexChanged)
        signals.append(self.form.useCompensation.stateChanged)
        signals.append(self.form.useStartPoint.stateChanged)
        signals.append(self.form.processHoles.stateChanged)
        signals.append(self.form.processPerimeter.stateChanged)
        signals.append(self.form.processCircles.stateChanged)

        return signals

    def on_Base_Geometry_change(self):
        '''on_Base_Geometry_change()...
        Called with a change made in Base Geometry.
        '''
        debugMsg('on_Base_Geometry_change()')
        self.sync_combobox_with_enumerations()  # located in PathOpGui module
        debugMsg(' -call updateVisibility()')
        self.updateVisibility()

    def setObjectMaps(self):
        # dictionary formats: {'property_name': UI_panel_input_name}
        # visibilityMap is for editor modes
        self.visibilityMap = {
            'processCircles': 'processCircles',
            'processHoles': 'processHoles',
            'processPerimeter': 'processPerimeter',
            'Side': 'cutSide'
        }
        # enumerationMap is for combo boxes
        self.enumerationMap = {}

    def custom_editor_mode_actions(self, modes_dict):
        '''custom_editor_mode_actions(modes_dict) ...
        Custom modifications to editor modes and related UI panel elements,
        and custom actions based on updated editor modes.
        '''
        if modes_dict['Side'] == 2:
            # Reset cutSide to 'Outside' for full model before hiding cutSide input
            self.selectInComboBox('Outside', self.form.cutSide)
            # self.obj.Side = 'Outside'

    def updateVisibility(self):
        self.apply_prop_editor_modes()  # located in PathOpGui module

    def registerSignalHandlers(self, obj):
        self.form.useCompensation.stateChanged.connect(self.updateVisibility)
# Eclass


Command = PathOpGui.SetupOperation('Profile',
        PathProfile.Create,
        TaskPanelOpPage,
        'Path-Contour',
        QtCore.QT_TRANSLATE_NOOP("PathProfile", "Profile"),
        QtCore.QT_TRANSLATE_NOOP("PathProfile", "Profile entire model, selected face(s) or selected edge(s)"),
        PathProfile.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathProfileFacesGui... done\n")
