# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.Conic2d import Conic2d
from typing import Final, overload

@export(
    Twin="Geom2dHyperbola",
    TwinPointer="Geom2dHyperbola",
    PythonName="Part.Geom2d.Hyperbola2d",
    FatherInclude="Mod/Part/App/Geom2d/Conic2dPy.h",
    Include="Mod/Part/App/Geometry2d.h",
    Constructor=True,
)
class Hyperbola2d(Conic2d):
    """
    Describes a hyperbola in 2D space
    To create a hyperbola there are several ways:
    Part.Geom2d.Hyperbola2d()
        Creates a hyperbola with major radius 2 and minor radius 1 with the
        center in (0,0)

    Part.Geom2d.Hyperbola2d(Hyperbola)
        Create a copy of the given hyperbola

    Part.Geom2d.Hyperbola2d(S1,S2,Center)
        Creates a hyperbola centered on the point Center, S1 and S2,
        its major axis is defined by Center and S1,
        its major radius is the distance between Center and S1, and
        its minor radius is the distance between S2 and the major axis.

    Part.Geom2d.Hyperbola2d(Center,MajorRadius,MinorRadius)
        Creates a hyperbola with major and minor radii MajorRadius and
        MinorRadius and located at Center

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    MajorRadius: float = ...
    """The major radius of the hyperbola."""

    MinorRadius: float = ...
    """The minor radius of the hyperbola."""

    Focal: Final[float] = ...
    """The focal distance of the hyperbola."""

    Focus1: Final[object] = ...
    """
    The first focus is on the positive side of the major axis of the hyperbola;
    the second focus is on the negative side.
    """

    Focus2: Final[object] = ...
    """
    The first focus is on the positive side of the major axis of the hyperbola;
    the second focus is on the negative side.
    """

    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, Hyperbola: "Hyperbola2d") -> None: ...
    @overload
    def __init__(self, S1: object, S2: object, Center: object) -> None: ...
    @overload
    def __init__(self, Center: object, MajorRadius: float, MinorRadius: float) -> None: ...
