#!/usr/bin/env python
# -*- coding: utf-8 -*-

# test to check color per face when after restore

import FreeCAD as App
import Part
import os
import tempfile
import unittest
from pivy import coin

class ColorPerFaceTest(unittest.TestCase):
    def setUp(self):
        TempPath = tempfile.gettempdir()
        self.fileName = TempPath + os.sep + "ColorPerFaceTest.FCStd"
        self.doc = App.newDocument()

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def testBox(self):
        box = self.doc.addObject("Part::Box","Box")
        self.doc.recompute()

        box.ViewObject.DiffuseColor = [(1.,0.,0.,0.),
                                       (1.,0.,0.,0.),
                                       (1.,0.,0.,0.),
                                       (1.,0.,0.,0.),
                                       (1.,1.,0.,0.),
                                       (1.,1.,0.,0.)]

        box.Visibility = False
        self.doc.recompute()

        self.doc.saveAs(self.fileName)
        App.closeDocument(self.doc.Name)

        self.doc = App.openDocument(self.fileName)
        box = self.doc.Box
        box.Visibility = True
        self.assertEqual(len(box.ViewObject.DiffuseColor), 6)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(box.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(2).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 6)

    def testBoxAndLink(self):
        box = self.doc.addObject("Part::Box","Box")
        self.doc.recompute()

        box.ViewObject.DiffuseColor = [(1.,0.,0.,0.),
                                       (1.,0.,0.,0.),
                                       (1.,0.,0.,0.),
                                       (1.,0.,0.,0.),
                                       (1.,1.,0.,0.),
                                       (1.,1.,0.,0.)]

        link = self.doc.addObject('App::Link','Link')
        link.setLink(box)
        box.Visibility = False
        self.doc.recompute()

        self.doc.saveAs(self.fileName)
        App.closeDocument(self.doc.Name)

        self.doc = App.openDocument(self.fileName)
        box = self.doc.Box
        box.Visibility = True
        self.assertEqual(len(box.ViewObject.DiffuseColor), 6)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(box.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(2).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 6)
