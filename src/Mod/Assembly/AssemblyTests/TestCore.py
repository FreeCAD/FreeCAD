# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
#                                                                           *
#    Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
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
# ***************************************************************************/

import FreeCAD as App
import Part
import unittest

import UtilsAssembly
import JointObject


def _msg(text, end="\n"):
    """Write messages to the console including the line ending."""
    App.Console.PrintMessage(text + end)


class AssemblyTestBase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """setUpClass()...
        This method is called upon instantiation of this test class.  Add code and objects here
        that are needed for the duration of the test() methods in this class.  In other words,
        set up the 'global' test environment here; use the `setUp()` method to set up a 'local'
        test environment.
        This method does not have access to the class `self` reference, but it
        is able to call static methods within this same class.
        """
        pass

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...
        This method is called prior to destruction of this test class.  Add code and objects here
        that cleanup the test environment after the test() methods in this class have been executed.
        This method does not have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        """
        pass

    # Setup and tear down methods called before and after each unit test
    def setUp(self):
        """setUp()...
        This method is called prior to each `test()` method.  Add code and objects here
        that are needed for multiple `test()` methods.
        """
        doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != doc_name:
                App.newDocument(doc_name)
        else:
            App.newDocument(doc_name)
        App.setActiveDocument(doc_name)
        self.doc = App.ActiveDocument

        self.assembly = App.ActiveDocument.addObject("Assembly::AssemblyObject", "Assembly")
        if self.assembly:
            self.jointgroup = self.assembly.newObject("Assembly::JointGroup", "Joints")

        _msg("  Temporary document '{}'".format(self.doc.Name))

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        App.closeDocument(self.doc.Name)


class TestCore(AssemblyTestBase):
    def _create_presolve_joint(
        self, joint_type, fixed_placement, moving_placement, moving_first=False
    ):
        self.assembly.Type = "Assembly"
        fixed = self.assembly.newObject("Part::Box", "FixedPart")
        moving = self.assembly.newObject("Part::Box", "MovingPart")
        fixed.Placement = fixed_placement
        moving.Placement = moving_placement
        ground = self.jointgroup.newObject("App::FeaturePython", "GroundedJoint")
        JointObject.GroundedJoint(ground, fixed)
        joint = self.jointgroup.newObject("App::FeaturePython", "Joint")
        JointObject.Joint(joint, JointObject.JointTypes.index(joint_type))
        if App.GuiUp:
            JointObject.ViewProviderJoint(joint.ViewObject)
        parts = [moving, fixed] if moving_first else [fixed, moving]
        joint.Reference1 = (parts[0], ["", ""])
        joint.Reference2 = (parts[1], ["", ""])
        self.assertIs(joint.Proxy.getAssembly(joint), self.assembly)
        return fixed, moving, joint

    def test_slider_presolve_preserves_axial_separation(self):
        rotations = [
            App.Rotation(),
            App.Rotation(App.Vector(1, 0, 0), -90),
            App.Rotation(15, 25, 35),
        ]
        for rotation in rotations:
            for distance in (-35, 35):
                for moving_first in (False, True):
                    with self.subTest(
                        rotation=rotation, distance=distance, moving_first=moving_first
                    ):
                        fixed_plc = App.Placement(App.Vector(10, 20, 30), rotation)
                        moving_plc = fixed_plc * App.Placement(
                            App.Vector(0, 0, distance), App.Rotation()
                        )
                        fixed, moving, joint = self._create_presolve_joint(
                            "Slider", fixed_plc, moving_plc, moving_first
                        )
                        self.assertTrue(joint.Proxy.matchJCS(joint))
                        self.assertTrue(fixed.Placement.isSame(fixed_plc, 1e-6))
                        self.assertTrue(moving.Placement.isSame(moving_plc, 1e-6))

    def test_slider_presolve_aligns_offset_connectors(self):
        fixed_plc = App.Placement(App.Vector(10, 20, 30), App.Rotation(15, 25, 35))
        moving_plc = App.Placement(App.Vector(40, 50, 60), App.Rotation(45, 55, 65))
        fixed, moving, joint = self._create_presolve_joint("Slider", fixed_plc, moving_plc)
        joint.Detach1 = True
        joint.Detach2 = True
        joint.Placement1 = App.Placement(App.Vector(2, 3, 4), App.Rotation(10, 20, 30))
        joint.Placement2 = App.Placement(App.Vector(5, 6, 7), App.Rotation(30, 20, 10))
        fixed_jcs = UtilsAssembly.getJcsGlobalPlc(joint.Placement1, joint.Reference1)
        moving_jcs = UtilsAssembly.getJcsGlobalPlc(joint.Placement2, joint.Reference2)
        distance = (fixed_jcs.inverse() * moving_jcs).Base.z
        same_direction = joint.Proxy.areJcsSameDir(joint)

        self.assertTrue(joint.Proxy.matchJCS(joint))
        moved_jcs = UtilsAssembly.getJcsGlobalPlc(joint.Placement2, joint.Reference2)
        if not same_direction:
            moved_jcs = UtilsAssembly.flipPlacement(moved_jcs)
        relative_jcs = fixed_jcs.inverse() * moved_jcs
        self.assertAlmostEqual(relative_jcs.Base.x, 0, places=6)
        self.assertAlmostEqual(relative_jcs.Base.y, 0, places=6)
        self.assertAlmostEqual(relative_jcs.Base.z, distance, places=6)
        self.assertTrue(relative_jcs.Rotation.isSame(App.Rotation(), 1e-6))
        self.assertTrue(fixed.Placement.isSame(fixed_plc, 1e-6))
        joint.Proxy.undoPreSolve(joint)
        self.assertTrue(moving.Placement.isSame(moving_plc, 1e-6))

    def test_fixed_presolve_still_matches_origins(self):
        fixed_plc = App.Placement(App.Vector(10, 20, 30), App.Rotation(15, 25, 35))
        moving_plc = fixed_plc * App.Placement(App.Vector(0, 0, 35), App.Rotation())
        fixed, moving, joint = self._create_presolve_joint("Fixed", fixed_plc, moving_plc)
        self.assertTrue(joint.Proxy.matchJCS(joint))
        self.assertTrue(fixed.Placement.isSame(fixed_plc, 1e-6))
        self.assertTrue(moving.Placement.isSame(fixed_plc, 1e-6))

    def test_slider_presolve_flipped_axis_keeps_separation(self):
        flip = App.Rotation(App.Vector(1, 0, 0), 180)
        for reverse in (False, True):
            with self.subTest(reverse=reverse):
                fixed_plc = App.Placement(App.Vector(10, 20, 30), App.Rotation(15, 25, 35))
                moving_plc = fixed_plc * App.Placement(
                    App.Vector(0, 0, 35), App.Rotation() if reverse else flip
                )
                fixed, moving, joint = self._create_presolve_joint("Slider", fixed_plc, moving_plc)
                self.assertTrue(joint.Proxy.matchJCS(joint, reverse=reverse))
                expected = fixed_plc * App.Placement(App.Vector(0, 0, 35), flip)
                self.assertTrue(moving.Placement.isSame(expected, 1e-6))
                self.assertTrue(fixed.Placement.isSame(fixed_plc, 1e-6))

    def test_slider_expression_recompute_preserves_axial_separation(self):
        fixed_plc = App.Placement()
        moving_plc = App.Placement(App.Vector(0, 0, 35), App.Rotation())
        fixed, moving, joint = self._create_presolve_joint("Slider", fixed_plc, moving_plc)
        variables = self.doc.addObject("App::VarSet", "Offsets")
        variables.addProperty("App::PropertyLength", "ConnectorOffset")
        variables.ConnectorOffset = 1
        joint.setExpression("Offset2.Base.x", "Offsets.ConnectorOffset")
        self.doc.recompute()
        moving.Placement = App.Placement(App.Vector(-1, 0, 35), App.Rotation())

        variables.ConnectorOffset = 2
        self.doc.recompute()
        self.assertTrue(fixed.Placement.isSame(fixed_plc, 1e-6))
        self.assertAlmostEqual(moving.Placement.Base.x, -2, places=6)
        self.assertAlmostEqual(moving.Placement.Base.z, 35, places=6)

    def test_component_count_for_link_array(self):
        source = self.doc.addObject("Part::Box", "ArraySource")
        array = self.assembly.newObject("App::Link", "Array")
        array.LinkedObject = source
        array.ElementCount = 3
        array.ShowElement = False
        self.doc.recompute()
        self.assertEqual(len(array.ElementList), 0)
        self.assertEqual(UtilsAssembly.number_of_components_in(self.assembly), 3)

        array.ShowElement = True
        self.doc.recompute()
        self.assertEqual(UtilsAssembly.number_of_components_in(self.assembly), 3)
        array.ElementList[1].Suppressed = True
        self.doc.recompute()
        self.assertEqual(UtilsAssembly.number_of_components_in(self.assembly), 2)
        array.ElementList[1].Suppressed = False
        self.doc.recompute()
        self.assertEqual(UtilsAssembly.number_of_components_in(self.assembly), 3)

    def test_generated_array_is_one_component(self):
        source = self.doc.addObject("Part::Box", "ArraySource")
        array = self.assembly.newObject("Part::LinkArrayLinear", "Array")
        array.LinkedObject = source
        array.Occurrences = 3
        self.doc.recompute()
        self.assertEqual(UtilsAssembly.number_of_components_in(self.assembly), 1)
        self.assertEqual(UtilsAssembly.getSubMovingParts(array, False), [array])
        self.assertEqual(UtilsAssembly.getObject((array, ["1.Face1"])), array)

    def test_suppressed_link_elements_are_not_movable(self):
        source = self.doc.addObject("Part::Box", "ArraySource")
        array = self.assembly.newObject("App::Link", "Array")
        array.LinkedObject = source
        array.ElementCount = 3
        self.doc.recompute()
        element = array.ElementList[1]
        element.Suppressed = True
        self.doc.recompute()
        self.assertNotIn(element, UtilsAssembly.getMovablePartsWithin(array))
        element.Suppressed = False
        self.doc.recompute()
        self.assertIn(element, UtilsAssembly.getMovablePartsWithin(array))

    def test_assembly_link_synchronizes_element_suppression(self):
        source = self.doc.addObject("Part::Box", "ArraySource")
        array = self.assembly.newObject("App::Link", "Array")
        array.LinkedObject = source
        array.ElementCount = 3
        array.ElementList[1].Suppressed = True
        parent = self.doc.addObject("Assembly::AssemblyObject", "ParentAssembly")
        instance = parent.newObject("Assembly::AssemblyLink", "Instance")
        instance.LinkedObject = self.assembly
        self.doc.recompute()
        local_array = next(obj for obj in instance.Group if obj.TypeId == "App::Link")
        self.assertTrue(local_array.ElementList[1].Suppressed)
        array.ElementList[1].Suppressed = False
        instance.touch()
        self.doc.recompute()
        self.assertFalse(local_array.ElementList[1].Suppressed)

    def test_assembly_link_maps_generated_array_joint(self):
        self._check_generated_array_joint("Face1")

    def test_assembly_link_maps_generated_array_whole_element_joint(self):
        self._check_generated_array_joint("")

    def _check_generated_array_joint(self, sub):
        source = self.doc.addObject("Part::Box", "ArraySource")
        array = self.assembly.newObject("Part::LinkArrayLinear", "Array")
        array.LinkedObject = source
        array.Occurrences = 3
        array.ShowElement = True
        self.doc.recompute()
        joint = self.jointgroup.newObject("App::FeaturePython", "Joint")
        JointObject.Joint(joint, 0)
        joint.Reference1 = (array.ElementList[1], [sub])
        joint.Reference2 = (array.ElementList[2], ["Face1"])
        parent = self.doc.addObject("Assembly::AssemblyObject", "ParentAssembly")
        instance = parent.newObject("Assembly::AssemblyLink", "Instance")
        instance.LinkedObject = self.assembly
        instance.Rigid = False
        self.doc.recompute()
        local_array = next(obj for obj in instance.Group if obj.TypeId == "App::Link")
        local_group = next(obj for obj in instance.Group if obj.TypeId == "Assembly::JointGroup")
        local_joint = local_group.Group[0]
        self.assertEqual(local_joint.Reference1, (local_array, ["1." + sub]))
        self.assertIsNotNone(local_array.getSubObject("1." + sub))

    def test_create_assembly(self):
        """Create an assembly."""
        operation = "Create Assembly Object"
        _msg("  Test '{}'".format(operation))
        self.assertTrue(self.assembly, "'{}' failed".format(operation))

    def test_create_jointGroup(self):
        """Create a joint group in an assembly."""
        operation = "Create JointGroup Object"
        _msg("  Test '{}'".format(operation))
        self.assertTrue(self.jointgroup, "'{}' failed".format(operation))

    def test_create_joint(self):
        """Create a joint in an assembly."""
        operation = "Create Joint Object"
        _msg("  Test '{}'".format(operation))

        joint = self.jointgroup.newObject("App::FeaturePython", "testJoint")
        self.assertTrue(joint, "'{}' failed (FeaturePython creation failed)".format(operation))
        JointObject.Joint(joint, 0)

        self.assertTrue(hasattr(joint, "JointType"), "'{}' failed".format(operation))

    def test_create_grounded_joint(self):
        """Create a grounded joint in an assembly."""
        operation = "Create Grounded Joint Object"
        _msg("  Test '{}'".format(operation))

        groundedjoint = self.jointgroup.newObject("App::FeaturePython", "testJoint")
        self.assertTrue(
            groundedjoint, "'{}' failed (FeaturePython creation failed)".format(operation)
        )

        box = self.assembly.newObject("Part::Box", "Box")

        JointObject.GroundedJoint(groundedjoint, box)

        self.assertTrue(
            hasattr(groundedjoint, "ObjectToGround"),
            "'{}' failed: No attribute 'ObjectToGround'".format(operation),
        )
        self.assertTrue(
            groundedjoint.ObjectToGround == box,
            "'{}' failed: ObjectToGround not set correctly.".format(operation),
        )

    def test_toggle_grounded_joint(self):
        """test grounding and ungrounding a part, added because of github.com/freecad/freecad/issues/28440"""
        operation = "Toggle Grounded Joint"
        _msg("  Test '{}'".format(operation))

        box = self.assembly.newObject("Part::Box", "Box")

        # ground the part
        groundedjoint = self.jointgroup.newObject("App::FeaturePython", "GroundedJoint")
        JointObject.GroundedJoint(groundedjoint, box)
        self.doc.recompute()

        # verify grounded
        self.assertTrue(
            hasattr(groundedjoint, "ObjectToGround"),
            "'{}' failed: No attribute 'ObjectToGround'".format(operation),
        )
        self.assertEqual(
            groundedjoint.ObjectToGround,
            box,
            "'{}' failed: ObjectToGround not set correctly".format(operation),
        )

        # unground the part
        self.doc.removeObject(groundedjoint.Name)
        self.doc.recompute()

        # verify no grounded joints remain in this part
        for joint in self.jointgroup.Group:
            if hasattr(joint, "ObjectToGround"):
                self.assertNotEqual(
                    joint.ObjectToGround,
                    box,
                    "'{}' failed: part still grounded after toggle".format(operation),
                )

    def test_find_placement(self):
        """Test find placement of joint."""
        operation = "Find placement"
        _msg("  Test '{}'".format(operation))

        joint = self.jointgroup.newObject("App::FeaturePython", "testJoint")
        JointObject.Joint(joint, 0)

        L = 2
        W = 3
        H = 7
        box = self.assembly.newObject("Part::Box", "Box")
        box.Length = L
        box.Width = W
        box.Height = H
        box.Placement = App.Placement(App.Vector(10, 20, 30), App.Rotation(15, 25, 35))

        # Step 0 : box with placement. No element selected
        ref = [self.assembly, [box.Name + ".", box.Name + "."]]
        plc = joint.Proxy.findPlacement(joint, ref)
        targetPlc = App.Placement(App.Vector(), App.Rotation())
        self.assertTrue(plc.isSame(targetPlc, 1e-6), "'{}' failed - Step 0".format(operation))

        # Step 1 : box with placement. Face + Vertex
        ref = [self.assembly, [box.Name + ".Face6", box.Name + ".Vertex7"]]
        plc = joint.Proxy.findPlacement(joint, ref)
        targetPlc = App.Placement(App.Vector(L, W, H), App.Rotation())
        self.assertTrue(plc.isSame(targetPlc, 1e-6), "'{}' failed - Step 1".format(operation))

        # Step 2 : box with placement. Edge + Vertex
        ref = [self.assembly, [box.Name + ".Edge8", box.Name + ".Vertex8"]]
        plc = joint.Proxy.findPlacement(joint, ref)
        targetPlc = App.Placement(App.Vector(L, W, 0), App.Rotation(0, -90, 270))
        self.assertTrue(plc.isSame(targetPlc, 1e-6), "'{}' failed - Step 2".format(operation))

        # Step 3 : box with placement. Vertex
        ref = [self.assembly, [box.Name + ".Vertex3", box.Name + ".Vertex3"]]
        plc = joint.Proxy.findPlacement(joint, ref)
        targetPlc = App.Placement(App.Vector(0, W, H), App.Rotation())
        _msg("  plc '{}'".format(plc))
        _msg("  targetPlc '{}'".format(targetPlc))
        self.assertTrue(plc.isSame(targetPlc, 1e-6), "'{}' failed - Step 3".format(operation))

        # Step 4 : box with placement. Face
        ref = [self.assembly, [box.Name + ".Face2", box.Name + ".Face2"]]
        plc = joint.Proxy.findPlacement(joint, ref)
        targetPlc = App.Placement(App.Vector(L, W / 2, H / 2), App.Rotation(0, -90, 180))
        _msg("  plc '{}'".format(plc))
        _msg("  targetPlc '{}'".format(targetPlc))
        self.assertTrue(plc.isSame(targetPlc, 1e-6), "'{}' failed - Step 4".format(operation))

    def test_solve_assembly(self):
        """Test solving an assembly."""
        operation = "Solve assembly"
        _msg("  Test '{}'".format(operation))

        box = self.assembly.newObject("Part::Box", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        box.Placement = App.Placement(App.Vector(10, 20, 30), App.Rotation(15, 25, 35))

        box2 = self.assembly.newObject("Part::Box", "Box")
        box2.Length = 10
        box2.Width = 10
        box2.Height = 10
        box2.Placement = App.Placement(App.Vector(40, 50, 60), App.Rotation(45, 55, 65))

        ground = self.jointgroup.newObject("App::FeaturePython", "GroundedJoint")
        JointObject.GroundedJoint(ground, box2)

        joint = self.jointgroup.newObject("App::FeaturePython", "testJoint")
        JointObject.Joint(joint, 0)

        refs = [
            [box2, ["Face6", "Vertex7"]],
            [box, ["Face6", "Vertex7"]],
        ]

        joint.Proxy.setJointConnectors(joint, refs)

        self.assertTrue(box.Placement.isSame(box2.Placement, 1e-6), "'{}'".format(operation))
