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
def make_pyramid(
    ctx: Context,
    doc_name: str,
    name: str,
    base_radius: float | None = None,
    base_sides: int = 4,
    height: float | None = None,
    base_rotation_deg: float = 0.0,
    base_points: list[list[float]] | None = None,
    apex: list[float] | None = None,
    placement: dict[str, Any] | None = None,
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Create a pyramid solid in one call. Two modes:

    Regular polygon base (default):
        Pass base_radius (circumscribed-circle radius), base_sides (>=3),
        and height. The base sits on the XY plane centered at the origin,
        the apex is at (0, 0, height). 'base_rotation_deg' rotates the base
        polygon around its center for orientation tweaks. base_sides=4 gives
        a square pyramid (keep roof), base_sides=3 a tetrahedron-style spike,
        base_sides=8 an octagonal spire, and so on.

    Arbitrary polygon base:
        Pass base_points as a list of [x, y, z] points (any planar polygon)
        plus apex as a single [x, y, z] point. Useful for irregular roofs.

    Args:
        doc_name: Document to add the pyramid to.
        name: Name for the new Part::Feature.
        base_radius: Circumscribed-circle radius for the regular base.
        base_sides: Number of sides for the regular base (>=3). Default 4.
        height: Apex height above the regular base in mm.
        base_rotation_deg: Rotate the regular base around its center.
        base_points: Optional list of [x,y,z] points for an irregular base.
        apex: Apex point [x,y,z] when base_points is supplied.
        placement: Optional placement dict applied to the whole pyramid.
        recompute: Whether to recompute after creation. Default True.
    """
    params: dict[str, Any] = {
        "base_sides": base_sides,
        "base_rotation_deg": base_rotation_deg,
    }
    if base_radius is not None:
        params["base_radius"] = base_radius
    if height is not None:
        params["height"] = height
    if base_points:
        params["base_points"] = base_points
    if apex is not None:
        params["apex"] = apex
    if placement is not None:
        params["placement"] = placement
    try:
        parashell = get_parashell_connection()
        res = parashell.make_pyramid(doc_name, name, params, recompute)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"make_pyramid failed: {res.get('error')}")
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"make_pyramid failed: {e}")
        return text_response(f"make_pyramid failed: {e}")
