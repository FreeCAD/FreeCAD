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
def get_object(
    ctx: Context, doc_name: str, obj_name: str
) -> list[TextContent | ImageContent]:
    """Get a specific object and its properties from a Parashell document.

    Args:
        doc_name: The name of the document.
        obj_name: The name of the object.
    """
    try:
        parashell = get_parashell_connection()
        screenshot = parashell.get_active_screenshot()
        response = json_response(parashell.get_object(doc_name, obj_name))
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"Failed to get object: {str(e)}")
        return text_response(f"Failed to get object: {str(e)}")
