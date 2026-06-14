# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Final, overload
from Base.Metadata import export
from Part.Conic2d import Conic2d

@export(
    Twin="Geom2dEllipse",
    TwinPointer="Geom2dEllipse",
    PythonName="Part.Geom2d.Ellipse2d",
    FatherInclude="Mod/Part/App/Geom2d/Conic2dPy.h",
    Include="Mod/Part/App/Geometry2d.h",
    Constructor=True,
)
class Ellipse2d(Conic2d):
    """
    Describes an ellipse in 2D space
    To create an ellipse there are several ways:
    Part.Geom2d.Ellipse2d()
        Creates an ellipse with major radius 2 and minor radius 1 with the
        center in (0,0)

    Part.Geom2d.Ellipse2d(Ellipse)
        Create a copy of the given ellipse

    Part.Geom2d.Ellipse2d(S1,S2,Center)
        Creates an ellipse centered on the point Center,
        its major axis is defined by Center and S1,
        its major radius is the distance between Center and S1, and
        its minor radius is the distance between S2 and the major axis.

    Part.Geom2d.Ellipse2d(Center,MajorRadius,MinorRadius)
        Creates an ellipse with major and minor radii MajorRadius and
        MinorRadius

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    MajorRadius: float = ...
    """The major radius of the ellipse."""

    MinorRadius: float = ...
    """The minor radius of the ellipse."""

    Focal: Final[float] = ...
    """The focal distance of the ellipse."""

    Focus1: Final[object] = ...
    """The first focus is on the positive side of the major axis of the ellipse."""

    Focus2: Final[object] = ...
    """The second focus is on the negative side of the major axis of the ellipse."""

    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, Ellipse: "Ellipse2d") -> None: ...
    @overload
    def __init__(self, S1: object, S2: object, Center: object) -> None: ...
    @overload
    def __init__(self, Center: object, MajorRadius: float, MinorRadius: float) -> None: ...
    @overload
    def __init__(self, *args, **kwargs) -> None: ...
