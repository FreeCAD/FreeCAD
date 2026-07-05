from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from bridge import (
    add_screenshot_if_available,
    get_parashell_connection,
    logger,
    mcp,
    state,
    text_response,
)


@mcp.tool()
def delete_object(
    ctx: Context, doc_name: str, obj_name: str, transaction_id: str
) -> list[TextContent | ImageContent]:
    """Delete an object from a Parashell document.

    Every model edit must run inside an open transaction. Pass the id from
    transaction_create; deletion is reversible via transaction_cancel until
    transaction_apply persists it.

    Args:
        doc_name: The name of the document containing the object.
        obj_name: The name of the object to delete.
        transaction_id: Id of the open transaction returned by transaction_create.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.delete_object(doc_name, obj_name, transaction_id)
        screenshot = parashell.get_active_screenshot()
        if res["success"]:
            response = text_response(
                f"Object '{res['object_name']}' deleted successfully"
            )
        else:
            response = text_response(f"Failed to delete object: {res['error']}")
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"Failed to delete object: {str(e)}")
        return text_response(f"Failed to delete object: {str(e)}")
