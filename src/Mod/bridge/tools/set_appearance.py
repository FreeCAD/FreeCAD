from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Any
from bridge import get_parashell_connection, json_response, logger, mcp, text_response


@mcp.tool()
def set_appearance(
    ctx: Context,
    doc_name: str,
    targets: list[str],
    properties: dict[str, Any],
) -> list[TextContent]:
    """Apply a single set of ViewObject properties to many objects at once.

    Replaces the per-object setattr loop for ShapeColor / Transparency /
    Visibility / DisplayMode. Targets accept object names or take_snapshot uids.

    Recognised properties:
      - ShapeColor / LineColor / PointColor: [r, g, b] or [r, g, b, a] in [0, 1].
      - Transparency: int 0..100.
      - LineWidth: float.
      - PointSize: float.
      - Visibility: bool.
      - DisplayMode: "Shaded", "Wireframe", "Flat Lines", "Hidden line", "Points", "As is".
      - Any other ViewObject property exposed by the underlying object — set
        verbatim if it exists, otherwise reported in 'errors'.

    Args:
        doc_name: Document containing the targets.
        targets: List of object names or uids.
        properties: Dict of view properties to apply to every target.
    """
    try:
        res = get_parashell_connection().set_appearance(doc_name, targets, properties)
        if not res.get("success"):
            return text_response(f"set_appearance failed: {res.get('error')}")
        return json_response(
            {
                "applied": res.get("applied", []),
                "missing": res.get("missing", []),
                "errors": res.get("errors", []),
            }
        )
    except Exception as e:
        logger.error(f"set_appearance failed: {e}")
        return text_response(f"set_appearance failed: {e}")
