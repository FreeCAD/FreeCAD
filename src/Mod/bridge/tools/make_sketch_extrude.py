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
def make_sketch_extrude(
    ctx: Context,
    doc_name: str,
    name: str,
    profile: list[dict[str, Any]],
    depth: float,
    direction: list[float] | None = None,
    placement: dict[str, Any] | None = None,
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Build a 2D profile from line/arc segments and extrude it into a solid.

    The profile is a list of geometry dicts using the same schema as
    edit_sketch_geometry. Edges must form a closed loop. Common entries:

      LineSegment:
        {"type": "LineSegment", "start": [x, y], "end": [x, y]}

      ArcOfCircle:
        {"type": "ArcOfCircle", "center": [x, y], "radius": r,
         "start_angle": deg_or_rad, "end_angle": deg_or_rad,
         "angle_unit": "deg" | "rad"}

      Other supported types: Circle (closed-loop only), Point, Ellipse.

    Use this for arrow slits, pointed-arch windows, gothic shapes, custom
    door cutouts, irregular plate outlines — anything that's hard to express
    as a chain of boolean primitives.

    Args:
        doc_name: Document to add the extrusion to.
        name: Name for the new Part::Feature.
        profile: List of geometry dicts forming a closed wire on the XY plane.
        depth: Extrusion depth in mm. Positive = +Z when 'direction' is omitted.
        direction: Optional [dx, dy, dz] extrusion direction. Defaults to +Z.
        placement: Optional placement applied after extrusion (translate/rotate
                   the whole solid).
        recompute: Whether to recompute after creation. Default True.
    """
    params: dict[str, Any] = {
        "profile": profile,
        "depth": depth,
    }
    if direction is not None:
        params["direction"] = direction
    if placement is not None:
        params["placement"] = placement
    try:
        parashell = get_parashell_connection()
        res = parashell.make_sketch_extrude(doc_name, name, params, recompute)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"make_sketch_extrude failed: {res.get('error')}")
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"make_sketch_extrude failed: {e}")
        return text_response(f"make_sketch_extrude failed: {e}")
