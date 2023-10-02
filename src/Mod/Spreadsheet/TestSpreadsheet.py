# ***************************************************************************
# *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
# *   Copyright (c) 2016 Eivind Kvedalen <eivind@kvedalen.name>             *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU General Public License (GPL)            *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# ***************************************************************************/

import os
import sys
import math
from math import sqrt
import unittest
import FreeCAD
import Part
import Sketcher
import tempfile
from FreeCAD import Base
from FreeCAD import Units

v = Base.Vector

# ----------------------------------------------------------------------------------
# define the functions to test the FreeCAD Spreadsheet module and expression engine
# ----------------------------------------------------------------------------------


class SpreadsheetCases(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument()
        self.TempPath = tempfile.gettempdir()
        FreeCAD.Console.PrintLog("  Using temp path: " + self.TempPath + "\n")

    def testAggregates(self):
        """Test all aggregate functions"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("B13", "4")
        sheet.set("B14", "5")
        sheet.set("B15", "6")
        sheet.set("C13", "4mm")
        sheet.set("C14", "5mm")
        sheet.set("C15", "6mm")
        sheet.set("C16", "6")

        sheet.set("A1", "=sum(1)")
        sheet.set("A2", "=sum(1;2)")
        sheet.set("A3", "=sum(1;2;3)")
        sheet.set("A4", "=sum(1;2;3;B13)")
        sheet.set("A5", "=sum(1;2;3;B13:B15)")

        sheet.set("B1", "=min(1)")
        sheet.set("B2", "=min(1;2)")
        sheet.set("B3", "=min(1;2;3)")
        sheet.set("B4", "=min(1;2;3;B13)")
        sheet.set("B5", "=min(1;2;3;B13:B15)")

        sheet.set("C1", "=max(1)")
        sheet.set("C2", "=max(1;2)")
        sheet.set("C3", "=max(1;2;3)")
        sheet.set("C4", "=max(1;2;3;B13)")
        sheet.set("C5", "=max(1;2;3;B13:B15)")

        sheet.set("D1", "=stddev(1)")
        sheet.set("D2", "=stddev(1;2)")
        sheet.set("D3", "=stddev(1;2;3)")
        sheet.set("D4", "=stddev(1;2;3;B13)")
        sheet.set("D5", "=stddev(1;2;3;B13:B15)")

        sheet.set("E1", "=count(1)")
        sheet.set("E2", "=count(1;2)")
        sheet.set("E3", "=count(1;2;3)")
        sheet.set("E4", "=count(1;2;3;B13)")
        sheet.set("E5", "=count(1;2;3;B13:B15)")

        sheet.set("F1", "=average(1)")
        sheet.set("F2", "=average(1;2)")
        sheet.set("F3", "=average(1;2;3)")
        sheet.set("F4", "=average(1;2;3;B13)")
        sheet.set("F5", "=average(1;2;3;B13:B15)")

        sheet.set("G1", "=average(C13:C15)")
        sheet.set("G2", "=min(C13:C15)")
        sheet.set("G3", "=max(C13:C15)")
        sheet.set("G4", "=count(C13:C15)")
        sheet.set("G5", "=stddev(C13:C15)")
        sheet.set("G6", "=sum(C13:C15)")

        sheet.set("H1", "=average(C13:C16)")
        sheet.set("H2", "=min(C13:C16)")
        sheet.set("H3", "=max(C13:C16)")
        sheet.set("H4", "=count(C13:C16)")
        sheet.set("H5", "=stddev(C13:C16)")
        sheet.set("H6", "=sum(C13:C16)")

        self.doc.recompute()
        self.assertEqual(sheet.A1, 1)
        self.assertEqual(sheet.A2, 3)
        self.assertEqual(sheet.A3, 6)
        self.assertEqual(sheet.A4, 10)
        self.assertEqual(sheet.A5, 21)

        self.assertEqual(sheet.B1, 1)
        self.assertEqual(sheet.B2, 1)
        self.assertEqual(sheet.B3, 1)
        self.assertEqual(sheet.B4, 1)
        self.assertEqual(sheet.B5, 1)

        self.assertEqual(sheet.C1, 1)
        self.assertEqual(sheet.C2, 2)
        self.assertEqual(sheet.C3, 3)
        self.assertEqual(sheet.C4, 4)
        self.assertEqual(sheet.C5, 6)

        self.assertTrue(
            sheet.D1.startswith("ERR: Invalid number of entries: at least two required.")
        )
        self.assertEqual(sheet.D2, 0.7071067811865476)
        self.assertEqual(sheet.D3, 1.0)
        self.assertEqual(sheet.D4, 1.2909944487358056)
        self.assertEqual(sheet.D5, 1.8708286933869707)

        self.assertEqual(sheet.E1, 1)
        self.assertEqual(sheet.E2, 2)
        self.assertEqual(sheet.E3, 3)
        self.assertEqual(sheet.E4, 4)
        self.assertEqual(sheet.E5, 6)

        self.assertEqual(sheet.F1, 1)
        self.assertEqual(sheet.F2, (1.0 + 2.0) / 2.0)
        self.assertEqual(sheet.F3, (1.0 + 2 + 3) / 3)
        self.assertEqual(sheet.F4, (1.0 + 2 + 3 + 4) / 4)
        self.assertEqual(sheet.F5, (1.0 + 2 + 3 + 4 + 5 + 6) / 6)

        self.assertEqual(sheet.G1, Units.Quantity("5 mm"))
        self.assertEqual(sheet.G2, Units.Quantity("4 mm"))
        self.assertEqual(sheet.G3, Units.Quantity("6 mm"))
        self.assertEqual(sheet.G4, 3)
        self.assertEqual(sheet.G5, Units.Quantity("1 mm"))
        self.assertEqual(sheet.G6, Units.Quantity("15 mm"))

        self.assertTrue(
            sheet.H1.startswith("ERR: Quantity::operator +=(): Unit mismatch in plus operation")
        )
        self.assertTrue(
            sheet.H2.startswith(
                "ERR: Quantity::operator <(): quantities need to have same unit to compare"
            )
        )
        self.assertTrue(
            sheet.H3.startswith(
                "ERR: Quantity::operator >(): quantities need to have same unit to compare"
            )
        )
        self.assertEqual(sheet.H4, 4)
        self.assertTrue(
            sheet.H5.startswith("ERR: Quantity::operator -(): Unit mismatch in minus operation")
        )
        self.assertTrue(
            sheet.H6.startswith("ERR: Quantity::operator +=(): Unit mismatch in plus operation")
        )

    def assertMostlyEqual(self, a, b):
        if type(a) is Units.Quantity:
            self.assertTrue(math.fabs(a.Value - b.Value) < 1e-14)
            self.assertTrue(a.Unit == b.Unit)
        else:
            self.assertTrue(math.fabs(a - b) < 1e-14)

    def testFunctions(self):
        """Test all built-in simple functions"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A1", "=cos(60)")  # Cos
        sheet.set("B1", "=cos(60deg)")
        sheet.set("C1", "=cos(pi / 2 * 1rad)")
        sheet.set("A2", "=sin(30)")  # Sin
        sheet.set("B2", "=sin(30deg)")
        sheet.set("C2", "=sin(pi / 6 * 1rad)")
        sheet.set("A3", "=tan(45)")  # Tan
        sheet.set("B3", "=tan(45deg)")
        sheet.set("C3", "=tan(pi / 4 * 1rad)")
        sheet.set("A4", "=abs(3)")  # Abs
        sheet.set("B4", "=abs(-3)")
        sheet.set("C4", "=abs(-3mm)")
        sheet.set("A5", "=exp(3)")  # Exp
        sheet.set("B5", "=exp(-3)")
        sheet.set("C5", "=exp(-3mm)")
        sheet.set("A6", "=log(3)")  # Log
        sheet.set("B6", "=log(-3)")
        sheet.set("C6", "=log(-3mm)")
        sheet.set("A7", "=log10(10)")  # Log10
        sheet.set("B7", "=log10(-3)")
        sheet.set("C7", "=log10(-3mm)")
        sheet.set("A8", "=round(3.4)")  # Round
        sheet.set("B8", "=round(3.6)")
        sheet.set("C8", "=round(-3.4)")
        sheet.set("D8", "=round(-3.6)")
        sheet.set("E8", "=round(3.4mm)")
        sheet.set("F8", "=round(3.6mm)")
        sheet.set("G8", "=round(-3.4mm)")
        sheet.set("H8", "=round(-3.6mm)")
        sheet.set("A9", "=trunc(3.4)")  # Trunc
        sheet.set("B9", "=trunc(3.6)")
        sheet.set("C9", "=trunc(-3.4)")
        sheet.set("D9", "=trunc(-3.6)")
        sheet.set("E9", "=trunc(3.4mm)")
        sheet.set("F9", "=trunc(3.6mm)")
        sheet.set("G9", "=trunc(-3.4mm)")
        sheet.set("H9", "=trunc(-3.6mm)")
        sheet.set("A10", "=ceil(3.4)")  # Ceil
        sheet.set("B10", "=ceil(3.6)")
        sheet.set("C10", "=ceil(-3.4)")
        sheet.set("D10", "=ceil(-3.6)")
        sheet.set("E10", "=ceil(3.4mm)")
        sheet.set("F10", "=ceil(3.6mm)")
        sheet.set("G10", "=ceil(-3.4mm)")
        sheet.set("H10", "=ceil(-3.6mm)")
        sheet.set("A11", "=floor(3.4)")  # Floor
        sheet.set("B11", "=floor(3.6)")
        sheet.set("C11", "=floor(-3.4)")
        sheet.set("D11", "=floor(-3.6)")
        sheet.set("E11", "=floor(3.4mm)")
        sheet.set("F11", "=floor(3.6mm)")
        sheet.set("G11", "=floor(-3.4mm)")
        sheet.set("H11", "=floor(-3.6mm)")
        sheet.set("A12", "=asin(0.5)")  # Asin
        sheet.set("B12", "=asin(0.5mm)")
        sheet.set("A13", "=acos(0.5)")  # Acos
        sheet.set("B13", "=acos(0.5mm)")
        sheet.set("A14", "=atan(sqrt(3))")  # Atan
        sheet.set("B14", "=atan(0.5mm)")
        sheet.set("A15", "=sinh(0.5)")  # Sinh
        sheet.set("B15", "=sinh(0.5mm)")
        sheet.set("A16", "=cosh(0.5)")  # Cosh
        sheet.set("B16", "=cosh(0.5mm)")
        sheet.set("A17", "=tanh(0.5)")  # Tanh
        sheet.set("B17", "=tanh(0.5mm)")
        sheet.set("A18", "=sqrt(4)")  # Sqrt
        sheet.set("B18", "=sqrt(4mm^2)")
        sheet.set("A19", "=mod(7; 4)")  # Mod
        sheet.set("B19", "=mod(-7; 4)")
        sheet.set("C19", "=mod(7mm; 4)")
        sheet.set("D19", "=mod(7mm; 4mm)")
        sheet.set("A20", "=atan2(3; 3)")  # Atan2
        sheet.set("B20", "=atan2(-3; 3)")
        sheet.set("C20", "=atan2(3mm; 3)")
        sheet.set("D20", "=atan2(3mm; 3mm)")
        sheet.set("A21", "=pow(7; 4)")  # Pow
        sheet.set("B21", "=pow(-7; 4)")
        sheet.set("C21", "=pow(7mm; 4)")
        sheet.set("D21", "=pow(7mm; 4mm)")
        sheet.set("A23", "=hypot(3; 4)")  # Hypot
        sheet.set("B23", "=hypot(-3; 4)")
        sheet.set("C23", "=hypot(3mm; 4)")
        sheet.set("D23", "=hypot(3mm; 4mm)")
        sheet.set("A24", "=hypot(3; 4; 5)")  # Hypot
        sheet.set("B24", "=hypot(-3; 4; 5)")
        sheet.set("C24", "=hypot(3mm; 4; 5)")
        sheet.set("D24", "=hypot(3mm; 4mm; 5mm)")
        sheet.set("A26", "=cath(5; 3)")  # Cath
        sheet.set("B26", "=cath(-5; 3)")
        sheet.set("C26", "=cath(5mm; 3)")
        sheet.set("D26", "=cath(5mm; 3mm)")

        l = math.sqrt(5 * 5 + 4 * 4 + 3 * 3)
        sheet.set("A27", "=cath(%0.15f; 5; 4)" % l)  # Cath
        sheet.set("B27", "=cath(%0.15f; -5; 4)" % l)
        sheet.set("C27", "=cath(%0.15f mm; 5mm; 4)" % l)
        sheet.set("D27", "=cath(%0.15f mm; 5mm; 4mm)" % l)

        self.doc.recompute()
        self.assertMostlyEqual(sheet.A1, 0.5)  # Cos
        self.assertMostlyEqual(sheet.B1, 0.5)
        self.assertMostlyEqual(sheet.C1, 0)
        self.assertMostlyEqual(sheet.A2, 0.5)  # Sin
        self.assertMostlyEqual(sheet.B2, 0.5)
        self.assertMostlyEqual(sheet.C2, 0.5)
        self.assertMostlyEqual(sheet.A3, 1)  # Tan
        self.assertMostlyEqual(sheet.B3, 1)
        self.assertMostlyEqual(sheet.C3, 1)
        self.assertMostlyEqual(sheet.A4, 3)  # Abs
        self.assertMostlyEqual(sheet.B4, 3)
        self.assertMostlyEqual(sheet.C4, Units.Quantity("3 mm"))
        self.assertMostlyEqual(sheet.A5, math.exp(3))  # Exp
        self.assertMostlyEqual(sheet.B5, math.exp(-3))
        self.assertTrue(sheet.C5.startswith("ERR: Unit must be empty."))
        self.assertMostlyEqual(sheet.A6, math.log(3))  # Log
        self.assertTrue(math.isnan(sheet.B6))
        self.assertTrue(sheet.C6.startswith("ERR: Unit must be empty."))
        self.assertMostlyEqual(sheet.A7, math.log10(10))  # Log10
        self.assertTrue(math.isnan(sheet.B7))
        self.assertTrue(sheet.C7.startswith("ERR: Unit must be empty."))
        self.assertMostlyEqual(sheet.A8, 3)  # Round
        self.assertMostlyEqual(sheet.B8, 4)
        self.assertMostlyEqual(sheet.C8, -3)
        self.assertMostlyEqual(sheet.D8, -4)
        self.assertEqual(sheet.E8, Units.Quantity("3 mm"))
        self.assertEqual(sheet.F8, Units.Quantity("4 mm"))
        self.assertEqual(sheet.G8, Units.Quantity("-3 mm"))
        self.assertEqual(sheet.H8, Units.Quantity("-4 mm"))
        self.assertMostlyEqual(sheet.A9, 3)  # Trunc
        self.assertMostlyEqual(sheet.B9, 3)
        self.assertMostlyEqual(sheet.C9, -3)
        self.assertMostlyEqual(sheet.D9, -3)
        self.assertEqual(sheet.E9, Units.Quantity("3 mm"))
        self.assertEqual(sheet.F9, Units.Quantity("3 mm"))
        self.assertEqual(sheet.G9, Units.Quantity("-3 mm"))
        self.assertEqual(sheet.H9, Units.Quantity("-3 mm"))
        self.assertMostlyEqual(sheet.A10, 4)  # Ceil
        self.assertMostlyEqual(sheet.B10, 4)
        self.assertMostlyEqual(sheet.C10, -3)
        self.assertMostlyEqual(sheet.D10, -3)
        self.assertMostlyEqual(sheet.E10, Units.Quantity("4 mm"))
        self.assertMostlyEqual(sheet.F10, Units.Quantity("4 mm"))
        self.assertMostlyEqual(sheet.G10, Units.Quantity("-3 mm"))
        self.assertMostlyEqual(sheet.H10, Units.Quantity("-3 mm"))
        self.assertMostlyEqual(sheet.A11, 3)  # Floor
        self.assertMostlyEqual(sheet.B11, 3)
        self.assertMostlyEqual(sheet.C11, -4)
        self.assertMostlyEqual(sheet.D11, -4)
        self.assertMostlyEqual(sheet.E11, Units.Quantity("3 mm"))
        self.assertMostlyEqual(sheet.F11, Units.Quantity("3 mm"))
        self.assertMostlyEqual(sheet.G11, Units.Quantity("-4 mm"))
        self.assertMostlyEqual(sheet.H11, Units.Quantity("-4 mm"))
        self.assertMostlyEqual(sheet.A12, Units.Quantity("30 deg"))  # Asin
        self.assertTrue(sheet.B12.startswith("ERR: Unit must be empty."))
        self.assertMostlyEqual(sheet.A13, Units.Quantity("60 deg"))  # Acos
        self.assertTrue(sheet.B13.startswith("ERR: Unit must be empty."))
        self.assertMostlyEqual(sheet.A14, Units.Quantity("60 deg"))  # Atan
        self.assertTrue(sheet.B14.startswith("ERR: Unit must be empty."))
        self.assertMostlyEqual(sheet.A15, math.sinh(0.5))  # Sinh
        self.assertTrue(sheet.B15.startswith("ERR: Unit must be empty."))
        self.assertMostlyEqual(sheet.A16, math.cosh(0.5))  # Cosh
        self.assertTrue(sheet.B16.startswith("ERR: Unit must be empty."))
        self.assertMostlyEqual(sheet.A17, math.tanh(0.5))  # Tanh
        self.assertTrue(sheet.B17.startswith("ERR: Unit must be empty."))
        self.assertMostlyEqual(sheet.A18, 2)  # Sqrt
        self.assertMostlyEqual(sheet.B18, Units.Quantity("2 mm"))
        self.assertMostlyEqual(sheet.A19, 3)  # Mod
        self.assertMostlyEqual(sheet.B19, -3)
        self.assertMostlyEqual(sheet.C19, Units.Quantity("3 mm"))
        self.assertEqual(sheet.D19, 3)
        self.assertMostlyEqual(sheet.A20, Units.Quantity("45 deg"))  # Atan2
        self.assertMostlyEqual(sheet.B20, Units.Quantity("-45 deg"))
        self.assertTrue(sheet.C20.startswith("ERR: Units must be equal"))
        self.assertMostlyEqual(sheet.D20, Units.Quantity("45 deg"))
        self.assertMostlyEqual(sheet.A21, 2401)  # Pow
        self.assertMostlyEqual(sheet.B21, 2401)
        self.assertMostlyEqual(sheet.C21, Units.Quantity("2401mm^4"))
        self.assertTrue(sheet.D21.startswith("ERR: Exponent is not allowed to have a unit."))
        self.assertMostlyEqual(sheet.A23, 5)  # Hypot
        self.assertMostlyEqual(sheet.B23, 5)
        self.assertTrue(sheet.C23.startswith("ERR: Units must be equal"))
        self.assertMostlyEqual(sheet.D23, Units.Quantity("5mm"))

        l = math.sqrt(3 * 3 + 4 * 4 + 5 * 5)
        self.assertMostlyEqual(sheet.A24, l)  # Hypot
        self.assertMostlyEqual(sheet.B24, l)
        self.assertTrue(sheet.C24.startswith("ERR: Units must be equal"))
        self.assertMostlyEqual(sheet.D24, Units.Quantity("7.07106781186548 mm"))
        self.assertMostlyEqual(sheet.A26, 4)  # Cath
        self.assertMostlyEqual(sheet.B26, 4)
        self.assertTrue(sheet.C26.startswith("ERR: Units must be equal"))
        self.assertMostlyEqual(sheet.D26, Units.Quantity("4mm"))

        l = math.sqrt(5 * 5 + 4 * 4 + 3 * 3)
        l = math.sqrt(l * l - 5 * 5 - 4 * 4)
        self.assertMostlyEqual(sheet.A27, l)  # Cath
        self.assertMostlyEqual(sheet.B27, l)
        self.assertTrue(sheet.C27.startswith("ERR: Units must be equal"))
        self.assertMostlyEqual(sheet.D27, Units.Quantity("3 mm"))

    def testRelationalOperators(self):
        """Test relational operators"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        # All should be 1 as result
        sheet.set("A1", "=1 == 1     ? 1 : 0")
        sheet.set("A2", "=1 != 1     ? 0 : 1")
        sheet.set("A3", "=1e9 == 1e9 ? 1 : 0")
        sheet.set("A4", "=1e9 != 1e9 ? 0 : 1")
        sheet.set("A5", "=1 > 1      ? 0 : 1")
        sheet.set("A6", "=2 > 1      ? 1 : 0")
        sheet.set("A7", "=1 > 2      ? 0 : 1")
        sheet.set("A8", "=1 < 1      ? 0 : 1")
        sheet.set("A9", "=1 < 2      ? 1 : 0")
        sheet.set("A10", "=2 < 1      ? 0 : 1")
        sheet.set("A11", "=1 >= 1     ? 1 : 0")
        sheet.set("A12", "=2 >= 1     ? 1 : 0")
        sheet.set("A13", "=1 >= 2     ? 0 : 1")
        sheet.set("A14", "=1 <= 1     ? 1 : 1")
        sheet.set("A15", "=1 <= 2     ? 1 : 0")
        sheet.set("A16", "=2 <= 1     ? 0 : 1")
        sheet.set("A17", "=1 >= 1.000000000000001 ? 0 : 1")
        sheet.set("A18", "=1 >= 1.0000000000000001 ? 1 : 0")
        sheet.set("A19", "=1 <= 1.000000000000001 ? 1 : 0")
        sheet.set("A20", "=1 <= 1.0000000000000001 ? 1 : 0")
        sheet.set("A21", "=1 == 1.000000000000001 ? 0 : 1")
        sheet.set("A22", "=1 == 1.0000000000000001 ? 1 : 0")
        sheet.set("A23", "=1 != 1.000000000000001 ? 1 : 0")
        sheet.set("A24", "=1 != 1.0000000000000001 ? 0 : 1")

        self.doc.recompute()
        self.assertEqual(sheet.A1, 1)
        self.assertEqual(sheet.A2, 1)
        self.assertEqual(sheet.A3, 1)
        self.assertEqual(sheet.A4, 1)
        self.assertEqual(sheet.A5, 1)
        self.assertEqual(sheet.A6, 1)
        self.assertEqual(sheet.A7, 1)
        self.assertEqual(sheet.A8, 1)
        self.assertEqual(sheet.A9, 1)
        self.assertEqual(sheet.A10, 1)
        self.assertEqual(sheet.A11, 1)
        self.assertEqual(sheet.A12, 1)
        self.assertEqual(sheet.A13, 1)
        self.assertEqual(sheet.A14, 1)
        self.assertEqual(sheet.A15, 1)
        self.assertEqual(sheet.A16, 1)
        self.assertEqual(sheet.A17, 1)
        self.assertEqual(sheet.A18, 1)
        self.assertEqual(sheet.A19, 1)
        self.assertEqual(sheet.A20, 1)
        self.assertEqual(sheet.A21, 1)
        self.assertEqual(sheet.A22, 1)
        self.assertEqual(sheet.A23, 1)
        self.assertEqual(sheet.A24, 1)

    def testUnits(self):
        """Units -- test unit calculations."""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A1", "=2mm + 3mm")
        sheet.set("A2", "=2mm - 3mm")
        sheet.set("A3", "=2mm * 3mm")
        sheet.set("A4", "=4mm / 2mm")
        sheet.set("A5", "=(4mm)^2")
        sheet.set("A6", "=5(mm^2)")
        sheet.set("A7", "=5mm^2")  #  ^2 operates on whole number
        sheet.set("A8", "=5")
        sheet.set("A9", "=5*1/K")  # Currently fails
        sheet.set("A10", "=5 K^-1")  # Currently fails
        sheet.set("A11", "=9.8 m/s^2")  # Currently fails
        sheet.setDisplayUnit("A8", "1/K")
        self.doc.recompute()
        self.assertEqual(sheet.A1, Units.Quantity("5mm"))
        self.assertEqual(sheet.A2, Units.Quantity("-1 mm"))
        self.assertEqual(sheet.A3, Units.Quantity("6 mm^2"))
        self.assertEqual(sheet.A4, Units.Quantity("2"))
        self.assertEqual(sheet.A5, Units.Quantity("16 mm^2"))
        self.assertEqual(sheet.A6, Units.Quantity("5 mm^2"))
        self.assertEqual(sheet.A7, Units.Quantity("5 mm^2"))
        self.assertEqual(sheet.A8, Units.Quantity("5"))
        self.assertEqual(sheet.A9, Units.Quantity("5 K^-1"))
        self.assertEqual(sheet.A10, Units.Quantity("5 K^-1"))
        self.assertEqual(sheet.A11, Units.Quantity("9.8 m/s^2"))

    def testPrecedence(self):
        """Precedence -- test precedence for relational operators and conditional operator."""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A1", "=1 < 2 ? 3 : 4")
        sheet.set("A2", "=1 + 2 < 3 + 4 ? 5 + 6 : 7 + 8")
        sheet.set("A3", "=1 + 2 * 1 < 3 + 4 ? 5 * 2 + 6 * 3 + 2 ^ 4 : 7 * 2 + 8 * 3 + 2 ^ 3")
        sheet.set("A4", "=123")
        sheet.set("A5", "=123 + 321")
        sheet.set("A6", "=123 * 2 + 321")
        sheet.set("A7", "=123 * 2 + 333 / 3")
        sheet.set("A8", "=123 * (2 + 321)")
        sheet.set("A9", "=3 ^ 4")
        sheet.set("A10", "=3 ^ 4 * 2")
        sheet.set("A11", "=3 ^ (4 * 2)")
        sheet.set("A12", "=3 ^ 4 + 4")
        sheet.set("A13", "=1 + 4 / 2 + 5")
        sheet.set("A14", "=(3 + 6) / (1 + 2)")
        sheet.set("A15", "=1 * 2 / 3 * 4")
        sheet.set("A16", "=(1 * 2) / (3 * 4)")
        # Test associativity
        sheet.set(
            "A17", "=3 ^ 4 ^ 2"
        )  # exponentiation is left-associative; to follow excel, openoffice, matlab, octave
        sheet.set("A18", "=3 ^ (4 ^ 2)")  # exponentiation is left-associative
        sheet.set("A19", "=(3 ^ 4) ^ 2")  # exponentiation is left-associative
        sheet.set("A20", "=3 + 4 + 2")
        sheet.set("A21", "=3 + (4 + 2)")
        sheet.set("A22", "=(3 + 4) + 2")
        sheet.set("A23", "=3 - 4 - 2")
        sheet.set("A24", "=3 - (4 - 2)")
        sheet.set("A25", "=(3 - 4) - 2")
        sheet.set("A26", "=3 * 4 * 2")
        sheet.set("A27", "=3 * (4 * 2)")
        sheet.set("A28", "=(3 * 4) * 2")
        sheet.set("A29", "=3 / 4 / 2")
        sheet.set("A30", "=3 / (4 / 2)")
        sheet.set("A31", "=(3 / 4) / 2")
        sheet.set("A32", "=pi * 3")
        sheet.set("A33", "=A32 / 3")
        sheet.set("A34", "=1 < 2 ? <<A>> : <<B>>")
        sheet.set("A35", "=min(A32:A33)")
        sheet.set("A36", "=(1 < 2 ? 0 : 1) * 3")
        sheet.set("A37", "=8/(2^2*2)")
        sheet.set("A38", "=(2^2*2)/8")
        sheet.set("A39", "=2^(2*2)/8")
        sheet.set("A40", "=8/2^(2*2)")
        sheet.set("A41", "=-1")
        sheet.set("A42", "=-(1)")
        sheet.set("A43", "=-(1 + 1)")
        sheet.set("A44", "=-(1 - 1)")
        sheet.set("A45", "=-(-1 + 1)")
        sheet.set("A46", "=-(-1 + -1)")
        sheet.set("A47", "=+1")
        sheet.set("A48", "=+(1)")
        sheet.set("A49", "=+(1 + 1)")
        sheet.set("A50", "=+(1 - 1)")
        sheet.set("A51", "=+(-1 + 1)")
        sheet.set("A52", "=+(-1 + -1)")

        self.doc.addObject("Part::Cylinder", "Cylinder")
        # We cannot use Thickness, as this feature requires a source shape,
        # otherwise it will cause recomputation failure. The new logic of
        # App::Document will not continue recompute any dependent objects

        #  self.doc.addObject("Part::Thickness", "Pipe")
        self.doc.addObject("Part::Box", "Box")
        self.doc.Box.Length = 1

        sheet.set("B1", "101")
        sheet.set("A53", "=-(-(B1-1)/2)")
        sheet.set("A54", '=-(Cylinder.Radius + Box.Length - 1"/2)')

        self.doc.recompute()
        self.assertEqual(sheet.getContents("A1"), "=1 < 2 ? 3 : 4")
        self.assertEqual(sheet.getContents("A2"), "=1 + 2 < 3 + 4 ? 5 + 6 : 7 + 8")
        self.assertEqual(
            sheet.getContents("A3"),
            "=1 + 2 * 1 < 3 + 4 ? 5 * 2 + 6 * 3 + 2 ^ 4 : 7 * 2 + 8 * 3 + 2 ^ 3",
        )
        self.assertEqual(sheet.A1, 3)
        self.assertEqual(sheet.A2, 11)
        self.assertEqual(sheet.A3, 44)
        self.assertEqual(sheet.A4, 123)
        self.assertEqual(sheet.A5, 444)
        self.assertEqual(sheet.A6, 567)
        self.assertEqual(sheet.A7, 357)
        self.assertEqual(sheet.A8, 39729)
        self.assertEqual(sheet.A9, 81)
        self.assertEqual(sheet.A10, 162)
        self.assertEqual(sheet.A11, 6561)
        self.assertEqual(sheet.A12, 85)
        self.assertEqual(sheet.A13, 8)
        self.assertEqual(sheet.A14, 3)
        self.assertEqual(sheet.A15, 8.0 / 3)
        self.assertEqual(sheet.A16, 1.0 / 6)
        self.assertEqual(sheet.A17, 6561)
        self.assertEqual(sheet.A18, 43046721)
        self.assertEqual(sheet.A19, 6561)
        self.assertEqual(sheet.A20, 9)
        self.assertEqual(sheet.A21, 9)
        self.assertEqual(sheet.A22, 9)
        self.assertEqual(sheet.A23, -3)
        self.assertEqual(sheet.A24, 1)
        self.assertEqual(sheet.A25, -3)
        self.assertEqual(sheet.A26, 24)
        self.assertEqual(sheet.A27, 24)
        self.assertEqual(sheet.A28, 24)
        self.assertEqual(sheet.A29, 3.0 / 8)
        self.assertEqual(sheet.A30, 3.0 / 2)
        self.assertEqual(sheet.A31, 3.0 / 8)
        self.assertEqual(sheet.A37, 1)
        self.assertEqual(sheet.A38, 1)
        self.assertEqual(sheet.A39, 2)
        self.assertEqual(sheet.A40, 0.5)
        self.assertEqual(sheet.A41, -1)
        self.assertEqual(sheet.A42, -1)
        self.assertEqual(sheet.A43, -2)
        self.assertEqual(sheet.A44, 0)
        self.assertEqual(sheet.A45, 0)
        self.assertEqual(sheet.A46, 2)
        self.assertEqual(sheet.A47, 1)
        self.assertEqual(sheet.A48, 1)
        self.assertEqual(sheet.A49, 2)
        self.assertEqual(sheet.A50, 0)
        self.assertEqual(sheet.A51, 0)
        self.assertEqual(sheet.A52, -2)
        self.assertEqual(sheet.A53, 50)
        self.assertEqual(sheet.A54, Units.Quantity("9.7mm"))
        self.assertEqual(sheet.getContents("A1"), "=1 < 2 ? 3 : 4")
        self.assertEqual(sheet.getContents("A2"), "=1 + 2 < 3 + 4 ? 5 + 6 : 7 + 8")
        self.assertEqual(
            sheet.getContents("A3"),
            "=1 + 2 * 1 < 3 + 4 ? 5 * 2 + 6 * 3 + 2 ^ 4 : 7 * 2 + 8 * 3 + 2 ^ 3",
        )
        self.assertEqual(sheet.getContents("A4"), "123")
        self.assertEqual(sheet.getContents("A5"), "=123 + 321")
        self.assertEqual(sheet.getContents("A6"), "=123 * 2 + 321")
        self.assertEqual(sheet.getContents("A7"), "=123 * 2 + 333 / 3")
        self.assertEqual(sheet.getContents("A8"), "=123 * (2 + 321)")
        self.assertEqual(sheet.getContents("A9"), "=3 ^ 4")
        self.assertEqual(sheet.getContents("A10"), "=3 ^ 4 * 2")
        self.assertEqual(sheet.getContents("A11"), "=3 ^ (4 * 2)")
        self.assertEqual(sheet.getContents("A12"), "=3 ^ 4 + 4")
        self.assertEqual(sheet.getContents("A13"), "=1 + 4 / 2 + 5")
        self.assertEqual(sheet.getContents("A14"), "=(3 + 6) / (1 + 2)")
        self.assertEqual(sheet.getContents("A15"), "=1 * 2 / 3 * 4")
        self.assertEqual(sheet.getContents("A16"), "=1 * 2 / (3 * 4)")
        self.assertEqual(sheet.getContents("A17"), "=3 ^ 4 ^ 2")
        self.assertEqual(sheet.getContents("A18"), "=3 ^ (4 ^ 2)")
        self.assertEqual(sheet.getContents("A19"), "=3 ^ 4 ^ 2")
        self.assertEqual(sheet.getContents("A20"), "=3 + 4 + 2")
        self.assertEqual(sheet.getContents("A21"), "=3 + 4 + 2")
        self.assertEqual(sheet.getContents("A22"), "=3 + 4 + 2")
        self.assertEqual(sheet.getContents("A23"), "=3 - 4 - 2")
        self.assertEqual(sheet.getContents("A24"), "=3 - (4 - 2)")
        self.assertEqual(sheet.getContents("A25"), "=3 - 4 - 2")
        self.assertEqual(sheet.getContents("A26"), "=3 * 4 * 2")
        self.assertEqual(sheet.getContents("A27"), "=3 * 4 * 2")
        self.assertEqual(sheet.getContents("A28"), "=3 * 4 * 2")
        self.assertEqual(sheet.getContents("A29"), "=3 / 4 / 2")
        self.assertEqual(sheet.getContents("A30"), "=3 / (4 / 2)")
        self.assertEqual(sheet.getContents("A31"), "=3 / 4 / 2")
        self.assertEqual(sheet.getContents("A32"), "=pi * 3")
        self.assertEqual(sheet.getContents("A33"), "=A32 / 3")
        self.assertEqual(sheet.getContents("A34"), "=1 < 2 ? <<A>> : <<B>>")
        self.assertEqual(sheet.getContents("A35"), "=min(A32:A33)")
        self.assertEqual(sheet.getContents("A36"), "=(1 < 2 ? 0 : 1) * 3")
        self.assertEqual(sheet.getContents("A37"), "=8 / (2 ^ 2 * 2)")
        self.assertEqual(sheet.getContents("A38"), "=2 ^ 2 * 2 / 8")
        self.assertEqual(sheet.getContents("A39"), "=2 ^ (2 * 2) / 8")
        self.assertEqual(sheet.getContents("A40"), "=8 / 2 ^ (2 * 2)")
        self.assertEqual(sheet.getContents("A41"), "=-1")
        self.assertEqual(sheet.getContents("A42"), "=-1")
        self.assertEqual(sheet.getContents("A43"), "=-(1 + 1)")
        self.assertEqual(sheet.getContents("A44"), "=-(1 - 1)")
        self.assertEqual(sheet.getContents("A45"), "=-(-1 + 1)")
        self.assertEqual(sheet.getContents("A46"), "=-(-1 + -1)")
        self.assertEqual(sheet.getContents("A47"), "=+1")
        self.assertEqual(sheet.getContents("A48"), "=+1")
        self.assertEqual(sheet.getContents("A49"), "=+(1 + 1)")
        self.assertEqual(sheet.getContents("A50"), "=+(1 - 1)")
        self.assertEqual(sheet.getContents("A51"), "=+(-1 + 1)")
        self.assertEqual(sheet.getContents("A52"), "=+(-1 + -1)")

    def testNumbers(self):
        """Test different numbers"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A1", "1")
        sheet.set("A2", "1.5")
        sheet.set("A3", ".5")
        sheet.set("A4", "1e2")
        sheet.set("A5", "1E2")
        sheet.set("A6", "1e-2")
        sheet.set("A7", "1E-2")
        sheet.set("A8", "1.5e2")
        sheet.set("A9", "1.5E2")
        sheet.set("A10", "1.5e-2")
        sheet.set("A11", "1.5E-2")
        sheet.set("A12", ".5e2")
        sheet.set("A13", ".5E2")
        sheet.set("A14", ".5e-2")
        sheet.set("A15", ".5E-2")
        sheet.set("A16", "1/1")
        sheet.set("A17", "1/2")
        sheet.set("A18", "2/4")
        self.doc.recompute()
        self.assertEqual(sheet.A1, 1)
        self.assertEqual(sheet.A2, 1.5)
        self.assertEqual(sheet.A3, 0.5)
        self.assertEqual(sheet.A4, 1e2)
        self.assertEqual(sheet.A5, 1e2)
        self.assertEqual(sheet.A6, 1e-2)
        self.assertEqual(sheet.A7, 1e-2)
        self.assertEqual(sheet.A8, 1.5e2)
        self.assertEqual(sheet.A9, 1.5e2)
        self.assertEqual(sheet.A10, 1.5e-2)
        self.assertEqual(sheet.A11, 1.5e-2)
        self.assertEqual(sheet.A12, 0.5e2)
        self.assertEqual(sheet.A13, 0.5e2)
        self.assertEqual(sheet.A14, 0.5e-2)
        self.assertEqual(sheet.A15, 0.5e-2)
        self.assertEqual(sheet.A16, 1)
        self.assertEqual(sheet.A17, 0.5)
        self.assertEqual(sheet.A18, 0.5)

    def testQuantitiesAndFractionsAsNumbers(self):
        """Test quantities and simple fractions as numbers"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A1", "1mm")
        sheet.set("A2", "1/2")
        sheet.set("A3", "4mm/2")
        sheet.set("A4", "2/mm")
        sheet.set("A5", "4/2mm")
        sheet.set("A6", "6mm/3s")
        self.doc.recompute()
        self.assertEqual(sheet.A1, Units.Quantity("1 mm"))
        self.assertEqual(sheet.A2, 0.5)
        self.assertEqual(sheet.A3, Units.Quantity("2 mm"))
        self.assertEqual(sheet.A4, Units.Quantity("2 1/mm"))
        self.assertEqual(sheet.A5, Units.Quantity("2 1/mm"))
        self.assertEqual(sheet.A6, Units.Quantity("2 mm/s"))

    def testRemoveRows(self):
        """Removing rows -- check renaming of internal cells"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A3", "123")
        sheet.set("A1", "=A3")
        sheet.removeRows("2", 1)
        self.assertEqual(sheet.getContents("A1"), "=A2")

    def testInsertRows(self):
        """Inserting rows -- check renaming of internal cells"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("B1", "=B2")
        sheet.set("B2", "124")
        # Calling getContents() here activates ObjectIdentifier internal cache,
        # which needs to be tested as well.
        self.assertEqual(sheet.getContents("B1"), "=B2")
        sheet.insertRows("2", 1)
        self.assertEqual(sheet.getContents("B1"), "=B3")

    def testIssue3225(self):
        """Inserting rows -- check renaming of internal cells"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("B2", "25")
        sheet.set("B3", "=B2")
        sheet.insertRows("2", 1)
        self.assertEqual(sheet.getContents("B4"), "=B3")

    def testRenameAlias(self):
        """Test renaming of alias1 to alias2 in a spreadsheet"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("B1", "124")
        sheet.setAlias("B1", "alias1")
        sheet.set("B2", "=alias1")
        self.doc.recompute()
        self.assertEqual(sheet.get("alias1"), 124)
        self.assertEqual(sheet.get("B1"), 124)
        self.assertEqual(sheet.get("B2"), 124)
        sheet.setAlias("B1", "alias2")
        self.doc.recompute()
        self.assertEqual(sheet.get("alias2"), 124)
        self.assertEqual(sheet.getContents("B2"), "=alias2")

    def testRenameAlias2(self):
        """Test renaming of alias1 to alias2 in a spreadsheet, when referenced from another object"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("B1", "124")
        sheet.setAlias("B1", "alias1")
        box = self.doc.addObject("Part::Box", "Box")
        box.setExpression("Length", "Spreadsheet.alias1")
        sheet.setAlias("B1", "alias2")
        self.assertEqual(box.ExpressionEngine[0][1], "Spreadsheet.alias2")

    def testRenameAlias3(self):
        """Test renaming of document object referenced from another object"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("B1", "124")
        sheet.setAlias("B1", "alias1")
        box = self.doc.addObject("Part::Box", "Box")
        box.setExpression("Length", "Spreadsheet.alias1")
        box2 = self.doc.addObject("Part::Box", "Box")
        box2.setExpression("Length", "<<Spreadsheet>>.alias1")
        sheet.Label = "Params"
        self.assertEqual(box.ExpressionEngine[0][1], "Spreadsheet.alias1")
        self.assertEqual(box2.ExpressionEngine[0][1], "<<Params>>.alias1")

    def testAlias(self):
        """Playing with aliases"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Calc")
        sheet.setAlias("A1", "Test")
        self.assertEqual(sheet.getAlias("A1"), "Test")

        sheet.set("A1", "4711")
        self.doc.recompute()
        self.assertEqual(sheet.get("Test"), 4711)
        self.assertEqual(sheet.get("Test"), sheet.get("A1"))

    def testAmbiguousAlias(self):
        """Try to set the same alias twice (bug #2402)"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Calc")
        sheet.setAlias("A1", "Test")
        try:
            sheet.setAlias("A2", "Test")
            self.fail("An ambiguous alias was set which shouldn't be allowed")
        except Exception:
            self.assertEqual(sheet.getAlias("A2"), None)

    def testClearAlias(self):
        """This was causing a crash"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Calc")
        sheet.setAlias("A1", "Test")
        sheet.setAlias("A1", "")
        self.assertEqual(sheet.getAlias("A1"), None)

    def testSetInvalidAlias(self):
        """Try to use a cell address as alias name"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Calc")
        try:
            sheet.setAlias("A1", "B1")
        except Exception:
            self.assertEqual(sheet.getAlias("A1"), None)
        else:
            self.fail("A cell address was used as alias which shouldn't be allowed")

    def testSetInvalidAlias2(self):
        """Try to use a unit (reserved word) as alias name"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Calc")
        try:
            sheet.setAlias("A1", "mA")
        except Exception:
            self.assertEqual(sheet.getAlias("A1"), None)
        else:
            self.fail("A unit (reserved word) was used as alias which shouldn't be allowed")

    def testPlacementName(self):
        """Object name is equal to property name (bug #2389)"""
        if not FreeCAD.GuiUp:
            return

        import FreeCADGui

        o = self.doc.addObject("Part::FeaturePython", "Placement")
        FreeCADGui.Selection.addSelection(o)

    def testInvoluteGear(self):
        """Support of boolean or integer values"""
        try:
            import InvoluteGearFeature
        except ImportError:
            return
        InvoluteGearFeature.makeInvoluteGear("InvoluteGear")
        self.doc.recompute()
        sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        sketch.addGeometry(Part.LineSegment(v(0, 0, 0), v(10, 10, 0)), False)
        sketch.addConstraint(Sketcher.Constraint("Distance", 0, 65.285388))
        sketch.setExpression("Constraints[0]", "InvoluteGear.NumberOfTeeth")
        self.doc.recompute()
        self.assertIn("Up-to-date", sketch.State)

    def testSketcher(self):
        """Mixup of Label and Name (bug #2407)"""
        sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.setAlias("A1", "Length")
        self.doc.recompute()
        sheet.set("A1", "47,11")
        self.doc.recompute()

        index = sketch.addGeometry(Part.LineSegment(v(0, 0, 0), v(10, 10, 0)), False)
        sketch.addConstraint(Sketcher.Constraint("Distance", index, 14.0))
        self.doc.recompute()
        sketch.setExpression("Constraints[0]", "<<Spreadsheet>>.Length")
        self.doc.recompute()
        sheet.Label = "Calc"
        self.doc.recompute()
        self.assertEqual(sketch.ExpressionEngine[0][1], "<<Calc>>.Length")
        self.assertIn("Up-to-date", sketch.State)

    def testCrossDocumentLinks(self):
        """Expressions across files are not saved (bug #2442)"""

        # Create a box
        box = self.doc.addObject("Part::Box", "Box")

        # Create a second document with a cylinder
        doc2 = FreeCAD.newDocument()
        cylinder = doc2.addObject("Part::Cylinder", "Cylinder")
        cylinder.setExpression("Radius", "cube#Cube.Height")

        # Save and close first document
        self.doc.saveAs(self.TempPath + os.sep + "cube.fcstd")
        FreeCAD.closeDocument(self.doc.Name)

        # Save and close second document
        doc2.saveAs(self.TempPath + os.sep + "cylinder.fcstd")
        FreeCAD.closeDocument(doc2.Name)

        # Open both documents again
        self.doc = FreeCAD.openDocument(self.TempPath + os.sep + "cube.fcstd")
        doc2 = FreeCAD.openDocument(self.TempPath + os.sep + "cylinder.fcstd")

        # Check reference between them
        self.assertEqual(doc2.getObject("Cylinder").ExpressionEngine[0][1], "cube#Cube.Height")

        # Close second document
        FreeCAD.closeDocument(doc2.Name)

    def testMatrix(self):
        """Test Matrix/Vector/Placement/Rotation operations"""

        def plm_equal(plm1, plm2):
            from math import sqrt

            qpair = zip(plm1.Rotation.Q, plm2.Rotation.Q)
            qdiff1 = sqrt(sum([(v1 - v2) ** 2 for v1, v2 in qpair]))
            qdiff2 = sqrt(sum([(v1 + v2) ** 2 for v1, v2 in qpair]))
            return (plm1.Base - plm2.Base).Length < 1e-7 and (qdiff1 < 1e-12 or dqiff2 < 1e-12)

        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")

        mat = FreeCAD.Matrix()
        mat.scale(2, 1, 2)
        imat = mat.inverse()

        vec = FreeCAD.Vector(2, 1, 2)

        rot = FreeCAD.Rotation(FreeCAD.Vector(0, 1, 0), 45)
        irot = rot.inverted()

        pla = FreeCAD.Placement(vec, rot)
        ipla = pla.inverse()

        sheet.set("A1", "=vector(2, 1, 2)")

        # different ways of calling mscale()
        sheet.set("B1", "=mscale(create(<<matrix>>), A1)")
        sheet.set("C1", "=mscale(create(<<matrix>>), tuple(2, 1, 2))")
        sheet.set("A2", "=mscale(create(<<matrix>>), 2, 1, 2)")

        # test matrix power operation
        sheet.set("B2", "=A2^-2")
        sheet.set("C2", "=A2^-1")
        sheet.set("D2", "=A2^0")
        sheet.set("E2", "=A2^1")
        sheet.set("F2", "=A2^2")
        sheet.set("G2", "=matrix(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)")
        sheet.set("H2", "=G2^-1")

        sheet.set("A3", "=rotation(vector(0, 1, 0), 45)")

        # test rotation power operation
        sheet.set("B3", "=A3^-2")
        sheet.set("C3", "=A3^-1")
        sheet.set("D3", "=A3^0")
        sheet.set("E3", "=A3^1")
        sheet.set("F3", "=A3^2")

        sheet.set("A4", "=placement(A1, A3)")

        # test placement power operation
        sheet.set("B4", "=A4^-2")
        sheet.set("C4", "=A4^-1")
        sheet.set("D4", "=A4^0")
        sheet.set("E4", "=A4^1")
        sheet.set("F4", "=A4^2")

        # vector transformation with mixing matrix and placement and rotation
        sheet.set("A5", "=A2*A3*A4*A1")
        sheet.set("B5", "=B2*B4*B3*A1")
        sheet.set("C5", "=C3*C2*C4*A1")
        sheet.set("D5", "=D3*D4*D2*A1")
        sheet.set("E5", "=E4*E2*E3*A1")
        sheet.set("F5", "=F3*F4*F2*A1")

        # inverse of the above transformation with power -1 and minvert()
        sheet.set("A6", "=A4^-1 * minvert(A3) * A2^-1 * A5")
        sheet.set("B6", "=minvert(B3) * B4^-1 * minvert(B2) * B5")
        sheet.set("C6", "=C4^-1 * C2^-1 * C3^-1 * C5")
        sheet.set("D6", "=minvert(D4*D2) * minvert(D3) * D5")
        sheet.set("E6", "=(E2 * E3)^-1 * E4^-1 * E5")
        sheet.set("F6", "=(F3*F4*F2)^-1 * F5")

        # Rotate and translate.
        sheet.set("A7", "=placement(vector(1; 2; 3), vector(1; 0; 0); 0)")
        sheet.set("B7", "=mrotate(A7; vector(1; 0; 0); 90)")
        sheet.set("C7", "=mrotatex(A7; 90)")
        sheet.set("D7", "=mrotatey(A7; 90)")
        sheet.set("E7", "=mrotatez(A7; 90)")
        sheet.set("F7", "=mtranslate(A7; vector(1; 2; 3))")
        sheet.set("G7", "=mtranslate(A7; 1; 2; 3)")

        # Compatibility with old syntax.
        sheet.set("A8", "=create(<<vector>>, 2, 1, 2)")
        sheet.set("B8", "=create(<<rotation>>, create(<<vector>>, 0, 1, 0), 45)")
        sheet.set("C8", "=create(<<placement>>, A8, B8)")

        self.doc.recompute()

        self.assertEqual(sheet.A1, vec)

        self.assertEqual(sheet.B1, mat)
        self.assertEqual(sheet.C1, mat)
        self.assertEqual(sheet.A2, mat)

        self.assertEqual(sheet.B2, imat * imat)
        self.assertEqual(sheet.B2, mat**-2)
        self.assertEqual(sheet.C2, imat)
        self.assertEqual(sheet.C2, mat**-1)
        self.assertEqual(sheet.D2, FreeCAD.Matrix())
        self.assertEqual(sheet.D2, mat**0)
        self.assertEqual(sheet.E2, mat)
        self.assertEqual(sheet.E2, mat**1)
        self.assertEqual(sheet.F2, mat * mat)
        self.assertEqual(sheet.F2, mat**2)

        self.assertTrue(sheet.H2.startswith("ERR: Cannot invert singular matrix"))

        self.assertEqual(sheet.A3, rot)

        rtol = 1e-12
        self.assertTrue(sheet.B3.isSame(irot * irot, rtol))
        self.assertTrue(sheet.B3.isSame(rot**-2, rtol))
        self.assertTrue(sheet.C3.isSame(irot, rtol))
        self.assertTrue(sheet.C3.isSame(rot**-1, rtol))
        self.assertTrue(sheet.D3.isSame(FreeCAD.Rotation(), rtol))
        self.assertTrue(sheet.D3.isSame(rot**0, rtol))
        self.assertTrue(sheet.E3.isSame(rot, rtol))
        self.assertTrue(sheet.E3.isSame(rot**1, rtol))
        self.assertTrue(sheet.F3.isSame(rot * rot, rtol))
        self.assertTrue(sheet.F3.isSame(rot**2, rtol))

        self.assertEqual(sheet.A4, pla)

        self.assertTrue(plm_equal(sheet.B4, ipla * ipla))
        self.assertTrue(plm_equal(sheet.B4, pla**-2))
        self.assertTrue(plm_equal(sheet.C4, ipla))
        self.assertTrue(plm_equal(sheet.C4, pla**-1))
        self.assertTrue(plm_equal(sheet.D4, FreeCAD.Placement()))
        self.assertTrue(plm_equal(sheet.D4, pla**0))
        self.assertTrue(plm_equal(sheet.E4, pla))
        self.assertTrue(plm_equal(sheet.E4, pla**1))
        self.assertTrue(plm_equal(sheet.F4, pla * pla))
        self.assertTrue(plm_equal(sheet.F4, pla**2))

        tol = 1e-10

        self.assertLess(
            sheet.A5.distanceToPoint(
                sheet.A2.multiply(sheet.A3.Matrix).multiply(sheet.A4.Matrix).multVec(vec)
            ),
            tol,
        )
        self.assertLess(
            sheet.B5.distanceToPoint(
                sheet.B2.multiply(sheet.B4.Matrix).multiply(sheet.B3.Matrix).multVec(vec)
            ),
            tol,
        )
        self.assertLess(
            sheet.C5.distanceToPoint(
                sheet.C3.Matrix.multiply(sheet.C2).multiply(sheet.C4.Matrix).multVec(vec)
            ),
            tol,
        )
        self.assertLess(
            sheet.D5.distanceToPoint(
                sheet.D3.Matrix.multiply(sheet.D4.Matrix).multiply(sheet.D2).multVec(vec)
            ),
            tol,
        )
        self.assertLess(
            sheet.E5.distanceToPoint(
                sheet.E4.Matrix.multiply(sheet.E2).multiply(sheet.E3.Matrix).multVec(vec)
            ),
            tol,
        )
        self.assertLess(
            sheet.F5.distanceToPoint(
                sheet.F3.Matrix.multiply(sheet.F4.Matrix).multiply(sheet.F2).multVec(vec)
            ),
            tol,
        )

        self.assertLess(sheet.A6.distanceToPoint(vec), tol)
        self.assertLess(sheet.B6.distanceToPoint(vec), tol)
        self.assertLess(sheet.C6.distanceToPoint(vec), tol)
        self.assertLess(sheet.D6.distanceToPoint(vec), tol)
        self.assertLess(sheet.E6.distanceToPoint(vec), tol)
        self.assertLess(sheet.F6.distanceToPoint(vec), tol)

        self.assertTrue(sheet.A7.Base.isEqual(FreeCAD.Vector(1, 2, 3), tol))
        self.assertTrue(sheet.B7.Base.isEqual(FreeCAD.Vector(1, -3, 2), tol))
        self.assertTrue(sheet.C7.Base.isEqual(FreeCAD.Vector(1, -3, 2), tol))
        self.assertTrue(sheet.D7.Base.isEqual(FreeCAD.Vector(3, 2.0, -1), tol))
        self.assertTrue(sheet.E7.Base.isEqual(FreeCAD.Vector(-2, 1, 3.0), tol))
        self.assertTrue(sheet.F7.Base.isEqual(FreeCAD.Vector(2, 4, 6), tol))
        self.assertTrue(sheet.G7.Base.isEqual(FreeCAD.Vector(2, 4, 6), tol))

        self.assertEqual(sheet.A8, vec)
        self.assertEqual(sheet.B8, rot)
        self.assertEqual(sheet.C8, pla)

    def testIssue3128(self):
        """Regression test for issue 3128; mod should work with arbitrary units for both arguments"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A1", "=mod(7mm;3mm)")
        sheet.set("A2", "=mod(7kg;3mm)")
        self.doc.recompute()
        self.assertEqual(sheet.A1, Units.Quantity("1"))
        self.assertEqual(sheet.A2, Units.Quantity("1 kg/mm"))

    def testIssue3363(self):
        """Regression test for issue 3363; Nested conditionals statement fails with additional conditional statement in false-branch"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A1", "1")
        sheet.set("B1", "=A1==1?11:(A1==2?12:13)")
        sheet.set("C1", "=A1==1?(A1==2?12:13) : 11")
        self.doc.recompute()

        # Save and close first document
        self.doc.saveAs(self.TempPath + os.sep + "conditionals.fcstd")
        FreeCAD.closeDocument(self.doc.Name)

        # Open documents again
        self.doc = FreeCAD.openDocument(self.TempPath + os.sep + "conditionals.fcstd")

        sheet = self.doc.getObject("Spreadsheet")
        self.assertEqual(sheet.getContents("B1"), "=A1 == 1 ? 11 : (A1 == 2 ? 12 : 13)")
        self.assertEqual(sheet.getContents("C1"), "=A1 == 1 ? (A1 == 2 ? 12 : 13) : 11")

    def testIssue3432(self):
        """Regression test for issue 3432; numbers with units are ignored from aggregates"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A1", "1mm")
        sheet.set("B1", "2mm")
        sheet.set("C1", "=max(A1:B1;3mm)")
        self.doc.recompute()
        self.assertEqual(sheet.get("C1"), Units.Quantity("3 mm"))

    def testIssue4156(self):
        """Regression test for issue 4156; necessarily use of leading '=' to enter an expression, creates inconsistent behavior depending on the spreadsheet state"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A3", "A1")
        sheet.set("A1", "1000")
        self.doc.recompute()
        sheet.set("A3", "")
        sheet.set("A3", "A1")
        self.assertEqual(sheet.getContents("A3"), "'A1")

    def testInsertRowsAlias(self):
        """Regression test for issue 4429; insert rows to sheet with aliases"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A3", "1")
        sheet.setAlias("A3", "alias1")
        sheet.set("A4", "=alias1 + 1")
        sheet.setAlias("A4", "alias2")
        sheet.set("A5", "=alias2 + 1")
        self.doc.recompute()
        sheet.insertRows("1", 1)
        self.doc.recompute()
        self.assertEqual(sheet.A6, 3)

    def testInsertColumnsAlias(self):
        """Regression test for issue 4429; insert columns to sheet with aliases"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("C1", "1")
        sheet.setAlias("C1", "alias1")
        sheet.set("D1", "=alias1 + 1")
        sheet.setAlias("D1", "alias2")
        sheet.set("E1", "=alias2 + 1")
        self.doc.recompute()
        sheet.insertColumns("A", 1)
        self.doc.recompute()
        self.assertEqual(sheet.F1, 3)

    def testRemoveRowsAlias(self):
        """Regression test for issue 4429; remove rows from sheet with aliases"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A3", "1")
        sheet.setAlias("A3", "alias1")
        sheet.set("A5", "=alias1 + 1")
        sheet.setAlias("A5", "alias2")
        sheet.set("A4", "=alias2 + 1")
        self.doc.recompute()
        sheet.removeRows("1", 1)
        self.doc.recompute()
        self.assertEqual(sheet.A3, 3)

    def testRemoveRowsAliasReuseName(self):
        """Regression test for issue 4492; deleted aliases remains in database"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.setAlias("B2", "test")
        self.doc.recompute()
        sheet.removeRows("2", 1)
        sheet.setAlias("B3", "test")

    def testRemoveColumnsAlias(self):
        """Regression test for issue 4429; remove columns from sheet with aliases"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("C1", "1")
        sheet.setAlias("C1", "alias1")
        sheet.set("E1", "=alias1 + 1")
        sheet.setAlias("E1", "alias2")
        sheet.set("D1", "=alias2 + 1")
        self.doc.recompute()
        sheet.removeColumns("A", 1)
        self.doc.recompute()
        self.assertEqual(sheet.C1, 3)

    def testRemoveColumnsAliasReuseName(self):
        """Regression test for issue 4492; deleted aliases remains in database"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.setAlias("B2", "test")
        self.doc.recompute()
        sheet.removeColumns("B", 1)
        sheet.setAlias("C3", "test")

    def testUndoAliasCreationReuseName(self):
        """Test deleted aliases by undo remains in database"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")

        self.doc.UndoMode = 1
        self.doc.openTransaction("create alias")
        sheet.setAlias("B2", "test")
        self.doc.commitTransaction()
        self.doc.recompute()

        self.doc.undo()
        self.doc.recompute()
        sheet.setAlias("C3", "test")

    def testCrossLinkEmptyPropertyName(self):
        # https://forum.freecad.org/viewtopic.php?f=3&t=58603
        base = FreeCAD.newDocument("base")
        sheet = base.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.setAlias("A1", "x")
        sheet.set("x", "42mm")
        base.recompute()

        square = FreeCAD.newDocument("square")
        body = square.addObject("PartDesign::Body", "Body")
        box = square.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        box.Length = 10.00
        box.Width = 10.00
        box.Height = 10.00
        square.recompute()

        basePath = self.TempPath + os.sep + "base.FCStd"
        base.saveAs(basePath)
        squarePath = self.TempPath + os.sep + "square.FCStd"
        square.saveAs(squarePath)

        base.save()
        square.save()

        FreeCAD.closeDocument(square.Name)
        FreeCAD.closeDocument(base.Name)

        ##
        ## preparation done
        base = FreeCAD.openDocument(basePath)
        square = FreeCAD.openDocument(squarePath)

        square.Box.setExpression("Length", "base#Spreadsheet.x")
        square.recompute()

        square.save()
        base.save()
        FreeCAD.closeDocument(square.Name)
        FreeCAD.closeDocument(base.Name)

    def testExpressionWithAlias(self):
        # https://forum.freecad.org/viewtopic.php?p=564502#p564502
        ss1 = self.doc.addObject("Spreadsheet::Sheet", "Input")
        ss1.setAlias("A1", "one")
        ss1.setAlias("A2", "two")
        ss1.set("A1", "1")
        ss1.set("A2", "2")
        self.doc.recompute()

        ss2 = self.doc.addObject("Spreadsheet::Sheet", "Output")
        ss2.set("A1", "=Input.A1 + Input.A2")
        ss2.set("A2", "=Input.one + Input.two")
        ss2.set("A3", "=<<Input>>.A1 + <<Input>>.A2")
        ss2.set("A4", "=<<Input>>.one + <<Input>>.two")
        self.doc.recompute()

        self.assertEqual(ss2.get("A1"), 3)
        self.assertEqual(ss2.get("A2"), 3)
        self.assertEqual(ss2.get("A3"), 3)
        self.assertEqual(ss2.get("A4"), 3)

        project_path = self.TempPath + os.sep + "alias.FCStd"
        self.doc.saveAs(project_path)
        FreeCAD.closeDocument(self.doc.Name)
        self.doc = FreeCAD.openDocument(project_path)
        ss1 = self.doc.Input
        ss2 = self.doc.Output

        self.assertEqual(ss2.get("A1"), 3)
        self.assertEqual(ss2.get("A2"), 3)
        self.assertEqual(ss2.get("A3"), 3)
        self.assertEqual(ss2.get("A4"), 3)

        ss1.set("A1", "2")
        self.doc.recompute()

        self.assertEqual(ss1.get("A1"), 2)
        self.assertEqual(ss1.get("one"), 2)

        self.assertEqual(ss2.get("A1"), 4)
        self.assertEqual(ss2.get("A2"), 4)
        self.assertEqual(ss2.get("A3"), 4)
        self.assertEqual(ss2.get("A4"), 4)

    def testIssue6844(self):
        body = self.doc.addObject("App::FeaturePython", "Body")
        body.addProperty("App::PropertyEnumeration", "Configuration")
        body.Configuration = ["Item1", "Item2", "Item3"]

        sheet = self.doc.addObject("Spreadsheet::Sheet", "Sheet")
        sheet.addProperty("App::PropertyString", "A2")
        sheet.A2 = "Item2"
        sheet.addProperty("App::PropertyEnumeration", "body")
        sheet.body = ["Item1", "Item2", "Item3"]

        sheet.setExpression(".body.Enum", "cells[<<A2:|>>]")
        sheet.setExpression(
            ".cells.Bind.B1.ZZ1",
            "tuple(.cells; <<B>> + str(hiddenref(Body.Configuration) + 2); <<ZZ>> + str(hiddenref(Body.Configuration) + 2))",
        )

        self.doc.recompute()
        self.doc.UndoMode = 0
        self.doc.removeObject("Body")
        sheet.clearAll()

    def testIssue6840(self):
        body = self.doc.addObject("App::FeaturePython", "Body")
        body.addProperty("App::PropertyEnumeration", "Configuration")
        body.Configuration = ["Item1", "Item2", "Item3"]

        sheet = self.doc.addObject("Spreadsheet::Sheet", "Sheet")
        sheet.addProperty("App::PropertyString", "A2")
        sheet.A2 = "Item2"
        sheet.addProperty("App::PropertyEnumeration", "body")
        sheet.body = ["Item1", "Item2", "Item3"]

        sheet.setExpression(".body.Enum", "cells[<<A2:|>>]")
        sheet.setExpression(
            ".cells.Bind.B1.ZZ1",
            "tuple(.cells; <<B>> + str(hiddenref(Body.Configuration) + 2); <<ZZ>> + str(hiddenref(Body.Configuration) + 2))",
        )

        self.doc.recompute()
        self.doc.clearDocument()

    def testFixPR6843(self):
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Sheet")
        sheet.set("A5", "a")
        sheet.set("A6", "b")
        self.doc.recompute()
        sheet.insertRows("6", 1)
        self.doc.recompute()
        self.assertEqual(sheet.A5, "a")
        self.assertEqual(sheet.A7, "b")
        with self.assertRaises(AttributeError):
            self.assertEqual(sheet.A6, "")

    def testBindAcrossSheets(self):
        ss1 = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet1")
        ss2 = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet2")
        ss2.set("B1", "B1")
        ss2.set("B2", "B2")
        ss2.set("C1", "C1")
        ss2.set("C2", "C2")
        ss2.set("D1", "D1")
        ss2.set("D2", "D2")

        ss1.setExpression(".cells.Bind.A3.C4", "tuple(Spreadsheet2.cells, <<B1>>, <<D2>>)")
        self.doc.recompute()

        self.assertEqual(ss1.A3, ss2.B1)
        self.assertEqual(ss1.A4, ss2.B2)
        self.assertEqual(ss1.B3, ss2.C1)
        self.assertEqual(ss1.B4, ss2.C2)
        self.assertEqual(ss1.C3, ss2.D1)
        self.assertEqual(ss1.C4, ss2.D2)

        self.assertEqual(len(ss1.ExpressionEngine), 1)
        ss1.setExpression(".cells.Bind.A3.C4", None)
        self.doc.recompute()

    def testBindHiddenRefAcrossSheets(self):
        ss1 = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet1")
        ss2 = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet2")
        ss2.set("B1", "B1")
        ss2.set("B2", "B2")
        ss2.set("C1", "C1")
        ss2.set("C2", "C2")
        ss2.set("D1", "D1")
        ss2.set("D2", "D2")

        self.doc.recompute()
        ss1.setExpression(".cells.Bind.A3.C4", None)
        ss1.setExpression(
            ".cells.BindHiddenRef.A3.C4", "hiddenref(tuple(Spreadsheet2.cells, <<B1>>, <<D2>>))"
        )
        self.doc.recompute()

        ss1.recompute()  # True
        self.assertEqual(ss1.A3, ss2.B1)

        ss1.setExpression(".cells.Bind.A3.C4", None)
        ss1.setExpression(".cells.BindHiddenRef.A3.C4", None)
        self.doc.recompute()
        self.assertEqual(len(ss1.ExpressionEngine), 0)

    def testMergeCells(self):
        ss1 = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet1")
        ss1.mergeCells("A1:B4")
        ss1.mergeCells("C1:D4")
        self.doc.recompute()
        ss1.set("B1", "fail")
        self.doc.recompute()
        with self.assertRaises(AttributeError):
            self.assertEqual(ss1.B1, "fail")

    def testMergeCellsAndBind(self):
        ss1 = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet1")
        ss1.mergeCells("A1:B1")
        ss1.setExpression(".cells.Bind.A1.A1", "tuple(.cells, <<A2>>, <<A2>>)")
        ss1.set("A2", "test")
        self.doc.recompute()
        self.assertEqual(ss1.A1, ss1.A2)
        ss1.set("B1", "fail")
        self.doc.recompute()
        with self.assertRaises(AttributeError):
            self.assertEqual(ss1.B1, "fail")

    def testGetUsedCells(self):
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        test_cells = ["B13", "C14", "D15"]
        for i, cell in enumerate(test_cells):
            sheet.set(cell, str(i))

        used_cells = sheet.getUsedCells()
        self.assertEqual(len(used_cells), len(test_cells))
        for cell in test_cells:
            self.assertTrue(cell in used_cells)

        for cell in test_cells:
            sheet.set(cell, "")
            sheet.setAlignment(cell, "center")
        non_empty_cells = sheet.getUsedCells()
        self.assertEqual(len(non_empty_cells), len(test_cells))  # Alignment counts as "used"

    def testGetUsedRange(self):
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        test_cells = ["C5", "Z3", "D10", "E20"]
        for i, cell in enumerate(test_cells):
            sheet.set(cell, str(i))
        used_range = sheet.getUsedRange()
        self.assertEquals(used_range, ("C3", "Z20"))

        for i, cell in enumerate(test_cells):
            sheet.set(cell, "")
            sheet.setAlignment(cell, "center")
        used_range = sheet.getUsedRange()
        self.assertEquals(used_range, ("C3", "Z20"))

    def testGetNonEmptyCells(self):
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        test_cells = ["B13", "C14", "D15"]
        for i, cell in enumerate(test_cells):
            sheet.set(cell, str(i))

        non_empty_cells = sheet.getNonEmptyCells()
        self.assertEqual(len(non_empty_cells), len(test_cells))
        for cell in test_cells:
            self.assertTrue(cell in non_empty_cells)

        for cell in test_cells:
            sheet.set(cell, "")
            sheet.setAlignment(cell, "center")
        non_empty_cells = sheet.getNonEmptyCells()
        self.assertEqual(len(non_empty_cells), 0)  # Alignment does not count as "non-empty"

    def testGetNonEmptyRange(self):
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        test_cells = ["C5", "Z3", "D10", "E20"]
        for i, cell in enumerate(test_cells):
            sheet.set(cell, str(i))
        non_empty_range = sheet.getNonEmptyRange()
        self.assertEquals(non_empty_range, ("C3", "Z20"))

        for i, cell in enumerate(test_cells):
            sheet.set(cell, "")
            sheet.setAlignment(cell, "center")
        more_cells = ["D10", "X5", "E10", "K15"]
        for i, cell in enumerate(more_cells):
            sheet.set(cell, str(i))
        non_empty_range = sheet.getNonEmptyRange()
        self.assertEquals(non_empty_range, ("D5", "X15"))

    def testAliasEmptyCell(self):
        # https://github.com/FreeCAD/FreeCAD/issues/7841
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.setAlias("A1", "aliasOfEmptyCell")
        self.assertEqual(sheet.getCellFromAlias("aliasOfEmptyCell"), "A1")

    def testParensAroundCondition(self):
        """Parens around a condition should be accepted"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")

        sheet.set("A1", "=(1 == 1) ? 1 : 0")
        self.doc.recompute()
        self.assertEqual(sheet.getContents("A1"), "=1 == 1 ? 1 : 0")
        self.assertEqual(sheet.A1, 1)

    def testIssue6395(self):
        """Testing strings are correctly saved and restored"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A1", "'36C")  # Use a string that may be parsed as a quantity
        self.doc.recompute()

        self.doc.saveAs(self.TempPath + os.sep + "string.fcstd")
        FreeCAD.closeDocument(self.doc.Name)

        self.doc = FreeCAD.openDocument(self.TempPath + os.sep + "string.fcstd")

        sheet = self.doc.getObject("Spreadsheet")
        self.assertEqual(sheet.getContents("A1"), "'36C")
        self.assertEqual(sheet.get("A1"), "36C")

    def testVectorFunctions(self):
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")

        sheet.set("A1", "=vcross(vector(1; 2; 3); vector(1; 5; 7))")

        sheet.set("B1", "=vdot(vector(1; 2; 3); vector(4; -5; 6))")

        sheet.set("C1", "=vangle(vector(1; 0; 0); vector(0; 1; 0))")

        sheet.set("D1", "=vnormalize(vector(1; 0; 0))")
        sheet.set("D2", "=vnormalize(vector(1; 1; 1))")

        sheet.set("E1", "=vscale(vector(1; 2; 3); 2; 3; 4)")
        sheet.set("E2", "=vscalex(vector(1; 2; 3); -2)")
        sheet.set("E3", "=vscaley(vector(1; 2; 3); -2)")
        sheet.set("E4", "=vscalez(vector(1; 2; 3); -2)")

        sheet.set("F1", "=vlinedist(vector(1; 2; 3); vector(2; 3; 4); vector(3; 4; 5))")
        sheet.set("F2", "=vlinesegdist(vector(1; 2; 3); vector(2; 3; 4); vector(3; 4; 5))")
        sheet.set("F3", "=vlineproj(vector(1; 2; 3); vector(2; 3; 4); vector(3; 4; 5))")
        sheet.set("F4", "=vplanedist(vector(1; 2; 3); vector(2; 3; 4); vector(3; 4; 5))")
        sheet.set("F5", "=vplaneproj(vector(1; 2; 3); vector(2; 3; 4); vector(3; 4; 5))")

        self.doc.recompute()

        tolerance = 1e-10

        self.assertEqual(sheet.A1, FreeCAD.Vector(-1, -4, 3))

        self.assertEqual(sheet.B1, 12)

        self.assertEqual(sheet.C1, 90)

        self.assertEqual(sheet.D1, FreeCAD.Vector(1, 0, 0))
        self.assertLess(
            sheet.D2.distanceToPoint(FreeCAD.Vector(1 / sqrt(3), 1 / sqrt(3), 1 / sqrt(3))),
            tolerance,
        )

        self.assertEqual(sheet.E1, FreeCAD.Vector(2, 6, 12))
        self.assertEqual(sheet.E2, FreeCAD.Vector(-2, 2, 3))
        self.assertEqual(sheet.E3, FreeCAD.Vector(1, -4, 3))
        self.assertEqual(sheet.E4, FreeCAD.Vector(1, 2, -6))

        self.assertLess(abs(sheet.F1.Value - 0.3464), 0.0001)
        self.assertEqual(sheet.F2, FreeCAD.Vector(1, 1, 1))
        self.assertLess(sheet.F3.distanceToPoint(FreeCAD.Vector(0.28, 0.04, -0.2)), tolerance)
        self.assertLess(abs(sheet.F4.Value - -1.6971), 0.0001)
        self.assertEqual(sheet.F5, FreeCAD.Vector(1.72, 2.96, 4.2))

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument(self.doc.Name)
