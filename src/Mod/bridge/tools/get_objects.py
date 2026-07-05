from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from bridge import (
    add_screenshot_if_available,
    get_parashell_connection,
    json_response,
    logger,
    mcp,
    state,
    text_response,
)


@mcp.tool()
def get_objects(ctx: Context, doc_name: str) -> list[TextContent | ImageContent]:
    """Get all objects in a Parashell document.

    Args:
        doc_name: The name of the document.
    """
    try:
        parashell = get_parashell_connection()
        screenshot = parashell.get_active_screenshot()
        response = json_response(parashell.get_objects(doc_name))
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"Failed to get objects: {str(e)}")
        return text_response(f"Failed to get objects: {str(e)}")
