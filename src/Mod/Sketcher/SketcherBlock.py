# SPDX-License-Identifier: LGPL-2.1-or-later

"""Geometry snapshots in the text format produced by Sketcher's Copy Elements.

Only the geometry-building vocabulary emitted by PythonConverter is accepted.
Files are parsed as data; they cannot import modules or execute Python scripts.
Copied constraints are read for format compatibility. The inserted snapshot is
controlled by its Group handle, rather than the source sketch's dimensions.
"""

import ast
import math
from pathlib import Path

import FreeCAD as App
import Part
import Sketcher

_CONSTRUCTORS = {
    "App.Vector": App.Vector,
    "Part.Point": Part.Point,
    "Part.LineSegment": Part.LineSegment,
    "Part.Circle": Part.Circle,
    "Part.ArcOfCircle": Part.ArcOfCircle,
    "Part.Ellipse": Part.Ellipse,
    "Part.ArcOfEllipse": Part.ArcOfEllipse,
    "Part.Hyperbola": Part.Hyperbola,
    "Part.ArcOfHyperbola": Part.ArcOfHyperbola,
    "Part.Parabola": Part.Parabola,
    "Part.ArcOfParabola": Part.ArcOfParabola,
    "Part.BSplineCurve": Part.BSplineCurve,
    "Sketcher.Constraint": Sketcher.Constraint,
}
_LISTS = {"geoList", "constrGeoList", "constraintList"}
_GEOMETRY_TYPES = tuple(value for name, value in _CONSTRUCTORS.items() if name.startswith("Part."))


def read(filename):
    """Read a .txt block as Part geometry, including construction flags."""
    text = Path(filename).read_text(encoding="utf-8-sig")
    if not text.startswith("# Copied from sketcher."):
        raise ValueError("Expected text produced by Sketcher's Copy Elements command")
    variables = {"lastGeoId": 0}
    geometry = []

    def value(node):
        if isinstance(node, ast.Constant) and type(node.value) in (str, int, float, bool):
            if isinstance(node.value, float) and not math.isfinite(node.value):
                raise ValueError("Block coordinates must be finite")
            return node.value
        if isinstance(node, ast.List):
            return [value(item) for item in node.elts]
        if isinstance(node, ast.Name) and node.id in variables:
            return variables[node.id]
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.USub, ast.UAdd)):
            number = value(node.operand)
            if type(number) not in (int, float):
                raise ValueError("Expected a number")
            return -number if isinstance(node.op, ast.USub) else number
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add):
            left, right = value(node.left), value(node.right)
            if type(left) is int and type(right) is int:
                return left + right
        if isinstance(node, ast.Call) and not node.keywords:
            name = ast.unparse(node.func)
            if name == "len" and len(node.args) == 1:
                if ast.unparse(node.args[0]) in ("ActiveSketch.Geometry", "objectStr.Geometry"):
                    return 0
            if name in _CONSTRUCTORS:
                return _CONSTRUCTORS[name](*(value(arg) for arg in node.args))
        raise ValueError("Unsupported expression in block file")

    for statement in ast.parse(text, filename=str(filename)).body:
        if isinstance(statement, ast.Assign) and len(statement.targets) == 1:
            target = statement.targets[0]
            if isinstance(target, ast.Name) and target.id in _LISTS | {"lastGeoId"}:
                result = value(statement.value)
                if (target.id in _LISTS and isinstance(result, list)) or (
                    target.id == "lastGeoId" and result == 0
                ):
                    variables[target.id] = result
                    continue
        elif isinstance(statement, ast.Delete):
            if all(isinstance(t, ast.Name) and t.id in _LISTS for t in statement.targets):
                for target in statement.targets:
                    variables.pop(target.id, None)
                continue
        elif isinstance(statement, ast.Expr) and isinstance(statement.value, ast.Call):
            call = statement.value
            name = ast.unparse(call.func)
            if call.keywords:
                raise ValueError("Unexpected keyword arguments in block file")
            if name in {f"{key}.append" for key in _LISTS} and len(call.args) == 1:
                variables[name.split(".")[0]].append(value(call.args[0]))
                continue
            if name == "objectStr.addGeometry" and len(call.args) == 2:
                items, construction = (value(arg) for arg in call.args)
                if type(construction) is not bool:
                    raise ValueError("Expected a construction flag")
                for item in items if isinstance(items, list) else [items]:
                    if not isinstance(item, _GEOMETRY_TYPES):
                        raise ValueError("Expected Part geometry")
                    facade = Sketcher.GeometryFacade(item)
                    facade.Construction = construction
                    geometry.append(facade.Geometry)
                continue
            if name == "objectStr.addConstraint" and len(call.args) == 1:
                items = value(call.args[0])
                if all(
                    isinstance(item, Sketcher.Constraint)
                    for item in (items if isinstance(items, list) else [items])
                ):
                    continue
        raise ValueError("Unsupported statement in block file")
    if not geometry:
        raise ValueError("The block file contains no geometry")
    return geometry


def insert_geometry(sketch, geometry, filename):
    """Import geometry at its native size and location as a file-backed Group."""
    if not geometry:
        raise ValueError("The source file contains no geometry")
    bounds = Part.makeCompound([geo.toShape() for geo in geometry]).BoundBox
    height = bounds.XLength < 1e-7
    size = bounds.YLength if height else bounds.XLength
    if not math.isfinite(size) or size < 1e-7:
        raise ValueError("The group width or height must be greater than zero")
    if abs(bounds.ZMin) > 1e-7 or abs(bounds.ZMax) > 1e-7:
        raise ValueError("Group geometry must lie in the sketch XY plane")
    start = App.Vector(bounds.XMin, bounds.YMin, 0)
    end = start + (App.Vector(0, size, 0) if height else App.Vector(size, 0, 0))
    elements = []
    for geo in geometry:
        index = sketch.addGeometry(geo, Sketcher.GeometryFacade(geo).Construction)
        elements.extend((index, 0))
    handle = sketch.addGeometry(Part.LineSegment(start, end), True)
    group = Sketcher.Constraint(
        "Group", [handle, 0] + elements, str(Path(filename).resolve()), height
    )
    group.Name = Path(filename).stem
    return sketch.addConstraint(group)


def reload(sketch, constraint_index):
    """Reload a text block; the caller owns the undo transaction."""
    group = sketch.Constraints[constraint_index]
    if group.Type != "Group" or not group.File:
        raise ValueError("The selected group has no source file")
    return sketch.replaceGroupGeometry(constraint_index, read(group.File))
