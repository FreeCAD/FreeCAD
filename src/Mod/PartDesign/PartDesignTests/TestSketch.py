#***************************************************************************
#*   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
#*                                                                         *
#*   This file is part of FreeCAD.                                         *
#*                                                                         *
#*   FreeCAD is free software: you can redistribute it and/or modify it    *
#*   under the terms of the GNU Lesser General Public License as           *
#*   published by the Free Software Foundation, either version 2.1 of the  *
#*   License, or (at your option) any later version.                       *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful, but        *
#*   WITHOUT ANY WARRANTY; without even the implied warranty of            *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
#*   Lesser General Public License for more details.                       *
#*                                                                         *
#*   You should have received a copy of the GNU Lesser General Public      *
#*   License along with FreeCAD. If not, see                               *
#*   <https://www.gnu.org/licenses/>.                                      *
#*                                                                         *
#***************************************************************************

import unittest

import FreeCAD

""" Test transaction interaction """
class TestSketch(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("PartDesignTestSketch")
        self.doc.UndoMode = True

    def testIssue17553(self):
        self.doc.openTransaction("Create box")
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.commitTransaction()
        self.doc.recompute()

        self.doc.openTransaction("Create sketch")
        body = self.doc.addObject('PartDesign::Body', 'Body')
        plane = self.doc.getObject('XY_Plane')
        self.doc.commitTransaction()

        self.doc.openTransaction("Rename object")
        box.Label = "Object"
        self.doc.commitTransaction()

        sketch = body.newObject('Sketcher::SketchObject', 'Sketch')
        sketch.AttachmentSupport = (plane, [''])
        sketch.MapMode = 'FlatFace'
        self.doc.recompute()

        self.assertEqual(sketch.InList, [body])
        self.assertEqual(sketch.OutList, [plane])
        sketch.AttachmentSupport == [(plane, ("",))]

        self.doc.undo() # undo renaming
        self.doc.undo() # undo body creation
        self.doc.undo() # undo box creation

        self.doc.openTransaction("Remove sketch")
        self.doc.removeObject(sketch.Name)
        self.doc.commitTransaction()

        self.doc.undo() # undo removal

        self.assertEqual(sketch.InList, [])
        self.assertEqual(sketch.OutList, [])
        self.assertEqual(sketch.AttachmentSupport, [])

    def testDependency(self):
        self.doc.openTransaction("Create box")
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.commitTransaction()
        self.doc.recompute()

        self.doc.openTransaction("Create sketch")
        body = self.doc.addObject('PartDesign::Body', 'Body')
        plane = self.doc.getObject('XY_Plane')
        self.doc.commitTransaction()

        self.doc.openTransaction("Rename object")
        box.Label = "Object"
        self.doc.commitTransaction()

        sketch = body.newObject('Sketcher::SketchObject', 'Sketch')
        sketch.AttachmentSupport = (plane, [''])
        sketch.MapMode = 'FlatFace'
        self.doc.recompute()

        sketch.OutList
        sketch.AttachmentSupport

        self.doc.undo() # undo renaming
        self.doc.undo() # undo body creation
        self.doc.undo() # undo box creation

        self.doc.DependencyGraph

    def tearDown(self):
        FreeCAD.closeDocument("PartDesignTestSketch")

