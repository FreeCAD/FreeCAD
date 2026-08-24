#!/usr/bin/env python3
"""Instance-level doghouse similarity using whole-component geometry/topology."""

from __future__ import annotations

import copy
import json
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

import numpy as np

try:
    from .labels import NEGATIVE_ROLES, ROLE_TO_ID
except ImportError:
    from labels import NEGATIVE_ROLES, ROLE_TO_ID


NUMERIC_FEATURE_NAMES = (
    "face_count",
    "internal_edges",
    "edge_density",
    "area_sum",
    "area_mean",
    "area_std",
    "area_max",
    "bbox_x",
    "bbox_y",
    "bbox_z",
    "bbox_diag",
    "plane_ratio",
    "cylinder_ratio",
    "cone_ratio",
    "sphere_ratio",
    "torus_ratio",
    "freeform_ratio",
    "has_radius_ratio",
    "span_mean",
    "span_max",
)


@dataclass(frozen=True)
class InstanceSignature:
    faces: tuple[int, ...]
    numeric: np.ndarray
    pmae: np.ndarray
    face_count: int
    internal_edges: int


NEGATIVE_IDS = frozenset(ROLE_TO_ID[role] for role in NEGATIVE_ROLES)


def _instance_faces(result: dict) -> list[tuple[int, list[int]]]:
    out = []
    for inst in result.get("doghouse_instances", []):
        faces = sorted(int(fi) for fi in inst.get("faces", []))
        if faces:
            out.append((int(inst["instance_id"]), faces))
    return sorted(out, key=lambda row: row[0])


def _component_bbox(data: dict[str, np.ndarray], faces: list[int], face_features: np.ndarray) -> np.ndarray:
    if "points" in data and "face_idx" in data:
        face_idx = np.asarray(data["face_idx"], dtype=np.int64)
        mask = np.isin(face_idx, np.asarray(faces, dtype=np.int64))
        if np.any(mask):
            pts = np.asarray(data["points"], dtype=np.float32)[mask]
            return (pts.max(axis=0) - pts.min(axis=0)).astype(np.float32)
    if face_features.shape[1] >= 9:
        return np.max(face_features[:, 6:9], axis=0).astype(np.float32)
    return np.zeros(3, dtype=np.float32)


def _internal_edge_count(adjacency: np.ndarray, faces: set[int]) -> int:
    if adjacency.size == 0:
        return 0
    count = 0
    for a, b in adjacency.astype(np.int64):
        if int(a) in faces and int(b) in faces:
            count += 1
    return count


def _components(faces: set[int], adjacency: np.ndarray) -> list[list[int]]:
    if not faces:
        return []
    graph: dict[int, set[int]] = defaultdict(set)
    for a, b in adjacency.astype(np.int64):
        a = int(a)
        b = int(b)
        if a in faces and b in faces:
            graph[a].add(b)
            graph[b].add(a)
    seen: set[int] = set()
    out: list[list[int]] = []
    for start in sorted(faces):
        if start in seen:
            continue
        stack = [start]
        seen.add(start)
        comp: list[int] = []
        while stack:
            cur = stack.pop()
            comp.append(cur)
            for nb in graph.get(cur, ()):
                if nb not in seen:
                    seen.add(nb)
                    stack.append(nb)
        out.append(sorted(comp))
    return out


def _safe_ratio(mask: np.ndarray, n: int) -> float:
    return float(mask.sum()) / max(n, 1)


