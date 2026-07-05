import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Any
from bridge import json_response, mcp


@mcp.tool()
def calc_series(
    ctx: Context,
    expression: str,
    variable: str = "x",
    point: Any = 0,
    order: int = 6,
) -> list[TextContent]:
    """Compute the Taylor or Laurent series of an expression.

    Args:
        expression: The expression to expand, e.g. 'exp(x)'.
        variable: The expansion variable. Default 'x'.
        point: The point to expand about. Default 0.
        order: The truncation order as a positive integer. Default 6.
    """
    return json_response(
        calcinator.series_expansion(expression, variable, point, order)
    )
