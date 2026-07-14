# ***************************************************************************
# *   Copyright (c) 2026 Justin Adams - Q2 Computing LLC                    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

import unittest
import os
import tempfile

from femsolver.calculix.spatial_hash_merger import SpatialHashNodeMerger, merge_inp_nodes


class TestSpatialHashMerger(unittest.TestCase):

    def test_single_node_stored(self):
        m = SpatialHashNodeMerger(tolerance=0.1)
        m.add_node(1, 0.0, 0.0, 0.0)
        nodes, node_map = m.get_merged_nodes()
        self.assertEqual(len(nodes), 1)
        self.assertEqual(node_map[1], 0)

    def test_coincident_nodes_merged(self):
        m = SpatialHashNodeMerger(tolerance=0.1)
        m.add_node(1, 0.0, 0.0, 0.0)
        m.add_node(2, 0.0, 0.0, 0.0)
        nodes, node_map = m.get_merged_nodes()
        self.assertEqual(len(nodes), 1)
        self.assertEqual(node_map[1], node_map[2])

    def test_distant_nodes_preserved(self):
        m = SpatialHashNodeMerger(tolerance=0.1)
        m.add_node(1, 0.0, 0.0, 0.0)
        m.add_node(2, 1.0, 0.0, 0.0)
        nodes, node_map = m.get_merged_nodes()
        self.assertEqual(len(nodes), 2)
        self.assertNotEqual(node_map[1], node_map[2])

    def test_within_tolerance_merged(self):
        m = SpatialHashNodeMerger(tolerance=0.1)
        m.add_node(1, 0.0, 0.0, 0.0)
        m.add_node(2, 0.05, 0.05, 0.05)
        nodes, node_map = m.get_merged_nodes()
        self.assertEqual(len(nodes), 1)

    def test_just_outside_tolerance_preserved(self):
        m = SpatialHashNodeMerger(tolerance=0.1)
        m.add_node(1, 0.0, 0.0, 0.0)
        m.add_node(2, 0.11, 0.0, 0.0)
        nodes, node_map = m.get_merged_nodes()
        self.assertEqual(len(nodes), 2)

    def test_non_sequential_ids_handled(self):
        m = SpatialHashNodeMerger(tolerance=0.1)
        m.add_node(100, 0.0, 0.0, 0.0)
        m.add_node(200, 1.0, 0.0, 0.0)
        m.add_node(300, 0.0, 0.0, 0.0)
        nodes, node_map = m.get_merged_nodes()
        self.assertEqual(len(nodes), 2)
        self.assertEqual(node_map[100], node_map[300])
        self.assertNotEqual(node_map[100], node_map[200])

    def test_negative_coordinates(self):
        m = SpatialHashNodeMerger(tolerance=0.1)
        m.add_node(1, -1.0, -1.0, -1.0)
        m.add_node(2, -1.0, -1.0, -1.0)
        nodes, node_map = m.get_merged_nodes()
        self.assertEqual(len(nodes), 1)
        self.assertEqual(node_map[1], node_map[2])

    def test_large_coordinate_values(self):
        m = SpatialHashNodeMerger(tolerance=0.1)
        m.add_node(1, 1234.567, 8901.234, 5678.901)
        m.add_node(2, 1234.567, 8901.234, 5678.901)
        nodes, node_map = m.get_merged_nodes()
        self.assertEqual(len(nodes), 1)

    def test_nodes_on_cell_boundary(self):
        m = SpatialHashNodeMerger(tolerance=0.1)
        m.add_node(1, 0.0, 0.0, 0.0)
        m.add_node(2, 0.1, 0.0, 0.0)
        nodes, _ = m.get_merged_nodes()
        self.assertIn(len(nodes), [1, 2])

    def test_on_complexity(self):
        import time

        def time_n_nodes(n):
            m = SpatialHashNodeMerger(tolerance=0.1)
            start = time.perf_counter()
            for i in range(n):
                m.add_node(i, float(i) * 0.5, 0.0, 0.0)
            return time.perf_counter() - start

        t_1k = time_n_nodes(1000)
        t_10k = time_n_nodes(10000)
        if t_1k > 0:
            ratio = t_10k / t_1k
            self.assertLess(
                ratio, 30, f"Complexity worse than O(N): 10x nodes took {ratio:.1f}x longer"
            )

    def test_large_mesh_does_not_crash(self):
        m = SpatialHashNodeMerger(tolerance=0.1)
        for i in range(100000):
            m.add_node(i, float(i) * 0.5, 0.0, 0.0)
        nodes, node_map = m.get_merged_nodes()
        self.assertEqual(len(nodes), 100000)