def extract_instance_signature(
    data: dict[str, np.ndarray],
    instance_faces: list[int] | tuple[int, ...] | set[int],
) -> InstanceSignature:
    """Extract whole-component geometry/topology signature for one candidate instance."""
    faces = tuple(sorted(int(fi) for fi in instance_faces))
    if not faces:
        raise ValueError("instance_faces must not be empty")

    face_features_all = np.asarray(data["face_features"], dtype=np.float32)
    f = face_features_all[np.asarray(faces, dtype=np.int64)]
    n = len(faces)
    areas = f[:, 1] if f.shape[1] > 1 else np.zeros(n, dtype=np.float32)
    spans = f[:, 6:9] if f.shape[1] >= 9 else np.zeros((n, 3), dtype=np.float32)
    face_type = f[:, 0].astype(np.int64) if f.shape[1] > 0 else np.zeros(n, dtype=np.int64)
    has_radius = f[:, 3] if f.shape[1] > 3 else np.zeros(n, dtype=np.float32)
    bbox = _component_bbox(data, list(faces), f)
    bbox_diag = float(np.linalg.norm(bbox))
    internal_edges = _internal_edge_count(
        np.asarray(data.get("adjacency", np.empty((0, 2))), dtype=np.int64),
        set(faces),
    )
    max_edges = n * (n - 1) / 2.0
    edge_density = float(internal_edges) / max(max_edges, 1.0)

    numeric = np.asarray(
        [
            float(n),
            float(internal_edges),
            edge_density,
            float(areas.sum()),
            float(areas.mean()) if areas.size else 0.0,
            float(areas.std()) if areas.size else 0.0,
            float(areas.max()) if areas.size else 0.0,
            float(bbox[0]),
            float(bbox[1]),
            float(bbox[2]),
            bbox_diag,
            _safe_ratio(face_type == 1, n),
            _safe_ratio(face_type == 2, n),
            _safe_ratio(face_type == 3, n),
            _safe_ratio(face_type == 4, n),
            _safe_ratio(face_type == 5, n),
            _safe_ratio((face_type == 0) | (face_type >= 6), n),
            float(np.count_nonzero(has_radius > 0.5)) / max(n, 1),
            float(spans.mean()) if spans.size else 0.0,
            float(spans.max()) if spans.size else 0.0,
        ],
        dtype=np.float32,
    )

    if "face_pmae" in data:
        p = np.asarray(data["face_pmae"], dtype=np.float32)[np.asarray(faces, dtype=np.int64)]
        pmae = p.mean(axis=0).astype(np.float32)
        norm = float(np.linalg.norm(pmae))
        if norm > 1e-6:
            pmae = pmae / norm
    else:
        pmae = np.empty(0, dtype=np.float32)

    return InstanceSignature(
        faces=faces,
        numeric=numeric,
        pmae=pmae.astype(np.float32),
        face_count=n,
        internal_edges=internal_edges,
    )


def _normalized_numeric(sig: InstanceSignature, gallery: dict) -> np.ndarray:
    mean = np.asarray(gallery.get("numeric_mean", np.zeros_like(sig.numeric)), dtype=np.float32)
    std = np.asarray(gallery.get("numeric_std", np.ones_like(sig.numeric)), dtype=np.float32)
    return (sig.numeric - mean) / np.maximum(std, 1e-6)


def _numeric_similarity(query: np.ndarray, gallery_numeric: np.ndarray) -> np.ndarray:
    if gallery_numeric.size == 0:
        return np.empty(0, dtype=np.float32)
    d = np.linalg.norm(gallery_numeric - query.reshape(1, -1), axis=1)
    return np.exp(-d / max(np.sqrt(query.shape[0]), 1.0)).astype(np.float32)


def _cosine_similarity(query: np.ndarray, gallery_pmae: np.ndarray) -> np.ndarray:
    if query.size == 0 or gallery_pmae.size == 0:
        return np.empty(0, dtype=np.float32)
    q = query / max(float(np.linalg.norm(query)), 1e-6)
    g = gallery_pmae / np.maximum(np.linalg.norm(gallery_pmae, axis=1, keepdims=True), 1e-6)
    return (g @ q).astype(np.float32)


