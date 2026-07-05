import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Any
from bridge import json_response, mcp


@mcp.tool()
def calc_product(
    ctx: Context,
    expression: str,
    variable: str = "x",
    lower: Any = 1,
    upper: Any = 10,
) -> list[TextContent]:
    """Evaluate the product of an expression over an index range.

    Args:
        expression: The factor, e.g. 'x'.
        variable: The index variable. Default 'x'.
        lower: Lower index bound. Default 1.
        upper: Upper index bound. Default 10.
    """
    return json_response(
        calcinator.product_sequence(expression, variable, lower, upper)
    )
