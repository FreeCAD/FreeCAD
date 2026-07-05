import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import json_response, mcp


@mcp.tool()
def calc_capabilities(ctx: Context) -> list[TextContent]:
    """Introspect the Calcinator scientific engine.

    Returns every available tool with its supported operations, the callable
    functions and constants reachable from calc_evaluate, and the installed
    numpy/scipy/sympy versions. Call this first to discover what is possible
    before invoking the other calc_* tools.
    """
    return json_response(calcinator.capabilities())
