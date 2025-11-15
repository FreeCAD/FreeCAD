# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.Vector import Vector
from TrimmedCurve import TrimmedCurve
from typing import overload

@export(
    Father="TrimmedCurvePy",
    PythonName="Part.ArcOfConic",
    Twin="GeomArcOfConic",
    TwinPointer="GeomArcOfConic",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/TrimmedCurvePy.h",
    Constructor=True,
)
class ArcOfConic(TrimmedCurve):
    """
    Describes a portion of a conic

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    @overload
    def __init__(self) -> None: ...

    Location: Vector = ...
    """Center of the conic."""

    Center: Vector = ...
    """Deprecated -- use Location."""

    AngleXU: float = ...
    """The angle between the X axis and the major axis of the conic."""

    Axis: Vector = ...
    """The axis direction of the conic"""

    XAxis: Vector = ...
    """The X axis direction of the circle"""

    YAxis: Vector = ...
    """The Y axis direction of the circle"""
