"""Load and arrange the ten-part Location validation kit in FreeCAD."""

from __future__ import annotations

import json
import sys
from pathlib import Path

import FreeCAD as App
import Import


def find_root():
    script = globals().get("__file__")
    if script:
        return Path(script).resolve().parent
    relative = Path("aimodule/automate/freecad_load_validation_examples.py")
    roots = [Path.cwd(), Path(sys.executable).resolve().parent]
    if App.getHomePath():
        roots.append(Path(App.getHomePath()).resolve())
    for root in roots:
        for parent in (root, *root.parents):
            candidate = parent / relative
            if candidate.is_file():
                return candidate.resolve().parent
    raise RuntimeError("Cannot locate aimodule/automate from this FreeCAD process")


ROOT = find_root()
DEMO = ROOT / "dataset/demo/location_validation_10_parts"


def imported_shape(doc, step_path):
    before = {obj.Name for obj in doc.Objects}
    Import.insert(str(step_path), doc.Name)
    added = [obj for obj in doc.Objects if obj.Name not in before and hasattr(obj, "Shape")]
    if not added:
        raise RuntimeError(f"No shape imported from {step_path}")
    return max(added, key=lambda obj: obj.Shape.BoundBox.DiagonalLength)


def move_minimum_to(obj, x, y, z=0.0):
    box = obj.Shape.BoundBox
    obj.Placement.Base = obj.Placement.Base + App.Vector(x - box.XMin, y - box.YMin, z - box.ZMin)


def run():
    manifest = json.loads((DEMO / "manifest.json").read_text(encoding="utf-8"))
    doc = App.newDocument("AutoMateValidation10")
    cursor_y = 0.0
    for pair in manifest["pairs"]:
        group = doc.addObject("App::DocumentObjectGroup", f"ValidationPair{pair['pair']:02d}")
        group.Label = "Pair {pair}: {mate_type}, {candidate_pair_count} candidates".format(**pair)
        objects = []
        for part in pair["parts"]:
            obj = imported_shape(doc, DEMO / part["file"])
            obj.Label = "Pair {pair:02d} {side} - select {face} [{mate}]".format(
                pair=pair["pair"], side=part["side"], face=part["selected_face_name"],
                mate=pair["mate_type"],
            )
            objects.append(obj); group.addObject(obj)
        move_minimum_to(objects[0], 0.0, cursor_y)
        width_a = objects[0].Shape.BoundBox.XLength
        move_minimum_to(objects[1], width_a + 50.0, cursor_y)
        row_height = max(objects[0].Shape.BoundBox.YLength, objects[1].Shape.BoundBox.YLength, 20.0)
        cursor_y += row_height + 50.0
        App.Console.PrintMessage(
            "Pair {pair}: select A/{face_a}, Ctrl-select B/{face_b}; expected {mate}; "
            "candidate pairs={count}.\n".format(
                pair=pair["pair"], face_a=pair["parts"][0]["selected_face_name"],
                face_b=pair["parts"][1]["selected_face_name"], mate=pair["mate_type"],
                count=pair["candidate_pair_count"],
            )
        )
    doc.recompute()
    return doc, manifest


AUTOMATE_VALIDATION_DOCUMENT, AUTOMATE_VALIDATION_MANIFEST = run()
