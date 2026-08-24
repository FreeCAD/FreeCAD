import unittest
from pathlib import Path
import sys

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from build_point_dataset import build_dataset, build_multi_dataset


def _geometry(face_offset=0):
    return {
        "schema": "doghouse_inference_geometry.v1",
        "num_faces": 2,
        "faces": [
            {
                "face_idx": 0,
                "face_type": "plane",
                "area": 10.0 + face_offset,
                "centroid": [float(face_offset), 0.0, 0.0],
                "bbox": {"min": [0.0, 0.0, 0.0], "max": [1.0, 1.0, 0.0]},
                "normal": [0.0, 0.0, 1.0],
            },
            {
                "face_idx": 1,
                "face_type": "cylinder",
                "area": 5.0,
                "radius": 3.0,
                "has_radius": 1,
                "centroid": [float(face_offset), 1.0, 0.0],
                "bbox": {"min": [0.0, 0.0, 0.0], "max": [1.0, 1.0, 1.0]},
                "normal": [1.0, 0.0, 0.0],
            },
        ],
        "adjacency_edges": [{"a": 0, "b": 1}],
    }


def _labels(instance_id):
    return {
        "schema": "doghouse_instance_labels.v1",
        "face_labels": [
            {
                "face_idx": 1,
                "role": "hole_wall",
                "instance_id": instance_id,
                "doghouse": 1,
            }
        ],
    }


class BuildPointDatasetTest(unittest.TestCase):
    def test_multi_dataset_offsets_faces_and_preserves_local_face_idx(self):
        first = build_dataset(_geometry(0), _labels(1), model_idx=0)
        second = build_dataset(_geometry(10), _labels(1), model_idx=1)

        merged = build_multi_dataset([first, second], model_names=["a", "b"])

        np.testing.assert_array_equal(merged["face_idx"], np.array([0, 1, 2, 3]))
        np.testing.assert_array_equal(merged["local_face_idx"], np.array([0, 1, 0, 1]))
        np.testing.assert_array_equal(merged["model_idx"], np.array([0, 0, 1, 1]))
        np.testing.assert_array_equal(merged["adjacency"], np.array([[0, 1], [2, 3]]))
        np.testing.assert_array_equal(merged["face_instance"], np.array([-1, 1, -1, 1]))
        self.assertEqual(merged["model_names"].tolist(), ["a", "b"])

    def test_hard_negative_role_is_not_doghouse(self):
        labels = {
            "schema": "doghouse_instance_labels.v1",
            "face_labels": [
                {
                    "face_idx": 0,
                    "role": "negative_rib",
                    "instance_id": -1,
                    "doghouse": 0,
                }
            ],
        }

        data = build_dataset(_geometry(0), labels)

        self.assertEqual(int(data["face_semantic"][0]), 9)
        self.assertEqual(int(data["face_doghouse"][0]), 0)
        self.assertEqual(int(data["face_instance"][0]), -1)

    def test_negative_fragment_role_is_not_doghouse(self):
        labels = {
            "schema": "doghouse_instance_labels.v1",
            "face_labels": [
                {
                    "face_idx": 0,
                    "role": "negative_fragment",
                    "instance_id": 2,
                    "doghouse": 1,
                }
            ],
        }

        data = build_dataset(_geometry(0), labels)

        self.assertEqual(int(data["face_semantic"][0]), 12)
        self.assertEqual(int(data["face_doghouse"][0]), 0)
        self.assertEqual(int(data["face_instance"][0]), -1)

    def test_non_hole_roles_belong_to_doghouse_instance(self):
        labels = {
            "schema": "doghouse_instance_labels.v1",
            "face_labels": [
                {
                    "face_idx": 1,
                    "role": "non_hole_cylinder",
                    "instance_id": 1,
                    "doghouse": 1,
                }
            ],
        }

        data = build_dataset(_geometry(0), labels)

        self.assertEqual(int(data["face_semantic"][1]), 7)
        self.assertEqual(int(data["face_doghouse"][1]), 1)
        self.assertEqual(int(data["face_instance"][1]), 1)

    def test_external_negative_role_overrides_later_positive_label(self):
        labels = {
            "schema": "doghouse_instance_labels.v1",
            "face_labels": [
                {
                    "face_idx": 0,
                    "role": "negative_rib",
                    "instance_id": -1,
                    "doghouse": 0,
                },
                {
                    "face_idx": 0,
                    "role": "doghouse",
                    "instance_id": 1,
                    "doghouse": 1,
                },
            ],
        }

        data = build_dataset(_geometry(0), labels)

        self.assertEqual(int(data["face_semantic"][0]), 9)
        self.assertEqual(int(data["face_doghouse"][0]), 0)
        self.assertEqual(int(data["face_instance"][0]), -1)

    def test_legacy_non_hole_instance_fields_belong_to_doghouse(self):
        labels = {
            "schema": "doghouse_instance_labels.v1",
            "doghouse_instances": [
                {
                    "instance_id": 1,
                    "faces": [0, 1],
                    "non_hole_cylinder_faces": [1],
                }
            ],
        }

        data = build_dataset(_geometry(0), labels)

        self.assertEqual(int(data["face_semantic"][1]), 7)
        self.assertEqual(int(data["face_doghouse"][1]), 1)
        self.assertEqual(int(data["face_instance"][1]), 1)


if __name__ == "__main__":
    unittest.main()
