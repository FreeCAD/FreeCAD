# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.Vector import Vector
from TrimmedCurve import TrimmedCurve
from Circle import Circle
from Ellipse import Ellipse
from Hyperbola import Hyperbola
from Parabola import Parabola
from typing import overload

@export(
    Father="TrimmedCurvePy",
    PythonName="Part.Arc",
    Twin="GeomTrimmedCurve",
    TwinPointer="GeomTrimmedCurve",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/TrimmedCurvePy.h",
    Constructor=True,
)
class Arc(TrimmedCurve):
    """
    Describes a portion of a curve

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    @overload
    def __init__(self, circ: Circle, u1: float, u2: float, sense: bool = ..., /) -> None: ...
    @overload
    def __init__(self, ellipse: Ellipse, u1: float, u2: float, sense: bool = ..., /) -> None: ...
    @overload
    def __init__(self, parabola: Parabola, u1: float, u2: float, sense: bool = ..., /) -> None: ...
    @overload
    def __init__(
        self, hyperbola: Hyperbola, u1: float, u2: float, sense: bool = ..., /
    ) -> None: ...
    @overload
    def __init__(self, p1: Vector, p2: Vector, p3: Vector, /) -> None: ...
