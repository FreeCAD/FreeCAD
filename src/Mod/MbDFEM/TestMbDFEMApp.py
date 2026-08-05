# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import tempfile
import unittest

import FreeCAD as App
import MbDFEM  # noqa: F401
import Part


class MbDFEMAssemblyTest(unittest.TestCase):
    @staticmethod
    def _assembly_folders(assembly):
        return {
            "Assemblies": assembly.getPropertyByName("_assembliesFolder"),
            "FixedParts": assembly.getPropertyByName("_fixedPartsFolder"),
            "Parts": assembly.getPropertyByName("_partsFolder"),
            "Joints": assembly.getPropertyByName("_jointsFolder"),
            "Motions": assembly.getPropertyByName("_motionsFolder"),
            "Actions": assembly.getPropertyByName("_actionsFolder"),
        }

    @staticmethod
    def _part_markers_folder(part):
        return part.getPropertyByName("_markersFolder")

    def test_tree_hierarchy(self):
        document = App.newDocument("MbDFEMTreeTest")

        try:
            assembly = document.addObject("MbDFEM::MbDAssembly", "MbDAssembly1")
            assembly_markers = [
                document.addObject("MbDFEM::MbDMarker", "MbDMarker1"),
                document.addObject("MbDFEM::MbDMarker", "MbDMarker2"),
            ]
            subassemblies = [
                document.addObject("MbDFEM::MbDAssembly", "MbDAssembly2"),
                document.addObject("MbDFEM::MbDAssembly", "MbDAssembly3"),
            ]
            parts = [
                document.addObject("MbDFEM::MbDPart", "MbDPart1"),
                document.addObject("MbDFEM::MbDPart", "MbDPart2"),
            ]
            joints = [
                document.addObject("MbDFEM::MbDJoint", "MbDJoint1"),
                document.addObject("MbDFEM::MbDJoint", "MbDJoint2"),
            ]
            motions = [
                document.addObject("MbDFEM::MbDMotion", "MbDMotion1"),
                document.addObject("MbDFEM::MbDMotion", "MbDMotion2"),
            ]
            actions = [
                document.addObject("MbDFEM::MbDAction", "MbDAction1"),
                document.addObject("MbDFEM::MbDAction", "MbDAction2"),
            ]
            part_markers = [
                [
                    document.addObject("MbDFEM::MbDMarker", "MbDMarker11"),
                    document.addObject("MbDFEM::MbDMarker", "MbDMarker12"),
                ],
                [
                    document.addObject("MbDFEM::MbDMarker", "MbDMarker21"),
                    document.addObject("MbDFEM::MbDMarker", "MbDMarker22"),
                ],
            ]

            for subassembly in subassemblies:
                assembly.addAssembly(subassembly)
            for part, markers in zip(parts, part_markers):
                assembly.addPart(part)
                for marker in markers:
                    part.addMarker(marker)
            for joint in joints:
                assembly.addJoint(joint)
            for motion in motions:
                assembly.addMotion(motion)
            for action in actions:
                assembly.addAction(action)
            joints[0].setMarkers(assembly_markers[0], part_markers[0][0])
            joints[1].setMarkerI(part_markers[0][1])
            joints[1].setMarkerJ(part_markers[1][0])
            motions[0].setMarkers(assembly_markers[0], part_markers[0][1])
            motions[1].setMarkers(assembly_markers[1], part_markers[1][1])
            actions[0].setMarkers(assembly_markers[0], part_markers[1][0])
            actions[1].setMarkers(assembly_markers[1], part_markers[1][1])

            self.assertEqual(assembly.assemblies, subassemblies)
            self.assertEqual(assembly.parts, parts)
            self.assertEqual(assembly.joints, joints)
            self.assertEqual(assembly.motions, motions)
            self.assertEqual(assembly.actions, actions)
            self.assertEqual(joints[0].markerI, assembly_markers[0])
            self.assertEqual(joints[0].markerJ, part_markers[0][0])
            self.assertEqual(joints[1].markerI, part_markers[0][1])
            self.assertEqual(joints[1].markerJ, part_markers[1][0])
            self.assertEqual(motions[0].markerI, assembly_markers[0])
            self.assertEqual(motions[0].markerJ, part_markers[0][1])
            self.assertEqual(actions[0].markerI, assembly_markers[0])
            self.assertEqual(actions[0].markerJ, part_markers[1][0])
            assembly_folders = self._assembly_folders(assembly)
            self.assertEqual(assembly_folders["Assemblies"].Group, subassemblies)
            self.assertEqual(assembly_folders["Parts"].Group, parts)
            self.assertEqual(assembly_folders["FixedParts"].Group, [])
            self.assertEqual(assembly_folders["Joints"].Group, joints)
            self.assertEqual(assembly_folders["Motions"].Group, motions)
            self.assertEqual(assembly_folders["Actions"].Group, actions)

            for part, markers in zip(parts, part_markers):
                self.assertEqual(part.markers, markers)
                self.assertEqual(self._part_markers_folder(part).Group, markers)
                self.assertEqual(part.Group, markers)
                for marker in markers:
                    self.assertEqual(marker.getParentGeoFeatureGroup(), part)
        finally:
            App.closeDocument(document.Name)

    def test_create_relationships_save_and_reopen(self):
        with tempfile.TemporaryDirectory() as directory:
            filename = os.path.join(directory, "MbDFEMAssembly.FCStd")
            document = App.newDocument("MbDFEMTest")

            try:
                assembly = document.addObject("MbDFEM::MbDAssembly", "Assembly")
                subassembly = document.addObject("MbDFEM::MbDAssembly", "Subassembly")
                part = document.addObject("MbDFEM::MbDPart", "Part")
                assembly_marker = document.addObject("MbDFEM::MbDMarker", "AssemblyMarker")
                part_marker = document.addObject("MbDFEM::MbDMarker", "PartMarker")
                joint = document.addObject("MbDFEM::MbDJoint", "Joint")
                motion = document.addObject("MbDFEM::MbDMotion", "Motion")
                action = document.addObject("MbDFEM::MbDAction", "Action")

                self.assertEqual(assembly.TypeId, "MbDFEM::MbDAssembly")
                self.assertEqual(subassembly.TypeId, "MbDFEM::MbDAssembly")
                self.assertEqual(part.TypeId, "MbDFEM::MbDPart")
                self.assertEqual(assembly_marker.TypeId, "MbDFEM::MbDMarker")
                self.assertEqual(joint.TypeId, "MbDFEM::MbDJoint")
                self.assertEqual(motion.TypeId, "MbDFEM::MbDMotion")
                self.assertEqual(action.TypeId, "MbDFEM::MbDAction")

                assembly.Placement.Base = App.Vector(1, 2, 3)
                part.Placement.Base = App.Vector(4, 5, 6)
                assembly_marker.Placement.Base = App.Vector(7, 8, 9)
                joint.jointType = "Gear"
                joint.gearRatio = 2.5
                joint.pitchRadius = 12.0
                gravity = assembly.ensureGravity()
                simulation_parameters = assembly.ensureSimulationParameters()
                animation_parameters = assembly.ensureAnimationParameters()
                gravity.gravity = App.Vector(0, -9.81, 0)
                simulation_parameters.endTime = 2.5
                simulation_parameters.stepSize = 0.002
                simulation_parameters.solverType = "DASSL"
                simulation_parameters.significantDigits = 8
                simulation_parameters.maxIterations = 250
                simulation_parameters.outputInterval = 0.02
                animation_parameters.frameRate = 60
                animation_parameters.playbackSpeed = 0.5
                animation_parameters.loop = False
                animation_parameters.showTrails = True
                animation_parameters.trailLength = 120
                animation_parameters.interpolateFrames = False
                self.assertEqual(joint.jointType, "Gear")
                self.assertEqual(joint.gearRatio, 2.5)
                self.assertEqual(joint.pitchRadius, 12.0)
                self.assertEqual(joint.Label, "Gear MbDJoint")
                self.assertEqual(gravity.TypeId, "MbDFEM::MbDGravity")
                gravity_objects = [
                    obj for obj in reopened.Objects if obj.TypeId == "MbDFEM::MbDGravity"
                ]
                self.assertEqual(gravity_objects, [gravity])
                self.assertEqual(simulation_parameters.TypeId, "MbDFEM::MbDSimulationParameters")
                self.assertEqual(animation_parameters.TypeId, "MbDFEM::MbDAnimationParameters")
                self.assertIs(assembly.getGravity(), gravity)
                self.assertIs(assembly.getSimulationParameters(), simulation_parameters)
                self.assertIs(assembly.getAnimationParameters(), animation_parameters)
                self.assertIs(assembly.ensureGravity(), gravity)
                self.assertIs(assembly.ensureSimulationParameters(), simulation_parameters)
                self.assertIs(assembly.ensureAnimationParameters(), animation_parameters)

                assembly.addPart(part)
                assembly.addPart(part)
                assembly.addAssembly(subassembly)
                assembly.addAssembly(subassembly)
                assembly.addAssembly(assembly)
                assembly.addJoint(joint)
                assembly.addJoint(joint)
                assembly.addMotion(motion)
                assembly.addMotion(motion)
                assembly.addAction(action)
                assembly.addAction(action)
                joint.setMarkers(assembly_marker, part_marker)
                motion.setMarkerI(assembly_marker)
                motion.setMarkerJ(part_marker)
                action.setMarkers(assembly_marker, part_marker)
                part.addMarker(part_marker)

                self.assertEqual(assembly.parts, [part])
                self.assertEqual(assembly.assemblies, [subassembly])
                self.assertEqual(assembly.joints, [joint])
                self.assertEqual(assembly.motions, [motion])
                self.assertEqual(assembly.actions, [action])
                self.assertEqual(joint.markerI, assembly_marker)
                self.assertEqual(joint.markerJ, part_marker)
                self.assertEqual(motion.markerI, assembly_marker)
                self.assertEqual(motion.markerJ, part_marker)
                self.assertEqual(action.markerI, assembly_marker)
                self.assertEqual(action.markerJ, part_marker)
                self.assertEqual(part.markers, [part_marker])
                self.assertEqual(part.Group, [part_marker])
                self.assertEqual(part_marker.getParentGeoFeatureGroup(), part)
                assembly_folders = self._assembly_folders(assembly)
                self.assertEqual(assembly_folders["Assemblies"].Group, [subassembly])
                self.assertEqual(assembly_folders["Parts"].Group, [part])
                self.assertEqual(assembly_folders["FixedParts"].Group, [])
                self.assertEqual(assembly_folders["Joints"].Group, [joint])
                self.assertEqual(assembly_folders["Motions"].Group, [motion])
                self.assertEqual(assembly_folders["Actions"].Group, [action])
                self.assertEqual(self._part_markers_folder(part).Group, [part_marker])

                with self.assertRaises(TypeError):
                    assembly.addPart(assembly_marker)
                with self.assertRaises(TypeError):
                    assembly.addAssembly(part)
                with self.assertRaises(TypeError):
                    assembly.addJoint(part)
                with self.assertRaises(TypeError):
                    assembly.addMotion(part)
                with self.assertRaises(TypeError):
                    assembly.addAction(part)
                with self.assertRaises(TypeError):
                    joint.setMarkerI(part)
                with self.assertRaises(TypeError):
                    joint.setMarkerJ(part)
                with self.assertRaises(TypeError):
                    joint.setMarkers(part, part_marker)
                with self.assertRaises(TypeError):
                    part.addMarker(part)

                document.saveAs(filename)
            finally:
                App.closeDocument(document.Name)

            reopened = App.openDocument(filename)
            try:
                assembly = reopened.getObject("Assembly")
                subassembly = reopened.getObject("Subassembly")
                part = reopened.getObject("Part")
                assembly_marker = reopened.getObject("AssemblyMarker")
                part_marker = reopened.getObject("PartMarker")
                joint = reopened.getObject("Joint")
                motion = reopened.getObject("Motion")
                action = reopened.getObject("Action")
                gravity = reopened.getObject("Assembly_Gravity")
                simulation_parameters = reopened.getObject("Assembly_SimulationParameters")
                animation_parameters = reopened.getObject("Assembly_AnimationParameters")

                self.assertIsNotNone(assembly)
                self.assertEqual(assembly.TypeId, "MbDFEM::MbDAssembly")
                self.assertEqual(subassembly.TypeId, "MbDFEM::MbDAssembly")
                self.assertEqual(part.TypeId, "MbDFEM::MbDPart")
                self.assertEqual(assembly_marker.TypeId, "MbDFEM::MbDMarker")
                self.assertEqual(joint.TypeId, "MbDFEM::MbDJoint")
                self.assertEqual(joint.jointType, "Gear")
                self.assertEqual(joint.gearRatio, 2.5)
                self.assertEqual(joint.pitchRadius, 12.0)
                self.assertEqual(joint.Label, "Gear MbDJoint")
                self.assertEqual(gravity.TypeId, "MbDFEM::MbDGravity")
                self.assertEqual(simulation_parameters.TypeId, "MbDFEM::MbDSimulationParameters")
                self.assertEqual(animation_parameters.TypeId, "MbDFEM::MbDAnimationParameters")
                self.assertIs(assembly.getGravity(), gravity)
                self.assertIs(assembly.getSimulationParameters(), simulation_parameters)
                self.assertIs(assembly.getAnimationParameters(), animation_parameters)
                self.assertEqual(gravity.Label, "Gravity")
                self.assertEqual(simulation_parameters.Label, "SimulationParameters")
                self.assertEqual(animation_parameters.Label, "AnimationParameters")
                self.assertEqual(gravity.gravity, App.Vector(0, -9.81, 0))
                self.assertEqual(simulation_parameters.endTime, 2.5)
                self.assertEqual(simulation_parameters.stepSize, 0.002)
                self.assertEqual(simulation_parameters.solverType, "DASSL")
                self.assertEqual(simulation_parameters.significantDigits, 8)
                self.assertEqual(simulation_parameters.maxIterations, 250)
                self.assertEqual(simulation_parameters.outputInterval, 0.02)
                self.assertEqual(animation_parameters.frameRate, 60)
                self.assertEqual(animation_parameters.playbackSpeed, 0.5)
                self.assertFalse(animation_parameters.loop)
                self.assertTrue(animation_parameters.showTrails)
                self.assertEqual(animation_parameters.trailLength, 120)
                self.assertFalse(animation_parameters.interpolateFrames)
                self.assertEqual(motion.TypeId, "MbDFEM::MbDMotion")
                self.assertEqual(action.TypeId, "MbDFEM::MbDAction")
                self.assertEqual(assembly.Placement.Base, App.Vector(1, 2, 3))
                self.assertEqual(part.Placement.Base, App.Vector(4, 5, 6))
                self.assertEqual(assembly_marker.Placement.Base, App.Vector(7, 8, 9))
                self.assertEqual(assembly.parts, [part])
                self.assertEqual(assembly.assemblies, [subassembly])
                self.assertEqual(assembly.joints, [joint])
                self.assertEqual(assembly.motions, [motion])
                self.assertEqual(assembly.actions, [action])
                self.assertEqual(joint.markerI, assembly_marker)
                self.assertEqual(joint.markerJ, part_marker)
                self.assertEqual(motion.markerI, assembly_marker)
                self.assertEqual(motion.markerJ, part_marker)
                self.assertEqual(action.markerI, assembly_marker)
                self.assertEqual(action.markerJ, part_marker)
                self.assertEqual(part.markers, [part_marker])
                self.assertEqual(part.Group, [part_marker])
                self.assertEqual(part_marker.getParentGeoFeatureGroup(), part)
                assembly_folders = self._assembly_folders(assembly)
                self.assertEqual(assembly_folders["Assemblies"].Group, [subassembly])
                self.assertEqual(assembly_folders["Parts"].Group, [part])
                self.assertEqual(assembly_folders["FixedParts"].Group, [])
                self.assertEqual(assembly_folders["Joints"].Group, [joint])
                self.assertEqual(assembly_folders["Motions"].Group, [motion])
                self.assertEqual(assembly_folders["Actions"].Group, [action])
                self.assertEqual(self._part_markers_folder(part).Group, [part_marker])

            finally:
                App.closeDocument(reopened.Name)

    def test_ground_part_moves_part_to_fixedparts(self):
        document = App.newDocument("MbDFEMGroundPartTest")

        try:
            assembly = document.addObject("MbDFEM::MbDAssembly", "Assembly")
            part = document.addObject("MbDFEM::MbDPart", "Part")

            assembly.addPart(part)
            self.assertEqual(assembly.parts, [part])
            self.assertEqual(assembly.fixedparts, [])
            self.assertEqual(assembly.getPartsFolder().Group, [part])
            self.assertIn(part, assembly.Group)
            self.assertEqual(part.getParentGeoFeatureGroup(), assembly)

            assembly.groundPart(part)
            self.assertEqual(assembly.parts, [])
            self.assertEqual(assembly.fixedparts, [part])
            self.assertEqual(assembly.getPartsFolder().Group, [])
            self.assertEqual(assembly.getFixedPartsFolder().Group, [part])
            self.assertIn(part, assembly.Group)
            self.assertEqual(part.getParentGeoFeatureGroup(), assembly)
            self.assertIs(document.getObject("Part"), part)
        finally:
            App.closeDocument(document.Name)

    def test_joint_type_updates_auto_label_but_preserves_custom_label(self):
        document = App.newDocument("MbDFEMJointLabelTest")

        try:
            joint = document.addObject("MbDFEM::MbDJoint", "Joint")
            joint.Label = "MbDJoint (A, B)"
            joint.jointType = "Revolute"
            self.assertEqual(joint.Label, "Revolute MbDJoint")

            joint.jointType = "RackPinion"
            self.assertEqual(joint.Label, "RackPinion MbDJoint")

            joint.Label = "Drive coupling"
            joint.jointType = "Gear"
            self.assertEqual(joint.Label, "Drive coupling")
        finally:
            App.closeDocument(document.Name)

    def test_tree_folder_subobject_paths_resolve_to_contained_objects(self):
        document = App.newDocument("MbDFEMFolderSubobjectPathTest")

        try:
            assembly = document.addObject("MbDFEM::MbDAssembly", "Assembly")
            part = document.addObject("MbDFEM::MbDPart", "Part")
            fixed_part = document.addObject("MbDFEM::MbDPart", "FixedPart")
            marker = document.addObject("MbDFEM::MbDMarker", "Marker")
            gravity = assembly.ensureGravity()
            simulation_parameters = assembly.ensureSimulationParameters()
            animation_parameters = assembly.ensureAnimationParameters()

            fixed_part.Shape = Part.makeBox(1, 1, 1)
            assembly.addPart(part)
            assembly.addFixedPart(fixed_part)
            part.addMarker(marker)

            part_path = f"{assembly.getPartsFolder().Name}.{part.Name}."
            fixed_part_path = f"{fixed_part.Name}."
            fixed_part_edge_path = f"{fixed_part.Name}.Edge1"
            fixed_part_face_path = f"{fixed_part.Name}.Face1"
            marker_path = f"{part.getMarkersFolder().Name}.{marker.Name}."
            gravity_path = f"{gravity.Name}."
            simulation_path = f"{simulation_parameters.Name}."
            animation_path = f"{animation_parameters.Name}."

            self.assertIs(assembly.getSubObject(part_path, retType=1), part)
            self.assertIs(assembly.getSubObject(fixed_part_path, retType=1), fixed_part)
            self.assertIs(assembly.getSubObject(fixed_part_edge_path, retType=1), fixed_part)
            self.assertIsNotNone(assembly.getSubObject(fixed_part_edge_path))
            self.assertIs(assembly.getSubObject(fixed_part_face_path, retType=1), fixed_part)
            self.assertIsNotNone(assembly.getSubObject(fixed_part_face_path))
            self.assertIs(part.getSubObject(marker_path, retType=1), marker)
            self.assertIs(assembly.getSubObject(gravity_path, retType=1), gravity)
            self.assertIs(assembly.getSubObject(simulation_path, retType=1), simulation_parameters)
            self.assertIs(assembly.getSubObject(animation_path, retType=1), animation_parameters)
        finally:
            App.closeDocument(document.Name)

    def test_child_element_visibility_maps_to_contained_objects(self):
        document = App.newDocument("MbDFEMElementVisibilityTest")

        try:
            assembly = document.addObject("MbDFEM::MbDAssembly", "Assembly")
            part = document.addObject("MbDFEM::MbDPart", "Pin_MbDPart")
            marker = document.addObject("MbDFEM::MbDMarker", "Marker")
            part.Label = "Pin001"
            marker.Label = "Marker001"

            assembly.addPart(part)
            part.addMarker(marker)

            self.assertEqual(assembly.isElementVisible("Pin_MbDPart"), 1)
            self.assertEqual(assembly.isElementVisible("Pin001"), -1)
            self.assertEqual(part.isElementVisible("Marker"), 1)
            self.assertEqual(part.isElementVisible("Marker001"), -1)

            self.assertEqual(assembly.setElementVisible("Pin_MbDPart", False), 0)
            self.assertFalse(part.Visibility)
            self.assertEqual(assembly.isElementVisible("Pin_MbDPart"), 0)
            self.assertEqual(assembly.setElementVisible("Pin001", True), -1)
            self.assertFalse(part.Visibility)
            self.assertEqual(assembly.setElementVisible("Pin_MbDPart", True), 1)
            self.assertTrue(part.Visibility)

            self.assertEqual(part.setElementVisible("Marker", False), 0)
            self.assertFalse(marker.Visibility)
            self.assertEqual(part.isElementVisible("Marker001"), -1)
            self.assertEqual(part.setElementVisible("Marker001", True), -1)
            self.assertFalse(marker.Visibility)
            self.assertEqual(part.setElementVisible("Marker", True), 1)
            self.assertTrue(marker.Visibility)
        finally:
            App.closeDocument(document.Name)

    def test_duplicate_labels_do_not_affect_internal_name_resolution(self):
        document = App.newDocument("MbDFEMDuplicateLabelTest")

        try:
            assembly = document.addObject("MbDFEM::MbDAssembly", "Assembly")
            first_part = document.addObject("MbDFEM::MbDPart", "FirstPart")
            second_part = document.addObject("MbDFEM::MbDPart", "SecondPart")
            first_marker = document.addObject("MbDFEM::MbDMarker", "FirstMarker")
            second_marker = document.addObject("MbDFEM::MbDMarker", "SecondMarker")

            first_part.Label = "Pin"
            second_part.Label = "Pin"
            first_marker.Label = "JointMarker"
            second_marker.Label = "JointMarker"

            assembly.addPart(first_part)
            assembly.addPart(second_part)
            first_part.addMarker(first_marker)
            second_part.addMarker(second_marker)

            self.assertEqual(assembly.setElementVisible("FirstPart", False), 0)
            self.assertFalse(first_part.Visibility)
            self.assertTrue(second_part.Visibility)
            self.assertEqual(assembly.setElementVisible("Pin", True), -1)
            self.assertFalse(first_part.Visibility)
            self.assertTrue(second_part.Visibility)

            self.assertEqual(second_part.setElementVisible("SecondMarker", False), 0)
            self.assertTrue(first_marker.Visibility)
            self.assertFalse(second_marker.Visibility)
            self.assertEqual(second_part.setElementVisible("JointMarker", True), -1)
            self.assertTrue(first_marker.Visibility)
            self.assertFalse(second_marker.Visibility)
        finally:
            App.closeDocument(document.Name)

    def test_assembly_placement_defines_parts_local_coordinate_system(self):
        document = App.newDocument("MbDFEMAssemblyPlacementTest")

        try:
            assembly = document.addObject("MbDFEM::MbDAssembly", "Assembly")
            part = document.addObject("MbDFEM::MbDPart", "Part")
            fixed_part = document.addObject("MbDFEM::MbDPart", "FixedPart")

            part.Placement.Base = App.Vector(1, 0, 0)
            fixed_part.Placement.Base = App.Vector(0, 2, 0)
            assembly.addPart(part)
            assembly.addFixedPart(fixed_part)
            self.assertIn(part, assembly.Group)
            self.assertIn(fixed_part, assembly.Group)
            self.assertEqual(part.getParentGeoFeatureGroup(), assembly)
            self.assertEqual(fixed_part.getParentGeoFeatureGroup(), assembly)

            assembly.Placement.Base = App.Vector(10, 0, 0)
            self.assertEqual(part.Placement.Base, App.Vector(1, 0, 0))
            self.assertEqual(fixed_part.Placement.Base, App.Vector(0, 2, 0))
            self.assertEqual(assembly.getPlacementOf("Part.", part).Base, App.Vector(11, 0, 0))
            self.assertEqual(
                assembly.getPlacementOf("FixedPart.", fixed_part).Base,
                App.Vector(10, 2, 0),
            )

            assembly.Placement = App.Placement(
                App.Vector(0, 0, 0),
                App.Rotation(App.Vector(0, 0, 1), 90),
            )
            self.assertEqual(part.Placement.Base, App.Vector(1, 0, 0))
            self.assertEqual(fixed_part.Placement.Base, App.Vector(0, 2, 0))
            self.assertLess(
                assembly.getPlacementOf("Part.", part).Base.distanceToPoint(App.Vector(0, 1, 0)),
                1e-7,
            )
            self.assertLess(
                assembly.getPlacementOf("FixedPart.", fixed_part).Base.distanceToPoint(
                    App.Vector(-2, 0, 0)
                ),
                1e-7,
            )
        finally:
            App.closeDocument(document.Name)

    def test_part_placement_defines_marker_local_coordinate_system(self):
        document = App.newDocument("MbDFEMPartPlacementTest")

        try:
            part = document.addObject("MbDFEM::MbDPart", "Part")
            marker = document.addObject("MbDFEM::MbDMarker", "Marker")

            marker.Placement.Base = App.Vector(1, 0, 0)
            part.addMarker(marker)

            self.assertEqual(part.markers, [marker])
            self.assertEqual(part.getMarkersFolder().Group, [marker])
            self.assertEqual(part.Group, [marker])
            self.assertEqual(marker.getParentGeoFeatureGroup(), part)

            part.Placement.Base = App.Vector(10, 0, 0)
            self.assertEqual(marker.Placement.Base, App.Vector(1, 0, 0))
            self.assertEqual(part.getPlacementOf("Marker.", marker).Base, App.Vector(11, 0, 0))

            part.Placement = App.Placement(
                App.Vector(0, 0, 0),
                App.Rotation(App.Vector(0, 0, 1), 90),
            )
            self.assertEqual(marker.Placement.Base, App.Vector(1, 0, 0))
            self.assertLess(
                part.getPlacementOf("Marker.", marker).Base.distanceToPoint(App.Vector(0, 1, 0)),
                1e-7,
            )
        finally:
            App.closeDocument(document.Name)

    def test_reparent_part_between_assemblies_removes_old_semantic_links(self):
        document = App.newDocument("MbDFEMPartReparentTest")

        try:
            first_assembly = document.addObject("MbDFEM::MbDAssembly", "FirstAssembly")
            second_assembly = document.addObject("MbDFEM::MbDAssembly", "SecondAssembly")
            part = document.addObject("MbDFEM::MbDPart", "Part")

            first_assembly.addPart(part)
            self.assertEqual(first_assembly.parts, [part])
            self.assertEqual(first_assembly.getPartsFolder().Group, [part])
            self.assertIn(part, first_assembly.Group)
            self.assertEqual(part.getParentGeoFeatureGroup(), first_assembly)

            second_assembly.addPart(part)

            self.assertEqual(first_assembly.parts, [])
            self.assertEqual(first_assembly.getPartsFolder().Group, [])
            self.assertNotIn(part, first_assembly.Group)
            self.assertEqual(second_assembly.parts, [part])
            self.assertEqual(second_assembly.getPartsFolder().Group, [part])
            self.assertIn(part, second_assembly.Group)
            self.assertEqual(part.getParentGeoFeatureGroup(), second_assembly)
        finally:
            App.closeDocument(document.Name)

    def test_reparent_marker_removes_old_part_semantic_links(self):
        document = App.newDocument("MbDFEMMarkerReparentTest")

        try:
            first_part = document.addObject("MbDFEM::MbDPart", "FirstPart")
            second_part = document.addObject("MbDFEM::MbDPart", "SecondPart")
            marker = document.addObject("MbDFEM::MbDMarker", "Marker")

            first_part.addMarker(marker)
            second_part.addMarker(marker)

            self.assertEqual(first_part.markers, [])
            self.assertEqual(first_part.getMarkersFolder().Group, [])
            self.assertNotIn(marker, first_part.Group)
            self.assertEqual(second_part.markers, [marker])
            self.assertEqual(second_part.getMarkersFolder().Group, [marker])
            self.assertEqual(marker.getParentGeoFeatureGroup(), second_part)
        finally:
            App.closeDocument(document.Name)

    def test_deleting_part_deletes_owned_markers(self):
        document = App.newDocument("MbDFEMDeletePartTest")

        try:
            assembly = document.addObject("MbDFEM::MbDAssembly", "Assembly")
            part = document.addObject("MbDFEM::MbDPart", "Part")
            marker = document.addObject("MbDFEM::MbDMarker", "Marker")

            assembly.addPart(part)
            part.addMarker(marker)
            marker_folder_name = part.getMarkersFolder().Name

            document.removeObject(part.Name)

            self.assertIsNone(document.getObject("Part"))
            self.assertIsNone(document.getObject("Marker"))
            self.assertIsNone(document.getObject(marker_folder_name))
            self.assertEqual(assembly.parts, [])
            self.assertEqual(assembly.getPartsFolder().Group, [])
            self.assertEqual(assembly.Group, [])
        finally:
            App.closeDocument(document.Name)

    def test_remove_marker_detaches_without_deleting_marker(self):
        document = App.newDocument("MbDFEMRemoveMarkerTest")

        try:
            part = document.addObject("MbDFEM::MbDPart", "Part")
            marker = document.addObject("MbDFEM::MbDMarker", "Marker")

            part.addMarker(marker)
            self.assertEqual(part.markers, [marker])
            self.assertEqual(part.getMarkersFolder().Group, [marker])
            self.assertIn(marker, part.Group)

            part.removeMarker(marker)

            self.assertIs(document.getObject("Marker"), marker)
            self.assertEqual(part.markers, [])
            self.assertEqual(part.getMarkersFolder().Group, [])
            self.assertNotIn(marker, part.Group)
            self.assertIsNone(marker.getParentGeoFeatureGroup())
        finally:
            App.closeDocument(document.Name)
