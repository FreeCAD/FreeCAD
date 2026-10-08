# SPDX-License-Identifier: LGPL-2.1-or-later
"""Editable native FreeCAD mounting plate; no external Python CAD dependencies."""

import math

import FreeCAD as App


def build(params):
    allowed = {"length", "width", "thickness", "hole_radius"}
    if set(params) - allowed:
        raise ValueError(f"Unknown parameters: {sorted(set(params) - allowed)}")
    values = {"length": 40.0, "width": 20.0, "thickness": 4.0, "hole_radius": 2.0}
    values.update(params)
    values = {key: float(value) for key, value in values.items()}
    if any(not math.isfinite(value) or value <= 0 for value in values.values()):
        raise ValueError("Dimensions must be finite & positive")
    if values["hole_radius"] >= min(values["length"] / 4, values["width"] / 2):
        raise ValueError("Holes must remain separate & inside plate")
    doc = App.newDocument("MountingPlate")
    base = doc.addObject("Part::Box", "Plate")
    base.Length, base.Width, base.Height = values["length"], values["width"], values["thickness"]
    holes = []
    for name, fraction in (("HoleLeft", 0.25), ("HoleRight", 0.75)):
        hole = doc.addObject("Part::Cylinder", name)
        hole.Radius = values["hole_radius"]
        hole.setExpression("Height", "Plate.Height")
        hole.setExpression("Placement.Base.x", f"Plate.Length * {fraction}")
        hole.setExpression("Placement.Base.y", "Plate.Width / 2")
        holes.append(hole)
    holes[1].setExpression("Radius", "HoleLeft.Radius")
    tool = doc.addObject("Part::MultiFuse", "Holes")
    tool.Shapes = holes
    result = doc.addObject("Part::Cut", "DrilledPlate")
    result.Base, result.Tool = base, tool
    result.Refine = True
    doc.recompute()
    return result
