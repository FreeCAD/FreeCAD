from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def get_sketch(ctx: Context, doc_name: str, sketch_name: str) -> list[TextContent]:
    """Inspect a Sketcher::SketchObject and return a structured snapshot.

    Returns a JSON document containing:
      - Name, Label, TypeId, Placement
      - Attachment: { map_mode, support: [{object, subelement}, ...] }
      - GeometryCount, Geometry: [{ index, type, construction, ...type-specific fields }]
        Geometry types include LineSegment (start_point, end_point), Circle (center,
        radius, normal), ArcOfCircle (center, radius, first/last_parameter_rad+deg,
        start/end_point), Point (position), Ellipse, ArcOfEllipse, BSplineCurve.
      - ConstraintCount, Constraints: [{ index, type, name, first, first_pos_id,
        first_pos_name, second, second_pos_id, value, driving, active, ... }]
      - OpenVertices, Shape summary

    Position ids: 0=none/edge, 1=start, 2=end, 3=center.

    Args:
        doc_name: The name of the document containing the sketch.
        sketch_name: The Sketcher::SketchObject name to inspect.
    """
    try:
        res = get_parashell_connection().get_sketch(doc_name, sketch_name)
        if res.get("success"):
            return json_response(res["sketch"])
        return text_response(f"Failed to get sketch: {res.get('error')}")
    except Exception as e:
        logger.error(f"Failed to get sketch: {e}")
        return text_response(f"Failed to get sketch: {e}")
