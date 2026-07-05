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
def create_group(
    ctx: Context,
    doc_name: str,
    name: str,
    members: list[str] | None = None,
    parent: str | None = None,
) -> list[TextContent | ImageContent]:
    """Create an App::DocumentObjectGroup and optionally populate it.

    Groups are a lightweight container — useful for organizing a flat object
    list into Base / Column / PlanetSystem.Earth / etc. and for applying a
    single placement to a whole sub-assembly.

    Members and parent accept either object names or take_snapshot uids.

    Args:
        doc_name: Document to create the group in.
        name: Name for the new group.
        members: Optional list of objects to move into the group at creation.
        parent: Optional parent group/container the new group should live inside.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.create_group(doc_name, name, members or [], parent)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"create_group failed: {res.get('error')}")
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"create_group failed: {e}")
        return text_response(f"create_group failed: {e}")
