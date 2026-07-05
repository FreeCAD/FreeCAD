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
def add_to_group(
    ctx: Context,
    doc_name: str,
    group: str,
    members: list[str],
) -> list[TextContent | ImageContent]:
    """Move existing objects into a group.

    Group and members accept either object names or take_snapshot uids.

    Args:
        doc_name: Document containing the objects.
        group: Group name or uid.
        members: List of object names / uids to move into the group.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.add_to_group(doc_name, group, members)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"add_to_group failed: {res.get('error')}")
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"add_to_group failed: {e}")
        return text_response(f"add_to_group failed: {e}")
