# SPDX-License-Identifier: LGPL-2.1-or-later

from Quantity import Quantity
from Vector import Vector
from Metadata import export, constmethod, class_declarations, no_args, typing_only
from PyObjectBase import PyObjectBase
from enum import IntEnum
from Rotation import Rotation
from Placement import Placement
from typing import Any, overload
from collections.abc import Sequence

class ScaleType(IntEnum):
    Other = -1
    NoScaling = 0
    NonUniformRight = 1
    NonUniformLeft = 2
    Uniform = 3

@export(
    TwinPointer="Matrix4D",
    Constructor=True,
    Delete=True,
    NumberProtocol=True,
    RichCompare=True,
)
@class_declarations("""public:
      MatrixPy(const Matrix4D & mat, PyTypeObject *T = &Type)
      :PyObjectBase(new Matrix4D(mat),T){}
      Matrix4D value() const
      { return *(getMatrixPtr()); }
    """)
class Matrix(PyObjectBase):
    """
    Base.Matrix class.

    A 4x4 Matrix.
    In particular, this matrix can represent an affine transformation, that is,
    given a 3D vector `x`, apply the transformation y = M*x + b, where the matrix
    `M` is a linear map and the vector `b` is a translation.
    `y` can be obtained using a linear transformation represented by the 4x4 matrix
    `A` conformed by the augmented 3x4 matrix (M|b), augmented by row with
    (0,0,0,1), therefore: (y, 1) = A*(x, 1).

    The following constructors are supported:

    Matrix()
    Empty constructor.

    Matrix(matrix)
    Copy constructor.
    matrix : Base.Matrix.

    Matrix(*coef)
    Define from 16 coefficients of the 4x4 matrix.
    coef : sequence of float
        The sequence can have up to 16 elements which complete the matrix by rows.

    Matrix(vector1, vector2, vector3, vector4)
    Define from four 3D vectors which represent the columns of the 3x4 submatrix,
    useful to represent an affine transformation. The fourth row is made up by
    (0,0,0,1).
    vector1 : Base.Vector
    vector2 : Base.Vector
    vector3 : Base.Vector
    vector4 : Base.Vector
        Default to (0,0,0). Optional.

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    @overload
    def __init__(self) -> None:
        """
        Empty constructor.
        """
    # fmt: off
    @overload
    def __init__(
        self,
        /,
        a11: float = 1.0, a12: float = 0.0, a13: float = 0.0, a14: float = 0.0,
        a21: float = 0.0, a22: float = 1.0, a23: float = 0.0, a24: float = 0.0,
        a31: float = 0.0, a32: float = 0.0, a33: float = 1.0, a34: float = 0.0,
        a41: float = 0.0, a42: float = 0.0, a43: float = 0.0, a44: float = 1.0,
    ) -> None:
        """
        Define from 16 coefficients of the 4x4 matrix.
        coef : sequence of float
            The sequence can have up to 16 elements which complete the matrix by rows.
        """
    # fmt: on

    @overload
    def __init__(self, matrix: Matrix, /) -> None:
        """
        Copy constructor.
        matrix : Base.Matrix.
        """

    @overload
    def __init__(self, v1: Vector, v2: Vector, v3: Vector, v4: Vector = ..., /) -> None:
        """
        Define from four 3D vectors which represent the columns of the 3x4 submatrix,
        useful to represent an affine transformation. The fourth row is made up by
        (0,0,0,1).
        v1 : Base.Vector
        v2 : Base.Vector
        v3 : Base.Vector
        v4 : Base.Vector
            Default to (0,0,0). Optional.
        """
    A11: float
    """The (1,1) matrix element."""
    A12: float
    """The (1,2) matrix element."""
    A13: float
    """The (1,3) matrix element."""
    A14: float
    """The (1,4) matrix element."""
    A21: float
    """The (2,1) matrix element."""
    A22: float
    """The (2,2) matrix element."""
    A23: float
    """The (2,3) matrix element."""
    A24: float
    """The (2,4) matrix element."""
    A31: float
    """The (3,1) matrix element."""
    A32: float
    """The (3,2) matrix element."""
    A33: float
    """The (3,3) matrix element."""
    A34: float
    """The (3,4) matrix element."""
    A41: float
    """The (4,1) matrix element."""
    A42: float
    """The (4,2) matrix element."""
    A43: float
    """The (4,3) matrix element."""
    A44: float
    """The (4,4) matrix element."""

    @property
    def A(self) -> tuple[float, ...]:
        """The matrix elements."""
    # no posonly ("/") because otherwise it won't match cpp definition
    @A.setter
    def A(self, value: Sequence[float]) -> None: ...
    @overload
    def move(self, vector: Vector | tuple[float, float, float], /) -> None: ...
    @overload
    def move(self, x: float, y: float, z: float, /) -> None: ...
    @overload
    def move(self, /, *args: Any) -> None:
        """
        Move the matrix along a vector, equivalent to left multiply the matrix
        by a pure translation transformation.

        vector : Base.Vector, tuple
        x : float
            `x` translation.
        y : float
            `y` translation.
        z : float
            `z` translation.
        """

    @overload
    def scale(self, vector: Vector | tuple[float, float, float], /) -> None: ...
    @overload
    def scale(self, x: float, y: float, z: float, /) -> None: ...
    @overload
    def scale(self, factor: float, /) -> None: ...
    @overload
    def scale(self, /, *args: Any) -> None:
        """
        Scale the first three rows of the matrix.

        vector : Base.Vector
        x : float
            First row factor scale.
        y : float
            Second row factor scale.
        z : float
            Third row factor scale.
        factor : float
            global factor scale.
        """

    @constmethod
    def hasScale(self, tol: float = 0.0, /) -> ScaleType:
        """
        Return an enum value of ScaleType. Possible values are:
        Uniform, NonUniformLeft, NonUniformRight, NoScaling or Other
        if it's not a scale matrix.

        tol : float
        """

    @constmethod
    def decompose(self) -> tuple[Matrix, Matrix, Matrix, Matrix]:
        """
        Return a tuple of matrices representing shear, scale, rotation and move.
        So that matrix = move * rotation * scale * shear.
        """

    @no_args
    def nullify(self) -> None:
        """
        Make this the null matrix.
        """

    @no_args
    @constmethod
    def isNull(self) -> bool:
        """
        Check if this is the null matrix.
        """

    @no_args
    def unity(self) -> None:
        """
        Make this matrix to unity (4D identity matrix).
        """

    @constmethod
    def isUnity(self, tol: float = 0.0, /) -> bool:
        """
        Check if this is the unit matrix (4D identity matrix).
        """

    def transform(self, vector: Vector, matrix2: Matrix, /) -> None:
        """
        Transform the matrix around a given point.
        Equivalent to left multiply the matrix by T*M*T_inv, where M is `matrix2`, T the
        translation generated by `vector` and T_inv the inverse translation.
        For example, if `matrix2` is a rotation, the result is the transformation generated
        by the current matrix followed by a rotation around the point represented by `vector`.

        vector : Base.Vector
        matrix2 : Base.Matrix
        """

    @constmethod
    def col(self, index: int, /) -> Vector:
        """
        Return the vector of a column, that is, the vector generated by the three
        first elements of the specified column.

        index : int
            Required column index.
        """

    def setCol(self, index: int, vector: Vector, /) -> None:
        """
        Set the vector of a column, that is, the three first elements of the specified
        column by index.

        index : int
            Required column index.
        vector : Base.Vector
        """

    @constmethod
    def row(self, index: int, /) -> Vector:
        """
        Return the vector of a row, that is, the vector generated by the three
        first elements of the specified row.

        index : int
            Required row index.
        """

    def setRow(self, index: int, vector: Vector, /) -> None:
        """
        Set the vector of a row, that is, the three first elements of the specified
        row by index.

        index : int
            Required row index.
        vector : Base.Vector
        """

    @no_args
    @constmethod
    def diagonal(self) -> Vector:
        """
        Return the diagonal of the 3x3 leading principal submatrix as vector.
        """

    def setDiagonal(self, vector: Vector, /) -> None:
        """
        Set the diagonal of the 3x3 leading principal submatrix.

        vector : Base.Vector
        """

    def rotateX(self, angle: float | Quantity, /) -> None:
        """
        Rotate around X axis.

        angle : float
            Angle in radians.
        """

    def rotateY(self, angle: float | Quantity, /) -> None:
        """
        Rotate around Y axis.

        angle : float
            Angle in radians.
        """

    def rotateZ(self, angle: float | Quantity, /) -> None:
        """
        Rotate around Z axis.

        angle : float
            Angle in radians.
        """

    @overload
    @constmethod
    def multiply(self, matrix: Matrix, /) -> Matrix: ...
    @overload
    @constmethod
    def multiply(self, vector: Vector, /) -> Vector: ...
    @overload
    @constmethod
    def multiply(self, obj: Matrix | Vector, /) -> Matrix | Vector:
        """
        Right multiply the matrix by the given object.
        If the argument is a vector, this is augmented to the 4D vector (`vector`, 1).

        matrix : Base.Matrix
        vector : Base.Vector
        """

    @constmethod
    def multVec(self, vector: Vector, /) -> Vector:
        """
        Compute the transformed vector using the matrix.

        vector : Base.Vector
        """

    @no_args
    def invert(self) -> None:
        """
        Compute the inverse matrix in-place, if possible.
        """

    @no_args
    @constmethod
    def inverse(self) -> Matrix:
        """
        Compute the inverse matrix, if possible.
        """

    @no_args
    def transpose(self) -> None:
        """
        Transpose the matrix in-place.
        """

    @no_args
    @constmethod
    def transposed(self) -> Matrix:
        """
        Returns a transposed copy of this matrix.
        """

    @no_args
    @constmethod
    def determinant(self) -> float:
        """
        Compute the determinant of the matrix.
        """

    @constmethod
    def isOrthogonal(self, tol: float = 1e-6, /) -> float:
        """
        Checks if the matrix is orthogonal, i.e. M * M^T = k*I and returns
        the multiple of the identity matrix. If it's not orthogonal 0 is returned.

        tol : float
            Tolerance used to check orthogonality.
        """

    @constmethod
    def submatrix(self, dim: int, /) -> Matrix:
        """
        Get the leading principal submatrix of the given dimension.
        The (4 - `dim`) remaining dimensions are completed with the
        corresponding identity matrix.

        dim : int
            Dimension parameter must be in the range [1,4].
        """

    @no_args
    @constmethod
    def analyze(self) -> str:
        """
        Analyzes the type of transformation.
        """

    @typing_only
    def __add__(self, other: Matrix, /) -> Matrix: ...
    @typing_only
    def __sub__(self, other: Matrix, /) -> Matrix: ...
    @overload
    @typing_only
    def __mul__(self, other: Vector, /) -> Vector: ...
    @overload
    @typing_only
    def __mul__(self, other: Rotation | Placement | Matrix | float, /) -> Matrix: ...
    @typing_only
    def __pow__(self, other: int, /) -> Matrix: ...
    @typing_only
    def __neg__(self) -> Matrix: ...
    @typing_only
    def __pos__(self) -> Matrix: ...
