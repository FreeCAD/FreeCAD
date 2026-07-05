from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from typing import Any, Literal
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
def make_arch_opening(
    ctx: Context,
    doc_name: str,
    name: str,
    width: float,
    height: float,
    depth: float,
    arch_kind: Literal["round", "pointed", "flat"] = "round",
    arch_height: float | None = None,
    placement: dict[str, Any] | None = None,
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Create an arched opening solid (rectangular base + arched top) in one call.

    The shape is positioned with the bottom of the rectangular section at z=0,
    centered on x=0, and extending along Y by 'depth'. Subtract this from a
    wall (boolean_op operation="cut") to punch a doorway, gateway, or window.

    Arch kinds:
      - "round": semicircular arch — arch_height is fixed at width/2 and
        'arch_height' is ignored. Total opening height must exceed width/2.
      - "pointed": gothic-style isosceles triangle arch — arch_height defaults
        to width when omitted.
      - "flat": shallow segmental arch above a rectangle — arch_height defaults
        to width/4 when omitted.

    Args:
        doc_name: Document to add the opening solid to.
        name: Name for the new Part::Feature.
        width: Opening width in mm.
        height: Total opening height in mm (rectangle + arch combined).
        depth: Wall thickness the opening must pass through in mm.
        arch_kind: "round", "pointed", or "flat".
        arch_height: Override arch height. Ignored for round arches.
        placement: Optional placement dict.
        recompute: Whether to recompute after creation. Default True.
    """
    params: dict[str, Any] = {
        "width": width,
        "height": height,
        "depth": depth,
        "arch_kind": arch_kind,
    }
    if arch_height is not None:
        params["arch_height"] = arch_height
    if placement is not None:
        params["placement"] = placement
    try:
        parashell = get_parashell_connection()
        res = parashell.make_arch_opening(doc_name, name, params, recompute)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"make_arch_opening failed: {res.get('error')}")
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"make_arch_opening failed: {e}")
        return text_response(f"make_arch_opening failed: {e}")
