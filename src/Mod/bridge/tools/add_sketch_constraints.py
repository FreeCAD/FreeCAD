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
def add_sketch_constraints(
    ctx: Context,
    doc_name: str,
    sketch_name: str,
    constraints: list[dict[str, Any]],
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Add one or more constraints to a Sketcher::SketchObject.

    Position ids accept ints (0=none/edge, 1=start, 2=end, 3=center) or names
    ("none", "start", "end", "center"). Geometry indices are sketch geometry ids
    as returned by get_sketch (use -1 / -2 for the sketch X / Y axes, -3 for origin).

    Constraint schemas (one entry per constraint, "type" is required, "name" is optional):

      - Coincident:    { "type": "Coincident", "first": id, "first_pos": pos,
                         "second": id, "second_pos": pos }
      - Horizontal:    { "type": "Horizontal", "first": geom_id }
                       or two-point form with first/first_pos/second/second_pos
      - Vertical:      same shape as Horizontal
      - Equal:         { "type": "Equal", "first": id, "second": id }
      - Parallel:      { "type": "Parallel", "first": id, "second": id }
      - Perpendicular: { "type": "Perpendicular", "first": id, "second": id }
      - Tangent:       { "type": "Tangent", "first": id, "second": id }
                       optional: first_pos / second_pos for endpoint tangency
      - Distance:      { "type": "Distance", "first": id, "value": mm }
                       or point-to-point: first/first_pos/second/second_pos/value
                       or point-to-edge:  first/first_pos/value
      - DistanceX / DistanceY: same shapes as Distance
      - Radius:        { "type": "Radius", "first": id, "value": mm }
      - Diameter:      { "type": "Diameter", "first": id, "value": mm }
      - Angle:         { "type": "Angle", "first": id, "second": id, "value": deg,
                         "angle_unit": "deg" | "rad" (default "deg") }
      - PointOnObject: { "type": "PointOnObject", "first": id, "first_pos": pos,
                         "second": edge_id }
      - Symmetric:     { "type": "Symmetric", "first": id, "first_pos": pos,
                         "second": id, "second_pos": pos,
                         "third": id, "third_pos": pos (optional, line form omits it) }
      - Block:         { "type": "Block", "first": id }

    All constraints accept an optional "driving": false to make the constraint
    a reference (only meaningful for dimensional constraints).

    Args:
        doc_name: Document containing the sketch.
        sketch_name: Sketcher::SketchObject name to constrain.
        constraints: List of constraint entry dicts.
        recompute: Whether to recompute the sketch and document after adding.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.add_sketch_constraints(
            doc_name, sketch_name, constraints, recompute
        )
        screenshot = parashell.get_sketch_view(doc_name, sketch_name)
        if res.get("success"):
            response = json_response(
                {
                    "added_indices": res.get("added_indices", []),
                    "constraint_count": res.get("constraint_count", 0),
                }
            )
        else:
            response = text_response(
                f"Failed to add sketch constraints: {res.get('error')}"
            )
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"Failed to add sketch constraints: {e}")
        return text_response(f"Failed to add sketch constraints: {e}")
