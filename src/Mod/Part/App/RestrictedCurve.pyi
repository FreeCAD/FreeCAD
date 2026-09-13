# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from TrimmedCurve import TrimmedCurve
from typing import overload
from GeometryCurve import GeometryCurve

@export(
    PythonName="Part.RestrictedCurve",
    Twin="GeomRestrictedCurve",
    TwinPointer="GeomRestrictedCurve",
    Include="Mod/Part/App/Geometry.h",
    # Father="TrimmedCurvePy",
    FatherInclude="Mod/Part/App/TrimmedCurvePy.h",
    Constructor=True,
)
class RestrictedCurve(TrimmedCurve):
    """
    Describes a subset of a general curve

    Author: Ajinkya P. Dahale (dahale.a.p@gmail.com)
    Licence: LGPL
    """

    # @overload
    # def __init__(self) -> None: ...

    FirstParam: float = ...
    """The first parameter of the restricted curve."""

    LastParam: float = ...
    """The last parameter of the restricted curve."""

    BasisCurve: GeometryCurve = ...
    """Sets or gets the basis curve."""
