import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Any
from bridge import json_response, mcp


@mcp.tool()
def calc_limit(
    ctx: Context,
    expression: str,
    variable: str = "x",
    point: Any = "0",
    direction: str = "+",
) -> list[TextContent]:
    """Compute the limit of an expression as a variable approaches a point.

    Args:
        expression: The expression, e.g. 'sin(x)/x'.
        variable: The variable that approaches the point. Default 'x'.
        point: Target value; accepts numbers or 'oo'/'-oo' for infinity. Default '0'.
        direction: Approach side, one of '+', '-', or '+-'. Default '+'.
    """
    return json_response(
        calcinator.limit_expression(expression, variable, point, direction)
    )
