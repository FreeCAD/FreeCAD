# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from GeometryExtension import GeometryExtension

@export(
    Include="Mod/Part/App/GeometryDefaultExtension.h",
)
class GeometryDoubleExtension(GeometryExtension):
    """
    A GeometryExtension extending geometry objects with a double.

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    def __init__(self) -> None: ...

    Value: float = ...
    """Returns the value of the GeometryDoubleExtension."""
