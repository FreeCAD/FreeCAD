# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 Russell Johnson (russ4262) <russ4262@gmail.com>    *
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

__title__ = "Path Slot Operation UI"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Slot operation page controller and command implementation."
__contributors__ = ""

# Standard
# Third-party
from PySide import QtCore
# FreeCAD
import FreeCAD
import FreeCADGui
import PathScripts.PathSlot as PathSlot
import PathScripts.PathGui as PathGui
import PathScripts.PathOpGui as PathOpGui


def debugMsg(msg):
    DEBUG = False
    if DEBUG:
        FreeCAD.Console.PrintMessage(__name__ + ':: ' + msg + '\n')


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    '''Page controller class for the Slot operation.'''

    def getForm(self):
        '''getForm() ... returns UI'''
        debugMsg('getForm()')
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpSlotEdit.ui")

    def initPage(self, obj):
        '''initPage(obj) ... Is called after getForm() to initiate the task panel.'''
        debugMsg('initPage()')
        self.setTitle("Slot - " + obj.Label)
        # Requirements due to Gui::QuantitySpinBox class use in UI panel
        self.geo1Extension = PathGui.QuantitySpinBox(self.form.geo1Extension, obj, 'ExtendPathStart')
        self.geo2Extension = PathGui.QuantitySpinBox(self.form.geo2Extension, obj, 'ExtendPathEnd')

    def setFields(self, obj):
        '''setFields(obj) ... transfers obj's property values to UI'''
        debugMsg('setFields()')
        self.sync_combobox_with_enumerations()  # Also updates self.propEnums

        self.updateQuantitySpinBoxes()

        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        idx = self.propEnums['Reference1'].index(obj.Reference1)
        self.form.geo1Reference.setCurrentIndex(idx)

        idx = self.propEnums['Reference2'].index(obj.Reference2)
        self.form.geo2Reference.setCurrentIndex(idx)

        self.selectInComboBox(obj.LayerMode, self.form.layerMode)
        self.selectInComboBox(obj.PathOrientation, self.form.pathOrientation)

        self.form.reverseDirection.setChecked(obj.ReverseDirection)

        self.updateVisibility()

    def updateQuantitySpinBoxes(self):
        self.geo1Extension.updateSpinBox()
        self.geo2Extension.updateSpinBox()

    def getFields(self, obj):
        '''getFields(obj) ... transfers values from UI to obj's proprties'''
        debugMsg('getFields()')
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        obj.Reference1 = str(self.form.geo1Reference.currentText())
        self.geo1Extension.updateProperty()

        obj.Reference2 = str(self.form.geo2Reference.currentText())
        self.geo2Extension.updateProperty()

        obj.LayerMode = str(self.form.layerMode.currentText())

        obj.PathOrientation = str(self.form.pathOrientation.currentText())

        obj.ReverseDirection = self.form.reverseDirection.isChecked()

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals for updating obj'''
        debugMsg('getSignalsForUpdate()')
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)
        signals.append(self.form.geo1Extension.editingFinished)
        signals.append(self.form.geo1Reference.currentIndexChanged)
        signals.append(self.form.geo2Extension.editingFinished)
        signals.append(self.form.geo2Reference.currentIndexChanged)
        signals.append(self.form.layerMode.currentIndexChanged)
        signals.append(self.form.pathOrientation.currentIndexChanged)
        signals.append(self.form.reverseDirection.stateChanged)
        return signals

    def on_Base_Geometry_change(self):
        '''on_Base_Geometry_change()...
        Called with a change made in Base Geometry.
        '''
        debugMsg('on_Base_Geometry_change()')
        self.sync_combobox_with_enumerations()  # located in gui_features module
        debugMsg(' -call updateVisibility()')
        self.updateVisibility()

    def setObjectMaps(self):
        # visibilityMap is for editor modes
        self.visibilityMap = {
            'Reference1': 'geo1Reference',
            'Reference2': 'geo2Reference',
            'ExtendRadius': None,  # Not in UI panel, then `None` value
            'PathOrientation': 'pathOrientation'
        }
        # enumerationMap is for combo boxes
        self.enumerationMap = {
            'Reference1': 'geo1Reference',
            'Reference2': 'geo2Reference'
        }

    def custom_editor_mode_actions(self, modes_dict):
        '''custom_editor_mode_actions(modes_dict) ...
        Custom modifications to editor modes and related UI panel elements,
        and custom actions based on updated editor modes.
        The visibility of UI `customPoints` frame is dependent
        upon use of Base Geometry: `Reference1` and `Reference2`.
        '''
        # Custom modification for Slot UI panel
        if modes_dict['Reference1'] != 2 or modes_dict['Reference2'] != 2:
            modes_dict['CustomPoints'] = 2  # add and set hide flag
            # add entry to visibility map
            self.visibilityMap['CustomPoints'] = 'customPoints'
        elif modes_dict['Reference1'] == 2 or modes_dict['Reference2'] == 2:
            modes_dict['CustomPoints'] = 0  # add and set show flag
            # add entry to visibility map
            self.visibilityMap['CustomPoints'] = 'customPoints'

    def updateVisibility(self):
        '''updateVisibility()... Updates visibility of Tasks panel objects.'''
        # debugMsg('updateVisibility()')
        self.apply_prop_editor_modes()  # located in gui_features module

    def registerSignalHandlers(self, obj):
        # debugMsg('registerSignalHandlers()')
        # self.form.pathOrientation.currentIndexChanged.connect(self.updateVisibility)
        pass
# Eclass


Command = PathOpGui.SetupOperation('Slot',
        PathSlot.Create,
        TaskPanelOpPage,
        'Path-Slot',
        QtCore.QT_TRANSLATE_NOOP("Slot", "Slot"),
        QtCore.QT_TRANSLATE_NOOP("Slot", "Create a Slot operation from selected geometry or custom points."),
        PathSlot.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathSlotGui... done\n")
