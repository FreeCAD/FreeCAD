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
def insert_part_from_library(
    ctx: Context, relative_path: str, transaction_id: str
) -> list[TextContent | ImageContent]:
    """Insert a part from the Parashell parts library addon.

    Every model edit must run inside an open transaction. Pass the id from
    transaction_create; persist with transaction_apply or roll back with
    transaction_cancel.

    Args:
        relative_path: Relative path of the part within the parts library directory.
        transaction_id: Id of the open transaction returned by transaction_create.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.insert_part_from_library(relative_path, transaction_id)
        screenshot = parashell.get_active_screenshot()
        if res["success"]:
            response = text_response(f"Part inserted from library: {res['message']}")
        else:
            response = text_response(
                f"Failed to insert part from library: {res['error']}"
            )
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"Failed to insert part from library: {str(e)}")
        return text_response(f"Failed to insert part from library: {str(e)}")
