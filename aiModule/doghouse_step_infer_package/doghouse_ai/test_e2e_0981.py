import json
import os
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from run_e2e_test_0981 import (
    ASSEMBLY_JSON,
    EXPECTED_GT_INSTANCES,
    MIN_FACE_IOU,
    MIN_RECALL,
    PRED_JSON,
    SUMMARY_JSON,
    build_report,
)


class E2E0981Test(unittest.TestCase):
    def test_cached_e2e_outputs_meet_thresholds(self):
        if not PRED_JSON.exists() or not ASSEMBLY_JSON.exists():
            self.skipTest("Run doghouse_ai/run_e2e_test_0981.py first to generate outputs")
        report = build_report(inference_s=None)
        det = report["detection"]
        asm = report["assembly"]

        self.assertEqual(det["gt_instances"], EXPECTED_GT_INSTANCES)
        self.assertEqual(det["pred_instances"], EXPECTED_GT_INSTANCES)
        self.assertEqual(det["extra_count"], 0)
        self.assertGreaterEqual(det["face_iou"], MIN_FACE_IOU)
        self.assertGreaterEqual(det["recall"], MIN_RECALL)
        self.assertEqual(asm["ok_count"], EXPECTED_GT_INSTANCES)
        self.assertTrue(all(row["mount_face_idx"] is not None for row in asm["instances"]))
        self.assertTrue(all(row["hole_group_count"] >= 1 for row in asm["instances"]))
        self.assertTrue(report["pass"])

    def test_summary_json_matches_report(self):
        if not SUMMARY_JSON.exists():
            self.skipTest("Summary JSON not generated yet")
        saved = json.loads(SUMMARY_JSON.read_text(encoding="utf-8"))
        self.assertTrue(saved.get("pass"))
        self.assertEqual(saved["detection"]["gt_instances"], EXPECTED_GT_INSTANCES)


@unittest.skipUnless(
    os.environ.get("DOGHOUSE_E2E_SLOW") == "1",
    "Set DOGHOUSE_E2E_SLOW=1 to run full STEP inference (~1 min)",
)
class E2E0981SlowTest(unittest.TestCase):
    def test_full_pipeline(self):
        import subprocess

        root = Path(__file__).resolve().parent.parent
        proc = subprocess.run(
            [sys.executable, str(Path(__file__).resolve().parent / "run_e2e_test_0981.py")],
            cwd=root,
            capture_output=True,
            text=True,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)


if __name__ == "__main__":
    unittest.main()
