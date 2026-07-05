from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
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
def clear_sketch_constraints(
    ctx: Context,
    doc_name: str,
    sketch_name: str,
    indices: list[int] | None = None,
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Remove constraints from a Sketcher::SketchObject.

    If `indices` is omitted or None, every constraint on the sketch is removed.
    Otherwise only the constraints at the listed indices are removed. Indices
    refer to the constraint list as returned by get_sketch (Constraints[i].index).

    Args:
        doc_name: Document containing the sketch.
        sketch_name: Sketcher::SketchObject name.
        indices: Specific constraint indices to remove. None / omitted clears all.
        recompute: Whether to recompute the sketch and document after deletion.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.clear_sketch_constraints(
            doc_name, sketch_name, indices, recompute
        )
        screenshot = parashell.get_sketch_view(doc_name, sketch_name)
        if res.get("success"):
            response = json_response(
                {
                    "deleted_indices": res.get("deleted_indices", []),
                    "constraint_count": res.get("constraint_count", 0),
                }
            )
        else:
            response = text_response(
                f"Failed to clear sketch constraints: {res.get('error')}"
            )
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"Failed to clear sketch constraints: {e}")
        return text_response(f"Failed to clear sketch constraints: {e}")
