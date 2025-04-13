from Metadata import export, constmethod
from PyObjectBase import PyObjectBase
from Axis import Axis as AxisPy
from Vector import Vector
from Placement import Placement
from Rotation import Rotation
from typing import Union

@export(
    Constructor=True,
    Delete=True,
)
class CoordinateSystem(PyObjectBase):
    """
    Base.CoordinateSystem class.

    An orthonormal right-handed coordinate system in 3D space.

    CoordinateSystem()
    Empty constructor.

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    Axis: AxisPy = None
    """Set or get axis."""

    XDirection: Vector = None
    """Set or get X-direction."""

    YDirection: Vector = None
    """Set or get Y-direction."""

    ZDirection: Vector = None
    """Set or get Z-direction."""

    Position: Vector = None
    """Set or get position."""

    def setAxes(self, axis: Union[AxisPy, Vector], xDir: Vector) -> None:
        """
        setAxes(axis, xDir) -> None

        Set axis or Z-direction, and X-direction.
        The X-direction is determined from the orthonormal compononent of `xDir`
        with respect to `axis` direction.

        axis : Base.Axis, Base.Vector
        xDir : Base.Vector
        """
        ...

    @constmethod
    def displacement(self, coordSystem2: "CoordinateSystem") -> Placement:
        """
        displacement(coordSystem2) -> Base.Placement

        Computes the placement from this to the passed coordinate system `coordSystem2`.

        coordSystem2 : Base.CoordinateSystem
        """
        ...

    def transformTo(self, vector: Vector) -> Vector:
        """
        transformTo(vector) -> Base.Vector

        Computes the coordinates of the point in coordinates of this coordinate system.

        vector : Base.Vector
        """
        ...

    def transform(self, trans: Union[Rotation, Placement]) -> None:
        """
        transform(trans) -> None

        Applies a transformation on this coordinate system.

        trans : Base.Rotation, Base.Placement
        """
        ...

    def setPlacement(self, placement: Placement) -> None:
        """
        setPlacement(placement) -> None

        Set placement to the coordinate system.

        placement : Base.Placement
        """
        ...
