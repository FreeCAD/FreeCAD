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

import FreeCAD
import FreeCADGui
import PathScripts.PathSlot as PathSlot
import PathScripts.PathGui as PathGui
import PathScripts.PathOpGui as PathOpGui

from PySide import QtCore

__title__ = "Path Slot Operation UI"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Slot operation page controller and command implementation."
__contributors__ = ""


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    '''Page controller class for the Slot operation.'''

    def initPage(self, obj):
        # pylint: disable=attribute-defined-outside-init
        self.CATS = [None, None]
        self.setTitle("Slot - " + obj.Label)
        # retrieve property enumerations
        self.propEnums = PathSlot.ObjectSlot.opPropertyEnumerations(False)
        # Requirements due to Gui::QuantitySpinBox class use in UI panel
        self.geo1Extension = PathGui.QuantitySpinBox(self.form.geo1Extension, obj, 'ExtendPathStart')
        self.geo2Extension = PathGui.QuantitySpinBox(self.form.geo2Extension, obj, 'ExtendPathEnd')
        # self.updateVisibility()

    def getForm(self):
        '''getForm() ... returns UI'''
        # FreeCAD.Console.PrintMessage('getForm()\n')
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpSlotEdit.ui")

    def updateQuantitySpinBoxes(self):
        # FreeCAD.Console.PrintMessage('updateQuantitySpinBoxes()\n')
        self.geo1Extension.updateSpinBox()
        self.geo2Extension.updateSpinBox()

    def getFields(self, obj):
        '''getFields(obj) ... transfers values from UI to obj's proprties'''
        # FreeCAD.Console.PrintMessage('getFields()\n')
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        obj.Reference1 = str(self.form.geo1Reference.currentText())
        self.geo1Extension.updateProperty()

        obj.Reference2 = str(self.form.geo2Reference.currentText())
        self.geo2Extension.updateProperty()

        val = self.propEnums['LayerMode'][self.form.layerMode.currentIndex()]
        obj.LayerMode = val

        val = self.propEnums['PathOrientation'][self.form.pathOrientation.currentIndex()]
        obj.PathOrientation = val

        if hasattr(self.form, 'reverseDirection'):
            obj.ReverseDirection = self.form.reverseDirection.isChecked()

    def setFields(self, obj):
        '''setFields(obj) ... transfers obj's property values to UI'''
        # FreeCAD.Console.PrintMessage('setFields()\n')
        self.updateQuantitySpinBoxes()

        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        idx = self.propEnums['Reference1'].index(obj.Reference1)
        self.form.geo1Reference.setCurrentIndex(idx)
        idx = self.propEnums['Reference2'].index(obj.Reference2)
        self.form.geo2Reference.setCurrentIndex(idx)

        self.selectInComboBox(obj.LayerMode, self.form.layerMode)
        self.selectInComboBox(obj.PathOrientation, self.form.pathOrientation)

        if obj.ReverseDirection:
            self.form.reverseDirection.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.reverseDirection.setCheckState(QtCore.Qt.Unchecked)

        # FreeCAD.Console.PrintMessage('... calling updateVisibility()\n')
        self.updateVisibility()

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals for updating obj'''
        # FreeCAD.Console.PrintMessage('getSignalsForUpdate()\n')
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

    def updateVisibility(self, sentObj=None):
        '''updateVisibility(sentObj=None)... Updates visibility of Tasks panel objects.'''
        # FreeCAD.Console.PrintMessage('updateVisibility()\n')
        hideFeatures = True
        if hasattr(self.obj, 'Base'):
            if self.obj.Base:
                self.form.customPoints.hide()
                self.form.featureReferences.show()
                self.form.pathOrientation_label.show()
                self.form.pathOrientation.show()
                hideFeatures = False
                base, sublist = self.obj.Base[0]
                subCnt = len(sublist)

                if subCnt == 1:
                    # Save value, then reset choices
                    self.resetRef1Choices()
                    n1 = sublist[0]
                    s1 = getattr(base.Shape, n1)
                    # Show Reference1 and cusomize options within
                    self.form.geo1Reference.show()
                    self.form.geo1Reference_label.show()
                    self.form.geo1Reference_label.setText('Reference:  {}'.format(n1))
                    self.customizeReference_1(n1, single=True)
                    # Hide Reference2
                    self.form.geo2Reference.hide()
                    self.form.geo2Reference_label.hide()
                    self.form.geo2Reference_label.setText('End Reference')
                    if self.CATS[1]:
                        self.CATS[1] = None
                elif subCnt == 2:
                    self.resetRef1Choices()
                    self.resetRef2Choices()
                    n1 = sublist[0]
                    n2 = sublist[1]
                    s1 = getattr(base.Shape, n1)
                    s2 = getattr(base.Shape, n2)
                    # Show Reference1 and cusomize options within
                    self.form.geo1Reference.show()
                    self.form.geo1Reference_label.show()
                    self.form.geo1Reference_label.setText('Start Reference:  {}'.format(n1))
                    self.customizeReference_1(n1)
                    # Show Reference2 and cusomize options within
                    self.form.geo2Reference.show()
                    self.form.geo2Reference_label.show()
                    self.form.geo2Reference_label.setText('End Reference:  {}'.format(n2))
                    self.customizeReference_2(n2)
            else:
                self.form.pathOrientation_label.hide()
                self.form.pathOrientation.hide()
        if hideFeatures:
            self.form.featureReferences.hide()
            self.form.customPoints.show()

    """
    'Reference1': ['Center of Mass', 'Center of BoundBox',
                    'Lowest Point', 'Highest Point', 'Long Edge',
                    'Short Edge', 'Vertex'],
    'Reference2': ['Center of Mass', 'Center of BoundBox',
                    'Lowest Point', 'Highest Point', 'Vertex']
    """

    def customizeReference_1(self, sub, single=False):
        # Customize Reference1 combobox options
        # by removing unavailable choices
        cat = sub[:4]
        if cat != self.CATS[0]:
            self.CATS[0] = cat
            cBox = self.form.geo1Reference
            cBox.blockSignals(True)
            for ri in PathSlot.removeIndexesFromReference_1(sub, single):
                cBox.removeItem(ri)
            cBox.blockSignals(False)

    def customizeReference_2(self, sub):
        # Customize Reference2 combobox options
        # by removing unavailable choices
        cat = sub[:4]
        if cat != self.CATS[1]:
            self.CATS[1] = cat
            cBox = self.form.geo2Reference
            cBox.blockSignals(True)
            for ri in PathSlot.removeIndexesFromReference_2(sub):
                cBox.removeItem(ri)
            cBox.blockSignals(False)
            cBox.setCurrentIndex(0)

    def resetRef1Choices(self):
        # Reset Reference1 choices
        ref1 = self.form.geo1Reference
        ref1.blockSignals(True)
        ref1.clear()  # Empty the combobox
        ref1.addItems(self.propEnums['Reference1'])
        ref1.blockSignals(False)

    def resetRef2Choices(self):
        # Reset Reference2 choices
        ref2 = self.form.geo2Reference
        ref2.blockSignals(True)
        ref2.clear()  # Empty the combobox
        ref2.addItems(self.propEnums['Reference2'])
        ref2.blockSignals(False)

    def registerSignalHandlers(self, obj):
        # FreeCAD.Console.PrintMessage('registerSignalHandlers()\n')
        # self.form.pathOrientation.currentIndexChanged.connect(self.updateVisibility)
        pass


Command = PathOpGui.SetupOperation('Slot',
        PathSlot.Create,
        TaskPanelOpPage,
        'Path-Slot',
        QtCore.QT_TRANSLATE_NOOP("Slot", "Slot"),
        QtCore.QT_TRANSLATE_NOOP("Slot", "Create a Slot operation from selected geometry or custom points."),
        PathSlot.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathSlotGui... done\n")
