import sys
import tempfile
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from instance_similarity import (
    apply_instance_similarity_filter,
    build_instance_gallery,
    extract_instance_signature,
    score_instance_similarity,
)


class InstanceSimilarityTest(unittest.TestCase):
    def _data(self):
        face_features = np.zeros((6, 12), dtype=np.float32)
        face_features[:, 0] = [1, 2, 6, 1, 2, 6]
        face_features[:, 1] = [10, 20, 15, 2, 3, 2]
        face_features[:, 3] = [0, 1, 0, 0, 1, 0]
        face_features[:, 6:9] = [
            [5, 4, 3],
            [4, 3, 2],
            [3, 3, 2],
            [1, 1, 1],
            [1, 2, 1],
            [2, 1, 1],
        ]
        face_features[:, 9:12] = 0.5
        return {
            "face_features": face_features,
            "face_idx": np.arange(6, dtype=np.int64),
            "points": np.asarray(
                [
                    [0, 0, 0],
                    [5, 0, 0],
                    [0, 4, 0],
                    [0, 0, 3],
                    [20, 20, 0],
                    [21, 20, 0],
                ],
                dtype=np.float32,
            ),
            "adjacency": np.asarray([[0, 1], [1, 2], [3, 4]], dtype=np.int64),
            "face_pmae": np.asarray(
                [
                    [1.0, 0.0],
                    [0.9, 0.1],
                    [1.0, 0.2],
                    [-1.0, 0.0],
                    [-0.9, -0.1],
                    [-1.0, -0.2],
                ],
                dtype=np.float32,
            ),
        }

    def test_extract_instance_signature_includes_topology_size_and_pmae(self):
        sig = extract_instance_signature(self._data(), [0, 1, 2])

        self.assertEqual(sig.face_count, 3)
        self.assertEqual(sig.internal_edges, 2)
        self.assertGreater(sig.numeric[0], 0)
        self.assertEqual(sig.pmae.shape, (2,))
        self.assertGreater(float(sig.pmae[0]), 0.99)
        self.assertGreater(float(sig.pmae[1]), 0.0)

    def test_score_prefers_positive_shape_over_negative_shape(self):
        data = self._data()
        pos = extract_instance_signature(data, [0, 1, 2])
        neg = extract_instance_signature(data, [3, 4])
        query = extract_instance_signature(data, [0, 1])
        gallery = {
            "numeric_mean": np.zeros_like(pos.numeric),
            "numeric_std": np.ones_like(pos.numeric),
            "positive_numeric": np.stack([pos.numeric]),
            "negative_numeric": np.stack([neg.numeric]),
            "positive_pmae": np.stack([pos.pmae]),
            "negative_pmae": np.stack([neg.pmae]),
            "threshold": 0.0,
        }

        score = score_instance_similarity(query, gallery)

        self.assertGreater(score["pos_sim"], score["neg_sim"])
        self.assertGreater(score["keep_score"], 0.0)

    def test_apply_instance_similarity_filter_rejects_negative_like_component(self):
        data = self._data()
        pos = extract_instance_signature(data, [0, 1, 2])
        neg = extract_instance_signature(data, [3, 4])
        gallery = {
            "numeric_mean": np.zeros_like(pos.numeric),
            "numeric_std": np.ones_like(pos.numeric),
            "positive_numeric": np.stack([pos.numeric]),
            "negative_numeric": np.stack([neg.numeric]),
            "positive_pmae": np.stack([pos.pmae]),
            "negative_pmae": np.stack([neg.pmae]),
            "threshold": np.asarray([0.0], dtype=np.float32),
        }
        result = {
            "face_predictions": [
                {"face_idx": 0, "doghouse": 1, "instance_id": 1, "role": "doghouse", "doghouse_ratio": 0.9},
                {"face_idx": 1, "doghouse": 1, "instance_id": 1, "role": "doghouse", "doghouse_ratio": 0.8},
                {"face_idx": 3, "doghouse": 1, "instance_id": 2, "role": "doghouse", "doghouse_ratio": 0.7},
                {"face_idx": 4, "doghouse": 1, "instance_id": 2, "role": "doghouse", "doghouse_ratio": 0.7},
            ],
            "doghouse_instances": [
                {"instance_id": 1, "faces": [0, 1]},
                {"instance_id": 2, "faces": [3, 4]},
            ],
        }

        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "gallery.npz"
            np.savez(path, **gallery)
            filtered = apply_instance_similarity_filter(result, data, path)

        self.assertEqual(len(filtered["doghouse_instances"]), 1)
        self.assertEqual(filtered["doghouse_instances"][0]["faces"], [0, 1])
        self.assertEqual(filtered["rejected_instance_similarity"][0]["faces"], [3, 4])

    def test_build_instance_gallery_uses_gt_and_prediction_extras(self):
        data = self._data()
        data.update(
            {
                "face_instance": np.asarray([1, 1, 1, -1, -1, -1], dtype=np.int64),
                "face_doghouse": np.asarray([1, 1, 1, 0, 0, 0], dtype=np.int64),
                "face_semantic": np.asarray([1, 1, 1, 12, 12, 0], dtype=np.int64),
                "model_name": np.asarray(["toy"]),
            }
        )
        labels = {"doghouse_instances": [{"instance_id": 1, "faces": [0, 1, 2]}]}
        prediction = {
            "doghouse_instances": [
                {"instance_id": 1, "faces": [0, 1, 2]},
                {"instance_id": 2, "faces": [3, 4]},
            ]
        }

        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            npz = root / "toy_graph.npz"
            np.savez(npz, **data)
            label_dir = root / "labels"
            label_dir.mkdir()
            (label_dir / "toy.json").write_text(__import__("json").dumps(labels), encoding="utf-8")
            pred_dir = root / "pred"
            (pred_dir / "toy").mkdir(parents=True)
            (pred_dir / "toy" / "toy_doghouse_pred_faces.json").write_text(
                __import__("json").dumps(prediction),
                encoding="utf-8",
            )
            gallery = build_instance_gallery(
                [npz],
                prediction_dirs=[pred_dir],
                label_dir=label_dir,
                extra_iou=0.2,
            )

        self.assertEqual(int(gallery["train_positive"][0]), 2)
        self.assertGreaterEqual(int(gallery["train_negative"][0]), 2)
        self.assertEqual(gallery["positive_numeric"].shape[1], len(gallery["numeric_feature_names"]))


if __name__ == "__main__":
    unittest.main()
