from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def clear_spreadsheet_cells(
    ctx: Context,
    doc_name: str,
    sheet_name: str,
    ranges: list[str] | str,
    recompute: bool = True,
) -> list[TextContent]:
    """Clear the content and style of cells (delete).

    Args:
        doc_name: Document containing the spreadsheet.
        sheet_name: Spreadsheet object name.
        ranges: A single cell ("B3"), a range ("A1:C10"), or a list of cells and
            ranges to clear.
        recompute: Recompute the document after clearing. Default True.
    """
    try:
        res = get_parashell_connection().clear_spreadsheet_cells(
            doc_name, sheet_name, ranges, recompute
        )
        if not res.get("success"):
            return text_response(f"clear_spreadsheet_cells failed: {res.get('error')}")
        cleared = res.get("cleared", [])
        return text_response(f"Cleared {len(cleared)} cell(s) in '{sheet_name}'.")
    except Exception as e:
        logger.error(f"clear_spreadsheet_cells failed: {e}")
        return text_response(f"clear_spreadsheet_cells failed: {e}")
