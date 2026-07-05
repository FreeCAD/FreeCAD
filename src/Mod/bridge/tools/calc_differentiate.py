import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import json_response, mcp


@mcp.tool()
def calc_differentiate(
    ctx: Context, expression: str, variable: str = "x", order: int = 1
) -> list[TextContent]:
    """Compute the symbolic derivative of an expression.

    Args:
        expression: The expression to differentiate, e.g. 'sin(x)*exp(x)'.
        variable: The differentiation variable. Default 'x'.
        order: The derivative order as a positive integer. Default 1.
    """
    return json_response(calcinator.differentiate(expression, variable, order))
