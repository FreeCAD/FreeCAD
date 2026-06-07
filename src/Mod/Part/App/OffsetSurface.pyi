# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from GeometrySurface import GeometrySurface

@export(
    Twin="GeomOffsetSurface",
    PythonName="Part.OffsetSurface",
    Constructor=True,
)
class OffsetSurface(GeometrySurface):
    """
    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    OffsetValue: float = 0.0
    """Sets or gets the offset value to offset the underlying surface."""

    BasisSurface: object = ...
    """Sets or gets the basic surface."""
