import tempfile
import unittest
from pathlib import Path
import sys

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from instance_filter import apply_instance_filter, instance_feature_matrix
from train_instance_filter import (
    prediction_examples_from_labels,
    negative_examples_from_failure_report,
    training_examples_from_npz,
)


class InstanceFilterTest(unittest.TestCase):
    def _result(self):
        return {
            "schema": "doghouse_face_predictions.v1",
            "face_predictions": [
                {
                    "face_idx": 0,
                    "doghouse": 1,
                    "instance_id": 1,
                    "role": "doghouse",
                    "doghouse_ratio": 0.9,
                },
                {
                    "face_idx": 1,
                    "doghouse": 1,
                    "instance_id": 1,
                    "role": "doghouse",
                    "doghouse_ratio": 0.8,
                },
                {
                    "face_idx": 2,
                    "doghouse": 1,
                    "instance_id": 2,
                    "role": "doghouse",
                    "doghouse_ratio": 0.75,
                },
            ],
            "doghouse_instances": [
                {"instance_id": 1, "faces": [0, 1]},
                {"instance_id": 2, "faces": [2]},
            ],
        }

    def test_instance_feature_matrix_includes_face_count_and_area(self):
        face_features = np.zeros((3, 9), dtype=np.float32)
        face_features[:, 1] = [10.0, 20.0, 1.0]

        features, ids = instance_feature_matrix(self._result(), {"face_features": face_features})

        self.assertEqual(ids, [1, 2])
        self.assertEqual(float(features[0, 0]), 2.0)
        self.assertEqual(float(features[0, 1]), 30.0)
        self.assertEqual(float(features[1, 0]), 1.0)
        self.assertEqual(float(features[1, 1]), 1.0)

    def test_apply_instance_filter_removes_low_scoring_fragment(self):
        face_features = np.zeros((3, 9), dtype=np.float32)
        face_features[:, 1] = [10.0, 20.0, 1.0]
        model = {
            "weights": np.array([1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float32),
            "bias": np.array([-1.5], dtype=np.float32),
            "mean": np.zeros(8, dtype=np.float32),
            "std": np.ones(8, dtype=np.float32),
            "threshold": np.array([0.5], dtype=np.float32),
        }

        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "filter.npz"
            np.savez(path, **model)
            filtered = apply_instance_filter(
                self._result(),
                {"face_features": face_features},
                path,
            )

        self.assertEqual(filtered["doghouse_instances"][0]["instance_id"], 1)
        self.assertEqual(filtered["doghouse_instances"][0]["faces"], [0, 1])
        self.assertIn("instance_filter_score", filtered["doghouse_instances"][0])
        self.assertEqual(filtered["face_predictions"][2]["doghouse"], 0)
        self.assertEqual(filtered["face_predictions"][2]["instance_id"], -1)
        self.assertEqual(filtered["face_predictions"][2]["role"], "background")
        self.assertEqual(filtered["rejected_doghouse_instances"][0]["instance_id"], 2)

    def test_training_examples_use_negative_fragment_components(self):
        data = {
            "face_features": np.zeros((4, 12), dtype=np.float32),
            "adjacency": np.asarray([[0, 1], [2, 3]], dtype=np.int64),
            "face_instance": np.asarray([1, 1, -1, -1], dtype=np.int64),
            "face_doghouse": np.asarray([1, 1, 0, 0], dtype=np.int64),
            "face_semantic": np.asarray([1, 1, 12, 12], dtype=np.int64),
        }
        data["face_features"][:, 1] = [10.0, 10.0, 1.0, 1.0]

        x, y = training_examples_from_npz(data)

        self.assertEqual(x.shape, (2, 8))
        self.assertEqual(sorted(y.tolist()), [0.0, 1.0])

    def test_failure_report_likely_fragments_become_negative_examples(self):
        data = {
            "face_features": np.zeros((4, 12), dtype=np.float32),
        }
        data["face_features"][:, 1] = [100.0, 100.0, 3.0, 4.0]
        report = {
            "instances": [
                {"instance_id": 1, "faces": [0, 1], "likely_fragment": False},
                {"instance_id": 2, "faces": [2, 3], "likely_fragment": True},
            ]
        }

        x, y = negative_examples_from_failure_report(report, data)

        self.assertEqual(x.shape, (1, 8))
        self.assertEqual(y.tolist(), [0.0])
        self.assertEqual(float(x[0, 0]), 2.0)
        self.assertEqual(float(x[0, 1]), 7.0)

    def test_prediction_examples_label_matched_and_extra_components(self):
        data = {
            "face_features": np.zeros((5, 12), dtype=np.float32),
        }
        data["face_features"][:, 1] = [10.0, 10.0, 10.0, 1.0, 1.0]
        labels = {
            "doghouse_instances": [
                {"instance_id": 1, "faces": [0, 1, 2]},
            ]
        }
        prediction = {
            "face_predictions": [
                {"face_idx": 0, "doghouse": 1, "instance_id": 1, "doghouse_ratio": 0.9},
                {"face_idx": 1, "doghouse": 1, "instance_id": 1, "doghouse_ratio": 0.8},
                {"face_idx": 3, "doghouse": 1, "instance_id": 2, "doghouse_ratio": 0.7},
                {"face_idx": 4, "doghouse": 1, "instance_id": 2, "doghouse_ratio": 0.7},
            ],
            "doghouse_instances": [
                {"instance_id": 1, "faces": [0, 1]},
                {"instance_id": 2, "faces": [3, 4]},
            ],
        }

        x, y = prediction_examples_from_labels(prediction, labels, data, positive_iou=0.5)

        self.assertEqual(x.shape, (2, 8))
        self.assertEqual(y.tolist(), [1.0, 0.0])


if __name__ == "__main__":
    unittest.main()
