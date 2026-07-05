from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Literal
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def merge_spreadsheet_cells(
    ctx: Context,
    doc_name: str,
    sheet_name: str,
    ranges: list[str] | str,
    action: Literal["merge", "split"] = "merge",
    recompute: bool = True,
) -> list[TextContent]:
    """Merge a rectangular range into one logical cell, or split a merged cell.

    Args:
        doc_name: Document containing the spreadsheet.
        sheet_name: Spreadsheet object name.
        ranges: For "merge", one or more ranges like "A1:C1". For "split", the
            top-left cell (or range) of each merged block to split.
        action: "merge" to combine cells, "split" to undo a merge. Default "merge".
        recompute: Recompute the document afterward. Default True.
    """
    try:
        res = get_parashell_connection().merge_spreadsheet_cells(
            doc_name, sheet_name, ranges, action, recompute
        )
        if not res.get("success"):
            return text_response(f"merge_spreadsheet_cells failed: {res.get('error')}")
        applied = res.get("ranges", [])
        return text_response(
            f"{action.capitalize()} applied to {len(applied)} range(s) in "
            f"'{sheet_name}': {', '.join(applied)}"
        )
    except Exception as e:
        logger.error(f"merge_spreadsheet_cells failed: {e}")
        return text_response(f"merge_spreadsheet_cells failed: {e}")
