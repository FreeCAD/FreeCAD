from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, mcp


@mcp.tool()
def get_selection(ctx: Context) -> list[TextContent]:
    """Get the objects the user currently has selected in the Parashell viewport.

    Returns each selected object with its document, internal name, label, TypeId,
    and any selected sub-elements (such as Face1, Edge3, Vertex2). Use this to
    resolve what the user means by "this", "that", or "the selected" object.
    """
    return json_response(get_parashell_connection().get_selection())
