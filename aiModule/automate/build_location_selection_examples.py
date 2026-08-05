"""Build paper-style face-conditioned MCF location selection examples."""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import math
import os
import statistics
import subprocess
import sys
from pathlib import Path

import torch


ROOT = Path(__file__).resolve().parent


class GraphCache:
    def __init__(self, cache_root, max_size=64):
        self.cache_root = Path(cache_root)
        self.max_size = max_size
        self.values = collections.OrderedDict()

    @staticmethod
    def relative_path(part_id):
        digest = hashlib.sha1(part_id.encode("utf-8")).hexdigest()
        return Path("parts") / digest[:2] / f"{part_id}.pt"

    def load(self, part_id):
        if part_id in self.values:
            self.values.move_to_end(part_id)
            return self.values[part_id]
        relative = self.relative_path(part_id)
        payload = torch.load(
            self.cache_root / relative, map_location="cpu", weights_only=False
        )
        if payload.get("part_id") != part_id:
            raise ValueError(f"Cache part ID mismatch for {part_id}")
        graph = payload["graph"]
        value = (relative, graph)
        self.values[part_id] = value
        if len(self.values) > self.max_size:
            self.values.popitem(last=False)
        return value


def relation_targets(relation, sources):
    if not sources or relation.numel() == 0:
        return set()
    source_tensor = torch.tensor(sorted(sources), dtype=relation.dtype)
    mask = torch.isin(relation[0].cpu(), source_tensor)
    return set(int(value) for value in relation[1, mask].tolist())


def flat_indices(mapping, local_indices):
    if not local_indices or mapping.numel() == 0:
        return set()
    local = torch.tensor(sorted(local_indices), dtype=mapping.dtype)
    mask = torch.isin(mapping[0].cpu(), local)
    return set(int(value) for value in mapping[1, mask].tolist())


def face_origin_neighborhood(graph, face_index):
    loops = relation_targets(graph.face_to_loop, {face_index})
    edges = relation_targets(graph.loop_to_edge, loops)
    vertices = relation_targets(graph.edge_to_vertex, edges)
    return (
        {int(graph.face_to_flat_topos[1, face_index])}
        | flat_indices(graph.loop_to_flat_topos, loops)
        | flat_indices(graph.edge_to_flat_topos, edges)
        | flat_indices(graph.vertex_to_flat_topos, vertices)
    )


def local_mcfs_for_face(graph, face_index):
    if not 0 <= face_index < int(graph.n_faces):
        raise IndexError(f"Face index out of range: {face_index}")
    allowed_origins = face_origin_neighborhood(graph, face_index)
    axis_refs = graph.mcf_refs[0].cpu()
    origin_refs = graph.mcf_refs[1].cpu()
    allowed = torch.tensor(sorted(allowed_origins), dtype=origin_refs.dtype)
    mask = (axis_refs == face_index) & torch.isin(origin_refs, allowed)
    return torch.nonzero(mask, as_tuple=False).flatten().tolist()


def equivalent_mcfs(graph, target_index, candidates, origin_tolerance, axis_tolerance_deg):
    if not candidates:
        return []
    mcfs = graph.mcfs
    target = mcfs[target_index]
    candidate_tensor = torch.tensor(candidates, dtype=torch.long)
    values = mcfs[candidate_tensor]
    target_axis = torch.nn.functional.normalize(target[:3], dim=0)
    axes = torch.nn.functional.normalize(values[:, :3], dim=1)
    cosine = (axes @ target_axis).clamp(-1.0, 1.0)
    angle = torch.rad2deg(torch.acos(cosine))
    origin_error = torch.linalg.vector_norm(values[:, 3:] - target[3:], dim=1)
    mask = (angle <= axis_tolerance_deg) & (origin_error <= origin_tolerance)
    return candidate_tensor[mask].tolist()


def percentile(values, fraction):
    if not values:
        return None
    ordered = sorted(values)
    index = min(len(ordered) - 1, round((len(ordered) - 1) * fraction))
    return ordered[index]


