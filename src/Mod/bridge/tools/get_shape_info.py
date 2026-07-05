from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def get_shape_info(
    ctx: Context,
    doc_name: str,
    obj_name: str,
) -> list[TextContent]:
    """Return shape diagnostics for a single object without writing Python.

    Reports state flags, must_execute, and shape diagnostics: type, is_null, is_valid,
    is_closed, volume, area, vertex/edge/face/solid counts, and bounding box (with
    min/max plus side lengths). For Sketcher::SketchObject it also includes geometry
    and constraint counts plus map_mode. The 'issues' field summarizes any problems
    (state:Invalid, must_execute, shape:null, shape:invalid, shape:empty,
    shape:zero_volume) and 'healthy' is true when 'issues' is empty.

    Use this for one-off "is this Pad/Pocket/Boolean still producing valid geometry?"
    checks. For multi-object scans, use check_objects.

    Args:
        doc_name: Document containing the object.
        obj_name: Object name to inspect.
    """
    try:
        res = get_parashell_connection().get_shape_info(doc_name, obj_name)
        if res.get("success"):
            return json_response(res["object"])
        return text_response(f"Failed to get shape info: {res.get('error')}")
    except Exception as e:
        logger.error(f"Failed to get shape info: {e}")
        return text_response(f"Failed to get shape info: {e}")
