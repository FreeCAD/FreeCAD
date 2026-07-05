import json
from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from bridge import (
    ToolResponse,
    add_screenshot_if_available,
    get_parashell_connection,
    logger,
    mcp,
    state,
    text_response,
)


@mcp.tool()
def recompute_document(
    ctx: Context,
    doc_name: str | None = None,
    force: bool = False,
    clear_redo: bool = False,
) -> list[TextContent | ImageContent]:
    """Recompute one or all open Parashell documents.

    Re-evaluates parametric features and dependency graphs. Useful after editing
    properties or geometry through execute_code or other tools where a recompute
    has not been triggered automatically.

    The response includes per-document health: how many objects were recomputed,
    how many remain unhealthy after the pass, and the full health record for each
    unhealthy object — including state flags, must_execute, and shape diagnostics
    (is_null, is_valid, is_closed, volume, area, vertex/edge/face/solid counts,
    and bounding box). Use check_objects for an on-demand scan that does not
    trigger a recompute.

    Args:
        doc_name: Document name to recompute. If omitted, recomputes every open document.
        force: Pass True to recompute every object regardless of touched state.
        clear_redo: Pass True to clear the document's undo/redo stack before recomputing.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.recompute_document(doc_name, force, clear_redo)
        screenshot = parashell.get_active_screenshot()
        if res.get("success"):
            results = res.get("results", [])
            total_recomputed = sum(int(r.get("objects_recomputed", 0)) for r in results)
            total_unhealthy = sum(int(r.get("unhealthy_count", 0)) for r in results)
            doc_summaries = ", ".join(
                f"{r.get('document', '?')}: {r.get('objects_recomputed', 0)} recomputed, "
                f"{r.get('unhealthy_count', 0)} unhealthy"
                for r in results
            )
            header = (
                f"Recompute complete across {len(results)} document(s). "
                f"Total objects recomputed: {total_recomputed}. "
                f"Total unhealthy objects: {total_unhealthy}. "
                f"[{doc_summaries}]"
            )
            response: ToolResponse = [
                TextContent(type="text", text=header),
                TextContent(
                    type="text",
                    text=json.dumps(
                        {"results": results}, ensure_ascii=False, indent=2, default=str
                    ),
                ),
            ]
        else:
            response = text_response(f"Failed to recompute: {res.get('error')}")
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"Failed to recompute: {e}")
        return text_response(f"Failed to recompute: {e}")