def score_instance_similarity(
    sig: InstanceSignature,
    gallery: dict,
    *,
    pmae_weight: float = 0.45,
    negative_weight: float = 0.75,
) -> dict:
    """Score one candidate against positive and negative whole-instance prototypes."""
    qn = _normalized_numeric(sig, gallery)
    pos_num = np.asarray(gallery.get("positive_numeric_norm", gallery.get("positive_numeric", np.empty((0, len(NUMERIC_FEATURE_NAMES))))), dtype=np.float32)
    neg_num = np.asarray(gallery.get("negative_numeric_norm", gallery.get("negative_numeric", np.empty((0, len(NUMERIC_FEATURE_NAMES))))), dtype=np.float32)
    if "positive_numeric_norm" not in gallery:
        pos_num = (pos_num - np.asarray(gallery.get("numeric_mean", 0), dtype=np.float32)) / np.maximum(np.asarray(gallery.get("numeric_std", 1), dtype=np.float32), 1e-6)
    if "negative_numeric_norm" not in gallery:
        neg_num = (neg_num - np.asarray(gallery.get("numeric_mean", 0), dtype=np.float32)) / np.maximum(np.asarray(gallery.get("numeric_std", 1), dtype=np.float32), 1e-6)

    pos_struct = _numeric_similarity(qn, pos_num)
    neg_struct = _numeric_similarity(qn, neg_num)
    pos_pmae = _cosine_similarity(sig.pmae, np.asarray(gallery.get("positive_pmae", np.empty((0, 0))), dtype=np.float32))
    neg_pmae = _cosine_similarity(sig.pmae, np.asarray(gallery.get("negative_pmae", np.empty((0, 0))), dtype=np.float32))

    if pos_pmae.size and pos_struct.size:
        pos_all = (1.0 - pmae_weight) * pos_struct + pmae_weight * ((pos_pmae + 1.0) * 0.5)
    else:
        pos_all = pos_struct
    if neg_pmae.size and neg_struct.size:
        neg_all = (1.0 - pmae_weight) * neg_struct + pmae_weight * ((neg_pmae + 1.0) * 0.5)
    else:
        neg_all = neg_struct

    pos_sim = float(pos_all.max()) if pos_all.size else 0.0
    neg_sim = float(neg_all.max()) if neg_all.size else 0.0
    keep_score = pos_sim - negative_weight * neg_sim
    return {
        "pos_sim": round(pos_sim, 6),
        "neg_sim": round(neg_sim, 6),
        "keep_score": round(float(keep_score), 6),
    }


def load_similarity_gallery(path: str | Path) -> dict:
    return dict(np.load(path, allow_pickle=True))


def save_similarity_gallery(path: str | Path, gallery: dict) -> None:
    out = Path(path)
    out.parent.mkdir(parents=True, exist_ok=True)
    np.savez(out, **gallery)


def _instances_from_labels(data: dict) -> list[list[int]]:
    return [
        sorted(int(fi) for fi in inst.get("faces", []))
        for inst in data.get("doghouse_instances", [])
        if inst.get("faces")
    ]


def _instances_from_prediction(data: dict) -> list[tuple[int, set[int]]]:
    instances = [
        (int(inst["instance_id"]), {int(fi) for fi in inst.get("faces", [])})
        for inst in data.get("doghouse_instances", [])
        if inst.get("faces")
    ]
    if instances:
        return instances
    by_id: dict[int, set[int]] = {}
    for row in data.get("face_predictions", []):
        if int(row.get("doghouse", 0)) <= 0:
            continue
        iid = int(row.get("instance_id", -1))
        if iid > 0:
            by_id.setdefault(iid, set()).add(int(row["face_idx"]))
    return sorted(by_id.items())


def _iou(a: set[int], b: set[int]) -> float:
    return len(a & b) / max(len(a | b), 1)


def _json_faces(faces) -> list[int]:
    return [int(fi) for fi in faces]


def _model_name(path: Path, data: dict[str, np.ndarray]) -> str:
    if "model_name" in data:
        value = np.asarray(data["model_name"]).reshape(-1)[0]
        return str(value)
    return path.stem.replace("_graph", "")


def _find_label(label_dir: Path | None, name: str) -> Path | None:
    if label_dir is None:
        return None
    candidates = [
        label_dir / f"{name}_annotation.json",
        label_dir / f"{name} annotation.json",
        label_dir / f"{name}.json",
    ]
    return next((p for p in candidates if p.exists()), None)


def _signatures_to_arrays(sigs: list[InstanceSignature]) -> tuple[np.ndarray, np.ndarray]:
    if not sigs:
        return (
            np.empty((0, len(NUMERIC_FEATURE_NAMES)), dtype=np.float32),
            np.empty((0, 0), dtype=np.float32),
        )
    numeric = np.stack([sig.numeric for sig in sigs]).astype(np.float32)
    pmae_dims = [sig.pmae.shape[0] for sig in sigs if sig.pmae.size]
    if pmae_dims and len(set(pmae_dims)) == 1 and all(
        sig.pmae.size and sig.pmae.shape[0] == pmae_dims[0] for sig in sigs
    ):
        pmae = np.stack([sig.pmae for sig in sigs]).astype(np.float32)
    else:
        pmae = np.empty((len(sigs), 0), dtype=np.float32)
    return numeric, pmae


