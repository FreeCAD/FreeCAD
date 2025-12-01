# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from TopoShape import TopoShape
from Base.Vector import Vector
from Base.Matrix import Matrix
from typing import Final, Dict, Tuple, overload

@export(
    Twin="TopoShape",
    TwinPointer="TopoShape",
    Include="Mod/Part/App/TopoShape.h",
    FatherInclude="Mod/Part/App/TopoShapePy.h",
    Constructor=True,
)
class TopoShapeSolid(TopoShape):
    """
    Part.Solid(shape): Create a solid out of shells of shape. If shape is a compsolid, the overall volume solid is created.

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    Licence: LGPL
    """

    Mass: Final[float] = 0.0
    """Returns the mass of the current system."""

    CenterOfMass: Final[Vector] = Vector()
    """
    Returns the center of mass of the current system.
    If the gravitational field is uniform, it is the center of gravity.
    The coordinates returned for the center of mass are expressed in the
    absolute Cartesian coordinate system.
    """

    MatrixOfInertia: Final[Matrix] = Matrix()
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

    StaticMoments: Final[object] = object()
    """
    Returns Ix, Iy, Iz, the static moments of inertia of the
    current system; i.e. the moments of inertia about the
    three axes of the Cartesian coordinate system.
    """

    PrincipalProperties: Final[Dict[str, float]] = {}
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

    OuterShell: Final[TopoShape] = TopoShape()
    """
    Returns the outer most shell of this solid or an null
    shape if the solid has no shells
    """

    @constmethod
    def getMomentOfInertia(self, point: Vector, direction: Vector, /) -> float:
        """
        computes the moment of inertia of the material system about the axis A.
        getMomentOfInertia(point,direction) -> Float
        """
        ...

    @constmethod
    def getRadiusOfGyration(self, point: Vector, direction: Vector, /) -> float:
        """
        Returns the radius of gyration of the current system about the axis A.
        getRadiusOfGyration(point,direction) -> Float
        """
        ...

    @overload
    @constmethod
    def offsetFaces(self, facesTuple: Tuple[TopoShape, ...], offset: float, /) -> TopoShape: ...
    @overload
    @constmethod
    def offsetFaces(self, facesDict: Dict[TopoShape, float], /) -> TopoShape: ...
    @constmethod
    def offsetFaces(self, *args, **kwargs) -> TopoShape:
        """
        Extrude single faces of the solid.
        offsetFaces(facesTuple, offset) -> Solid
        or
        offsetFaces(dict) -> Solid
        --
        Example:
        solid.offsetFaces((solid.Faces[0],solid.Faces[1]), 1.5)

        solid.offsetFaces({solid.Faces[0]:1.0,solid.Faces[1]:2.0})
        """
        ...
