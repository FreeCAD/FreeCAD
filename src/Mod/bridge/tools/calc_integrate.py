import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Any
from bridge import json_response, mcp


@mcp.tool()
def calc_integrate(
    ctx: Context,
    expression: str,
    variable: str = "x",
    lower: Any = None,
    upper: Any = None,
) -> list[TextContent]:
    """Compute a definite or indefinite integral symbolically.

    Args:
        expression: The integrand, e.g. 'x**2' or '1/x'.
        variable: The integration variable. Default 'x'.
        lower: Lower bound for a definite integral. Omit for an indefinite one.
        upper: Upper bound for a definite integral. Omit for an indefinite one.
    """
    return json_response(
        calcinator.integrate_expression(expression, variable, lower, upper)
    )
