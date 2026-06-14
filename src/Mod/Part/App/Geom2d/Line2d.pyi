# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.Geom2d.Curve2d import Curve2d

@export(
    PythonName="Part.Geom2d.Line2d",
    Twin="Geom2dLine",
    TwinPointer="Geom2dLine",
    Include="Mod/Part/App/Geometry2d.h",
    FatherInclude="Mod/Part/App/Geom2d/Curve2dPy.h",
    Constructor=True,
)
class Line2d(Curve2d):
    """
    Describes an infinite line in 2D space
    To create a line there are several ways:
    Part.Geom2d.Line2d()
        Creates a default line.

    Part.Geom2d.Line2d(Line)
        Creates a copy of the given line.

    Part.Geom2d.Line2d(Point,Dir)
        Creates a line that goes through two given points.

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Location: object = ...
    """Returns the location of this line."""

    Direction: object = ...
    """Returns the direction of this line."""
