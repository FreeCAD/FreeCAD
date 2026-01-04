# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, overload
from typing import Final
from Part.Geom2d import ArcOfConic2d

@export(
    PythonName="Part.Geom2d.ArcOfHyperbola2d",
    Twin="Geom2dArcOfHyperbola",
    TwinPointer="Geom2dArcOfHyperbola",
    Include="Mod/Part/App/Geometry2d.h",
    FatherInclude="Mod/Part/App/Geom2d/ArcOfConic2dPy.h",
    Constructor=True,
)
class ArcOfHyperbola2d(ArcOfConic2d):
    """
    Describes a portion of an hyperbola
    Author: Werner Mayer (wmayer@users.sourceforge.net) Licence: LGPL
    """

    MajorRadius: float = ...
    """The major radius of the hyperbola."""

    MinorRadius: float = ...
    """The minor radius of the hyperbola."""

    Hyperbola: Final[object] = ...
    """The internal hyperbola representation"""

    @overload
    def __init__(self) -> None: ...
