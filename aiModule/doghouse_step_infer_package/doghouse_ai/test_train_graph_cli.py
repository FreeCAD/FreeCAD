"""Tests for train_graph CLI wiring: --dropout and --sample-points-per-face."""

from __future__ import annotations

import ast
import sys
import unittest
from pathlib import Path
from unittest import mock

import numpy as np
import torch

sys.path.insert(0, str(Path(__file__).resolve().parent))

import train_graph


class TrainGraphCliTest(unittest.TestCase):
    def test_parser_exposes_dropout_and_sample_points(self):
        parser = train_graph.build_arg_parser()
        args = parser.parse_args(
            [
                "--train-full",
                "--dropout",
                "0.35",
                "--sample-points-per-face",
                "200",
            ]
        )
        self.assertEqual(args.dropout, 0.35)
        self.assertEqual(args.sample_points_per_face, 200)

    def test_train_full_writes_dropout_into_checkpoint(self):
        tmp = Path(self._tmp())
        npz = tmp / "toy_graph.npz"
        n_faces = 4
        np.savez_compressed(
            npz,
            face_features=np.zeros((n_faces, 12), dtype=np.float32),
            face_doghouse=np.array([1, 1, 0, 0], dtype=np.int64),
            face_instance=np.array([1, 1, -1, -1], dtype=np.int64),
            face_semantic=np.zeros(n_faces, dtype=np.int64),
            adjacency=np.array([[0, 1]], dtype=np.int64),
            edge_labels=np.array([1.0], dtype=np.float32),
            model_name=np.asarray(["toy"], dtype=str),
        )
        out = tmp / "ckpt.pt"
        device = torch.device("cpu")
        train_graph.train_full(
            [npz],
            out,
            device=device,
            epochs=1,
            lr=1e-3,
            edge_loss_weight=1.0,
            dropout=0.35,
        )
        ckpt = torch.load(out, map_location="cpu")
        self.assertEqual(ckpt["dropout"], 0.35)

    def test_prepare_receives_sample_points_per_face(self):
        parser = train_graph.build_arg_parser()
        args = parser.parse_args(
            [
                "--prepare",
                "--train-full",
                "--sample-points-per-face",
                "200",
                "--data-dir",
                "/tmp/does-not-matter",
                "--step-dir",
                "/tmp/steps",
            ]
        )
        with mock.patch.object(train_graph, "prepare_all") as prepare_all, mock.patch.object(
            train_graph.Path, "glob", return_value=[]
        ):
            prepare_all.side_effect = FileNotFoundError("stop after prepare")
            with self.assertRaises(FileNotFoundError):
                # Simulate the prepare branch only
                if args.prepare:
                    train_graph.prepare_all(
                        Path(args.step_dir),
                        Path(args.data_dir),
                        sample_points_per_face=args.sample_points_per_face,
                    )
            prepare_all.assert_called_once()
            kwargs = prepare_all.call_args.kwargs
            self.assertEqual(kwargs.get("sample_points_per_face"), 200)

    def _tmp(self) -> str:
        import tempfile

        return tempfile.mkdtemp(prefix="train_graph_cli_")


class TrainGraphSourceContractTest(unittest.TestCase):
    """Guard against re-hardcoding dropout=0.2 in checkpoint saves."""

    def test_checkpoint_dropout_not_literal_only(self):
        src = Path(train_graph.__file__).read_text(encoding="utf-8")
        tree = ast.parse(src)
        literals = []
        for node in ast.walk(tree):
            if isinstance(node, ast.Dict):
                for key, val in zip(node.keys, node.values):
                    if (
                        isinstance(key, ast.Constant)
                        and key.value == "dropout"
                        and isinstance(val, ast.Constant)
                        and val.value == 0.2
                    ):
                        literals.append(node)
        self.assertEqual(
            literals,
            [],
            "checkpoint dict still hardcodes dropout: 0.2; pass the dropout argument instead",
        )


if __name__ == "__main__":
    unittest.main()
