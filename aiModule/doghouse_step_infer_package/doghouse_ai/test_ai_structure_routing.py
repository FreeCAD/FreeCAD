"""VF2 analytic holes win; otherwise AI structure mount/hole fallback."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path
from unittest import mock

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

import doghouse_assembly_features as asm
from graph_model import FaceGraphGNN
from infer_graph import graph_result_from_arrays


class AiStructureRoutingTest(unittest.TestCase):
    def test_reliable_analytic_hole_detection(self):
        class FakeFace:
            def __init__(self, kind, radius=None):
                self.kind = kind
                self.radius = radius

        faces = [FakeFace("bspline"), FakeFace("cylinder", 3.0)]
        with mock.patch.object(asm, "_face_type", side_effect=lambda f: f.kind), mock.patch.object(
            asm, "_radius_u_v", side_effect=lambda f: (f.radius, 2.0, 2.0)
        ):
            ok = asm._vf2_has_reliable_analytic_hole(
                [{"face_indices": [1], "radius": 3.0, "axis_source": "analytic"}],
                faces,
                min_radius=2.0,
                max_radius=6.0,
            )
            bad = asm._vf2_has_reliable_analytic_hole(
                [{"face_indices": [0], "radius": 3.0, "axis_source": "freeform_pca"}],
                faces,
                min_radius=2.0,
                max_radius=6.0,
            )
        self.assertTrue(ok)
        self.assertFalse(bad)

    def test_vf2_reliable_not_overridden_by_ai(self):
        class FakeFace:
            def __init__(self, kind, **kw):
                self.kind = kind
                self.__dict__.update(kw)

        faces = [
            FakeFace("plane", area=100.0, center=np.zeros(3), normal=np.array([0.0, 0.0, 1.0])),
            FakeFace(
                "cylinder",
                radius=3.0,
                u=6.28,
                v=2.0,
                center=np.array([0.0, 0.0, 1.0]),
                axis=np.array([0.0, 0.0, 1.0]),
            ),
            FakeFace("plane", area=200.0, center=np.array([10.0, 0.0, 0.0]), normal=np.array([1.0, 0.0, 0.0])),
        ]
        adj = {0: {1}, 1: {0}, 2: set()}
        prediction = {
            "face_predictions": [
                {"face_idx": 0, "role": "mount", "mount_prob": 0.99, "hole_wall_prob": 0.01},
                {"face_idx": 1, "role": "hole_wall", "mount_prob": 0.01, "hole_wall_prob": 0.99, "hole_wall": 1},
                {"face_idx": 2, "role": "doghouse", "mount_prob": 0.2, "hole_wall_prob": 0.01},
            ],
            "doghouse_instances": [{"instance_id": 1, "faces": [0, 1, 2]}],
        }
        vf2_mount = {
            "face_idx": 2,
            "score": 99.0,
            "center": [10.0, 0.0, 0.0],
            "normal": [1.0, 0.0, 0.0],
            "near_hole_faces": [1],
        }
        vf2_holes = [
            {
                "face_indices": [1],
                "radius": 3.0,
                "center": [0.0, 0.0, 1.0],
                "axis": [0.0, 0.0, 1.0],
                "axis_source": "analytic",
            }
        ]
        with mock.patch.object(asm, "_load_step_and_adjacency", return_value=(None, faces, adj)), mock.patch.object(
            asm, "_dominant_plane_normal", return_value=None
        ), mock.patch.object(
            asm, "_vf2_mount_and_holes", return_value=(vf2_mount, vf2_holes, [vf2_mount])
        ), mock.patch.object(asm, "_face_type", side_effect=lambda f: f.kind), mock.patch.object(
            asm, "_radius_u_v", side_effect=lambda f: (getattr(f, "radius", None), 2.0, 2.0)
        ):
            result = asm.extract_assembly_features(
                "dummy.step",
                prediction,
                use_vf2=True,
                prefer_ai_structure_fallback=True,
            )
        inst = result["instances"][0]
        self.assertEqual(inst["mount_face"]["face_idx"], 2)
        self.assertTrue(str(inst["hole_method"]).startswith("vf2"))
        self.assertTrue(inst["vf2_reliable_analytic"])

    def test_unreliable_vf2_falls_back_to_ai_structure(self):
        class FakeFace:
            def __init__(self, kind, **kw):
                self.kind = kind
                self.__dict__.update(kw)

        faces = [
            FakeFace("plane", area=100.0, center=np.zeros(3), normal=np.array([0.0, 0.0, 1.0])),
            FakeFace("bspline", area=10.0, center=np.array([0.0, 0.0, 1.0])),
            FakeFace("plane", area=200.0, center=np.array([10.0, 0.0, 0.0]), normal=np.array([1.0, 0.0, 0.0])),
        ]
        adj = {0: {1}, 1: {0, 2}, 2: {1}}
        prediction = {
            "face_predictions": [
                {"face_idx": 0, "role": "mount", "mount_prob": 0.95, "hole_wall_prob": 0.05},
                {
                    "face_idx": 1,
                    "role": "hole_wall",
                    "mount_prob": 0.05,
                    "hole_wall_prob": 0.92,
                    "hole_wall": 1,
                },
                {"face_idx": 2, "role": "doghouse", "mount_prob": 0.1, "hole_wall_prob": 0.05},
            ],
            "doghouse_instances": [{"instance_id": 1, "faces": [0, 1, 2]}],
        }
        vf2_mount = {
            "face_idx": 2,
            "score": 99.0,
            "center": [10.0, 0.0, 0.0],
            "normal": [1.0, 0.0, 0.0],
            "near_hole_faces": [1],
        }
        vf2_holes = [
            {
                "face_indices": [1],
                "radius": 3.0,
                "axis_source": "freeform_pca",
            }
        ]
        with mock.patch.object(asm, "_load_step_and_adjacency", return_value=(None, faces, adj)), mock.patch.object(
            asm, "_dominant_plane_normal", return_value=None
        ), mock.patch.object(
            asm, "_vf2_mount_and_holes", return_value=(vf2_mount, vf2_holes, [vf2_mount])
        ), mock.patch.object(asm, "_face_type", side_effect=lambda f: f.kind), mock.patch.object(
            asm, "_surface_area", side_effect=lambda f: float(f.area)
        ), mock.patch.object(
            asm, "_plane_info", side_effect=lambda f: (f.center, f.normal) if f.kind == "plane" else (None, None)
        ), mock.patch.object(asm, "_radius_u_v", return_value=(None, 2.0, 2.0)), mock.patch.object(
            asm, "_outer_support_margin", return_value=0.0
        ), mock.patch.object(asm, "_surface_center", return_value=np.zeros(3)):
            result = asm.extract_assembly_features(
                "dummy.step",
                prediction,
                use_vf2=True,
                prefer_ai_structure_fallback=True,
                mount_min_area=35.0,
            )
        inst = result["instances"][0]
        self.assertEqual(inst["hole_method"], "ai_structure")
        self.assertEqual(inst["mount_face"]["face_idx"], 0)
        self.assertIn(1, inst["hole_groups"][0]["face_indices"])
        self.assertFalse(inst["vf2_reliable_analytic"])

    def test_graph_result_emits_mount_prob(self):
        node = np.array([0.9, 0.9, 0.1], dtype=np.float32)
        edge = np.array([0.9], dtype=np.float32)
        adj = np.array([[0, 1]], dtype=np.int64)
        hole = np.array([0.1, 0.8, 0.0], dtype=np.float32)
        mount = np.array([0.9, 0.1, 0.0], dtype=np.float32)
        out = graph_result_from_arrays(
            node,
            edge,
            adj,
            hole_wall_prob=hole,
            mount_prob=mount,
            mount_threshold=0.35,
            hole_wall_threshold=0.35,
        )
        by = {r["face_idx"]: r for r in out["face_predictions"]}
        self.assertEqual(by[0]["role"], "mount")
        self.assertAlmostEqual(by[0]["mount_prob"], 0.9)
        self.assertEqual(by[1]["role"], "hole_wall")

    def test_model_forward_all_returns_mount_logits(self):
        import torch

        model = FaceGraphGNN(in_dim=12, hole_wall_head=True, mount_head=True)
        x = torch.zeros(3, 12)
        edge_index = torch.tensor([[0, 1], [1, 0]], dtype=torch.long)
        edge_pairs = torch.tensor([[0], [1]], dtype=torch.long)
        node, edge, sem, hole, mount = model.forward_all(x, edge_index, edge_pairs)
        self.assertEqual(tuple(node.shape), (3,))
        self.assertIsNotNone(hole)
        self.assertIsNotNone(mount)
        self.assertEqual(tuple(mount.shape), (3,))


if __name__ == "__main__":
    unittest.main()
