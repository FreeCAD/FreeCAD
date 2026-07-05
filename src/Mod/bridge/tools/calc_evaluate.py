import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Any
from bridge import json_response, mcp


@mcp.tool()
def calc_evaluate(
    ctx: Context, expression: str, variables: dict[str, Any] | None = None
) -> list[TextContent]:
    """Numerically evaluate a scientific expression.

    Runs against a rich namespace: Python math/cmath, numpy (prefixed np_, e.g.
    np_linspace, np_mean), and the modules math, np, stats, optimize, linalg,
    special, constants, and sp. Use ** for powers and * for multiplication.

    Args:
        expression: The expression to evaluate, e.g. 'sin(pi/4)**2 + cos(pi/4)**2'
            or 'np_mean(np_array([1, 2, 3, 4]))'.
        variables: Optional mapping of names to numbers or lists bound before
            evaluation; lists become numpy arrays.
    """
    return json_response(calcinator.evaluate(expression, variables))
