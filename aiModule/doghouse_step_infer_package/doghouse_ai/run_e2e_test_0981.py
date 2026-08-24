#!/usr/bin/env python3
"""End-to-end integration test for 未命名-0981535409815353 (0981 pillar trim)."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import time
from pathlib import Path

DOGHOUSE_DIR = Path(__file__).resolve().parent
ROOT = DOGHOUSE_DIR.parent
MODEL_STEM = "未命名-0981535409815353"
STEP_PATH = DOGHOUSE_DIR / "step - 副本2" / f"{MODEL_STEM}.step"
LABEL_JSON = DOGHOUSE_DIR / "step - 副本2" / f"{MODEL_STEM}_annotation.json"
OUTPUT_DIR = DOGHOUSE_DIR / "e2e_test_0981"

PRED_JSON = OUTPUT_DIR / f"{MODEL_STEM}_doghouse_pred_faces.json"
ASSEMBLY_JSON = OUTPUT_DIR / f"{MODEL_STEM}_doghouse_assembly_features.json"
COLORED_STEP = OUTPUT_DIR / f"{MODEL_STEM}_assembly_colored.step"
SUMMARY_JSON = OUTPUT_DIR / f"{MODEL_STEM}_e2e_summary.json"

MIN_FACE_IOU = 0.92
MIN_RECALL = 1.0
EXPECTED_GT_INSTANCES = 5


def _import_eval():
    sys.path.insert(0, str(DOGHOUSE_DIR))
    try:
        from eval_face_predictions import evaluate_face_predictions
    except ImportError:
        from doghouse_ai.eval_face_predictions import evaluate_face_predictions
    return evaluate_face_predictions


def run_inference(python: str) -> float:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    cmd = [
        python,
        str(DOGHOUSE_DIR / "infer_from_step.py"),
        "--step",
        str(STEP_PATH),
        "--output-dir",
        str(OUTPUT_DIR),
        "--extract-assembly-features",
        "--use-vf2",
        "--assembly-output-step",
        str(COLORED_STEP),
    ]
    t0 = time.perf_counter()
    subprocess.run(cmd, check=True, cwd=ROOT)
    return time.perf_counter() - t0


def summarize_assembly(path: Path) -> dict:
    data = json.loads(path.read_text(encoding="utf-8"))
    instances = []
    for inst in data.get("instances", []):
        mount = inst.get("mount_face") or {}
        hole_groups = inst.get("hole_groups") or []
        instances.append(
            {
                "instance_id": int(inst.get("instance_id", -1)),
                "status": inst.get("status"),
                "mount_face_idx": mount.get("face_idx"),
                "hole_group_count": len(hole_groups),
            }
        )
    return {
        "instance_count": len(instances),
        "ok_count": sum(1 for row in instances if row.get("status") == "ok"),
        "instances": instances,
    }


def build_report(*, inference_s: float | None) -> dict:
    evaluate_face_predictions = _import_eval()
    pred = json.loads(PRED_JSON.read_text(encoding="utf-8"))
    detection = evaluate_face_predictions(LABEL_JSON, PRED_JSON)
    assembly = summarize_assembly(ASSEMBLY_JSON)
    return {
        "model": MODEL_STEM,
        "step": str(STEP_PATH),
        "label_json": str(LABEL_JSON),
        "output_dir": str(OUTPUT_DIR),
        "prediction_json": str(PRED_JSON),
        "assembly_json": str(ASSEMBLY_JSON),
        "colored_step": str(COLORED_STEP) if COLORED_STEP.exists() else None,
        "inference_s": round(inference_s, 3) if inference_s is not None else None,
        "pipeline": pred.get("pipeline"),
        "detection": detection,
        "assembly": assembly,
        "pass": bool(
            detection["gt_instances"] == EXPECTED_GT_INSTANCES
            and detection["pred_instances"] == EXPECTED_GT_INSTANCES
            and detection["extra_count"] == 0
            and detection["face_iou"] >= MIN_FACE_IOU
            and detection["recall"] >= MIN_RECALL
            and assembly["ok_count"] == EXPECTED_GT_INSTANCES
            and all(row["mount_face_idx"] is not None for row in assembly["instances"])
            and all(row["hole_group_count"] >= 1 for row in assembly["instances"])
        ),
    }


def print_report(report: dict) -> None:
    det = report["detection"]
    asm = report["assembly"]
    print(f"model: {report['model']}")
    if report.get("inference_s") is not None:
        print(f"inference_s: {report['inference_s']}")
    print(
        f"detection: gt={det['gt_instances']} pred={det['pred_instances']} "
        f"face_iou={det['face_iou']:.4f} recall={det['recall']:.4f} extra={det['extra_count']}"
    )
    for match in det["matches"]:
        print(
            f"  gt#{match['gt']} <- pred#{match['pred']} iou={match['iou']:.4f}"
        )
    print(f"assembly: ok={asm['ok_count']}/{asm['instance_count']}")
    for row in asm["instances"]:
        print(
            f"  inst#{row['instance_id']} status={row['status']} "
            f"mount={row['mount_face_idx']} hole_groups={row['hole_group_count']}"
        )
    print(f"PASS: {report['pass']}")
    print(f"summary: {SUMMARY_JSON}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--eval-only",
        action="store_true",
        help="Only evaluate existing outputs under e2e_test_0981/",
    )
    parser.add_argument(
        "--python",
        default=sys.executable,
        help="Python interpreter for infer_from_step.py",
    )
    args = parser.parse_args()

    inference_s = None
    if not args.eval_only:
        inference_s = run_inference(args.python)
    elif not PRED_JSON.exists() or not ASSEMBLY_JSON.exists():
        raise SystemExit(
            f"Missing outputs in {OUTPUT_DIR}; run without --eval-only first."
        )

    report = build_report(inference_s=inference_s)
    SUMMARY_JSON.write_text(
        json.dumps(report, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    print_report(report)
    return 0 if report["pass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
