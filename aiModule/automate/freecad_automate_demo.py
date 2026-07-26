"""Visualize AutoMate mating-coordinate-frame candidates in FreeCAD.

Select one or more objects that contain a Shape, then run this file from
FreeCAD's Python console with:

    exec(open(r"E:\FreeCAD\aimodule\automate\freecad_automate_demo.py").read())
"""

from __future__ import annotations

import math
import os
import sys
import tempfile

import FreeCAD as App
import FreeCADGui as Gui
import Import
import Part


SCRIPT_PATH = globals().get(
    "__file__", r"E:\FreeCAD\aimodule\automate\freecad_automate_demo.py"
)
AUTOMATE_ROOT = os.path.dirname(os.path.abspath(SCRIPT_PATH))
AUTOMATE_BUILD = os.path.join(AUTOMATE_ROOT, "build-msvc-clean")
MAX_CANDIDATES = 100


def _load_automate_cpp():
    if AUTOMATE_BUILD not in sys.path:
        sys.path.insert(0, AUTOMATE_BUILD)
    try:
        import automate_cpp
    except ImportError as exc:
        raise RuntimeError(
            "Cannot import automate_cpp. Build the AutoMate C++ extension first."
        ) from exc
    return automate_cpp


def _canonical_axis(values):
    axis = [float(values[i]) for i in range(3)]
    length = math.sqrt(sum(value * value for value in axis))
    if length < 1.0e-12:
        return None
    axis = [value / length for value in axis]
    for value in axis:
        if abs(value) > 1.0e-9:
            if value < 0.0:
                axis = [-component for component in axis]
            break
    return axis


def _unique_candidates(mcfs, tolerance):
    unique = []
    seen = set()
    for mcf in mcfs:
        origin = [float(mcf.origin[i]) for i in range(3)]
        axis = _canonical_axis(mcf.axis)
        if axis is None:
            continue
        key = tuple(round(value / tolerance) for value in origin)
        key += tuple(round(value * 10000.0) for value in axis)
        if key in seen:
            continue
        seen.add(key)
        unique.append((origin, axis))
    return unique


def run(max_candidates=MAX_CANDIDATES):
    selected = [obj for obj in Gui.Selection.getSelection() if hasattr(obj, "Shape")]
    if not selected:
        raise RuntimeError("Select at least one object containing a Shape first.")

    doc = App.ActiveDocument
    if doc is None:
        raise RuntimeError("No active FreeCAD document.")

    automate_cpp = _load_automate_cpp()
    fd, step_path = tempfile.mkstemp(prefix="freecad_automate_", suffix=".step")
    os.close(fd)
    try:
        Import.export(selected, step_path)
        options = automate_cpp.PartOptions()
        options.default_mcfs = True
        part = automate_cpp.Part(step_path, options)
    finally:
        try:
            os.remove(step_path)
        except OSError:
            pass

    if not part.is_valid:
        raise RuntimeError("AutoMate could not read the exported STEP shape.")

    bounds = selected[0].Shape.BoundBox
    diagonal = max(bounds.DiagonalLength, 1.0)
    tolerance = max(diagonal * 1.0e-5, 1.0e-7)
    axis_length = diagonal * 0.08
    point_radius = diagonal * 0.006
    candidates = _unique_candidates(part.default_mcfs, tolerance)
    displayed = candidates[: int(max_candidates)]

    old = doc.getObject("AutoMate_CandidateAxes")
    if old is not None:
        doc.removeObject(old.Name)
    old = doc.getObject("AutoMate_CandidateOrigins")
    if old is not None:
        doc.removeObject(old.Name)

    axes = []
    origins = []
    for origin_values, axis_values in displayed:
        origin = App.Vector(*origin_values)
        axis = App.Vector(*axis_values)
        axes.append(Part.makeLine(origin, origin + axis * axis_length))
        origins.append(Part.makeSphere(point_radius, origin))

    axis_obj = doc.addObject("Part::Feature", "AutoMate_CandidateAxes")
    axis_obj.Label = "AutoMate candidate axes"
    axis_obj.Shape = Part.makeCompound(axes)
    axis_obj.ViewObject.LineColor = (1.0, 0.15, 0.15)
    axis_obj.ViewObject.LineWidth = 2.0

    origin_obj = doc.addObject("Part::Feature", "AutoMate_CandidateOrigins")
    origin_obj.Label = "AutoMate candidate origins"
    origin_obj.Shape = Part.makeCompound(origins)
    origin_obj.ViewObject.ShapeColor = (1.0, 0.75, 0.0)

    doc.recompute()
    Gui.activeDocument().activeView().fitAll()
    App.Console.PrintMessage(
        "AutoMate: faces={}, edges={}, raw MCFs={}, unique MCFs={}, displayed={}\n".format(
            len(part.brep.nodes.faces),
            len(part.brep.nodes.edges),
            len(part.default_mcfs),
            len(candidates),
            len(displayed),
        )
    )
    return axis_obj, origin_obj


run()
