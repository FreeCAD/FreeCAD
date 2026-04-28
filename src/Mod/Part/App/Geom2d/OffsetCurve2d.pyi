# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Metadata import export
from Part.Geom2d import Curve2d

@export(
    Name="OffsetCurve2dPy",
    Namespace="Part",
    Twin="Geom2dOffsetCurve",
    TwinPointer="Geom2dOffsetCurve",
    PythonName="Part.Geom2d.OffsetCurve2d",
    FatherInclude="Mod/Part/App/Geom2d/Curve2dPy.h",
    Include="Mod/Part/App/Geometry2d.h",
    Father="Curve2dPy",
    FatherNamespace="Part",
    Constructor=True,
)
class OffsetCurve2d(Curve2d):
    """
    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    OffsetValue: float = ...
    """Sets or gets the offset value to offset the underlying curve."""

    BasisCurve: object = ...
    """Sets or gets the basic curve."""
