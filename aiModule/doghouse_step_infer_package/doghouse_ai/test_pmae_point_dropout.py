"""Tests for Point-MAE multi-view point dropout helpers."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from extract_pmae_face_features import apply_point_dropout, mean_stack_embeddings


class PointDropoutTest(unittest.TestCase):
    def test_apply_point_dropout_keeps_at_least_one_point_per_face(self):
        points = np.arange(30, dtype=np.float32).reshape(10, 3)
        face_idx = np.array([0, 0, 0, 1, 1, 2, 2, 2, 2, 2], dtype=np.int64)
        rng = np.random.default_rng(0)
        kept_pts, kept_faces = apply_point_dropout(
            points, face_idx, max_ratio=0.9, rng=rng
        )
        self.assertEqual(kept_pts.shape[1], 3)
        self.assertEqual(len(kept_pts), len(kept_faces))
        for face in np.unique(face_idx):
            self.assertGreaterEqual(int((kept_faces == face).sum()), 1)

    def test_apply_point_dropout_zero_ratio_keeps_all(self):
        points = np.random.default_rng(1).normal(size=(20, 3)).astype(np.float32)
        face_idx = np.repeat(np.arange(5), 4).astype(np.int64)
        kept_pts, kept_faces = apply_point_dropout(
            points, face_idx, max_ratio=0.0, rng=np.random.default_rng(2)
        )
        self.assertEqual(len(kept_pts), 20)
        np.testing.assert_array_equal(kept_faces, face_idx)

    def test_mean_stack_embeddings_averages_views(self):
        a = np.ones((3, 4), dtype=np.float32)
        b = np.full((3, 4), 3.0, dtype=np.float32)
        out = mean_stack_embeddings([a, b])
        np.testing.assert_allclose(out, np.full((3, 4), 2.0, dtype=np.float32))


if __name__ == "__main__":
    unittest.main()
