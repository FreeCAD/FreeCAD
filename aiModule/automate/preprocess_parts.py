"""Build resumable compact B-Rep graph caches for AutoMate parts."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import subprocess
import sys
import time
import traceback
from pathlib import Path

import torch


ROOT = Path(__file__).resolve().parent
DATASET = ROOT / "dataset"
BUILD_DIR = ROOT / "build-ai"
DEFAULT_MATES = ROOT / "audit" / "mates_1000.jsonl"
DEFAULT_CACHE = DATASET / "cache" / "brep_graph_v1"
SCHEMA_VERSION = 1


def graph_path(cache_root, part_id):
    digest = hashlib.sha1(part_id.encode("utf-8")).hexdigest()
    return cache_root / "parts" / digest[:2] / f"{part_id}.pt"


def collect_part_ids(mates_path):
    part_ids = set()
    with mates_path.open("r", encoding="utf-8") as stream:
        for line in stream:
            row = json.loads(line)
            if row.get("status") != "ok":
                continue
            for side in row.get("sides", []):
                part_ids.add(side["part_id"])
    return sorted(part_ids)


def tensor_diagnostics(graph):
    invalid = {}
    for key, value in graph.to_dict().items():
        if torch.is_tensor(value) and (value.is_floating_point() or value.is_complex()):
            count = int((~torch.isfinite(value)).sum().item())
            if count:
                invalid[key] = count
    return invalid


def filter_invalid_mcfs(graph):
    if not hasattr(graph, "mcfs"):
        return 0
    valid = torch.isfinite(graph.mcfs).all(dim=1)
    invalid_count = int((~valid).sum().item())
    graph.mcfs = graph.mcfs[valid]
    if hasattr(graph, "mcf_refs"):
        graph.mcf_refs = graph.mcf_refs[:, valid]
    return invalid_count


def atomic_torch_save(value, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_suffix(destination.suffix + f".{os.getpid()}.tmp")
    try:
        torch.save(value, temporary)
        os.replace(temporary, destination)
    finally:
        if temporary.exists():
            temporary.unlink()


def build_options(PartOptions, PartFeatures):
    part_options = PartOptions()
    part_options.tesselate = False
    part_options.num_uv_samples = 0
    part_options.num_random_samples = 0
    part_options.num_sdf_samples = 0
    part_options.default_mcfs = True
    part_options.collect_inferences = False

    graph_options = PartFeatures()
    graph_options.mesh = False
    graph_options.mesh_to_topology = False
    graph_options.samples = False
    graph_options.face_samples = False
    graph_options.edge_samples = False
    graph_options.random_samples = False
    graph_options.uniform_samples = False
    return part_options, graph_options


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mates", type=Path, default=DEFAULT_MATES)
    parser.add_argument("--part-list", type=Path)
    parser.add_argument("--cache-dir", type=Path, default=DEFAULT_CACHE)
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument("--overwrite", action="store_true")
    parser.add_argument("--progress-every", type=int, default=25)
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--manifest-name", default="manifest.jsonl")
    parser.add_argument("--summary-name", default="summary.json")
    args = parser.parse_args()

    if args.workers > 1 and not args.part_list:
        command = [sys.executable, str(ROOT / "parallel_preprocess_parts.py"),
                   "--mates", str(args.mates), "--cache-dir", str(args.cache_dir),
                   "--workers", str(args.workers), "--progress-every", str(args.progress_every)]
        raise SystemExit(subprocess.call(command))

    sys.path.insert(0, str(BUILD_DIR))
    dll_dir = None
    if os.name == "nt":
        dll_dir = os.add_dll_directory(str(Path(sys.prefix) / "Library" / "bin"))
    from automate_cpp import Part, PartOptions
    from automate.brep import PartFeatures, part_to_graph

    part_options, graph_options = build_options(PartOptions, PartFeatures)
    if args.part_list:
        part_ids = [line.strip() for line in args.part_list.read_text(encoding="utf-8").splitlines() if line.strip()]
    else:
        part_ids = collect_part_ids(args.mates)
    if args.limit > 0:
        part_ids = part_ids[: args.limit]

    args.cache_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = args.cache_dir / args.manifest_name
    summary_path = args.cache_dir / args.summary_name
    step_dir = DATASET / "step"
    start = time.perf_counter()
    counts = {"built": 0, "skipped": 0, "failed": 0}
    total_bytes = 0

    with manifest_path.open("a", encoding="utf-8") as manifest:
        for index, part_id in enumerate(part_ids, 1):
            destination = graph_path(args.cache_dir, part_id)
            if destination.is_file() and not args.overwrite:
                counts["skipped"] += 1
                total_bytes += destination.stat().st_size
                continue

            step_path = step_dir / f"{part_id}.step"
            row = {"part_id": part_id, "step": str(step_path), "cache": str(destination)}
            part_start = time.perf_counter()
            try:
                if not step_path.is_file():
                    raise FileNotFoundError(step_path)
                part = Part(str(step_path), part_options)
                if not part.is_valid:
                    raise ValueError("automate_cpp returned an invalid part")
                graph = part_to_graph(part, graph_options)
                invalid_mcfs = filter_invalid_mcfs(graph)
                invalid_tensors = tensor_diagnostics(graph)
                invalid_non_mcf = {
                    key: count for key, count in invalid_tensors.items() if key != "mcfs"
                }
                if invalid_non_mcf:
                    raise ValueError(f"non-finite graph tensors: {invalid_non_mcf}")

                payload = {
                    "schema_version": SCHEMA_VERSION,
                    "part_id": part_id,
                    "source": {
                        "path": str(step_path),
                        "size": step_path.stat().st_size,
                        "mtime_ns": step_path.stat().st_mtime_ns,
                    },
                    "features": {
                        "mesh": False,
                        "samples": False,
                        "mcfs": True,
                    },
                    "invalid_mcf_count": invalid_mcfs,
                    "graph": graph,
                }
                atomic_torch_save(payload, destination)
                size = destination.stat().st_size
                total_bytes += size
                counts["built"] += 1
                row.update({
                    "status": "built",
                    "bytes": size,
                    "faces": int(graph.n_faces),
                    "edges": int(graph.n_edges),
                    "vertices": int(graph.n_vertices),
                    "loops": int(graph.n_loops),
                    "mcfs": int(graph.mcfs.shape[0]),
                    "invalid_mcfs": invalid_mcfs,
                })
            except Exception as exc:
                counts["failed"] += 1
                row.update({
                    "status": "error",
                    "error_type": type(exc).__name__,
                    "error": str(exc),
                    "traceback": traceback.format_exc(limit=5),
                })
            row["elapsed_seconds"] = time.perf_counter() - part_start
            manifest.write(json.dumps(row) + "\n")
            manifest.flush()

            if index % args.progress_every == 0 or index == len(part_ids):
                elapsed = time.perf_counter() - start
                print(
                    f"parts={index}/{len(part_ids)} built={counts['built']} "
                    f"skipped={counts['skipped']} failed={counts['failed']} "
                    f"elapsed={elapsed:.1f}s",
                    flush=True,
                )

    elapsed = time.perf_counter() - start
    summary = {
        "schema_version": SCHEMA_VERSION,
        "requested_parts": len(part_ids),
        **counts,
        "success_rate": (counts["built"] + counts["skipped"]) / len(part_ids) if part_ids else 0.0,
        "cache_bytes": total_bytes,
        "elapsed_seconds": elapsed,
        "mates_source": str(args.mates),
    }
    summary_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(json.dumps(summary, indent=2))
    print(f"cache={args.cache_dir}")


if __name__ == "__main__":
    main()
