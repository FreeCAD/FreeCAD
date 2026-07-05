from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def get_bounding_box(
    ctx: Context,
    doc_name: str,
    target: str,
) -> list[TextContent]:
    """Return the bounding box of an object or subelement.

    Target accepts a plain object name, a take_snapshot uid (obj_/sub_), or a
    subelement reference like "Pad001#Face3". Reports min/max in each axis,
    side lengths (x_length / y_length / z_length), center, and 3D diagonal.

    Args:
        doc_name: Document containing the target.
        target: Object name, uid, or "Object#Face3"-style reference.
    """
    try:
        res = get_parashell_connection().get_bounding_box(doc_name, target)
        if not res.get("success"):
            return text_response(f"get_bounding_box failed: {res.get('error')}")
        return json_response(
            {
                "object": res.get("object"),
                "subelement": res.get("subelement", ""),
                "bbox": res.get("bbox"),
            }
        )
    except Exception as e:
        logger.error(f"get_bounding_box failed: {e}")
        return text_response(f"get_bounding_box failed: {e}")
