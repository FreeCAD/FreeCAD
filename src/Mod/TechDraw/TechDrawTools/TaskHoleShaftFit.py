# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2023 edi <edi271@a1.net>                                *
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
"""Provides the TechDraw HoleShaftFit Task Dialog with full ISO 286 support."""

__title__ = "TechDrawTools.TaskHoleShaftFit"
__author__ = "edi"
__url__ = "https://www.freecad.org"
__version__ = "00.03"
__date__ = "2026/10/02"

import os
import sys
import re
from functools import partial

import FreeCAD as App
import FreeCADGui as Gui

# Force the current directory into Python's path to bypass FreeCAD's relative import blocker
current_dir = os.path.dirname(os.path.realpath(__file__))
if current_dir not in sys.path:
    sys.path.insert(0, current_dir)

from iso286_tables import MEASURE_RANGES, IT_TABLE, DEVIATIONS

translate = App.Qt.translate


class TaskHoleShaftFit:
    def __init__(self, sel):

        loose = translate("TechDraw_HoleShaftFit", "Loose fit")
        snug = translate("TechDraw_HoleShaftFit", "Snug fit")
        press = translate("TechDraw_HoleShaftFit", "Press fit")
        
        self.isHoleBasis = True
        self.sel = sel

        # Expanded ISO 286 fit combinations [Shaft, Hole, Fit Category] for Shaft Basis
        self.holeValues = [
            ["h9", "D10", loose], ["h9", "E9", loose], ["h9", "F8", loose],
            ["h6", "G7", loose], ["c11", "H11", loose], ["d10", "H10", loose],
            ["e8", "H8", loose], ["f7", "H8", loose], ["f8", "H8", loose],
            ["g6", "H7", loose], ["h6", "H7", loose], ["h7", "H8", loose],
            ["h9", "H9", loose], ["h11", "H11", loose],
            ["js6", "H7", snug], ["k6", "H7", snug], ["m6", "H7", snug],
            ["n6", "H7", snug], ["h6", "K7", snug], ["h6", "M7", snug],
            ["h6", "N7", snug],
            ["p6", "H7", press], ["r6", "H7", press], ["s6", "H7", press],
            ["t6", "H7", press], ["u6", "H7", press], ["x6", "H7", press],
            ["z6", "H7", press], ["h6", "P7", press], ["h6", "R7", press],
            ["h6", "S7", press], ["h6", "T7", press], ["h6", "U7", press],
        ]

        # Expanded ISO 286 fit combinations [Hole, Shaft, Fit Category] for Hole Basis
        self.shaftValues = [
            ["H11", "c11", loose], ["H10", "d10", loose], ["H9", "d9", loose],
            ["H8", "e8", loose], ["H8", "f7", loose], ["H8", "f8", loose],
            ["H7", "g6", loose], ["H7", "h6", loose], ["H8", "h7", loose],
            ["H9", "h9", loose], ["H11", "h11", loose], ["D10", "h9", loose],
            ["E9", "h9", loose], ["F8", "h9", loose], ["G7", "h6", loose],
            ["H7", "js6", snug], ["H7", "k6", snug], ["H7", "m6", snug],
            ["H7", "n6", snug], ["K7", "h6", snug], ["M7", "h6", snug],
            ["N7", "h6", snug],
            ["H7", "p6", press], ["H7", "r6", press], ["H7", "s6", press],
            ["H7", "t6", press], ["H7", "u6", press], ["H7", "x6", press],
            ["P7", "h6", press], ["R7", "h6", press], ["S7", "h6", press],
            ["T7", "h6", press], ["U7", "h6", press],
        ]

        # Safely resolve the UI path
        self._uiPath = os.path.join(current_dir, "Gui", "TaskHoleShaftFit.ui").replace("\\", "/")
        ui_macro = os.path.join(current_dir, "TaskHoleShaftFit.ui").replace("\\", "/")
        ui_fallback = os.path.join(App.getHomePath(), "Mod/TechDraw/TechDrawTools/Gui/TaskHoleShaftFit.ui").replace("\\", "/")

        if os.path.exists(self._uiPath):
            pass
        elif os.path.exists(ui_macro):
            self._uiPath = ui_macro
        else:
            App.Console.PrintWarning("TaskHoleShaftFit: Local UI not found, falling back to core path.\n")
            self._uiPath = ui_fallback

        self.form = Gui.PySideUic.loadUi(self._uiPath)
        
        if self.form is None:
            App.Console.PrintError(f"TaskHoleShaftFit: CRITICAL ERROR - Could not load UI file at {self._uiPath}\n")
            return

        self.form.setWindowTitle(translate("TechDraw_HoleShaftFit", "Hole/Shaft Fit ISO 286"))

        self.form.rbHoleBase.clicked.connect(partial(self.on_HoleShaftChanged, True))
        self.form.rbShaftBase.clicked.connect(partial(self.on_HoleShaftChanged, False))
        self.form.cbField.currentIndexChanged.connect(self.on_FieldChanged)

        self.setShaftFields()
        App.ActiveDocument.openTransaction("Add hole or shaft fit")

    def setHoleFields(self):
        """Set shaft-basis fit choices in the combo box."""
        self.form.cbField.blockSignals(True)
        self.form.cbField.clear()
        for value in self.holeValues:
            # Display format: Hole / Shaft (e.g., D10 / h9)
            self.form.cbField.addItem(f"{value[1]} / {value[0]}")
        self.form.cbField.blockSignals(False)
        
        if self.holeValues:
            self.form.lbBaseField.setText("")
            self.form.lbFitType.setText(self.holeValues[0][2])

    def setShaftFields(self):
        """Set hole-basis fit choices in the combo box."""
        self.form.cbField.blockSignals(True)
        self.form.cbField.clear()
        for value in self.shaftValues:
            # Display format: Hole / Shaft (e.g., H7 / g6)
            self.form.cbField.addItem(f"{value[0]} / {value[1]}")
        self.form.cbField.blockSignals(False)

        if self.shaftValues:
            self.form.lbBaseField.setText("")
            self.form.lbFitType.setText(self.shaftValues[0][2])

    def on_HoleShaftChanged(self, isHoleBasis):
        """Slot: Change base fit between hole base and shaft base."""
        self.isHoleBasis = isHoleBasis
        if self.isHoleBasis:
            self.setShaftFields()
        else:
            self.setHoleFields()

    def on_FieldChanged(self):
        """Slot: Handle change of selected tolerance field."""
        currentIndex = self.form.cbField.currentIndex()
        if currentIndex < 0:
            return

        values = self.shaftValues if self.isHoleBasis else self.holeValues
        if currentIndex < len(values):
            self.form.lbBaseField.setText("")
            self.form.lbFitType.setText(values[currentIndex][2])

    def accept(self):
        """Slot: OK button pressed."""
        currentIndex = self.form.cbField.currentIndex()
        if currentIndex < 0:
            return

        selectedField = self.shaftValues[currentIndex][1] if self.isHoleBasis else self.holeValues[currentIndex][1]
        baseField = self.shaftValues[currentIndex][0] if self.isHoleBasis else self.holeValues[currentIndex][0]

        fitString = f"{baseField}/{selectedField}" if self.isHoleBasis else f"{selectedField}/{baseField}"

        match = re.match(r"([a-zA-Z]+)(\d+)", selectedField)
        if not match:
            App.Console.PrintError(f"TaskHoleShaftFit: Could not parse tolerance field '{selectedField}'\n")
            App.ActiveDocument.abortTransaction()
            Gui.Control.closeDialog()
            return

        fieldChar, quality_str = match.groups()
        quality = int(quality_str)

        dim = self.sel[0].Object
        value = dim.getRawValue()

        iso = ISO286()
        iso.calculate(value, fieldChar, quality)
        rangeValues = iso.getValues()

        mainFormat = dim.FormatSpec
        dim.FormatSpec = mainFormat + " " + fitString
        dim.EqualTolerance = False
        dim.OverTolerance = rangeValues[0]
        dim.UnderTolerance = rangeValues[1]

        if dim.OverTolerance < 0:
            dim.FormatSpecOverTolerance = "(%-0.6w)"
        elif dim.OverTolerance > 0:
            dim.FormatSpecOverTolerance = "(+%-0.6w)"
        else:
            dim.FormatSpecOverTolerance = "( %-0.6w)"

        if dim.UnderTolerance < 0:
            dim.FormatSpecUnderTolerance = "(%-0.6w)"
        elif dim.UnderTolerance > 0:
            dim.FormatSpecUnderTolerance = "(+%-0.6w)"
        else:
            dim.FormatSpecUnderTolerance = "( %-0.6w)"

        Gui.Control.closeDialog()
        App.ActiveDocument.commitTransaction()

    def reject(self):
        App.ActiveDocument.abortTransaction()
        return True


