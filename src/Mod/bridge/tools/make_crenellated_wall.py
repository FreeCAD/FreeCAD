from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from typing import Any
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
def make_crenellated_wall(
    ctx: Context,
    doc_name: str,
    name: str,
    length: float,
    thickness: float,
    height: float,
    merlon_width: float,
    gap_width: float,
    merlon_height: float,
    start_with_merlon: bool = True,
    placement: dict[str, Any] | None = None,
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Create a crenellated wall — base wall with merlons on top — in one call.

    Replaces hand-placed merlon loops for castle walls, fortifications, garden
    walls, parapets. The base wall has height (height - merlon_height); the
    merlons sit on top and step along the wall length with width 'merlon_width'
    separated by gaps of 'gap_width'.

    Args:
        doc_name: Document to add the wall to.
        name: Name for the new Part::Feature.
        length: Total wall length in mm.
        thickness: Wall thickness in mm.
        height: Total height including merlons in mm.
        merlon_width: Width of each merlon along the wall in mm.
        gap_width: Crenel (gap) width in mm.
        merlon_height: Height of merlons above the base wall in mm.
        start_with_merlon: When True, the wall begins with a merlon at x=0.
                           When False, it begins with a gap.
        placement: Optional placement dict.
        recompute: Whether to recompute after creation. Default True.
    """
    params = {
        "length": length,
        "thickness": thickness,
        "height": height,
        "merlon_width": merlon_width,
        "gap_width": gap_width,
        "merlon_height": merlon_height,
        "start_with_merlon": start_with_merlon,
    }
    if placement is not None:
        params["placement"] = placement
    try:
        parashell = get_parashell_connection()
        res = parashell.make_crenellated_wall(doc_name, name, params, recompute)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(
                f"make_crenellated_wall failed: {res.get('error')}"
            )
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"make_crenellated_wall failed: {e}")
        return text_response(f"make_crenellated_wall failed: {e}")
