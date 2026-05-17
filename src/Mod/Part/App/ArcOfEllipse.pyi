# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, typing_only
from ArcOfConic import ArcOfConic
from typing import Final, overload

@export(
    PythonName="Part.ArcOfEllipse",
    Twin="GeomArcOfEllipse",
    TwinPointer="GeomArcOfEllipse",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/ArcOfConicPy.h",
    FatherNamespace="Part",
    Constructor=True,
)
class ArcOfEllipse(ArcOfConic):
    """
    Describes a portion of an ellipse

    Author: Abdullah Tahiri (abdullah.tahiri.yo[at]gmail.com)
    Licence: LGPL
    """

    MajorRadius: float = ...
    """The major radius of the ellipse."""

    MinorRadius: float = ...
    """The minor radius of the ellipse."""

    Ellipse: Final[object] = ...
    """The internal ellipse representation"""

    @typing_only
    @overload
    def __init__(
        self, ellipse: "Part.Ellipse", u1: float, u2: float, sense: bool = ..., /
    ) -> None: ...
