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
def make_segment_between_points(
    ctx: Context,
    doc_name: str,
    name: str,
    p1: list[float],
    p2: list[float],
    cross_section: Literal["circular", "box"] = "circular",
    radius: float | None = None,
    width: float | None = None,
    height: float | None = None,
    end_caps: Literal["flat", "round"] = "flat",
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Create a segment (cylinder or box) between two world-space points.

    The required cross-section dimensions depend on 'cross_section':
      - "circular": pass 'radius'. Optional 'end_caps="round"' fuses
        hemispherical caps on each tip.
      - "box": pass 'width' (and optional 'height'; defaults to width for square cross-section).
        The box is centered on the segment axis.

    Direction vector and rotation are computed automatically — replaces manual
    Vector / Rotation cross-product math for diagonals like gatehouse chains,
    diagonal struts, frame elements, support cables, gantry rails.

    Args:
        doc_name: Document to add the segment to.
        name: Name for the new Part::Feature.
        p1: Start point [x, y, z] in mm.
        p2: End point [x, y, z] in mm.
        cross_section: "circular" or "box". Default circular.
        radius: Segment radius for circular cross-section.
        width: Box width for box cross-section.
        height: Box height for box cross-section. Defaults to width.
        end_caps: For circular cross-section only — "flat" or "round".
        recompute: Whether to recompute after creation. Default True.
    """
    params: dict[str, Any] = {
        "p1": p1,
        "p2": p2,
        "cross_section": cross_section,
        "end_caps": end_caps,
    }
    if radius is not None:
        params["radius"] = radius
    if width is not None:
        params["width"] = width
    if height is not None:
        params["height"] = height
    try:
        parashell = get_parashell_connection()
        res = parashell.make_segment_between_points(doc_name, name, params, recompute)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(
                f"make_segment_between_points failed: {res.get('error')}"
            )
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"make_segment_between_points failed: {e}")
        return text_response(f"make_segment_between_points failed: {e}")
