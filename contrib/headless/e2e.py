# SPDX-License-Identifier: LGPL-2.1-or-later
"""Exercise real FreeCAD processes, editable documents & STEP roundtrips."""

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--freecad", required=True)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--step", action="append", default=[], help="Optional real STEP fixture")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    args.out.mkdir(parents=True, exist_ok=False)
    results = []

    def run(name, model, params, expected_volume=None, expected_size=None, success=True):
        command = [sys.executable, str(root / "run.py"), str(root / model),
                   "--freecad", args.freecad, "--out", str(args.out / name),
                   "--params", json.dumps(params)]
        process = subprocess.run(command, capture_output=True, text=True, timeout=150,
                                 creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0)
        report = json.loads((args.out / name / "result.json").read_text(encoding="utf-8"))
        if report["ok"] != success or (process.returncode == 0) != success:
            raise RuntimeError(f"{name}: {process.stdout}\n{process.stderr}")
        if success:
            with zipfile.ZipFile(report["files"]["document"]) as archive:
                view = ET.fromstring(archive.read("GuiDocument.xml"))
            visible = [provider.get("name") for provider in view.findall("./ViewProviderData/ViewProvider")
                       if provider.find('./Properties/Property[@name="Visibility"]/Bool').get("value") == "true"]
            if set(visible) != set(report["visible_objects"]) or not view.find("Camera").get("settings"):
                raise RuntimeError(f"{name}: missing result visibility or camera")
            for stage in ("measurement", "native_roundtrip", "step_roundtrip"):
                measured = report[stage]
                if expected_volume is not None and not math.isclose(measured["volume_mm3"], expected_volume, abs_tol=1e-5, rel_tol=1e-9):
                    raise RuntimeError(f"{name}/{stage}: incorrect volume {measured}")
                if expected_size and any(abs(a-b) > 1e-5 for a,b in zip(measured["size_mm"], expected_size)):
                    raise RuntimeError(f"{name}/{stage}: incorrect dimensions {measured}")
        results.append({"scenario": name, "passed": True, "result": report})
        return report

    plate = run("plate", "plate.py", {}, (40 * 20 - 8 * math.pi) * 4, [40, 20, 4])
    original = Path(plate["files"]["document"])
    digest = hashlib.sha256(original.read_bytes()).hexdigest()
    run("variant", "plate.py", {"length": 60, "width": 24, "thickness": 5},
        (60 * 24 - 8 * math.pi) * 5, [60, 24, 5])
    run("revised", "revise_plate.py", {"document": str(original)},
        (55 * 20 - 17 * math.pi) * 6, [55, 20, 6])
    if hashlib.sha256(original.read_bytes()).hexdigest() != digest:
        raise RuntimeError("Revision changed the original document")
    run("invalid", "plate.py", {"hole_radius": 30}, success=False)
    run("assembly", "assembly.py", {}, 24 + 2 * math.pi, [11, 7, 4])
    for index, source in enumerate(args.step):
        run(f"import-{index}", "import_step.py", {"source": str(Path(source).resolve(strict=True))})
    (args.out / "e2e.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
    print(json.dumps({"passed": len(results), "report": str(args.out / "e2e.json")}))


if __name__ == "__main__":
    main()
