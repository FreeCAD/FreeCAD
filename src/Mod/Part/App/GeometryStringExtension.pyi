# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from GeometryExtension import GeometryExtension

@export(
    Include="Mod/Part/App/GeometryDefaultExtension.h",
)
class GeometryStringExtension(GeometryExtension):
    """
    A GeometryExtension extending geometry objects with a string.

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    def __init__(self) -> None: ...

    Value: str = ...
    """returns the value of the GeometryStringExtension."""
