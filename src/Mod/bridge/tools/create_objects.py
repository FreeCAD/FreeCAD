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
def create_objects(
    ctx: Context,
    doc_name: str,
    items: list[dict[str, Any]],
    recompute: bool = True,
) -> list[TextContent | ImageContent]:
    """Batch-create many objects in a single call.

    Each entry in 'items' has the same shape as create_object's parameters:
        {
          "Name": "Tier1",                  optional, auto-generated when omitted
          "Type": "Part::Box",              required, Parashell TypeId
          "Properties": {                   optional, applied via the same setter
              "Length": 50,
              "Width": 50,
              "Height": 10,
              "Placement": {"Base": {"x": 0, "y": 0, "z": 100}}
          },
          "Analysis": "MyFEMAnalysis"       optional FEM container
        }

    The whole list is processed inside a single GUI task, so the recompute happens
    once at the end (set recompute=False to defer entirely). Errors on individual
    items don't abort the batch — they are reported in the 'errors' array with the
    item index, attempted name, and message.

    Args:
        doc_name: Document to create objects in.
        items: List of object specs.
        recompute: Whether to recompute the document at the end. Default True.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.create_objects(doc_name, items, recompute)
        screenshot = parashell.get_active_screenshot()
        if not res.get("success"):
            response = text_response(f"Failed batch create: {res.get('error')}")
        else:
            response = json_response(
                {
                    "created_count": res.get("created_count", 0),
                    "error_count": res.get("error_count", 0),
                    "created": res.get("created", []),
                    "errors": res.get("errors", []),
                }
            )
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"create_objects failed: {e}")
        return text_response(f"create_objects failed: {e}")
