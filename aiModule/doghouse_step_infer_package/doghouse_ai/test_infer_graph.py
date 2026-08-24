import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from infer_graph import graph_result_from_arrays


class InferGraphTest(unittest.TestCase):
    def test_min_instance_faces_removes_single_face_fragment(self):
        node_prob = np.asarray([0.9, 0.8, 0.6, 0.1], dtype=np.float32)
        adjacency = np.asarray([[0, 1], [1, 2], [2, 3]], dtype=np.int64)
        edge_prob = np.asarray([0.9, 0.1, 0.9], dtype=np.float32)

        result = graph_result_from_arrays(
            node_prob,
            edge_prob,
            adjacency,
            node_threshold=0.5,
            edge_threshold=0.5,
            min_instance_faces=2,
        )

        self.assertEqual(result["doghouse_instances"], [{"instance_id": 1, "faces": [0, 1]}])
        self.assertEqual(result["face_predictions"][2]["doghouse"], 0)
        self.assertEqual(result["face_predictions"][2]["instance_id"], -1)
        self.assertEqual(result["face_predictions"][2]["role"], "background")
        self.assertEqual(result["removed_small_doghouse_instances"][0]["faces"], [2])


if __name__ == "__main__":
    unittest.main()
