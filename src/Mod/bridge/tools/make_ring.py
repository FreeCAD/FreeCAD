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
def make_ring(
    ctx: Context,
    doc_name: str,
    name: str,
    outer_radius: float,
    thickness: float,
    inner_radius: float = 0.0,
    tilt_deg: float = 0.0,
    placement: dict[str, Any] | None = None,
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Create a ring (or solid disc when inner_radius=0) in one call.

    Replaces the manual outer-cylinder-minus-inner-cylinder pattern. The optional
    tilt_deg rotates the ring about the X axis, useful for Saturn-style rings or
    tilted orbital tracks. Custom placements (translation + rotation) override
    the default flat XY orientation.

    Args:
        doc_name: Document to add the ring to.
        name: Name for the new Part::Feature.
        outer_radius: Outer ring radius (mm).
        thickness: Ring thickness along the local Z axis (mm).
        inner_radius: Inner radius (mm). 0 produces a solid disc.
        tilt_deg: Rotation about local X axis applied after construction.
        placement: Optional placement dict {"Base": {x,y,z}, "Rotation": {Axis: {x,y,z}, Angle: deg}}.
        recompute: Whether to recompute after creation. Default True.
    """
    params = {
        "outer_radius": outer_radius,
        "inner_radius": inner_radius,
        "thickness": thickness,
        "tilt_deg": tilt_deg,
    }
    if placement is not None:
        params["placement"] = placement
    try:
        parashell = get_parashell_connection()
        res = parashell.make_ring(doc_name, name, params, recompute)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"make_ring failed: {res.get('error')}")
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"make_ring failed: {e}")
        return text_response(f"make_ring failed: {e}")
