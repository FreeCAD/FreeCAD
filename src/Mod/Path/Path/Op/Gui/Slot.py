# -*- coding: utf-8 -*-
# ***************************************************************************
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
import Path.Base.Gui.Util as PathGuiUtil
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Slot as PathSlot
import PathGui

from PySide import QtCore

__title__ = "Path Slot Operation UI"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecad.org"
__doc__ = "Slot operation page controller and command implementation."
__contributors__ = ""


DEBUG = False


def debugMsg(msg):
    global DEBUG
    if DEBUG:
        FreeCAD.Console.PrintMessage("PathSlotGui:: " + msg + "\n")


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for the Slot operation."""

    def getForm(self):
        """getForm() ... returns UI"""
        debugMsg("getForm()")
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpSlotEdit.ui")

    def initPage(self, obj):
        """initPage(obj) ... Is called after getForm() to initiate the task panel."""
        debugMsg("initPage()")
        self.CATS = [None, None]
        self.propEnums = PathSlot.ObjectSlot.propertyEnumerations(dataType="raw")
        self.ENUMS = dict()
        self.setTitle("Slot - " + obj.Label)
        # retrieve property enumerations
        # Requirements due to Gui::QuantitySpinBox class use in UI panel
        self.geo1Extension = PathGuiUtil.QuantitySpinBox(
            self.form.geo1Extension, obj, "ExtendPathStart"
        )
        self.geo2Extension = PathGuiUtil.QuantitySpinBox(
            self.form.geo2Extension, obj, "ExtendPathEnd"
        )

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        debugMsg("setFields()")
        debugMsg("... calling updateVisibility()")
        self.updateVisibility()

        self.updateQuantitySpinBoxes()

        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        enums = [t[1] for t in self.propEnums["Reference1"]]
        if "Reference1" in self.ENUMS:
            enums = self.ENUMS["Reference1"]
        debugMsg(" -enums1: {}".format(enums))
        idx = 0
        if obj.Reference1 in enums:
            idx = enums.index(obj.Reference1)
        self.form.geo1Reference.setCurrentIndex(idx)

        enums = [t[1] for t in self.propEnums["Reference2"]]
        if "Reference2" in self.ENUMS:
            enums = self.ENUMS["Reference2"]
        debugMsg(" -enums2: {}".format(enums))
        idx = 0
        if obj.Reference2 in enums:
            idx = enums.index(obj.Reference2)
        self.form.geo2Reference.setCurrentIndex(idx)

        self.selectInComboBox(obj.LayerMode, self.form.layerMode)
        self.selectInComboBox(obj.PathOrientation, self.form.pathOrientation)

        if obj.ReverseDirection:
            self.form.reverseDirection.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.reverseDirection.setCheckState(QtCore.Qt.Unchecked)

    def updateQuantitySpinBoxes(self):
        self.geo1Extension.updateSpinBox()
        self.geo2Extension.updateSpinBox()

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        debugMsg("getFields()")
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        obj.Reference1 = str(self.form.geo1Reference.currentText())
        self.geo1Extension.updateProperty()

        obj.Reference2 = str(self.form.geo2Reference.currentText())
        self.geo2Extension.updateProperty()

        val = self.propEnums["LayerMode"][self.form.layerMode.currentIndex()][1]
        obj.LayerMode = val

        val = self.propEnums["PathOrientation"][
            self.form.pathOrientation.currentIndex()
        ][1]
        obj.PathOrientation = val

        obj.ReverseDirection = self.form.reverseDirection.isChecked()

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        debugMsg("getSignalsForUpdate()")
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
        """updateVisibility(sentObj=None)... Updates visibility of Tasks panel objects."""
        # debugMsg('updateVisibility()')
        hideFeatures = True
        if hasattr(self.obj, "Base"):
            if self.obj.Base:
                self.form.customPoints.hide()
                self.form.featureReferences.show()
                self.form.pathOrientation_label.show()
                self.form.pathOrientation.show()
                hideFeatures = False
                base, sublist = self.obj.Base[0]
                subCnt = len(sublist)

                if subCnt == 1:
                    debugMsg(" -subCnt == 1")
                    # Save value, then reset choices
                    n1 = sublist[0]
                    # s1 = getattr(base.Shape, n1)
                    # Show Reference1 and customize options within
                    self.form.geo1Reference.show()
                    self.form.geo1Reference_label.show()
                    self.form.geo1Reference_label.setText("Reference:  {}".format(n1))
                    self.customizeReference_1(n1, single=True)
                    # Hide Reference2
                    self.form.geo2Reference.hide()
                    self.form.geo2Reference_label.hide()
                    self.form.geo2Reference_label.setText("End Reference")
                    if self.CATS[1]:
                        self.CATS[1] = None
                elif subCnt == 2:
                    debugMsg(" -subCnt == 2")
                    n1 = sublist[0]
                    n2 = sublist[1]
                    # s1 = getattr(base.Shape, n1)
                    # s2 = getattr(base.Shape, n2)
                    # Show Reference1 and customize options within
                    self.form.geo1Reference.show()
                    self.form.geo1Reference_label.show()
                    self.form.geo1Reference_label.setText(
                        "Start Reference:  {}".format(n1)
                    )
                    self.customizeReference_1(n1)
                    # Show Reference2 and customize options within
                    self.form.geo2Reference.show()
                    self.form.geo2Reference_label.show()
                    self.form.geo2Reference_label.setText(
                        "End Reference:  {}".format(n2)
                    )
                    self.customizeReference_2(n2)
            else:
                self.form.pathOrientation_label.hide()
                self.form.pathOrientation.hide()

        if hideFeatures:
            # reset values
            self.CATS = [None, None]
            self.selectInComboBox("Start to End", self.form.pathOrientation)
            # hide inputs and show message
            self.form.featureReferences.hide()
            self.form.customPoints.show()

    def customizeReference_1(self, sub, single=False):
        debugMsg("customizeReference_1()")
        # Customize Reference1 combobox options
        # by removing unavailable choices
        cat = sub[:4]
        if cat != self.CATS[0]:
            self.CATS[0] = cat
            slot = PathSlot.ObjectSlot
            enums = slot._makeReference1Enumerations(slot, sub, single)
            self.ENUMS["Reference1"] = enums
            debugMsg("Ref1: {}".format(enums))
            rawEnums = slot.propertyEnumerations(dataType="raw")["Reference1"]
            enumTups = [(t, d) for t, d in rawEnums if d in enums]
            self._updateComboBox(self.form.geo1Reference, enumTups)

    def customizeReference_2(self, sub):
        debugMsg("customizeReference_2()")
        # Customize Reference2 combobox options
        # by removing unavailable choices
        cat = sub[:4]
        if cat != self.CATS[1]:
            self.CATS[1] = cat
            slot = PathSlot.ObjectSlot
            enums = slot._makeReference2Enumerations(slot, sub)
            self.ENUMS["Reference2"] = enums
            debugMsg("Ref2: {}".format(enums))
            rawEnums = slot.propertyEnumerations(dataType="raw")["Reference2"]
            enumTups = [(t, d) for t, d in rawEnums if d in enums]
            self._updateComboBox(self.form.geo2Reference, enumTups)

    def registerSignalHandlers(self, obj):
        # debugMsg('registerSignalHandlers()')
        # self.form.pathOrientation.currentIndexChanged.connect(self.updateVisibility)
        pass

    def _updateComboBox(self, cBox, enumTups):
        cBox.blockSignals(True)
        cBox.clear()
        for text, data in enumTups:  #  load enumerations
            cBox.addItem(text, data)
        self.selectInSlotComboBox(data, cBox)
        cBox.blockSignals(False)

    def selectInSlotComboBox(self, name, combo):
        """selectInSlotComboBox(name, combo) ...
        helper function to select a specific value in a combo box."""

        # Search using currentData and return if found
        newindex = combo.findData(name)
        if newindex >= 0:
            combo.setCurrentIndex(newindex)
            return

        # if not found, search using current text
        newindex = combo.findText(name, QtCore.Qt.MatchFixedString)
        if newindex >= 0:
            combo.setCurrentIndex(newindex)
            return

        # not found, return unchanged
        combo.setCurrentIndex(0)
        return


Command = PathOpGui.SetupOperation(
    "Slot",
    PathSlot.Create,
    TaskPanelOpPage,
    "Path_Slot",
    QtCore.QT_TRANSLATE_NOOP("Path_Slot", "Slot"),
    QtCore.QT_TRANSLATE_NOOP(
        "Path_Slot", "Create a Slot operation from selected geometry or custom points."
    ),
    PathSlot.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathSlotGui... done\n")
