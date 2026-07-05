import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Any
from bridge import json_response, mcp


@mcp.tool()
def calc_vector(
    ctx: Context,
    operation: str,
    vector_a: list[Any],
    vector_b: Any = None,
) -> list[TextContent]:
    """Perform vector algebra.

    Operations: dot, cross, add, subtract, scale, norm, normalize, angle,
    projection. cross requires two 3-D vectors; scale expects vector_b to be a
    numeric scalar; norm and normalize ignore vector_b.

    Args:
        operation: The operation name from the list above.
        vector_a: The first vector as a 1-D list.
        vector_b: The second vector, or a scalar for scale.
    """
    return json_response(calcinator.vector_operation(operation, vector_a, vector_b))
