# SPDX-License-Identifier: LGPL-2.1-or-later

"""Import FreeCADMbD result frames into an MbDFEM document."""

import json
from pathlib import Path

import FreeCAD as App


def import_results(assembly, filename):
    """Import a JSON result file and return the created/updated result object.

    Expected result shape:
    {
      "frames": [
        {
          "time": 0.0,
          "placements": {
            "PartName": {"base": [x, y, z], "rotation": [q0, q1, q2, q3]}
          }
        }
      ]
    }
    """
    path = Path(filename)
    data = json.loads(path.read_text(encoding="utf-8"))
    frames = data.get("frames", [])

    document = assembly.Document
    result = _ensure_result_object(assembly)
    part_names = _part_names(assembly)

    result.Assembly = assembly
    result.ResultFile = str(path)
    result.PartNames = part_names
    result.Times = [float(frame.get("time", index)) for index, frame in enumerate(frames)]
    result.Placements = _flatten_placements(part_names, frames)

    if frames:
        apply_frame(assembly, frames[-1])
        result.CurrentFrame = len(frames) - 1

    document.recompute()
    return result


def apply_frame(assembly, frame):
    placements = frame.get("placements", {})
    for part in list(getattr(assembly, "parts", [])) + list(getattr(assembly, "fixedparts", [])):
        placement_data = placements.get(part.Name)
        if placement_data:
            part.Placement = _placement_from_data(placement_data)


def _ensure_result_object(assembly):
    document = assembly.Document
    name = f"{assembly.Name}_FreeCADMbDResults"
    result = document.getObject(name)
    if result is None:
        result = document.addObject("App::FeaturePython", name)
        result.Label = "FreeCADMbD Results"

    _ensure_property(result, "App::PropertyLink", "Assembly", "MbDFEM", "Solved MbDAssembly")
    _ensure_property(result, "App::PropertyString", "ResultFile", "MbDFEM", "FreeCADMbD result file")
    _ensure_property(result, "App::PropertyStringList", "PartNames", "MbDFEM", "Part order for result frames")
    _ensure_property(result, "App::PropertyFloatList", "Times", "MbDFEM", "Frame times")
    _ensure_property(result, "App::PropertyPlacementList", "Placements", "MbDFEM", "Flattened frame placements")
    _ensure_property(result, "App::PropertyInteger", "CurrentFrame", "MbDFEM", "Current imported frame")
    return result


def _ensure_property(obj, property_type, name, group, description):
    if not hasattr(obj, name):
        obj.addProperty(property_type, name, group, description)


def _assembly_parts(assembly):
    return list(getattr(assembly, "parts", [])) + list(getattr(assembly, "fixedparts", []))


def _part_names(assembly):
    return [part.Name for part in _assembly_parts(assembly)]


def _flatten_placements(part_names, frames):
    placements = []
    for frame in frames:
        frame_placements = frame.get("placements", {})
        for part_name in part_names:
            placement_data = frame_placements.get(part_name)
            placements.append(_placement_from_data(placement_data) if placement_data else App.Placement())
    return placements


def _placement_from_data(data):
    base = data.get("base", [0.0, 0.0, 0.0])
    rotation = data.get("rotation", [0.0, 0.0, 0.0, 1.0])
    return App.Placement(
        App.Vector(float(base[0]), float(base[1]), float(base[2])),
        App.Rotation(float(rotation[0]), float(rotation[1]), float(rotation[2]), float(rotation[3])),
    )
