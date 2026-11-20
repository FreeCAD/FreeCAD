# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD <hello@astocad.com>                         *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import os
import tempfile
import unittest

import FreeCAD as App
import Part

import FastenerLinkObject
import JointObject
import UtilsAssembly


class TestFastenerLink(unittest.TestCase):
    """The fastener links of the Insert Fastener tool, without the Fasteners workbench:
    a cylinder stands for the fastener."""

    HOLES = (20, 50, 80)

    def setUp(self):
        self.doc = App.newDocument("TestFastenerLink")
        self.assembly = self.doc.addObject("Assembly::AssemblyObject", "Assembly")
        self.jointgroup = self.assembly.newObject("Assembly::JointGroup", "Joints")

        # A plate with through holes, not aligned with the axes of the assembly
        shape = Part.makeBox(100, 40, 5)
        for x in self.HOLES:
            shape = shape.cut(Part.makeCylinder(3, 5, App.Vector(x, 20, 0)))
        self.plate = self.assembly.newObject("Part::Feature", "Plate")
        self.plate.Shape = shape
        self.plate.Placement = App.Placement(
            App.Vector(10, 20, 30), App.Rotation(App.Vector(1, 0, 0), 90)
        )
        ground = self.jointgroup.newObject("App::FeaturePython", "GroundedJoint")
        JointObject.GroundedJoint(ground, self.plate)

        self.fastener = self.doc.addObject("Part::Cylinder", "Fastener")
        self.fastener.Radius = 2
        self.fastener.Height = 8
        self.doc.recompute()
        # The outside of the plate, in the assembly
        self.top = self.plate.Placement.Rotation.multVec(App.Vector(0, 0, 1))

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def hole(self, x, top=True):
        """The reference to the edge of a hole, on the top or the bottom face of the plate"""
        z = 5 if top else 0
        for index, edge in enumerate(self.plate.Shape.Edges, 1):
            if edge.Curve.TypeId != "Part::GeomCircle":
                continue
            local = self.plate.Placement.inverse().multVec(edge.Curve.Location)
            if abs(local.x - x) < 1e-6 and abs(local.z - z) < 1e-6:
                sub = "Edge{}".format(index)
                return (self.plate, [sub, sub])
        self.fail("no hole at {}".format(x))

    def center(self, x, top=True):
        return self.plate.Placement.multVec(App.Vector(x, 20, 5 if top else 0))

    def zAxis(self, component):
        return component.Placement.Rotation.multVec(App.Vector(0, 0, 1))

    def solve(self):
        self.doc.recompute()
        self.assertEqual(self.assembly.solve(), 0)

    def assertOnHole(self, component, x, top=True, reversed=False):
        gap = (component.Placement.Base - self.center(x, top)).Length
        self.assertLess(gap, 1e-5, "fastener {} off its hole".format(component.Name))
        outside = self.top if top else -self.top
        expected = -1.0 if reversed else 1.0
        self.assertAlmostEqual(self.zAxis(component).dot(outside), expected, places=5)

    def makeLink(self, holes):
        link = FastenerLinkObject.makeFastenerLink(self.assembly, self.fastener)
        FastenerLinkObject.attach(link, holes[0])
        for ref in holes[1:]:
            component = FastenerLinkObject.addFastener(link, App.Placement())
            FastenerLinkObject.attach(component, ref)
        self.solve()
        return link

    def testPythonLinkIsAComponent(self):
        link = FastenerLinkObject.makeFastenerLink(self.assembly, self.fastener)
        self.doc.recompute()
        self.assertTrue(FastenerLinkObject.isFastenerLink(link))
        self.assertTrue(UtilsAssembly.isLink(link))
        component, sub = UtilsAssembly.getComponentReference(
            self.assembly, self.assembly, link.Name + ".Edge1"
        )
        self.assertEqual(component, link)
        self.assertEqual(sub, "Edge1")

    def testHoleInfo(self):
        info = FastenerLinkObject.holeInfo(self.hole(20))
        self.assertAlmostEqual(info["radius"], 3.0)
        self.assertLess((info["center"] - self.center(20)).Length, 1e-9)
        self.assertAlmostEqual(info["normal"].dot(self.top), 1.0)
        info = FastenerLinkObject.holeInfo(self.hole(20, top=False))
        self.assertAlmostEqual(info["normal"].dot(self.top), -1.0)
        # Not a circular edge
        self.assertIsNone(FastenerLinkObject.holeInfo((self.plate, ["Edge1", "Edge1"])))
        self.assertIsNone(FastenerLinkObject.holeInfo((self.plate, ["Face1", "Face1"])))

    def testAttachPointsOutOfTheMaterial(self):
        """A fastener goes on its hole head outside, on whichever side the edge is"""
        link = self.makeLink([self.hole(20), self.hole(50, top=False)])
        first, second = FastenerLinkObject.components(link)
        self.assertOnHole(first, 20)
        self.assertOnHole(second, 50, top=False)

    def testAttachmentIsReadFromTheJoint(self):
        link = FastenerLinkObject.makeFastenerLink(self.assembly, self.fastener)
        self.assertEqual(FastenerLinkObject.attachment(link), (None, None))
        ref = self.hole(20)
        joint = FastenerLinkObject.attach(link, ref)
        self.solve()
        found, hole = FastenerLinkObject.attachment(link)
        self.assertEqual(found, joint)
        self.assertEqual(hole[0], self.plate)
        self.assertEqual(hole[1][0], ref[1][0])
        self.doc.removeObject(joint.Name)
        self.assertEqual(FastenerLinkObject.attachment(link), (None, None))

    def testReverseSurvivesTheSolver(self):
        link = self.makeLink([self.hole(20)])
        FastenerLinkObject.reverse(link)
        self.solve()
        self.assertOnHole(link, 20, reversed=True)
        FastenerLinkObject.reverse(link)
        self.solve()
        self.assertOnHole(link, 20)

    def testOneFastenerIsAPlainLink(self):
        link = self.makeLink([self.hole(20)])
        self.assertEqual(link.ElementCount, 0)
        self.assertEqual(FastenerLinkObject.components(link), [link])
        self.assertOnHole(link, 20)

    def testAddFastenersMakesElements(self):
        """With a second fastener the link becomes a group: the first fastener keeps its
        place and its joint, which now refers to the first element"""
        link = self.makeLink([self.hole(20)])
        joint, _hole = FastenerLinkObject.attachment(link)

        second = FastenerLinkObject.addFastener(link, App.Placement())
        FastenerLinkObject.attach(second, self.hole(50))
        third = FastenerLinkObject.addFastener(link, App.Placement())
        FastenerLinkObject.attach(third, self.hole(80))
        self.solve()

        fasteners = FastenerLinkObject.components(link)
        self.assertEqual(link.ElementCount, 3)
        self.assertEqual(len(fasteners), 3)
        self.assertTrue(link.Placement.isIdentity())
        self.assertEqual(FastenerLinkObject.attachment(fasteners[0])[0], joint)
        for component, x in zip(fasteners, self.HOLES):
            self.assertEqual(component.TypeId, "App::LinkElement")
            self.assertOnHole(component, x)
        # Each fastener is a component of the assembly
        parts = self.assembly.getComponents() if hasattr(self.assembly, "getComponents") else None
        if parts is not None:
            for component in fasteners:
                self.assertIn(component, parts)

    def testRemoveFastenerInTheMiddle(self):
        link = self.makeLink([self.hole(x) for x in self.HOLES])
        before = FastenerLinkObject.components(link)
        removedJoint = FastenerLinkObject.attachment(before[1])[0].Name
        lastJoint = FastenerLinkObject.attachment(before[2])[0]

        FastenerLinkObject.removeFastener(link, 1)
        self.solve()

        fasteners = FastenerLinkObject.components(link)
        self.assertEqual(len(fasteners), 2)
        self.assertIsNone(self.doc.getObject(removedJoint))
        self.assertEqual(FastenerLinkObject.attachment(fasteners[1])[0], lastJoint)
        self.assertOnHole(fasteners[0], 20)
        self.assertOnHole(fasteners[1], 80)

        # Fasteners can be added again afterwards
        added = FastenerLinkObject.addFastener(link, App.Placement())
        FastenerLinkObject.attach(added, self.hole(50))
        self.solve()
        fasteners = FastenerLinkObject.components(link)
        self.assertEqual(len(set(component.Name for component in fasteners)), 3)
        self.assertOnHole(fasteners[2], 50)

    def testBackToOneFastener(self):
        """Left with one fastener, the link is a plain link again, with the joint"""
        link = self.makeLink([self.hole(20), self.hole(50)])
        joint = FastenerLinkObject.attachment(FastenerLinkObject.components(link)[1])[0]

        FastenerLinkObject.removeFastener(link, 0)
        self.solve()

        self.assertEqual(link.ElementCount, 0)
        self.assertEqual(FastenerLinkObject.components(link), [link])
        self.assertEqual(FastenerLinkObject.attachment(link)[0], joint)
        self.assertOnHole(link, 50)
        self.assertEqual(
            [obj for obj in self.doc.Objects if obj.TypeId == "App::LinkElement"], []
        )
        # The last fastener stays
        FastenerLinkObject.removeFastener(link, 0)
        self.assertEqual(FastenerLinkObject.components(link), [link])

    def testSetSource(self):
        """All the fasteners of a link follow the fastener that the link links to"""
        link = self.makeLink([self.hole(20), self.hole(50)])
        other = self.doc.addObject("Part::Cylinder", "Other")
        FastenerLinkObject.setSource(link, other)
        self.solve()
        for component in FastenerLinkObject.components(link):
            self.assertEqual(component.getLinkedObject(), other)
        self.assertEqual(FastenerLinkObject.otherUsers(link), [])
        self.assertOnHole(FastenerLinkObject.components(link)[1], 50)

    def testOtherUsers(self):
        link = self.makeLink([self.hole(20), self.hole(50)])
        self.assertEqual(FastenerLinkObject.otherUsers(link), [])
        other = FastenerLinkObject.makeFastenerLink(self.assembly, self.fastener)
        self.doc.recompute()
        self.assertEqual(FastenerLinkObject.otherUsers(link), [other])

    def testLabelsKeepTheSizeOfTheFastener(self):
        """The links and the joints are named after the fastener, whose name ends with its
        size: the unique numbers of the document must not replace these digits."""
        params = App.ParamGet("User parameter:BaseApp/Preferences/Document")
        self.addCleanup(params.SetBool, "DuplicateLabels", params.GetBool("DuplicateLabels"))
        params.SetBool("DuplicateLabels", False)
        self.fastener.Label = "ISO4762_M5_10"
        first = self.makeLink([self.hole(20), self.hole(50)])
        second = self.makeLink([self.hole(80)])
        for link in (first, second):
            link.Label = self.fastener.Label
            self.assertEqual(link.Label, "ISO4762_M5_10")
        self.assertEqual(self.fastener.Label, "ISO4762_M5_10")

        joints = [
            FastenerLinkObject.attachment(component)[0]
            for link in (first, second)
            for component in FastenerLinkObject.components(link)
        ]
        labels = [joint.Label for joint in joints]
        self.assertEqual(len(set(labels)), 3)
        for label in labels:
            self.assertTrue(label.startswith("ISO4762_M5_10_Fixed"), label)
        # A joint named again after the fastener keeps its size as well
        joints[1].Label = FastenerLinkObject.jointLabel(self.fastener)
        self.assertTrue(joints[1].Label.startswith("ISO4762_M5_10_Fixed"), joints[1].Label)
        self.assertNotEqual(joints[1].Label, joints[0].Label)

    def testSaveAndRestore(self):
        link = self.makeLink([self.hole(20), self.hole(50)])
        name = link.Name
        path = os.path.join(tempfile.mkdtemp(), "TestFastenerLink.FCStd")
        self.doc.saveAs(path)
        App.closeDocument(self.doc.Name)
        self.doc = App.openDocument(path)
        link = self.doc.getObject(name)
        self.assertTrue(FastenerLinkObject.isFastenerLink(link))
        fasteners = FastenerLinkObject.components(link)
        self.assertEqual(len(fasteners), 2)
        for component in fasteners:
            self.assertIsNotNone(FastenerLinkObject.attachment(component)[0])
