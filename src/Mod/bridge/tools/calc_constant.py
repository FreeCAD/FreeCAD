import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import json_response, mcp


@mcp.tool()
def calc_constant(ctx: Context, name: str) -> list[TextContent]:
    """Look up a physical or mathematical constant.

    Args:
        name: The constant name, e.g. 'pi', 'golden', 'speed of light in vacuum',
            or any key from scipy.constants.physical_constants. Partial names
            return suggestions.
    """
    return json_response(calcinator.constant(name))
