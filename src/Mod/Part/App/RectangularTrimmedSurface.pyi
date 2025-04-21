from Base.Metadata import export
from GeometrySurface import GeometrySurface
from typing import Any, Final, Tuple


@export(
    Twin="GeomTrimmedSurface",
    TwinPointer="GeomTrimmedSurface",
    PythonName="Part.RectangularTrimmedSurface",
    FatherInclude="Mod/Part/App/GeometrySurfacePy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class RectangularTrimmedSurface(GeometrySurface):
    """
    Describes a portion of a surface (a patch) limited by two values of the
    u parameter in the u parametric direction, and two values of the v parameter in the v parametric
    direction. The domain of the trimmed surface must be within the domain of the surface being trimmed.

    The trimmed surface is defined by:
    - the basis surface, and
    - the values (umin, umax) and (vmin, vmax) which limit it in the u and v parametric directions.

    The trimmed surface is built from a copy of the basis surface. Therefore, when the basis surface
    is modified the trimmed surface is not changed. Consequently, the trimmed surface does not
    necessarily have the same orientation as the basis surface.

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    BasisSurface: Final[Any] = None
    """Represents the basis surface from which the trimmed surface is derived."""

    def setTrim(self, params: Tuple[float, float, float, float]) -> None:
        """
        setTrim(self, params: (u1, u2, v1, v2)) -> None

        Modifies this patch by changing the trim values applied to the original surface
        """
        ...
