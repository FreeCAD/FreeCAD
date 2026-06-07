# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.GeometryExtension import GeometryExtension

@export(
    Include="Mod/Part/App/GeometryDefaultExtension.h",
)
class GeometryIntExtension(GeometryExtension):
    """
    A GeometryExtension extending geometry objects with an int.

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    def __init__(self) -> None: ...

    Value: int = ...
    """returns the value of the GeometryIntExtension."""
