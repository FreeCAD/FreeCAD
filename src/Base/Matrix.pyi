from Vector import Vector
from Metadata import export, constmethod, class_declarations, no_args
from PyObjectBase import PyObjectBase
from enum import IntEnum
from typing import overload, Union, Tuple, Sequence

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
@class_declarations(
    """public:
      MatrixPy(const Matrix4D & mat, PyTypeObject *T = &Type)
      :PyObjectBase(new Matrix4D(mat),T){}
      Matrix4D value() const
      { return *(getMatrixPtr()); }
    """
)
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

    A11: float = 0.0
    """The (1,1) matrix element."""
    A12: float = 0.0
    """The (1,2) matrix element."""
    A13: float = 0.0
    """The (1,3) matrix element."""
    A14: float = 0.0
    """The (1,4) matrix element."""
    A21: float = 0.0
    """The (2,1) matrix element."""
    A22: float = 0.0
    """The (2,2) matrix element."""
    A23: float = 0.0
    """The (2,3) matrix element."""
    A24: float = 0.0
    """The (2,4) matrix element."""
    A31: float = 0.0
    """The (3,1) matrix element."""
    A32: float = 0.0
    """The (3,2) matrix element."""
    A33: float = 0.0
    """The (3,3) matrix element."""
    A34: float = 0.0
    """The (3,4) matrix element."""
    A41: float = 0.0
    """The (4,1) matrix element."""
    A42: float = 0.0
    """The (4,2) matrix element."""
    A43: float = 0.0
    """The (4,3) matrix element."""
    A44: float = 0.0
    """The (4,4) matrix element."""

    A: Sequence[float] = []
    """The matrix elements."""

    @overload
    def move(self, vector: Vector) -> None: ...
    @overload
    def move(self, x: float, y: float, z: float) -> None: ...
    def move(self, *args) -> None:
        """
        move(vector) -> None
        move(x, y, z) -> None

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
        ...

    @overload
    def scale(self, vector: Vector) -> None: ...
    @overload
    def scale(self, x: float, y: float, z: float) -> None: ...
    @overload
    def scale(self, factor: float) -> None: ...
    def scale(self, *args) -> None:
        """
        scale(vector) -> None
        scale(x, y, z) -> None
        scale(factor) -> None

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
        ...

    @constmethod
    def hasScale(self, tol: float = 0) -> ScaleType:
        """
        hasScale(tol=0) -> ScaleType

        Return an enum value of ScaleType. Possible values are:
        Uniform, NonUniformLeft, NonUniformRight, NoScaling or Other
        if it's not a scale matrix.

        tol : float
        """
        ...

    @constmethod
    def decompose(self) -> Tuple["Matrix", "Matrix", "Matrix", "Matrix"]:
        """
        decompose() -> Base.Matrix, Base.Matrix, Base.Matrix, Base.Matrix

        Return a tuple of matrices representing shear, scale, rotation and move.
        So that matrix = move * rotation * scale * shear.
        """
        ...

    @no_args
    def nullify(self) -> None:
        """
        nullify() -> None

        Make this the null matrix.
        """
        ...

    @no_args
    @constmethod
    def isNull(self) -> bool:
        """
        isNull() -> bool

        Check if this is the null matrix.
        """
        ...

    @no_args
    def unity(self) -> None:
        """
        unity() -> None

        Make this matrix to unity (4D identity matrix).
        """
        ...

    @constmethod
    def isUnity(self, tol: float = 0.0) -> bool:
        """
        isUnity([tol=0.0]) -> bool

        Check if this is the unit matrix (4D identity matrix).
        """
        ...

    def transform(self, vector: Vector, matrix2: "Matrix") -> None:
        """
        transform(vector, matrix2) -> None

        Transform the matrix around a given point.
        Equivalent to left multiply the matrix by T*M*T_inv, where M is `matrix2`, T the
        translation generated by `vector` and T_inv the inverse translation.
        For example, if `matrix2` is a rotation, the result is the transformation generated
        by the current matrix followed by a rotation around the point represented by `vector`.

        vector : Base.Vector
        matrix2 : Base.Matrix
        """
        ...

    @constmethod
    def col(self, index: int) -> Vector:
        """
        col(index) -> Base.Vector

        Return the vector of a column, that is, the vector generated by the three
        first elements of the specified column.

        index : int
            Required column index.
        """
        ...

    def setCol(self, index: int, vector: Vector) -> None:
        """
        setCol(index, vector) -> None

        Set the vector of a column, that is, the three first elements of the specified
        column by index.

        index : int
            Required column index.
        vector : Base.Vector
        """
        ...

    @constmethod
    def row(self, index: int) -> Vector:
        """
        row(index) -> Base.Vector

        Return the vector of a row, that is, the vector generated by the three
        first elements of the specified row.

        index : int
            Required row index.
        """
        ...

    def setRow(self, index: int, vector: Vector) -> None:
        """
        setRow(index, vector) -> None

        Set the vector of a row, that is, the three first elements of the specified
        row by index.

        index : int
            Required row index.
        vector : Base.Vector
        """
        ...

    @no_args
    @constmethod
    def diagonal(self) -> Vector:
        """
        diagonal() -> Base.Vector

        Return the diagonal of the 3x3 leading principal submatrix as vector.
        """
        ...

    def setDiagonal(self, vector: Vector) -> None:
        """
        setDiagonal(vector) -> None

        Set the diagonal of the 3x3 leading principal submatrix.

        vector : Base.Vector
        """
        ...

    def rotateX(self, angle: float) -> None:
        """
        rotateX(angle) -> None

        Rotate around X axis.

        angle : float
            Angle in radians.
        """
        ...

    def rotateY(self, angle: float) -> None:
        """
        rotateY(angle) -> None

        Rotate around Y axis.

        angle : float
            Angle in radians.
        """
        ...

    def rotateZ(self, angle: float) -> None:
        """
        rotateZ(angle) -> None

        Rotate around Z axis.

        angle : float
            Angle in radians.
        """
        ...

    @overload
    def multiply(self, matrix: "Matrix") -> "Matrix": ...
    @overload
    def multiply(self, vector: Vector) -> Vector: ...
    @constmethod
    def multiply(self, obj: Union["Matrix", Vector]) -> Union["Matrix", Vector]:
        """
        multiply(matrix) -> Base.Matrix
        multiply(vector) -> Base.Vector

        Right multiply the matrix by the given object.
        If the argument is a vector, this is augmented to the 4D vector (`vector`, 1).

        matrix : Base.Matrix
        vector : Base.Vector
        """
        ...

    @constmethod
    def multVec(self, vector: Vector) -> Vector:
        """
        multVec(vector) -> Base.Vector

        Compute the transformed vector using the matrix.

        vector : Base.Vector
        """
        ...

    @no_args
    def invert(self) -> None:
        """
        invert() -> None

        Compute the inverse matrix in-place, if possible.
        """
        ...

    @no_args
    @constmethod
    def inverse(self) -> "Matrix":
        """
        inverse() -> Base.Matrix

        Compute the inverse matrix, if possible.
        """
        ...

    @no_args
    def transpose(self) -> None:
        """
        transpose() -> None

        Transpose the matrix in-place.
        """
        ...

    @no_args
    @constmethod
    def transposed(self) -> "Matrix":
        """
        transposed() -> Base.Matrix

        Returns a transposed copy of this matrix.
        """
        ...

    @no_args
    @constmethod
    def determinant(self) -> float:
        """
        determinant() -> float

        Compute the determinant of the matrix.
        """
        ...

    @constmethod
    def isOrthogonal(self, tol: float = 1e-6) -> float:
        """
        isOrthogonal(tol=1e-6) -> float

        Checks if the matrix is orthogonal, i.e. M * M^T = k*I and returns
        the multiple of the identity matrix. If it's not orthogonal 0 is returned.

        tol : float
            Tolerance used to check orthogonality.
        """
        ...

    @constmethod
    def submatrix(self, dim: int) -> "Matrix":
        """
        submatrix(dim) -> Base.Matrix

        Get the leading principal submatrix of the given dimension.
        The (4 - `dim`) remaining dimensions are completed with the
        corresponding identity matrix.

        dim : int
            Dimension parameter must be in the range [1,4].
        """
        ...

    @no_args
    @constmethod
    def analyze(self) -> str:
        """
        analyze() -> str

        Analyzes the type of transformation.
        """
        ...
