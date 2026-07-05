from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from typing import Any
from bridge import (
    add_screenshot_if_available,
    get_parashell_connection,
    logger,
    mcp,
    state,
    text_response,
)


@mcp.tool()
def edit_object(
    ctx: Context,
    doc_name: str,
    obj_name: str,
    obj_properties: dict[str, Any],
    transaction_id: str,
) -> list[TextContent | ImageContent]:
    """Edit an object in Parashell.

    Every model edit must run inside an open transaction. Pass the id from
    transaction_create; the change is rolled back by transaction_cancel or
    persisted by transaction_apply.

    Args:
        doc_name: The name of the document containing the object.
        obj_name: The name of the object to edit.
        obj_properties: The properties to update on the object.
        transaction_id: Id of the open transaction returned by transaction_create.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.edit_object(
            doc_name, obj_name, {"Properties": obj_properties}, transaction_id
        )
        screenshot = parashell.get_active_screenshot()
        if res["success"]:
            response = text_response(
                f"Object '{res['object_name']}' edited successfully"
            )
        else:
            response = text_response(f"Failed to edit object: {res['error']}")
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"Failed to edit object: {str(e)}")
        return text_response(f"Failed to edit object: {str(e)}")
