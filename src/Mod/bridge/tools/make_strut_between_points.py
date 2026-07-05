from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from typing import Literal
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
def make_strut_between_points(
    ctx: Context,
    doc_name: str,
    name: str,
    p1: list[float],
    p2: list[float],
    radius: float,
    end_caps: Literal["flat", "round"] = "flat",
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Create a cylindrical strut between two world-space points.

    Computes the direction vector and rotation automatically — replaces manual
    Vector / Rotation math for diagonal supports, frame elements, gantry struts.

    Args:
        doc_name: Document to add the strut to.
        name: Name for the new Part::Feature.
        p1: Start point [x, y, z] in mm.
        p2: End point [x, y, z] in mm.
        radius: Strut radius in mm.
        end_caps: "flat" (default) or "round" (hemispherical caps fused on each end).
        recompute: Whether to recompute after creation. Default True.
    """
    params = {"p1": p1, "p2": p2, "radius": radius, "end_caps": end_caps}
    try:
        parashell = get_parashell_connection()
        res = parashell.make_strut_between_points(doc_name, name, params, recompute)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(
                f"make_strut_between_points failed: {res.get('error')}"
            )
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"make_strut_between_points failed: {e}")
        return text_response(f"make_strut_between_points failed: {e}")
