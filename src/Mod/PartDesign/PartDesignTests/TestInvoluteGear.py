#***************************************************************************
#*   Copyright (c) 2021 Jonas Bähr <jonas.baehr@web.de>                    *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import unittest

import FreeCAD
from Part import makeCircle, Precision
import InvoluteGearFeature

class TestInvoluteGear(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestInvoluteGear")

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

    def testDefaultGearProfile(self):
        InvoluteGearFeature.makeInvoluteGear('TestGear')
        gear = self.Doc.getObject('TestGear')
        self.assertSuccessfulRecompute(gear)
        self.assertClosedWire(gear.Shape)

    def testDefaultInternalGearProfile(self):
        gear = InvoluteGearFeature.makeInvoluteGear('InvoluteGear')
        gear.ExternalGear = False
        self.assertSuccessfulRecompute(gear)
        self.assertClosedWire(gear.Shape)

    def testLowPrecisionGearProfile(self):
        gear = InvoluteGearFeature.makeInvoluteGear('InvoluteGear')
        gear.HighPrecision = False
        self.assertSuccessfulRecompute(gear)
        self.assertClosedWire(gear.Shape)

    def testLowPrecisionInternalGearProfile(self):
        gear = InvoluteGearFeature.makeInvoluteGear('InvoluteGear')
        gear.ExternalGear = False
        gear.HighPrecision = False
        self.assertSuccessfulRecompute(gear)
        self.assertClosedWire(gear.Shape)

    def testCustomizedGearProfile(self):
        gear = InvoluteGearFeature.makeInvoluteGear('InvoluteGear')
        z = 12
        m = 1
        gear.NumberOfTeeth = z
        gear.Modules = f'{m} mm'
        gear.PressureAngle = '14.5 deg'
        self.assertSuccessfulRecompute(gear)
        self.assertClosedWire(gear.Shape)
        pitch_diameter = m * z
        default_addendum = 1
        default_dedendum = 1.25
        tip_diameter = pitch_diameter + 2 * default_addendum * m
        root_diameter = pitch_diameter - 2 * default_dedendum * m
        # the test purpose here is just to ensure the gear's parameters are used,
        # not super precise profile verification. Thus a lax delta is just file here.
        delta = 0.01
        self.assertIntersection(gear.Shape, makeCircle(pitch_diameter/2), "Expecting intersection at pitch circle")
        self.assertNoIntersection(gear.Shape, makeCircle(tip_diameter/2 + delta), "Teeth extent beyond tip circle")
        self.assertNoIntersection(gear.Shape, makeCircle(root_diameter/2 - delta), "Teeth extend below root circle")

    def testUsagePadGearProfile(self):
        profile = InvoluteGearFeature.makeInvoluteGear('GearProfile')
        body = self.Doc.addObject('PartDesign::Body','GearBody')
        body.addObject(profile)
        pad = body.newObject("PartDesign::Pad","GearPad")
        pad.Profile = profile
        pad.Length = '5 mm' # that our gear's "Face Width"
        self.assertSuccessfulRecompute()
        self.assertSolid(pad.Shape)

    def testUsagePocketInternalGearProfile(self):
        profile = InvoluteGearFeature.makeInvoluteGear('GearProfile')
        profile.ExternalGear = False
        # boolean cuts with lots of B-splines are quite slow, so let's make it less complex
        profile.HighPrecision = False
        profile.NumberOfTeeth = 8
        body = self.Doc.addObject('PartDesign::Body','GearBody')
        body.addObject(profile)
        cylinder = body.newObject('PartDesign::AdditiveCylinder','GearCylinder')
        default_dedendum = 1.25
        rim_width = 3 * FreeCAD.Units.MilliMetre
        cylinder.Height = '5 mm' # that our gear's "Face Width"
        cylinder.Radius = profile.NumberOfTeeth * profile.Modules / 2 + default_dedendum * profile.Modules + rim_width
        pocket = body.newObject('PartDesign::Pocket','GearPocket')
        pocket.Profile = profile
        pocket.Reversed = True # need to "pocket upwards" into the cylinder
        pocket.Type = 'ThroughAll'
        self.assertSuccessfulRecompute()
        self.assertSolid(pocket.Shape)

    def assertSuccessfulRecompute(self, *objs, msg=None):
        if (len(objs) == 0):
            self.Doc.recompute()
            objs = self.Doc.Objects
        else:
            self.Doc.recompute(objs)
        failed_objects = [o.Name for o in objs if 'Invalid' in o.State]
        if (len(failed_objects) > 0):
            self.fail(msg or f"Recompute failed for {failed_objects}")

    def assertClosedWire(self, shape, msg=None):
        self.assertEqual(shape.ShapeType, 'Wire', msg=msg)
        self.assertTrue(shape.isClosed(), msg=msg)

    def assertIntersection(self, shape1, shape2, msg=None):
        self.failUnless(self._check_intersection(shape1, shape2), msg or "Given shapes do not intersect.")

    def assertNoIntersection(self, shape1, shape2, msg=None):
        self.failIf(self._check_intersection(shape1, shape2), msg or "Given shapes intersect.")

    def _check_intersection(self, shape1, shape2):
        distance, _, _ = shape1.distToShape(shape2)
        return distance < Precision.intersection()

    def assertSolid(self, shape, msg=None):
        self.assertEqual(shape.ShapeType, 'Solid', msg=msg)
