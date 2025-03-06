from Metadata import export, constmethod, class_declarations
from PyObjectBase import PyObjectBase
from Vector import Vector
from Matrix import Matrix
from typing import overload, Tuple, List, Final

@export(
    Constructor=True,
    Delete=True,
    NumberProtocol=True,
    RichCompare=True,
)
@class_declarations(
    """public:
    RotationPy(const Rotation & mat, PyTypeObject *T = &Type)
    :PyObjectBase(new Rotation(mat),T){}
    Rotation value() const
    { return *(getRotationPtr()); }
        """
)
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

    Rotation(angle1, angle2, angle3)
    Define from three floats (Euler angles) as yaw-pitch-roll in XY'Z'' convention.
    angle1 : float
    angle2 : float
    angle3 : float

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

    Rotation(dir1, dir2, dir3, seq)
    Define from three vectors that define rotated axes directions plus an optional
    3-characher string of capital letters 'X', 'Y', 'Z' that sets the order of
    importance of the axes (e.g., 'ZXY' means z direction is followed strictly,
    x is used but corrected if necessary, y is ignored).
    dir1 : Base.Vector
    dir2 : Base.Vector
    dir3 : Base.Vector
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

    Q: Tuple[float, ...] = ()
    """The rotation elements (as quaternion)."""

    Axis: object = None
    """The rotation axis of the quaternion."""

    RawAxis: Final[object] = None
    """The rotation axis without normalization."""

    Angle: float = 0.0
    """The rotation angle of the quaternion."""

    # TODO: Provide strongly-typed enum for `seq`
    # fmt: off
    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, rotation: "Rotation") -> None: ...
    @overload
    def __init__(self, axis: Vector, angle: float) -> None: ...
    @overload
    def __init__(self, vector_start: Vector, vector_end: Vector) -> None: ...
    @overload
    def __init__(self, angle1: float, angle2: float, angle3: float) -> None: ...
    @overload
    def __init__(self, seq: str, angle1: float, angle2: float, angle3: float) -> None: ...
    @overload
    def __init__(self, x: float, y: float, z: float, w: float) -> None: ...
    @overload
    def __init__(self, dir1: Vector, dir2: Vector, dir3: Vector, seq: str) -> None: ...
    @overload
    def __init__(self, matrix: Matrix) -> None: ...
    @overload
    def __init__(self, *coef: float) -> None: ...
    # fmt: on

    def invert(self) -> None:
        """
        invert() -> None

        Sets the rotation to its inverse.
        """
        ...

    @constmethod
    def inverted(self) -> "Rotation":
        """
        inverted() -> Base.Rotation

        Returns the inverse of the rotation.
        """
        ...

    def isSame(self, rotation: "Rotation", tol: float = 0) -> bool:
        """
        isSame(rotation, tol=0) -> bool

        Checks if `rotation` perform the same transformation as this rotation.

        rotation : Base.Rotation
        tol : float
            Tolerance used to compare both rotations.
            If tol is negative or zero, no tolerance is used.
        """
        ...

    @constmethod
    def multiply(self, rotation: "Rotation") -> "Rotation":
        """
        multiply(rotation) -> Base.Rotation

        Right multiply this rotation with another rotation.

        rotation : Base.Rotation
            Rotation by which to multiply this rotation.
        """
        ...

    @constmethod
    def multVec(self, vector: Vector) -> Vector:
        """
        multVec(vector) -> Base.Vector

        Compute the transformed vector using the rotation.

        vector : Base.Vector
            Vector to be transformed.
        """
        ...

    @constmethod
    def slerp(self, rotation2: "Rotation", t: float) -> "Rotation":
        """
        slerp(rotation2, t) -> Base.Rotation

        Spherical Linear Interpolation (SLERP) of this rotation and `rotation2`.

        t : float
            Parameter of the path. t=0 returns this rotation, t=1 returns `rotation2`.
        """
        ...

    def setYawPitchRoll(self, angle1: float, angle2: float, angle3: float) -> None:
        """
        setYawPitchRoll(angle1, angle2, angle3) -> None

        Set the Euler angles of this rotation as yaw-pitch-roll in XY'Z'' convention.

        angle1 : float
            Angle around yaw axis in degrees.
        angle2 : float
            Angle around pitch axis in degrees.
        angle3 : float
            Angle around roll axis in degrees.
        """
        ...

    @constmethod
    def getYawPitchRoll(self) -> Tuple[float, float, float]:
        """
        getYawPitchRoll() -> tuple

        Get the Euler angles of this rotation as yaw-pitch-roll in XY'Z'' convention.
        The angles are given in degrees.
        """
        ...

    def setEulerAngles(self, seq: str, angle1: float, angle2: float, angle3: float) -> None:
        """
        setEulerAngles(seq, angle1, angle2, angle3) -> None

        Set the Euler angles in a given sequence for this rotation.
        The angles must be given in degrees.

        seq : str
            Euler sequence name. All possible values given by toEulerAngles().
        angle1 : float
        angle2 : float
        angle3 : float
        """
        ...

    @constmethod
    def toEulerAngles(self, seq: str = "") -> List[float]:
        """
        toEulerAngles(seq) -> list

        Get the Euler angles in a given sequence for this rotation.

        seq : str
            Euler sequence name. If not given, the function returns
            all possible values of `seq`. Optional.
        """
        ...

    @constmethod
    def toMatrix(self) -> Matrix:
        """
        toMatrix() -> Base.Matrix

        Convert the rotation to a 4D matrix representation.
        """
        ...

    @constmethod
    def isNull(self) -> bool:
        """
        isNull() -> bool

        Returns True if all values in the quaternion representation are zero.
        """
        ...

    @constmethod
    def isIdentity(self, tol: float = 0) -> bool:
        """
        isIdentity(tol=0) -> bool

        Returns True if the rotation equals the 4D identity matrix.
        tol : float
            Tolerance used to check for identity.
            If tol is negative or zero, no tolerance is used.
        """
        ...
