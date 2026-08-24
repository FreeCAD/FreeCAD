from __future__ import annotations

import unittest

import torch

from pointnet2_ops import pointnet2_utils


class PointNet2OpsFallbackTest(unittest.TestCase):
    def test_furthest_point_sample_returns_requested_count(self):
        points = torch.tensor(
            [
                [
                    [0.0, 0.0, 0.0],
                    [1.0, 0.0, 0.0],
                    [2.0, 0.0, 0.0],
                    [3.0, 0.0, 0.0],
                ]
            ],
            dtype=torch.float32,
        )

        idx = pointnet2_utils.furthest_point_sample(points, 3)

        self.assertEqual(tuple(idx.shape), (1, 3))
        self.assertEqual(idx.dtype, torch.long)

    def test_gather_operation_gathers_channel_first_points(self):
        features = torch.tensor([[[10.0, 20.0, 30.0], [1.0, 2.0, 3.0]]])
        idx = torch.tensor([[2, 0]], dtype=torch.long)

        gathered = pointnet2_utils.gather_operation(features, idx)

        self.assertTrue(torch.equal(gathered, torch.tensor([[[30.0, 10.0], [3.0, 1.0]]])))


if __name__ == "__main__":
    unittest.main()
