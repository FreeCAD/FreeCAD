from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def create_document(ctx: Context, name: str) -> list[TextContent]:
    """Create a new document in Parashell.

    Args:
        name: The name of the document to create.
    """
    try:
        res = get_parashell_connection().create_document(name)
        if res["success"]:
            return text_response(
                f"Document '{res['document_name']}' created successfully"
            )
        return text_response(f"Failed to create document: {res['error']}")
    except Exception as e:
        logger.error(f"Failed to create document: {str(e)}")
        return text_response(f"Failed to create document: {str(e)}")
