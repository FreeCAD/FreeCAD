# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import PathScripts.PathUtil as PathUtil
import TestSketcherApp

from PathTests.PathTestUtils import PathTestBase

class TestPathUtil(PathTestBase):

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathUtils")

    def tearDown(self):
        FreeCAD.closeDocument("TestPathUtils")

    def test00(self):
        '''Check that isValidBaseObject detects solids.'''
        box = self.doc.addObject('Part::Box', 'Box')
        cylinder = self.doc.addObject('Part::Cylinder', 'Cylinder')
        self.doc.recompute()
        self.assertTrue(PathUtil.isValidBaseObject(box))
        self.assertTrue(PathUtil.isValidBaseObject(cylinder))

    def test01(self):
        '''Check that isValidBaseObject detects PDs.'''
        body = self.doc.addObject('PartDesign::Body', 'Body')
        box  = self.doc.addObject('PartDesign::AdditiveBox', 'Box')
        body.addObject(box)
        self.doc.recompute()
        self.assertTrue(PathUtil.isValidBaseObject(body))

    def test02(self):
        '''Check that isValidBaseObject detects compounds.'''
        box = self.doc.addObject('Part::Box', 'Box')
        box.Length = 10
        box.Width = 10
        box.Height = 1
        box.Placement = FreeCAD.Placement(FreeCAD.Vector(-5,-5,0), FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        cyl = self.doc.addObject('Part::Cylinder', 'Cylinder')
        cyl.Radius = 1
        cyl.Height = 10
        box.Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,-5), FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        cut = self.doc.addObject('Part::Cut', 'Cut')
        cut.Base = box
        cut.Tool = cyl
        self.doc.recompute()
        self.assertTrue(PathUtil.isValidBaseObject(cut))


    def test03(self):
        '''Check that isValidBaseObject ignores sketches.'''
        body = self.doc.addObject('PartDesign::Body', 'Body')
        sketch = self.doc.addObject('Sketcher::SketchObject', 'Sketch')
        body.addObject(sketch)
        TestSketcherApp.CreateSlotPlateSet(sketch)
        self.doc.recompute()
        pad = self.doc.addObject('PartDesign::Pad', 'Pad')
        body.addObject(pad)
        pad.Profile = sketch
        self.doc.recompute()

        # the body is a solid
        self.assertTrue(PathUtil.isValidBaseObject(body))

        # the pad inside the body cannot be used due to the linking constraints
        self.assertFalse(PathUtil.isValidBaseObject(pad))

        # the sketch is no good neither
        self.assertFalse(PathUtil.isValidBaseObject(sketch))

    def test04(self):
        '''Check that Part is handled correctly.'''
        part = self.doc.addObject('App::Part', 'Part')

        # an empty part is not a valid base object
        self.assertFalse(PathUtil.isValidBaseObject(part))

        # a non-empty part where none of the objects have a shape, is no good either
        fp = self.doc.addObject('App::FeaturePython', 'Feature')
        part.addObject(fp)
        self.assertFalse(PathUtil.isValidBaseObject(part))

        # create a valid base object
        box = self.doc.addObject("Part::Box","Box")
        self.assertTrue(PathUtil.isValidBaseObject(box))

        # a part with at least one valid object is valid
        part.addObject(box)
        self.assertTrue(PathUtil.isValidBaseObject(part))

        # however, the object itself is no longer valid
        self.assertFalse(PathUtil.isValidBaseObject(box))

