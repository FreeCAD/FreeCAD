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
def make_gear(
    ctx: Context,
    doc_name: str,
    name: str,
    module: float,
    tooth_count: int,
    thickness: float,
    pressure_angle_deg: float = 20.0,
    hub_radius: float = 0.0,
    bore_radius: float = 0.0,
    helix_angle_deg: float = 0.0,
    placement: dict[str, Any] | None = None,
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Create an involute spur (or helical) gear in one call.

    Generates a true involute tooth profile, extrudes it to the requested
    thickness, optionally fuses an axial hub, and optionally cuts a central bore.
    Replaces hand-built gears made from boxes-as-teeth fused to a disc.

    Geometry:
      pitch_radius = module * tooth_count / 2
      addendum     = module
      dedendum     = 1.25 * module
      base_radius  = pitch_radius * cos(pressure_angle)

    Helical gears (helix_angle_deg != 0) are produced by lofting twisted copies
    of the tooth profile.

    Args:
        doc_name: Document to add the gear to.
        name: Name for the new Part::Feature.
        module: Gear module in mm. Pitch diameter = module * tooth_count.
        tooth_count: Number of teeth (>= 4).
        thickness: Axial gear thickness in mm.
        pressure_angle_deg: Standard 20° unless you need a custom value.
        hub_radius: Optional cylindrical hub fused to the gear face. 0 = no hub.
        bore_radius: Optional cylindrical bore through the center. 0 = no bore.
        helix_angle_deg: Helical twist angle. 0 = spur gear.
        placement: Optional placement dict.
        recompute: Whether to recompute after creation. Default True.
    """
    params = {
        "module": module,
        "tooth_count": tooth_count,
        "thickness": thickness,
        "pressure_angle_deg": pressure_angle_deg,
        "hub_radius": hub_radius,
        "bore_radius": bore_radius,
        "helix_angle_deg": helix_angle_deg,
    }
    if placement is not None:
        params["placement"] = placement
    try:
        parashell = get_parashell_connection()
        res = parashell.make_gear(doc_name, name, params, recompute)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"make_gear failed: {res.get('error')}")
        else:
            response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"make_gear failed: {e}")
        return text_response(f"make_gear failed: {e}")
