"""Load a real ten-part validation assembly for incremental AutoMate testing."""

from __future__ import annotations

import json
import sys
from pathlib import Path

import FreeCAD as App
import Import


def find_root():
    script = globals().get("__file__")
    if script: return Path(script).resolve().parent
    relative = Path("aimodule/automate/freecad_load_validation_sequence.py")
    roots = [Path.cwd(), Path(sys.executable).resolve().parent]
    if App.getHomePath(): roots.append(Path(App.getHomePath()).resolve())
    for root in roots:
        for parent in (root, *root.parents):
            candidate = parent / relative
            if candidate.is_file(): return candidate.resolve().parent
    raise RuntimeError("Cannot locate aimodule/automate")


ROOT = find_root()
DEMO = ROOT / "dataset/demo/location_validation_sequence_10"


def imported_shape(doc, path):
    before = {obj.Name for obj in doc.Objects}
    Import.insert(str(path), doc.Name)
    added = [obj for obj in doc.Objects if obj.Name not in before and hasattr(obj, "Shape")]
    if not added: raise RuntimeError(f"No shape imported from {path}")
    return max(added, key=lambda obj: obj.Shape.BoundBox.DiagonalLength)


def move_minimum_to(obj, x, y, z=0.0):
    box = obj.Shape.BoundBox
    obj.Placement.Base = obj.Placement.Base + App.Vector(x-box.XMin, y-box.YMin, z-box.ZMin)


def run():
    manifest = json.loads((DEMO / "manifest.json").read_text(encoding="utf-8"))
    doc = App.newDocument("AutoMateValidationSequence10")
    objects = {}
    cursor_x = 0.0
    for part in manifest["parts"]:
        obj = imported_shape(doc, DEMO / part["file"])
        obj.Label = "Sequence Part {number:02d} ({role})".format(**part)
        move_minimum_to(obj, cursor_x, 0.0)
        cursor_x += max(obj.Shape.BoundBox.XLength, 20.0) + 40.0
        objects[part["number"]] = obj
    doc.recompute()
    App.Console.PrintMessage("AutoMate sequential validation: keep Part 01 fixed.\n")
    for step in manifest["steps"]:
        App.Console.PrintMessage(
            "Step {step}: select Part {a_num:02d}/{a_face}, Ctrl-select new Part "
            "{b_num:02d}/{b_face}; expected {mate}; candidates={count}.\n".format(
                step=step["step"], a_num=step["a"]["part_number"],
                a_face=step["a"]["selected_face_name"], b_num=step["b"]["part_number"],
                b_face=step["b"]["selected_face_name"], mate=step["mate_type"],
                count=step["candidate_pair_count"],
            )
        )
    return doc, manifest, objects


AUTOMATE_SEQUENCE_DOCUMENT, AUTOMATE_SEQUENCE_MANIFEST, AUTOMATE_SEQUENCE_OBJECTS = run()
