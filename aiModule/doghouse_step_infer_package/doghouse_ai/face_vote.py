#!/usr/bin/env python3
"""Project point predictions back to CAD face labels by face_idx voting."""

from __future__ import annotations

import argparse
import json
from collections import Counter, defaultdict
from pathlib import Path

import numpy as np

try:
    from .labels import ID_TO_ROLE
except ImportError:
    from labels import ID_TO_ROLE


def _majority(values):
    if len(values) == 0:
        return 0
    c = Counter(int(x) for x in values)
    return c.most_common(1)[0][0]


def _label_components(dog_faces: set[int], graph: dict[int, set[int]]) -> dict[int, int]:
    """Label each doghouse face with its connected-component id (1-based)."""
    seen: dict[int, int] = {}
    cid = 0
    for start in sorted(dog_faces):
        if start in seen:
            continue
        cid += 1
        stack = [start]
        seen[start] = cid
        while stack:
            cur = stack.pop()
            for nb in graph.get(cur, ()):  # type: ignore[union-attr]
                if nb in dog_faces and nb not in seen:
                    seen[nb] = cid
                    stack.append(nb)
    return seen


def _close_mask(
    dog_faces: set[int],
    adjacency: np.ndarray,
    *,
    ratio: float = 0.66,
    iters: int = 4,
) -> set[int]:
    """Morphological closing on the face adjacency graph — fill interior holes.

    A background face is flipped to doghouse only when a **single** existing
    component already surrounds it (>= ``ratio`` of its neighbours belong to
    that one component, and it has >= 2 doghouse neighbours). Requiring the
    dominant neighbours to share **one** component id prevents bridging two
    distinct doghouses together, so the operation reconnects fragments of the
    same instance without merging separate instances.

    This removes the brittle dependence on ``min_component_faces``: a doghouse
    that the point model split into small pieces (a few interior faces dipping
    below threshold) is rejoined into one full-size component, while genuine
    noise stays tiny.
    """
    if adjacency is None or not len(adjacency) or not dog_faces:
        return set(dog_faces)
    full: dict[int, set[int]] = defaultdict(set)
    for a, b in adjacency:
        a = int(a)
        b = int(b)
        full[a].add(b)
        full[b].add(a)
    all_faces = set(full)
    current = set(dog_faces)
    for _ in range(max(0, int(iters))):
        labels = _label_components(current, full)
        additions: dict[int, int] = {}
        for fi in all_faces - current:
            nbrs = full.get(fi, set())
            dog_nbrs = [x for x in nbrs if x in current]
            if len(dog_nbrs) < 2:
                continue
            counts = Counter(labels[x] for x in dog_nbrs)
            _, top = counts.most_common(1)[0]
            if top / max(len(nbrs), 1) >= ratio:
                additions[fi] = 1
        if not additions:
            break
        current |= set(additions)
    return current


def _component_instance_ids(
    dog_faces: set[int],
    adjacency: np.ndarray,
    *,
    min_faces: int = 8,
) -> dict[int, int]:
    """Split doghouse face mask into CAD adjacency connected components."""
    if not dog_faces:
        return {}
    graph = defaultdict(set)
    for a, b in adjacency:
        a = int(a)
        b = int(b)
        if a in dog_faces and b in dog_faces:
            graph[a].add(b)
            graph[b].add(a)

    seen = set()
    comps = []
    for start in sorted(dog_faces):
        if start in seen:
            continue
        stack = [start]
        seen.add(start)
        comp = []
        while stack:
            cur = stack.pop()
            comp.append(cur)
            for nb in graph.get(cur, ()):
                if nb not in seen:
                    seen.add(nb)
                    stack.append(nb)
        comps.append(sorted(comp))

    # Stable numbering: large components first, then by first face.
    comps = [c for c in comps if len(c) >= int(min_faces)]
    comps.sort(key=lambda c: (-len(c), c[0]))
    out = {}
    for iid, comp in enumerate(comps, 1):
        for fi in comp:
            out[int(fi)] = iid
    return out


