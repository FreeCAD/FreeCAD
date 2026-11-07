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
"""Provides the TechDraw HoleShaftFit Task Dialog with dual ISO 286 dropdowns."""

__title__ = "TechDrawTools.TaskHoleShaftFit"
__author__ = "edi"
__url__ = "https://www.freecad.org"
__version__ = "00.04"
__date__ = "2026/10/02"

import os
import sys
import re

import FreeCAD as App
import FreeCADGui as Gui

current_dir = os.path.dirname(os.path.realpath(__file__))
if current_dir not in sys.path:
    sys.path.insert(0, current_dir)

from iso286_tables import MEASURE_RANGES, IT_TABLE, DEVIATIONS

translate = App.Qt.translate


class TaskHoleShaftFit:
    def __init__(self, sel):
        self.sel = sel

        self.holeClasses = [
            "- None -", "A11", "B11", "C11", "D10", "E9", "F8", "F7", "G7", "H6", "H7",
            "H8", "H9", "H11", "JS6", "JS7", "K7", "M7", "N7", "P7", "R7", "S7", "T7", "U7"
        ]

        self.shaftClasses = [
            "- None -", "a11", "b11", "c11", "d9", "d10", "e8", "f7", "f8", "g6", "h6",
            "h7", "h9", "h11", "js6", "js7", "k6", "m6", "n6", "p6", "r6", "s6", "t6", "u6", "x6", "z6"
        ]

        # Resolve UI file path
        self._uiPath = os.path.join(current_dir, "Gui", "TaskHoleShaftFit.ui").replace("\\", "/")
        if not os.path.exists(self._uiPath):
            self._uiPath = os.path.join(current_dir, "..", "Gui", "TaskHoleShaftFit.ui").replace("\\", "/")

        self.form = Gui.PySideUic.loadUi(self._uiPath)
        
        if self.form is None:
            App.Console.PrintError(f"TaskHoleShaftFit: CRITICAL ERROR - Could not load UI at {self._uiPath}\n")
            return

        self.form.setWindowTitle(translate("TechDraw_HoleShaftFit", "Hole/Shaft Fit ISO 286"))

        # Populate combo boxes
        self.form.cbHole.addItems(self.holeClasses)
        self.form.cbShaft.addItems(self.shaftClasses)

        # Defaults: H7 / g6
        self.form.cbHole.setCurrentText("H7")
        self.form.cbShaft.setCurrentText("g6")

        self.form.cbHole.currentIndexChanged.connect(self.on_selection_changed)
        self.form.cbShaft.currentIndexChanged.connect(self.on_selection_changed)

        self.on_selection_changed()
        App.ActiveDocument.openTransaction("Add hole or shaft fit")

    def on_selection_changed(self):
        hole = self.form.cbHole.currentText()
        shaft = self.form.cbShaft.currentText()

        if hole != "- None -" and shaft != "- None -":
            if shaft in ["p6", "r6", "s6", "t6", "u6", "x6", "z6"] or hole in ["P7", "R7", "S7", "T7", "U7"]:
                fit_type = translate("TechDraw_HoleShaftFit", "Press fit")
            elif shaft in ["js6", "js7", "k6", "m6", "n6"] or hole in ["JS6", "JS7", "K7", "M7", "N7"]:
                fit_type = translate("TechDraw_HoleShaftFit", "Snug fit")
            else:
                fit_type = translate("TechDraw_HoleShaftFit", "Loose fit")
        elif hole != "- None -" or shaft != "- None -":
            fit_type = translate("TechDraw_HoleShaftFit", "Single tolerance")
        else:
            fit_type = translate("TechDraw_HoleShaftFit", "- None -")

        self.form.lbFitType.setText(fit_type)

    def accept(self):
        if not self.sel or not hasattr(self.sel[0], "Object"):
            Gui.Control.closeDialog()
            App.ActiveDocument.abortTransaction()
            return

        hole = self.form.cbHole.currentText()
        shaft = self.form.cbShaft.currentText()

        if hole == "- None -" and shaft == "- None -":
            Gui.Control.closeDialog()
            App.ActiveDocument.abortTransaction()
            return

        dim = self.sel[0].Object
        value = dim.getRawValue()
        iso = ISO286()

        fit_str = ""
        upper_dev = 0.0
        lower_dev = 0.0
        show_numeric = True

        if hole != "- None -" and shaft != "- None -":
            fit_str = f"{hole}/{shaft}"
            # Do NOT combine numerical tolerances for assembly fit callouts
            show_numeric = False 
        else:
            selected = hole if hole != "- None -" else shaft
            fit_str = selected
            match = re.match(r"([a-zA-Z]+)(\d+)", selected)
            if match:
                iso.calculate(value, match.group(1), int(match.group(2)))
                upper_dev, lower_dev = iso.getValues()

        # Clean prior ISO 286 fit string (e.g. H7, g6, H7/g6) without stripping units (e.g. 'mm')
        mainFormat = re.sub(r"\s+([A-Za-z]{1,2}\d+(/[a-zA-Z]{1,2}\d+)?)$", "", dim.FormatSpec)
        dim.FormatSpec = mainFormat + " " + fit_str
        dim.EqualTolerance = False
        dim.OverTolerance = upper_dev
        dim.UnderTolerance = lower_dev

        if show_numeric:
            dim.FormatSpecOverTolerance = "(+%.3f)" if dim.OverTolerance > 0 else ("(%.3f)" if dim.OverTolerance < 0 else "( %.3f)")
            dim.FormatSpecUnderTolerance = "(+%.3f)" if dim.UnderTolerance > 0 else ("(%.3f)" if dim.UnderTolerance < 0 else "( %.3f)")
        else:
            dim.FormatSpecOverTolerance = ""
            dim.FormatSpecUnderTolerance = ""

        dim.recompute()
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
        nominalRange = self.getNominalRange(value)
        itValue = self.getITValue(quality, nominalRange)

        if fieldChar in ["js", "JS"]:
            halfIT = itValue / 2.0
            self.upperValue = halfIT
            self.lowerValue = -halfIT
            return

        baseDev = self.getFieldValue(fieldChar, nominalRange)

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