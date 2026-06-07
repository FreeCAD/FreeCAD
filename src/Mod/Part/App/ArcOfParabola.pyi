# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from ArcOfConic import ArcOfConic
from typing import Final

@export(
    PythonName="Part.ArcOfParabola",
    Twin="GeomArcOfParabola",
)
class ArcOfParabola(ArcOfConic):
    """
    Describes a portion of a parabola

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    def __init__(self) -> None: ...

    Focal: float = ...
    """The focal length of the parabola."""

    Parabola: Final[object] = ...
    """The internal parabola representation"""
