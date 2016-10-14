# (c) 2016 Werner Mayer
# (c) 2016 Eivind Kvedalen
# LGPL

import os
import sys
import math
import unittest
import FreeCAD
import Part
import Sketcher
import tempfile
from FreeCAD import Base
from Units import Unit,Quantity

v = Base.Vector

#----------------------------------------------------------------------------------
# define the functions to test the FreeCAD Spreadsheet module and expression engine
#----------------------------------------------------------------------------------


class SpreadsheetCases(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument()
        self.TempPath = tempfile.gettempdir()
        FreeCAD.Console.PrintLog( '  Using temp path: ' + self.TempPath + '\n')

    def testAggregates(self):
        """ Test all aggregate functions """
        sheet = self.doc.addObject('Spreadsheet::Sheet','Spreadsheet')
        sheet.set('B3',  '4')
        sheet.set('B4',  '5')
        sheet.set('B5',  '6')
        sheet.set('A1',  '=sum(1)')
        sheet.set('A2',  '=sum(1;2)')
        sheet.set('A3',  '=sum(1;2;3)')
        sheet.set('A4',  '=sum(1;2;3;B3)')
        sheet.set('A5',  '=sum(1;2;3;B3:B5)')
        sheet.set('A6',  '=stddev(1;2;3)')
        sheet.set('A7',  '=average(1;2;3)')
        sheet.set('A8',  '=count(1;2;3)')
        sheet.set('A9',  '=min(1;2;3)')
        sheet.set('A10', '=max(1;2;3)')
        self.doc.recompute()
        self.assertEqual(sheet.A1, 1)
        self.assertEqual(sheet.A2, 3)
        self.assertEqual(sheet.A3, 6)
        self.assertEqual(sheet.A4, 10)
        self.assertEqual(sheet.A5, 21)
        self.assertEqual(sheet.A6, 1)
        self.assertEqual(sheet.A7, 2)
        self.assertEqual(sheet.A8, 3)
        self.assertEqual(sheet.A9, 1)
        self.assertEqual(sheet.A10, 3)

    def assertMostlyEqual(self, a, b):
        if type(a) is Quantity:
            self.assertTrue( math.fabs(a.Value - b.Value) < 1e-14)
            self.assertTrue( a.Unit == b.Unit)
        else:
            self.assertTrue( math.fabs(a - b) < 1e-14)

    def testFunctions(self):
        """ Test all built-in simple functions """
        doc = FreeCAD.newDocument()
        sheet = self.doc.addObject('Spreadsheet::Sheet','Spreadsheet')
        sheet.set('A1',  '=cos(60)')   # Cos
        sheet.set('B1',  '=cos(60deg)')
        sheet.set('C1',  '=cos(pi / 2 * 1rad)')
        sheet.set('A2',  '=sin(30)')   # Sin
        sheet.set('B2',  '=sin(30deg)')
        sheet.set('C2',  '=sin(pi / 6 * 1rad)')
        sheet.set('A3',  '=tan(45)')   # Tan
        sheet.set('B3',  '=tan(45deg)')
        sheet.set('C3',  '=tan(pi / 4 * 1rad)')
        sheet.set('A4',  '=abs(3)')    # Abs
        sheet.set('B4',  '=abs(-3)')
        sheet.set('C4',  '=abs(-3mm)')
        sheet.set('A5',  '=exp(3)')    # Exp
        sheet.set('B5',  '=exp(-3)')
        sheet.set('C5',  '=exp(-3mm)')
        sheet.set('A6',  '=log(3)')    # Log
        sheet.set('B6',  '=log(-3)')
        sheet.set('C6',  '=log(-3mm)')
        sheet.set('A7',  '=log10(10)')  # Log10
        sheet.set('B7',  '=log10(-3)')
        sheet.set('C7',  '=log10(-3mm)')
        sheet.set('A8',  '=round(3.4)')# Round
        sheet.set('B8',  '=round(3.6)')
        sheet.set('C8',  '=round(-3.4)')
        sheet.set('D8',  '=round(-3.6)')
        sheet.set('E8',  '=round(3.4mm)')
        sheet.set('F8',  '=round(3.6mm)')
        sheet.set('G8',  '=round(-3.4mm)')
        sheet.set('H8',  '=round(-3.6mm)')
        sheet.set('A9',  '=trunc(3.4)')# Trunc
        sheet.set('B9',  '=trunc(3.6)')
        sheet.set('C9',  '=trunc(-3.4)')
        sheet.set('D9',  '=trunc(-3.6)')
        sheet.set('E9',  '=trunc(3.4mm)')
        sheet.set('F9',  '=trunc(3.6mm)')
        sheet.set('G9',  '=trunc(-3.4mm)')
        sheet.set('H9',  '=trunc(-3.6mm)')
        sheet.set('A10',  '=ceil(3.4)') # Ceil
        sheet.set('B10',  '=ceil(3.6)')
        sheet.set('C10',  '=ceil(-3.4)')
        sheet.set('D10',  '=ceil(-3.6)')
        sheet.set('E10',  '=ceil(3.4mm)')
        sheet.set('F10',  '=ceil(3.6mm)')
        sheet.set('G10',  '=ceil(-3.4mm)')
        sheet.set('H10',  '=ceil(-3.6mm)')
        sheet.set('A11', '=floor(3.4)')# Floor
        sheet.set('B11', '=floor(3.6)')
        sheet.set('C11', '=floor(-3.4)')
        sheet.set('D11', '=floor(-3.6)')
        sheet.set('E11', '=floor(3.4mm)')
        sheet.set('F11', '=floor(3.6mm)')
        sheet.set('G11', '=floor(-3.4mm)')
        sheet.set('H11', '=floor(-3.6mm)')
        sheet.set('A12', '=asin(0.5)') # Asin
        sheet.set('B12', '=asin(0.5mm)')
        sheet.set('A13', '=acos(0.5)') # Acos
        sheet.set('B13', '=acos(0.5mm)')
        sheet.set('A14', '=atan(sqrt(3))') # Atan
        sheet.set('B14', '=atan(0.5mm)')
        sheet.set('A15', '=sinh(0.5)') # Sinh
        sheet.set('B15', '=sinh(0.5mm)')
        sheet.set('A16', '=cosh(0.5)') # Cosh
        sheet.set('B16', '=cosh(0.5mm)')
        sheet.set('A17', '=tanh(0.5)') # Tanh
        sheet.set('B17', '=tanh(0.5mm)')
        sheet.set('A18', '=sqrt(4)')   # Sqrt
        sheet.set('B18', '=sqrt(4mm^2)')
        sheet.set('A19', '=mod(7; 4)') # Mod
        sheet.set('B19', '=mod(-7; 4)')
        sheet.set('C19', '=mod(7mm; 4)')
        sheet.set('D19', '=mod(7mm; 4mm)')
        sheet.set('A20', '=atan2(3; 3)')       # Atan2
        sheet.set('B20', '=atan2(-3; 3)')
        sheet.set('C20', '=atan2(3mm; 3)')
        sheet.set('D20', '=atan2(3mm; 3mm)')
        sheet.set('A21', '=pow(7; 4)') # Pow
        sheet.set('B21', '=pow(-7; 4)')
        sheet.set('C21', '=pow(7mm; 4)')
        sheet.set('D21', '=pow(7mm; 4mm)')
        self.doc.recompute()
        self.assertMostlyEqual(sheet.A1,  0.5)   # Cos
        self.assertMostlyEqual(sheet.B1,  0.5)
        self.assertMostlyEqual(sheet.C1,  0)
        self.assertMostlyEqual(sheet.A2,  0.5)   # Sin
        self.assertMostlyEqual(sheet.B2,  0.5)
        self.assertMostlyEqual(sheet.C2,  0.5)
        self.assertMostlyEqual(sheet.A3,  1)   # Tan
        self.assertMostlyEqual(sheet.B3,  1)
        self.assertMostlyEqual(sheet.C3,  1 )
        self.assertMostlyEqual(sheet.A4,  3)    # Abs
        self.assertMostlyEqual(sheet.B4,  3)
        self.assertMostlyEqual(sheet.C4,  Quantity('3 mm'))
        self.assertMostlyEqual(sheet.A5,  math.exp(3))  # Exp
        self.assertMostlyEqual(sheet.B5,  math.exp(-3))
        self.assertEqual(sheet.C5,  u'ERR: Unit must be empty.')
        self.assertMostlyEqual(sheet.A6,  math.log(3))    # Log
        self.assertTrue(math.isnan(sheet.B6))
        self.assertEqual(sheet.C6,  u'ERR: Unit must be empty.')
        self.assertMostlyEqual(sheet.A7,  math.log10(10))  # Log10
        self.assertTrue(math.isnan(sheet.B7))
        self.assertEqual(sheet.C7,  u'ERR: Unit must be empty.')
        self.assertMostlyEqual(sheet.A8,  3) # Round
        self.assertMostlyEqual(sheet.B8,  4)
        self.assertMostlyEqual(sheet.C8,  -3)
        self.assertMostlyEqual(sheet.D8,  -4)
        self.assertEqual(sheet.E8,  Quantity('3 mm'))
        self.assertEqual(sheet.F8,  Quantity('4 mm'))
        self.assertEqual(sheet.G8,  Quantity('-3 mm'))
        self.assertEqual(sheet.H8,  Quantity('-4 mm'))
        self.assertMostlyEqual(sheet.A9,  3)# Trunc
        self.assertMostlyEqual(sheet.B9,  3)
        self.assertMostlyEqual(sheet.C9,  -3)
        self.assertMostlyEqual(sheet.D9,  -3)
        self.assertEqual(sheet.E9,  Quantity('3 mm'))
        self.assertEqual(sheet.F9,  Quantity('3 mm'))
        self.assertEqual(sheet.G9,  Quantity('-3 mm'))
        self.assertEqual(sheet.H9,  Quantity('-3 mm'))
        self.assertMostlyEqual(sheet.A10, 4) # Ceil
        self.assertMostlyEqual(sheet.B10, 4)
        self.assertMostlyEqual(sheet.C10, -3)
        self.assertMostlyEqual(sheet.D10, -3)
        self.assertMostlyEqual(sheet.E10, Quantity('4 mm'))
        self.assertMostlyEqual(sheet.F10, Quantity('4 mm'))
        self.assertMostlyEqual(sheet.G10, Quantity('-3 mm'))
        self.assertMostlyEqual(sheet.H10, Quantity('-3 mm'))
        self.assertMostlyEqual(sheet.A11, 3)# Floor
        self.assertMostlyEqual(sheet.B11, 3)
        self.assertMostlyEqual(sheet.C11, -4)
        self.assertMostlyEqual(sheet.D11, -4)
        self.assertMostlyEqual(sheet.E11, Quantity('3 mm'))
        self.assertMostlyEqual(sheet.F11, Quantity('3 mm'))
        self.assertMostlyEqual(sheet.G11, Quantity('-4 mm'))
        self.assertMostlyEqual(sheet.H11, Quantity('-4 mm'))
        self.assertMostlyEqual(sheet.A12, Quantity('30 deg')) # Asin
        self.assertEqual(sheet.B12, u'ERR: Unit must be empty.')
        self.assertMostlyEqual(sheet.A13, Quantity('60 deg')) # Acos
        self.assertEqual(sheet.B13, u'ERR: Unit must be empty.')
        self.assertMostlyEqual(sheet.A14, Quantity('60 deg')) # Atan
        self.assertEqual(sheet.B14, u'ERR: Unit must be empty.')
        self.assertMostlyEqual(sheet.A15, math.sinh(0.5)) # Sinh
        self.assertEqual(sheet.B15, u'ERR: Unit must be empty.')
        self.assertMostlyEqual(sheet.A16, math.cosh(0.5)) # Cosh
        self.assertEqual(sheet.B16, u'ERR: Unit must be empty.')
        self.assertMostlyEqual(sheet.A17, math.tanh(0.5)) # Tanh
        self.assertEqual(sheet.B17, u'ERR: Unit must be empty.')
        self.assertMostlyEqual(sheet.A18, 2)   # Sqrt
        self.assertMostlyEqual(sheet.B18, Quantity('2 mm'))
        self.assertMostlyEqual(sheet.A19, 3) # Mod
        self.assertMostlyEqual(sheet.B19, -3)
        self.assertMostlyEqual(sheet.C19, Quantity('3 mm'))
        self.assertEqual(sheet.D19, u'ERR: Second argument must have empty unit.')
        self.assertMostlyEqual(sheet.A20, Quantity('45 deg'))       # Atan2
        self.assertMostlyEqual(sheet.B20, Quantity('-45 deg'))
        self.assertEqual(sheet.C20, u'ERR: Units must be equal')
        self.assertMostlyEqual(sheet.D20, Quantity('45 deg'))
        self.assertMostlyEqual(sheet.A21, 2401) # Pow
        self.assertMostlyEqual(sheet.B21, 2401)
        self.assertMostlyEqual(sheet.C21, Quantity('2401mm^4'))
        self.assertEqual(sheet.D21, u'ERR: Exponent is not allowed to have a unit.')
        
    def testRelationalOperators(self):
        """ Test relational operators """
        sheet = self.doc.addObject('Spreadsheet::Sheet','Spreadsheet')
        # All should be 1 as result
        sheet.set('A1',  '=1 == 1     ? 1 : 0')
        sheet.set('A2',  '=1 != 1     ? 0 : 1')
        sheet.set('A3',  '=1e9 == 1e9 ? 1 : 0')
        sheet.set('A4',  '=1e9 != 1e9 ? 0 : 1')
        sheet.set('A5',  '=1 > 1      ? 0 : 1')
        sheet.set('A6',  '=2 > 1      ? 1 : 0')
        sheet.set('A7',  '=1 > 2      ? 0 : 1')
        sheet.set('A8',  '=1 < 1      ? 0 : 1')
        sheet.set('A9',  '=1 < 2      ? 1 : 0')
        sheet.set('A10', '=2 < 1      ? 0 : 1')
        sheet.set('A11', '=1 >= 1     ? 1 : 0')
        sheet.set('A12', '=2 >= 1     ? 1 : 0')
        sheet.set('A13', '=1 >= 2     ? 0 : 1')
        sheet.set('A14', '=1 <= 1     ? 1 : 1')
        sheet.set('A15', '=1 <= 2     ? 1 : 0')
        sheet.set('A16', '=2 <= 1     ? 0 : 1')
        sheet.set('A17', '=1 >= 1.000000000000001 ? 0 : 1')
        sheet.set('A18', '=1 >= 1.0000000000000001 ? 1 : 0')
        sheet.set('A19', '=1 <= 1.000000000000001 ? 1 : 0')
        sheet.set('A20', '=1 <= 1.0000000000000001 ? 1 : 0')
        sheet.set('A21', '=1 == 1.000000000000001 ? 0 : 1')
        sheet.set('A22', '=1 == 1.0000000000000001 ? 1 : 0')
        sheet.set('A23', '=1 != 1.000000000000001 ? 1 : 0')
        sheet.set('A24', '=1 != 1.0000000000000001 ? 0 : 1')

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
        """ Units -- test unit calculations. """
        sheet = self.doc.addObject('Spreadsheet::Sheet','Spreadsheet')
        sheet.set('A1', '=2mm + 3mm')
        sheet.set('A2', '=2mm - 3mm')
        sheet.set('A3', '=2mm * 3mm')
        sheet.set('A4', '=4mm / 2mm')
        sheet.set('A5', '=(4mm)^2')
        sheet.set('A6', '=5(mm^2)')
        sheet.set('A7', '=5mm^2') #  ^2 operates on whole number
        sheet.set('A8', '=5')
        sheet.set('A9', '=5*1/K') # Currently fails
        sheet.set('A10', '=5 K^-1') # Currently fails
        sheet.set('A11', '=9.8 m/s^2') # Currently fails
        sheet.setDisplayUnit('A8', '1/K')
        self.doc.recompute()
        self.assertEqual(sheet.A1, Quantity('5mm'))
        self.assertEqual(sheet.A2, Quantity('-1 mm'))
        self.assertEqual(sheet.A3, Quantity('6 mm^2'))
        self.assertEqual(sheet.A4, Quantity('2'))
        self.assertEqual(sheet.A5, Quantity('16 mm^2'))
        self.assertEqual(sheet.A6, Quantity('5 mm^2'))
        self.assertEqual(sheet.A7, Quantity('5 mm^2'))
        self.assertEqual(sheet.A8, Quantity('5'))
        self.assertEqual(sheet.A9, Quantity('5 K^-1'))
        self.assertEqual(sheet.A10, Quantity('5 K^-1'))
        self.assertEqual(sheet.A11, Quantity('9.8 m/s^2'))

    def testPrecedence(self):
        """ Precedence -- test precedence for relational operators and conditional operator. """
        sheet = self.doc.addObject('Spreadsheet::Sheet','Spreadsheet')
        sheet.set('A1', '=1 < 2 ? 3 : 4')
        sheet.set('A2', '=1 + 2 < 3 + 4 ? 5 + 6 : 7 + 8')
        sheet.set('A3', '=1 + 2 * 1 < 3 + 4 ? 5 * 2 + 6 * 3 + 2 ^ 4 : 7 * 2 + 8 * 3 + 2 ^ 3')
        sheet.set('A4', '=123')
        sheet.set('A5', '=123 + 321')
        sheet.set('A6', '=123 * 2 + 321')
        sheet.set('A7', '=123 * 2 + 333 / 3')
        sheet.set('A8', '=123 * (2 + 321)')
        sheet.set('A9', '=3 ^ 4')
        sheet.set('A10', '=3 ^ 4 * 2')
        sheet.set('A11', '=3 ^ (4 * 2)')
        sheet.set('A12', '=3 ^ 4 + 4')
        sheet.set('A13', '=1 + 4 / 2 + 5')
        sheet.set('A14', '=(3 + 6) / (1 + 2)')
        sheet.set('A15', '=1 * 2 / 3 * 4')
        sheet.set('A16', '=(1 * 2) / (3 * 4)')
        # Test associativity
        sheet.set('A17', '=3 ^ 4 ^ 2') # exponentiation is left-associative; to follow excel, openoffice, matlab, octave
        sheet.set('A18', '=3 ^ (4 ^ 2)') # exponentiation is left-associative
        sheet.set('A19', '=(3 ^ 4) ^ 2') # exponentiation is left-associative
        sheet.set('A20', '=3 + 4 + 2')
        sheet.set('A21', '=3 + (4 + 2)')
        sheet.set('A22', '=(3 + 4) + 2')
        sheet.set('A23', '=3 - 4 - 2')
        sheet.set('A24', '=3 - (4 - 2)')
        sheet.set('A25', '=(3 - 4) - 2')
        sheet.set('A26', '=3 * 4 * 2')
        sheet.set('A27', '=3 * (4 * 2)')
        sheet.set('A28', '=(3 * 4) * 2')
        sheet.set('A29', '=3 / 4 / 2')
        sheet.set('A30', '=3 / (4 / 2)')
        sheet.set('A31', '=(3 / 4) / 2')
        sheet.set('A32', '=pi * 3')
        sheet.set('A33', '=A32 / 3')
        sheet.set('A34', '=1 < 2 ? <<A>> : <<B>>')
        sheet.set('A35', '=min(A32:A33)')
        sheet.set('A36', '=(1 < 2 ? 0 : 1) * 3')
        self.doc.recompute()
        self.assertEqual(sheet.getContents("A1"), "=1 < 2 ? 3 : 4")
        self.assertEqual(sheet.getContents("A2"), "=1 + 2 < 3 + 4 ? 5 + 6 : 7 + 8")
        self.assertEqual(sheet.getContents("A3"), "=1 + 2 * 1 < 3 + 4 ? 5 * 2 + 6 * 3 + 2 ^ 4 : 7 * 2 + 8 * 3 + 2 ^ 3")
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
        self.assertEqual(sheet.A15, 8.0/3)
        self.assertEqual(sheet.A16, 1.0/6)
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
        self.assertEqual(sheet.A29, 3.0/8)
        self.assertEqual(sheet.A30, 3.0/2)
        self.assertEqual(sheet.A31, 3.0/8)
        self.assertEqual(sheet.getContents('A1'), '=1 < 2 ? 3 : 4')
        self.assertEqual(sheet.getContents('A2'), '=1 + 2 < 3 + 4 ? 5 + 6 : 7 + 8')
        self.assertEqual(sheet.getContents('A3'), '=1 + 2 * 1 < 3 + 4 ? 5 * 2 + 6 * 3 + 2 ^ 4 : 7 * 2 + 8 * 3 + 2 ^ 3')
        self.assertEqual(sheet.getContents('A4'), '123')
        self.assertEqual(sheet.getContents('A5'), '=123 + 321')
        self.assertEqual(sheet.getContents('A6'), '=123 * 2 + 321')
        self.assertEqual(sheet.getContents('A7'), '=123 * 2 + 333 / 3')
        self.assertEqual(sheet.getContents('A8'), '=123 * (2 + 321)')
        self.assertEqual(sheet.getContents('A9'), '=3 ^ 4')
        self.assertEqual(sheet.getContents('A10'), '=3 ^ 4 * 2')
        self.assertEqual(sheet.getContents('A11'), '=3 ^ (4 * 2)')
        self.assertEqual(sheet.getContents('A12'), '=3 ^ 4 + 4')
        self.assertEqual(sheet.getContents('A13'), '=1 + 4 / 2 + 5')
        self.assertEqual(sheet.getContents('A14'), '=(3 + 6) / (1 + 2)')
        self.assertEqual(sheet.getContents('A15'), '=1 * 2 / 3 * 4')
        self.assertEqual(sheet.getContents('A16'), '=1 * 2 / 3 * 4')
        self.assertEqual(sheet.getContents('A17'), '=3 ^ 4 ^ 2')
        self.assertEqual(sheet.getContents('A18'), '=3 ^ (4 ^ 2)')
        self.assertEqual(sheet.getContents('A19'), '=3 ^ 4 ^ 2')
        self.assertEqual(sheet.getContents('A20'), '=3 + 4 + 2')
        self.assertEqual(sheet.getContents('A21'), '=3 + 4 + 2')
        self.assertEqual(sheet.getContents('A22'), '=3 + 4 + 2')
        self.assertEqual(sheet.getContents('A23'), '=3 - 4 - 2')
        self.assertEqual(sheet.getContents('A24'), '=3 - (4 - 2)')
        self.assertEqual(sheet.getContents('A25'), '=3 - 4 - 2')
        self.assertEqual(sheet.getContents('A26'), '=3 * 4 * 2')
        self.assertEqual(sheet.getContents('A27'), '=3 * 4 * 2')
        self.assertEqual(sheet.getContents('A28'), '=3 * 4 * 2')
        self.assertEqual(sheet.getContents('A29'), '=3 / 4 / 2')
        self.assertEqual(sheet.getContents('A30'), '=3 / (4 / 2)')
        self.assertEqual(sheet.getContents('A31'), '=3 / 4 / 2')
        self.assertEqual(sheet.getContents('A32'), '=pi * 3')
        self.assertEqual(sheet.getContents('A33'), '=A32 / 3')
        self.assertEqual(sheet.getContents('A34'), '=1 < 2 ? <<A>> : <<B>>')
        self.assertEqual(sheet.getContents('A35'), '=min(A32:A33)')
        self.assertEqual(sheet.getContents('A36'), '=(1 < 2 ? 0 : 1) * 3')

    def testNumbers(self):
        """ Test different numbers """
        sheet = self.doc.addObject('Spreadsheet::Sheet','Spreadsheet')
        sheet.set('A1', '1')
        sheet.set('A2', '1.5')
        sheet.set('A3', '.5')
        sheet.set('A4', '1e2')
        sheet.set('A5', '1E2')
        sheet.set('A6', '1e-2')
        sheet.set('A7', '1E-2')
        sheet.set('A8', '1.5e2')
        sheet.set('A9', '1.5E2')
        sheet.set('A10', '1.5e-2')
        sheet.set('A11', '1.5E-2')
        sheet.set('A12', '.5e2')
        sheet.set('A13', '.5E2')
        sheet.set('A14', '.5e-2')
        sheet.set('A15', '.5E-2')
        sheet.set('A16', '1/1')
        sheet.set('A17', '1/2')
        sheet.set('A18', '2/4')
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
        
    def testRemoveRows(self):
        """ Removing rows -- check renaming of internal cells """
        sheet = self.doc.addObject('Spreadsheet::Sheet','Spreadsheet')
        sheet.set('A3', '123')
        sheet.set('A1', '=A3')
        sheet.removeRows('2', 1)
        self.assertEqual(sheet.getContents("A1"),"=A2")

    def testInsertRows(self):
        """ Inserting rows -- check renaming of internal cells """
        sheet = self.doc.addObject('Spreadsheet::Sheet','Spreadsheet')
        sheet.set('B1', '=B2')
        sheet.set('B2', '124')
        sheet.insertRows('2', 1)
        self.assertEqual(sheet.getContents("B1"),"=B3")

    def testRenameAlias(self):
        """ Test renaming of alias1 to alias2 in a spreadsheet """
        sheet = self.doc.addObject('Spreadsheet::Sheet','Spreadsheet')
        sheet.set('B1', '124')
        sheet.setAlias('B1', 'alias1')
        sheet.set('B2', '=alias1')
        self.doc.recompute()
        self.assertEqual(sheet.get("alias1"), 124)
        self.assertEqual(sheet.get("B1"), 124)
        self.assertEqual(sheet.get("B2"), 124)
        sheet.setAlias('B1', 'alias2')
        self.doc.recompute()
        self.assertEqual(sheet.get("alias2"), 124)
        self.assertEqual(sheet.getContents("B2"),"=alias2")

    def testRenameAlias2(self):
        """ Test renaming of alias1 to alias2 in a spreadsheet, when referenced from another object """
        sheet = self.doc.addObject('Spreadsheet::Sheet','Spreadsheet')
        sheet.set('B1', '124')
        sheet.setAlias('B1', 'alias1')
        box = self.doc.addObject('Part::Box', 'Box')
        box.setExpression('Length', 'Spreadsheet.alias1')
        sheet.setAlias('B1', 'alias2')
        self.assertEqual(box.ExpressionEngine[0][1], "Spreadsheet.alias2");

    def testRenameAlias3(self):
        """ Test renaming of document object referenced from another object """
        sheet = self.doc.addObject('Spreadsheet::Sheet','Spreadsheet')
        sheet.set('B1', '124')
        sheet.setAlias('B1', 'alias1')
        box = self.doc.addObject('Part::Box', 'Box')
        box.setExpression('Length', 'Spreadsheet.alias1')
        sheet.Label = "Params"
        self.assertEqual(box.ExpressionEngine[0][1], "Params.alias1");

    def testAlias(self):
        """ Playing with aliases """
        sheet = self.doc.addObject("Spreadsheet::Sheet","Calc")
        sheet.setAlias("A1","Test")
        self.assertEqual(sheet.getAlias("A1"),"Test")

        sheet.set("A1","4711")
        self.doc.recompute()
        self.assertEqual(sheet.get("Test"),4711)
        self.assertEqual(sheet.get("Test"),sheet.get("A1"))

    def testAmbiguousAlias(self):
        """ Try to set the same alias twice (bug #2402) """
        sheet = self.doc.addObject("Spreadsheet::Sheet","Calc")
        sheet.setAlias("A1","Test")
        try:
            sheet.setAlias("A2","Test")
            self.fail("An ambiguous alias was set which shouldn't be allowed")
        except:
            self.assertEqual(sheet.getAlias("A2"),None)

    def testClearAlias(self):
        """ This was causing a crash """
        sheet = self.doc.addObject("Spreadsheet::Sheet","Calc")
        sheet.setAlias("A1","Test")
        sheet.setAlias("A1","")
        self.assertEqual(sheet.getAlias("A1"),None)

    def testSetInvalidAlias(self):
        """ Try to use a cell address as alias name """
        sheet = self.doc.addObject("Spreadsheet::Sheet","Calc")
        try:
            sheet.setAlias("A1","B1")
        except:
            self.assertEqual(sheet.getAlias("A1"),None)
        else:
            self.fail("A cell address was used as alias which shouldn't be allowed")

    def testPlacementName(self):
        """ Object name is equal to property name (bug #2389) """
        if not FreeCAD.GuiUp:
            return
        
        import FreeCADGui
        o = self.doc.addObject("Part::FeaturePython","Placement")
        FreeCADGui.Selection.addSelection(o)

    def testInvoluteGear(self):
        """ Support of boolean or integer values """
        try:
            import InvoluteGearFeature
        except ImportError:
            return
        InvoluteGearFeature.makeInvoluteGear('InvoluteGear')
        self.doc.recompute()
        sketch=self.doc.addObject('Sketcher::SketchObject','Sketch')
        sketch.addGeometry(Part.Line(v(0,0,0),v(10,10,0)),False)
        sketch.addConstraint(Sketcher.Constraint('Distance',0,65.285388)) 
        sketch.setExpression('Constraints[0]', 'InvoluteGear.NumberOfTeeth')
        self.doc.recompute()
        self.assertIn('Up-to-date',sketch.State)

    def testSketcher(self):
        """ Mixup of Label and Name (bug #2407)"""
        sketch=self.doc.addObject('Sketcher::SketchObject','Sketch')
        sheet=self.doc.addObject('Spreadsheet::Sheet','Spreadsheet')
        sheet.setAlias('A1', 'Length')
        self.doc.recompute()
        sheet.set('A1', '47,11')
        self.doc.recompute()

        index=sketch.addGeometry(Part.Line(v(0,0,0),v(10,10,0)),False)
        sketch.addConstraint(Sketcher.Constraint('Distance',index,14.0)) 
        self.doc.recompute()
        sketch.setExpression('Constraints[0]', u'Spreadsheet.Length')
        self.doc.recompute()
        sheet.Label="Calc"
        self.doc.recompute()
        self.assertEqual(sketch.ExpressionEngine[0][1],'Calc.Length')
        self.assertIn('Up-to-date',sketch.State)

    def testCrossDocumentLinks(self):
        """ Expressions accross files are not saved (bug #2442) """

        # Create a box
        box = self.doc.addObject('Part::Box', 'Box')

        # Create a second document with a cylinder
        doc2 = FreeCAD.newDocument()
        cylinder = doc2.addObject('Part::Cylinder', 'Cylinder')
        cylinder.setExpression('Radius', 'cube#Cube.Height')

        # Save and close first document
        self.doc.saveAs(self.TempPath + os.sep + 'cube.fcstd')
        FreeCAD.closeDocument(self.doc.Name)

        # Save and close second document
        doc2.saveAs(self.TempPath + os.sep + 'cylinder.fcstd')
        FreeCAD.closeDocument(doc2.Name)

        # Open both documents again
        self.doc = FreeCAD.openDocument(self.TempPath + os.sep + 'cube.fcstd')
        doc2 = FreeCAD.openDocument(self.TempPath + os.sep + 'cylinder.fcstd')

        # Check reference between them
        self.assertEqual(doc2.getObject('Cylinder').ExpressionEngine[0][1], 'cube#Cube.Height')

        # Close second document
        FreeCAD.closeDocument(doc2.Name)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument(self.doc.Name)
