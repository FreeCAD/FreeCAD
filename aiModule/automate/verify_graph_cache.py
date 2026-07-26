"""Verify AutoMate B-Rep graph cache integrity."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import torch


ROOT = Path(__file__).resolve().parent
DEFAULT_CACHE = ROOT / "dataset" / "cache" / "brep_graph_v1"


def expected_part_ids(mates_path):
    if mates_path is None:
        return set()
    result = set()
    with mates_path.open(encoding="utf-8") as stream:
        for line in stream:
            row = json.loads(line)
            if row.get("status") == "ok":
                result.update(side["part_id"] for side in row["sides"])
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cache-dir", type=Path, default=DEFAULT_CACHE)
    parser.add_argument("--mates", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    files = sorted((args.cache_dir / "parts").rglob("*.pt"))
    expected = expected_part_ids(args.mates)
    seen = set()
    failures = []
    totals = {"faces": 0, "edges": 0, "vertices": 0, "loops": 0, "mcfs": 0}
    invalid_mcfs = 0
    total_bytes = 0

    for path in files:
        try:
            payload = torch.load(path, map_location="cpu", weights_only=False)
            if payload["schema_version"] != 1:
                raise ValueError(f"unsupported schema {payload['schema_version']}")
            part_id = payload["part_id"]
            if part_id in seen:
                raise ValueError(f"duplicate part_id {part_id}")
            seen.add(part_id)
            graph = payload["graph"]
            required = (
                "faces", "loops", "edges", "vertices", "flat_topos",
                "part_feat", "mcfs", "mcf_refs",
            )
            missing = [name for name in required if not hasattr(graph, name)]
            if missing:
                raise ValueError(f"missing graph attributes: {missing}")
            for key, value in graph.to_dict().items():
                if torch.is_tensor(value) and (value.is_floating_point() or value.is_complex()):
                    if not torch.isfinite(value).all():
                        raise ValueError(f"non-finite tensor {key}")
            if graph.mcf_refs.shape[1] != graph.mcfs.shape[0]:
                raise ValueError("mcf_refs and mcfs are not aligned")
            if graph.mcfs.ndim != 2 or graph.mcfs.shape[1] != 6:
                raise ValueError(f"unexpected mcfs shape {tuple(graph.mcfs.shape)}")
            if graph.mcf_refs.ndim != 2 or graph.mcf_refs.shape[0] != 3:
                raise ValueError(f"unexpected mcf_refs shape {tuple(graph.mcf_refs.shape)}")
            if graph.mcf_refs.numel():
                topology_references = graph.mcf_refs[:2]
                if int(topology_references.min()) < 0:
                    raise ValueError("negative MCF topology reference")
                if int(topology_references.max()) >= graph.flat_topos.shape[0]:
                    raise ValueError("MCF topology reference out of range")
                if int(graph.mcf_refs[2].min()) < 0:
                    raise ValueError("negative MCF inference type")
            totals["faces"] += int(graph.n_faces)
            totals["edges"] += int(graph.n_edges)
            totals["vertices"] += int(graph.n_vertices)
            totals["loops"] += int(graph.n_loops)
            totals["mcfs"] += int(graph.mcfs.shape[0])
            invalid_mcfs += int(payload.get("invalid_mcf_count", 0))
            total_bytes += path.stat().st_size
        except Exception as exc:
            failures.append({"path": str(path), "error": str(exc)})

    missing_expected = sorted(expected - seen)
    unexpected = sorted(seen - expected) if expected else []
    if missing_expected:
        failures.append({"path": str(args.mates), "error": f"missing {len(missing_expected)} expected parts"})
    result = {
        "cache_dir": str(args.cache_dir),
        "files": len(files),
        "unique_parts": len(seen),
        "failures": len(failures),
        "expected_parts": len(expected) if args.mates else None,
        "missing_expected_parts": len(missing_expected),
        "unexpected_parts": len(unexpected),
        "total_bytes": total_bytes,
        "invalid_mcfs_filtered": invalid_mcfs,
        "totals": totals,
        "failure_details": failures[:20],
    }
    output = args.output or args.cache_dir / "verification.json"
    output.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    print(f"verification={output}")
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
