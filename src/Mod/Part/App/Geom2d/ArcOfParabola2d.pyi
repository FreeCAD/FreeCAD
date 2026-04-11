# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Metadata import export, overload
from typing import Final
from Part.Geom2d import ArcOfConic2d

@export(
    PythonName="Part.Geom2d.ArcOfParabola2d",
    Twin="Geom2dArcOfParabola",
    TwinPointer="Geom2dArcOfParabola",
    Include="Mod/Part/App/Geometry2d.h",
    FatherInclude="Mod/Part/App/Geom2d/ArcOfConic2dPy.h",
    Constructor=True,
)
class ArcOfParabola2d(ArcOfConic2d):
    """
    Describes a portion of a parabola.

    Author: Werner Mayer
    Licence: LGPL
    """

    Focal: float = ...
    """The focal length of the parabola."""

    Parabola: Final[object] = ...
    """The internal parabola representation."""

    @overload
    def __init__(self) -> None: ...
