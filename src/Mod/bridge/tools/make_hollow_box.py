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
def make_hollow_box(
    ctx: Context,
    doc_name: str,
    name: str,
    length: float,
    width: float,
    height: float,
    thickness: float,
    open_top: bool = False,
    open_bottom: bool = False,
    placement: dict[str, Any] | None = None,
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Create a hollow box (outer minus inner) in one call.

    Replaces the manual outer-box-minus-inner-box pattern for curtain walls,
    moats, hollow plinths, container bodies. Outer dimensions are length × width
    × height; the cavity is shrunk by 'thickness' on each side.

    Args:
        doc_name: Document to add the hollow box to.
        name: Name for the new Part::Feature.
        length / width / height: Outer dimensions in mm.
        thickness: Wall thickness in mm. Must be < min(length, width)/2 and
                   (depending on open_top/bottom) < height/2.
        open_top: When True, the top face is removed (no ceiling).
        open_bottom: When True, the bottom face is removed (no floor).
        placement: Optional placement dict.
        recompute: Whether to recompute after creation. Default True.
    """
    params = {
        "length": length,
        "width": width,
        "height": height,
        "thickness": thickness,
        "open_top": open_top,
        "open_bottom": open_bottom,
    }
    if placement is not None:
        params["placement"] = placement
    try:
        parashell = get_parashell_connection()
        res = parashell.make_hollow_box(doc_name, name, params, recompute)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"make_hollow_box failed: {res.get('error')}")
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"make_hollow_box failed: {e}")
        return text_response(f"make_hollow_box failed: {e}")
