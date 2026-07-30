"""Run the trained AutoMate model for two selected FreeCAD objects.

Start FreeCAD from the repository root, select exactly two objects containing
Shapes, and execute:

    exec(open(r"aimodule\automate\freecad_mate_prediction.py", encoding="utf-8").read())
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import FreeCAD as App
import FreeCADGui as Gui
import Import
import Part


def _find_script_path():
    script_path = globals().get("__file__")
    if script_path:
        return Path(script_path).resolve()

    # FreeCAD's Python console does not define __file__ for exec(open(...)).
    # Search relative to the working directory and FreeCAD executable/home so
    # the same script works from the repository root and build/debug/bin.
    search_roots = [Path.cwd(), Path(sys.executable).resolve().parent]
    freecad_home = App.getHomePath()
    if freecad_home:
        search_roots.append(Path(freecad_home).resolve())

    relative_script = Path("aimodule") / "automate" / "freecad_mate_prediction.py"
    for root in search_roots:
        for parent in (root, *root.parents):
            for candidate in (parent / relative_script, parent / "freecad_mate_prediction.py"):
                if candidate.is_file():
                    return candidate.resolve()

    raise RuntimeError(
        "Cannot locate aimodule/automate/freecad_mate_prediction.py relative "
        "to the current FreeCAD process. Start FreeCAD from the repository root."
    )


SCRIPT_PATH = _find_script_path()
AUTOMATE_ROOT = str(SCRIPT_PATH.parent)
TOP_K = 5
# Experimental one-click placement: keep A fixed and transform B so the
# rank-1 mating-coordinate frames coincide. Ctrl+Z restores B afterwards.
APPLY_BEST = True
# Face normals usually oppose each other when two mating surfaces contact.
ALIGN_OPPOSITE = True
METERS_TO_FREECAD = 1000.0


def _pixi_executable():
    executable = shutil.which("pixi")
    if executable:
        return executable
    user_pixi = Path.home() / ".pixi" / "bin" / ("pixi.exe" if os.name == "nt" else "pixi")
    if user_pixi.is_file():
        return str(user_pixi)
    raise RuntimeError("Cannot find pixi on PATH or in the current user's ~/.pixi/bin directory.")


def _remove_previous(doc):
    group = doc.getObject("AutoMate_Predictions")
    if group is None:
        return
    for child in list(group.Group):
        doc.removeObject(child.Name)
    doc.removeObject(group.Name)


def _add_axis(doc, name, label, origin_values, axis_values, length, radius, color):
    origin = App.Vector(*origin_values)
    axis = App.Vector(*axis_values)
    if axis.Length < 1.0e-12:
        raise ValueError("Prediction contains a zero-length axis")
    axis.normalize()
    shape = Part.makeCompound([
        Part.makeLine(origin, origin + axis * length),
        Part.makeSphere(radius, origin),
    ])
    obj = doc.addObject("Part::Feature", name)
    obj.Label = label
    obj.Shape = shape
    obj.ViewObject.LineColor = color
    obj.ViewObject.ShapeColor = color
    obj.ViewObject.LineWidth = 3.0
    return obj


def _frame_in_freecad_units(frame):
    """Convert AutoMate SI origins (metres) to FreeCAD coordinates (mm)."""
    return {
        "index": frame["index"],
        "origin": [value * METERS_TO_FREECAD for value in frame["origin"]],
        "axis": list(frame["axis"]),
    }


def _rank1_placement(object_b, recommendation, align_opposite=True):
    """Return the global delta that maps the recommended B frame onto A."""
    frame_a = _frame_in_freecad_units(recommendation["a"])
    frame_b = _frame_in_freecad_units(recommendation["b"])
    origin_a = App.Vector(*frame_a["origin"])
    axis_a = App.Vector(*frame_a["axis"])
    origin_b = App.Vector(*frame_b["origin"])
    axis_b = App.Vector(*frame_b["axis"])
    if axis_a.Length < 1.0e-12 or axis_b.Length < 1.0e-12:
        raise ValueError("Cannot apply a prediction with a zero-length axis")
    axis_a.normalize()
    axis_b.normalize()
    target_axis = axis_a * -1.0 if align_opposite else axis_a
    rotation = App.Rotation(axis_b, target_axis)
    # p' = R * (p - origin_b) + origin_a
    translation = origin_a - rotation.multVec(origin_b)
    return App.Placement(translation, rotation)


def _transform_frame(frame, placement):
    origin = placement.multVec(App.Vector(*frame["origin"]))
    axis = placement.Rotation.multVec(App.Vector(*frame["axis"]))
    return {
        "index": frame["index"],
        "origin": [origin.x, origin.y, origin.z],
        "axis": [axis.x, axis.y, axis.z],
    }


def run(top_k=TOP_K):
    selected = [obj for obj in Gui.Selection.getSelection() if hasattr(obj, "Shape")]
    if len(selected) != 2:
        raise RuntimeError("Select exactly two objects containing Shapes, in A then B order.")
    doc = App.ActiveDocument
    if doc is None:
        raise RuntimeError("No active FreeCAD document.")

    with tempfile.TemporaryDirectory(prefix="freecad_automate_predict_") as temporary:
        step_a = os.path.join(temporary, "part_a.step")
        step_b = os.path.join(temporary, "part_b.step")
        output = os.path.join(temporary, "predictions.json")
        Import.export([selected[0]], step_a)
        Import.export([selected[1]], step_b)
        command = [
            _pixi_executable(), "run", "python", "scripts/infer_mate.py",
            step_a, step_b, "--output", output, "--top-k", str(int(top_k)),
        ]
        completed = subprocess.run(
            command,
            cwd=AUTOMATE_ROOT,
            capture_output=True,
            text=True,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
        )
        if completed.returncode:
            raise RuntimeError(
                "AutoMate inference failed:\n{}".format(
                    completed.stderr or completed.stdout
                )
            )
        with open(output, "r", encoding="utf-8") as stream:
            result = json.load(stream)

    _remove_previous(doc)
    doc.openTransaction("Apply AutoMate rank 1")
    group = doc.addObject("App::DocumentObjectGroup", "AutoMate_Predictions")
    group.Label = "AutoMate Top-{} predictions".format(top_k)
    group.addProperty("App::PropertyString", "Checkpoint", "AutoMate")
    group.addProperty("App::PropertyInteger", "CheckpointEpoch", "AutoMate")
    group.addProperty("App::PropertyBool", "AutoPlacementEnabled", "AutoMate")
    group.Checkpoint = result.get("checkpoint", "")
    group.CheckpointEpoch = int(result.get("checkpoint_epoch", -1))
    group.AutoPlacementEnabled = bool(APPLY_BEST)
    applied_delta = None
    original_placement = App.Placement(selected[1].Placement)
    if APPLY_BEST and result["recommendations"]:
        applied_delta = _rank1_placement(
            selected[1], result["recommendations"][0], ALIGN_OPPOSITE
        )
        selected[1].Placement = applied_delta.multiply(selected[1].Placement)
        moved_placement = App.Placement(selected[1].Placement)
        group.addProperty("App::PropertyLink", "MovedObject", "AutoMate")
        group.addProperty("App::PropertyPlacement", "OriginalPlacement", "AutoMate")
        group.addProperty("App::PropertyPlacement", "AppliedDelta", "AutoMate")
        group.addProperty("App::PropertyBool", "AxesOpposed", "AutoMate")
        group.MovedObject = selected[1]
        group.OriginalPlacement = original_placement
        group.AppliedDelta = applied_delta
        group.AxesOpposed = ALIGN_OPPOSITE
        App.Console.PrintMessage(
            "AutoMate Placement: B before={} delta={} after={}\n".format(
                original_placement, applied_delta, moved_placement
            )
        )
    diagonal = max(
        selected[0].Shape.BoundBox.DiagonalLength,
        selected[1].Shape.BoundBox.DiagonalLength,
        1.0,
    )
    length = diagonal * 0.10
    radius = diagonal * 0.006
    for recommendation in result["recommendations"]:
        rank = recommendation["rank"]
        score = recommendation["score"]
        probability = recommendation["probability"]
        mate_type = recommendation.get("mate_type", "UNKNOWN")
        mate_type_confidence = recommendation.get("mate_type_confidence", 0.0)
        a = _frame_in_freecad_units(recommendation["a"])
        b = _frame_in_freecad_units(recommendation["b"])
        # B has already moved, so draw all of its candidate frames in the new
        # global placement. Rank 1 will coincide with the selected A frame.
        if applied_delta is not None:
            b = _transform_frame(b, applied_delta)
        obj_a = _add_axis(
            doc, "AutoMate_R{}_A".format(rank),
            "AutoMate #{} A [{} {:.1%}]".format(
                rank, mate_type, mate_type_confidence
            ),
            a["origin"], a["axis"], length, radius, (1.0, 0.15, 0.15),
        )
        obj_b = _add_axis(
            doc, "AutoMate_R{}_B".format(rank),
            "AutoMate #{} B [{} {:.1%}]".format(
                rank, mate_type, mate_type_confidence
            ),
            b["origin"], b["axis"], length, radius, (0.15, 0.35, 1.0),
        )
        for obj, candidate_index in ((obj_a, a["index"]), (obj_b, b["index"])):
            obj.addProperty("App::PropertyInteger", "Rank", "AutoMate")
            obj.addProperty("App::PropertyFloat", "Score", "AutoMate")
            obj.addProperty("App::PropertyFloat", "Probability", "AutoMate")
            obj.addProperty("App::PropertyInteger", "CandidateIndex", "AutoMate")
            obj.addProperty("App::PropertyString", "MateType", "AutoMate")
            obj.addProperty("App::PropertyFloat", "MateTypeConfidence", "AutoMate")
            obj.addProperty("App::PropertyString", "MateTypeProbabilities", "AutoMate")
            obj.Rank = rank
            obj.Score = score
            obj.Probability = probability
            obj.CandidateIndex = candidate_index
            obj.MateType = mate_type
            obj.MateTypeConfidence = mate_type_confidence
            obj.MateTypeProbabilities = json.dumps(
                recommendation.get("mate_type_probabilities", {}),
                ensure_ascii=False,
                sort_keys=True,
            )
        # Only show the best recommendation initially; toggle the others in tree view.
        obj_a.ViewObject.Visibility = rank == 1
        obj_b.ViewObject.Visibility = rank == 1
        group.addObject(obj_a)
        group.addObject(obj_b)

    doc.recompute()
    doc.commitTransaction()
    App.Console.PrintMessage(
        "AutoMate: A MCFs={}, B MCFs={}, pairs={}, Top-K={}, inference={:.2f}s\n".format(
            result["parts"][0]["mcf_count"], result["parts"][1]["mcf_count"],
            result["pair_count"], len(result["recommendations"]),
            result["elapsed_seconds"],
        )
    )
    App.Console.PrintMessage(
        "AutoMate: red=A, blue=B; only rank 1 is visible initially. "
        "Toggle other ranks under AutoMate Predictions.\n"
    )
    if result["recommendations"]:
        best = result["recommendations"][0]
        App.Console.PrintMessage(
            "AutoMate: rank 1 predicts mate type {} ({:.1%} confidence); "
            "automatic Placement is {}.\n".format(
                best.get("mate_type", "UNKNOWN"),
                best.get("mate_type_confidence", 0.0),
                "enabled" if APPLY_BEST else "disabled",
            )
        )
    if applied_delta is not None:
        App.Console.PrintMessage(
            "AutoMate: applied rank 1 to '{}'; Ctrl+Z restores its previous Placement.\n".format(
                selected[1].Label
            )
        )
    return group, result


AUTOMATE_PREDICTION = run()
