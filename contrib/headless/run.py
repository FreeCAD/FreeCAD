# SPDX-License-Identifier: LGPL-2.1-or-later
"""Run a trusted Python model in an isolated FreeCAD command-line process."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("model", type=Path, help="Python file exposing build(params)")
    parser.add_argument("--freecad", required=True, type=Path, help="FreeCADCmd executable")
    parser.add_argument("--out", required=True, type=Path, help="New output directory")
    parser.add_argument("--params", default="{}", help="JSON parameter object")
    parser.add_argument("--timeout", type=float, default=120)
    args = parser.parse_args()
    model = args.model.resolve(strict=True)
    executable = args.freecad.resolve(strict=True)
    params = json.loads(args.params)
    if not isinstance(params, dict) or not 0 < args.timeout <= 3600:
        parser.error("params must be an object; timeout must be in (0, 3600]")
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    request = {"model": str(model), "params": params, "out": str(out)}
    request_path = out / "request.json"
    request_path.write_text(json.dumps(request), encoding="utf-8")
    env = os.environ.copy()
    env["FREECAD_MODEL_REQUEST"] = str(request_path)
    worker = Path(__file__).with_name("worker.py")
    failure = None
    with (out / "freecad.log").open("w", encoding="utf-8") as log:
        try:
            process = subprocess.run(
                [str(executable), str(worker)],
                env=env,
                stdin=subprocess.DEVNULL,
                stdout=log,
                stderr=subprocess.STDOUT,
                timeout=args.timeout,
                creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0,
                check=False,
            )
            if process.returncode:
                failure = f"FreeCAD exited with status {process.returncode}"
        except (subprocess.TimeoutExpired, OSError) as exc:
            failure = str(exc)
    report_path = out / "result.json"
    if failure or not report_path.exists():
        report = {"ok": False, "error": failure or "FreeCAD produced no result", "out": str(out)}
        report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    else:
        report = json.loads(report_path.read_text(encoding="utf-8"))
    print(json.dumps(report, indent=2))
    return 0 if report.get("ok") else 1


if __name__ == "__main__":
    sys.exit(main())
