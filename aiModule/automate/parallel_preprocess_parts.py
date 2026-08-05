"""Build B-Rep graph caches in isolated worker processes on Windows."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import time
from pathlib import Path

from preprocess_parts import collect_part_ids


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mates", type=Path, required=True)
    parser.add_argument("--cache-dir", type=Path, required=True)
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--progress-every", type=int, default=100)
    args = parser.parse_args()
    if args.workers < 1:
        raise ValueError("workers must be positive")

    part_ids = collect_part_ids(args.mates)
    args.cache_dir.mkdir(parents=True, exist_ok=True)
    shard_dir = args.cache_dir / "worker_shards"
    shard_dir.mkdir(exist_ok=True)
    shards = [[] for _ in range(min(args.workers, max(1, len(part_ids))))]
    for index, part_id in enumerate(part_ids):
        shards[index % len(shards)].append(part_id)

    processes = []
    start = time.perf_counter()
    for worker, values in enumerate(shards):
        part_list = shard_dir / f"parts-{worker:02d}.txt"
        part_list.write_text("\n".join(values) + "\n", encoding="utf-8")
        stdout = (shard_dir / f"worker-{worker:02d}.stdout.log").open("w", encoding="utf-8")
        stderr = (shard_dir / f"worker-{worker:02d}.stderr.log").open("w", encoding="utf-8")
        command = [
            sys.executable, "preprocess_parts.py", "--part-list", str(part_list),
            "--mates", str(args.mates), "--cache-dir", str(args.cache_dir),
            "--manifest-name", f"manifest-{worker:02d}.jsonl",
            "--summary-name", f"summary-{worker:02d}.json",
            "--progress-every", str(args.progress_every), "--workers", "1",
        ]
        processes.append((worker, subprocess.Popen(command, stdout=stdout, stderr=stderr), stdout, stderr))

    failed = []
    for worker, process, stdout, stderr in processes:
        return_code = process.wait()
        stdout.close()
        stderr.close()
        print(f"worker={worker} exit_code={return_code}", flush=True)
        if return_code:
            failed.append(worker)
    if failed:
        raise SystemExit(f"cache workers failed: {failed}")

    totals = {"built": 0, "skipped": 0, "failed": 0, "cache_bytes": 0}
    manifest = args.cache_dir / "manifest.jsonl"
    with manifest.open("w", encoding="utf-8") as output:
        for worker in range(len(shards)):
            worker_manifest = args.cache_dir / f"manifest-{worker:02d}.jsonl"
            output.write(worker_manifest.read_text(encoding="utf-8"))
            summary = json.loads((args.cache_dir / f"summary-{worker:02d}.json").read_text(encoding="utf-8"))
            for key in totals:
                totals[key] += summary[key]
    elapsed = time.perf_counter() - start
    summary = {
        "schema_version": 1, "workers": len(shards), "requested_parts": len(part_ids),
        **totals, "success_rate": (totals["built"] + totals["skipped"]) / len(part_ids) if part_ids else 0.0,
        "elapsed_seconds": elapsed, "mates_source": str(args.mates),
    }
    (args.cache_dir / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
