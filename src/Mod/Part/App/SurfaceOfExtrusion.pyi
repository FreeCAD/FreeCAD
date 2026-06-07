# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from GeometrySurface import GeometrySurface
from GeometryCurve import GeometryCurve
from Base.Vector import Vector

@export(
    Twin="GeomSurfaceOfExtrusion",
    PythonName="Part.SurfaceOfExtrusion",
    Constructor=True,
)
class SurfaceOfExtrusion(GeometrySurface):
    """
    Describes a surface of linear extrusion
    Author: Werner Mayer (<wmayer@users.sourceforge.net>)
    Licence: LGPL
    """

    Direction: Vector = ...
    """Sets or gets the direction of revolution."""

    BasisCurve: GeometryCurve = ...
    """Sets or gets the basic curve."""
