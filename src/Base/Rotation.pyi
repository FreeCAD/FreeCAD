# SPDX-License-Identifier: LGPL-2.1-or-later

from Metadata import export, constmethod, class_declarations, typing_only
from PyObjectBase import PyObjectBase
from Vector import Vector
from Matrix import Matrix
from Placement import Placement
from typing import Final, TypeAlias, Literal, overload, TYPE_CHECKING
from collections.abc import Sequence

@export(
    Constructor=True,
    Delete=True,
    NumberProtocol=True,
    RichCompare=True,
)
@class_declarations("""public:
    RotationPy(const Rotation & mat, PyTypeObject *T = &Type)
    :PyObjectBase(new Rotation(mat),T){}
    Rotation value() const
    { return *(getRotationPtr()); }
        """)
class Rotation(PyObjectBase):
    """
    Base.Rotation class.

    A Rotation using a quaternion.

    The following constructors are supported:

    Rotation()
    Empty constructor.

    Rotation(rotation)
    Copy constructor.

    Rotation(Axis, Radian)
    Rotation(Axis, Degree)
    Define from an axis and an angle (in radians or degrees according to the keyword).
    Axis : Base.Vector
    Radian : float
    Degree : float

    Rotation(vector_start, vector_end)
    Define from two vectors (rotation from/to vector).
    vector_start : Base.Vector
    vector_end : Base.Vector

    Rotation(yaw, pitch, roll)
    Define from three floats (Euler angles) as yaw-pitch-roll in XY'Z'' convention.
    yaw : float
    pitch : float
    roll : float

    Rotation(seq, angle1, angle2, angle3)
    Define from one string and three floats (Euler angles) as Euler rotation
    of a given type. Call toEulerAngles() for supported sequence types.
    seq : str
    angle1 : float
    angle2 : float
    angle3 : float

    Rotation(x, y, z, w)
    Define from four floats (quaternion) where the quaternion is specified as:
    q = xi+yj+zk+w, i.e. the last parameter is the real part.
    x : float
    y : float
    z : float
    w : float

    Rotation(xdir, ydir, zdir, seq)
    Define from three vectors that define rotated axes directions plus an optional
    3-characher string of capital letters 'X', 'Y', 'Z' that sets the order of
    importance of the axes (e.g., 'ZXY' means z direction is followed strictly,
    x is used but corrected if necessary, y is ignored).
    xdir : Base.Vector
    ydir : Base.Vector
    zdir : Base.Vector
    seq : str

    Rotation(matrix)
    Define from a matrix rotation in the 4D representation.
    matrix : Base.Matrix

    Rotation(*coef)
    Define from 16 or 9 elements which represent the rotation in the 4D matrix
    representation or in the 3D matrix representation, respectively.
    coef : sequence of float

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    if TYPE_CHECKING:
        _Seq: TypeAlias = Literal["XYZ", "XZY", "YXZ", "YZX", "ZXY", "ZYX"]

    Q: tuple[float, float, float, float]
    """The rotation elements (as quaternion)."""

    @property
    def Axis(self) -> Vector:
        """The rotation axis of the quaternion."""

    @Axis.setter
    def Axis(self, value: Vector | Sequence[float], /) -> None: ...

    RawAxis: Final[Vector]
    """The rotation axis without normalization."""

    Angle: float
    """The rotation angle of the quaternion."""

    # TODO: Provide strongly-typed enum for `seq`
    # fmt: off
    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, rotation: Rotation, /) -> None: ...
    @overload
    def __init__(self, /, Axis: Vector, Degree: float) -> None: ...
    @overload
    def __init__(self, vector_start: Vector, vector_end: Vector, /) -> None: ...
    @overload
    def __init__(self, yaw: float, pitch: float, roll: float, /) -> None: ...
    @overload
    def __init__(self, seq: _Seq, angle1: float, angle2: float, angle3: float, /) -> None: ...
    @overload
    def __init__(self, x: float, y: float, z: float, w: float, /) -> None: ...
    @overload
    def __init__(self, xdir: Vector, ydir: Vector, zdir: Vector, seq: _Seq = "ZXY", /) -> None: ...
    @overload
    def __init__(self, matrix: Matrix, /) -> None: ...
    @overload
    def __init__(self, /, Axis: Vector, *, Radian: float) -> None: ...
    @overload
    def __init__(
        self,
        a11: float, a12: float, a13: float, a14: float,
        a21: float, a22: float, a23: float, a24: float,
        a31: float, a32: float, a33: float, a34: float,
        a41: float, a42: float, a43: float, a44: float,
        /,
    ) -> None: ...
    @overload
    def __init__(
        self,
        a11: float, a12: float, a13: float,
        a21: float, a22: float, a23: float,
        a31: float, a32: float, a33: float,
        /,
    ) -> None: ...
    # fmt: on

    def invert(self) -> None:
        """
        Sets the rotation to its inverse.
        """

    @constmethod
    def inverted(self) -> Rotation:
        """
        Returns the inverse of the rotation.
        """

    @constmethod
    def isSame(self, rotation: Rotation, tol: float = 0.0, /) -> bool:
        """
        Checks if `rotation` perform the same transformation as this rotation.

        rotation : Base.Rotation
        tol : float
            Tolerance used to compare both rotations.
            If tol is negative or zero, no tolerance is used.
        """

    @constmethod
    def multiply(self, rotation: Rotation, /) -> Rotation:
        """
        Right multiply this rotation with another rotation.

        rotation : Base.Rotation
            Rotation by which to multiply this rotation.
        """
    # fmt: off
    @typing_only
    @overload
    def __mul__(self, vector: Vector, /) -> Vector: ...

    @typing_only
    @overload
    def __mul__(self, matrix: Matrix, /) -> Matrix: ...

    @typing_only
    @overload
    def __mul__(self, placement: Placement, /) -> Placement: ...

    @typing_only
    @overload
    def __mul__(self, rotation: Rotation, /) -> Rotation: ...

    @typing_only
    def __pow__(self, other: int, /) -> Rotation: ...
    # fmt: on

    @constmethod
    def multVec(self, vector: Vector, /) -> Vector:
        """
        Compute the transformed vector using the rotation.

        vector : Base.Vector
            Vector to be transformed.
        """

    @constmethod
    def slerp(self, rotation2: Rotation, t: float, /) -> Rotation:
        """
        Spherical Linear Interpolation (SLERP) of this rotation and `rotation2`.

        t : float
            Parameter of the path. t=0 returns this rotation, t=1 returns `rotation2`.
        """

    def setYawPitchRoll(self, yaw: float, pitch: float, roll: float, /) -> None:
        """
        Set the Euler angles of this rotation as yaw-pitch-roll in XY'Z'' convention.

        yaw : float
            Angle around yaw axis in degrees.
        pitch : float
            Angle around pitch axis in degrees.
        roll : float
            Angle around roll axis in degrees.
        """

    @constmethod
    def getYawPitchRoll(self) -> tuple[float, float, float]:
        """
        Get the Euler angles of this rotation as yaw-pitch-roll in XY'Z'' convention.
        The angles are given in degrees.
        """

    def setEulerAngles(self, seq: _Seq, angle1: float, angle2: float, angle3: float, /) -> None:
        """
        Set the Euler angles in a given sequence for this rotation.
        The angles must be given in degrees.

        seq : str
            Euler sequence name. All possible values given by toEulerAngles().
        angle1 : float
        angle2 : float
        angle3 : float
        """

    @overload
    @constmethod
    def toEulerAngles(self) -> list[str]:
        """
        Returns all possible values of `seq`.
        """

    @overload
    @constmethod
    def toEulerAngles(self, seq: _Seq, /) -> tuple[float, float, float]:
        """
        Get the Euler angles in a given sequence for this rotation.

        seq : str
            Euler sequence name. If not given, the function returns
            all possible values of `seq`. Optional.
        """

    @constmethod
    def toMatrix(self) -> Matrix:
        """
        Convert the rotation to a 4D matrix representation.
        """

    @constmethod
    def isNull(self) -> bool:
        """
        Returns True if all values in the quaternion representation are zero.
        """

    @constmethod
    def isIdentity(self, tol: float = 0.0, /) -> bool:
        """
        Returns True if the rotation equals the 4D identity matrix.
        tol : float
            Tolerance used to check for identity.
            If tol is negative or zero, no tolerance is used.
        """
