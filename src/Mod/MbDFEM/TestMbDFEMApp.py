# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import tempfile
import unittest

import FreeCAD as App
import MbDFEM  # noqa: F401


class MbDFEMAssemblyTest(unittest.TestCase):
    def test_tree_hierarchy(self):
        document = App.newDocument("MbDFEMTreeTest")

        try:
            assembly = document.addObject("MbDFEM::MbDAssembly", "MbDAssembly1")
            assembly_markers = [
                document.addObject("MbDFEM::MbDMarker", "MbDMarker1"),
                document.addObject("MbDFEM::MbDMarker", "MbDMarker2"),
            ]
            parts = [
                document.addObject("MbDFEM::MbDPart", "MbDPart1"),
                document.addObject("MbDFEM::MbDPart", "MbDPart2"),
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

            for marker in assembly_markers:
                assembly.addMarker(marker)
            for part, markers in zip(parts, part_markers):
                assembly.addPart(part)
                for marker in markers:
                    part.addMarker(marker)

            assembly_groups = {group.Label: group for group in assembly.Group}
            self.assertEqual(list(assembly_groups), ["Markers", "Parts"])
            self.assertEqual(assembly_groups["Markers"].Group, assembly_markers)
            self.assertEqual(assembly_groups["Parts"].Group, parts)

            for part, markers in zip(parts, part_markers):
                self.assertEqual([group.Label for group in part.Group], ["Markers"])
                self.assertEqual(part.Group[0].Group, markers)
        finally:
            App.closeDocument(document.Name)

    def test_create_relationships_save_and_reopen(self):
        with tempfile.TemporaryDirectory() as directory:
            filename = os.path.join(directory, "MbDFEMAssembly.FCStd")
            document = App.newDocument("MbDFEMTest")

            try:
                assembly = document.addObject("MbDFEM::MbDAssembly", "Assembly")
                part = document.addObject("MbDFEM::MbDPart", "Part")
                assembly_marker = document.addObject("MbDFEM::MbDMarker", "AssemblyMarker")
                part_marker = document.addObject("MbDFEM::MbDMarker", "PartMarker")

                self.assertEqual(assembly.TypeId, "MbDFEM::MbDAssembly")
                self.assertEqual(part.TypeId, "MbDFEM::MbDPart")
                self.assertEqual(assembly_marker.TypeId, "MbDFEM::MbDMarker")

                assembly.Placement.Base = App.Vector(1, 2, 3)
                part.Placement.Base = App.Vector(4, 5, 6)
                assembly_marker.Placement.Base = App.Vector(7, 8, 9)

                assembly.addPart(part)
                assembly.addPart(part)
                assembly.addMarker(assembly_marker)
                part.addMarker(part_marker)

                self.assertEqual(assembly.parts, [part])
                self.assertEqual(assembly.markers, [assembly_marker])
                self.assertEqual(part.markers, [part_marker])

                assembly_groups = {group.Label: group for group in assembly.Group}
                self.assertEqual(list(assembly_groups), ["Markers", "Parts"])
                self.assertEqual(assembly_groups["Markers"].Group, [assembly_marker])
                self.assertEqual(assembly_groups["Parts"].Group, [part])
                part_groups = {group.Label: group for group in part.Group}
                self.assertEqual(list(part_groups), ["Markers"])
                self.assertEqual(part_groups["Markers"].Group, [part_marker])

                with self.assertRaises(TypeError):
                    assembly.addPart(assembly_marker)
                with self.assertRaises(TypeError):
                    part.addMarker(part)

                document.saveAs(filename)
            finally:
                App.closeDocument(document.Name)

            reopened = App.openDocument(filename)
            try:
                assembly = reopened.getObject("Assembly")
                part = reopened.getObject("Part")
                assembly_marker = reopened.getObject("AssemblyMarker")
                part_marker = reopened.getObject("PartMarker")

                self.assertIsNotNone(assembly)
                self.assertEqual(assembly.TypeId, "MbDFEM::MbDAssembly")
                self.assertEqual(part.TypeId, "MbDFEM::MbDPart")
                self.assertEqual(assembly_marker.TypeId, "MbDFEM::MbDMarker")
                self.assertEqual(assembly.Placement.Base, App.Vector(1, 2, 3))
                self.assertEqual(part.Placement.Base, App.Vector(4, 5, 6))
                self.assertEqual(assembly_marker.Placement.Base, App.Vector(7, 8, 9))
                self.assertEqual(assembly.parts, [part])
                self.assertEqual(assembly.markers, [assembly_marker])
                self.assertEqual(part.markers, [part_marker])

                assembly_groups = {group.Label: group for group in assembly.Group}
                self.assertEqual(list(assembly_groups), ["Markers", "Parts"])
                self.assertEqual(assembly_groups["Markers"].Group, [assembly_marker])
                self.assertEqual(assembly_groups["Parts"].Group, [part])
                part_groups = {group.Label: group for group in part.Group}
                self.assertEqual(list(part_groups), ["Markers"])
                self.assertEqual(part_groups["Markers"].Group, [part_marker])
            finally:
                App.closeDocument(reopened.Name)
