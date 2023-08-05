#!/usr/bin/env python
# -*- coding: utf-8 -*-

# basic test script for PropertyTopoShapeList

import FreeCAD as App
import Part
import os
import tempfile
import unittest

class TopoShapeListTest(unittest.TestCase):
    def setUp(self):
        """Creates a document and a FeaturePython that has a PropertyTopoShapeList property"""
        print("TopoShapeListTest - setting up")
        TempPath = tempfile.gettempdir()
        self.fileName = TempPath + os.sep + "TopoShapeListTest.FCStd"
        self.objName = "TestObject"
        App.newDocument("TopoShapeList")
        App.setActiveDocument("TopoShapeList")
        doc = App.newDocument()
        obj = doc.addObject("App::FeaturePython", self.objName)
        obj.addProperty("Part::PropertyTopoShapeList", "Shapes")
        box = Part.makeBox(1,1,1)
        box2 = Part.makeBox(1,1,2)
        box3 = Part.makeBox(1,1,3)
        obj.Shapes = [box, box2, box3]
        doc.saveAs(self.fileName)
        App.closeDocument(doc.Name)
        print("TopoShapeListTest: setUp complete")

    def tearDown(self):
        print("TopoShapeListTest finished")
        App.closeDocument("TopoShapeList")

    def testMakeTopoShapeList(self):
        """Tests PropertyTopoShapeList"""
        print("running TopoShapeListTest")
        doc = App.openDocument(self.fileName)
        doc.UndoMode = 1
        obj = doc.getObject(self.objName)
        boxes = obj.Shapes

        maxError = 0.0000001
        error = abs(1.0 - boxes[0].Volume)
        self.assertLessEqual(error, maxError, "TopoShapeList entry 0 has wrong volume: {0}".format(boxes[0].Volume))
        error = abs(2.0 - boxes[1].Volume)
        self.assertLessEqual(error, maxError, "TopoShapeList entry 1 has wrong volume: {0}".format(boxes[1].Volume))
        error = abs(3.0 - boxes[2].Volume)
        self.assertLessEqual(error, maxError, "TopoShapeList entry 2 has wrong volume: {0}".format(boxes[2].Volume))

        twoboxes = [boxes[1], boxes[2]]
        doc.openTransaction("Change shapes")
        obj.Shapes = twoboxes
        doc.commitTransaction()
        self.assertEqual(len(obj.Shapes), 2, "TopoShapeList has wrong entry count (1): {0}".format(len(obj.Shapes)))

        doc.undo()

        self.assertEqual(len(obj.Shapes), 3, "TopoShapeList has wrong entry count (2): {0}".format(len(obj.Shapes)))
        App.closeDocument(doc.Name)

