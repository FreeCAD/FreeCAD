"""Mount selection must prefer typical mount+hole topology over freeform local axes."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path
from unittest import mock

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from doghouse_assembly_features import (
    _coaxial_through_hole,
    _fastener_topology_score,
    _mount_axis_direction_score,
)


class MountTopologyScoreTest(unittest.TestCase):
    def test_coaxial_through_ignores_freeform_pca_axis(self):
        center = np.array([0.0, 0.0, 0.0])
        normal = np.array([0.0, 0.0, 1.0])
        groups = [
            {
                "center": [0.0, 0.0, 1.0],
                "axis": [0.0, 0.0, 1.0],
                "radius": 0.0,
                "axis_source": "freeform_pca",
                "face_indices": [9],
            }
        ]
        self.assertFalse(_coaxial_through_hole(center, normal, groups))

    def test_coaxial_through_keeps_analytic_radius_hole(self):
        center = np.array([0.0, 0.0, 0.0])
        normal = np.array([0.0, 0.0, 1.0])
        groups = [
            {
                "center": [0.0, 0.0, 1.0],
                "axis": [0.0, 0.0, 1.0],
                "radius": 2.5,
                "face_indices": [9],
            }
        ]
        self.assertTrue(_coaxial_through_hole(center, normal, groups))

    def test_axis_direction_score_weak_for_freeform_pca(self):
        groups = [
            {
                "axis": [0.0, 0.0, 1.0],
                "axis_source": "freeform_pca",
                "radius": 0.0,
            }
        ]
        score = _mount_axis_direction_score(np.array([0.0, 0.0, 1.0]), groups)
        self.assertAlmostEqual(score, 0.5)
        # Analytic hole present: freeform ignored, signed analytic used.
        groups_mixed = [
            {
                "axis": [0.0, 0.0, 1.0],
                "axis_source": "freeform_pca",
                "radius": 0.0,
            },
            {
                "axis": [0.0, 0.0, 1.0],
                "radius": 2.5,
            },
        ]
        score2 = _mount_axis_direction_score(np.array([0.0, 0.0, 1.0]), groups_mixed)
        self.assertAlmostEqual(score2, 1.0)

    def test_fastener_topology_prefers_many_small_radius_walls(self):
        # Fake faces: only indices matter; radius/type come from mocked helpers.
        faces = [object() for _ in range(10)]
        adj = {
            0: {1, 2, 3, 4},  # rich mount-like neighborhood
            5: {6, 7},       # sparse sidewall-like neighborhood
        }
        scope = set(range(10))

        def fake_face_type(face):
            idx = faces.index(face)
            return "cylinder" if idx in {1, 2, 3, 6} else "plane"

        def fake_radius_u_v(face):
            idx = faces.index(face)
            # 1,2,3: fastener R=2; 6: large structural R=10
            radius = {1: 2.0, 2: 2.0, 3: 2.5, 6: 10.0}.get(idx)
            return radius, 3.0, 2.0

        with mock.patch("doghouse_assembly_features._face_type", side_effect=fake_face_type), mock.patch(
            "doghouse_assembly_features._radius_u_v", side_effect=fake_radius_u_v
        ):
            rich = _fastener_topology_score(faces, adj, 0, scope, min_radius=2.0, max_radius=6.0)
            sparse = _fastener_topology_score(faces, adj, 5, scope, min_radius=2.0, max_radius=6.0)
        self.assertGreater(rich, sparse)
        self.assertGreaterEqual(rich, 20.0)  # 3 fastener walls * 10, capped at 4

    def test_topology_bonus_only_when_no_analytic_hole(self):
        from doghouse_assembly_features import _topology_bonus_for_mount_candidate

        freeform_only = [{"radius": 0.0, "axis_source": "freeform_pca", "face_indices": [1]}]
        analytic = [{"radius": 3.0, "face_indices": [2], "source": "vf2"}]
        self.assertGreater(
            _topology_bonus_for_mount_candidate(32.0, freeform_only),
            0.0,
        )
        self.assertEqual(_topology_bonus_for_mount_candidate(32.0, analytic), 0.0)

    def test_topology_bonus_zero_for_neighbor_topology_analytic(self):
        """Neighbor-recovered analytic holes must not also get topology bonus."""
        from doghouse_assembly_features import (
            _topology_bonus_for_mount_candidate,
            _vf2_analytic_mount_bonus,
        )

        recovered = [
            {
                "radius": 3.0,
                "face_indices": [2],
                "source": "neighbor_topology",
            }
        ]
        self.assertEqual(_topology_bonus_for_mount_candidate(20.0, recovered), 0.0)
        self.assertEqual(_vf2_analytic_mount_bonus(recovered), -25.0)
        vf2 = [{"radius": 3.0, "face_indices": [2], "source": "vf2"}]
        self.assertEqual(_vf2_analytic_mount_bonus(vf2), 0.0)

    def test_topology_bonus_capped_for_freeform_only(self):
        from doghouse_assembly_features import _topology_bonus_for_mount_candidate

        freeform_only = [{"radius": 0.0, "axis_source": "freeform_pca", "face_indices": [1]}]
        self.assertEqual(_topology_bonus_for_mount_candidate(40.0, freeform_only), 20.0)

    def test_neighbor_analytic_hole_groups_from_adj(self):
        from doghouse_assembly_features import _neighbor_analytic_hole_groups

        faces = [object() for _ in range(5)]
        adj = {0: {1, 2}, 1: {0}, 2: {0}}
        scope = {0, 1, 2}

        def fake_face_type(face):
            idx = faces.index(face)
            return "cylinder" if idx in {1, 2} else "plane"

        def fake_radius_u_v(face):
            idx = faces.index(face)
            return ({1: 3.0, 2: 10.0}.get(idx), 3.0, 2.0)

        def fake_cylinder_info(face):
            idx = faces.index(face)
            if idx == 1:
                return np.array([0.0, 0.0, 0.0]), np.array([0.0, 1.0, 0.0]), 3.0
            return None, None, None

        def fake_plane_info(face):
            # Mount normal aligned with cylinder axis (+Y).
            return np.array([0.0, 0.0, 0.0]), np.array([0.0, 1.0, 0.0])

        with mock.patch("doghouse_assembly_features._face_type", side_effect=fake_face_type), mock.patch(
            "doghouse_assembly_features._radius_u_v", side_effect=fake_radius_u_v
        ), mock.patch(
            "doghouse_assembly_features._cylinder_info", side_effect=fake_cylinder_info
        ), mock.patch(
            "doghouse_assembly_features._plane_info", side_effect=fake_plane_info
        ):
            groups = _neighbor_analytic_hole_groups(
                faces, adj, 0, scope, min_radius=2.0, max_radius=6.0
            )
        self.assertEqual(len(groups), 1)
        self.assertEqual(groups[0]["face_indices"], [1])
        self.assertEqual(groups[0]["source"], "neighbor_topology")

    def test_neighbor_analytic_rejects_non_coaxial_cylinder(self):
        from doghouse_assembly_features import _neighbor_analytic_hole_groups

        faces = [object(), object()]
        adj = {0: {1}, 1: {0}}
        scope = {0, 1}

        with mock.patch("doghouse_assembly_features._face_type", side_effect=lambda f: "cylinder" if faces.index(f) == 1 else "plane"), mock.patch(
            "doghouse_assembly_features._radius_u_v", return_value=(3.0, 3.0, 2.0)
        ), mock.patch(
            "doghouse_assembly_features._cylinder_info",
            return_value=(np.zeros(3), np.array([1.0, 0.0, 0.0]), 3.0),
        ), mock.patch(
            "doghouse_assembly_features._plane_info",
            return_value=(np.zeros(3), np.array([0.0, 1.0, 0.0])),
        ):
            groups = _neighbor_analytic_hole_groups(
                faces, adj, 0, scope, min_radius=2.0, max_radius=6.0
            )
        self.assertEqual(groups, [])


if __name__ == "__main__":
    unittest.main()
