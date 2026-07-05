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
def create_pattern(
    ctx: Context,
    doc_name: str,
    source_object: str,
    kind: Literal["circular", "linear", "grid"],
    count: int,
    params: dict[str, Any] | None = None,
    copy_mode: Literal["duplicate", "link"] = "duplicate",
    name_prefix: str | None = None,
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Place N copies of a source object in a circular, linear, or grid pattern.

    Pattern kinds and their params:

      - "circular":
          {"center": [x, y, z],          default [0, 0, 0]
           "axis":   [x, y, z],          default [0, 0, 1]
           "total_angle_deg": 360,       360 = full circle, e.g. 90 for quarter sweep
           "full_circle": true,          when true, count divides 360 exactly
           "include_first": true}        when false, first instance is skipped

      - "linear":
          {"direction": [x, y, z],       default [1, 0, 0]
           "spacing": 50.0,              distance between adjacent copies in mm
           "length": 200.0}              alternative to 'spacing': total span

      - "grid":
          {"u_axis": [1, 0, 0],          row direction
           "v_axis": [0, 1, 0],          column direction
           "cols": 4,                    columns per row
           "rows": 3,                    number of rows
           "spacing_x": 50,
           "spacing_y": 50}              count is ignored when rows*cols is given

    copy_mode controls how the copies are produced:
      - "duplicate": creates an independent Part::Feature for each copy with a
        deep-copied shape. Best for permanent geometry like gear teeth, planet
        arms, sun rays.
      - "link": creates an App::Link to the source for each copy. Edits to the
        source propagate to every copy. Best for orbital tracks or instances
        that should share a master.

    Source can be referenced by name or take_snapshot uid.

    Args:
        doc_name: Document containing the source.
        source_object: Object name or uid.
        kind: "circular", "linear", or "grid".
        count: Number of copies. For circular full circles this becomes the
               division count.
        params: Pattern-specific params (see above).
        copy_mode: "duplicate" or "link".
        name_prefix: Optional prefix for new object names. Defaults to
                     "<source>_<kind>_".
        recompute: Whether to recompute after placement. Default True.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.create_pattern(
            doc_name,
            source_object,
            kind,
            count,
            params,
            copy_mode,
            name_prefix,
            recompute,
        )
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"create_pattern failed: {res.get('error')}")
        else:
            response = json_response(
                {
                    "source": res.get("source"),
                    "kind": res.get("kind"),
                    "count": res.get("count", 0),
                    "created": res.get("created", []),
                }
            )
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"create_pattern failed: {e}")
        return text_response(f"create_pattern failed: {e}")
