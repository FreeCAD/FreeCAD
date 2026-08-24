#!/usr/bin/env python3
"""Evaluate doghouse face prediction JSON against instance label JSON."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def load_json(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def _instances_from_labels(data):
    return {
        int(inst["instance_id"]): {int(x) for x in inst.get("faces", [])}
        for inst in data.get("doghouse_instances", [])
        if inst.get("faces")
    }


def _instances_from_prediction(data):
    instances = {
        int(inst["instance_id"]): {int(x) for x in inst.get("faces", [])}
        for inst in data.get("doghouse_instances", [])
        if inst.get("faces")
    }
    if instances:
        return instances
    out = {}
    for row in data.get("face_predictions", []):
        if int(row.get("doghouse", 0)) <= 0:
            continue
        iid = int(row.get("instance_id", -1))
        if iid <= 0:
            continue
        out.setdefault(iid, set()).add(int(row["face_idx"]))
    return out


def iou(a, b):
    if not a and not b:
        return 1.0
    return len(a & b) / max(len(a | b), 1)


def greedy_match(gt, pred):
    pairs = []
    used_gt = set()
    used_pred = set()
    candidates = []
    for gi, gf in gt.items():
        for pi, pf in pred.items():
            candidates.append((iou(gf, pf), gi, pi))
    candidates.sort(reverse=True)
    for score, gi, pi in candidates:
        if gi in used_gt or pi in used_pred:
            continue
        used_gt.add(gi)
        used_pred.add(pi)
        pairs.append((gi, pi, score))
    return pairs, set(gt) - used_gt, set(pred) - used_pred


def evaluate_face_predictions(label_json: str | Path, pred_json: str | Path) -> dict:
    gt = _instances_from_labels(load_json(label_json))
    pred = _instances_from_prediction(load_json(pred_json))
    pairs, missing, extra = greedy_match(gt, pred)

    gt_union = set().union(*gt.values()) if gt else set()
    pred_union = set().union(*pred.values()) if pred else set()
    face_iou = iou(gt_union, pred_union)
    precision = len(gt_union & pred_union) / max(len(pred_union), 1)
    recall = len(gt_union & pred_union) / max(len(gt_union), 1)

    return {
        "gt_instances": len(gt),
        "pred_instances": len(pred),
        "matched_instances": len(pairs),
        "missing_gt": sorted(missing),
        "extra_pred": sorted(extra),
        "extra_count": len(extra),
        "face_iou": round(face_iou, 6),
        "precision": round(precision, 6),
        "recall": round(recall, 6),
        "matches": [
            {
                "gt": int(gi),
                "pred": int(pi),
                "iou": round(float(score), 6),
                "gt_faces": len(gt[gi]),
                "pred_faces": len(pred[pi]),
            }
            for gi, pi, score in pairs
        ],
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--label-json", required=True)
    parser.add_argument("--pred-json", required=True)
    args = parser.parse_args()

    report = evaluate_face_predictions(args.label_json, args.pred_json)
    print(f"gt_instances={report['gt_instances']} pred_instances={report['pred_instances']}")
    print(
        f"face_iou={report['face_iou']:.4f} "
        f"precision={report['precision']:.4f} recall={report['recall']:.4f}"
    )
    print("matches:")
    for match in report["matches"]:
        print(
            f"  gt#{match['gt']}({match['gt_faces']}) <- pred#{match['pred']}({match['pred_faces']}) "
            f"iou={match['iou']:.4f}"
        )
    if report["missing_gt"]:
        print(f"missing_gt={report['missing_gt']}")
    if report["extra_pred"]:
        print(f"extra_pred={report['extra_pred']}")


if __name__ == "__main__":
    main()
