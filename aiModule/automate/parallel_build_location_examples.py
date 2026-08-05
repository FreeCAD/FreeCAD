"""Build Location selection examples in isolated worker processes."""

from __future__ import annotations

import argparse
import collections
import json
import statistics
import subprocess
import sys
from pathlib import Path

from build_location_selection_examples import percentile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--audit", type=Path, required=True)
    parser.add_argument("--cache-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--max-pairs", type=int, default=10000)
    args = parser.parse_args()
    if args.output_dir.exists() and any(args.output_dir.iterdir()):
        raise RuntimeError(f"Output directory is not empty: {args.output_dir}")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    shard_root = args.output_dir / "worker_shards"
    shard_root.mkdir()
    audit_paths = [shard_root / f"audit-{i:02d}.jsonl" for i in range(args.workers)]
    streams = [path.open("w", encoding="utf-8") for path in audit_paths]
    try:
        with args.audit.open(encoding="utf-8") as source:
            for index, line in enumerate(source):
                streams[index % args.workers].write(line)
    finally:
        for stream in streams:
            stream.close()

    processes = []
    for worker, audit_path in enumerate(audit_paths):
        worker_output = shard_root / f"output-{worker:02d}"
        stdout = (shard_root / f"worker-{worker:02d}.stdout.log").open("w", encoding="utf-8")
        stderr = (shard_root / f"worker-{worker:02d}.stderr.log").open("w", encoding="utf-8")
        command = [
            sys.executable, "build_location_selection_examples.py", "--audit", str(audit_path),
            "--cache-dir", str(args.cache_dir), "--output-dir", str(worker_output),
            "--max-pairs", str(args.max_pairs), "--progress-every", "5000", "--workers", "1",
        ]
        processes.append((worker, subprocess.Popen(command, stdout=stdout, stderr=stderr), stdout, stderr))
    failed = []
    for worker, process, stdout, stderr in processes:
        code = process.wait(); stdout.close(); stderr.close()
        print(f"worker={worker} exit_code={code}", flush=True)
        if code: failed.append(worker)
    if failed:
        raise SystemExit(f"Location workers failed: {failed}")

    candidates, locals_per_side, positives = [], [], []
    rejection_counts = collections.Counter()
    accepted = rejected = source_rows = 0
    with (args.output_dir / "examples.jsonl").open("w", encoding="utf-8") as examples_out, \
         (args.output_dir / "rejected.jsonl").open("w", encoding="utf-8") as rejected_out:
        for worker in range(args.workers):
            worker_output = shard_root / f"output-{worker:02d}"
            worker_summary = json.loads((worker_output / "summary.json").read_text(encoding="utf-8"))
            source_rows += worker_summary["source_rows"]
            accepted += worker_summary["accepted"]
            rejected += worker_summary["rejected"]
            rejection_counts.update(worker_summary["rejection_counts"])
            with (worker_output / "examples.jsonl").open(encoding="utf-8") as source:
                for line in source:
                    examples_out.write(line)
                    row = json.loads(line)
                    candidates.append(row["candidate_pair_count"])
                    locals_per_side.extend(len(side["local_mcfs"]) for side in row["sides"])
                    positives.append(len(row["positive_pairs"]))
            rejected_out.write((worker_output / "rejected.jsonl").read_text(encoding="utf-8"))
    summary = {
        "schema_version": 1, "workers": args.workers, "source_audit": str(args.audit),
        "cache_root": str(args.cache_dir), "source_rows": source_rows, "accepted": accepted,
        "rejected": rejected, "rejection_counts": dict(sorted(rejection_counts.items())),
        "thresholds": {"max_pairs": args.max_pairs, "max_angle_deg": 1.0, "max_line_error_m": 1e-4,
                       "equivalent_origin_tolerance_m": 1e-6, "equivalent_axis_tolerance_deg": 1e-4},
        "candidate_pairs": {"median": statistics.median(candidates) if candidates else None,
                            "p90": percentile(candidates, .90), "p95": percentile(candidates, .95),
                            "p99": percentile(candidates, .99), "max": max(candidates, default=None)},
        "local_mcfs_per_side": {"median": statistics.median(locals_per_side) if locals_per_side else None,
                                "p95": percentile(locals_per_side, .95), "max": max(locals_per_side, default=None)},
        "positive_pairs": {"median": statistics.median(positives) if positives else None,
                           "p95": percentile(positives, .95), "max": max(positives, default=None)},
    }
    (args.output_dir / "summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
