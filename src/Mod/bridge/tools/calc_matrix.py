import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Any
from bridge import json_response, mcp


@mcp.tool()
def calc_matrix(
    ctx: Context,
    operation: str,
    matrix: list[list[Any]],
    matrix_b: list[list[Any]] | None = None,
    vector: list[Any] | None = None,
) -> list[TextContent]:
    """Perform linear algebra on matrices.

    Unary operations: determinant, inverse, transpose, rank, trace,
    eigenvalues, eigenvectors, nullspace, rref, norm. Binary operations:
    add, subtract, multiply (need matrix_b) and solve (needs vector as the
    right-hand side). Numeric matrices use numpy; symbolic entries use sympy.

    Args:
        operation: The operation name from the lists above.
        matrix: The primary matrix as a list of equal-length rows.
        matrix_b: The second matrix for add, subtract, or multiply.
        vector: The right-hand side for solve.
    """
    return json_response(
        calcinator.matrix_operation(operation, matrix, matrix_b, vector)
    )
