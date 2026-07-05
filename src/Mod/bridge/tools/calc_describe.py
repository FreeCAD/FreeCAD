import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import json_response, mcp


@mcp.tool()
def calc_describe(ctx: Context, name: str) -> list[TextContent]:
    """Describe a single function or constant in the calc_evaluate namespace.

    Args:
        name: Symbol to inspect (e.g. 'erf', 'np_linspace', 'pi'). Partial
            names return close matches so you can discover the exact spelling.
    """
    return json_response(calcinator.describe(name))
