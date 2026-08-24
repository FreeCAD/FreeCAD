from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    steps = sorted((root / "step - 副本2").glob("*.step"))
    output_dir = root / "outputs" / "freecad_plugin" / "batch_auto_infer"
    output_dir.mkdir(parents=True, exist_ok=True)
    script = root / "doghouse_ai" / "recommend_and_assemble.py"
    results = []
    for step in steps:
        target = output_dir / f"{step.stem}_recommend.json"
        cmd = [
            sys.executable,
            str(script),
            "--auto-doghouse",
            "--step",
            str(step),
            "--recommend-output",
            str(target),
            "--all-doghouses-same-clip",
            "--no-assemble",
        ]
        print(f"RUN {step.name}", flush=True)
        proc = subprocess.run(cmd, cwd=str(root), text=True, capture_output=True, check=False)
        ok = proc.returncode == 0 and target.exists()
        summary = {"step": step.name, "ok": ok, "returncode": proc.returncode}
        if ok:
            data = json.loads(target.read_text(encoding="utf-8"))
            summary.update(
                {
                    "hole_count": data.get("hole_count"),
                    "selected_clip": data.get("selected_clip"),
                    "mode": data.get("mode"),
                }
            )
        else:
            summary["error"] = (proc.stderr or proc.stdout)[-1600:]
        results.append(summary)
        print(summary, flush=True)
    summary_path = root / "outputs" / "freecad_plugin" / "batch_auto_infer_summary.json"
    summary_path.write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"summary: {summary_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
