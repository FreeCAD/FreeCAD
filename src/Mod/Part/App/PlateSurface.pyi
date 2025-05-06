from Base.Metadata import export
from GeometrySurface import GeometrySurface


@export(
    Twin="GeomPlateSurface",
    TwinPointer="GeomPlateSurface",
    PythonName="Part.PlateSurface",
    FatherInclude="Mod/Part/App/GeometrySurfacePy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class PlateSurface(GeometrySurface):
    """
    Represents a plate surface in FreeCAD. Plate surfaces can be defined by specifying points or curves as constraints, and they can also be approximated to B-spline surfaces using the makeApprox method. This class is commonly used in CAD modeling for creating surfaces that represent flat or curved plates, such as sheet metal components or structural elements.
    """

    def makeApprox(
        self,
        *,
        Tol3d: float = 0,
        MaxSegments: int = 0,
        MaxDegree: int = 0,
        MaxDistance: float = 0,
        CritOrder: int = 0,
        Continuity: str = "",
        EnlargeCoeff: float = 0
    ) -> None:
        """
        Approximate the plate surface to a B-Spline surface
        """
        ...
