#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import FreeCAD
import FreeCADGui
import unittest


class TechDrawTaskTest(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument()

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def testDrawProjGroup(self):
        obj = self.doc.addObject("TechDraw::DrawProjGroup")
        with self.assertRaises(RuntimeError):
            obj.ViewObject.doubleClicked()

    def testDrawViewSection(self):
        obj = self.doc.addObject("TechDraw::DrawViewSection")
        with self.assertRaises(RuntimeError):
            obj.ViewObject.doubleClicked()

    def testDrawComplexSection(self):
        obj = self.doc.addObject("TechDraw::DrawComplexSection")
        with self.assertRaises(RuntimeError):
            obj.ViewObject.doubleClicked()

    def testDrawRichAnno(self):
        obj = self.doc.addObject("TechDraw::DrawRichAnno")
        with self.assertRaises(RuntimeError):
            obj.ViewObject.doubleClicked()

    def testDrawLeaderLine(self):
        obj = self.doc.addObject("TechDraw::DrawLeaderLine")
        with self.assertRaises(RuntimeError):
            obj.ViewObject.doubleClicked()

    def testDrawViewDetail(self):
        obj = self.doc.addObject("TechDraw::DrawViewDetail")
        with self.assertRaises(RuntimeError):
            obj.ViewObject.doubleClicked()

