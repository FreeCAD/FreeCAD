"""Legacy AI-hole tests redirected to structure-routing coverage."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

import numpy as np
import torch

sys.path.insert(0, str(Path(__file__).resolve().parent))

from graph_model import FaceGraphGNN
from infer_graph import graph_result_from_arrays
from train_graph import build_arg_parser, train_full


class AiHoleAssemblyTest(unittest.TestCase):
    def test_parser_exposes_structure_loss_weights(self):
        args = build_arg_parser().parse_args(
            ["--train-full", "--hole-wall-loss-weight", "0.5", "--mount-loss-weight", "0.5"]
        )
        self.assertEqual(args.hole_wall_loss_weight, 0.5)
        self.assertEqual(args.mount_loss_weight, 0.5)

    def test_train_full_writes_structure_head_flags(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            npz = tmp_path / "toy_graph.npz"
            n = 4
            feats = np.zeros((n, 12), dtype=np.float32)
            feats[:, 0] = 1.0
            sem = np.zeros(n, dtype=np.int64)
            sem[0] = 2  # mount
            sem[1] = 3  # hole_wall
            np.savez_compressed(
                npz,
                face_features=feats,
                face_doghouse=np.array([1, 1, 0, 0], dtype=np.int64),
                face_instance=np.array([1, 1, 0, 0], dtype=np.int64),
                face_semantic=sem,
                adjacency=np.array([[0, 1]], dtype=np.int64),
                edge_labels=np.array([1.0], dtype=np.float32),
            )
            out = tmp_path / "ckpt.pt"
            train_full(
                [npz],
                out,
                device=torch.device("cpu"),
                epochs=1,
                lr=1e-3,
                edge_loss_weight=1.0,
                hole_wall_loss_weight=0.5,
                mount_loss_weight=0.5,
            )
            ckpt = torch.load(out, map_location="cpu")
            self.assertTrue(ckpt["hole_wall_head"])
            self.assertTrue(ckpt["mount_head"])
            model = FaceGraphGNN(in_dim=12, hole_wall_head=True, mount_head=True)
            model.load_state_dict(ckpt["model_state"])
            self.assertIsNotNone(model.hole_wall_head)
            self.assertIsNotNone(model.mount_head)

    def test_graph_result_includes_structure_probs(self):
        node = np.array([0.9, 0.8], dtype=np.float32)
        edge = np.array([0.9], dtype=np.float32)
        adj = np.array([[0, 1]], dtype=np.int64)
        hole = np.array([0.1, 0.8], dtype=np.float32)
        mount = np.array([0.9, 0.1], dtype=np.float32)
        out = graph_result_from_arrays(
            node, edge, adj, hole_wall_prob=hole, mount_prob=mount
        )
        rows = out["face_predictions"]
        self.assertEqual(rows[0]["role"], "mount")
        self.assertEqual(rows[1]["role"], "hole_wall")
        self.assertAlmostEqual(rows[0]["mount_prob"], 0.9)
        self.assertAlmostEqual(rows[1]["hole_wall_prob"], 0.8)


if __name__ == "__main__":
    unittest.main()
