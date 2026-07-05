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
def remove_from_group(
    ctx: Context,
    doc_name: str,
    group: str,
    members: list[str],
) -> list[TextContent | ImageContent]:
    """Remove objects from a group without deleting them.

    Args:
        doc_name: Document containing the objects.
        group: Group name or uid.
        members: List of object names / uids to remove from the group.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.remove_from_group(doc_name, group, members)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"remove_from_group failed: {res.get('error')}")
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"remove_from_group failed: {e}")
        return text_response(f"remove_from_group failed: {e}")
