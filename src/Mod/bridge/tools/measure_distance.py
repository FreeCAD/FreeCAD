from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def measure_distance(
    ctx: Context,
    doc_name: str,
    a,
    b,
) -> list[TextContent]:
    """Measure the distance between two points, objects, or subelements.

    Each of 'a' and 'b' can be:
      - An object name or uid (uses the shape's center of mass / bbox center)
      - A subelement reference like "Pad001#Vertex2", "Pad001#Edge5", or
        "Pad001#Face3" (uses Vertex.Point, Edge midpoint via CenterOfMass,
        or face center)
      - A literal point: [x, y, z] or {"x": .., "y": .., "z": ..}

    Returns the absolute distance plus per-axis distances (distance_x/y/z) and
    the resolved point coordinates so the math can be verified.

    Args:
        doc_name: Document containing referenced objects.
        a: First endpoint identifier or literal point.
        b: Second endpoint identifier or literal point.
    """
    try:
        res = get_parashell_connection().measure_distance(doc_name, a, b)
        if not res.get("success"):
            return text_response(f"measure_distance failed: {res.get('error')}")
        return json_response({k: v for k, v in res.items() if k != "success"})
    except Exception as e:
        logger.error(f"measure_distance failed: {e}")
        return text_response(f"measure_distance failed: {e}")
