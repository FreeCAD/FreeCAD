from __future__ import annotations

import tempfile
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from recommend_and_assemble import (
    assembly_features_to_hole_groups,
    build_clip_recommendation_payload,
    _clip_depth_from_geom_json,
    _hole_depth_for_groups,
    _load_or_infer_prediction,
    rank_clip_defs_for_hole,
    resolve_auto_prediction_json,
)


class AutoDoghouseAssemblyTest(unittest.TestCase):
    def test_resolve_auto_prediction_json_finds_space_annotation(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            step = root / "pillar.step"
            step.write_text("", encoding="utf-8")
            expected = root / "pillar annotation.json"
            expected.write_text("{}", encoding="utf-8")

            self.assertEqual(resolve_auto_prediction_json(step), expected)

    def test_load_or_infer_prediction_runs_inference_without_annotation(self):
        args = SimpleNamespace(
            step="model.step",
            prediction_json="",
            recommend_output="out/recommendation.json",
            doghouse_infer_output_dir="",
            infer_cpu=True,
        )
        fake_prediction = {"schema": "doghouse_face_predictions.v1", "doghouse_instances": []}

        with patch("recommend_and_assemble.resolve_auto_prediction_json", return_value=None), patch(
            "infer_from_step.infer_step", return_value=fake_prediction
        ) as infer_step:
            prediction, pred_path = _load_or_infer_prediction(args)

        self.assertEqual(prediction, fake_prediction)
        self.assertIsNone(pred_path)
        self.assertTrue(infer_step.called)
        infer_args = infer_step.call_args.args[0]
        self.assertEqual(infer_args.step, "model.step")
        self.assertTrue(str(infer_args.output_dir).replace("\\", "/").endswith("out/infer"))

    def test_load_or_infer_prediction_ignores_sidecar_annotation_by_default(self):
        args = SimpleNamespace(
            step="model.step",
            prediction_json="",
            recommend_output="out/recommendation.json",
            doghouse_infer_output_dir="",
            infer_cpu=True,
        )
        fake_prediction = {"schema": "doghouse_face_predictions.v1", "doghouse_instances": []}

        with patch(
            "recommend_and_assemble.resolve_auto_prediction_json",
            return_value=Path("model_annotation.json"),
        ) as resolve_prediction, patch(
            "infer_from_step.infer_step", return_value=fake_prediction
        ) as infer_step:
            prediction, pred_path = _load_or_infer_prediction(args)

        self.assertEqual(prediction, fake_prediction)
        self.assertIsNone(pred_path)
        self.assertFalse(resolve_prediction.called)
        self.assertTrue(infer_step.called)

    def test_assembly_features_to_hole_groups_builds_mount_pair(self):
        features = {
            "instances": [
                {
                    "instance_id": 5,
                    "status": "ok",
                    "mount_face": {
                        "face_idx": 960,
                        "center": [1.0, 2.0, 3.0],
                        "normal": [0.0, 0.0, 1.0],
                    },
                    "hole_groups": [
                        {
                            "face_indices": [10, 11, 951],
                            "center": [1.0, 2.0, 8.0],
                            "axis": [0.0, 0.0, 1.0],
                            "radius": 3.2,
                            "v_max": 2.5,
                        }
                    ],
                }
            ]
        }

        groups = assembly_features_to_hole_groups(features)

        self.assertEqual(len(groups), 1)
        group = groups[0]
        self.assertEqual(group["instance_id"], 5)
        self.assertEqual(group["mount_face_idx"], 960)
        self.assertTrue(group["mount_valid"])
        self.assertEqual(group["mount_source"], "assembly_features")
        np.testing.assert_allclose(group["axis"], [0.0, 0.0, 1.0])
        np.testing.assert_allclose(group["geom_axis"], [0.0, 0.0, 1.0])
        np.testing.assert_allclose(group["placement_center"], [1.0, 2.0, 3.0])
        self.assertEqual(group["mount"][0], 960)
        self.assertEqual(group["depth_mm"], 2.5)

    def test_rank_clip_defs_falls_back_without_recommend_module(self):
        clip_defs = [
            ("diameter_gap_too_large", 3.0, 0.0, "loose.step", {}),
            ("diameter_gap_in_range", 4.0, 0.0, "ideal.step", {}),
            ("diameter_gap_too_small", 4.6, 0.0, "tight.step", {}),
        ]

        ranked = rank_clip_defs_for_hole(clip_defs, 4.8)

        self.assertEqual(ranked[0][3], "diameter_gap_in_range")

    def test_clip_geom_depth_uses_bolt_cyl_face_depth(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            geom = root / "clip.geom.json"
            geom.write_text(
                """{
  "faces": [
    {"face_idx": 0, "radius": 4.0, "depth": 9.0},
    {"face_idx": 7, "radius": 3.0, "depth": 2.25}
  ]
}""",
                encoding="utf-8",
            )

            depth = _clip_depth_from_geom_json(root, "clip", [7], clip_radius=3.0)

        self.assertEqual(depth, 2.25)

    def test_clip_geom_depth_averages_multiple_bolt_cyl_faces(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            geom = root / "clip.geom.json"
            geom.write_text(
                """{
  "faces": [
    {"face_idx": 2, "radius": 4.0, "depth": 1.0},
    {"face_idx": 3, "radius": 4.0, "depth": 10.0},
    {"face_idx": 10, "radius": 4.0, "depth": 1.0}
  ]
}""",
                encoding="utf-8",
            )

            depth = _clip_depth_from_geom_json(root, "clip", [2, 3, 10], clip_radius=4.0)

        self.assertAlmostEqual(depth, 4.0)

    def test_hole_depth_for_groups_uses_v_max_median(self):
        groups = [{"v_max": 2.0}, {"depth_mm": 2.4}, {"v_depth": 10.0}]

        self.assertEqual(_hole_depth_for_groups(groups), 2.4)

    def test_rank_clip_defs_uses_depth_as_tiebreaker(self):
        clip_defs = [
            ("better_diameter_worse_depth", 3.8, 6.0, "wrong.step", {}),
            ("worse_diameter_better_depth", 4.0, 2.3, "right.step", {}),
        ]

        ranked = rank_clip_defs_for_hole(clip_defs, 4.8, hole_depth_mm=2.3)

        self.assertEqual(ranked[0][3], "worse_diameter_better_depth")

    def test_rank_clip_defs_keeps_valid_diameter_before_depth_match(self):
        clip_defs = [
            ("invalid_diameter_good_depth", 3.0, 2.3, "invalid.step", {}),
            ("valid_diameter_bad_depth", 4.0, 10.0, "valid.step", {}),
        ]

        ranked = rank_clip_defs_for_hole(clip_defs, 4.8, hole_depth_mm=2.3)

        self.assertEqual(ranked[0][3], "valid_diameter_bad_depth")

    def test_rank_clip_defs_returns_each_depth_gap(self):
        clip_defs = [
            ("depth_a", 4.0, 10.0, "a.step", {}),
            ("depth_b", 4.0, 2.5, "b.step", {}),
        ]

        ranked = rank_clip_defs_for_hole(clip_defs, 4.8, hole_depth_mm=2.0)
        gaps = {row[3]: row[2] for row in ranked}

        self.assertAlmostEqual(gaps["depth_a"], 8.0)
        self.assertAlmostEqual(gaps["depth_b"], 0.5)

    def test_recommendation_payload_contains_plugin_fields(self):
        hole_groups = [
            {
                "radius": 4.8,
                "depth_mm": 2.3,
                "center": [0.0, 0.0, 0.0],
                "axis": [0.0, 0.0, 1.0],
                "mount": (1, (0.0, 0.0, 0.0), (0.0, 0.0, 1.0)),
                "mount_face_idx": 1,
                "face_indices": [2, 3],
                "mount_valid": True,
            }
        ]
        clip_defs = [
            ("invalid_but_close_height", 2.75, 2.3, "a.step", {}),
            ("valid", 4.0, 2.1, "b.step", {}),
        ]

        payload = build_clip_recommendation_payload(
            hole_groups,
            clip_defs,
            tolerance=0.5,
            source_step="model.step",
        )

        self.assertEqual(payload["schema"], "doghouse_clip_recommendation.v1")
        self.assertEqual(payload["source_step"], "model.step")
        self.assertEqual(payload["hole_count"], 1)
        self.assertAlmostEqual(payload["hole_diameter_mm"], 9.6)
        self.assertAlmostEqual(payload["hole_depth_mm"], 2.3)
        self.assertEqual(payload["holes"][0]["hole_face_indices"], [2, 3])
        self.assertEqual(payload["holes"][0]["mount_face_idx"], 1)
        self.assertEqual(payload["selected_clip"], "valid")
        first = payload["clips"][0]
        self.assertEqual(first["name"], "valid")
        self.assertTrue(first["diameter_valid"])
        self.assertAlmostEqual(first["diameter_gap_mm"], 1.6)
        self.assertAlmostEqual(first["depth_gap_mm"], 0.2)

    def test_recommendation_payload_can_use_one_clip_for_all_doghouses(self):
        hole_groups = [
            {
                "radius": 3.0,
                "depth_mm": 2.0,
                "center": [0.0, 0.0, 0.0],
                "axis": [0.0, 0.0, 1.0],
                "mount": (1, (0.0, 0.0, 0.0), (0.0, 0.0, 1.0)),
                "mount_face_idx": 1,
                "face_indices": [10],
            },
            {
                "radius": 3.0,
                "depth_mm": 2.1,
                "center": [1.0, 0.0, 0.0],
                "axis": [0.0, 0.0, 1.0],
                "mount": (2, (1.0, 0.0, 0.0), (0.0, 0.0, 1.0)),
                "mount_face_idx": 2,
                "face_indices": [11],
            },
            {
                "radius": 4.0,
                "depth_mm": 1.9,
                "center": [2.0, 0.0, 0.0],
                "axis": [0.0, 0.0, 1.0],
                "mount": (3, (2.0, 0.0, 0.0), (0.0, 0.0, 1.0)),
                "mount_face_idx": 3,
                "face_indices": [12],
            },
        ]
        clip_defs = [
            ("r3_best", 2.25, 2.0, "r3.step", {}),
            ("r4_best", 3.0, 2.0, "r4.step", {}),
        ]

        payload = build_clip_recommendation_payload(
            hole_groups,
            clip_defs,
            tolerance=0.5,
            source_step="m5.step",
            all_holes_same_clip=True,
        )

        self.assertEqual(payload["hole_count"], 3)
        self.assertEqual(len(payload["holes"]), 3)
        self.assertEqual(payload["mode"], "all_holes_same_clip")
        self.assertAlmostEqual(payload["representative_hole_radius_mm"], 3.0)
        self.assertEqual(payload["selected_clip"], "r3_best")


if __name__ == "__main__":
    unittest.main()
