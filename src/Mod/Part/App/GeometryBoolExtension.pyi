# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from GeometryExtension import GeometryExtension

@export(
    Include="Mod/Part/App/GeometryDefaultExtension.h",
)
class GeometryBoolExtension(GeometryExtension):
    """
    A GeometryExtension extending geometry objects with a boolean.
    """

    def __init__(self) -> None: ...

    Value: bool = ...
    """Returns the value of the GeometryBoolExtension."""
