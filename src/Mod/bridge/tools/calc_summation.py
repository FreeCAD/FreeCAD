import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Any
from bridge import json_response, mcp


@mcp.tool()
def calc_summation(
    ctx: Context,
    expression: str,
    variable: str = "x",
    lower: Any = 0,
    upper: Any = 10,
) -> list[TextContent]:
    """Evaluate the summation of an expression over an index range.

    Args:
        expression: The summand, e.g. 'x**2'.
        variable: The index variable. Default 'x'.
        lower: Lower index bound. Default 0.
        upper: Upper index bound; accepts 'oo' for infinite series. Default 10.
    """
    return json_response(calcinator.summation(expression, variable, lower, upper))
