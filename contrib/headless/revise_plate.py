# SPDX-License-Identifier: LGPL-2.1-or-later
"""Reopen plate.FCStd, change dimensions & add another editable cut."""

from pathlib import Path

import FreeCAD as App


def build(params):
    doc = App.openDocument(str(Path(params["document"]).resolve(strict=True)))
    doc.Plate.Length = 55
    doc.Plate.Height = 6
    # Existing hole positions & depths follow their expressions after reopening.
    hole = doc.addObject("Part::Cylinder", "CentreHole")
    hole.Radius = 3
    hole.setExpression("Height", "Plate.Height")
    hole.setExpression("Placement.Base.x", "Plate.Length / 2")
    hole.setExpression("Placement.Base.y", "Plate.Width / 2")
    result = doc.addObject("Part::Cut", "RevisedPlate")
    result.Base, result.Tool = doc.DrilledPlate, hole
    result.Refine = True
    doc.recompute()
    if abs(doc.HoleRight.Placement.Base.x - 41.25) > 1e-9:
        raise ValueError("Saved hole-position expression did not recompute")
    if doc.HoleLeft.Height.Value != 6:
        raise ValueError("Saved hole-depth expression did not recompute")
    return result