def atomic_write_text(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + f".{os.getpid()}.tmp")
    try:
        temporary.write_text(value, encoding="utf-8")
        os.replace(temporary, path)
    finally:
        if temporary.exists():
            temporary.unlink()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--audit", type=Path, required=True)
    parser.add_argument("--cache-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--max-pairs", type=int, default=10000)
    parser.add_argument("--max-angle-deg", type=float, default=1.0)
    parser.add_argument("--max-line-error-m", type=float, default=1.0e-4)
    parser.add_argument("--equivalent-origin-tolerance-m", type=float, default=1.0e-6)
    parser.add_argument("--equivalent-axis-tolerance-deg", type=float, default=1.0e-4)
    parser.add_argument("--progress-every", type=int, default=1000)
    parser.add_argument("--workers", type=int, default=4)
    args = parser.parse_args()
    if args.workers > 1:
        command = [sys.executable, str(ROOT / "parallel_build_location_examples.py"),
                   "--audit", str(args.audit), "--cache-dir", str(args.cache_dir),
                   "--output-dir", str(args.output_dir), "--max-pairs", str(args.max_pairs),
                   "--workers", str(args.workers)]
        raise SystemExit(subprocess.call(command))
    if args.max_pairs <= 0:
        raise ValueError("max-pairs must be positive")
    if args.output_dir.exists() and any(args.output_dir.iterdir()):
        raise RuntimeError(f"Output directory is not empty: {args.output_dir}")
    args.output_dir.mkdir(parents=True, exist_ok=True)

    graph_cache = GraphCache(args.cache_dir)
    accepted_count = 0
    rejected_count = 0
    candidate_counts = []
    local_counts = []
    positive_counts = []
    rejection_counts = collections.Counter()
    source_rows = 0

    examples_path = args.output_dir / "examples.jsonl"
    rejected_path = args.output_dir / "rejected.jsonl"
    examples_temporary = examples_path.with_suffix(f".jsonl.{os.getpid()}.tmp")
    rejected_temporary = rejected_path.with_suffix(f".jsonl.{os.getpid()}.tmp")
    with (
        args.audit.open(encoding="utf-8") as stream,
        examples_temporary.open("w", encoding="utf-8", newline="\n") as examples_stream,
        rejected_temporary.open("w", encoding="utf-8", newline="\n") as rejected_stream,
    ):
        for source_rows, line in enumerate(stream, 1):
            audit = json.loads(line)
            reason = None
            detail = None
            try:
                if audit.get("status") != "ok":
                    raise ValueError("audit_error")
                side_a, side_b = audit["sides"]
                if any(
                    float(side["angle_deg"]) > args.max_angle_deg
                    for side in (side_a, side_b)
                ):
                    raise ValueError("axis_not_recalled")
                if any(
                    float(side["line_error_m"]) > args.max_line_error_m
                    for side in (side_a, side_b)
                ):
                    raise ValueError("axis_line_not_recalled")

                relative_a, graph_a = graph_cache.load(side_a["part_id"])
                relative_b, graph_b = graph_cache.load(side_b["part_id"])
                positive_a = int(side_a["candidate_index"])
                positive_b = int(side_b["candidate_index"])
                if not 0 <= positive_a < int(graph_a.mcfs.shape[0]):
                    raise ValueError("candidate_a_out_of_range")
                if not 0 <= positive_b < int(graph_b.mcfs.shape[0]):
                    raise ValueError("candidate_b_out_of_range")

                selected_face_a = int(graph_a.mcf_refs[0, positive_a])
                selected_face_b = int(graph_b.mcf_refs[0, positive_b])
                if selected_face_a >= int(graph_a.n_faces) or selected_face_b >= int(graph_b.n_faces):
                    raise ValueError("non_face_orientation")
                local_a = local_mcfs_for_face(graph_a, selected_face_a)
                local_b = local_mcfs_for_face(graph_b, selected_face_b)
                if positive_a not in local_a or positive_b not in local_b:
                    raise ValueError("positive_not_in_face_neighborhood")
                pair_count = len(local_a) * len(local_b)
                if pair_count == 0:
                    raise ValueError("empty_candidate_pairs")
                if pair_count > args.max_pairs:
                    raise ValueError("candidate_pairs_over_limit")

                equivalent_a = equivalent_mcfs(
                    graph_a,
                    positive_a,
                    local_a,
                    args.equivalent_origin_tolerance_m,
                    args.equivalent_axis_tolerance_deg,
                )
                equivalent_b = equivalent_mcfs(
                    graph_b,
                    positive_b,
                    local_b,
                    args.equivalent_origin_tolerance_m,
                    args.equivalent_axis_tolerance_deg,
                )
                positive_pairs = [[a, b] for a in equivalent_a for b in equivalent_b]
                if not positive_pairs:
                    raise ValueError("no_equivalent_positive_pairs")

                sample_key = ":".join(
                    (
                        str(audit["assembly_id"]),
                        str(audit.get("mate_id")),
                        side_a["part_id"],
                        side_b["part_id"],
                        str(selected_face_a),
                        str(selected_face_b),
                    )
                )
                row = {
                    "schema_version": 1,
                    "sample_id": hashlib.sha1(sample_key.encode("utf-8")).hexdigest(),
                    "assembly_id": audit["assembly_id"],
                    "mate_id": audit.get("mate_id"),
                    "mate_type": audit.get("mate_type"),
                    "sides": [
                        {
                            "part_id": side_a["part_id"],
                            "cache": str(relative_a),
                            "selected_face": selected_face_a,
                            "local_mcfs": local_a,
                        },
                        {
                            "part_id": side_b["part_id"],
                            "cache": str(relative_b),
                            "selected_face": selected_face_b,
                            "local_mcfs": local_b,
                        },
                    ],
                    "positive_pairs": positive_pairs,
                    "candidate_pair_count": pair_count,
                }
                examples_stream.write(json.dumps(row) + "\n")
                accepted_count += 1
                candidate_counts.append(pair_count)
                local_counts.extend((len(local_a), len(local_b)))
                positive_counts.append(len(positive_pairs))
            except Exception as exc:
                reason = str(exc) if isinstance(exc, ValueError) else type(exc).__name__
                detail = str(exc)

            if reason is not None:
                rejection_counts[reason] += 1
                rejected_stream.write(
                    json.dumps({
                        "assembly_id": audit.get("assembly_id"),
                        "mate_id": audit.get("mate_id"),
                        "mate_type": audit.get("mate_type"),
                        "reason": reason,
                        "detail": detail,
                    }) + "\n"
                )
                rejected_count += 1
            if args.progress_every and source_rows % args.progress_every == 0:
                print(
                    f"source={source_rows} accepted={accepted_count} "
                    f"rejected={rejected_count}",
                    flush=True,
                )

    summary = {
        "schema_version": 1,
        "source_audit": str(args.audit),
        "cache_root": str(args.cache_dir),
        "source_rows": source_rows,
        "accepted": accepted_count,
        "rejected": rejected_count,
        "rejection_counts": dict(sorted(rejection_counts.items())),
        "thresholds": {
            "max_pairs": args.max_pairs,
            "max_angle_deg": args.max_angle_deg,
            "max_line_error_m": args.max_line_error_m,
            "equivalent_origin_tolerance_m": args.equivalent_origin_tolerance_m,
            "equivalent_axis_tolerance_deg": args.equivalent_axis_tolerance_deg,
        },
        "candidate_pairs": {
            "median": statistics.median(candidate_counts) if candidate_counts else None,
            "p90": percentile(candidate_counts, 0.90),
            "p95": percentile(candidate_counts, 0.95),
            "p99": percentile(candidate_counts, 0.99),
            "max": max(candidate_counts, default=None),
        },
        "local_mcfs_per_side": {
            "median": statistics.median(local_counts) if local_counts else None,
            "p95": percentile(local_counts, 0.95),
            "max": max(local_counts, default=None),
        },
        "positive_pairs": {
            "median": statistics.median(positive_counts) if positive_counts else None,
            "p95": percentile(positive_counts, 0.95),
            "max": max(positive_counts, default=None),
        },
    }
    os.replace(examples_temporary, examples_path)
    os.replace(rejected_temporary, rejected_path)
    atomic_write_text(
        args.output_dir / "summary.json",
        json.dumps(summary, ensure_ascii=False, indent=2),
    )
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
