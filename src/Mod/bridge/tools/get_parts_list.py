from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, mcp, text_response


@mcp.tool()
def get_parts_list(ctx: Context) -> list[TextContent]:
    """Get the list of parts available in the Parashell parts library addon."""
    parts = get_parashell_connection().get_parts_list()
    if parts:
        return json_response(parts)
    return text_response(
        "No parts found in the parts library. You must add parts_library addon."
    )
