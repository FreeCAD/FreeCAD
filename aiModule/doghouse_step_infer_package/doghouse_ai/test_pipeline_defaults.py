import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from infer_from_step import _build_arg_parser as build_infer_parser
from pipeline_defaults import (
    DEFAULT_MIN_INSTANCE_FACES,
    PRODUCTION_GRAPH_CHECKPOINT,
    PRODUCTION_INSTANCE_SIM_GALLERY,
    apply_graph_postprocess,
    resolve_checkpoint,
)


class PipelineDefaultsTest(unittest.TestCase):
    def test_infer_parser_uses_integrated_graph_defaults(self):
        args = build_infer_parser().parse_args(["--step", "part.step"])

        self.assertEqual(args.backbone, "graph")
        self.assertIsNone(args.checkpoint)
        self.assertEqual(args.min_instance_faces, DEFAULT_MIN_INSTANCE_FACES)
        self.assertTrue(args.instance_sim_filter)

    def test_resolve_checkpoint_defaults_to_production_graph(self):
        path = resolve_checkpoint(None, backbone="graph")
        self.assertEqual(path, PRODUCTION_GRAPH_CHECKPOINT)

    def test_apply_graph_postprocess_uses_production_gallery_by_default(self):
        if not PRODUCTION_INSTANCE_SIM_GALLERY.exists():
            self.skipTest("production gallery not present")
        result = {
            "face_predictions": [
                {"face_idx": 0, "doghouse": 1, "instance_id": 1, "role": "doghouse", "doghouse_ratio": 0.9},
            ],
            "doghouse_instances": [{"instance_id": 1, "faces": [0]}],
            "min_instance_faces": 2,
        }
        data = {
            "face_features": np.zeros((1, 12), dtype=np.float32),
            "adjacency": np.empty((0, 2), dtype=np.int64),
        }

        filtered = apply_graph_postprocess(result, data, instance_sim_gallery=None, enable_instance_sim=True)

        self.assertIn("pipeline", filtered)
        self.assertTrue(filtered["pipeline"]["instance_similarity_enabled"])

    def test_disable_instance_similarity_records_pipeline_flag(self):
        result = {
            "face_predictions": [],
            "doghouse_instances": [],
            "min_instance_faces": 2,
        }
        data = {"face_features": np.zeros((1, 12), dtype=np.float32), "adjacency": np.empty((0, 2), dtype=np.int64)}

        filtered = apply_graph_postprocess(
            result,
            data,
            instance_sim_gallery=None,
            enable_instance_sim=False,
        )

        self.assertFalse(filtered["pipeline"]["instance_similarity_enabled"])
        self.assertEqual(filtered["instance_similarity_filter"]["enabled"], False)


if __name__ == "__main__":
    unittest.main()
