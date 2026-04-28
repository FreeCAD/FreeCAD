# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.Vector import Vector
from TopoShape import TopoShape
from typing import Final, Tuple, Dict, Optional, List

@export(
    Twin="TopoShape",
    TwinPointer="TopoShape",
    Include="Mod/Part/App/TopoShape.h",
    FatherInclude="Mod/Part/App/TopoShapePy.h",
    Constructor=True,
)
class TopoShapeFace(TopoShape):
    """
    TopoShapeFace is the OpenCasCade topological face wrapper

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    Licence: LGPL
    """

    Tolerance: float = ...
    """Set or get the tolerance of the vertex"""

    ParameterRange: Final[Tuple] = ...
    """Returns a 4 tuple with the parameter range"""

    Surface: Final[object] = ...
    """Returns the geometric surface of the face"""

    Wire: Final[object] = ...
    """
    The outer wire of this face
    deprecated -- use OuterWire
    """

    OuterWire: Final[object] = ...
    """The outer wire of this face"""

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

    def addWire(self, wire: object, /) -> None:
        """
        Adds a wire to the face.
        addWire(wire)
        """
        ...

    @constmethod
    def makeOffset(self, dist: float, /) -> object:
        """
        Offset the face by a given amount.
        makeOffset(dist) -> Face
        --
        Returns Compound of Wires. Deprecated - use makeOffset2D instead.
        """
        ...

    @constmethod
    def makeEvolved(
        self,
        Profile: TopoShape,
        Join: int,
        *,
        AxeProf: bool,
        Solid: bool,
        ProfOnSpine: bool,
        Tolerance: float,
    ) -> TopoShape:
        """
        Profile along the spine
        """
        ...

    @constmethod
    def getUVNodes(self) -> List[Tuple[float, float]]:
        """
        Get the list of (u,v) nodes of the tessellation
        getUVNodes() -> list
        --
        An exception is raised if the face is not triangulated.
        """
        ...

    @constmethod
    def tangentAt(self, u: float, v: float, /) -> Vector:
        """
        Get the tangent in u and v isoparametric at the given point if defined
        tangentAt(u,v) -> Vector
        """
        ...

    @constmethod
    def valueAt(self, u: float, v: float, /) -> Vector:
        """
        Get the point at the given parameter [0|Length] if defined
        valueAt(u,v) -> Vector
        """
        ...

    @constmethod
    def normalAt(self, pos: float, /) -> Vector:
        """
        Get the normal vector at the given parameter [0|Length] if defined
        normalAt(pos) -> Vector
        """
        ...

    @constmethod
    def derivative1At(self, u: float, v: float, /) -> Tuple[Vector, Vector]:
        """
        Get the first derivative at the given parameter [0|Length] if defined
        derivative1At(u,v) -> (vectorU,vectorV)
        """
        ...

    @constmethod
    def derivative2At(self, u: float, v: float, /) -> Tuple[Vector, Vector]:
        """
        Vector = d2At(pos) - Get the second derivative at the given parameter [0|Length] if defined
        derivative2At(u,v) -> (vectorU,vectorV)
        """
        ...

    @constmethod
    def curvatureAt(self, u: float, v: float, /) -> float:
        """
        Get the curvature at the given parameter [0|Length] if defined
        curvatureAt(u,v) -> Float
        """
        ...

    @constmethod
    def isPartOfDomain(self, u: float, v: float, /) -> bool:
        """
        Check if a given (u,v) pair is inside the domain of a face
        isPartOfDomain(u,v) -> bool
        """
        ...

    @constmethod
    def makeHalfSpace(self, pos: object, /) -> object:
        """
        Make a half-space solid by this face and a reference point.
        makeHalfSpace(pos) -> Shape
        """
        ...

    def validate(self) -> None:
        """
        Validate the face.
        validate()
        """
        ...

    @constmethod
    def countNodes(self) -> int:
        """
        Returns the number of nodes of the triangulation.
        """
        ...

    @constmethod
    def countTriangles(self) -> int:
        """
        Returns the number of triangles of the triangulation.
        """
        ...

    @constmethod
    def curveOnSurface(self, Edge: object, /) -> Optional[Tuple[object, float, float]]:
        """
        Returns the curve associated to the edge in the parametric space of the face.
        curveOnSurface(Edge) -> (curve, min, max) or None
        --
        If this curve exists then a tuple of curve and parameter range is returned.
        Returns None if this curve does not exist.
        """
        ...

    def cutHoles(self, list_of_wires: List[object], /) -> None:
        """
        Cut holes in the face.
        cutHoles(list_of_wires)
        """
        ...
