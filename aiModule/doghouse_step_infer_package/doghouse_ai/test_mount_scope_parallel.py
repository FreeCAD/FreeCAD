"""Mount face must be parallel to doghouse-region major plane; hole in bbox center."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path
from unittest import mock

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from doghouse_assembly_features import (
    _hole_edge_in_mount_bbox_center,
    _mount_parallel_to_scope_major,
    _scope_area_weighted_plane_normal,
)


class MountScopeParallelTest(unittest.TestCase):
    def test_scope_area_weighted_prefers_dominant_hemisphere(self):
        faces = [object(), object(), object()]

        def fake_type(face):
            return "plane"

        def fake_plane(face):
            idx = faces.index(face)
            if idx == 0:
                return np.zeros(3), np.array([0.0, 0.0, 1.0])
            if idx == 1:
                return np.zeros(3), np.array([0.0, 0.0, 1.0])
            return np.zeros(3), np.array([1.0, 0.0, 0.0])

        def fake_area(face):
            return {0: 200.0, 1: 180.0, 2: 50.0}[faces.index(face)]

        with mock.patch("doghouse_assembly_features._face_type", side_effect=fake_type), mock.patch(
            "doghouse_assembly_features._plane_info", side_effect=fake_plane
        ), mock.patch("doghouse_assembly_features._surface_area", side_effect=fake_area):
            n = _scope_area_weighted_plane_normal(faces, {0, 1, 2})
        self.assertIsNotNone(n)
        # Two large Z-planes dominate one small X-sidewall.
        self.assertGreater(abs(float(n[2])), 0.9)

    def test_parallel_gate_rejects_orthogonal_sidewall(self):
        major = np.array([0.0, 0.0, 1.0])
        ok, align = _mount_parallel_to_scope_major(np.array([0.0, 0.0, 1.0]), major)
        self.assertTrue(ok)
        self.assertGreater(align, 0.99)
        # Nearly orthogonal sidewall.
        ok2, align2 = _mount_parallel_to_scope_major(np.array([1.0, 0.0, 0.0]), major, min_dot=0.50)
        self.assertFalse(ok2)
        self.assertLess(align2, 0.50)

    def test_hole_edge_requires_bbox_center(self):
        mount_face = object()
        faces = [object()]
        group = {
            "center": [0.0, 0.0, 0.0],
            "axis": [0.0, 0.0, 1.0],
            "face_indices": [0],
        }
        # bbox u,v in [-10,10]; center band is 40% => [-4,4]
        with mock.patch(
            "doghouse_assembly_features._face_plane_bounds",
            return_value=(-10.0, 10.0, -10.0, 10.0),
        ), mock.patch(
            "doghouse_assembly_features._hole_edge_circle_centers",
            return_value=[np.array([0.0, 0.0, 0.0])],
        ):
            ok, c = _hole_edge_in_mount_bbox_center(
                group,
                faces,
                mount_face,
                np.array([0.0, 0.0, 0.0]),
                np.array([0.0, 0.0, 1.0]),
            )
        self.assertTrue(ok)
        self.assertGreater(c, 0.9)

        with mock.patch(
            "doghouse_assembly_features._face_plane_bounds",
            return_value=(-10.0, 10.0, -10.0, 10.0),
        ), mock.patch(
            "doghouse_assembly_features._hole_edge_circle_centers",
            return_value=[np.array([8.0, 0.0, 0.0])],
        ):
            ok2, _ = _hole_edge_in_mount_bbox_center(
                group,
                faces,
                mount_face,
                np.array([0.0, 0.0, 0.0]),
                np.array([0.0, 0.0, 1.0]),
            )
        self.assertFalse(ok2)


if __name__ == "__main__":
    unittest.main()
