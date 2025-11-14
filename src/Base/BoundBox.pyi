# SPDX-License-Identifier: LGPL-2.1-or-later

from Metadata import export, constmethod
from PyObjectBase import PyObjectBase
from Vector import Vector
from Matrix import Matrix
from typing import overload, Any, Final, Tuple, Union

@export(
    TwinPointer="BoundBox3d",
    Constructor=True,
    Delete=True,
)
class BoundBox(PyObjectBase):
    """
    Base.BoundBox class.

    This class represents a bounding box.
    A bounding box is a rectangular cuboid which is a way to describe outer
    boundaries and is obtained from a lot of 3D types.
    It is often used to check if a 3D entity lies in the range of another object.
    Checking for bounding interference first can save a lot of computing time!
    An invalid BoundBox is represented by inconsistent values at each direction:
    The maximum float value of the system at the minimum coordinates, and the
    opposite value at the maximum coordinates.

    The following constructors are supported:

    BoundBox()
    Empty constructor. Returns an invalid BoundBox.

    BoundBox(boundBox)
    Copy constructor.
    boundBox : Base.BoundBox

    BoundBox(xMin, yMin=0, zMin=0, xMax=0, yMax=0, zMax=0)
    Define from the minimum and maximum values at each direction.
    xMin : float
        Minimum value at x-coordinate.
    yMin : float
        Minimum value at y-coordinate.
    zMin : float
        Minimum value at z-coordinate.
    xMax : float
        Maximum value at x-coordinate.
    yMax : float
        Maximum value at y-coordinate.
    zMax : float
        Maximum value at z-coordinate.

    App.BoundBox(min, max)
    Define from two containers representing the minimum and maximum values of the
    coordinates in each direction.
    min : Base.Vector, tuple
        Minimum values of the coordinates.
    max : Base.Vector, tuple
        Maximum values of the coordinates.

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    Center: Final[Any] = None
    """Center point of the bounding box."""

    XMax: float = 0.0
    """The maximum x boundary position."""

    YMax: float = 0.0
    """The maximum y boundary position."""

    ZMax: float = 0.0
    """The maximum z boundary position."""

    XMin: float = 0.0
    """The minimum x boundary position."""

    YMin: float = 0.0
    """The minimum y boundary position."""

    ZMin: float = 0.0
    """The minimum z boundary position."""

    XLength: Final[float] = 0.0
    """Length of the bounding box in x direction."""

    YLength: Final[float] = 0.0
    """Length of the bounding box in y direction."""

    ZLength: Final[float] = 0.0
    """Length of the bounding box in z direction."""

    DiagonalLength: Final[float] = 0.0
    """Diagonal length of the bounding box."""

    # fmt: off
    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, boundBox: "BoundBox") -> None: ...
    @overload
    def __init__(
        self,
        xMin: float,
        yMin: float = 0,
        zMin: float = 0,
        xMax: float = 0,
        yMax: float = 0,
        zMax: float = 0,
    ) -> None: ...
    @overload
    def __init__(
        self,
        min: Union[Vector, Tuple[float, float, float]],
        max: Union[Vector, Tuple[float, float, float]],
    ) -> None: ...
    # fmt: on

    def setVoid(self) -> None:
        """
        setVoid() -> None

        Invalidate the bounding box.
        """
        ...

    @constmethod
    def isValid(self) -> bool:
        """
        isValid() -> bool

        Checks if the bounding box is valid.
        """
        ...

    @overload
    def add(self, minMax: Vector) -> None: ...
    @overload
    def add(self, minMax: Tuple[float, float, float]) -> None: ...
    @overload
    def add(self, x: float, y: float, z: float) -> None: ...
    def add(self, *args: Any, **kwargs: Any) -> None:
        """
        add(minMax) -> None
        add(x, y, z) -> None

        Increase the maximum values or decrease the minimum values of this BoundBox by
        replacing the current values with the given values, so the bounding box can grow
        but not shrink.

        minMax : Base.Vector, tuple
            Values to enlarge at each direction.
        x : float
            Value to enlarge at x-direction.
        y : float
            Value to enlarge at y-direction.
        z : float
            Value to enlarge at z-direction.
        """
        ...

    @constmethod
    def getPoint(self, index: int) -> Vector:
        """
        getPoint(index) -> Base.Vector

        Get the point of the given index.
        The index must be in the range of [0, 7].

        index : int
        """
        ...

    @constmethod
    def getEdge(self, index: int) -> Tuple[Vector, ...]:
        """
        getEdge(index) -> tuple of Base.Vector

        Get the edge points of the given index.
        The index must be in the range of [0, 11].

        index : int
        """
        ...

    @overload
    def closestPoint(self, point: Vector) -> Vector: ...
    @overload
    def closestPoint(self, x: float, y: float, z: float) -> Vector: ...
    @constmethod
    def closestPoint(self, *args: Any, **kwargs: Any) -> Vector:
        """
        closestPoint(point) -> Base.Vector
        closestPoint(x, y, z) -> Base.Vector

        Get the closest point of the bounding box to the given point.

        point : Base.Vector, tuple
            Coordinates of the given point.
        x : float
            X-coordinate of the given point.
        y : float
            Y-coordinate of the given point.
        z : float
            Z-coordinate of the given point.
        """
        ...

    @overload
    def intersect(self, boundBox2: "BoundBox") -> bool: ...
    @overload
    def intersect(
        self,
        base: Union[Vector, Tuple[float, float, float]],
        dir: Union[Vector, Tuple[float, float, float]],
    ) -> bool: ...
    def intersect(self, *args: Any) -> bool:
        """
        intersect(boundBox2) -> bool
        intersect(base, dir) -> bool

        Checks if the given object intersects with this bounding box. That can be
        another bounding box or a line specified by base and direction.

        boundBox2 : Base.BoundBox
        base : Base.Vector, tuple
        dir : Base.Vector, tuple
        """
        ...

    def intersected(self, boundBox2: "BoundBox") -> "BoundBox":
        """
        intersected(boundBox2) -> Base.BoundBox

        Returns the intersection of this and the given bounding box.

        boundBox2 : Base.BoundBox
        """
        ...

    def united(self, boundBox2: "BoundBox") -> "BoundBox":
        """
        united(boundBox2) -> Base.BoundBox

        Returns the union of this and the given bounding box.

        boundBox2 : Base.BoundBox
        """
        ...

    def enlarge(self, variation: float) -> None:
        """
        enlarge(variation) -> None

        Decrease the minimum values and increase the maximum values by the given value.
        A negative value shrinks the bounding box.

        variation : float
        """
        ...

    def getIntersectionPoint(self, base: Vector, dir: Vector, epsilon: float = 0.0001) -> Vector:
        """
        getIntersectionPoint(base, dir, epsilon=0.0001) -> Base.Vector

        Calculate the intersection point of a line with the bounding box.
        The base point must lie inside the bounding box, if not an exception is thrown.

        base : Base.Vector
            Base point of the line.
        dir : Base.Vector
            Direction of the line.
        epsilon : float
            Bounding box size tolerance.
        """
        ...

    @overload
    def move(self, displacement: Vector) -> None: ...
    @overload
    def move(self, displacement: Tuple[float, float, float]) -> None: ...
    @overload
    def move(self, x: float, y: float, z: float) -> None: ...
    def move(self, *args: Any, **kwargs: Any) -> None:
        """
        move(displacement) -> None
        move(x, y, z) -> None

        Move the bounding box by the given values.

        displacement : Base.Vector, tuple
            Displacement at each direction.
        x : float
            Displacement at x-direction.
        y : float
            Displacement at y-direction.
        z : float
            Displacement at z-direction.
        """
        ...

    @overload
    def scale(self, factor: Vector) -> None: ...
    @overload
    def scale(self, factor: Tuple[float, float, float]) -> None: ...
    @overload
    def scale(self, x: float, y: float, z: float) -> None: ...
    def scale(self, *args: Any, **kwargs: Any) -> None:
        """
        scale(factor) -> None
        scale(x, y, z) -> None

        Scale the bounding box by the given values.

        factor : Base.Vector, tuple
            Factor scale at each direction.
        x : float
            Scale at x-direction.
        y : float
            Scale at y-direction.
        z : float
            Scale at z-direction.
        """
        ...

    def transformed(self, matrix: Matrix) -> "BoundBox":
        """
        transformed(matrix) -> Base.BoundBox

        Returns a new BoundBox containing the transformed rectangular cuboid
        represented by this BoundBox.

        matrix : Base.Matrix
            Transformation matrix.
        """
        ...

    def isCutPlane(self, base: Vector, normal: Vector) -> bool:
        """
        isCutPlane(base, normal) -> bool

        Check if the plane specified by base and normal intersects (cuts) this bounding
        box.

        base : Base.Vector
        normal : Base.Vector
        """
        ...

    @overload
    def isInside(self, object: Vector) -> bool: ...
    @overload
    def isInside(self, object: "BoundBox") -> bool: ...
    @overload
    def isInside(self, x: float, y: float, z: float) -> bool: ...
    def isInside(self, *args: Any) -> bool:
        """
        isInside(object) -> bool
        isInside(x, y, z) -> bool

        Check if a point or a bounding box is inside this bounding box.

        object : Base.Vector, Base.BoundBox
            Object to check if it is inside this bounding box.
        x : float
            X-coordinate of the point to check.
        y : float
            Y-coordinate of the point to check.
        z : float
            Z-coordinate of the point to check.
        """
        ...
