from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, mcp


@mcp.tool()
def list_documents(ctx: Context) -> list[TextContent]:
    """Get the list of currently open documents in Parashell."""
    return json_response(get_parashell_connection().list_documents())
