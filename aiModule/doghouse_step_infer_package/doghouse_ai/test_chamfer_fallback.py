from __future__ import annotations

import unittest

import torch

from extensions.chamfer_dist import ChamferDistanceL1, ChamferDistanceL2


class ChamferFallbackTest(unittest.TestCase):
    def test_chamfer_l2_runs_without_cuda_extension(self):
        xyz1 = torch.tensor([[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]])
        xyz2 = torch.tensor([[[0.0, 0.0, 0.0], [2.0, 0.0, 0.0]]])

        value = ChamferDistanceL2()(xyz1, xyz2)

        self.assertGreaterEqual(float(value), 0.0)

    def test_chamfer_l1_runs_without_cuda_extension(self):
        xyz1 = torch.tensor([[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]])
        xyz2 = torch.tensor([[[0.0, 0.0, 0.0], [2.0, 0.0, 0.0]]])

        value = ChamferDistanceL1()(xyz1, xyz2)

        self.assertGreaterEqual(float(value), 0.0)


if __name__ == "__main__":
    unittest.main()
