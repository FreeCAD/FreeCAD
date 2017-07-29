#   (c) Juergen Riegel (juergen.riegel@web.de) 20010 LGPL

import FreeCAD
import unittest
import math

def tu(str):
    return FreeCAD.Units.Quantity(str).Value

def ts(q):
    return q.UserString

def ts2(q):
    return FreeCAD.Units.Quantity(q.UserString).UserString


#---------------------------------------------------------------------------
# define the functions to test the FreeCAD UnitApi code
#---------------------------------------------------------------------------


def compare(x, y):
    return math.fabs(x - y) < 0.00001


class UnitBasicCases(unittest.TestCase):

    def testConversions(self):
        #tu = FreeCAD.Units.translateUnit
        self.failUnless(compare(tu('10 m'), 10000.0))
        self.failUnless(compare(tu('3/8 in'), 9.525))
        self.failUnless(compare(tu('100 km/h'), 27777.77777777))
        self.failUnless(compare(tu('m^2*kg*s^-3*A^-2'), 1000000.0))
        self.failUnless(compare(tu('(m^2*kg)/(A^2*s^3)'), 1000000.0))
        self.failUnless(compare(tu('2*pi rad'), 360.0))
        self.failUnless(compare(tu('2*pi rad') / tu('gon'), 400.0))
        self.failUnless(compare(tu('999 kg') / tu('1 m^3'), 0.000009999))

    def testImperial(self):
        #tu = FreeCAD.Units.translateUnit
        self.failUnless(compare(tu('3/8in'), 9.525))
        #self.failUnless(compare(tu('1fo(3+7/16)in'),392.112500))thisgivesaparsersyntaxerror!!!
        self.failUnless(compare(tu('1\'(3+7/16)"'), 392.112500))

    def testSelfConsistency(self):
        qu = FreeCAD.Units.Quantity("0.23 W/m/K")
        self.assertTrue(ts(qu), ts2(qu))
        qu = FreeCAD.Units.Quantity("237 mm*kg/(s^3*K)")
        self.assertTrue(ts(qu), ts2(qu))
        qu = FreeCAD.Units.Quantity("237.000 W/mm/K")
        self.assertTrue(ts(qu), ts2(qu))

    def testTrigonometric(self):
        #tu=FreeCAD.Units.translateUnit
        self.failUnless(compare(tu('sin(pi)'), math.sin(math.pi)))
        self.failUnless(compare(tu('cos(pi)'), math.cos(math.pi)))
        self.failUnless(compare(tu('tan(pi)'), math.tan(math.pi)))
