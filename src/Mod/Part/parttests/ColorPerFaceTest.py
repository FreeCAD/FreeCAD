#!/usr/bin/env python
# -*- coding: utf-8 -*-

# test to check color per face when after restore

import FreeCAD as App
import Part
import os
import tempfile
import unittest
from BOPTools import BOPFeatures
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

        box.ViewObject.DiffuseColor = [(1.,0.,0.,1.),
                                       (1.,0.,0.,1.),
                                       (1.,0.,0.,1.),
                                       (1.,0.,0.,1.),
                                       (1.,1.,0.,1.),
                                       (1.,1.,0.,1.)]

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

        box.ViewObject.DiffuseColor = [(1.,0.,0.,1.),
                                       (1.,0.,0.,1.),
                                       (1.,0.,0.,1.),
                                       (1.,0.,0.,1.),
                                       (1.,1.,0.,1.),
                                       (1.,1.,0.,1.)]

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

    def testTransparency(self):
        """
        If color per face is set then changing the transparency must not revert it
        """
        box = self.doc.addObject("Part::Box","Box")
        self.doc.recompute()

        box.ViewObject.DiffuseColor = [(1.,0.,0.,1.),
                                       (1.,0.,0.,1.),
                                       (1.,0.,0.,1.),
                                       (1.,0.,0.,1.),
                                       (1.,1.,0.,1.),
                                       (1.,1.,0.,1.)]

        box.ViewObject.Transparency = 35
        self.assertEqual(box.ViewObject.Transparency, 35)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterialBinding.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(box.ViewObject.RootNode)
        paths = sa.getPaths()

        bind = paths.get(2).getTail()
        self.assertEqual(bind.value.getValue(), bind.PER_PART)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(box.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(2).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 6)

    def testMultiFuse(self):
        """
        Both input objects are red. So, it's expected that the output object is red, too.
        """
        box = self.doc.addObject("Part::Box","Box")
        cyl = self.doc.addObject("Part::Cylinder","Cylinder")
        box.ViewObject.ShapeColor = (1.,0.,0.,1.)
        cyl.ViewObject.ShapeColor = (1.,0.,0.,1.)
        self.doc.recompute()

        bp = BOPFeatures.BOPFeatures(self.doc)
        fuse = bp.make_multi_fuse([box.Name, cyl.Name])
        self.assertEqual(fuse.TypeId, "Part::MultiFuse")
        fuse.Refine = False
        self.doc.recompute()

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterialBinding.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(fuse.ViewObject.RootNode)
        paths = sa.getPaths()

        bind = paths.get(2).getTail()
        self.assertEqual(bind.value.getValue(), bind.PER_PART)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(fuse.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(2).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 11)

        self.assertEqual(len(fuse.Shape.Faces), 11)
        self.assertEqual(len(fuse.ViewObject.DiffuseColor), 11)
        self.assertEqual(fuse.ViewObject.DiffuseColor[0], (1.,0.,0.,1.))

    def testMultiFuseSaveRestore(self):
        box = self.doc.addObject("Part::Box","Box")
        cyl = self.doc.addObject("Part::Cylinder","Cylinder")
        box.ViewObject.ShapeColor = (1.,0.,0.,1.)
        cyl.ViewObject.ShapeColor = (1.,0.,0.,1.)
        self.doc.recompute()

        bp = BOPFeatures.BOPFeatures(self.doc)
        fuse = bp.make_multi_fuse([box.Name, cyl.Name])
        self.assertEqual(fuse.TypeId, "Part::MultiFuse")
        fuse.Refine = False
        self.doc.recompute()

        fuse.ViewObject.DiffuseColor = [(1.,0.,0.,1.)] * 11

        self.doc.saveAs(self.fileName)
        App.closeDocument(self.doc.Name)

        self.doc = App.openDocument(self.fileName)

        fuse = self.doc.ActiveObject
        self.assertEqual(len(fuse.Shape.Faces), 11)
        self.assertEqual(len(fuse.ViewObject.DiffuseColor), 11)
        self.assertEqual(fuse.ViewObject.DiffuseColor[0], (1.,0.,0.,1.))

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterialBinding.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(fuse.ViewObject.RootNode)
        paths = sa.getPaths()

        bind = paths.get(2).getTail()
        self.assertEqual(bind.value.getValue(), bind.PER_PART)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(fuse.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(2).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 11)