class TestMergeInpNodes(unittest.TestCase):

    def _make_inp(self, content):
        f = tempfile.NamedTemporaryFile(mode="w", suffix=".inp", delete=False, encoding="latin-1")
        f.write(content)
        f.close()
        return f.name

    def _count_nodes(self, path):
        node_lines = []
        in_node = False
        with open(path, "r", encoding="latin-1") as f:
            for line in f:
                s = line.strip().upper()
                if s.startswith("*NODE"):
                    in_node = True
                    continue
                if in_node:
                    if s.startswith("*"):
                        in_node = False
                        continue
                    if line.strip():
                        node_lines.append(line.strip())
        return node_lines

    def test_single_mesh_node_count_unchanged(self):
        """Single mesh with no coincident nodes — count unchanged."""
        inp = self._make_inp(
            "*Node, NSET=Nall\n"
            "1, 0.0, 0.0, 0.0\n"
            "2, 1.0, 0.0, 0.0\n"
            "3, 1.0, 1.0, 0.0\n"
            "4, 0.0, 1.0, 0.0\n"
            "*Element, TYPE=S4\n"
            "1, 1, 2, 3, 4\n"
        )
        try:
            merge_inp_nodes(inp, tolerance=0.1)
            node_lines = self._count_nodes(inp)
            self.assertEqual(len(node_lines), 4)
        finally:
            os.unlink(inp)

    def test_coincident_nodes_reduced(self):
        """Single mesh with coincident nodes — duplicates removed."""
        inp = self._make_inp(
            "*Node, NSET=Nall\n"
            "1, 0.0, 0.0, 0.0\n"
            "2, 1.0, 0.0, 0.0\n"
            "3, 0.0, 0.0, 0.0\n"
            "*Element, TYPE=S3\n"
            "1, 1, 2, 3\n"
        )
        try:
            merge_inp_nodes(inp, tolerance=0.1)
            node_lines = self._count_nodes(inp)
            self.assertEqual(len(node_lines), 2)
        finally:
            os.unlink(inp)

    def test_multiple_node_blocks_merged(self):
        """
        Replicates the actual FreeCAD bug: two *NODE blocks with
        colliding IDs from different meshes.

        Cube nodes: 1=(10,0,10), 2=(10,-10,10), 3=(0,10,10)
        Line nodes: 1=(10,0,10) [coincident with cube node 1], 2=(5,5,5) [unique]

        Expected: 4 unique nodes after merge (3 + 2 - 1 coincident).
        """
        inp = self._make_inp(
            "** Cube mesh\n"
            "*Node, NSET=Nall\n"
            "1, 10, 0, 10\n"
            "2, 10, -10, 10\n"
            "3, 0, 10, 10\n"
            "*Element, TYPE=C3D4\n"
            "1, 1, 2, 3, 1\n"
            "** Line mesh appended - node IDs collide\n"
            "*Node, NSET=Nall\n"
            "1, 10, 0, 10\n"
            "2, 5, 5, 5\n"
            "*Element, TYPE=B31, ELSET=Eedges\n"
            "1, 1, 2\n"
        )
        try:
            merge_inp_nodes(inp, tolerance=0.1)
            node_lines = self._count_nodes(inp)

            # 3 cube + 2 line - 1 coincident = 4 unique
            self.assertEqual(len(node_lines), 4)

            # No duplicate IDs
            ids = [int(n.split(",")[0].strip()) for n in node_lines]
            self.assertEqual(len(ids), len(set(ids)))

            # Sequential from 1
            self.assertEqual(ids, list(range(1, len(ids) + 1)))

            # Unique line mesh node (5,5,5) is preserved
            found_555 = any(
                abs(float(n.split(",")[1]) - 5) < 0.1
                and abs(float(n.split(",")[2]) - 5) < 0.1
                and abs(float(n.split(",")[3]) - 5) < 0.1
                for n in node_lines
            )
            self.assertTrue(found_555, "Unique line mesh node (5,5,5) not found")
        finally:
            os.unlink(inp)

    def test_no_duplicate_ids_after_merge(self):
        """Two *NODE blocks with colliding IDs — no duplicates in output."""
        inp = self._make_inp(
            "*Node, NSET=Nall\n"
            "1, 0.0, 0.0, 0.0\n"
            "2, 1.0, 0.0, 0.0\n"
            "*Node, NSET=Nall\n"
            "1, 0.0, 0.0, 0.0\n"
            "3, 2.0, 0.0, 0.0\n"
            "*Element, TYPE=B31\n"
            "1, 1, 2\n"
        )
        try:
            merge_inp_nodes(inp, tolerance=0.1)
            node_lines = self._count_nodes(inp)
            ids = [int(n.split(",")[0].strip()) for n in node_lines]
            self.assertEqual(len(ids), len(set(ids)), "Duplicate IDs found")
            self.assertEqual(len(node_lines), 3)
        finally:
            os.unlink(inp)

    def test_sequential_ids_after_merge(self):
        """Output node IDs are sequential starting from 1."""
        inp = self._make_inp(
            "*Node, NSET=Nall\n"
            "1, 0.0, 0.0, 0.0\n"
            "2, 1.0, 0.0, 0.0\n"
            "*Node, NSET=Nall\n"
            "1, 0.0, 0.0, 0.0\n"
            "2, 2.0, 0.0, 0.0\n"
            "*Element, TYPE=B31\n"
            "1, 1, 2\n"
        )
        try:
            merge_inp_nodes(inp, tolerance=0.1)
            node_lines = self._count_nodes(inp)
            ids = [int(n.split(",")[0].strip()) for n in node_lines]
            self.assertEqual(ids, list(range(1, len(ids) + 1)), "Node IDs are not sequential")
        finally:
            os.unlink(inp)

    def test_real_freecad_mixed_mesh_node_count(self):
        """
        Integration test replicating the real FreeCAD scenario.
        Cube mesh 5 nodes + Line mesh 2 nodes - 2 coincident = 5 unique.
        Uses actual coordinates observed from FreeCAD writeABAQUS output.
        """
        inp = self._make_inp(
            "*Node, NSET=Nall\n"
            "1, 10, 0, 10\n"
            "2, 10, -10, 10\n"
            "3, 0, 10, 10\n"
            "4, 0, 10, 0\n"
            "5, 10, 0, 0\n"
            "*Element, TYPE=C3D4\n"
            "1, 1, 2, 3, 4\n"
            "** Line mesh\n"
            "*Node, NSET=Nall\n"
            "1, 10, 0, 10\n"
            "2, 10, -10, 10\n"
            "*Element, TYPE=B31, ELSET=Eedges\n"
            "1, 1, 2\n"
        )
        try:
            merge_inp_nodes(inp, tolerance=0.1)
            node_lines = self._count_nodes(inp)

            # 5 cube nodes + 2 line nodes - 2 coincident = 5 unique
            self.assertEqual(len(node_lines), 5)

            ids = [int(n.split(",")[0].strip()) for n in node_lines]
            self.assertEqual(len(ids), len(set(ids)), "Duplicate IDs found")
            self.assertEqual(ids, list(range(1, len(ids) + 1)))
        finally:
            os.unlink(inp)


if __name__ == "__main__":
    unittest.main(verbosity=2)