def build_instance_gallery(
    graph_npz_list: list[Path],
    *,
    prediction_dirs: list[Path] | None = None,
    positive_prediction_dirs: list[Path] | None = None,
    label_dir: Path | None = None,
    extra_iou: float = 0.2,
    positive_cover: float = 0.8,
    threshold: float = 0.0,
) -> dict[str, np.ndarray]:
    """Build positive/negative whole-instance prototypes from labels and predictions.

    ``positive_prediction_dirs`` adds predicted components that cover enough GT
    faces (max cover >= ``positive_cover``) as extra positive morphologies. This
    is used when a model merges several GT doghouses into one larger component
    (e.g. structure-head M5) so the similarity filter does not reject them.
    """
    positive: list[InstanceSignature] = []
    negative: list[InstanceSignature] = []
    meta: list[dict] = []

    for path in graph_npz_list:
        data = dict(np.load(path, allow_pickle=True))
        name = _model_name(path, data)
        adjacency = np.asarray(data.get("adjacency", np.empty((0, 2))), dtype=np.int64)

        if "face_instance" in data and "face_doghouse" in data:
            face_instance = np.asarray(data["face_instance"], dtype=np.int64)
            face_doghouse = np.asarray(data["face_doghouse"], dtype=np.int64)
            for iid in sorted(int(v) for v in np.unique(face_instance) if int(v) > 0):
                faces = set(np.where((face_instance == iid) & (face_doghouse == 1))[0].astype(int))
                for comp in _components(faces, adjacency):
                    positive.append(extract_instance_signature(data, comp))
                    meta.append({"model": name, "source": "gt_npz", "label": "positive", "faces": _json_faces(comp)})

        if "face_semantic" in data:
            face_semantic = np.asarray(data["face_semantic"], dtype=np.int64)
            neg_faces = set(np.where(np.isin(face_semantic, list(NEGATIVE_IDS)))[0].astype(int))
            for comp in _components(neg_faces, adjacency):
                negative.append(extract_instance_signature(data, comp))
                meta.append({"model": name, "source": "hard_negative", "label": "negative", "faces": _json_faces(comp)})

        label_path = _find_label(label_dir, name)
        labels = json.loads(label_path.read_text(encoding="utf-8")) if label_path else None
        gt_instances = [set(faces) for faces in _instances_from_labels(labels or {})]
        if labels:
            for faces in _instances_from_labels(labels):
                positive.append(extract_instance_signature(data, faces))
                meta.append({"model": name, "source": "label_json", "label": "positive", "faces": _json_faces(faces)})

        # GT components from npz face_instance (for cover checks even without label json).
        if not gt_instances and "face_instance" in data and "face_doghouse" in data:
            face_instance = np.asarray(data["face_instance"], dtype=np.int64)
            face_doghouse = np.asarray(data["face_doghouse"], dtype=np.int64)
            for iid in sorted(int(v) for v in np.unique(face_instance) if int(v) > 0):
                faces = set(np.where((face_instance == iid) & (face_doghouse == 1))[0].astype(int))
                if faces:
                    gt_instances.append(faces)

        for pred_dir in prediction_dirs or []:
            pred_path = pred_dir / name / f"{name}_doghouse_pred_faces.json"
            if not pred_path.exists() or not gt_instances:
                continue
            prediction = json.loads(pred_path.read_text(encoding="utf-8"))
            for iid, faces in _instances_from_prediction(prediction):
                best_iou = max((_iou(faces, gt) for gt in gt_instances), default=0.0)
                if best_iou < extra_iou:
                    comp = sorted(faces)
                    negative.append(extract_instance_signature(data, comp))
                    meta.append(
                        {
                            "model": name,
                            "source": "prediction_extra",
                            "label": "negative",
                            "instance_id": int(iid),
                            "best_iou": round(float(best_iou), 6),
                            "faces": _json_faces(comp),
                        }
                    )

        for pred_dir in positive_prediction_dirs or []:
            pred_path = pred_dir / name / f"{name}_doghouse_pred_faces.json"
            if not pred_path.exists() or not gt_instances:
                continue
            prediction = json.loads(pred_path.read_text(encoding="utf-8"))
            # Prefer unfiltered / rejected morphologies when present.
            candidates = list(_instances_from_prediction(prediction))
            for row in prediction.get("rejected_instance_similarity") or []:
                faces = {int(fi) for fi in row.get("faces", [])}
                if faces:
                    candidates.append((int(row.get("instance_id", -1)), faces))
            seen_face_keys: set[tuple[int, ...]] = set()
            for iid, faces in candidates:
                key = tuple(sorted(faces))
                if key in seen_face_keys:
                    continue
                seen_face_keys.add(key)
                covers = [len(faces & gt) / max(len(gt), 1) for gt in gt_instances]
                best_cover = max(covers) if covers else 0.0
                if best_cover < float(positive_cover):
                    continue
                positive.append(extract_instance_signature(data, sorted(faces)))
                meta.append(
                    {
                        "model": name,
                        "source": "prediction_positive_morphology",
                        "label": "positive",
                        "instance_id": int(iid),
                        "best_cover": round(float(best_cover), 6),
                        "faces": _json_faces(faces),
                    }
                )

    if not positive:
        raise ValueError("instance similarity gallery requires at least one positive instance")
    if not negative:
        raise ValueError("instance similarity gallery requires at least one negative instance")

    pos_numeric, pos_pmae = _signatures_to_arrays(positive)
    neg_numeric, neg_pmae = _signatures_to_arrays(negative)
    all_numeric = np.vstack([pos_numeric, neg_numeric]).astype(np.float32)
    mean = all_numeric.mean(axis=0).astype(np.float32)
    std = np.maximum(all_numeric.std(axis=0), 1e-6).astype(np.float32)
    gallery = {
        "positive_numeric": pos_numeric,
        "negative_numeric": neg_numeric,
        "positive_numeric_norm": ((pos_numeric - mean) / std).astype(np.float32),
        "negative_numeric_norm": ((neg_numeric - mean) / std).astype(np.float32),
        "positive_pmae": pos_pmae,
        "negative_pmae": neg_pmae,
        "numeric_mean": mean,
        "numeric_std": std,
        "threshold": np.asarray([float(threshold)], dtype=np.float32),
        "train_positive": np.asarray([len(positive)], dtype=np.int64),
        "train_negative": np.asarray([len(negative)], dtype=np.int64),
        "numeric_feature_names": np.asarray(NUMERIC_FEATURE_NAMES),
        "meta_json": np.asarray([json.dumps(meta, ensure_ascii=False)]),
    }
    return gallery


