import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import json_response, mcp


@mcp.tool()
def calc_transform(
    ctx: Context, expression: str, operation: str = "simplify"
) -> list[TextContent]:
    """Rewrite an expression with a symbolic transformation.

    Args:
        expression: The expression to transform, e.g. '(x + 1)**2'.
        operation: One of simplify, expand, factor, cancel, apart, together,
            trigsimp, expand_trig, radsimp, ratsimp, powsimp, logcombine,
            expand_log, nsimplify. Default 'simplify'.
    """
    return json_response(calcinator.transform_expression(expression, operation))
