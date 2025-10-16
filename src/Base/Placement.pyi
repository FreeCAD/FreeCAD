# SPDX-License-Identifier: LGPL-2.1-or-later

from Metadata import export, constmethod, class_declarations
from PyObjectBase import PyObjectBase
from Matrix import Matrix as MatrixPy
from Rotation import Rotation as RotationPy
from Vector import Vector
from typing import Sequence, overload

@export(
    Constructor=True,
    Delete=True,
    NumberProtocol=True,
    RichCompare=True,
)
@class_declarations(
    """public:
            PlacementPy(const Placement & pla, PyTypeObject *T = &Type)
            :PyObjectBase(new Placement(pla),T){}
            Placement value() const
            { return *(getPlacementPtr()); }
        """
)
class Placement(PyObjectBase):
    """
    Base.Placement class.

    A Placement defines an orientation (rotation) and a position (base) in 3D space.
    It is used when no scaling or other distortion is needed.

    The following constructors are supported:

    Placement()
    Empty constructor.

    Placement(placement)
    Copy constructor.
    placement : Base.Placement

    Placement(matrix)
    Define from a 4D matrix consisting of rotation and translation.
    matrix : Base.Matrix

    Placement(base, rotation)
    Define from position and rotation.
    base : Base.Vector
    rotation : Base.Rotation

    Placement(base, rotation, center)
    Define from position and rotation with center.
    base : Base.Vector
    rotation : Base.Rotation
    center : Base.Vector

    Placement(base, axis, angle)
    define position and rotation.
    base : Base.Vector
    axis : Base.Vector
    angle : float
    """

    Base: Vector = None
    """Vector to the Base Position of the Placement."""

    Rotation: Vector = None
    """Orientation of the placement expressed as rotation."""

    Matrix: MatrixPy = None
    """Set/get matrix representation of the placement."""

    # fmt: off
    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, placement: "Placement") -> None: ...
    @overload
    def __init__(self, matrix: MatrixPy) -> None: ...
    @overload
    def __init__(self, base: Vector, rotation: RotationPy) -> None: ...
    @overload
    def __init__(self, base: Vector, rotation: RotationPy, center: Vector) -> None: ...
    @overload
    def __init__(self, base: Vector, axis: Vector, angle: float) -> None: ...
    # fmt: on

    @constmethod
    def copy(self) -> "Placement":
        """
        copy() -> Base.Placement

        Returns a copy of this placement.
        """
        ...

    def move(self, vector: Vector) -> None:
        """
        move(vector) -> None

        Move the placement along a vector.

        vector : Base.Vector
            Vector by which to move the placement.
        """
        ...

    def translate(self, vector: Vector) -> None:
        """
        translate(vector) -> None

        Alias to move(), to be compatible with TopoShape.translate().

        vector : Base.Vector
            Vector by which to move the placement.
        """
        ...

    @overload
    def rotate(
        self, center: Sequence[float], axis: Sequence[float], angle: float, *, comp: bool = False
    ) -> None: ...
    def rotate(self, center: Vector, axis: Vector, angle: float, *, comp: bool = False) -> None:
        """
        rotate(center, axis, angle, comp) -> None

        Rotate the current placement around center and axis with the given angle.
        This method is compatible with TopoShape.rotate() if the (optional) keyword
        argument comp is True (default=False).

        center : Base.Vector, sequence of float
            Rotation center.
        axis : Base.Vector, sequence of float
            Rotation axis.
        angle : float
            Rotation angle in degrees.
        comp : bool
            optional keyword only argument, if True (default=False),
        behave like TopoShape.rotate() (i.e. the resulting placements are interchangeable).
        """
        ...

    @constmethod
    def multiply(self, placement: "Placement") -> "Placement":
        """
        multiply(placement) -> Base.Placement

        Right multiply this placement with another placement.
        Also available as `*` operator.

        placement : Base.Placement
            Placement by which to multiply this placement.
        """
        ...

    @constmethod
    def multVec(self, vector: Vector) -> Vector:
        """
        multVec(vector) -> Base.Vector

        Compute the transformed vector using the placement.

        vector : Base.Vector
            Vector to be transformed.
        """
        ...

    @constmethod
    def toMatrix(self) -> MatrixPy:
        """
        toMatrix() -> Base.Matrix

        Compute the matrix representation of the placement.
        """
        ...

    @constmethod
    def inverse(self) -> "Placement":
        """
        inverse() -> Base.Placement

        Compute the inverse placement.
        """
        ...

    @constmethod
    def pow(self, t: float, shorten: bool = True) -> "Placement":
        """
        pow(t, shorten=True) -> Base.Placement

        Raise this placement to real power using ScLERP interpolation.
        Also available as `**` operator.

        t : float
            Real power.
        shorten : bool
            If True, ensures rotation quaternion is net positive to make
            the path shorter.
        """
        ...

    @constmethod
    def sclerp(self, placement2: "Placement", t: float, shorten: bool = True) -> "Placement":
        """
        sclerp(placement2, t, shorten=True) -> Base.Placement

        Screw Linear Interpolation (ScLERP) between this placement and `placement2`.
        Interpolation is a continuous motion along a helical path parametrized by `t`
        made of equal transforms if discretized.
        If quaternions of rotations of the two placements differ in sign, the interpolation
        will take a long path.

        placement2 : Base.Placement
        t : float
            Parameter of helical path. t=0 returns this placement, t=1 returns
            `placement2`. t can also be outside of [0, 1] range for extrapolation.
        shorten : bool
            If True, the signs are harmonized before interpolation and the interpolation
            takes the shorter path.
        """
        ...

    @constmethod
    def slerp(self, placement2: "Placement", t: float) -> "Placement":
        """
        slerp(placement2, t) -> Base.Placement

        Spherical Linear Interpolation (SLERP) between this placement and `placement2`.
        This function performs independent interpolation of rotation and movement.
        Result of such interpolation might be not what application expects, thus this tool
        might be considered for simple cases or for interpolating between small intervals.
        For more complex cases you better use the advanced sclerp() function.

        placement2 : Base.Placement
        t : float
            Parameter of the path. t=0 returns this placement, t=1 returns `placement2`.
        """
        ...

    @constmethod
    def isIdentity(self, tol: float = 0.0) -> bool:
        """
        isIdentity([tol=0.0]) -> bool

        Returns True if the placement has no displacement and no rotation.
        Matrix representation is the 4D identity matrix.
        tol : float
            Tolerance used to check for identity.
            If tol is negative or zero, no tolerance is used.
        """
        ...

    @constmethod
    def isSame(self, other: "Placement", tol: float = 0.0) -> bool:
        """
        isSame(Base.Placement, [tol=0.0]) -> bool

        Checks whether this and the given placement are the same.
        The default tolerance is set to 0.0
        """
        ...