def apply_instance_similarity_filter(
    result: dict,
    data: dict[str, np.ndarray],
    gallery_path: str | Path,
    *,
    threshold: float | None = None,
) -> dict:
    gallery = load_similarity_gallery(gallery_path)
    keep_threshold = float(np.asarray(gallery.get("threshold", 0.0)).reshape(-1)[0] if threshold is None else threshold)
    filtered = copy.deepcopy(result)
    kept = []
    rejected = []
    old_to_new = {}
    for inst in filtered.get("doghouse_instances", []):
        faces = [int(fi) for fi in inst.get("faces", [])]
        sig = extract_instance_signature(data, faces)
        score = score_instance_similarity(sig, gallery)
        inst["instance_similarity"] = score
        if float(score["keep_score"]) >= keep_threshold:
            new_iid = len(kept) + 1
            old_to_new[int(inst["instance_id"])] = new_iid
            inst["instance_id"] = new_iid
            kept.append(inst)
        else:
            rejected.append(inst)

    rejected_ids = {int(inst["instance_id"]) for inst in rejected}
    for row in filtered.get("face_predictions", []):
        iid = int(row.get("instance_id", -1))
        if iid in rejected_ids:
            row["doghouse"] = 0
            row["instance_id"] = -1
            row["role"] = "background"
            row["instance_similarity_rejected"] = True
        elif iid in old_to_new:
            row["instance_id"] = old_to_new[iid]

    filtered["doghouse_instances"] = kept
    filtered["rejected_instance_similarity"] = rejected
    filtered["instance_similarity_filter"] = {
        "path": str(gallery_path),
        "threshold": round(keep_threshold, 6),
        "numeric_feature_names": list(NUMERIC_FEATURE_NAMES),
    }
    return filtered


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--prediction", required=True)
    parser.add_argument("--npz", required=True)
    parser.add_argument("--gallery", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--threshold", type=float)
    args = parser.parse_args()

    result = json.loads(Path(args.prediction).read_text(encoding="utf-8"))
    data = dict(np.load(args.npz, allow_pickle=True))
    filtered = apply_instance_similarity_filter(result, data, args.gallery, threshold=args.threshold)
    out = Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(filtered, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"saved: {out}")
    print(f"kept instances: {len(filtered['doghouse_instances'])}")
    print(f"rejected instances: {len(filtered.get('rejected_instance_similarity', []))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
