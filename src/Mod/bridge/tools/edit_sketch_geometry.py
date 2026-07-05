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
def edit_sketch_geometry(
    ctx: Context,
    doc_name: str,
    sketch_name: str,
    operation: Literal["append", "replace", "delete"] = "append",
    geometry: list[dict[str, Any]] | None = None,
    delete_indices: list[int] | None = None,
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Add, replace, or delete geometry entries in a Sketcher::SketchObject.

    Operations:
      - "append": add `geometry` entries to the sketch (default).
      - "replace": delete all existing geometry, then add `geometry` entries.
      - "delete": remove geometry entries listed in `delete_indices`.

    Geometry entry schema (each item in `geometry`):
      Common: { "type": <one of below>, "construction": bool (optional, default false) }

      - LineSegment / Line:
          { "type": "LineSegment", "start": [x, y] | {"x":..,"y":..,"z":..},
            "end":   [x, y] | {...} }
      - Circle:
          { "type": "Circle", "center": [x, y] | {...}, "radius": float,
            "normal": [x, y, z] (optional, default [0,0,1]) }
      - ArcOfCircle / Arc:
          { "type": "ArcOfCircle", "center": [...], "radius": float,
            "start_angle": float, "end_angle": float,
            "angle_unit": "deg" | "rad" (default "deg"),
            "normal": [...] (optional) }
      - Point:
          { "type": "Point", "position": [x, y] | {...} }
      - Ellipse:
          { "type": "Ellipse", "center": [...], "major_radius": float,
            "minor_radius": float,
            "major_axis_direction": [...] (optional, default [1,0,0]) }

    Args:
        doc_name: Document containing the sketch.
        sketch_name: Sketcher::SketchObject name to edit.
        operation: "append", "replace", or "delete".
        geometry: List of geometry entries to add (used by append/replace).
        delete_indices: Indices to remove (required for "delete").
        recompute: Whether to recompute the sketch and document after editing.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.edit_sketch_geometry(
            doc_name, sketch_name, operation, geometry, delete_indices, recompute
        )
        screenshot = parashell.get_sketch_view(doc_name, sketch_name)
        if res.get("success"):
            response = json_response(
                {
                    "operation": operation,
                    "added_indices": res.get("added_indices", []),
                    "deleted_indices": res.get("deleted_indices", []),
                    "geometry_count": res.get("geometry_count", 0),
                }
            )
        else:
            response = text_response(
                f"Failed to edit sketch geometry: {res.get('error')}"
            )
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"Failed to edit sketch geometry: {e}")
        return text_response(f"Failed to edit sketch geometry: {e}")
