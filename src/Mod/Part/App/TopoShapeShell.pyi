from Base.Metadata import export, constmethod
from TopoShape import TopoShape
from typing import Final, Dict


@export(
    Twin="TopoShape",
    TwinPointer="TopoShape",
    Include="Mod/Part/App/TopoShape.h",
    FatherInclude="Mod/Part/App/TopoShapePy.h",
    Constructor=True,
)
class TopoShapeShell(TopoShape):
    """
    Create a shell out of a list of faces

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    Licence: LGPL
    """

    Mass: Final[object] = ...
    """Returns the mass of the current system."""

    CenterOfMass: Final[object] = ...
    """
    Returns the center of mass of the current system.
    If the gravitational field is uniform, it is the center of gravity.
    The coordinates returned for the center of mass are expressed in the
    absolute Cartesian coordinate system.
    """

    MatrixOfInertia: Final[object] = ...
    """
    Returns the matrix of inertia. It is a symmetrical matrix.
    The coefficients of the matrix are the quadratic moments of
    inertia.
    
     | Ixx Ixy Ixz 0 |
     | Ixy Iyy Iyz 0 |
     | Ixz Iyz Izz 0 |
     | 0   0   0   1 |
    
    The moments of inertia are denoted by Ixx, Iyy, Izz.
    The products of inertia are denoted by Ixy, Ixz, Iyz.
    The matrix of inertia is returned in the central coordinate
    system (G, Gx, Gy, Gz) where G is the centre of mass of the
    system and Gx, Gy, Gz the directions parallel to the X(1,0,0)
    Y(0,1,0) Z(0,0,1) directions of the absolute cartesian
    coordinate system.
    """

    StaticMoments: Final[object] = ...
    """
    Returns Ix, Iy, Iz, the static moments of inertia of the
    current system; i.e. the moments of inertia about the
    three axes of the Cartesian coordinate system.
    """

    PrincipalProperties: Final[Dict] = ...
    """
    Computes the principal properties of inertia of the current system.
    There is always a set of axes for which the products
    of inertia of a geometric system are equal to 0; i.e. the
    matrix of inertia of the system is diagonal. These axes
    are the principal axes of inertia. Their origin is
    coincident with the center of mass of the system. The
    associated moments are called the principal moments of inertia.
    This function computes the eigen values and the
    eigen vectors of the matrix of inertia of the system.
    """

    def add(self, face: object) -> None:
        """
        Add a face to the shell.
        add(face)
        """
        ...

    @constmethod
    def getFreeEdges(self) -> object:
        """
        Get free edges as compound.
        getFreeEdges() -> compound
        """
        ...

    @constmethod
    def getBadEdges(self) -> object:
        """
        Get bad edges as compound.
        getBadEdges() -> compound
        """
        ...

    @constmethod
    def makeHalfSpace(self, point: object) -> object:
        """
        Make a half-space solid by this shell and a reference point.
        makeHalfSpace(point) -> Solid
        """
        ...
