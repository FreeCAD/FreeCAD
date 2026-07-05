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
def make_hollow_cylinder(
    ctx: Context,
    doc_name: str,
    name: str,
    outer_radius: float,
    height: float,
    thickness: float,
    open_top: bool = True,
    open_bottom: bool = False,
    placement: dict[str, Any] | None = None,
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Create a hollow cylinder (outer cylinder minus inner cylinder) in one call.

    Replaces the manual outer-cylinder-minus-inner-cylinder pattern for wells,
    pipes, towers, silos. Inner radius = outer_radius - thickness.

    Args:
        doc_name: Document to add the cylinder to.
        name: Name for the new Part::Feature.
        outer_radius: Outer radius in mm.
        height: Height in mm.
        thickness: Wall thickness in mm. Must be < outer_radius.
        open_top: When True, the top is open. Default True (well-style).
        open_bottom: When True, the bottom is open.
        placement: Optional placement dict.
        recompute: Whether to recompute after creation. Default True.
    """
    params = {
        "outer_radius": outer_radius,
        "height": height,
        "thickness": thickness,
        "open_top": open_top,
        "open_bottom": open_bottom,
    }
    if placement is not None:
        params["placement"] = placement
    try:
        parashell = get_parashell_connection()
        res = parashell.make_hollow_cylinder(doc_name, name, params, recompute)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"make_hollow_cylinder failed: {res.get('error')}")
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"make_hollow_cylinder failed: {e}")
        return text_response(f"make_hollow_cylinder failed: {e}")
