#!/usr/bin/env python3
"""Sample-200 + dropout ablation: prepare → PMAE → train → 8-model eval.

Does not overwrite production checkpoints / pmae_face_emb. Outputs under
``ablation_s200_dropout/``.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import time
from pathlib import Path

import numpy as np

DOGHOUSE_DIR = Path(__file__).resolve().parent
ROOT = DOGHOUSE_DIR.parent
HOLDOUT_STEM = "未命名-6302001001-B126302102001-B12"
BASELINE_SUMMARY = DOGHOUSE_DIR / "compare_8_mixed_pmae_train301_min2_simfilter_summary.json"

VARIANTS = {
    "s200": {
        "sample_points_per_face": 200,
        "dropout": 0.2,
        "point_dropout_views": 1,
        "point_dropout_max_ratio": 0.4,
        "emb_subdir": "pmae_face_emb_s200",
    },
    "s200_d035": {
        "sample_points_per_face": 200,
        "dropout": 0.35,
        "point_dropout_views": 1,
        "point_dropout_max_ratio": 0.4,
        "emb_subdir": "pmae_face_emb_s200",
    },
    "s200_d035_ptdrop": {
        "sample_points_per_face": 200,
        "dropout": 0.35,
        "point_dropout_views": 4,
        "point_dropout_max_ratio": 0.4,
        "emb_subdir": "pmae_face_emb_s200_ptdrop",
    },
}


def _python() -> str:
    return sys.executable


def _run(cmd: list[str], *, cwd: Path | None = None) -> None:
    print("+", " ".join(cmd), flush=True)
    subprocess.run(cmd, check=True, cwd=str(cwd or ROOT))


def discover_eval_models(step_dir: Path) -> list[str]:
    sys.path.insert(0, str(DOGHOUSE_DIR))
    from prepare_graph_data import discover_pairs

    return [Path(step_name).stem for step_name, _ in discover_pairs(step_dir)]


def label_path_for_stem(step_dir: Path, stem: str) -> Path:
    sys.path.insert(0, str(DOGHOUSE_DIR))
    from prepare_graph_data import _find_label_for_step

    path = _find_label_for_step(step_dir / f"{stem}.step")
    if path is None:
        raise FileNotFoundError(f"no label JSON for {stem}")
    return path


def prepare_graph(step_dir: Path, out_dir: Path, sample_points: int) -> None:
    _run(
        [
            _python(),
            str(DOGHOUSE_DIR / "prepare_graph_data.py"),
            "--step-dir",
            str(step_dir),
            "--output-dir",
            str(out_dir),
            "--sample-points-per-face",
            str(sample_points),
        ],
        cwd=DOGHOUSE_DIR,
    )


def export_pmae_inputs(graph_dir: Path, out_dir: Path) -> None:
    _run(
        [
            _python(),
            str(DOGHOUSE_DIR / "export_pointcloud_for_pmae.py"),
            "--graph-dir",
            str(graph_dir),
            "--output-dir",
            str(out_dir),
        ],
        cwd=DOGHOUSE_DIR,
    )


def extract_pmae(
    *,
    ckpt: Path,
    input_dir: Path,
    output_dir: Path,
    views: int,
    max_ratio: float,
) -> None:
    cmd = [
        _python(),
        str(ROOT / "extract_pmae_face_features.py"),
        "--ckpt",
        str(Path(ckpt).resolve()),
        "--input-dir",
        str(Path(input_dir).resolve()),
        "--output-dir",
        str(Path(output_dir).resolve()),
        "--num-group",
        "256",
        "--point-dropout-views",
        str(views),
        "--point-dropout-max-ratio",
        str(max_ratio),
    ]
    _run(cmd, cwd=ROOT)


def train_variant(
    *,
    graph_dir: Path,
    emb_dir: Path,
    output_ckpt: Path,
    dropout: float,
    epochs: int,
    holdout_stem: str,
) -> list[Path]:
    all_npz = sorted(graph_dir.glob("*_graph.npz"))
    train_npz = [
        p for p in all_npz if p.stem.replace("_graph", "") != holdout_stem
    ]
    if not train_npz:
        raise RuntimeError("no training graph npz after holdout exclusion")
    # train_graph expects a directory of npz; stage a train-only dir via symlinks
    train_dir = output_ckpt.parent / f"{output_ckpt.stem}_train_npz"
    if train_dir.exists():
        for old in train_dir.glob("*"):
            old.unlink()
    else:
        train_dir.mkdir(parents=True, exist_ok=True)
    for src in train_npz:
        link = train_dir / src.name
        if link.exists() or link.is_symlink():
            link.unlink()
        link.symlink_to(src.resolve())

    _run(
        [
            _python(),
            str(DOGHOUSE_DIR / "train_graph.py"),
            "--data-dir",
            str(train_dir),
            "--train-full",
            "--output",
            str(output_ckpt),
            "--epochs",
            str(epochs),
            "--dropout",
            str(dropout),
            "--pmae-dir",
            str(emb_dir),
        ],
        cwd=ROOT,
    )
    return train_npz


def eval_model(
    *,
    step_dir: Path,
    stem: str,
    out_dir: Path,
    checkpoint: Path,
    emb_dir: Path,
    sample_points: int,
    gallery: Path,
) -> dict:
    sys.path.insert(0, str(DOGHOUSE_DIR))
    from eval_face_predictions import evaluate_face_predictions

    model_out = out_dir / stem
    model_out.mkdir(parents=True, exist_ok=True)
    step_path = step_dir / f"{stem}.step"
    pred_json = model_out / f"{stem}_doghouse_pred_faces.json"
    cmd = [
        _python(),
        str(DOGHOUSE_DIR / "infer_from_step.py"),
        "--step",
        str(step_path),
        "--output-dir",
        str(model_out),
        "--backbone",
        "graph",
        "--checkpoint",
        str(checkpoint),
        "--pmae-face-emb-dir",
        str(emb_dir),
        "--sample-points-per-face",
        str(sample_points),
        "--min-instance-faces",
        "2",
        "--instance-sim-filter",
        "--instance-sim-gallery",
        str(gallery),
    ]
    t0 = time.perf_counter()
    _run(cmd, cwd=ROOT)
    elapsed = time.perf_counter() - t0
    label = label_path_for_stem(step_dir, stem)
    metrics = evaluate_face_predictions(label, pred_json)
    metrics["model"] = stem
    metrics["inference_s"] = round(elapsed, 3)
    metrics["prediction"] = str(pred_json)
    return metrics


def load_baseline_holdout() -> dict | None:
    if not BASELINE_SUMMARY.exists():
        return None
    data = json.loads(BASELINE_SUMMARY.read_text(encoding="utf-8"))
    for row in data.get("models", []):
        if row.get("model") == HOLDOUT_STEM:
            return {
                "face_iou": row["after"]["face_iou"],
                "pred_instances": row["after"]["pred_instances"],
                "gt_instances": row["after"]["gt_instances"],
                "extra_count": row["after"]["extra_count"],
                "mean_after_face_iou": data.get("mean_after_face_iou"),
            }
    return None


def summarize_variant(name: str, rows: list[dict], baseline_holdout: dict | None) -> dict:
    holdout = next((r for r in rows if r["model"] == HOLDOUT_STEM), None)
    train_rows = [r for r in rows if r["model"] != HOLDOUT_STEM]
    mean_all = float(np.mean([r["face_iou"] for r in rows])) if rows else 0.0
    mean_train = float(np.mean([r["face_iou"] for r in train_rows])) if train_rows else 0.0
    total_extra = int(sum(r["extra_count"] for r in rows))
    decision = {
        "promote_to_production": False,
        "reasons": [],
    }
    if holdout is None:
        decision["reasons"].append("holdout metrics missing")
    else:
        base_iou = float(baseline_holdout["face_iou"]) if baseline_holdout else None
        base_extra = int(baseline_holdout["extra_count"]) if baseline_holdout else None
        base_mean = float(baseline_holdout["mean_after_face_iou"]) if baseline_holdout else None
        if base_iou is not None and holdout["face_iou"] > base_iou:
            decision["reasons"].append(
                f"holdout Face IoU improved {base_iou:.4f} -> {holdout['face_iou']:.4f}"
            )
        else:
            decision["reasons"].append(
                f"holdout Face IoU not improved ({holdout['face_iou']:.4f}"
                + (f" vs baseline {base_iou:.4f})" if base_iou is not None else ")")
            )
        if base_extra is not None and holdout["extra_count"] <= base_extra:
            decision["reasons"].append(
                f"holdout extra_count ok ({holdout['extra_count']} <= {base_extra})"
            )
        else:
            decision["reasons"].append(
                f"holdout extra_count not better ({holdout['extra_count']})"
            )
        if base_mean is not None and mean_all >= base_mean - 0.01:
            decision["reasons"].append(
                f"mean Face IoU within 0.01 of baseline ({mean_all:.4f} vs {base_mean:.4f})"
            )
        else:
            decision["reasons"].append(
                f"mean Face IoU regression risk ({mean_all:.4f}"
                + (f" vs baseline {base_mean:.4f})" if base_mean is not None else ")")
            )
        decision["promote_to_production"] = bool(
            base_iou is not None
            and holdout["face_iou"] > base_iou
            and holdout["extra_count"] <= (base_extra if base_extra is not None else holdout["extra_count"])
            and (base_mean is None or mean_all >= base_mean - 0.01)
        )
    return {
        "variant": name,
        "mean_face_iou": round(mean_all, 6),
        "mean_train_face_iou": round(mean_train, 6),
        "total_extra": total_extra,
        "holdout": holdout,
        "models": rows,
        "decision": decision,
    }


def run_ablation(args: argparse.Namespace) -> dict:
    step_dir = Path(args.step_dir).resolve()
    out_root = Path(args.output_dir).resolve()
    out_root.mkdir(parents=True, exist_ok=True)
    graph_dir = out_root / "graph_train_s200"
    pmae_input_dir = out_root / "pmae_input_s200"
    ckpt_dir = out_root / "checkpoints"
    ckpt_dir.mkdir(parents=True, exist_ok=True)
    gallery = Path(args.gallery).resolve()
    pmae_ckpt = Path(args.pmae_ckpt).resolve()

    models = discover_eval_models(step_dir)
    if HOLDOUT_STEM not in models:
        raise RuntimeError(f"holdout {HOLDOUT_STEM} not found among labeled models: {models}")

    if not args.skip_prepare:
        prepare_graph(step_dir, graph_dir, sample_points=200)
        export_pmae_inputs(graph_dir, pmae_input_dir)

    baseline_holdout = load_baseline_holdout()
    variant_summaries = []
    selected = args.variants or list(VARIANTS)

    # Extract embeddings once per unique emb_subdir.
    emb_jobs: dict[str, dict] = {}
    for name in selected:
        cfg = VARIANTS[name]
        emb_jobs.setdefault(
            cfg["emb_subdir"],
            {
                "views": cfg["point_dropout_views"],
                "max_ratio": cfg["point_dropout_max_ratio"],
            },
        )
    if not args.skip_pmae:
        for emb_subdir, job in emb_jobs.items():
            extract_pmae(
                ckpt=pmae_ckpt,
                input_dir=pmae_input_dir,
                output_dir=out_root / emb_subdir,
                views=job["views"],
                max_ratio=job["max_ratio"],
            )

    for name in selected:
        cfg = VARIANTS[name]
        emb_dir = out_root / cfg["emb_subdir"]
        ckpt_path = ckpt_dir / f"doghouse_graph_{name}.pt"
        eval_dir = out_root / "eval" / name
        if not args.skip_train:
            train_variant(
                graph_dir=graph_dir,
                emb_dir=emb_dir,
                output_ckpt=ckpt_path,
                dropout=cfg["dropout"],
                epochs=args.epochs,
                holdout_stem=HOLDOUT_STEM,
            )
        rows = []
        for stem in models:
            rows.append(
                eval_model(
                    step_dir=step_dir,
                    stem=stem,
                    out_dir=eval_dir,
                    checkpoint=ckpt_path,
                    emb_dir=emb_dir,
                    sample_points=cfg["sample_points_per_face"],
                    gallery=gallery,
                )
            )
        summary = summarize_variant(name, rows, baseline_holdout)
        summary["checkpoint"] = str(ckpt_path)
        summary["pmae_face_emb_dir"] = str(emb_dir)
        summary["config"] = cfg
        variant_summaries.append(summary)
        (out_root / f"summary_{name}.json").write_text(
            json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8"
        )
        print(
            f"[{name}] mean_iou={summary['mean_face_iou']:.4f} "
            f"holdout_iou={summary['holdout']['face_iou'] if summary['holdout'] else 'n/a'} "
            f"promote={summary['decision']['promote_to_production']}",
            flush=True,
        )

    report = {
        "schema": "doghouse_sample200_dropout_ablation.v1",
        "holdout": HOLDOUT_STEM,
        "baseline_holdout": baseline_holdout,
        "variants": variant_summaries,
        "recommendation": _recommend(variant_summaries),
    }
    summary_path = out_root / "summary.json"
    summary_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"saved: {summary_path}")
    print("recommendation:", json.dumps(report["recommendation"], ensure_ascii=False))
    return report


def _recommend(variant_summaries: list[dict]) -> dict:
    promotable = [v for v in variant_summaries if v["decision"]["promote_to_production"]]
    if not promotable:
        return {
            "action": "keep_production_defaults",
            "best_variant": None,
            "note": "No variant met success criteria; do not switch pipeline_defaults.",
        }
    best = max(
        promotable,
        key=lambda v: (
            (v["holdout"] or {}).get("face_iou", 0.0),
            v["mean_face_iou"],
        ),
    )
    return {
        "action": "candidate_for_production",
        "best_variant": best["variant"],
        "note": (
            f"{best['variant']} met criteria but was NOT auto-switched; "
            "review summary before updating pipeline_defaults."
        ),
    }


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "--step-dir",
        default=str(DOGHOUSE_DIR / "step - 副本2"),
    )
    p.add_argument(
        "--output-dir",
        default=str(DOGHOUSE_DIR / "ablation_s200_dropout"),
    )
    p.add_argument(
        "--pmae-ckpt",
        default=str(DOGHOUSE_DIR / "ckpt-last.pth"),
    )
    p.add_argument(
        "--gallery",
        default=str(
            DOGHOUSE_DIR
            / "checkpoints"
            / "doghouse_instance_similarity_gallery_7_plus_B126302301001.npz"
        ),
    )
    p.add_argument("--epochs", type=int, default=200)
    p.add_argument(
        "--variants",
        nargs="+",
        choices=sorted(VARIANTS),
        default=None,
        help="Subset of variants to run (default: all)",
    )
    p.add_argument("--skip-prepare", action="store_true")
    p.add_argument("--skip-pmae", action="store_true")
    p.add_argument("--skip-train", action="store_true")
    return p


def main() -> int:
    args = build_arg_parser().parse_args()
    run_ablation(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