def vote_faces(
    face_idx: np.ndarray,
    semantic_pred: np.ndarray,
    instance_pred: np.ndarray | None = None,
    doghouse_pred: np.ndarray | None = None,
    adjacency: np.ndarray | None = None,
    threshold: float = 0.5,
    min_component_faces: int = 8,
    close_ratio: float = 0.66,
    close_iters: int = 4,
) -> dict:
    """Return face-level doghouse predictions from point predictions."""
    by_face = defaultdict(list)
    for i, fi in enumerate(face_idx):
        by_face[int(fi)].append(i)

    rows = []
    dog_faces = set()
    for fi in sorted(by_face):
        idxs = np.asarray(by_face[fi], dtype=np.int64)
        sem = semantic_pred[idxs]
        sem_id = _majority(sem)
        if doghouse_pred is None:
            dog_ratio = float(np.mean(sem > 0))
            is_doghouse = dog_ratio >= threshold or sem_id > 0
        else:
            dog_ratio = float(np.mean(doghouse_pred[idxs] > 0))
            is_doghouse = dog_ratio >= threshold

        if instance_pred is None:
            iid = -1
        else:
            positive = instance_pred[idxs]
            positive = positive[positive > 0]
            iid = _majority(positive) if len(positive) else -1
        if is_doghouse and iid <= 0:
            iid = 1

        role = ID_TO_ROLE.get(int(sem_id), "background")
        if is_doghouse and role == "background":
            role = "doghouse"

        row = {
            "face_idx": int(fi),
            "doghouse": 1 if is_doghouse else 0,
            "instance_id": int(iid if is_doghouse else -1),
            "role": role if is_doghouse else "background",
            "doghouse_ratio": round(dog_ratio, 6),
        }
        rows.append(row)
        if is_doghouse:
            dog_faces.add(int(fi))

    if instance_pred is None and adjacency is not None and len(adjacency):
        # Fill interior mask holes so a doghouse split into fragments by a few
        # sub-threshold faces is rejoined into one component (model-agnostic;
        # avoids relying on min_component_faces to recover small instances).
        closed_faces = _close_mask(
            dog_faces, adjacency, ratio=close_ratio, iters=close_iters,
        )
        component_ids = _component_instance_ids(
            closed_faces, adjacency, min_faces=min_component_faces,
        )
        row_by_face = {int(r["face_idx"]): r for r in rows}
        for fi, iid in component_ids.items():
            row = row_by_face.get(int(fi))
            if row is None:
                continue
            if not row["doghouse"]:  # face added by closing
                row["doghouse"] = 1
                if row["role"] == "background":
                    row["role"] = "doghouse"
            row["instance_id"] = int(iid)
        for row in rows:
            if row["doghouse"] and int(row["face_idx"]) not in component_ids:
                row["doghouse"] = 0
                row["instance_id"] = -1
                row["role"] = "background"

    instances = defaultdict(set)
    for row in rows:
        if row["doghouse"]:
            instances[int(row["instance_id"])].add(int(row["face_idx"]))

    return {
        "schema": "doghouse_face_predictions.v1",
        "face_predictions": rows,
        "doghouse_instances": [
            {"instance_id": int(iid), "faces": sorted(faces)}
            for iid, faces in sorted(instances.items())
            if iid > 0
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--npz", required=True, help="Dataset .npz containing face_idx")
    parser.add_argument("--semantic-pred", help="Optional .npy point semantic predictions")
    parser.add_argument("--instance-pred", help="Optional .npy point instance predictions")
    parser.add_argument("--doghouse-pred", help="Optional .npy point 0/1 predictions")
    parser.add_argument("--use-ground-truth", action="store_true", help="Use labels inside npz as predictions")
    parser.add_argument("--threshold", type=float, default=0.5)
    parser.add_argument("--min-component-faces", type=int, default=8)
    parser.add_argument(
        "--close-ratio",
        type=float,
        default=0.66,
        help="Morphological closing: min fraction of neighbours in one component to fill a face (<=0 disables)",
    )
    parser.add_argument("--close-iters", type=int, default=4)
    parser.add_argument(
        "--no-component-split",
        action="store_true",
        help="Do not split binary doghouse mask by CAD face adjacency",
    )
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    data = np.load(args.npz)
    face_idx = data["face_idx"]
    if args.use_ground_truth:
        semantic = data["semantic"]
        instance = data["instance_id"]
        doghouse = data["doghouse"]
    else:
        if not args.semantic_pred:
            raise ValueError("--semantic-pred is required unless --use-ground-truth")
        semantic = np.load(args.semantic_pred)
        instance = np.load(args.instance_pred) if args.instance_pred else None
        doghouse = np.load(args.doghouse_pred) if args.doghouse_pred else None

    result = vote_faces(
        face_idx,
        semantic,
        instance_pred=instance,
        doghouse_pred=doghouse,
        adjacency=None if args.no_component_split else data.get("adjacency"),
        threshold=args.threshold,
        min_component_faces=args.min_component_faces,
        close_ratio=args.close_ratio,
        close_iters=args.close_iters,
    )
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    with open(output, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=2, ensure_ascii=False)
    print(f"saved: {output}")
    print(f"faces: {len(result['face_predictions'])}")
    print(f"instances: {len(result['doghouse_instances'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
