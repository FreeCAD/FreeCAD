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
# Test Spreadsheet module and expression engine
# ----------------------------------------------------------------------------------


#############################################################################################
class SpreadsheetAggregates(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument()
        self.sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test_sum(self):
        self.sheet.set("B13", "4")
        self.sheet.set("B14", "5")
        self.sheet.set("B15", "6")
        self.sheet.set("A1", "=sum(1)")
        self.sheet.set("A2", "=sum(1;2)")
        self.sheet.set("A3", "=sum(1;2;3)")
        self.sheet.set("A4", "=sum(1;2;3;B13)")
        self.sheet.set("A5", "=sum(1;2;3;B13:B15)")

        self.doc.recompute()

        self.assertEqual(self.sheet.A1, 1)
        self.assertEqual(self.sheet.A2, 3)
        self.assertEqual(self.sheet.A3, 6)
        self.assertEqual(self.sheet.A4, 10)
        self.assertEqual(self.sheet.A5, 21)

    def test_min(self):
        self.sheet.set("B13", "4")
        self.sheet.set("B14", "5")
        self.sheet.set("B15", "6")
        self.sheet.set("B1", "=min(1)")
        self.sheet.set("B2", "=min(1;2)")
        self.sheet.set("B3", "=min(1;2;3)")
        self.sheet.set("B4", "=min(1;2;3;B13)")
        self.sheet.set("B5", "=min(1;2;3;B13:B15)")

        self.doc.recompute()

        self.assertEqual(self.sheet.B1, 1)
        self.assertEqual(self.sheet.B2, 1)
        self.assertEqual(self.sheet.B3, 1)
        self.assertEqual(self.sheet.B4, 1)
        self.assertEqual(self.sheet.B5, 1)

    def test_max(self):
        self.sheet.set("B13", "4")
        self.sheet.set("B14", "5")
        self.sheet.set("B15", "6")
        self.sheet.set("C1", "=max(1)")
        self.sheet.set("C2", "=max(1;2)")
        self.sheet.set("C3", "=max(1;2;3)")
        self.sheet.set("C4", "=max(1;2;3;B13)")
        self.sheet.set("C5", "=max(1;2;3;B13:B15)")

        self.doc.recompute()

        self.assertEqual(self.sheet.C1, 1)
        self.assertEqual(self.sheet.C2, 2)
        self.assertEqual(self.sheet.C3, 3)
        self.assertEqual(self.sheet.C4, 4)
        self.assertEqual(self.sheet.C5, 6)

    def test_stddev(self):
        self.sheet.set("B13", "4")
        self.sheet.set("B14", "5")
        self.sheet.set("B15", "6")
        self.sheet.set("D1", "=stddev(1)")
        self.sheet.set("D2", "=stddev(1;2)")
        self.sheet.set("D3", "=stddev(1;2;3)")
        self.sheet.set("D4", "=stddev(1;2;3;B13)")
        self.sheet.set("D5", "=stddev(1;2;3;B13:B15)")

        self.doc.recompute()

        self.assertTrue(
            self.sheet.D1.startswith("ERR: Invalid number of entries: at least two required.")
        )
        self.assertEqual(self.sheet.D2, 0.7071067811865476)
        self.assertEqual(self.sheet.D3, 1.0)
        self.assertEqual(self.sheet.D4, 1.2909944487358056)
        self.assertEqual(self.sheet.D5, 1.8708286933869707)

    def test_count(self):
        self.sheet.set("B13", "4")
        self.sheet.set("B14", "5")
        self.sheet.set("B15", "6")
        self.sheet.set("E1", "=count(1)")
        self.sheet.set("E2", "=count(1;2)")
        self.sheet.set("E3", "=count(1;2;3)")
        self.sheet.set("E4", "=count(1;2;3;B13)")
        self.sheet.set("E5", "=count(1;2;3;B13:B15)")

        self.doc.recompute()

        self.assertEqual(self.sheet.E1, 1)
        self.assertEqual(self.sheet.E2, 2)
        self.assertEqual(self.sheet.E3, 3)
        self.assertEqual(self.sheet.E4, 4)
        self.assertEqual(self.sheet.E5, 6)

    def test_average(self):
        self.sheet.set("B13", "4")
        self.sheet.set("B14", "5")
        self.sheet.set("B15", "6")
        self.sheet.set("F1", "=average(1)")
        self.sheet.set("F2", "=average(1;2)")
        self.sheet.set("F3", "=average(1;2;3)")
        self.sheet.set("F4", "=average(1;2;3;B13)")
        self.sheet.set("F5", "=average(1;2;3;B13:B15)")

        self.doc.recompute()

        self.assertEqual(self.sheet.F1, 1)
        self.assertEqual(self.sheet.F2, (1.0 + 2.0) / 2.0)
        self.assertEqual(self.sheet.F3, (1.0 + 2 + 3) / 3)
        self.assertEqual(self.sheet.F4, (1.0 + 2 + 3 + 4) / 4)
        self.assertEqual(self.sheet.F5, (1.0 + 2 + 3 + 4 + 5 + 6) / 6)

    def test_range(self):
        self.sheet.set("C13", "4mm")
        self.sheet.set("C14", "5mm")
        self.sheet.set("C15", "6mm")
        self.sheet.set("G1", "=average(C13:C15)")
        self.sheet.set("G2", "=min(C13:C15)")
        self.sheet.set("G3", "=max(C13:C15)")
        self.sheet.set("G4", "=count(C13:C15)")
        self.sheet.set("G5", "=stddev(C13:C15)")
        self.sheet.set("G6", "=sum(C13:C15)")

        self.doc.recompute()

        self.assertEqual(self.sheet.G1, Units.Quantity("5 mm"))
        self.assertEqual(self.sheet.G2, Units.Quantity("4 mm"))
        self.assertEqual(self.sheet.G3, Units.Quantity("6 mm"))
        self.assertEqual(self.sheet.G4, 3)
        self.assertEqual(self.sheet.G5, Units.Quantity("1 mm"))
        self.assertEqual(self.sheet.G6, Units.Quantity("15 mm"))

    def test_range_invalid(self):
        self.sheet.set("C13", "4mm")
        self.sheet.set("C14", "5mm")
        self.sheet.set("C15", "6mm")
        self.sheet.set("C16", "6")
        self.sheet.set("H1", "=average(C13:C16)")
        self.sheet.set("H2", "=min(C13:C16)")
        self.sheet.set("H3", "=max(C13:C16)")
        self.sheet.set("H4", "=count(C13:C16)")
        self.sheet.set("H5", "=stddev(C13:C16)")
        self.sheet.set("H6", "=sum(C13:C16)")

        self.doc.recompute()

        self.assertTrue(
            self.sheet.H1.startswith(
                "ERR: Quantity::operator +=(): Unit mismatch in plus operation"
            )
        )
        self.assertTrue(
            self.sheet.H2.startswith(
                "ERR: Quantity::operator <(): quantities need to have same unit to compare"
            )
        )
        self.assertTrue(
            self.sheet.H3.startswith(
                "ERR: Quantity::operator >(): quantities need to have same unit to compare"
            )
        )
        self.assertEqual(self.sheet.H4, 4)
        self.assertTrue(
            self.sheet.H5.startswith(
                "ERR: Quantity::operator -(): Unit mismatch in minus operation"
            )
        )
        self.assertTrue(
            self.sheet.H6.startswith(
                "ERR: Quantity::operator +=(): Unit mismatch in plus operation"
            )
        )

    def test_and(self):
        self.sheet.set("C20", "4")
        self.sheet.set("C21", "5")
        self.sheet.set("C22", "6")
        self.sheet.set("C23", "0")

        self.sheet.set("A1", "=and(1)")
        self.sheet.set("A2", "=and(1;2)")
        self.sheet.set("A3", "=and(1;2;3)")
        self.sheet.set("A4", "=and(1;2;3;C20)")
        self.sheet.set("A5", "=and(1;2;3;C20:C22)")
        self.sheet.set("A6", "=and(1;2;3;C20:C23)")

        self.sheet.set("B1", "=and(0)")
        self.sheet.set("B2", "=and(0;1;2)")
        self.sheet.set("B3", "=and(0;1;2;3)")
        self.sheet.set("B4", "=and(1;2;0)")
        self.sheet.set("B5", "=and(1;2;3;0)")
        self.sheet.set("B6", "=and(1;0;2)")
        self.sheet.set("B6", "=and(1;0;2;0;3)")

        self.doc.recompute()

        self.assertEqual(self.sheet.A1, 1)
        self.assertEqual(self.sheet.A2, 1)
        self.assertEqual(self.sheet.A3, 1)
        self.assertEqual(self.sheet.A4, 1)
        self.assertEqual(self.sheet.A5, 1)
        self.assertEqual(self.sheet.A6, 0)

        self.assertEqual(self.sheet.B1, 0)
        self.assertEqual(self.sheet.B2, 0)
        self.assertEqual(self.sheet.B3, 0)
        self.assertEqual(self.sheet.B4, 0)
        self.assertEqual(self.sheet.B5, 0)
        self.assertEqual(self.sheet.B6, 0)

    def test_or(self):
        self.sheet.set("C20", "4")
        self.sheet.set("C21", "5")
        self.sheet.set("C22", "6")
        self.sheet.set("C23", "0")
        self.sheet.set("C24", "0")

        self.sheet.set("A1", "=or(1)")
        self.sheet.set("A2", "=or(1;2)")
        self.sheet.set("A3", "=or(1;2;3)")
        self.sheet.set("A4", "=or(1;2;3;C20)")
        self.sheet.set("A5", "=or(1;2;3;C20:C22)")
        self.sheet.set("A6", "=or(1;2;3;C20:C23)")

        self.sheet.set("B1", "=or(0)")
        self.sheet.set("B2", "=or(0;1;2)")
        self.sheet.set("B3", "=or(0;1;2;3)")
        self.sheet.set("B4", "=or(1;2;0)")
        self.sheet.set("B5", "=or(1;2;3;0)")
        self.sheet.set("B6", "=or(1;0;2)")
        self.sheet.set("B6", "=or(1;0;2;0;3)")

        self.sheet.set("C1", "=or(0)")
        self.sheet.set("C2", "=or(0;0)")
        self.sheet.set("C3", "=or(0mm;0;0)")
        self.sheet.set("C4", "=or(0;0;0;C23)")
        self.sheet.set("C5", "=or(0;0;0;C23:C24)")
        self.sheet.set("C6", "=or(C23:C24)")
        self.sheet.set("C7", "=or(C22:C24)")

        self.doc.recompute()

        self.assertEqual(self.sheet.A1, 1)
        self.assertEqual(self.sheet.A2, 1)
        self.assertEqual(self.sheet.A3, 1)
        self.assertEqual(self.sheet.A4, 1)
        self.assertEqual(self.sheet.A5, 1)
        self.assertEqual(self.sheet.A6, 1)

        self.assertEqual(self.sheet.B1, 0)
        self.assertEqual(self.sheet.B2, 1)
        self.assertEqual(self.sheet.B3, 1)
        self.assertEqual(self.sheet.B4, 1)
        self.assertEqual(self.sheet.B5, 1)
        self.assertEqual(self.sheet.B6, 1)

        self.assertEqual(self.sheet.C1, 0)
        self.assertEqual(self.sheet.C2, 0)
        self.assertEqual(self.sheet.C3, 0)
        self.assertEqual(self.sheet.C4, 0)
        self.assertEqual(self.sheet.C5, 0)
        self.assertEqual(self.sheet.C6, 0)
        self.assertEqual(self.sheet.C7, 1)


#############################################################################################
class SpreadsheetFunction(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument()
        self.sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def assertMostlyEqual(self, a, b):
        if type(a) is Units.Quantity:
            self.assertTrue(math.fabs(a.Value - b.Value) < 1e-14)
            self.assertTrue(a.Unit == b.Unit)
        else:
            self.assertNotIsInstance(a, str)
            self.assertNotIsInstance(b, str)
            self.assertTrue(
                math.fabs(a - b) < 1e-14,
                "Values are not 'Mostly Equal': %s != %s" % (a, b),
            )

    def test_cos(self):
        self.sheet.set("A1", "=cos(60)")
        self.sheet.set("B1", "=cos(60deg)")
        self.sheet.set("C1", "=cos(pi / 2 * 1rad)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A1, 0.5)
        self.assertMostlyEqual(self.sheet.B1, 0.5)
        self.assertMostlyEqual(self.sheet.C1, 0)

    def test_sin(self):
        self.sheet.set("A2", "=sin(30)")
        self.sheet.set("B2", "=sin(30deg)")
        self.sheet.set("C2", "=sin(pi / 6 * 1rad)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A2, 0.5)
        self.assertMostlyEqual(self.sheet.B2, 0.5)
        self.assertMostlyEqual(self.sheet.C2, 0.5)

    def test_tan(self):
        self.sheet.set("A3", "=tan(45)")
        self.sheet.set("B3", "=tan(45deg)")
        self.sheet.set("C3", "=tan(pi / 4 * 1rad)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A3, 1)
        self.assertMostlyEqual(self.sheet.B3, 1)
        self.assertMostlyEqual(self.sheet.C3, 1)

    def test_abs(self):
        self.sheet.set("A4", "=abs(3)")
        self.sheet.set("B4", "=abs(-3)")
        self.sheet.set("C4", "=abs(-3mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A4, 3)
        self.assertMostlyEqual(self.sheet.B4, 3)
        self.assertMostlyEqual(self.sheet.C4, Units.Quantity("3 mm"))

    def test_exp(self):
        self.sheet.set("A5", "=exp(3)")
        self.sheet.set("B5", "=exp(-3)")
        self.sheet.set("C5", "=exp(-3mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A5, math.exp(3))
        self.assertMostlyEqual(self.sheet.B5, math.exp(-3))
        self.assertTrue(self.sheet.C5.startswith("ERR: Unit must be empty."))

    def test_log(self):
        self.sheet.set("A6", "=log(3)")
        self.sheet.set("B6", "=log(-3)")
        self.sheet.set("C6", "=log(-3mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A6, math.log(3))
        self.assertTrue(math.isnan(self.sheet.B6))
        self.assertTrue(self.sheet.C6.startswith("ERR: Unit must be empty."))

    def test_log10(self):
        self.sheet.set("A7", "=log10(10)")
        self.sheet.set("B7", "=log10(-3)")
        self.sheet.set("C7", "=log10(-3mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A7, math.log10(10))
        self.assertTrue(math.isnan(self.sheet.B7))
        self.assertTrue(self.sheet.C7.startswith("ERR: Unit must be empty."))

    def test_round(self):
        self.sheet.set("A8", "=round(3.4)")
        self.sheet.set("B8", "=round(3.6)")
        self.sheet.set("C8", "=round(-3.4)")
        self.sheet.set("D8", "=round(-3.6)")
        self.sheet.set("E8", "=round(3.4mm)")
        self.sheet.set("F8", "=round(3.6mm)")
        self.sheet.set("G8", "=round(-3.4mm)")
        self.sheet.set("H8", "=round(-3.6mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A8, 3)
        self.assertMostlyEqual(self.sheet.B8, 4)
        self.assertMostlyEqual(self.sheet.C8, -3)
        self.assertMostlyEqual(self.sheet.D8, -4)
        self.assertEqual(self.sheet.E8, Units.Quantity("3 mm"))
        self.assertEqual(self.sheet.F8, Units.Quantity("4 mm"))
        self.assertEqual(self.sheet.G8, Units.Quantity("-3 mm"))
        self.assertEqual(self.sheet.H8, Units.Quantity("-4 mm"))

    def test_trunc(self):
        self.sheet.set("A9", "=trunc(3.4)")
        self.sheet.set("B9", "=trunc(3.6)")
        self.sheet.set("C9", "=trunc(-3.4)")
        self.sheet.set("D9", "=trunc(-3.6)")
        self.sheet.set("E9", "=trunc(3.4mm)")
        self.sheet.set("F9", "=trunc(3.6mm)")
        self.sheet.set("G9", "=trunc(-3.4mm)")
        self.sheet.set("H9", "=trunc(-3.6mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A9, 3)
        self.assertMostlyEqual(self.sheet.B9, 3)
        self.assertMostlyEqual(self.sheet.C9, -3)
        self.assertMostlyEqual(self.sheet.D9, -3)
        self.assertEqual(self.sheet.E9, Units.Quantity("3 mm"))
        self.assertEqual(self.sheet.F9, Units.Quantity("3 mm"))
        self.assertEqual(self.sheet.G9, Units.Quantity("-3 mm"))
        self.assertEqual(self.sheet.H9, Units.Quantity("-3 mm"))

    def test_ceil(self):
        self.sheet.set("A10", "=ceil(3.4)")
        self.sheet.set("B10", "=ceil(3.6)")
        self.sheet.set("C10", "=ceil(-3.4)")
        self.sheet.set("D10", "=ceil(-3.6)")
        self.sheet.set("E10", "=ceil(3.4mm)")
        self.sheet.set("F10", "=ceil(3.6mm)")
        self.sheet.set("G10", "=ceil(-3.4mm)")
        self.sheet.set("H10", "=ceil(-3.6mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A10, 4)
        self.assertMostlyEqual(self.sheet.B10, 4)
        self.assertMostlyEqual(self.sheet.C10, -3)
        self.assertMostlyEqual(self.sheet.D10, -3)
        self.assertMostlyEqual(self.sheet.E10, Units.Quantity("4 mm"))
        self.assertMostlyEqual(self.sheet.F10, Units.Quantity("4 mm"))
        self.assertMostlyEqual(self.sheet.G10, Units.Quantity("-3 mm"))
        self.assertMostlyEqual(self.sheet.H10, Units.Quantity("-3 mm"))

    def test_floor(self):
        self.sheet.set("A11", "=floor(3.4)")
        self.sheet.set("B11", "=floor(3.6)")
        self.sheet.set("C11", "=floor(-3.4)")
        self.sheet.set("D11", "=floor(-3.6)")
        self.sheet.set("E11", "=floor(3.4mm)")
        self.sheet.set("F11", "=floor(3.6mm)")
        self.sheet.set("G11", "=floor(-3.4mm)")
        self.sheet.set("H11", "=floor(-3.6mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A11, 3)
        self.assertMostlyEqual(self.sheet.B11, 3)
        self.assertMostlyEqual(self.sheet.C11, -4)
        self.assertMostlyEqual(self.sheet.D11, -4)
        self.assertMostlyEqual(self.sheet.E11, Units.Quantity("3 mm"))
        self.assertMostlyEqual(self.sheet.F11, Units.Quantity("3 mm"))
        self.assertMostlyEqual(self.sheet.G11, Units.Quantity("-4 mm"))
        self.assertMostlyEqual(self.sheet.H11, Units.Quantity("-4 mm"))

    def test_asin(self):
        self.sheet.set("A12", "=asin(0.5)")
        self.sheet.set("B12", "=asin(0.5mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A12, Units.Quantity("30 deg"))
        self.assertTrue(self.sheet.B12.startswith("ERR: Unit must be empty."))

    def test_acos(self):
        self.sheet.set("A13", "=acos(0.5)")
        self.sheet.set("B13", "=acos(0.5mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A13, Units.Quantity("60 deg"))
        self.assertTrue(self.sheet.B13.startswith("ERR: Unit must be empty."))

    def test_atan(self):
        self.sheet.set("A14", "=atan(sqrt(3))")
        self.sheet.set("B14", "=atan(0.5mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A14, Units.Quantity("60 deg"))
        self.assertTrue(self.sheet.B14.startswith("ERR: Unit must be empty."))

    def test_asinh(self):
        self.sheet.set("A15", "=sinh(0.5)")
        self.sheet.set("B15", "=sinh(0.5mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A15, math.sinh(0.5))
        self.assertTrue(self.sheet.B15.startswith("ERR: Unit must be empty."))

    def test_cosh(self):
        self.sheet.set("A16", "=cosh(0.5)")
        self.sheet.set("B16", "=cosh(0.5mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A16, math.cosh(0.5))
        self.assertTrue(self.sheet.B16.startswith("ERR: Unit must be empty."))

    def test_tanh(self):
        self.sheet.set("A17", "=tanh(0.5)")
        self.sheet.set("B17", "=tanh(0.5mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A17, math.tanh(0.5))
        self.assertTrue(self.sheet.B17.startswith("ERR: Unit must be empty."))

    def test_sqrt(self):
        self.sheet.set("A18", "=sqrt(4)")
        self.sheet.set("B18", "=sqrt(4mm^2)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A18, 2)
        self.assertMostlyEqual(self.sheet.B18, Units.Quantity("2 mm"))

    def test_mod(self):
        self.sheet.set("A19", "=mod(7; 4)")
        self.sheet.set("B19", "=mod(-7; 4)")
        self.sheet.set("C19", "=mod(7mm; 4)")
        self.sheet.set("D19", "=mod(7mm; 4mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A19, 3)
        self.assertMostlyEqual(self.sheet.B19, -3)
        self.assertMostlyEqual(self.sheet.C19, Units.Quantity("3 mm"))
        self.assertEqual(self.sheet.D19, 3)

    def test_atan2(self):
        self.sheet.set("A20", "=atan2(3; 3)")
        self.sheet.set("B20", "=atan2(-3; 3)")
        self.sheet.set("C20", "=atan2(3mm; 3)")
        self.sheet.set("D20", "=atan2(3mm; 3mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A20, Units.Quantity("45 deg"))
        self.assertMostlyEqual(self.sheet.B20, Units.Quantity("-45 deg"))
        self.assertTrue(self.sheet.C20.startswith("ERR: Units must be equal"))
        self.assertMostlyEqual(self.sheet.D20, Units.Quantity("45 deg"))

    def test_pow(self):
        self.sheet.set("A21", "=pow(7; 4)")
        self.sheet.set("B21", "=pow(-7; 4)")
        self.sheet.set("C21", "=pow(7mm; 4)")
        self.sheet.set("D21", "=pow(7mm; 4mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A21, 2401)
        self.assertMostlyEqual(self.sheet.B21, 2401)
        self.assertMostlyEqual(self.sheet.C21, Units.Quantity("2401mm^4"))
        self.assertTrue(self.sheet.D21.startswith("ERR: Exponent is not allowed to have a unit."))

    def test_hypot(self):
        self.sheet.set("A23", "=hypot(3; 4)")
        self.sheet.set("B23", "=hypot(-3; 4)")
        self.sheet.set("C23", "=hypot(3mm; 4)")
        self.sheet.set("D23", "=hypot(3mm; 4mm)")
        self.sheet.set("A24", "=hypot(3; 4; 5)")
        self.sheet.set("B24", "=hypot(-3; 4; 5)")
        self.sheet.set("C24", "=hypot(3mm; 4; 5)")
        self.sheet.set("D24", "=hypot(3mm; 4mm; 5mm)")

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A23, 5)
        self.assertMostlyEqual(self.sheet.B23, 5)
        self.assertTrue(self.sheet.C23.startswith("ERR: Units must be equal"))
        self.assertMostlyEqual(self.sheet.D23, Units.Quantity("5mm"))
        self.assertMostlyEqual(self.sheet.A24, math.sqrt(3 * 3 + 4 * 4 + 5 * 5))
        self.assertMostlyEqual(self.sheet.B24, math.sqrt(3 * 3 + 4 * 4 + 5 * 5))
        self.assertTrue(self.sheet.C24.startswith("ERR: Units must be equal"))
        self.assertMostlyEqual(self.sheet.D24, Units.Quantity("7.07106781186548 mm"))

    def test_cath(self):
        self.sheet.set("A26", "=cath(5; 3)")
        self.sheet.set("B26", "=cath(-5; 3)")
        self.sheet.set("C26", "=cath(5mm; 3)")
        self.sheet.set("D26", "=cath(5mm; 3mm)")
        l = math.sqrt(5 * 5 + 4 * 4 + 3 * 3)
        self.sheet.set("A27", "=cath(%0.15f; 5; 4)" % l)
        self.sheet.set("B27", "=cath(%0.15f; -5; 4)" % l)
        self.sheet.set("C27", "=cath(%0.15f mm; 5mm; 4)" % l)
        self.sheet.set("D27", "=cath(%0.15f mm; 5mm; 4mm)" % l)

        self.doc.recompute()

        self.assertMostlyEqual(self.sheet.A26, 4)
        self.assertMostlyEqual(self.sheet.B26, 4)
        self.assertTrue(self.sheet.C26.startswith("ERR: Units must be equal"))
        self.assertMostlyEqual(self.sheet.D26, Units.Quantity("4mm"))
        self.assertMostlyEqual(self.sheet.A27, math.sqrt(l * l - 5 * 5 - 4 * 4))
        self.assertMostlyEqual(self.sheet.B27, math.sqrt(l * l - 5 * 5 - 4 * 4))
        self.assertTrue(self.sheet.C27.startswith("ERR: Units must be equal"))
        self.assertMostlyEqual(self.sheet.D27, Units.Quantity("3 mm"))

    def test_not(self):
        self.sheet.set("A20", "=not(3)")
        self.sheet.set("B20", "=not(-3)")
        self.sheet.set("C20", "=not(-3.5)")
        self.sheet.set("D20", "=not(3mm)")
        self.sheet.set("E20", "=not(3.5mm)")
        self.sheet.set("F20", "=not(-3.5mm)")
        self.sheet.set("G20", "=not(0)")
        self.sheet.set("H20", "=not(0mm)")
        self.sheet.set("I20", "=not(1)")

        self.doc.recompute()

        self.assertEqual(self.sheet.A20, 0)
        self.assertEqual(self.sheet.B20, 0)
        self.assertEqual(self.sheet.C20, 0)
        self.assertEqual(self.sheet.D20, 0)
        self.assertEqual(self.sheet.E20, 0)
        self.assertEqual(self.sheet.F20, 0)
        self.assertEqual(self.sheet.G20, 1)
        self.assertEqual(self.sheet.H20, 1)
        self.assertEqual(self.sheet.I20, 0)

        self.sheet.set("J21", f"=not(not({1e-7}))")
        self.sheet.set("J22", f"=not(not({1e-8}))")
        self.doc.recompute()
        self.assertEqual(self.sheet.J21, 1)
        self.assertEqual(self.sheet.J22, 0)


#############################################################################################
class SpreadsheetCases(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument()
        self.TempPath = tempfile.gettempdir()
        FreeCAD.Console.PrintLog("  Using temp path: " + self.TempPath + "\n")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

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
        sheet.set("A7", "=5mm^2")  # ^2 operates on whole number
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
            return (plm1.Base - plm2.Base).Length < 1e-7 and (qdiff1 < 1e-12 or qdiff2 < 1e-12)

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

    def testIssue19517(self):
        """Regression test for issue 19517; mod should work with units"""
        sheet = self.doc.addObject("Spreadsheet::Sheet", "Spreadsheet")
        sheet.set("A1", "=mod(7mm;3mm)")
        self.doc.recompute()
        self.assertEqual(sheet.A1, Units.Quantity("1 mm"))
        try:
            sheet.set("A2", "=mod(7kg;3mm)")
            self.doc.recompute()
            self.fail("Units need to be the same or dimensionless")
        except Exception:
            pass

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

    def test_cross_link_empty_property_name(self):
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

        base_path = self.TempPath + os.sep + "base.FCStd"
        base.saveAs(base_path)
        square_path = self.TempPath + os.sep + "square.FCStd"
        square.saveAs(square_path)

        base.save()
        square.save()

        FreeCAD.closeDocument(square.Name)
        FreeCAD.closeDocument(base.Name)

        ##
        ## preparation done
        base = FreeCAD.openDocument(base_path)
        square = FreeCAD.openDocument(square_path)

        square.Box.setExpression("Length", "base#Spreadsheet.x")
        square.recompute()

        square.save()
        base.save()
        FreeCAD.closeDocument(square.Name)
        FreeCAD.closeDocument(base.Name)

    def test_expression_with_alias(self):
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
        self.assertEqual(used_range, ("C3", "Z20"))

        for i, cell in enumerate(test_cells):
            sheet.set(cell, "")
            sheet.setAlignment(cell, "center")
        used_range = sheet.getUsedRange()
        self.assertEqual(used_range, ("C3", "Z20"))

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
        self.assertEqual(non_empty_range, ("C3", "Z20"))

        for i, cell in enumerate(test_cells):
            sheet.set(cell, "")
            sheet.setAlignment(cell, "center")
        more_cells = ["D10", "X5", "E10", "K15"]
        for i, cell in enumerate(more_cells):
            sheet.set(cell, str(i))
        non_empty_range = sheet.getNonEmptyRange()
        self.assertEqual(non_empty_range, ("D5", "X15"))

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
