# (c) 2016 Werner Mayer
# LGPL

import os
import sys
import unittest
import FreeCAD
import Part
import Sketcher
from FreeCAD import Base

v = Base.Vector

#----------------------------------------------------------------------------------
# define the functions to test the FreeCAD Spreadsheet module and expression engine
#----------------------------------------------------------------------------------


class SpreadsheetCases(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument()

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

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument(self.doc.Name)
