import json
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def check_objects(
    ctx: Context,
    doc_name: str,
    obj_names: list[str] | None = None,
    only_unhealthy: bool = False,
) -> list[TextContent]:
    """Inspect the validity of one or more objects in a Parashell document.

    Runs a non-destructive health scan (no recompute). For each object reports:
      - state flags (Parashell's internal State list, e.g. Touched, Invalid, Restore Error)
      - must_execute (whether the object still requires re-execution)
      - shape diagnostics (when a Shape exists): type, is_null, is_valid, is_closed,
        volume, area, vertex_count, edge_count, face_count, solid_count, bbox
      - sketch diagnostics (for Sketcher::SketchObject): geometry/constraint counts,
        fully_constrained, open_vertex_count, map_mode
      - issues: a list of short tokens describing why the object is unhealthy
        (state:Invalid, must_execute, shape:null, shape:invalid, shape:empty,
        shape:zero_volume)
      - healthy: convenience boolean (true when issues is empty)

    Use this after a sketch swap, attachment edit, or feature parameter change to
    confirm dependent features (Pad, Pocket, Boolean, Fillet, etc.) still produce
    valid geometry. Pair with recompute_document when you need to force the
    re-evaluation first.

    Args:
        doc_name: Document to inspect.
        obj_names: Specific objects to check. If omitted, checks every object in the document.
        only_unhealthy: When True, the returned 'objects' list is filtered to entries
                        with at least one issue. The unhealthy_count and missing_objects
                        fields are still reported regardless.
    """
    try:
        res = get_parashell_connection().check_objects(
            doc_name, obj_names, only_unhealthy
        )
        if not res.get("success"):
            return text_response(f"Failed to check objects: {res.get('error')}")

        unhealthy_count = int(res.get("unhealthy_count", 0))
        checked_count = int(res.get("checked_count", 0))
        object_count = int(res.get("object_count", 0))
        missing = res.get("missing_objects", []) or []
        missing_part = f" Missing: {missing}." if missing else ""
        verdict = (
            "All checked objects are healthy."
            if unhealthy_count == 0
            else f"{unhealthy_count} unhealthy object(s) detected."
        )

        header = (
            f"Health scan of '{res.get('document')}': checked {checked_count} of "
            f"{object_count} object(s). {verdict}{missing_part}"
        )
        payload = {
            "document": res.get("document"),
            "object_count": object_count,
            "checked_count": checked_count,
            "unhealthy_count": unhealthy_count,
            "missing_objects": missing,
            "objects": res.get("objects", []),
        }
        return [
            TextContent(type="text", text=header),
            TextContent(
                type="text",
                text=json.dumps(payload, ensure_ascii=False, indent=2, default=str),
            ),
        ]
    except Exception as e:
        logger.error(f"Failed to check objects: {e}")
        return text_response(f"Failed to check objects: {e}")
