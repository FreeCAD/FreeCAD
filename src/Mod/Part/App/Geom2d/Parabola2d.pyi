# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.App.Geom2d.Conic2d import Conic2d
from typing import Final

@export(
    Include="Mod/Part/App/Geometry2d.h",
    FatherInclude="Mod/Part/App/Geom2d/Conic2dPy.h",
    Twin="Geom2dParabola",
    TwinPointer="Geom2dParabola",
    PythonName="Part.Geom2d.Parabola2d",
    Constructor=True,
)
class Parabola2d(Conic2d):
    """
    Describes a parabola in 2D space

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Focal: float = ...
    """
    The focal distance is the distance between the apex and the focus of the parabola.
    """

    Focus: Final[object] = ...
    """
    The focus is on the positive side of the
    'X Axis' of the local coordinate system of the parabola.
    """

    Parameter: Final[float] = ...
    """
    Compute the parameter of this parabola which is the distance between its focus
    and its directrix. This distance is twice the focal length.
    """
