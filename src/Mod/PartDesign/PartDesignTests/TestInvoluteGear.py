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
import pathlib

import FreeCAD
from Part import makeCircle, Precision
import InvoluteGearFeature

FIXTURE_PATH = pathlib.Path(__file__).parent / "Fixtures"

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

    def testCustomizedGearProfileForSplinedShaft(self):
        spline = InvoluteGearFeature.makeInvoluteGear('InvoluteSplinedShaft')
        z = 12
        m = 2
        add_coef = 0.5
        ded_coef = 0.9
        spline.NumberOfTeeth = z
        spline.Modules = f'{m} mm'
        spline.PressureAngle = '30 deg'
        spline.AddendumCoefficient = add_coef
        spline.DedendumCoefficient = ded_coef
        spline.RootFilletCoefficient = 0.4
        self.assertSuccessfulRecompute(spline)
        self.assertClosedWire(spline.Shape)
        pitch_diameter = m * z
        tip_diameter = pitch_diameter + 2 * add_coef * m
        root_diameter = pitch_diameter - 2 * ded_coef * m
        # the test purpose here is just to ensure the gear's parameters are used,
        # not super precise profile verification. Thus a lax delta is just file here.
        delta = 0.01
        self.assertIntersection(spline.Shape, makeCircle(pitch_diameter/2), "Expecting intersection at pitch circle")
        self.assertNoIntersection(spline.Shape, makeCircle(tip_diameter/2 + delta), "Teeth extent beyond tip circle")
        self.assertNoIntersection(spline.Shape, makeCircle(root_diameter/2 - delta), "Teeth extend below root circle")

    def testCustomizedGearProfileForSplinedHub(self):
        hub = InvoluteGearFeature.makeInvoluteGear('InvoluteSplinedHub')
        hub.ExternalGear = False
        z = 12
        m = 2
        add_coef = 0.5
        ded_coef = 0.9
        hub.NumberOfTeeth = z
        hub.Modules = f'{m} mm'
        hub.PressureAngle = '30 deg'
        hub.AddendumCoefficient = add_coef
        hub.DedendumCoefficient = ded_coef
        hub.RootFilletCoefficient = 0.4
        self.assertSuccessfulRecompute(hub)
        self.assertClosedWire(hub.Shape)
        pitch_diameter = m * z
        tip_diameter = pitch_diameter - 2 * add_coef * m
        root_diameter = pitch_diameter + 2 * ded_coef * m
        # the test purpose here is just to ensure the gear's parameters are used,
        # not super precise profile verification. Thus a lax delta is just file here.
        delta = 0.01
        self.assertIntersection(hub.Shape, makeCircle(pitch_diameter/2), "Expecting intersection at pitch circle")
        self.assertNoIntersection(hub.Shape, makeCircle(tip_diameter/2 - delta), "Teeth extent below tip circle")
        self.assertNoIntersection(hub.Shape, makeCircle(root_diameter/2 + delta), "Teeth extend beyond root circle")

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

    def testRecomputeExternalGearFromV020(self):
        FreeCAD.closeDocument(self.Doc.Name) # this was created in setUp(self)
        self.Doc = FreeCAD.openDocument(str(FIXTURE_PATH / "InvoluteGear_v0-20.FCStd"))
        created_with = f"created with {self.Doc.getProgramVersion()}"
        gear = self.Doc.InvoluteGear # from fixture
        fixture_length = 187.752 # from fixture, rounded to micrometer
        self.assertClosedWire(gear.Shape) # no recompute yet, i.e. check original
        self.assertAlmostEqual(fixture_length, gear.Shape.Length, places=3,
            msg=f"Total wire length does not match fixture for gear {created_with}")
        gear.enforceRecompute()
        self.assertSuccessfulRecompute(gear, msg=f"Cannot recompute gear {created_with}")
        relative_tolerance_per_tooth = 1e-3 # wild guess: changes of <0.1%/tooth are ok
        length_delta = fixture_length * relative_tolerance_per_tooth * gear.NumberOfTeeth
        self.assertAlmostEqual(fixture_length, gear.Shape.Length, delta=length_delta,
            msg=f"Total wire length changed after recomputing gear {created_with}")

    def testRecomputeInternalGearFromV020(self):
        FreeCAD.closeDocument(self.Doc.Name) # this was created in setUp(self)
        self.Doc = FreeCAD.openDocument(str(FIXTURE_PATH / "InternalInvoluteGear_v0-20.FCStd"))
        created_with = f"created with {self.Doc.getProgramVersion()}"
        gear = self.Doc.InvoluteGear # from fixture
        fixture_length = 165.408 # from fixture, rounded to micrometer
        self.assertClosedWire(gear.Shape) # no recompute yet, i.e. check original
        self.assertAlmostEqual(fixture_length, gear.Shape.Length, places=3,
            msg=f"Total wire length does not match fixture for gear {created_with}")
        gear.enforceRecompute()
        self.assertSuccessfulRecompute(gear, msg=f"Cannot recompute gear {created_with}")
        relative_tolerance_per_tooth = 1e-3 # wild guess: changes of <0.1%/tooth are ok
        length_delta = fixture_length * relative_tolerance_per_tooth * gear.NumberOfTeeth
        self.assertAlmostEqual(fixture_length, gear.Shape.Length, delta=length_delta,
            msg=f"Total wire length changed after recomputing gear {created_with}")

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
