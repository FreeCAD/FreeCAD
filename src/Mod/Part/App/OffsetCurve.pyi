# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.Vector import Vector
from GeometryCurve import GeometryCurve

@export(
    PythonName="Part.OffsetCurve",
    Twin="GeomOffsetCurve",
)
class OffsetCurve(GeometryCurve):
    """
    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def __init__(self) -> None: ...

    OffsetValue: float = ...
    """Sets or gets the offset value to offset the underlying curve."""

    OffsetDirection: Vector = ...
    """Sets or gets the offset direction to offset the underlying curve."""

    BasisCurve: GeometryCurve = ...
    """Sets or gets the basic curve."""