class ISO286:
    """Calculates ISO 286 tolerance deviations and IT grades up to 500mm."""

    def __init__(self):
        self.upperValue = 0.0
        self.lowerValue = 0.0
        self.nominalRange = 0

    def getNominalRange(self, measureValue):
        index = 1
        while index < len(MEASURE_RANGES) and measureValue > MEASURE_RANGES[index]:
            index += 1
        return max(0, index - 1)

    def getITValue(self, quality, nominalRangeIndex):
        if quality in IT_TABLE:
            return IT_TABLE[quality][nominalRangeIndex]
        return IT_TABLE[7][nominalRangeIndex]

    def getFieldValue(self, fieldChar, nominalRangeIndex):
        if fieldChar in DEVIATIONS:
            return DEVIATIONS[fieldChar][nominalRangeIndex]
        return 0

    def calculate(self, value, fieldChar, quality):
        self.nominalRange = self.getNominalRange(value)
        itValue = self.getITValue(quality, self.nominalRange)

        if fieldChar in ["js", "JS"]:
            halfIT = itValue // 2
            self.upperValue = halfIT
            self.lowerValue = -halfIT
            return

        baseDev = self.getFieldValue(fieldChar, self.nominalRange)

        if fieldChar.islower():
            if fieldChar <= "h":
                self.upperValue = baseDev
                self.lowerValue = self.upperValue - itValue
            else:
                self.lowerValue = baseDev
                self.upperValue = self.lowerValue + itValue
        else:
            if fieldChar <= "H":
                self.lowerValue = baseDev
                self.upperValue = self.lowerValue + itValue
            else:
                self.upperValue = baseDev
                self.lowerValue = self.upperValue - itValue

    def getValues(self):
        return (self.upperValue / 1000.0, self.lowerValue / 1000.0)