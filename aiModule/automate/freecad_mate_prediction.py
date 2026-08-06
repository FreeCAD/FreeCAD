"""Run the trained AutoMate model for two selected FreeCAD objects.

Start FreeCAD from the repository root, select one mating face on each of two
objects (A then B), and execute:

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
# Show all Location Top-K placements interactively before committing one.
SHOW_TOPK_PREVIEW = True
# For two selected cylindrical faces, snap the nearest pair of axial face
# boundaries after MCF alignment. This resolves insertion depth, not roll.
AXIAL_CONTACT_CORRECTION = True
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


def _selected_face_signature(face):
    center = face.CenterOfMass
    return json.dumps({
        "area_m2": float(face.Area) / (METERS_TO_FREECAD ** 2),
        "center_m": [center.x / METERS_TO_FREECAD, center.y / METERS_TO_FREECAD,
                     center.z / METERS_TO_FREECAD],
    }, separators=(",", ":"))


def _geometry_object(obj):
    """Return the untransformed source object behind an App::Link."""
    try:
        linked = obj.getLinkedObject(True)
        if linked is not None and hasattr(linked, "Shape"):
            return linked
    except Exception:
        pass
    return obj


def _geometry_shape(obj):
    return _geometry_object(obj).Shape


def _global_placement(obj):
    try:
        return App.Placement(obj.getGlobalPlacement())
    except Exception:
        return App.Placement(obj.Placement)


def _two_selected_faces():
    entries = [entry for entry in Gui.Selection.getSelectionEx()
               if hasattr(entry.Object, "Shape")]
    if len(entries) != 2:
        raise RuntimeError("Select exactly one face on each of two objects, in A then B order.")
    selected = []
    for side, entry in zip(("A", "B"), entries):
        faces = [(name, shape) for name, shape in zip(entry.SubElementNames, entry.SubObjects)
                 if name.startswith("Face")]
        if len(faces) != 1 or len(entry.SubElementNames) != 1:
            raise RuntimeError("Side {} must have exactly one selected FaceN subelement.".format(side))
        name, _selected_subobject = faces[0]
        try:
            index = int(name[4:]) - 1
        except ValueError as exc:
            raise RuntimeError("Invalid selected face name: {}".format(name)) from exc
        # SelectionEx.SubObjects may be returned in document/global coordinates
        # for links and assembly objects.  Inference is deliberately performed
        # on obj.Shape in its local coordinates, so the signature must use the
        # same local shape as the exported STEP file.
        local_face = _geometry_shape(entry.Object).getElement(name)
        selected.append((entry.Object, name, index, _selected_face_signature(local_face)))
    if selected[0][0] is selected[1][0]:
        raise RuntimeError("Select faces on two different objects.")
    return selected


def _rank1_placement(object_a, recommendation, align_opposite=True):
    """Return B's final Placement from two part-local recommended frames.

    Both model frames are generated from local ``obj.Shape`` geometry.  Only
    A's frame is converted to document coordinates here.  This avoids applying
    an existing object Placement twice when STEP export bakes it into geometry.
    """
    frame_a = _frame_in_freecad_units(recommendation["a"])
    frame_b = _frame_in_freecad_units(recommendation["b"])
    placement_a = _global_placement(object_a)
    origin_a = placement_a.multVec(App.Vector(*frame_a["origin"]))
    axis_a = placement_a.Rotation.multVec(App.Vector(*frame_a["axis"]))
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


def _selected_local_face(obj, face_name):
    try:
        return _geometry_shape(obj).getElement(face_name)
    except Exception as exc:
        raise RuntimeError("Cannot resolve {} on '{}'".format(face_name, obj.Label)) from exc


def _is_cylindrical_face(obj, face_name):
    surface_name = type(_selected_local_face(obj, face_name).Surface).__name__.lower()
    return "cylinder" in surface_name


def _axial_face_bounds(obj, face_name, placement, axis):
    face = _selected_local_face(obj, face_name)
    values = []
    for vertex in face.Vertexes:
        point = placement.multVec(vertex.Point)
        values.append(point.dot(axis))
    if not values:
        point = placement.multVec(face.CenterOfMass)
        values.append(point.dot(axis))
    return min(values), max(values)


def _candidate_placement(object_a, object_b, face_name_a, face_name_b,
                         original_b, recommendation, align_opposite=ALIGN_OPPOSITE):
    desired_global = _rank1_placement(object_a, recommendation, align_opposite)
    current_global_b = _global_placement(object_b)
    parent_global_b = current_global_b.multiply(original_b.inverse())
    placement = parent_global_b.inverse().multiply(desired_global)
    delta = placement.multiply(original_b.inverse())
    correction = 0.0
    correction_enabled = (
        AXIAL_CONTACT_CORRECTION
        and _is_cylindrical_face(object_a, face_name_a)
        and _is_cylindrical_face(object_b, face_name_b)
    )
    if correction_enabled:
        frame_a = _frame_in_freecad_units(recommendation["a"])
        placement_a = _global_placement(object_a)
        axis = placement_a.Rotation.multVec(App.Vector(*frame_a["axis"]))
        if axis.Length < 1.0e-12:
            raise ValueError("Cannot correct a zero-length mating axis")
        axis.normalize()
        bounds_a = _axial_face_bounds(
            object_a, face_name_a, placement_a, axis
        )
        bounds_b = _axial_face_bounds(object_b, face_name_b, desired_global, axis)
        corrections = [a_value - b_value for a_value in bounds_a for b_value in bounds_b]
        correction = min(corrections, key=abs)
        axial_delta = App.Placement(axis * correction, App.Rotation())
        desired_global = axial_delta.multiply(desired_global)
        placement = parent_global_b.inverse().multiply(desired_global)
        delta = placement.multiply(original_b.inverse())

    # One MCF contains only an origin and one axis, so rotation around that
    # axis is not predicted. Provide the other common discrete orientation by
    # rotating 180 degrees around A's mating axis through the mating origin.
    frame_a = _frame_in_freecad_units(recommendation["a"])
    placement_a = _global_placement(object_a)
    roll_origin = placement_a.multVec(App.Vector(*frame_a["origin"]))
    roll_axis = placement_a.Rotation.multVec(App.Vector(*frame_a["axis"]))
    roll_axis.normalize()
    roll_rotation = App.Rotation(roll_axis, 180.0)
    roll_translation = roll_origin - roll_rotation.multVec(roll_origin)
    roll_delta_global = App.Placement(roll_translation, roll_rotation)
    desired_global_180 = roll_delta_global.multiply(desired_global)
    placement_180 = parent_global_b.inverse().multiply(desired_global_180)
    delta_180 = placement_180.multiply(original_b.inverse())
    return {
        "placement": placement,
        "delta": delta,
        "placement_180": placement_180,
        "delta_180": delta_180,
        "axial_correction_mm": correction,
        "axial_correction_enabled": correction_enabled,
    }


def _refresh_object_document(obj):
    """Recompute the document that owns obj, which may not be ActiveDocument."""
    owner = getattr(obj, "Document", None)
    if owner is not None:
        owner.recompute()
    try:
        Gui.updateGui()
    except Exception:
        pass


def _global_bbox_stats(obj):
    box = _geometry_shape(obj).BoundBox
    placement = _global_placement(obj)
    points = [
        placement.multVec(App.Vector(x, y, z))
        for x in (box.XMin, box.XMax)
        for y in (box.YMin, box.YMax)
        for z in (box.ZMin, box.ZMax)
    ]
    minimum = [min(getattr(point, axis) for point in points) for axis in ("x", "y", "z")]
    maximum = [max(getattr(point, axis) for point in points) for axis in ("x", "y", "z")]
    center = [(low + high) * 0.5 for low, high in zip(minimum, maximum)]
    size = [high - low for low, high in zip(minimum, maximum)]
    return center, size


def _preview_topk(object_b, original_placement, recommendations, placements):
    if not SHOW_TOPK_PREVIEW or not recommendations:
        return 0, False, False
    try:
        try:
            from PySide import QtCore, QtWidgets
        except ImportError:
            try:
                from PySide2 import QtCore, QtWidgets
            except ImportError:
                from PySide import QtCore, QtGui as QtWidgets
    except ImportError:
        App.Console.PrintWarning("AutoMate: Qt unavailable; applying Rank 1 without preview.\n")
        return 0, False, False

    dialog = QtWidgets.QDialog(Gui.getMainWindow())
    dialog.setWindowTitle("AutoMate Top-K Placement Preview")
    dialog.setModal(False)
    dialog.setWindowModality(QtCore.Qt.NonModal)
    layout = QtWidgets.QVBoxLayout(dialog)
    layout.addWidget(QtWidgets.QLabel(
        "Choose a Location candidate. Switching the row previews object B; "
        "Cancel restores its original Placement."
    ))
    combo = QtWidgets.QComboBox(dialog)
    for recommendation, candidate in zip(recommendations, placements):
        combo.addItem(
            "Rank {rank}: {mate_type} ({confidence:.1%}), Location {probability:.1%}, "
            "axial snap {correction:+.3f} mm".format(
                rank=recommendation["rank"],
                mate_type=recommendation.get("mate_type", "UNKNOWN"),
                confidence=recommendation.get("mate_type_confidence", 0.0),
                probability=recommendation.get("probability", 0.0),
                correction=candidate["axial_correction_mm"],
            )
        )
    layout.addWidget(combo)
    flip_orientation = QtWidgets.QCheckBox(
        "Rotate object B 180 degrees around the mating axis"
    )
    layout.addWidget(flip_orientation)
    same_axis = QtWidgets.QCheckBox(
        "Align MCF axes in the same direction (default: opposed)"
    )
    layout.addWidget(same_axis)
    note = QtWidgets.QLabel(
        "Cylindrical correction aligns the nearest axial boundaries. "
        "Rotation about the cylinder axis remains unconstrained."
    )
    note.setWordWrap(True)
    layout.addWidget(note)
    buttons = QtWidgets.QDialogButtonBox(
        QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel,
        parent=dialog,
    )
    layout.addWidget(buttons)

    def preview(index):
        candidate = placements[index]["same"] if same_axis.isChecked() else placements[index]
        key = "placement_180" if flip_orientation.isChecked() else "placement"
        object_b.Placement = App.Placement(candidate[key])
        _refresh_object_document(object_b)
        try:
            Gui.activeDocument().activeView().redraw()
        except Exception:
            pass

    combo.currentIndexChanged.connect(preview)
    flip_orientation.stateChanged.connect(lambda _state: preview(combo.currentIndex()))
    same_axis.stateChanged.connect(lambda _state: preview(combo.currentIndex()))
    buttons.accepted.connect(dialog.accept)
    buttons.rejected.connect(dialog.reject)
    preview(0)
    # QDialog.exec_() forces application modality in FreeCAD. A local event
    # loop keeps this script waiting for the final choice while the modeless
    # dialog leaves the main window and 3D view fully interactive.
    event_loop = QtCore.QEventLoop()
    dialog.finished.connect(lambda _result: event_loop.quit())
    dialog.show()
    dialog.raise_()
    if hasattr(event_loop, "exec"):
        event_loop.exec()
    else:
        event_loop.exec_()
    accepted = dialog.result() == QtWidgets.QDialog.Accepted
    selected_index = combo.currentIndex()
    object_b.Placement = App.Placement(original_placement)
    _refresh_object_document(object_b)
    if not accepted:
        return None
    return (
        selected_index,
        bool(flip_orientation.isChecked()),
        bool(same_axis.isChecked()),
    )


def _transform_frame(frame, placement):
    origin = placement.multVec(App.Vector(*frame["origin"]))
    axis = placement.Rotation.multVec(App.Vector(*frame["axis"]))
    return {
        "index": frame["index"],
        "origin": [origin.x, origin.y, origin.z],
        "axis": [axis.x, axis.y, axis.z],
    }


def run(top_k=TOP_K):
    face_selections = _two_selected_faces()
    selected = [item[0] for item in face_selections]
    doc = App.ActiveDocument
    if doc is None:
        raise RuntimeError("No active FreeCAD document.")
    App.Console.PrintMessage(
        "AutoMate objects: A='{}' document='{}'; B='{}' document='{}'; "
        "active document='{}'.\n".format(
            selected[0].Label, selected[0].Document.Name,
            selected[1].Label, selected[1].Document.Name, doc.Name,
        )
    )
    print(
        "AutoMate objects: A='{}' document='{}'; B='{}' document='{}'; "
        "active document='{}'.".format(
            selected[0].Label, selected[0].Document.Name,
            selected[1].Label, selected[1].Document.Name, doc.Name,
        )
    )

    with tempfile.TemporaryDirectory(prefix="freecad_automate_predict_") as temporary:
        step_a = os.path.join(temporary, "part_a.step")
        step_b = os.path.join(temporary, "part_b.step")
        output = os.path.join(temporary, "predictions.json")
        # Export local TopoShapes, never document-placed objects.  The model
        # consequently has one stable coordinate convention for normal parts,
        # App::Links, and objects imported with large global CAD offsets.
        _geometry_shape(selected[0]).exportStep(step_a)
        _geometry_shape(selected[1]).exportStep(step_b)
        command = [
            _pixi_executable(), "run", "python", "scripts/infer_selected_faces.py",
            step_a, step_b,
            "--face-a", str(face_selections[0][2]),
            "--face-b", str(face_selections[1][2]),
            "--face-signature-a", face_selections[0][3],
            "--face-signature-b", face_selections[1][3],
            "--output", output, "--top-k", str(int(top_k)),
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

    original_placement = App.Placement(selected[1].Placement)
    candidate_placements = []
    for recommendation in result["recommendations"]:
        opposed_candidate = _candidate_placement(
            selected[0], selected[1], face_selections[0][1], face_selections[1][1],
            original_placement, recommendation, align_opposite=True,
        )
        opposed_candidate["same"] = _candidate_placement(
            selected[0], selected[1], face_selections[0][1], face_selections[1][1],
            original_placement, recommendation, align_opposite=False,
        )
        candidate_placements.append(opposed_candidate)
    selected_recommendation_index = None
    selected_orientation_180 = False
    selected_same_axis = False
    if APPLY_BEST and result["recommendations"]:
        preview_selection = _preview_topk(
            selected[1], original_placement, result["recommendations"], candidate_placements
        )
        if preview_selection is None:
            App.Console.PrintMessage("AutoMate: placement preview cancelled; no document changes.\n")
            return None, result
        (selected_recommendation_index,
         selected_orientation_180,
         selected_same_axis) = preview_selection

    _remove_previous(doc)
    doc.openTransaction("Apply AutoMate selected placement")
    group = doc.addObject("App::DocumentObjectGroup", "AutoMate_Predictions")
    group.Label = "AutoMate Top-{} predictions".format(top_k)
    group.addProperty("App::PropertyString", "LocationCheckpoint", "AutoMate")
    group.addProperty("App::PropertyInteger", "LocationCheckpointEpoch", "AutoMate")
    group.addProperty("App::PropertyString", "MateTypeCheckpoint", "AutoMate")
    group.addProperty("App::PropertyInteger", "MateTypeCheckpointEpoch", "AutoMate")
    group.addProperty("App::PropertyString", "SelectedFaceA", "AutoMate")
    group.addProperty("App::PropertyString", "SelectedFaceB", "AutoMate")
    group.addProperty("App::PropertyInteger", "ResolvedFaceIndexA", "AutoMate")
    group.addProperty("App::PropertyInteger", "ResolvedFaceIndexB", "AutoMate")
    group.addProperty("App::PropertyInteger", "AppliedRank", "AutoMate")
    group.addProperty("App::PropertyFloat", "AxialCorrectionMm", "AutoMate")
    group.addProperty("App::PropertyBool", "AxialCorrectionEnabled", "AutoMate")
    group.addProperty("App::PropertyFloat", "RollCorrectionDeg", "AutoMate")
    group.addProperty("App::PropertyBool", "AutoPlacementEnabled", "AutoMate")
    group.LocationCheckpoint = result.get("location_checkpoint", "")
    group.LocationCheckpointEpoch = int(result.get("location_checkpoint_epoch", -1))
    group.MateTypeCheckpoint = result.get("mate_type_checkpoint", "")
    group.MateTypeCheckpointEpoch = int(result.get("mate_type_checkpoint_epoch", -1))
    group.SelectedFaceA = face_selections[0][1]
    group.SelectedFaceB = face_selections[1][1]
    group.ResolvedFaceIndexA = int(result["parts"][0]["selected_face"])
    group.ResolvedFaceIndexB = int(result["parts"][1]["selected_face"])
    group.AppliedRank = 0
    group.AxialCorrectionMm = 0.0
    group.AxialCorrectionEnabled = False
    group.RollCorrectionDeg = 0.0
    group.AutoPlacementEnabled = bool(APPLY_BEST)
    applied_delta = None
    if selected_recommendation_index is not None:
        candidate = candidate_placements[selected_recommendation_index]
        if selected_same_axis:
            candidate = candidate["same"]
        chosen = dict(candidate)
        if selected_orientation_180:
            chosen["placement"] = chosen["placement_180"]
            chosen["delta"] = chosen["delta_180"]
        chosen_recommendation = result["recommendations"][selected_recommendation_index]
        applied_delta = chosen["delta"]
        selected[1].Placement = App.Placement(chosen["placement"])
        _refresh_object_document(selected[1])
        moved_placement = App.Placement(selected[1].Placement)
        link_type = (
            "App::PropertyLink" if selected[1].Document is doc
            else "App::PropertyXLink"
        )
        group.addProperty(link_type, "MovedObject", "AutoMate")
        group.addProperty("App::PropertyPlacement", "OriginalPlacement", "AutoMate")
        group.addProperty("App::PropertyPlacement", "AppliedDelta", "AutoMate")
        group.addProperty("App::PropertyBool", "AxesOpposed", "AutoMate")
        group.MovedObject = selected[1]
        group.OriginalPlacement = original_placement
        group.AppliedDelta = applied_delta
        group.AxesOpposed = not selected_same_axis
        group.AppliedRank = int(chosen_recommendation["rank"])
        group.AxialCorrectionMm = float(chosen["axial_correction_mm"])
        group.AxialCorrectionEnabled = bool(chosen["axial_correction_enabled"])
        group.RollCorrectionDeg = 180.0 if selected_orientation_180 else 0.0
        App.Console.PrintMessage(
            "AutoMate Placement: selected Rank {}, axial correction={:+.3f} mm, "
            "axis relation={}, roll correction={:.0f} deg; "
            "B before={} delta={} after={}\n".format(
                chosen_recommendation["rank"], chosen["axial_correction_mm"],
                "same" if selected_same_axis else "opposed",
                group.RollCorrectionDeg,
                original_placement, applied_delta, moved_placement
            )
        )
    diagonal = max(
        _geometry_shape(selected[0]).BoundBox.DiagonalLength,
        _geometry_shape(selected[1]).BoundBox.DiagonalLength,
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
        a = _transform_frame(
            _frame_in_freecad_units(recommendation["a"]), _global_placement(selected[0])
        )
        placed_center, placed_size = _global_bbox_stats(selected[1])
        App.Console.PrintMessage(
            "AutoMate B global bbox center=({:.3f}, {:.3f}, {:.3f}) mm, "
            "size=({:.3f}, {:.3f}, {:.3f}) mm.\n".format(
                *placed_center, *placed_size,
            )
        )
        placed_center, placed_size = _global_bbox_stats(selected[1])
        print(
            "AutoMate Placement: rank={} B-local={} B-global={} bbox-center={} "
            "bbox-size={}".format(
                chosen_recommendation["rank"], moved_placement,
                _global_placement(selected[1]),
                tuple(round(value, 3) for value in placed_center),
                tuple(round(value, 3) for value in placed_size),
            )
        )
        b = _transform_frame(
            _frame_in_freecad_units(recommendation["b"]), _global_placement(selected[1])
        )
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
        visible_rank = (
            result["recommendations"][selected_recommendation_index]["rank"]
            if selected_recommendation_index is not None else 1
        )
        obj_a.ViewObject.Visibility = rank == visible_rank
        obj_b.ViewObject.Visibility = rank == visible_rank
        group.addObject(obj_a)
        group.addObject(obj_b)

    doc.recompute()
    doc.commitTransaction()
    App.Console.PrintMessage(
        "AutoMate: selected faces {}->{}, {}->{}; local MCFs={}, {}; "
        "pairs={}, Top-K={}, inference={:.2f}s\n".format(
            face_selections[0][1], result["parts"][0]["selected_face"],
            face_selections[1][1], result["parts"][1]["selected_face"],
            result["parts"][0]["local_mcf_count"], result["parts"][1]["local_mcf_count"],
            result["pair_count"], len(result["recommendations"]),
            result["elapsed_seconds"],
        )
    )
    App.Console.PrintMessage(
        "AutoMate: red=A, blue=B; only the selected rank is visible initially. "
        "Toggle other ranks under AutoMate Predictions.\n"
    )
    if result["recommendations"]:
        best = result["recommendations"][0]
        App.Console.PrintMessage(
            "AutoMate: rank 1 predicts mate type {} ({:.1%} confidence); "
            "selected Placement rank is {}.\n".format(
                best.get("mate_type", "UNKNOWN"),
                best.get("mate_type_confidence", 0.0),
                (result["recommendations"][selected_recommendation_index]["rank"]
                 if selected_recommendation_index is not None else "none"),
            )
        )
    if applied_delta is not None:
        App.Console.PrintMessage(
            "AutoMate: applied selected rank to '{}'; Ctrl+Z restores its previous Placement.\n".format(
                selected[1].Label,
            )
        )
    return group, result


AUTOMATE_PREDICTION = run()
